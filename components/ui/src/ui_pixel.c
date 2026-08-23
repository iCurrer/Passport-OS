// components/ui/src/ui_pixel.c
#include "ui_pixel.h"
#include <stdio.h>

lv_obj_t *ui_block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

lv_obj_t *ui_label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

void ui_battery_pct(int soc, char *out, size_t out_sz)
{
    if (soc >= 0) {
        int r = (soc + 5) / 10 * 10;   // 取整到十位
        if (r > 100) r = 100;
        snprintf(out, out_sz, "%d%%", r);
    } else {
        snprintf(out, out_sz, "--");
    }
}

ui_header_result_t ui_header_bar(lv_obj_t *parent, const char *brand, int battery_soc,
                                  uint32_t txt_color, uint32_t parent_bg_color,
                                  uint32_t batt_fill_color, uint32_t div_color,
                                  const lv_font_t *brand_font, const lv_font_t *pct_font)
{
    // 左上:品牌文字
    lv_obj_t *brand_lbl = ui_label(parent, brand, brand_font, txt_color);
    lv_obj_set_pos(brand_lbl, 22, 13);

    // 右上:电池外框(20x10)
    ui_block(parent, 158, 14, 20, 10, txt_color);
    // 电池内芯
    ui_block(parent, 160, 16, 16, 6, parent_bg_color);
    // 电池填充(宽度根据 SOC 计算)
    int fill_w = (battery_soc >= 0) ? (16 * battery_soc) / 100 : 0;
    if (fill_w > 16) fill_w = 16;
    lv_obj_t *fill = ui_block(parent, 160, 16, fill_w, 6, batt_fill_color);

    // 电量百分比数字(取整到十位),置于电量条右侧
    char pct_buf[16];
    ui_battery_pct(battery_soc, pct_buf, sizeof(pct_buf));
    lv_obj_t *pct = ui_label(parent, pct_buf, pct_font, txt_color);
    lv_obj_set_pos(pct, 186, 12);

    // 底部分隔线
    ui_block(parent, 22, 40, 240 - 22 * 2, 1, div_color);

    return (ui_header_result_t){ .brand = brand_lbl, .fill = fill, .pct = pct };
}