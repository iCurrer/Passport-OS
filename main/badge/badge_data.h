// main/badge/badge_data.h —— 名牌可自定义字段的存储模型 + NVS 持久化。
#pragma once

#include "badge.h"

// 初始化:打开 NVS 并载入四个字段(缺失则写默认值)。幂等。
void badge_data_init(void);

// 读一个字段当前值(返回内部静态缓冲,不可修改)。
const char *badge_data_get(badge_field_t field);

// 写一个字段:更新内存并持久化到 NVS。不涉及 UI。
void badge_data_set(badge_field_t field, const char *s);

// 立即提交 NVS(深度睡眠前调用,避免丢写入)。
void badge_data_commit(void);