// main/badge.h —— 磨砂黑「心情电子名牌」界面。
// 开机直入本界面:顶部导航栏 + 左侧形象 + 右侧姓名/职位/状态 + 底部 dock。
// 姓名/顶部文字/职位/状态均存 NVS,可通过 BLE 自定义(见 badge_update_text)。
#pragma once

#include "bsp_button.h"

// 可自定义字段(与 NVS key 一一对应)
typedef enum {
    BADGE_FIELD_NAME = 0,   // 姓名
    BADGE_FIELD_TOP,        // 顶部导航栏文字
    BADGE_FIELD_TITLE,      // 职位
    BADGE_FIELD_STATUS,     // 状态
    BADGE_FIELD_MAX,
} badge_field_t;

// 名牌界面初始化并进入(幂等)。须在 LVGL 锁内调用。
void badge_enter(void);

// 按键分发。可从 button 组件任务调用;内部自行管理 LVGL 锁与深度睡眠。
void badge_key(bsp_btn_t btn, bsp_btn_ev_t ev);

// 更新一个可自定义字段:写 NVS + 刷新 UI。可从任意任务调用(内部加 LVGL 锁)。
void badge_update_text(badge_field_t field, const char *s);

// 读取一个字段当前值(供 BLE 读特性返回)。返回指向内部静态缓冲的指针。
const char *badge_get_text(badge_field_t field);
