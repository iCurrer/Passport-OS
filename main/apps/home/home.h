// main/apps/home/home.h —— Passport OS V2 HOME(身份)页。
// 由 app_router 通过页面表调用。enter 须在 LVGL 锁内(router 已持锁)。
#pragma once

#include "app_pages.h"

#ifdef __cplusplus
extern "C" {
#endif

// 构建并加载 HOME 屏幕。page 参数供绘制 Footer 指示器定位。
void home_enter(app_page_t page);

// 销毁待退出的 HOME 屏幕(先于 router 记录新页)。
void home_exit(void);

#ifdef __cplusplus
}
#endif