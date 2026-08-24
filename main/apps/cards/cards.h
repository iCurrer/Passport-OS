// main/apps/cards/cards.h —— Passport OS V2 第 3 页 CARDS / QR(个人链接)页。
// 由 app_router 通过页面表调用。enter/exit 须在 LVGL 锁内。
#pragma once

#include "app_pages.h"

#ifdef __cplusplus
extern "C" {
#endif

// 构建并加载 CARDS 屏幕(动态生成二维码)。
void cards_enter(app_page_t page);

// 销毁待退出的 CARDS 屏幕(释放二维码位图缓冲)。
void cards_exit(void);

#ifdef __cplusplus
}
#endif