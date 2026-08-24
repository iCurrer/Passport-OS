// main/apps/profile/profile.h —— Passport OS V2 PROFILE(完整档案)页。
// 由 app_router 通过页面表调用。enter 须在 LVGL 锁内(router 已持锁)。
#pragma once

#include "app_pages.h"

#ifdef __cplusplus
extern "C" {
#endif

// 构建并加载 PROFILE 屏幕。
void profile_enter(app_page_t page);

// 销毁待退出的 PROFILE 屏幕。
void profile_exit(void);

#ifdef __cplusplus
}
#endif