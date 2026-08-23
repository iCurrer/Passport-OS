// main/settings/settings.h —— 设置页面:息屏时间/蓝牙/版本信息。
#pragma once

#include "bsp_button.h"

void settings_enter(void);
void settings_exit(void);
void settings_key(bsp_btn_t btn, bsp_btn_ev_t ev);