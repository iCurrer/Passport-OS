// main/apps/status/status.h —— Passport OS V2 第 2 页 STATUS(状态)页。
// 由 app_router 通过页面表调用。enter/exit 须在 LVGL 锁内。
#pragma once

#include "app_pages.h"

#ifdef __cplusplus
extern "C" {
#endif

// 构建并加载 STATUS 屏幕。
void status_enter(app_page_t page);

// 销毁待退出的 STATUS 屏幕。
void status_exit(void);

// 把当前状态循环切到下一档并写 NVS,并刷新 STATUS 页(若在场)。
// 须在 LVGL 锁内调用(由 router 持锁调用);STATUS 页不在场时仅更新 NVS。
void status_cycle(void);

#ifdef __cplusplus
}
#endif