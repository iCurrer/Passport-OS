// components/ui/include/ui_pixel.h —— 可复用的像素风 LVGL UI 原语。
// 与硬件无关、与业务无关:只提供"画纯色块 / 建标签"等最小编织件,
// 供应用层(badge 等)拼装页面,避免各页面各写一份 block() 造成重复。
#pragma once

#include "lvgl.h"

// 在 parent 内按本地坐标画一个纯色矩形块(直角、无边框、无内边距)。
lv_obj_t *ui_block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color);

// 创建带指定字体与颜色的标签。
lv_obj_t *ui_label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color);