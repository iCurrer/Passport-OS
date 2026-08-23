// main/badge/badge_power.h —— 名牌电源管理:自动关机计时 + 深度睡眠 + 按键唤醒。
#pragma once

#include <stdbool.h>
#include <stdint.h>

// 初始化活动计时与自动关机定时器;记录开机时刻。幂等。
void badge_power_init(void);

// 按键活动标记。返回 false 表示正处于开机忽略期,调用方应直接返回。
bool badge_power_key_activity(void);

// 非按键活动标记(如 BLE 写入):无条件刷新最近活动时间。
void badge_power_activity(void);

// 设置/获取自动休眠超时(秒)。0=永不自动休眠。设置侧持久化到 NVS。
void badge_power_set_timeout(uint32_t seconds);
uint32_t badge_power_get_timeout(void);