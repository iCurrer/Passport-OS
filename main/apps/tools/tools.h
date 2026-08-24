// main/apps/tools/tools.h —— Passport OS V2 第 5 页 TOOLS(工具,纵向列表)页。
// 由 app_router 通过页面表调用。enter/exit 须在 LVGL 锁内;key 由 router 持锁回调。
#pragma once

#include "app_pages.h"
#include "bsp_button.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 构建并加载 TOOLS 列表屏幕。
void tools_enter(app_page_t page);

// 销毁待退出的 TOOLS 屏幕(含已进入的工具子屏/停止计时器)。
void tools_exit(void);

// TOOLS 页局部按键(短按):列表内 UP/DOWN 选择、OK 进入工具;工具内 OK 操作。
// 须在 LVGL 锁内调用(router 持锁)。返回 true 表示已消费(不再走全局翻页)。
bool tools_key(bsp_btn_t btn, bsp_btn_ev_t ev);

#ifdef __cplusplus
}
#endif