// main/app/app_router.c —— Passport OS V2 全局页面路由实现。
//
// 职责:
//   1. 维护当前页索引,提供 UP/DOWN 循环翻页(HOME↔SETTINGS 双向环绕)。
//   2. 统一解析全局按键(CLICK=短按 / LONG=长按)并映射为路由意图。
//   3. 页面渲染走 ds_* design-system(TASK-01):占位页先展示 Header+居中页码+Page Indicator,
//      后续各 TASK 用真实页面渲染替换。
//
// 线程规则:
//   - 所有 lv_* 访问在 bsp_lvgl_lock/unlock 内。
//   - 按键来自 button 组件任务,本文件统一加锁,页面局部操作也在此锁内执行。
//   - 电源活跃计时/自动休眠复用 badge_power(原由 badge_enter 负责的启动初值在此补上)。
#include "app_router.h"
#include "home.h"             // HOME 页真实渲染
#include "profile.h"          // PROFILE 页真实渲染
#include "status.h"           // STATUS 页真实渲染 + status_cycle(快速状态切换)
#include "cards.h"            // CARDS/QR 页真实渲染
#include "dashboard.h"        // DASHBOARD 页真实渲染
#include "tools.h"            // TOOLS 页真实渲染 + 页内按键(tools_key)
#include "games.h"            // GAMES 页真实渲染 + 页内按键(games_key)
#include "settings_page.h"    // SETTINGS 页真实渲染 + 页内按键(settings_page_key)
#include "badge_data.h"       // badge_data_init / get(名称/职位/状态/顶部文字)
#include "avatar_storage.h"   // avatar_storage_init(SPIFFS 挂载 + 头像文件)
#include "badge_power.h"      // badge_power_init / badge_power_key_activity(休眠计时)
#include "bsp_display.h"      // bsp_lvgl_lock / unlock
#include "bsp_battery.h"      // bsp_battery_soc(Header 电量)
#include "badge_fonts.h"      // badge_font_gb2312 / _small(中英文显示)
#include "ds_tokens.h"        // DS_* 颜色与骨架
#include "ds_widgets.h"       // ds_header / ds_footer
#include "lvgl.h"
#include "esp_log.h"
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

static const char *TAG = "app_router";

static app_page_t  s_cur = APP_PAGE_HOME;

// ============================================================================
// 纯导航逻辑(host 可单测)
// ============================================================================
app_page_t app_router_page_cycle(app_page_t cur, int dir)
{
    if ((int)cur < 0 || cur >= APP_PAGE_COUNT) cur = APP_PAGE_HOME;  // 入参钳位
    int n = ((int)cur + dir + APP_PAGE_COUNT) % APP_PAGE_COUNT;
    return (app_page_t)n;
}

// ============================================================================
// 页面渲染表:每页一个 build/destroy。全部 8 页使用各页真实渲染(apps/*)。
// ============================================================================
typedef struct {
    void (*build)(app_page_t page);               // 构建并加载该页屏幕(router 已持锁)
    void (*destroy)(void);                        // 销毁该页屏幕
    void (*action)(void);                         // OK 短按"进入/操作当前页"(可 NULL);须持锁
    bool (*key)(bsp_btn_t, bsp_btn_ev_t);         // 页内局部按键(短按优先;返回 true=已消费);须持锁
} app_page_render_t;

static const app_page_render_t s_pages[APP_PAGE_COUNT] = {
    { home_enter,       home_exit,       NULL,         NULL },          // HOME
    { profile_enter,    profile_exit,    NULL,         NULL },          // PROFILE
    { status_enter,     status_exit,     status_cycle, NULL },          // STATUS
    { cards_enter,      cards_exit,      NULL,         NULL },          // CARDS
    { dashboard_enter,  dashboard_exit,  NULL,         NULL },          // DASHBOARD
    { tools_enter,      tools_exit,      NULL,         tools_key },     // TOOLS
    { games_enter,      games_exit,      NULL,         games_key },     // GAMES
    { settings_page_enter, settings_page_exit, NULL, settings_page_key }, // SETTINGS
};

// 切换页:退出旧页 → 记录新页 → 渲染新页。须持 LVGL 锁。
static void goto_page(app_page_t page)
{
    if (page >= APP_PAGE_COUNT) page = APP_PAGE_HOME;
    s_pages[s_cur].destroy();
    s_cur = page;
    s_pages[page].build(page);
}

