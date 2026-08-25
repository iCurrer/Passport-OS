// main/badge/badge_data.c —— 名牌可自定义字段的存储模型 + NVS 持久化。
// 只负责"字段值"这一层:内存缓冲 + NVS 读写,不碰 UI、不碰硬件。
#include "badge_data.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// 六个可自定义字段的 NVS key(与 badge.h 的 badge_field_t 顺序一致)
#define NVS_NS       "badge"
#define NVS_KEY_NAME   "name"
#define NVS_KEY_TOP    "top"
#define NVS_KEY_TITLE  "title"
#define NVS_KEY_STATUS "status"
#define NVS_KEY_BIO    "bio"
#define NVS_KEY_QR     "qr"

static nvs_handle_t s_nvs;

static char s_name[32]   = "李秋实";
static char s_top[24]    = "FoloToy";     // 顶部导航栏文字
static char s_title[32]  = "豆包大学";     // 职位
static char s_status[16] = "自由";        // 状态
static char s_bio[48]    = "";            // 简介
static char s_qr[256]    = "";            // 二维码内容(微信/网址等;只用于生成二维码)

// 读 NVS 字符串;为空或不存在则写入默认值。
static void nvs_load_str(const char *key, char *buf, size_t size, const char *def)
{
    size_t len = size;
    if (nvs_get_str(s_nvs, key, buf, &len) != ESP_OK || buf[0] == '\0') {
        strncpy(buf, def, size - 1);
        buf[size - 1] = 0;
        // 默认值为空时不做无意义回写,避免每次开机都写 NVS
        if (def[0] != '\0') {
            nvs_set_str(s_nvs, key, buf);
            nvs_commit(s_nvs);
        }
    }
}

void badge_data_init(void)
{
    if (s_nvs) return;

    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    if (nvs_open(NVS_NS, NVS_READWRITE, &s_nvs) != ESP_OK) { s_nvs = 0; return; }

    // 六个可自定义字段:默认值,预留 BLE 手机修改。
    nvs_load_str(NVS_KEY_NAME,   s_name,   sizeof(s_name),   "张三");
    nvs_load_str(NVS_KEY_TOP,    s_top,    sizeof(s_top),    "FoloToy");
    nvs_load_str(NVS_KEY_TITLE,  s_title,  sizeof(s_title),  "豆包大学");
    nvs_load_str(NVS_KEY_STATUS, s_status, sizeof(s_status), "自由");
    nvs_load_str(NVS_KEY_BIO,    s_bio,    sizeof(s_bio),    "");
    nvs_load_str(NVS_KEY_QR,     s_qr,     sizeof(s_qr),     "");
}

const char *badge_data_get(badge_field_t field)
{
    switch (field) {
    case BADGE_FIELD_NAME:   return s_name;
    case BADGE_FIELD_TOP:    return s_top;
    case BADGE_FIELD_TITLE:  return s_title;
    case BADGE_FIELD_STATUS: return s_status;
    case BADGE_FIELD_BIO:    return s_bio;
    case BADGE_FIELD_QR:     return s_qr;
    default:                 return "";
    }
}

void badge_data_set(badge_field_t field, const char *s)
{
    if (!s) return;

    char *buf = NULL; size_t size = 0; const char *key = NULL;
    switch (field) {
    case BADGE_FIELD_NAME:   buf = s_name;   size = sizeof(s_name);   key = NVS_KEY_NAME;   break;
    case BADGE_FIELD_TOP:    buf = s_top;    size = sizeof(s_top);    key = NVS_KEY_TOP;    break;
    case BADGE_FIELD_TITLE:  buf = s_title;  size = sizeof(s_title);  key = NVS_KEY_TITLE;  break;
    case BADGE_FIELD_STATUS: buf = s_status; size = sizeof(s_status); key = NVS_KEY_STATUS; break;
    case BADGE_FIELD_BIO:    buf = s_bio;    size = sizeof(s_bio);    key = NVS_KEY_BIO;    break;
    case BADGE_FIELD_QR:     buf = s_qr;     size = sizeof(s_qr);     key = NVS_KEY_QR;     break;
    default:                 return;
    }

    strncpy(buf, s, size - 1);
    buf[size - 1] = 0;

    // 持久化到 NVS
    if (s_nvs) {
        nvs_set_str(s_nvs, key, buf);
        nvs_commit(s_nvs);
    }
}

void badge_data_commit(void)
{
    if (s_nvs) nvs_commit(s_nvs);
}