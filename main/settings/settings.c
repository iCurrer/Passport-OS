// main/settings/settings.c —— 设置页面:息屏时间/蓝牙/版本信息。
//
// 列表式 UI, UP/DOWN 导航, OK 切换/选择, OK 长按返回名牌。
// 线程规则:所有 LVGL 操作在 LVGL 锁内,按键从 badge_key 转发。
#include "settings.h"
#include "badge.h"              // g_badge_sub, badge_enter
#include "badge_power.h"
#include "ble.h"
#include "ui_pixel.h"
#include "badge_fonts.h"
#include "badge_theme.h"        // 主题色(与首页统一)
#include "bsp_display.h"        // bsp_lvgl_lock / unlock
#include "bsp_battery.h"        // bsp_battery_soc(顶部电量条)
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "settings";

// 配色统一使用 badge_theme.h 的磨砂绿;仅保留设置页特有的行背景色。
#define ROW_BG        BADGE_DOCK_BG   // 未选中行背景(dock 同款深绿)
#define SEL_BG        0x2E4A3E        // 选中行高亮背景(绿色调)

// 布局
#define SCREEN_W      240
#define SCREEN_H      320
#define LIST_Y        60     // 顶部导航栏分隔线(y=40)下方
#define ROW_X         12
#define ROW_W         (SCREEN_W - 24)
#define ITEM_H        44
#define ITEM_GAP      4
#define LABEL_X       28
#define VALUE_X       148

// 设置项索引
enum { ITEM_SLEEP = 0, ITEM_BLE, ITEM_VERSION, ITEM_COUNT };

// 休眠时间选项(秒)
static const uint32_t s_sleep_opts[] = { 30, 60, 120, 300, 0 };  // 0=永不
static const char *s_sleep_labels[] = { "30秒", "1分钟", "2分钟", "5分钟", "永不" };
#define SLEEP_OPT_COUNT (sizeof(s_sleep_opts) / sizeof(s_sleep_opts[0]))

static struct {
    lv_obj_t *scr;
    lv_obj_t *rows[ITEM_COUNT];     // 每行高亮背景块
    lv_obj_t *labels[ITEM_COUNT];   // 每行名称
    lv_obj_t *values[ITEM_COUNT];   // 每行值
    lv_obj_t *hint;
    int sel;                        // 选中项索引
    bool ble_on;                    // 蓝牙开关状态
    int sleep_idx;                  // 休眠选项索引
} s;

// 刷新值显示
static void refresh_value(int idx)
{
    char buf[32];
    switch (idx) {
    case ITEM_SLEEP:
        snprintf(buf, sizeof(buf), "%s", s_sleep_labels[s.sleep_idx]);
        break;
    case ITEM_BLE:
        snprintf(buf, sizeof(buf), "%s", s.ble_on ? "ON" : "OFF");
        break;
    case ITEM_VERSION:
        snprintf(buf, sizeof(buf), "v1.0.0");
        break;
    default:
        return;
    }
    lv_label_set_text(s.values[idx], buf);
}

// 刷新选中高亮(行背景 + 名称文字色)
static void refresh_sel(void)
{
    for (int i = 0; i < ITEM_COUNT; i++) {
        bool sel = (i == s.sel);
        lv_obj_set_style_bg_color(s.rows[i], lv_color_hex(sel ? SEL_BG : ROW_BG), 0);
        lv_obj_set_style_text_color(s.labels[i], lv_color_hex(sel ? BADGE_ACCENT : BADGE_TXT_MUTED), 0);
    }
}

// 切换休眠时间
static void cycle_sleep(void)
{
    s.sleep_idx = (s.sleep_idx + 1) % SLEEP_OPT_COUNT;
    badge_power_set_timeout(s_sleep_opts[s.sleep_idx]);
    refresh_value(ITEM_SLEEP);
}

// 切换蓝牙
static void toggle_ble(void)
{
    s.ble_on = !s.ble_on;
    if (s.ble_on) ble_restart();
    else          ble_stop();
    refresh_value(ITEM_BLE);
}

// 向上导航
static void nav_up(void)
{
    if (s.sel > 0) s.sel--;
    else s.sel = ITEM_COUNT - 1;
    refresh_sel();
}

