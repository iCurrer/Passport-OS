// main/apps/tools/tools.c —— Passport OS V2 第 5 页 TOOLS(工具)页。
//
// 交互(TASK-08 起):由共享 app_list 控制器驱动三态导航。
//   ENTRY 入口态:标题"TOOLS" + 简介 + "OK TO ENTER";UP/DOWN 短按交全局翻页。
//   MENU 菜单态: TIMER / STOPWATCH / CALCULATOR / MORSE,UP/DOWN 选择、OK 进入;
//               长按 OK 返回入口。
//   CHILD 子屏: STOPWATCH 秒表(OK 启动/停止);其余 COMING SOON 占位;长按 OK 返回菜单。
// 子屏的具体构建/按键由本页回调提供,状态机与按键复用 app_list,不重复实现。
#include "tools.h"
#include "app_list.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stddef.h>
#include <stdio.h>

static const char *TAG = "tools";

// 工具列表
#define TOOLS_COUNT 4
enum { TOOL_TIMER = 0, TOOL_STOPWATCH, TOOL_CALCULATOR, TOOL_MORSE };
static const char *s_tool_names[TOOLS_COUNT] = {
    "TIMER", "STOPWATCH", "CALCULATOR", "MORSE",
};

// 秒表布局
#define SW_TIME_Y    100
#define SW_HINT_Y    152

// 子屏自身状态(秒表)
static lv_obj_t   *s_sw_scr;        // 秒表屏
static ds_dots_t   s_sw_dots;
static lv_obj_t   *s_sw_lbl;        // 秒表时间
static lv_timer_t *s_sw_timer;
static bool        s_sw_running;
static int64_t     s_sw_accum_us;   // 已累计
static int64_t     s_sw_start_us;   // 当前运行段起点

static app_list_ctl_t *s_ctl;

// ---------------------------------------------------------------------------
// 秒表工具(子屏)
// ---------------------------------------------------------------------------
static void sw_tick(lv_timer_t *t)
{
    (void)t;
    if (!s_sw_scr || !s_sw_lbl) return;
    int64_t now = esp_timer_get_time();
    int64_t el = s_sw_accum_us + (s_sw_running ? (now - s_sw_start_us) : 0);
    int64_t es = el / 1000000;
    char b[24];
    snprintf(b, sizeof(b), "%02d:%02d", (int)(es / 60), (int)(es % 60));
    lv_label_set_text(s_sw_lbl, b);
}

static void sw_toggle(void)
{
    if (s_sw_running) {
        s_sw_accum_us += esp_timer_get_time() - s_sw_start_us;
        s_sw_running = false;
    } else {
        s_sw_start_us = esp_timer_get_time();
        s_sw_running = true;
    }
    sw_tick(NULL);   // 立即刷新
}

static void build_stopwatch_screen(void)
{
    s_sw_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_sw_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_sw_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_sw_scr, 0, 0);
    lv_obj_set_style_pad_all(s_sw_scr, 0, 0);

    ds_header(s_sw_scr, "STOPWATCH", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    s_sw_lbl = lv_label_create(s_sw_scr);
    lv_obj_set_style_text_font(s_sw_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_sw_lbl, lv_color_hex(DS_TEXT_PRIMARY), 0);
    lv_obj_align(s_sw_lbl, LV_ALIGN_TOP_MID, 0, SW_TIME_Y);

    lv_obj_t *hint = ui_label(s_sw_scr, "OK: START / STOP", &badge_font_gb2312_small,
                              DS_TEXT_SECONDARY);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, SW_HINT_Y);

    ds_footer(s_sw_scr, &s_sw_dots, APP_PAGE_COUNT, APP_PAGE_TOOLS);
    lv_screen_load(s_sw_scr);

    s_sw_running = false;
    s_sw_accum_us = 0;
    s_sw_timer = lv_timer_create(sw_tick, 1000, NULL);
    sw_tick(NULL);
}

// 进入选中工具子屏。stopwatch → 秒表;其余 → COMING SOON。
static bool enter_child_cb(void *ctx, int index)
{
    (void)ctx;
    if (index == TOOL_STOPWATCH) {
        build_stopwatch_screen();
    } else {
        lv_obj_t *s = lv_obj_create(NULL);
        lv_obj_remove_flag(s, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(s, lv_color_hex(DS_BG), 0);
        lv_obj_set_style_border_width(s, 0, 0);
        lv_obj_set_style_pad_all(s, 0, 0);
        ds_header(s, s_tool_names[index], bsp_battery_soc(),
                  &badge_font_gb2312_small, &lv_font_montserrat_14);
        lv_obj_t *lbl = ui_label(s, "COMING SOON", &badge_font_gb2312_small,
                                 DS_TEXT_SECONDARY);
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 120);
        ds_footer(s, NULL, APP_PAGE_COUNT, APP_PAGE_TOOLS);
        lv_screen_load(s);
    }
    ESP_LOGI(TAG, "enter tool %d", index);
    return true;
}

// 退出子屏:停秒表定时器。占位屏对象随 LVGL 屏销毁回收。
static void exit_child_cb(void *ctx)
{
    (void)ctx;
    if (s_sw_timer) { lv_timer_del(s_sw_timer); s_sw_timer = NULL; }
    s_sw_scr = NULL;
    s_sw_lbl = NULL;
}

// 子屏按键:秒表 OK 启停;占位工具忽略。长按 OK 由控制器统一"返回菜单"。
static app_list_child_key_t child_key_cb(void *ctx, bsp_btn_t btn, bsp_btn_ev_t ev)
{
    (void)ctx;
    if (ev == BSP_BTN_CLICK && btn == BSP_BTN_OK && s_sw_scr) {
        sw_toggle();
        return APP_LIST_CHILD_KEY_OK;
    }
    return APP_LIST_CHILD_KEY_NONE;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------
void tools_enter(app_page_t page)
{
    (void)page;
    if (!s_ctl) {
        static const app_list_cfg_t cfg = {
            .title = "TOOLS",
            .intro = "TOOLS",
            .sub = "Timer / Stopwatch / Calculator / Morse",
            .page = APP_PAGE_TOOLS,
            .items = s_tool_names,
            .item_count = TOOLS_COUNT,
            .enter_child = enter_child_cb,
            .exit_child = exit_child_cb,
            .child_key = child_key_cb,
        };
        s_ctl = app_list_create(&cfg);
    }
    app_list_enter(s_ctl);
}

void tools_exit(void)
{
    if (s_ctl) { app_list_destroy(s_ctl); s_ctl = NULL; }
}

bool tools_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    return s_ctl ? app_list_key(s_ctl, btn, ev) : false;
}