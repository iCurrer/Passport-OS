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

// 页面标题(英文,符合 UI 文字规范)。与 app_pages.h 顺序一致。
static const char *s_titles[APP_PAGE_COUNT] = {
    "HOME", "PROFILE", "STATUS", "CARDS",
    "DASHBOARD", "TOOLS", "GAMES", "SETTINGS",
};

// 占位页(未实现页面)状态
static lv_obj_t   *s_ph_scr;     // 占位页根 screen
static ds_dots_t   s_ph_dots;    // 占位页 Page Indicator
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
// 占位页渲染(Header + 居中页码/标题 + Footer 指示器)。须持 LVGL 锁。
// 各页 TASK 实现后,在 s_pages 表里把对应格子换成真实页的 enter/exit。
// ============================================================================
static void app_ph_enter(app_page_t page)
{
    if (page < 0 || page >= APP_PAGE_COUNT) return;

    s_ph_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_ph_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_ph_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_ph_scr, 0, 0);
    lv_obj_set_style_pad_all(s_ph_scr, 0, 0);

    // Header:页标题 + 电量 + 分隔线
    ds_header(s_ph_scr, s_titles[page], bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    // 占位主体:PAGE n/8 + 标题(后续真实页面替换)
    char buf[32];
    snprintf(buf, sizeof(buf), "PAGE %d / %d\n%s",
             (int)page + 1, APP_PAGE_COUNT, s_titles[page]);
    lv_obj_t *lbl = lv_label_create(s_ph_scr);
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_font(lbl, &badge_font_gb2312, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(DS_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl);
    lv_obj_set_y(lbl, 120);

    // Footer:Page Indicator
    ds_footer(s_ph_scr, &s_ph_dots, APP_PAGE_COUNT, page);

    lv_screen_load(s_ph_scr);
}

static void app_ph_exit(void)
{
    if (s_ph_scr) { lv_obj_delete(s_ph_scr); s_ph_scr = NULL; }
    // s_ph_dots 引用的圆点对象随 screen 一并删除,不再使用
}

// ============================================================================
// 页面渲染表:每页一个 build/destroy。HOME 用真实渲染,其余页暂用占位。
// 后续各页面 TASK 逐个把占位格替换为对应 apps/<page> 的 enter/exit。
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
    { app_ph_enter,     app_ph_exit,     NULL,         NULL },          // SETTINGS
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

    // 短按:若当前页有局部按键处理,优先交给它;消费则返回(如 TOOLS 列表选择)。
    if (ev == BSP_BTN_CLICK && s_pages[s_cur].key) {
        if (!bsp_lvgl_lock(300)) return;
        bool handled = s_pages[s_cur].key(btn, ev);
        bsp_lvgl_unlock();
        if (handled) return;
    }

    // 其余(长按 + 未被页面消费的短按)→ 全局映射
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
    s_ph_scr = NULL;
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