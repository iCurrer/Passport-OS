// main/avatar/avatar_storage.h —— Passport OS V2 头像文件存储(SPIFFS /avatar.bin)。
#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// 头像规格:80x80 RGB565(每像素 2 字节)。
#define AVATAR_W        80
#define AVATAR_H        80
#define AVATAR_SIZE     (AVATAR_W * AVATAR_H * 2)   // 12,800 字节

// 挂载 SPIFFS(/spiffs),失败自动格式化。幂等。
esp_err_t avatar_storage_init(void);

// /avatar.bin 是否存在。
bool avatar_storage_has(void);

// 保存头像数据到 /avatar.bin。
esp_err_t avatar_storage_save(const uint8_t *data, size_t size);

// 读取头像到 buf(size 至少 AVATAR_SIZE)。返回读取字节数,失败返回 -1。
ssize_t avatar_storage_load(uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif