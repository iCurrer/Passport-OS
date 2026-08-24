// components/ui/include/ds_widgets.h
//
// Passport OS V2 design-system 可复用原语：Header / Footer / Page Indicator。
// 纯 LVGL、与硬件无关，供各页面复用；颜色/间距取自 ds_tokens.h。
// 迁移期需调用旧 ui_pixel 的页面仍可继续用 ui_header_bar 等，二者并存不冲突。
// 对应规范见 docs/UI_DESIGN_SPEC.md §1 / §4。
#pragma once

#include "lvgl.h"
#include "ds_tokens.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Header 复用结果：供调用方持有引用，用于外部刷新（标题/电量条/百分比）。
typedef struct {
    lv_obj_t *brand;   // 标题文字标签
    lv_obj_t *fill;    // 电量填充块
    lv_obj_t *pct;     // 电量百分比文字
} ds_header_result_t;

// Page Indicator 圆点集合（供 Router / 页面切换时刷新高亮）。
typedef struct {
    lv_obj_t *dots[DS_PAGES_MAX];   // 至多 DS_PAGES_MAX 个圆点
    uint8_t   count;
} ds_dots_t;

// V2 Header：左侧标题 + 右侧电量条（外框/内芯/填充）+ 百分比 + 底部分隔线，
// 配色一律使用 V2 深色 token。battery_soc: 0~100 或 -1（显示 "--"）。
// 字库由调用方传入：中文标题用主工程 assets 的 GB2312 字库，数字用 Montserrat。
// num_font 为 NULL 时数字默认用 lv_font_montserrat_14。
ds_header_result_t ds_header(lv_obj_t *parent, const char *brand, int battery_soc,
                             const lv_font_t *brand_font, const lv_font_t *num_font);

// Footer：底部 1px 分隔线（y=271）+ Page Indicator。整组水平居中于 240。
void ds_footer(lv_obj_t *parent, ds_dots_t *dots, uint8_t count, uint8_t current);

// 单独绘制 Page Indicator 圆点。current 为 0-based 当前页；当前页实心(强调色)，其余空心(次要色描边)。
void ds_page_dots(lv_obj_t *parent, ds_dots_t *dots, uint8_t count, uint8_t current);

// 页码变化时刷新高亮，复用已建圆点对象（不重建），把高亮移到新的 current。
void ds_page_dots_set(ds_dots_t *dots, uint8_t current);

#ifdef __cplusplus
}
#endif