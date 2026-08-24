// main/apps/home/home.c —— Passport OS V2 第 0 页 HOME(身份)页。
//
// 布局(参考 docs/UI_DESIGN_SPEC.md §6):
//   Header(0-35)      :ds_header(顶部文字 top + 电量)
//   Content(36-271)   :头像(缩放到约 56x110,水平居中上段)
//                      姓名(24px) / 职位(14px) / 状态(色点+文字,成组居中)
//   Footer(272-319)   :Page Indicator(HOME=第 0 点实心)
//
// 数据显示动态字段(绝不写死进固件):top/name/title/status 来自 badge_data(NVS)。
// 头像当前为现有 80x157 全身素材的等比缩放;真正的 80x80 自定义头像文件在 TASK-12。
#include "home.h"
#include "badge_data.h"
#include "badge_avatar.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include <stddef.h>

static lv_obj_t *s_scr;
static ds_dots_t s_dots;

// 头像缩放因子:现有素材 80x157 → 等比缩到 约56x110(256=100%)。
#define HOME_AVATAR_SCALE  180

// 布局 y(Content 36-271)
#define HOME_AVATAR_Y    56
#define HOME_NAME_Y      178
#define HOME_TITLE_Y     212
#define HOME_STATUS_Y    244
#define HOME_STATUS_GAP  12     // 状态色点到文字的水平间距

void home_enter(app_page_t page)
{
    (void)page;

    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    // Header:顶部文字(可自定义)+ 电量
    ds_header(s_scr, badge_data_get(BADGE_FIELD_TOP), bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    // 头像:等比缩小后水平居中。显式设置缩放后尺寸,确保后续布局按实际渲染区域计算(解决与姓名重叠)。
    lv_obj_t *avatar = lv_image_create(s_scr);
    lv_image_set_src(avatar, &badge_avatar);
    lv_image_set_scale(avatar, HOME_AVATAR_SCALE);
    uint32_t av_w = badge_avatar.header.w * HOME_AVATAR_SCALE / 256;
    uint32_t av_h = badge_avatar.header.h * HOME_AVATAR_SCALE / 256;
    lv_obj_set_size(avatar, av_w, av_h);
    lv_obj_align(avatar, LV_ALIGN_TOP_MID, 0, HOME_AVATAR_Y);

    // 姓名(主字号)
    lv_obj_t *name = ui_label(s_scr, badge_data_get(BADGE_FIELD_NAME),
                              &badge_font_gb2312, DS_TEXT_PRIMARY);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, HOME_NAME_Y);

    // 职位(次字号)
    lv_obj_t *title = ui_label(s_scr, badge_data_get(BADGE_FIELD_TITLE),
                               &badge_font_gb2312_small, DS_TEXT_SECONDARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HOME_TITLE_Y);

    // 状态:色点 + 文字,成组水平居中
    lv_obj_t *tag = ui_label(s_scr, badge_data_get(BADGE_FIELD_STATUS),
                             &badge_font_gb2312_small, DS_ACCENT);
    lv_obj_update_layout(tag);
    int tw = lv_obj_get_width(tag);
    int tx = 120 - tw / 2;
    lv_obj_set_pos(tag, tx, HOME_STATUS_Y);
    ui_block(s_scr, tx - HOME_STATUS_GAP - 6, HOME_STATUS_Y + 3, 6, 6, DS_ACCENT);

    // Footer:Page Indicator(HOME=第 0 页实心)
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_HOME);

    lv_screen_load(s_scr);
}

void home_exit(void)
{
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    // s_dots 引用的圆点对象随 screen 一并删除,不再使用
}