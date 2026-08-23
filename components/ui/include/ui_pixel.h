// components/ui/include/ui_pixel.h —— 可复用的像素风 LVGL UI 原语。
// 与硬件无关、与业务无关:只提供"画纯色块 / 建标签"等最小编织件,
// 供应用层(badge 等)拼装页面,避免各页面各写一份 block() 造成重复。
#pragma once

#include "lvgl.h"

// 在 parent 内按本地坐标画一个纯色矩形块(直角、无边框、无内边距)。
lv_obj_t *ui_block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color);

// 创建带指定字体与颜色的标签。
lv_obj_t *ui_label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color);

// 将电量(0~100 或 -1)格式化为显示文本("100%"/"--"),取整到十位。
// 供导航栏初始绘制与各页定时刷新共用,避免取整逻辑多处重复。
void ui_battery_pct(int soc, char *out, size_t out_sz);

// 复用的顶部导航栏结果(供调用方持有引用,用于定时/外部刷新)
typedef struct {
    lv_obj_t *brand;    // 品牌文字标签(可 lv_label_set_text 更新)
    lv_obj_t *fill;     // 电量填充块(可 lv_obj_set_width 更新)
    lv_obj_t *pct;      // 电量百分比文字标签(可 lv_label_set_text 更新)
} ui_header_result_t;

// 复用的顶部导航栏:左侧品牌文字 + 右侧电量条(外框+填充+内芯) + 百分比数字 + 底部分隔线。
// 颜色由调用方传入,实现不含业务逻辑。battery_soc: 0~100 或 -1(显示 "--%")。
ui_header_result_t ui_header_bar(lv_obj_t *parent, const char *brand, int battery_soc,
                                  uint32_t txt_color, uint32_t parent_bg_color,
                                  uint32_t batt_fill_color, uint32_t div_color,
                                  const lv_font_t *brand_font, const lv_font_t *pct_font);