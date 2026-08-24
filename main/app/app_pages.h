// main/app/app_pages.h —— Passport OS V2 页面模型（8 页，plan §4）。
// 页面 id 与数组下标一一对应；页面顺序即翻页顺序（UP/DOWN 循环）。
#pragma once

typedef enum {
    APP_PAGE_HOME = 0,
    APP_PAGE_PROFILE,
    APP_PAGE_STATUS,
    APP_PAGE_CARDS,
    APP_PAGE_DASHBOARD,
    APP_PAGE_TOOLS,
    APP_PAGE_GAMES,
    APP_PAGE_SETTINGS,
    APP_PAGE_COUNT,   // 8
} app_page_t;