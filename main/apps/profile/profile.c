// main/apps/profile/profile.c —— Passport OS V2 第 1 页 PROFILE(完整档案)页。
//
// 布局(参考 docs/UI_DESIGN_SPEC.md §7):
//   Header(0-35)      :ds_header("PROFILE" + 电量)
//   Content(36-271)   :头像(缩放到约 64 宽,居中上段)
//                      姓名(24px) / 职位(14px) / 简介 bio(14px) / 网站(14px)
//   Footer(272-319)   :Page Indicator(PROFILE=第 1 点实心)
//
// 数据显示动态字段(绝不写死进固件):name/title/bio/website 来自 badge_data(NVS)。
// 空字段自动跳过,避免空行。头像为现有素材等比缩放;真 80x80 头像在 TASK-12。
#include "profile.h"
#include "badge_data.h"
#include "badge_avatar.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include <stddef.h>
#include <stdint.h>

static lv_obj_t *s_scr;
static ds_dots_t s_dots;

// 头像缩放:现有素材 80x157 → 64 宽,等比高约 125(256=100%)。
#define PROFILE_AVATAR_W   64
#define PROFILE_AVATAR_SCALE  (PROFILE_AVATAR_W * 256 / 80)

// 布局 y(Content 36-271)
#define PROFILE_AVATAR_Y   48
#define PROFILE_NAME_Y     184
#define PROFILE_TITLE_Y    216
#define PROFILE_BIO_Y      236
#define PROFILE_WEB_Y      256

// 居中放置一个行标签(忽略空串,返回是否显示)。
static bool add_centered_line(lv_obj_t *parent, const char *text, const lv_font_t *font,
                              uint32_t color, int y)
{
    if (!text || text[0] == '\0') return false;
    lv_obj_t *lbl = ui_label(parent, text, font, color);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, y);
    return true;
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

    // 头像:等比缩小后居中;显式设置尺寸避免布局重叠。
    lv_obj_t *avatar = lv_image_create(s_scr);
    lv_image_set_src(avatar, &badge_avatar);
    lv_image_set_scale(avatar, PROFILE_AVATAR_SCALE);
    uint32_t av_h = badge_avatar.header.h * PROFILE_AVATAR_SCALE / 256;
    lv_obj_set_size(avatar, PROFILE_AVATAR_W, av_h);
    lv_obj_align(avatar, LV_ALIGN_TOP_MID, 0, PROFILE_AVATAR_Y);

    // 档案字段(空字段自动跳过)
    add_centered_line(s_scr, badge_data_get(BADGE_FIELD_NAME), &badge_font_gb2312,
                      DS_TEXT_PRIMARY, PROFILE_NAME_Y);
    add_centered_line(s_scr, badge_data_get(BADGE_FIELD_TITLE), &badge_font_gb2312_small,
                      DS_TEXT_SECONDARY, PROFILE_TITLE_Y);
    add_centered_line(s_scr, badge_data_get(BADGE_FIELD_BIO), &badge_font_gb2312_small,
                      DS_TEXT_SECONDARY, PROFILE_BIO_Y);
    add_centered_line(s_scr, badge_data_get(BADGE_FIELD_WEBSITE), &badge_font_gb2312_small,
                      DS_TEXT_SECONDARY, PROFILE_WEB_Y);

    // Footer:Page Indicator(PROFILE=第 1 点实心)
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_PROFILE);

    lv_screen_load(s_scr);
}

void profile_exit(void)
{
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
}