// 向下导航
static void nav_down(void)
{
    if (s.sel < ITEM_COUNT - 1) s.sel++;
    else s.sel = 0;
    refresh_sel();
}

// 确认选择
static void do_select(void)
{
    switch (s.sel) {
    case ITEM_SLEEP:    cycle_sleep(); break;
    case ITEM_BLE:      toggle_ble();  break;
    case ITEM_VERSION:  /* 只读,不操作 */ break;
    }
}

// ---- 公开 API ----

void settings_enter(void)
{
    memset(&s, 0, sizeof(s));
    s.ble_on = ble_is_enabled();   // 从 NVS 读取真实蓝牙开关状态
    // 从 badge_power 读取当前超时,匹配选项
    uint32_t cur = badge_power_get_timeout();
    s.sleep_idx = 0;
    for (int i = 0; i < (int)SLEEP_OPT_COUNT; i++) {
        if (s_sleep_opts[i] == cur) { s.sleep_idx = i; break; }
    }

    s.scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s.scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s.scr, lv_color_hex(BADGE_BG_COLOR), 0);
    lv_obj_set_style_border_width(s.scr, 0, 0);
    lv_obj_set_style_pad_all(s.scr, 0, 0);

    // 顶部导航栏(与首页同款:左侧标题 + 电量条 + 分隔线)
    ui_header_bar(s.scr, "设置", bsp_battery_soc(),
                  BADGE_TXT_MUTED, BADGE_BG_COLOR,
                  BADGE_BATT_NORMAL, BADGE_LINE_DIV,
                  &badge_font_gb2312_small, &lv_font_montserrat_14);

    // 列表项(每行:高亮背景块 + 左名称 + 右值,垂直居中)
    const char *item_names[] = { "休眠", "蓝牙", "版本" };
    for (int i = 0; i < ITEM_COUNT; i++) {
        int y = LIST_Y + i * (ITEM_H + ITEM_GAP);
        s.rows[i] = ui_block(s.scr, ROW_X, y, ROW_W, ITEM_H, i == 0 ? SEL_BG : ROW_BG);
        s.labels[i] = ui_label(s.scr, item_names[i], &badge_font_gb2312_small,
                               i == 0 ? BADGE_ACCENT : BADGE_TXT_MUTED);
        lv_obj_set_pos(s.labels[i], LABEL_X, y + (ITEM_H - 20) / 2);
        s.values[i] = ui_label(s.scr, "", &badge_font_gb2312_small, BADGE_TXT_MUTED);
        lv_obj_set_pos(s.values[i], VALUE_X, y + (ITEM_H - 20) / 2);
    }
    refresh_value(ITEM_SLEEP);
    refresh_value(ITEM_BLE);
    refresh_value(ITEM_VERSION);

    s.hint = ui_label(s.scr, "OK=选择  长按=返回", &badge_font_gb2312_small, BADGE_TXT_MUTED);
    lv_obj_center(s.hint);
    lv_obj_set_y(s.hint, SCREEN_H - 36);

    lv_screen_load(s.scr);
    ESP_LOGI(TAG, "settings entered, sleep=%lu, ble=%d", (unsigned long)cur, s.ble_on);
}

void settings_exit(void)
{
    if (s.scr) { lv_obj_delete(s.scr); s.scr = NULL; }
    memset(s.labels, 0, sizeof(s.labels));
    memset(s.values, 0, sizeof(s.values));
    s.hint = NULL;
    ESP_LOGI(TAG, "settings exited");
}

void settings_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    // 按键回调运行在 button 任务,所有 LVGL 操作须加锁。
    // (长按 OK 返回名牌由 badge.c 全局处理,这里只处理本页操作)
    if (!bsp_lvgl_lock(300)) return;

    if (ev != BSP_BTN_CLICK) {
        bsp_lvgl_unlock();
        return;
    }

    switch (btn) {
    case BSP_BTN_UP:   nav_up();     break;
    case BSP_BTN_DOWN: nav_down();   break;
    case BSP_BTN_OK:   do_select();  break;
    default: break;
    }

    bsp_lvgl_unlock();
}