// ============================================================================
// 按键 → 路由意图(纯映射,host 可单测)
// ============================================================================
typedef enum {
    APP_INTENT_NONE = 0,
    APP_INTENT_PREV,           // 上一页
    APP_INTENT_NEXT,           // 下一页
    APP_INTENT_OK_ACTION,      // 进入/操作当前页
    APP_INTENT_HOME,           // 回 HOME
    APP_INTENT_STATUS_TOGGLE,  // 快速状态切换
} app_intent_t;

static app_intent_t map_event(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (ev == BSP_BTN_CLICK) {
        switch (btn) {
        case BSP_BTN_UP:   return APP_INTENT_PREV;
        case BSP_BTN_DOWN: return APP_INTENT_NEXT;
        case BSP_BTN_OK:   return APP_INTENT_OK_ACTION;
        default: break;
        }
    } else if (ev == BSP_BTN_LONG) {
        switch (btn) {
        case BSP_BTN_UP:   return APP_INTENT_HOME;
        case BSP_BTN_OK:   return APP_INTENT_HOME;
        case BSP_BTN_DOWN: return APP_INTENT_STATUS_TOGGLE;
        default: break;
        }
    }
    return APP_INTENT_NONE;
}

void app_router_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (!badge_power_key_activity()) return;   // 开机忽略期:吞掉

    // 短按/长按:若当前页有局部按键处理,优先交给它。列表页(TOOLS/GAMES/SETTINGS)用
    // app_list 控制器消费:长按 OK 在 ENTRY 态不消费(交全局回首页),在 MENU/CHILD 态
    // "返回上一级";UP/DOWN 长按不消费则交全局(回 HOME / 快速状态切换)。
    if ((ev == BSP_BTN_CLICK || ev == BSP_BTN_LONG) && s_pages[s_cur].key) {
        if (!bsp_lvgl_lock(300)) return;
        bool handled = s_pages[s_cur].key(btn, ev);
        bsp_lvgl_unlock();
        if (handled) return;
    }

    // 其余(未被页面消费的短按/长按)→ 全局映射
    app_intent_t it = map_event(btn, ev);
    if (it == APP_INTENT_NONE) return;

    if (!bsp_lvgl_lock(300)) return;
    switch (it) {
    case APP_INTENT_PREV:
        goto_page(app_router_page_cycle(s_cur, -1));
        break;
    case APP_INTENT_NEXT:
        goto_page(app_router_page_cycle(s_cur, +1));
        break;
    case APP_INTENT_HOME:
        if (s_cur != APP_PAGE_HOME) goto_page(APP_PAGE_HOME);
        break;
    case APP_INTENT_OK_ACTION:
        // 进入/操作当前页:转发给该页的 action 回调(STATUS 页=status_cycle;其余页暂无)。
        if (s_pages[s_cur].action) s_pages[s_cur].action();
        break;
    case APP_INTENT_STATUS_TOGGLE:
        // 快速状态切换(DOWN 长按):全局切下一档状态并写 NVS;STATUS 页在场时同步刷新。
        status_cycle();
        break;
    default: break;
    }
    bsp_lvgl_unlock();
}

void app_router_goto(app_page_t page)
{
    if (page >= APP_PAGE_COUNT) page = APP_PAGE_HOME;
    if (!bsp_lvgl_lock(300)) return;
    goto_page(page);
    bsp_lvgl_unlock();
}

void app_router_init(void)
{
    badge_data_init();           // 载入动态字段(NVS):名称/职位/状态/顶部文字(原由 badge_enter 负责)
    avatar_storage_init();       // 挂载 SPIFFS,头像文件 /avatar.bin
    badge_power_init();          // 启动自动休眠计时 + 复位诊断(原由 badge_enter 负责)
    s_cur = APP_PAGE_HOME;
    ESP_LOGI(TAG, "router ready, pages=%d", APP_PAGE_COUNT);
}

void app_router_enter(void)
{
    goto_page(s_cur);            // 须在 LVGL 锁内(main 调用时已持锁)
}

app_page_t app_router_current(void)
{
    return s_cur;
}