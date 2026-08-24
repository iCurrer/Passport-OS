// main/avatar/avatar_storage.c —— 头像文件存储实现(SPIFFS)。
#include "avatar_storage.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "avatar";
#define AVATAR_PATH    "/spiffs/avatar.bin"
#define SPIFFS_BASE    "/spiffs"
#define SPIFFS_LABEL   "storage"

static bool s_mounted;

esp_err_t avatar_storage_init(void)
{
    if (s_mounted) return ESP_OK;

    esp_vfs_spiffs_conf_t conf = {
        .base_path            = SPIFFS_BASE,
        .partition_label      = SPIFFS_LABEL,
        .max_files            = 4,
        .format_if_mount_failed = true,   // 首次/损坏时自动格式化
    };
    esp_err_t r = esp_vfs_spiffs_register(&conf);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS 挂载失败: %s", esp_err_to_name(r));
        return r;
    }
    s_mounted = true;
    ESP_LOGI(TAG, "SPIFFS %s 已挂载", SPIFFS_BASE);
    return ESP_OK;
}

bool avatar_storage_has(void)
{
    if (!s_mounted) return false;
    FILE *f = fopen(AVATAR_PATH, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

esp_err_t avatar_storage_save(const uint8_t *data, size_t size)
{
    if (!s_mounted) return ESP_ERR_INVALID_STATE;
    FILE *f = fopen(AVATAR_PATH, "wb");
    if (!f) return ESP_ERR_NOT_FOUND;
    size_t w = fwrite(data, 1, size, f);
    fclose(f);
    if (w != size) { ESP_LOGE(TAG, "头像写入失败 %u/%u", (unsigned)w, (unsigned)size); return ESP_FAIL; }
    ESP_LOGI(TAG, "头像已保存 %u B", (unsigned)size);
    return ESP_OK;
}

ssize_t avatar_storage_load(uint8_t *buf, size_t size)
{
    if (!s_mounted) return -1;
    FILE *f = fopen(AVATAR_PATH, "rb");
    if (!f) return -1;
    size_t r = fread(buf, 1, size, f);
    fclose(f);
    return (ssize_t)r;
}