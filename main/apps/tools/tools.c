// main/apps/tools/tools.c —— Passport OS V2 第 5 页 TOOLS(工具)页。
//
// 列表(参考 docs/UI_DESIGN_SPEC.md §11):TIMER / STOPWATCH / CALCULATOR / MORSE。
// 交互:列表内 UP/DOWN 选择(高亮)、OK 进入工具;工具内 OK 操作。
// 全局语义:短按在页面局部优先(列表选择/工具操作),消费不了才翻页;长按全局(OK/UP 回 HOME)。
// 本 TASK 实现 STOPWATCH(秒表,OK 启动/停止),其余工具为 COMING SOON 占位。
#include "tools.h"
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
#include <string.h>

static const char *TAG = "tools";

// 工具列表
#define TOOLS_COUNT 4
enum { TOOL_TIMER = 0, TOOL_STOPWATCH, TOOL_CALCULATOR, TOOL_MORSE };
static const char *s_tool_names[TOOLS_COUNT] = {
    "TIMER", "STOPWATCH", "CALCULATOR", "MORSE",
};

// 列表布局(Content 36-271)
#define LIST_X       16
#define LIST_W       208
#define ROW_H        44
#define ROW_Y0       56
#define ROW_GAP      48
#define LABEL_X      28
#define LABEL_Y_OFS  15     // 标签相对行顶的垂直偏移(居中)

// 秒表布局
#define SW_TIME_Y    100
#define SW_HINT_Y    152

typedef enum { TOOLS_LIST, TOOLS_TOOL } tools_mode_t;

static tools_mode_t s_mode = TOOLS_LIST;
static int          s_sel = 0;          // 当前选中工具
static lv_obj_t    *s_scr;              // 当前屏幕(列表或工具)
static ds_dots_t    s_dots;
static lv_obj_t    *s_row_bg[TOOLS_COUNT];    // 列表行背景
static lv_obj_t    *s_row_name[TOOLS_COUNT];  // 列表行名称
static lv_obj_t    *s_sw_lbl;           // 秒表时间
static lv_timer_t  *s_sw_timer;
static bool         s_sw_running;
static int64_t      s_sw_accum_us;      // 已累计
static int64_t      s_sw_start_us;      // 当前运行段起点

// ---------------------------------------------------------------------------
// 列表页
// ---------------------------------------------------------------------------
static void list_refresh_sel(void)
{
    // 原地刷新高亮,不重建对象(避免文字被盖/内存泄漏)
    for (int i = 0; i < TOOLS_COUNT; i++) {
        bool sel = (i == s_sel);
        lv_obj_set_style_bg_color(s_row_bg[i], lv_color_hex(sel ? DS_CARD : DS_BG), 0);
        lv_obj_set_style_text_color(s_row_name[i],
                                    lv_color_hex(sel ? DS_ACCENT : DS_TEXT_SECONDARY), 0);
    }
}

static void build_list_screen(void)
{
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    ds_header(s_scr, "TOOLS", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    // 行对象只建一次
    for (int i = 0; i < TOOLS_COUNT; i++) {
        int y = ROW_Y0 + i * ROW_GAP;
        s_row_bg[i] = ui_block(s_scr, LIST_X, y, LIST_W, ROW_H, DS_BG);
        s_row_name[i] = ui_label(s_scr, s_tool_names[i], &badge_font_gb2312_small,
                                 DS_TEXT_SECONDARY);
        lv_obj_set_pos(s_row_name[i], LABEL_X, y + LABEL_Y_OFS);
    }
    list_refresh_sel();
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_TOOLS);
    lv_screen_load(s_scr);
}

// ---------------------------------------------------------------------------
// 秒表工具
// ---------------------------------------------------------------------------
static void sw_tick(lv_timer_t *t)
{
    (void)t;
    if (!s_scr || !s_sw_lbl) return;
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
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    ds_header(s_scr, "STOPWATCH", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    s_sw_lbl = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_sw_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_sw_lbl, lv_color_hex(DS_TEXT_PRIMARY), 0);
    lv_obj_align(s_sw_lbl, LV_ALIGN_TOP_MID, 0, SW_TIME_Y);

    lv_obj_t *hint = ui_label(s_scr, "OK: START / STOP", &badge_font_gb2312_small,
                              DS_TEXT_SECONDARY);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, SW_HINT_Y);

    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_TOOLS);
    lv_screen_load(s_scr);

    s_sw_running = false;
    s_sw_accum_us = 0;
    s_sw_timer = lv_timer_create(sw_tick, 1000, NULL);
    sw_tick(NULL);
}

// 进入某工具(列表 → 工具子屏)
static void enter_tool(int idx)
{
    lv_obj_delete(s_scr); s_scr = NULL;
    s_mode = TOOLS_TOOL;
    if (idx == TOOL_STOPWATCH) {
        build_stopwatch_screen();
    } else {
        // 占位工具屏
        s_scr = lv_obj_create(NULL);
        lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
        lv_obj_set_style_border_width(s_scr, 0, 0);
        lv_obj_set_style_pad_all(s_scr, 0, 0);
        ds_header(s_scr, s_tool_names[idx], bsp_battery_soc(),
                  &badge_font_gb2312_small, &lv_font_montserrat_14);
        lv_obj_t *lbl = ui_label(s_scr, "COMING SOON", &badge_font_gb2312_small,
                                 DS_TEXT_SECONDARY);
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 120);
        ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_TOOLS);
        lv_screen_load(s_scr);
    }
    ESP_LOGI(TAG, "enter tool %d", idx);
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------
void tools_enter(app_page_t page)
{
    (void)page;
    s_mode = TOOLS_LIST;
    s_sel = 0;
    build_list_screen();
}

void tools_exit(void)
{
    if (s_sw_timer) { lv_timer_del(s_sw_timer); s_sw_timer = NULL; }
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    s_sw_lbl = NULL;
}

bool tools_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (ev != BSP_BTN_CLICK) return false;   // 长按等仍走全局(返回 HOME 等)

    if (s_mode == TOOLS_LIST) {
        switch (btn) {
        case BSP_BTN_UP:   s_sel = (s_sel + TOOLS_COUNT - 1) % TOOLS_COUNT; break;
        case BSP_BTN_DOWN: s_sel = (s_sel + 1) % TOOLS_COUNT;               break;
        case BSP_BTN_OK:   enter_tool(s_sel); return true;
        default: return false;
        }
        list_refresh_sel();
        return true;
    }

    // 工具模式
    if (btn == BSP_BTN_OK) {
        sw_toggle();          // 仅秒表有 OK 操作;占位工具忽略
        return true;
    }
    return false;             // UP/DOWN → 走全局翻页离开工具
}