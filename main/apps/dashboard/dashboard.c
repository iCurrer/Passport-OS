// main/apps/dashboard/dashboard.c —— Passport OS V2 第 4 页 DASHBOARD(仪表盘)页。
//
// 布局(参考 docs/UI_DESIGN_SPEC.md §10,诚实无假数据):
//   Header(0-35)      :ds_header("DASHBOARD" + 电量)
//   Content(36-271)   :运行时长(UPTIME,esp_timer 实时刷新,大号数字)
//                      "UPTIME" 小标签 → 强调分隔线 → 状态行(FOCUS/NEXT/BLE/WIFI)
//   Footer(272-319)   :Page Indicator(DASHBOARD=第 4 点实心)
//
// 时钟说明:本机无 RTC、Wi-Fi/BLE 默认关,无可靠墙钟来源 → 不造假时间/日期。
// 展示真实设备状态:UPTIME(esp_timer)、本页 FOCUS 会话计时、BLE/WIFI 开关。NEXT 日程占位。
// 每秒用 lv_timer 刷新 UPTIME 与 FOCUS;退出先停定时器再删对象(红线)。
#include "dashboard.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "ble.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stddef.h>
#include <stdio.h>

static const char *TAG = "dashboard";

// 布局 y(Content 36-271)
#define DASH_UPTIME_Y   56      // 运行时长(大号数字)
#define DASH_UPTIME_TAG 86      // "UPTIME" 小标签
#define DASH_DIV_Y      112     // 强调分隔线
#define DASH_ROW_Y      132     // 状态行起始
#define DASH_ROW_GAP    22
#define DASH_LABEL_X    16      // 左标签 x
#define DASH_VALUE_X    150     // 右数值 x

static lv_obj_t     *s_scr;
static ds_dots_t     s_dots;
static lv_obj_t     *s_uptime_lbl;
static lv_obj_t     *s_focus_lbl;
static lv_timer_t   *s_timer;
static int64_t       s_focus_start_us;   // 进入本页时间(用于 FOCUS 会话计时)

// 每秒刷新:UPTIME(esp_timer)+ FOCUS(本页会话)。运行在 LVGL 任务,已持锁。
static void tick_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_scr) return;

    int64_t now = esp_timer_get_time();
    int64_t up_s = now / 1000000;
    char b[24];
    snprintf(b, sizeof(b), "%02d:%02d:%02d",
             (int)(up_s / 3600), (int)((up_s / 60) % 60), (int)(up_s % 60));
    if (s_uptime_lbl) lv_label_set_text(s_uptime_lbl, b);

    int64_t f_s = (now - s_focus_start_us) / 1000000;
    snprintf(b, sizeof(b), "%02d:%02d", (int)(f_s / 60), (int)(f_s % 60));
    if (s_focus_lbl) lv_label_set_text(s_focus_lbl, b);
}

// 加一个"左标签(次) | 右数值(主)"行;val_out 可回传数值 label 供后续刷新。
static void add_row(lv_obj_t *parent, int y, const char *label, const char *value,
                    lv_obj_t **val_out)
{
    lv_obj_t *l = ui_label(parent, label, &badge_font_gb2312_small, DS_TEXT_SECONDARY);
    lv_obj_set_pos(l, DASH_LABEL_X, y);
    lv_obj_t *v = ui_label(parent, value, &badge_font_gb2312_small, DS_TEXT_PRIMARY);
    lv_obj_set_pos(v, DASH_VALUE_X, y);
    if (val_out) *val_out = v;
}

void dashboard_enter(app_page_t page)
{
    (void)page;

    s_focus_start_us = esp_timer_get_time();

    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    ds_header(s_scr, "DASHBOARD", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    // UPTIME 大号数字(Montserrat 20,数字字体)
    s_uptime_lbl = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_uptime_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_uptime_lbl, lv_color_hex(DS_TEXT_PRIMARY), 0);
    lv_obj_align(s_uptime_lbl, LV_ALIGN_TOP_MID, 0, DASH_UPTIME_Y);

    // "UPTIME" 小标签
    lv_obj_t *tag = ui_label(s_scr, "UPTIME", &badge_font_gb2312_small, DS_TEXT_SECONDARY);
    lv_obj_align(tag, LV_ALIGN_TOP_MID, 0, DASH_UPTIME_TAG);

    // 强调分隔线
    ui_block(s_scr, 120 - 36, DASH_DIV_Y, 72, 2, DS_ACCENT);

    // 状态行:FOCUS(本页会话)/ NEXT(占位)/ BLE / WIFI
    add_row(s_scr, DASH_ROW_Y + 0 * DASH_ROW_GAP, "FOCUS", "00:00", &s_focus_lbl);
    add_row(s_scr, DASH_ROW_Y + 1 * DASH_ROW_GAP, "NEXT", "NONE", NULL);
    lv_obj_t *ble_v = NULL;
    add_row(s_scr, DASH_ROW_Y + 2 * DASH_ROW_GAP, "BLE",
            ble_is_enabled() ? "ON" : "OFF", &ble_v);
    (void)ble_v;
    add_row(s_scr, DASH_ROW_Y + 3 * DASH_ROW_GAP, "WIFI", "OFF", NULL);  // 未启用 Wi-Fi

    // Footer:Page Indicator(DASHBOARD=第 4 点实心)
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_DASHBOARD);

    lv_screen_load(s_scr);

    // 每秒刷新定时器(LVGL 任务内,已持锁)
    s_timer = lv_timer_create(tick_cb, 1000, NULL);
    ESP_LOGI(TAG, "dashboard entered");
}

void dashboard_exit(void)
{
    // 先停定时器,再删对象(红线)
    if (s_timer) { lv_timer_del(s_timer); s_timer = NULL; }
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    s_uptime_lbl = NULL;
    s_focus_lbl = NULL;
}