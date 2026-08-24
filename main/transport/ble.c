// main/ble.c —— 名牌 BLE 服务(ESP-IDF NimBLE)。
// 广播名:FoloToy-Badge。GATT 服务含 7 个可写/可读特性:
//   name / top / title / status / bio / website / github
// 写入时更新 NVS 并刷新 LVGL 界面(见 badge_update_text)。
// 安卓端使用对应 128 位 UUID: 0000FFEx-0000-1000-8000-00805F9B34FB。
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "badge.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "ble";

// 16 位 UUID(安卓端用 128 位形式)
#define UUID_SVC     0xFFE0
#define UUID_NAME    0xFFE1
#define UUID_TOP     0xFFE2
#define UUID_TITLE   0xFFE3
#define UUID_STAT    0xFFE4
#define UUID_BIO     0xFFE5
#define UUID_WEBSITE 0xFFE6
#define UUID_GITHUB  0xFFE7

#define DEVICE_NAME "FoloToy-Badge"

// BLE 开关状态(默认开启),持久化到 NVS (与 badge_power 共用 badge_cfg 命名空间)
#define NVS_NS_CFG    "badge_cfg"
#define NVS_KEY_BLE   "ble_on"

static bool s_ble_enabled = true;

static void start_advertising(void);

// 特性访问回调:arg 指向 badge_field_t
static int on_char_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    badge_field_t field = (badge_field_t)(intptr_t)arg;

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        // 提取写入的字符串(可能跨 mbuf)。64 容纳最长字段缓冲(bio/website/github=48)
        char buf[64];
        uint16_t len = 0;
        int r = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf) - 1, &len);
        if (r == 0) {
            buf[len] = 0;
            ESP_LOGI(TAG, "写特性 field=%d len=%d: %s", field, len, buf);
            badge_update_text(field, buf);
        }
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        const char *val = badge_get_text(field);
        int r = os_mbuf_append(ctxt->om, val, (uint16_t)strlen(val));
        return r == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(UUID_SVC),
        .characteristics = (struct ble_gatt_chr_def[]){
            { .uuid = BLE_UUID16_DECLARE(UUID_NAME),
              .access_cb = on_char_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
              .arg = (void *)(intptr_t)BADGE_FIELD_NAME },
            { .uuid = BLE_UUID16_DECLARE(UUID_TOP),
              .access_cb = on_char_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
              .arg = (void *)(intptr_t)BADGE_FIELD_TOP },
            { .uuid = BLE_UUID16_DECLARE(UUID_TITLE),
              .access_cb = on_char_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
              .arg = (void *)(intptr_t)BADGE_FIELD_TITLE },
            { .uuid = BLE_UUID16_DECLARE(UUID_STAT),
              .access_cb = on_char_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
              .arg = (void *)(intptr_t)BADGE_FIELD_STATUS },
            { .uuid = BLE_UUID16_DECLARE(UUID_BIO),
              .access_cb = on_char_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
              .arg = (void *)(intptr_t)BADGE_FIELD_BIO },
            { .uuid = BLE_UUID16_DECLARE(UUID_WEBSITE),
              .access_cb = on_char_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
              .arg = (void *)(intptr_t)BADGE_FIELD_WEBSITE },
            { .uuid = BLE_UUID16_DECLARE(UUID_GITHUB),
              .access_cb = on_char_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
              .arg = (void *)(intptr_t)BADGE_FIELD_GITHUB },
            { 0 },
        },
    },
    { 0 },
};

// GAP 事件:断开/广播结束都重新广播
static int gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status != 0) {       // 连接失败
            ESP_LOGI(TAG, "连接失败,重新广播");
            start_advertising();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "已断开,重新广播");
        start_advertising();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        start_advertising();
        break;
    default:
        break;
    }
    return 0;
}

static void start_advertising(void)
{
    if (!s_ble_enabled) return;   // 蓝牙关闭时不广播

    struct ble_gap_adv_params adv_params = { 0 };
    struct ble_hs_adv_fields fields = { 0 };

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;   // 可连接
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;   // 可发现
    adv_params.itvl_min = 1600;                     // 广播间隔 1000ms(0.625ms单位),降低广播功耗
    adv_params.itvl_max = 1600;
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                      &adv_params, gap_event, NULL);
}

static void on_sync(void) { start_advertising(); }
static void on_reset(int reason) { ESP_LOGW(TAG, "BLE 复位 reason=%d", reason); }

static void host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_init(void)
{
    // ⚠ 临时开发/联调覆盖:当前无设置页开关(TASK-13 才实现 BLE 开关),故开机强制开启 BLE,
    //   以便手机连接测试。最终 V2 低功耗行为为"BLE 默认关 + 设置页开启,同步完即关并深睡",
    //   届时恢复读取 NVS ble_on(并配合 TASK-13 的开关 UI)。
    s_ble_enabled = true;
    ESP_LOGI(TAG, "BLE 开关状态=%s", s_ble_enabled ? "开" : "关");

    if (nimble_port_init() != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE 初始化失败");
        return;
    }

    ble_svc_gap_device_name_set(DEVICE_NAME);
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    nimble_port_freertos_init(host_task);
    ESP_LOGI(TAG, "BLE 已初始化%s", s_ble_enabled ? "并开始广播" : "(蓝牙关闭,不广播)");
}

// 设置开关状态并写入 NVS
static void ble_set_enabled(bool en)
{
    s_ble_enabled = en;
    nvs_handle_t nvs;
    if (nvs_open(NVS_NS_CFG, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u8(nvs, NVS_KEY_BLE, en ? 1 : 0);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

void ble_stop(void)
{
    ble_set_enabled(false);
    int rc = ble_gap_adv_stop();
    ESP_LOGI(TAG, "BLE 广播已停止 rc=%d", rc);
}

void ble_restart(void)
{
    ble_set_enabled(true);
    start_advertising();
    ESP_LOGI(TAG, "BLE 广播已重新开始");
}

bool ble_is_enabled(void)
{
    return s_ble_enabled;
}
