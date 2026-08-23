// main/game/game.h —— 像素游戏模块公开 API。
// 实现 enter/exit/key 三接口,遵循应用层模块规范。
// 游戏接管全屏,退出后由调用方恢复 badge 界面。
#pragma once

#include "bsp_button.h"

// 进入游戏(创建 LVGL screen,启动游戏循环)。须在 LVGL 锁内调用。
void game_enter(void);

// 退出游戏(停止循环,删除 screen)。须在 LVGL 锁内调用。
void game_exit(void);

// 游戏内按键分发。可从 button 任务调用;内部自行管理 LVGL 锁。
void game_key(bsp_btn_t btn, bsp_btn_ev_t ev);