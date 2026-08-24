// main/apps/home/home.c —— Passport OS V2 第 0 页 HOME(身份)页。
//
// 布局(参考 docs/UI_DESIGN_SPEC.md §6):
//   Header(0-35)      :ds_header(顶部文字 top + 电量)
//   Content(36-271)   :头像(缩放到约 56x110,水平居中上段)
//                      姓名(24px) / 职位(14px) / 状态(色点+文字,成组居中)
//   Footer(272-319)   :Page Indicator(HOME=第 0 点实心)
//
// 数据显示动态字段(绝不写死进固件):top/name/title/status 来自 badge_data(NVS)。
// 头像:优先显示用户经 BLE 上传的 80x80 RGB565(/avatar.bin,TASK-12),否则回退到内置 80x157 素材缩放。
#include "home.h"
#include "badge_data.h"
#include "badge_avatar.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "avatar_storage.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static lv_obj_t        *s_scr;
static ds_dots_t        s_dots;
static uint8_t         *s_avatar_buf;   // 用户头像 RGB565 缓冲
static lv_image_dsc_t   s_avatar_dsc;   // 头像 image 描述符(静态,需驻留)

// 头像缩放因子:现有素材 80x157 → 等比缩到约 46x90(256=100%)。
#define HOME_AVATAR_SCALE  147

// 用户头像显示尺寸(80x80 原图放大到 90x90):放大因子 90/80=1.125 → 288/256
#define HOME_AVATAR_DISP   90
#define HOME_AVATAR_ZOOM   288

// 布局 y(Content 36-271)。头像 90px 垂直居中于 Header 分隔线(34)与姓名之间:
//   头像顶 54(顶到 34 间距 20) → 头像底 144 → 姓名顶 164(间距 20) → 职位 198 → 状态 230
#define HOME_AVATAR_Y    54
#define HOME_NAME_Y      164
#define HOME_TITLE_Y     198
#define HOME_STATUS_Y    230
#define HOME_STATUS_GAP  12     // 状态色点到文字的水平间距

// 放置用户头像(80x80 RGB565,从 /avatar.bin 读入),放大到 90x90 显示。成功返回 true。
static bool place_user_avatar(lv_obj_t *scr)
{
    if (!avatar_storage_has()) return false;
    s_avatar_buf = malloc(AVATAR_SIZE);
    if (!s_avatar_buf) return false;
    if (avatar_storage_load(s_avatar_buf, AVATAR_SIZE) != AVATAR_SIZE) {
        free(s_avatar_buf); s_avatar_buf = NULL;
        return false;
    }
    memset(&s_avatar_dsc, 0, sizeof(s_avatar_dsc));
    s_avatar_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_avatar_dsc.header.cf    = LV_COLOR_FORMAT_RGB565;
    s_avatar_dsc.header.w     = AVATAR_W;
    s_avatar_dsc.header.h     = AVATAR_H;
    s_avatar_dsc.data_size    = AVATAR_SIZE;
    s_avatar_dsc.data         = s_avatar_buf;
    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, &s_avatar_dsc);
    lv_image_set_scale(img, HOME_AVATAR_ZOOM);          // 80→90 放大
    lv_obj_set_size(img, HOME_AVATAR_DISP, HOME_AVATAR_DISP);
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, HOME_AVATAR_Y);
    return true;
}

// 回退:内置全身素材等比缩小居中(高度对齐 90)。
static void place_sprite_avatar(lv_obj_t *scr)
{
    lv_obj_t *avatar = lv_image_create(scr);
    lv_image_set_src(avatar, &badge_avatar);
    lv_image_set_scale(avatar, HOME_AVATAR_SCALE);
    uint32_t av_w = badge_avatar.header.w * HOME_AVATAR_SCALE / 256;
    uint32_t av_h = badge_avatar.header.h * HOME_AVATAR_SCALE / 256;
    lv_obj_set_size(avatar, av_w, av_h);
    lv_obj_align(avatar, LV_ALIGN_TOP_MID, 0, HOME_AVATAR_Y);
}

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

    // 头像:优先用户上传 80x80,否则回退内置素材。
    s_avatar_buf = NULL;
    if (!place_user_avatar(s_scr)) place_sprite_avatar(s_scr);

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
    if (s_avatar_buf) { free(s_avatar_buf); s_avatar_buf = NULL; }
    // s_dots 引用的圆点对象随 screen 一并删除,不再使用
}