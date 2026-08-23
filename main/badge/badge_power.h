// main/badge/badge_power.h —— 名牌电源管理:自动关机计时 + 深度睡眠 + 按键唤醒。
#pragma once

#include <stdbool.h>

// 初始化活动计时与自动关机定时器;记录开机时刻。幂等。
void badge_power_init(void);

// 按键活动标记。返回 false 表示正处于开机忽略期,调用方应直接返回。
bool badge_power_key_activity(void);

// 非按键活动标记(如 BLE 写入):无条件刷新最近活动时间。
void badge_power_activity(void);