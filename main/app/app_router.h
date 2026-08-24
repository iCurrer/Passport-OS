// main/app/app_router.h —— Passport OS V2 全局页面路由（上下翻页）。
//
// 统一管理全局按键（plan §5 / skill 核心交互），取代 main.c 直连 badge_key 的分发：
//   UP 短按 → 上一页；DOWN 短按 → 下一页；OK 短按 → 页面局部操作
//   UP 长按 → 回 HOME；OK 长按 → 回 HOME；DOWN 长按 → 快速状态切换
// 页面循环翻页，最多 8 页（app_pages.h）。禁止各页面自行管理全局按键。
#pragma once

#include "app_pages.h"
#include "bsp_button.h"

#ifdef __cplusplus
extern "C" {
#endif

// 初始化路由（幂等）：启动电源自动休眠计时，页码复位到 HOME。
void app_router_init(void);

// 渲染当前页(懒：建当前页占位 UI)。须在 LVGL 锁内调用(或由内部加锁)。
void app_router_enter(void);

// 全局按键分发。从 button 组件任务调用；内部自行管理 LVGL 锁与电源活跃计时。
void app_router_key(bsp_btn_t btn, bsp_btn_ev_t ev);

// 跳转到指定页(内部加锁)。
void app_router_goto(app_page_t page);

// 当前页(0-based)。
app_page_t app_router_current(void);

// 纯导航逻辑:页码右移 dir(+1/-1) 循环(clamp 非法入参到 HOME)。host 可单测。
app_page_t app_router_page_cycle(app_page_t cur, int dir);

#ifdef __cplusplus
}
#endif