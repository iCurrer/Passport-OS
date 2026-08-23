// main/badge/badge.c —— 名牌应用门面:编排 data/power/ui 三个子模块。
// 对外暴露 badge.h 的 4 个接口,内部不含具体实现细节,
// 依赖方向 badge -> {ui,power,data},单向无环。
#include "badge.h"
#include "badge_data.h"
#include "badge_power.h"
#include "badge_ui.h"

void badge_enter(void)
{
    badge_power_init();   // 计时 + 自动关机定时器 + 复位诊断
    badge_data_init();    // 载入字段(UI 初值依赖它,故先于 ui)
    badge_ui_init();      // 建屏并载入
}

void badge_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    // 开机前短暂忽略按键;其余活动时间在此统一刷新。
    if (!badge_power_key_activity()) return;

    // OK 短按/长按暂空:真关机由物理电源开关负责。
    if (ev != BSP_BTN_CLICK) return;

    if (btn == BSP_BTN_UP)        badge_ui_dock_prev();
    else if (btn == BSP_BTN_DOWN) badge_ui_dock_next();
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