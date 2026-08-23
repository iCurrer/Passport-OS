// main/badge/badge_ui.h —— 名牌界面(LVGL 布局与渲染)。
#pragma once

#include "badge.h"

// 构建并载入名牌界面(须在 LVGL 锁内调用)。字段初值取自 badge_data。
void badge_ui_init(void);

// 刷新单个字段对应的标签并重排位置(内部加 LVGL 锁,可从 BLE 任务调用)。
void badge_ui_set_field(badge_field_t field);

// 底部 dock 选中项切换(上/下)。
void badge_ui_dock_prev(void);
void badge_ui_dock_next(void);

// 进入当前 dock 选中项对应的功能(如游戏)。
void badge_ui_dock_enter(void);