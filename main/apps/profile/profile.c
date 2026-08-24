// main/apps/profile/profile.c —— Passport OS V2 第 1 页 PROFILE(完整档案)页。
//
// 布局(参考 docs/UI_DESIGN_SPEC.md §7,纯文字版):
//   Header(0-35)      :ds_header("PROFILE" + 电量)
//   Content(36-271)   :姓名(30px 主) → 职位(14px 次) → 强调分隔线 → 状态(色点+文字)
//                      简介 bio(14px)
//   Footer(272-319)   :Page Indicator(PROFILE=第 1 点实心)
//
// 本页为无头像的个人信息页,采用固定坐标排布(非动态累计),让字号放大后视觉均衡:
//   姓名 30px @56..86 → 职位 @110 → 分隔线 @142 → 状态 @176 → 简介 @212..226
//   全字段满时末行 <271,不触 Footer(272)。空字段时其下的分隔线/后续行自动跳过。
#include "profile.h"
#include "badge_data.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include <stddef.h>

static lv_obj_t *s_scr;
static ds_dots_t s_dots;

// 固定坐标(30px 姓名放大后的无头像信息页排布)
#define PR_NAME_Y      56
#define PR_TITLE_Y     110
#define PR_DIV_Y       142
#define PR_STATUS_Y    176
#define PR_BIO_Y       212

// 状态行:强调色点 + 状态文字,成组水平居中于 y。
static void place_status(lv_obj_t *parent, int y)
{
    const char *st = badge_data_get(BADGE_FIELD_STATUS);
    if (!st || st[0] == '\0') return;

    lv_obj_t *tag = ui_label(parent, st, &badge_font_gb2312_small, DS_ACCENT);
    lv_obj_update_layout(tag);
    int tw = lv_obj_get_width(tag);
    int tx = 120 - tw / 2;
    lv_obj_set_pos(tag, tx, y);
    ui_block(parent, tx - 12 - 6, y + 3, 6, 6, DS_ACCENT);
}

// 居中放置一行文字于 y(空串跳过)。
static void place_line(lv_obj_t *parent, int y, const char *text,
                       const lv_font_t *font, uint32_t color)
{
    if (!text || text[0] == '\0') return;
    lv_obj_t *lbl = ui_label(parent, text, font, color);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, y);
}

void profile_enter(app_page_t page)
{
    (void)page;

    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    ds_header(s_scr, "PROFILE", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    // 纯文字档案(固定坐标,无头像):姓名(30px)→ 职位 → 分隔线 → 状态 → 简介
    place_line(s_scr, PR_NAME_Y, badge_data_get(BADGE_FIELD_NAME),
               &badge_font_gb2312, DS_TEXT_PRIMARY);
    place_line(s_scr, PR_TITLE_Y, badge_data_get(BADGE_FIELD_TITLE),
               &badge_font_gb2312_small, DS_TEXT_SECONDARY);

    // 有职位或简介时都画分隔线(分隔姓名/职位组与下方信息),职位为空则跳过。
    bool has_title = badge_data_get(BADGE_FIELD_TITLE)[0] != '\0';
    bool has_bio   = badge_data_get(BADGE_FIELD_BIO)[0] != '\0';
    if (has_title || has_bio) {
        ui_block(s_scr, 120 - 36, PR_DIV_Y, 72, 2, DS_ACCENT);
    }

    place_status(s_scr, PR_STATUS_Y);
    place_line(s_scr, PR_BIO_Y, badge_data_get(BADGE_FIELD_BIO),
               &badge_font_gb2312_small, DS_TEXT_SECONDARY);

    // Footer:Page Indicator(PROFILE=第 1 点实心)
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_PROFILE);

    lv_screen_load(s_scr);
}

void profile_exit(void)
{
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
}