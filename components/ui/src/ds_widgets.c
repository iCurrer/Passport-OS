// components/ui/src/ds_widgets.c
//
// Passport OS V2 design-system 原语实现（Header / Footer / Page Indicator）。
// 纯 LVGL + ds_tokens，不依赖硬件与主工程 assets。
#include "ds_widgets.h"
#include "ui_pixel.h"          // 复用 ui_battery_pct 的电量百分比取整格式
#include <stddef.h>
#include <stdbool.h>

// 样式一个圆点：实心=强调色填充；空心=次要色 1px 描边 + 透明填充。
static void apply_dot(lv_obj_t *dot, bool filled)
{
    // 空心用透明底 + 描边描出圆环；实心用纯色填充。
    lv_obj_set_style_bg_opa(dot, filled ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(DS_ACCENT), 0);
    lv_obj_set_style_border_width(dot, filled ? 0 : 1, 0);
    lv_obj_set_style_border_color(dot, lv_color_hex(DS_TEXT_SECONDARY), 0);
}

// 在 parent 上建一个圆点。
static lv_obj_t *dot_create(lv_obj_t *parent, int x, int y, bool filled)
{
    lv_obj_t *d = lv_obj_create(parent);
    lv_obj_remove_flag(d, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(d, x, y);
    lv_obj_set_size(d, DS_DOT_SIZE, DS_DOT_SIZE);
    lv_obj_set_style_radius(d, DS_DOT_SIZE / 2, 0);
    lv_obj_set_style_pad_all(d, 0, 0);
    apply_dot(d, filled);
    return d;
}

// ---------------------------------------------------------------------------
// ds_header
// ---------------------------------------------------------------------------
ds_header_result_t ds_header(lv_obj_t *parent, const char *brand, int battery_soc,
                             const lv_font_t *brand_font, const lv_font_t *num_font)
{
    if (!num_font) num_font = &lv_font_montserrat_14;

    // 左侧标题
    lv_obj_t *brand_lbl = lv_label_create(parent);
    lv_label_set_text(brand_lbl, brand ? brand : "");
    lv_obj_set_style_text_font(brand_lbl, brand_font, 0);
    lv_obj_set_style_text_color(brand_lbl, lv_color_hex(DS_TEXT_PRIMARY), 0);
    lv_obj_set_pos(brand_lbl, DS_HDR_BRAND_X, DS_HDR_BRAND_Y);

    // 电量外框（透明底 + 主色 1px 描边）
    lv_obj_t *frame = lv_obj_create(parent);
    lv_obj_remove_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(frame, DS_HDR_BATT_X, DS_HDR_BATT_Y);
    lv_obj_set_size(frame, DS_HDR_BATT_W, DS_HDR_BATT_H);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_set_style_radius(frame, 2, 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(DS_TEXT_PRIMARY), 0);
    lv_obj_set_style_border_width(frame, 1, 0);

    // 电量内芯（背景色挖空，露出负空间）
    lv_obj_t *core = lv_obj_create(parent);
    lv_obj_remove_flag(core, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(core, DS_HDR_BATT_CORE_X, DS_HDR_BATT_CORE_Y);
    lv_obj_set_size(core, DS_HDR_BATT_CORE_W, DS_HDR_BATT_CORE_H);
    lv_obj_set_style_pad_all(core, 0, 0);
    lv_obj_set_style_border_width(core, 0, 0);
    lv_obj_set_style_bg_color(core, lv_color_hex(DS_BG), 0);

    // 电量填充（宽度随 SOC；正常主色，<20% 警告，<10% 危险）
    int fill_w = (battery_soc >= 0) ? (DS_HDR_BATT_CORE_W * battery_soc) / 100 : 0;
    if (fill_w > DS_HDR_BATT_CORE_W) fill_w = DS_HDR_BATT_CORE_W;
    if (fill_w < 0) fill_w = 0;
    uint32_t fc = DS_TEXT_PRIMARY;
    if (battery_soc >= 0 && battery_soc < 10)      fc = DS_DANGER;
    else if (battery_soc >= 0 && battery_soc < 20) fc = DS_WARN;
    lv_obj_t *fill = lv_obj_create(parent);
    lv_obj_remove_flag(fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(fill, DS_HDR_BATT_CORE_X, DS_HDR_BATT_CORE_Y);
    lv_obj_set_size(fill, fill_w, DS_HDR_BATT_CORE_H);
    lv_obj_set_style_pad_all(fill, 0, 0);
    lv_obj_set_style_border_width(fill, 0, 0);
    lv_obj_set_style_bg_color(fill, lv_color_hex(fc), 0);

    // 电量百分比（取整到十位）
    char pct_buf[8];
    ui_battery_pct(battery_soc, pct_buf, sizeof(pct_buf));
    lv_obj_t *pct = lv_label_create(parent);
    lv_label_set_text(pct, pct_buf);
    lv_obj_set_style_text_font(pct, num_font, 0);
    lv_obj_set_style_text_color(pct, lv_color_hex(DS_TEXT_PRIMARY), 0);
    lv_obj_set_pos(pct, DS_HDR_BATT_PCT_X, DS_HDR_BATT_PCT_Y);

    // Header 底部分隔线（y=34，左右留 DS_MARGIN_X）
    ui_block(parent, DS_MARGIN_X, DS_HEADER_LINE_Y,
             DS_SCREEN_W - DS_MARGIN_X * 2, 1, DS_LINE);

    ds_header_result_t r = { .brand = brand_lbl, .fill = fill, .pct = pct };
    return r;
}

// ---------------------------------------------------------------------------
// Page Indicator
// ---------------------------------------------------------------------------
void ds_page_dots(lv_obj_t *parent, ds_dots_t *dots, uint8_t count, uint8_t current)
{
    if (!dots || count == 0 || count > DS_PAGES_MAX) return;
    dots->count = count;
    if (current >= count) current = 0;

    int total = (count - 1) * DS_DOT_GAP + DS_DOT_SIZE;
    int x0 = (DS_SCREEN_W - total) / 2;
    for (int i = 0; i < count; i++) {
        int x = x0 + i * DS_DOT_GAP;
        dots->dots[i] = dot_create(parent, x, DS_DOT_Y, i == (int)current);
    }
}

void ds_page_dots_set(ds_dots_t *dots, uint8_t current)
{
    if (!dots || current >= dots->count) return;
    for (int i = 0; i < dots->count; i++) {
        if (!dots->dots[i]) continue;
        apply_dot(dots->dots[i], i == (int)current);
    }
}

// ---------------------------------------------------------------------------
// Footer（顶部分隔线 + Page Indicator）
// ---------------------------------------------------------------------------
void ds_footer(lv_obj_t *parent, ds_dots_t *dots, uint8_t count, uint8_t current)
{
    // Footer 顶缘分隔线（y=271）
    ui_block(parent, 0, DS_FOOTER_Y - 1, DS_SCREEN_W, 1, DS_LINE);
    ds_page_dots(parent, dots, count, current);
}