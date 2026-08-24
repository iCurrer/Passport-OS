// main/apps/games/games.h —— Passport OS V2 第 6 页 GAMES(游戏菜单)页。
// 由 app_router 通过页面表调用。enter/exit 须在 LVGL 锁内;key 由 router 持锁回调。
#pragma once

#include "app_pages.h"
#include "bsp_button.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 构建并加载 GAMES 列表屏幕。
void games_enter(app_page_t page);

// 销毁待退出的 GAMES 屏幕(含正在运行的游戏/停止计时器)。
void games_exit(void);

// GAMES 页局部按键(短按):列表内 UP/DOWN 选择、OK 进入;游戏内转发 game_key。
// 须在 LVGL 锁内调用(router 持锁)。返回 true 表示已消费(不再走全局翻页)。
bool games_key(bsp_btn_t btn, bsp_btn_ev_t ev);

#ifdef __cplusplus
}
#endif