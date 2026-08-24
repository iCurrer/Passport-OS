// main/apps/profile/profile.c —— Passport OS V2 第 1 页 PROFILE(完整档案)页。
//
// 布局(参考 docs/UI_DESIGN_SPEC.md §7,纯文字版):
//   Header(0-35)      :ds_header("PROFILE" + 电量)
//   Content(36-271)   :姓名(24px 主) → 职位(14px 次) → 强调分隔线 → 状态(色点+文字)
//                      简介 bio(14px) → 网站(14px) → GitHub(14px)
//   Footer(272-319)   :Page Indicator(PROFILE=第 1 点实心)
//
// 原则:极简、高对比、低信息密度。字段来自 badge_data(NVS),空字段自动收起(不留空行)。
// 使用垂直累计 y 游标,空字段不推进,保证排版紧凑。
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
static int s_y;                    // 内容区垂直累计游标

// 在 y 游标处放置一个水平居中的行(空串跳过,不推进游标)。返回是否放置。
static bool place_line(lv_obj_t *parent, const char *text, const lv_font_t *font,
                       uint32_t color, int advance)
{
    if (!text || text[0] == '\0') return false;
    lv_obj_t *lbl = ui_label(parent, text, font, color);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, s_y);
    s_y += advance;
    return true;
}

// 状态行:强调色点 + 状态文字,成组水平居中。
static void place_status(lv_obj_t *parent)
{
    const char *st = badge_data_get(BADGE_FIELD_STATUS);
    if (!st || st[0] == '\0') return;

    lv_obj_t *tag = ui_label(parent, st, &badge_font_gb2312_small, DS_ACCENT);
    lv_obj_update_layout(tag);
    int tw = lv_obj_get_width(tag);
    int tx = 120 - tw / 2;
    lv_obj_set_pos(tag, tx, s_y);
    ui_block(parent, tx - 12 - 6, s_y + 3, 6, 6, DS_ACCENT);
    s_y += 30;
}

// 强调分隔线(水平居中,宽度适中)。
static void place_divider(lv_obj_t *parent)
{
    ui_block(parent, 120 - 36, s_y, 72, 2, DS_ACCENT);
    s_y += 26;
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

    // 纯文字档案:姓名 → 职位 → 分隔线 → 状态 → 简介 → 网站 → GitHub
    s_y = 56;
    place_line(s_scr, badge_data_get(BADGE_FIELD_NAME), &badge_font_gb2312,
               DS_TEXT_PRIMARY, 42);
    place_line(s_scr, badge_data_get(BADGE_FIELD_TITLE), &badge_font_gb2312_small,
               DS_TEXT_SECONDARY, 24);
    place_divider(s_scr);
    place_status(s_scr);
    place_line(s_scr, badge_data_get(BADGE_FIELD_BIO), &badge_font_gb2312_small,
               DS_TEXT_SECONDARY, 26);
    place_line(s_scr, badge_data_get(BADGE_FIELD_WEBSITE), &badge_font_gb2312_small,
               DS_TEXT_SECONDARY, 26);
    place_line(s_scr, badge_data_get(BADGE_FIELD_GITHUB), &badge_font_gb2312_small,
               DS_TEXT_SECONDARY, 26);

    // Footer:Page Indicator(PROFILE=第 1 点实心)
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_PROFILE);

    lv_screen_load(s_scr);
}

void profile_exit(void)
{
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
}