// main/apps/settings/settings_page.c —— Passport OS V2 第 7 页 SETTINGS(设置)页。
//
// 列表:BLE(开关)/ SLEEP(休眠超时)/ VERSION(版本只读)。
// 交互:UP/DOWN 选择(高亮),OK 开关/循环。全局语义:短按页面局部优先,长按全局(OK/UP 回 HOME)。
// BLE 开关经 ble_stop/ble_restart 持久化到 NVS(ble_on);SLEEP 经 badge_power_set_timeout 持久化。
//
// 刷新规则:行对象只在 enter 时创建一次;选择刷新仅原地更新样式/文字,绝不重建对象(避免文字被盖/内存泄漏)。
#include "settings_page.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "badge_power.h"
#include "ble.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "settings_page";

#define SETTINGS_COUNT 3
enum { S_BLE = 0, S_SLEEP, S_VERSION };
static const char *s_names[SETTINGS_COUNT] = { "BLE", "SLEEP", "VERSION" };

// 休眠超时选项(秒;0=永不)
static const uint32_t s_sleep_opts[] = { 30, 60, 120, 300, 0 };
static const char   *s_sleep_labels[] = { "30s", "1m", "2m", "5m", "never" };
#define SLEEP_N  (sizeof(s_sleep_opts) / sizeof(s_sleep_opts[0]))

// 列表布局(Content 36-271)
#define LIST_X       16
#define LIST_W       208
#define ROW_H        44
#define ROW_Y0       56
#define ROW_GAP      48
#define LABEL_X      28
#define VALUE_X      150
#define LABEL_Y_OFS  15

static lv_obj_t *s_scr;
static ds_dots_t s_dots;
static int       s_sel;
static lv_obj_t *s_row_bg[SETTINGS_COUNT];    // 行背景块
static lv_obj_t *s_row_name[SETTINGS_COUNT];  // 行名称
static lv_obj_t *s_row_val[SETTINGS_COUNT];   // 行数值

static int find_sleep_idx(void)
{
    uint32_t cur = badge_power_get_timeout();
    for (int i = 0; i < (int)SLEEP_N; i++)
        if (s_sleep_opts[i] == cur) return i;
    return 0;
}

static const char *value_str(int idx)
{
    switch (idx) {
    case S_BLE:     return ble_is_enabled() ? "ON" : "OFF";
    case S_SLEEP:   return s_sleep_labels[find_sleep_idx()];
    case S_VERSION: return "v2.0.0";
    default:        return "";
    }
}

// 原地刷新选中高亮与数值(不重建对象)
static void refresh_sel(void)
{
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        bool sel = (i == s_sel);
        lv_obj_set_style_bg_color(s_row_bg[i], lv_color_hex(sel ? DS_CARD : DS_BG), 0);
        lv_obj_set_style_text_color(s_row_name[i],
                                    lv_color_hex(sel ? DS_ACCENT : DS_TEXT_SECONDARY), 0);
        lv_label_set_text(s_row_val[i], value_str(i));
    }
}

// 构建一次行对象(仅 enter 时调用)
static void build_rows(void)
{
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        int y = ROW_Y0 + i * ROW_GAP;
        s_row_bg[i] = ui_block(s_scr, LIST_X, y, LIST_W, ROW_H, DS_BG);
        s_row_name[i] = ui_label(s_scr, s_names[i], &badge_font_gb2312_small, DS_TEXT_SECONDARY);
        lv_obj_set_pos(s_row_name[i], LABEL_X, y + LABEL_Y_OFS);
        s_row_val[i] = ui_label(s_scr, "", &badge_font_gb2312_small, DS_TEXT_PRIMARY);
        lv_obj_set_pos(s_row_val[i], VALUE_X, y + LABEL_Y_OFS);
    }
    refresh_sel();
}

static void operate(int sel)
{
    switch (sel) {
    case S_BLE:
        if (ble_is_enabled()) ble_stop(); else ble_restart();
        ESP_LOGI(TAG, "BLE -> %s", ble_is_enabled() ? "ON" : "OFF");
        break;
    case S_SLEEP: {
        int idx = (find_sleep_idx() + 1) % SLEEP_N;
        badge_power_set_timeout(s_sleep_opts[idx]);
        break;
    }
    case S_VERSION: /* 只读 */ break;
    default: break;
    }
    refresh_sel();
}

void settings_page_enter(app_page_t page)
{
    (void)page;
    s_sel = 0;
    for (int i = 0; i < SETTINGS_COUNT; i++) { s_row_bg[i] = NULL; s_row_name[i] = NULL; s_row_val[i] = NULL; }

    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    ds_header(s_scr, "SETTINGS", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);
    build_rows();
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_SETTINGS);
    lv_screen_load(s_scr);
}

void settings_page_exit(void)
{
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    for (int i = 0; i < SETTINGS_COUNT; i++) { s_row_bg[i] = NULL; s_row_name[i] = NULL; s_row_val[i] = NULL; }
}

bool settings_page_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (ev != BSP_BTN_CLICK) return false;   // 长按仍走全局(返回 HOME 等)

    switch (btn) {
    case BSP_BTN_UP:   s_sel = (s_sel + SETTINGS_COUNT - 1) % SETTINGS_COUNT; break;
    case BSP_BTN_DOWN: s_sel = (s_sel + 1) % SETTINGS_COUNT;                  break;
    case BSP_BTN_OK:   operate(s_sel); return true;
    default: return false;
    }
    refresh_sel();
    return true;
}