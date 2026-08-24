// main/apps/settings/settings_page.c —— Passport OS V2 第 7 页 SETTINGS(设置)页。
//
// 交互(TASK-13 起):由共享 app_list 控制器驱动三态导航,本页无子屏(OK 原地操作)。
//   ENTRY 入口态:标题"SETTINGS" + 简介 + "OK TO ENTER";UP/DOWN 短按交全局翻页。
//   MENU 菜单态: BLE / SLEEP / VERSION,UP/DOWN 选择、OK 开关/循环;长按 OK 返回入口。
// BLE 开关经 ble_stop/ble_restart 持久化到 NVS(ble_on);SLEEP 经 badge_power_set_timeout 持久化。
// 行值列(BLE ON/OFF、SLEEP 标签、版本)由 value_cb 提供,原地操作后控制器自动刷新。
#include "settings_page.h"
#include "app_list.h"
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
#include <stdint.h>

static const char *TAG = "settings_page";

#define SETTINGS_COUNT 3
enum { S_BLE = 0, S_SLEEP, S_VERSION };
static const char *s_names[SETTINGS_COUNT] = { "BLE", "SLEEP", "VERSION" };

// 休眠超时选项(秒;0=永不)
static const uint32_t s_sleep_opts[] = { 30, 60, 120, 300, 0 };
static const char   *s_sleep_labels[] = { "30s", "1m", "2m", "5m", "never" };
#define SLEEP_N  (sizeof(s_sleep_opts) / sizeof(s_sleep_opts[0]))

static app_list_ctl_t *s_ctl;

static int find_sleep_idx(void)
{
    uint32_t cur = badge_power_get_timeout();
    for (int i = 0; i < (int)SLEEP_N; i++)
        if (s_sleep_opts[i] == cur) return i;
    return 0;
}

// 行右侧值文案。
static const char *value_cb(void *ctx, int index)
{
    (void)ctx;
    switch (index) {
    case S_BLE:     return ble_is_enabled() ? "ON" : "OFF";
    case S_SLEEP:   return s_sleep_labels[find_sleep_idx()];
    case S_VERSION: return "v2.0.0";
    default:        return "";
    }
}

// OK 原地操作(本页无子屏,返回 false 让控制器留在 MENU 并刷新值列)。
static bool enter_child_cb(void *ctx, int index)
{
    (void)ctx;
    (void)index;
    switch (index) {
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
    return false;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------
void settings_page_enter(app_page_t page)
{
    (void)page;
    if (!s_ctl) {
        static const app_list_cfg_t cfg = {
            .title = "SETTINGS",
            .intro = "SETTINGS",
            .sub = "BLE / Sleep / Version",
            .page = APP_PAGE_SETTINGS,
            .items = s_names,
            .item_count = SETTINGS_COUNT,
            .value_cb = value_cb,
            .enter_child = enter_child_cb,
        };
        s_ctl = app_list_create(&cfg);
    }
    app_list_enter(s_ctl);
}

void settings_page_exit(void)
{
    if (s_ctl) { app_list_destroy(s_ctl); s_ctl = NULL; }
}

bool settings_page_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    return s_ctl ? app_list_key(s_ctl, btn, ev) : false;
}