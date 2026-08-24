// main/game/game.h —— 像素游戏模块公开 API。
// 实现 enter/exit/key 三接口;游戏接管全屏,退出后由调用方(GAMES 页)恢复菜单。
// key 不再自行加锁 —— 由调用方(Router 委托)持锁调用;返回结果告知是否已消费/已退出。
#pragma once

#include "bsp_button.h"

// 游戏按键分发结果
typedef enum {
    GAME_KEY_NONE = 0,      // 未消费(交给全局处理,如长按)
    GAME_KEY_CONSUMED,      // 已消费(游戏内操作)
    GAME_KEY_EXITED,        // 游戏已结束并退出(调用方应重建游戏菜单)
} game_key_result_t;

// 进入游戏(创建 LVGL screen,启动游戏循环)。须在 LVGL 锁内调用。
void game_enter(void);

// 退出游戏(停止循环,删除 screen)。须在 LVGL 锁内调用。
void game_exit(void);

// 游戏内按键分发。须在 LVGL 锁内调用(调用方持锁)。返回结果见 game_key_result_t。
game_key_result_t game_key(bsp_btn_t btn, bsp_btn_ev_t ev);