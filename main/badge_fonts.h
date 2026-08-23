// main/badge_fonts.h —— 名牌字体。
#pragma once

#include "lvgl.h"

// 姓名用大字号(34px):收录 李秋实
extern const lv_font_t badge_font_name;
// 状态/职位用小字号(22px):收录 自由豆包大学
extern const lv_font_t badge_font_status;
// GB2312 全量标准中文字库(24px,6763 字):用于姓名/职位/状态等可自定义中文字段
extern const lv_font_t badge_font_gb2312;
// GB2312 小号字库(14px,含 ASCII):用于顶部导航栏文字
extern const lv_font_t badge_font_gb2312_small;
