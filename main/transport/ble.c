// main/ble.c —— 名牌 BLE 服务(ESP-IDF NimBLE)。
// 广播名:FoloToy-Badge。GATT 服务含 6 个可写/可读特性:
//   name / top / title / status / bio / qr(二维码内容)
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
#include "avatar_storage.h"
#include "esp_crc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "ble";

// 16 位 UUID(安卓端用 128 位形式)
#define UUID_SVC     0xFFE0
#define UUID_NAME    0xFFE1
#define UUID_TOP     0xFFE2
#define UUID_TITLE   0xFFE3
#define UUID_STAT    0xFFE4
#define UUID_BIO     0xFFE5
#define UUID_QR      0xFFE7   // 二维码内容(微信/网址等;只用于生成二维码,不展示文本)
#define UUID_AV_CTRL 0xFFE8   // 头像控制(命令:START size crc / CANCEL)
#define UUID_AV_DATA 0xFFE9   // 头像数据(分块写入)

// 头像单次写入接收缓冲上限(防止滥用内存)
#define AV_MAX_SIZE   (64 * 1024)

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
        // 提取写入的字符串(可能跨 mbuf)。256 容纳最长字段缓冲(qr=256)。
        char buf[256];
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

// ============================================================================
// 头像上传(分块 + CRC32 校验)
// ============================================================================
static uint8_t  *s_av_buf;
static uint32_t  s_av_size;
static uint32_t  s_av_off;
static uint32_t  s_av_crc;
static bool      s_av_active;

static void avatar_abort(void)
{
    if (s_av_buf) { free(s_av_buf); s_av_buf = NULL; }
    s_av_active = false;
    s_av_size = s_av_off = s_av_crc = 0;
}

// 头像控制特性:写 "START <size> <crc32>" 开始;写 "CANCEL" 中止。
static int on_av_ctrl(uint16_t conn_handle, uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle; (void)attr_handle; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return 0;

    char cmd[32];
    uint16_t len = 0;
    if (ble_hs_mbuf_to_flat(ctxt->om, cmd, sizeof(cmd) - 1, &len) != 0) return 0;
    cmd[len] = 0;

    if (strncmp(cmd, "CANCEL", 6) == 0) {
        avatar_abort();
        ESP_LOGI(TAG, "头像传输已中止");
        return 0;
    }
    if (strncmp(cmd, "START", 5) == 0) {
        uint32_t size = 0, crc = 0;
        if (sscanf(cmd, "START %lu %lu", &size, &crc) != 2) return 0;
        if (size == 0 || size > AV_MAX_SIZE) { ESP_LOGE(TAG, "头像尺寸非法 %lu", (unsigned long)size); return 0; }
        avatar_abort();
        s_av_buf = malloc(size);
        if (!s_av_buf) { ESP_LOGE(TAG, "头像缓冲分配失败 %lu B", (unsigned long)size); return 0; }
        s_av_size = size;
        s_av_crc = crc;
        s_av_off = 0;
        s_av_active = true;
        ESP_LOGI(TAG, "头像传输开始 size=%lu crc=%lu", (unsigned long)size, (unsigned long)crc);
        return 0;
    }
    return 0;
}

// 头像数据特性:分块写入,收满后校验 CRC 并存盘。
static int on_av_data(uint16_t conn_handle, uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle; (void)attr_handle; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return 0;
    if (!s_av_active) return 0;

    uint8_t buf[244];
    uint16_t len = 0;
    if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &len) != 0) return 0;
    if (len == 0) return 0;

    uint32_t n = (uint32_t)len;
    if (s_av_off + n > s_av_size) n = s_av_size - s_av_off;
    memcpy(s_av_buf + s_av_off, buf, n);
    s_av_off += n;

    if (s_av_off >= s_av_size) {
        uint32_t calc = esp_crc32_le(0, s_av_buf, s_av_size);
        if (calc == s_av_crc) {
            avatar_storage_save(s_av_buf, s_av_size);
            ESP_LOGI(TAG, "头像校验通过并保存");
        } else {
            ESP_LOGE(TAG, "头像 CRC 校验失败: 收到 %08lx 期望 %08lx",
                     (unsigned long)calc, (unsigned long)s_av_crc);
        }
        avatar_abort();
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
            { .uuid = BLE_UUID16_DECLARE(UUID_QR),
              .access_cb = on_char_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
              .arg = (void *)(intptr_t)BADGE_FIELD_QR },
            { .uuid = BLE_UUID16_DECLARE(UUID_AV_CTRL),
              .access_cb = on_av_ctrl, .flags = BLE_GATT_CHR_F_WRITE, .arg = NULL },
            { .uuid = BLE_UUID16_DECLARE(UUID_AV_DATA),
              .access_cb = on_av_data, .flags = BLE_GATT_CHR_F_WRITE, .arg = NULL },
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
    // 从 NVS 读取上次保存的蓝牙开关状态(默认开启);SETTINGS 页可开关并持久化。
    nvs_handle_t nvs;
    if (nvs_open(NVS_NS_CFG, NVS_READONLY, &nvs) == ESP_OK) {
        uint8_t v = 1;
        if (nvs_get_u8(nvs, NVS_KEY_BLE, &v) == ESP_OK) {
            s_ble_enabled = (v != 0);
        }
        nvs_close(nvs);
    }
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
