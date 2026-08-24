// main/apps/settings/settings_page.h —— Passport OS V2 第 7 页 SETTINGS(设置)页。
// 由 app_router 通过页面表调用。enter/exit 须在 LVGL 锁内;key 由 router 持锁回调。
// 命名 settings_page 以免与旧 main/settings 模块冲突。
#pragma once

#include "app_pages.h"
#include "bsp_button.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 构建并加载 SETTINGS 列表屏幕。
void settings_page_enter(app_page_t page);

// 销毁待退出的 SETTINGS 屏幕。
void settings_page_exit(void);

// SETTINGS 页局部按键(短按):UP/DOWN 选择、OK 开关/循环。返回 true=已消费。
bool settings_page_key(bsp_btn_t btn, bsp_btn_ev_t ev);

#ifdef __cplusplus
}
#endif