// main/badge/badge.c —— 名牌应用门面:编排 data/power/ui 三个子模块。
// 对外暴露 badge.h 的 4 个接口,内部不含具体实现细节,
// 依赖方向 badge -> {ui,power,data},单向无环。
#include "badge.h"
#include "badge_data.h"
#include "badge_power.h"
#include "badge_ui.h"
#include "bsp_display.h"       // bsp_lvgl_lock / unlock(子页面进出须加锁)
#include "game.h"
#include "settings.h"

badge_sub_t g_badge_sub = BADGE_SUB_NONE;

void badge_enter(void)
{
    g_badge_sub = BADGE_SUB_NONE;
    badge_power_init();   // 计时 + 自动关机定时器 + 复位诊断
    badge_data_init();    // 载入字段(UI 初值依赖它,故先于 ui)
    badge_ui_init();      // 建屏并载入
}

void badge_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (!badge_power_key_activity()) return;

    // 子页面活跃时:
    if (g_badge_sub != BADGE_SUB_NONE) {
        // 全局:任意子页面 OK 长按返回名牌(exit + badge_enter 都须在 LVGL 锁内)
        if (btn == BSP_BTN_OK && ev == BSP_BTN_LONG) {
            badge_sub_t was = g_badge_sub;
            g_badge_sub = BADGE_SUB_NONE;
            if (!bsp_lvgl_lock(300)) { g_badge_sub = was; return; }
            if (was == BADGE_SUB_GAME)         game_exit();
            else if (was == BADGE_SUB_SETTINGS) settings_exit();
            badge_enter();
            bsp_lvgl_unlock();
            return;
        }
        // 其它按键转发给当前子页面
        if (g_badge_sub == BADGE_SUB_GAME)         game_key(btn, ev);
        else if (g_badge_sub == BADGE_SUB_SETTINGS) settings_key(btn, ev);
        return;
    }

    // 名牌界面:仅响应 CLICK
    if (ev != BSP_BTN_CLICK) return;

    if (btn == BSP_BTN_UP)        badge_ui_dock_prev();
    else if (btn == BSP_BTN_DOWN) badge_ui_dock_next();
    else if (btn == BSP_BTN_OK)   badge_ui_dock_enter();
}

void badge_update_text(badge_field_t field, const char *s)
{
    if (!s) return;

    badge_power_activity();   // BLE 写入也算活动,重置自动休眠计时
    badge_data_set(field, s); // 写内存 + 持久化 NVS
    badge_ui_set_field(field);// 刷新 UI
}

const char *badge_get_text(badge_field_t field)
{
    return badge_data_get(field);
}