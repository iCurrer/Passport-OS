// main/apps/dashboard/dashboard.h —— Passport OS V2 第 4 页 DASHBOARD(仪表盘)页。
// 由 app_router 通过页面表调用。enter/exit 须在 LVGL 锁内。
#pragma once

#include "app_pages.h"

#ifdef __cplusplus
extern "C" {
#endif

// 构建并加载 DASHBOARD 屏幕(含每秒刷新定时器)。
void dashboard_enter(app_page_t page);

// 销毁待退出的 DASHBOARD 屏幕(先停定时器再删对象)。
void dashboard_exit(void);

#ifdef __cplusplus
}
#endif