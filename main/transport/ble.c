// main/ble.c —— 名牌 BLE 服务(ESP-IDF NimBLE)。
// 广播名:FoloToy-Badge。GATT 服务含 4 个可写/可读特性:
//   name / top / title / status,对应姓名/顶部文字/职位/状态。
// 写入时更新 NVS 并刷新 LVGL 界面(见 badge_update_text)。
// 安卓端使用对应 128 位 UUID: 0000FFEx-0000-1000-8000-00805F9B34FB。
#include "esp_log.h"
#include "nvs_flash.h"
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
#define UUID_SVC   0xFFE0
#define UUID_NAME  0xFFE1
#define UUID_TOP   0xFFE2
#define UUID_TITLE 0xFFE3
#define UUID_STAT  0xFFE4

#define DEVICE_NAME "FoloToy-Badge"

static void start_advertising(void);

// 特性访问回调:arg 指向 badge_field_t
static int on_char_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    badge_field_t field = (badge_field_t)(intptr_t)arg;

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        // 提取写入的字符串(可能跨 mbuf)
        char buf[40];
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
    ESP_LOGI(TAG, "BLE 已初始化并开始广播");
}
