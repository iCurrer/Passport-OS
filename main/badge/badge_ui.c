// main/badge/badge_ui.c —— 磨砂绿「心情电子名牌」界面(LVGL 布局与渲染)。
//
// 屏幕:ST7789P3 240x320,有效显示区约 y=50..285。
// 布局:右上角电量 → 姓名(大字) → 状态(主题色) → 英文小字 → 表情小人。
//
// 线程规则:
//   - LVGL 对象只在 LVGL 锁内操作(见 badge_ui_set_field)。
//   - 电量刷新用 lv_timer(跑在 LVGL 任务,已持锁)。
#include "badge_ui.h"
#include "badge_data.h"
#include "badge_fonts.h"
#include "badge_avatar.h"
#include "bsp_display.h"          // bsp_lvgl_lock / unlock
#include "bsp_battery.h"
#include "ui_pixel.h"             // ui_block / ui_label(共用绘制原语)
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "badge_ui";

// ---------------------------------------------------------------------------
// 主题色(磨砂绿)
// ---------------------------------------------------------------------------
#define BG_COLOR        0x1E352C   // 磨砂绿背景(提亮的深墨绿,哑光)
#define TXT_PRIMARY     0xF4F8F5   // 姓名主文字(近白微绿)
#define TXT_MUTED       0xAFC0B6   // 次要文字(灰绿)
#define BATT_NORMAL 0xF4F8F5   // 正常电量:近白填充
#define BATT_WARN   0xF0A030   // 低电量(<20%):橙色
#define BATT_LOW    0xE04545   // 很低电量(<10%):红色

// ---------------------------------------------------------------------------
// 布局规范(240x320,有效显示区约 y=50..285)
//   网格:左右统一留白 LAYOUT_MARGIN_X;头部/主体/底部三段式。
//   层级:姓名(主,24px 白)> 职位(次,14px 灰)> 状态(辅,14px 主题色+圆点)。
//   坐标改动只改这里,不在 UI 代码里散落魔法数字。
// ---------------------------------------------------------------------------
#define ACCENT          0x4CD964   // 主题强调色(充电绿):状态文字/圆点/dock 选中
#define LINE_DIV        0x2A473B   // 分隔线色(深绿)
#define DOCK_BG         0x16241C   // 底部 dock 背景(深绿)

#define LAYOUT_MARGIN_X     22     // 左右留白(与头像/信息列左边距一致)
#define LAYOUT_HEAD_LINE    40     // 头部底部分隔线 y
#define LAYOUT_AVATAR_X     22     // 左侧头像
#define LAYOUT_AVATAR_Y     80
#define INFO_CENTER_X       171    // 右侧信息列水平居中的轴(头像右缘102 到屏幕240 的中心)
#define LAYOUT_NAME_Y       108    // 姓名(主,24px)
#define LAYOUT_DIV_Y        140    // 姓名下细分隔线(宽度 64)
#define LAYOUT_TITLE_Y      152    // 职位(次,14px)
#define LAYOUT_STATUS_Y     172    // 状态文字(辅,14px)
#define LAYOUT_STATUS_DOT_Y 177    // 状态前置圆点(6x6)
#define LAYOUT_DOCK_LINE    262    // dock 顶部分隔线
#define LAYOUT_DOCK_Y       264    // dock 背景起点

#define DOCK_COUNT  2          // 底部 dock 图标数

// ---------------------------------------------------------------------------
// UI 对象
// ---------------------------------------------------------------------------
static lv_obj_t *s_scr;
static lv_obj_t *s_brand_lbl;              // 顶部导航栏文字
static lv_obj_t *s_name_lbl;               // 姓名
static lv_obj_t *s_title_lbl;              // 职位(豆包大学)
static lv_obj_t *s_tag_lbl;                // 状态文字(主题色,前置圆点)
static lv_obj_t *s_div;                    // 姓名下细分隔线(随姓名宽度伸缩)
static lv_obj_t *s_tag_dot;                // 状态前置圆点
static lv_obj_t *s_avatar;                 // 左侧自定义像素形象
static lv_obj_t *s_batt_fill;              // 电量条填充块
static lv_obj_t *s_batt_txt;               // 电量百分比文字(图标右侧)
static lv_timer_t *s_batt_timer;           // 电量刷新
static lv_obj_t *s_dock;                   // 底部 dock 容器

static int s_dock_sel = 0;                 // 底部 dock 当前选中项

static void update_info_position(void);

// 更新右上角电量条。lv_timer 跑在 LVGL 任务,已持锁。
static void update_battery(void)
{
    int soc = bsp_battery_soc();
    int fill_w = (soc >= 0) ? (16 * soc) / 100 : 0;   // 电池内芯 16px 宽
    lv_obj_set_width(s_batt_fill, fill_w);

    // 填充色:正常=白,<20%=橙,<10%=红
    uint32_t c = BATT_NORMAL;
    if (soc >= 0 && soc < 10)      c = BATT_LOW;
    else if (soc >= 0 && soc < 20) c = BATT_WARN;
    lv_obj_set_style_bg_color(s_batt_fill, lv_color_hex(c), 0);

    // 电量数字:取整到十位(100/90/80...),不显示精确值
    char buf[16];
    if (soc < 0) lv_label_set_text(s_batt_txt, "--");
    else {
        int r = (soc + 5) / 10 * 10;
        if (r > 100) r = 100;
        snprintf(buf, sizeof(buf), "%d%%", r);
        lv_label_set_text(s_batt_txt, buf);
    }
}

static void batt_tick(lv_timer_t *t) { (void)t; update_battery(); }

// 底部 dock 占位图标:带边框的方块 + 中心圆点。选中项顶部加主题色指示条。
static void dock_icon(lv_obj_t *parent, int x, int y, bool sel)
{
    ui_block(parent, x, y, 34, 3, sel ? ACCENT : 0x2C4A3E);   // 顶部 3px 指示条
    uint32_t b = sel ? TXT_PRIMARY : 0x3D624F;
    uint32_t d = sel ? ACCENT : 0x3D624F;
    ui_block(parent, x, y + 9, 34, 30, b);
    ui_block(parent, x + 2, y + 11, 30, 26, DOCK_BG);
    ui_block(parent, x + 13, y + 19, 8, 8, d);
}

// 重建底部 dock(两个图标),高亮当前选中项。须持 LVGL 锁。
static void dock_draw(void)
{
    if (s_dock) { lv_obj_delete(s_dock); s_dock = NULL; }
    s_dock = lv_obj_create(s_scr);
    lv_obj_remove_flag(s_dock, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_dock, 0, LAYOUT_DOCK_Y);
    lv_obj_set_size(s_dock, 240, 320 - LAYOUT_DOCK_Y);   // 容器高度延伸到屏幕底
    lv_obj_set_style_bg_opa(s_dock, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_dock, 0, 0);
    lv_obj_set_style_pad_all(s_dock, 0, 0);
    dock_icon(s_dock, 72, 6, s_dock_sel == 0);
    dock_icon(s_dock, 132, 6, s_dock_sel == 1);
}

// 右侧信息列以 INFO_CENTER_X 为轴水平居中(姓名/下划线/职位/状态),2/3/4字都不偏左不贴右。须持 LVGL 锁。
static void update_info_position(void)
{
    if (!s_name_lbl) return;
    lv_obj_update_layout(s_name_lbl);
    int name_w = lv_obj_get_width(s_name_lbl);
    lv_obj_set_pos(s_name_lbl, INFO_CENTER_X - name_w / 2, LAYOUT_NAME_Y);
    if (s_div) { lv_obj_set_width(s_div, name_w); lv_obj_set_pos(s_div, INFO_CENTER_X - name_w / 2, LAYOUT_DIV_Y); }
    if (s_title_lbl) { lv_obj_update_layout(s_title_lbl); int w = lv_obj_get_width(s_title_lbl); lv_obj_set_pos(s_title_lbl, INFO_CENTER_X - w / 2, LAYOUT_TITLE_Y); }
    if (s_tag_lbl) {
        lv_obj_update_layout(s_tag_lbl);
        int w = lv_obj_get_width(s_tag_lbl);
        int x = INFO_CENTER_X - w / 2;
        lv_obj_set_pos(s_tag_lbl, x, LAYOUT_STATUS_Y);
        if (s_tag_dot) lv_obj_set_pos(s_tag_dot, x - 14, LAYOUT_STATUS_DOT_Y);
    }
}

void badge_ui_init(void)
{
    // 根屏:磨砂黑背景
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(BG_COLOR), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    // ===================== 头部导航栏 =====================
    // 左上:品牌(小字,弱);右上:电量。底部细分隔线。
    s_brand_lbl = ui_label(s_scr, badge_data_get(BADGE_FIELD_TOP), &badge_font_gb2312_small, TXT_MUTED);
    lv_obj_set_pos(s_brand_lbl, LAYOUT_MARGIN_X, 13);

    // 右上:电量条(无尖尖),图标与数字留小间距
    ui_block(s_scr, 158, 14, 20, 10, TXT_MUTED);               // 电池外框
    ui_block(s_scr, 160, 16, 16, 6, BG_COLOR);                 // 外框内芯
    s_batt_fill = ui_block(s_scr, 160, 16, 0, 6, BATT_NORMAL); // 电量填充
    s_batt_txt = ui_label(s_scr, "", &lv_font_montserrat_14, TXT_MUTED);
    lv_obj_set_pos(s_batt_txt, 186, 12);

    // ===================== 主体:左头像 + 右信息 =====================
    // 左侧头像:主视觉,垂直居中对齐信息列。
    s_avatar = lv_image_create(s_scr);
    lv_image_set_src(s_avatar, &badge_avatar);
    lv_obj_set_pos(s_avatar, LAYOUT_AVATAR_X, LAYOUT_AVATAR_Y);

    // 右侧信息列:以 INFO_CENTER_X 为轴水平居中。层级 姓名(主)> 职位(次)> 状态(辅)。
    s_name_lbl = ui_label(s_scr, badge_data_get(BADGE_FIELD_NAME), &badge_font_gb2312, TXT_PRIMARY);

    // 姓名下细分隔线:与姓名同宽,随姓名长度自动伸缩。
    s_div = ui_block(s_scr, INFO_CENTER_X, LAYOUT_DIV_Y, 1, 2, LINE_DIV);

    s_title_lbl = ui_label(s_scr, badge_data_get(BADGE_FIELD_TITLE), &badge_font_gb2312_small, TXT_MUTED);

    // 状态:前置主题色圆点指示 + 主题色文字(辅)。
    s_tag_dot = ui_block(s_scr, INFO_CENTER_X, LAYOUT_STATUS_DOT_Y, 6, 6, ACCENT);
    s_tag_lbl = ui_label(s_scr, badge_data_get(BADGE_FIELD_STATUS), &badge_font_gb2312_small, ACCENT);

    update_info_position();   // 统一居中排版(姓名/下划线/职位/状态)

    // ===================== 底部 dock 功能区 =====================
    ui_block(s_scr, 0, LAYOUT_DOCK_LINE, 240, 2, LINE_DIV);       // dock 顶部分隔线
    ui_block(s_scr, 0, LAYOUT_DOCK_Y, 240, 320 - LAYOUT_DOCK_Y, DOCK_BG);   // dock 背景(延伸到屏幕底)
    dock_draw();                            // 两个占位图标(高亮当前选中)

    update_battery();

    s_batt_timer = lv_timer_create(batt_tick, 1000, NULL);

    lv_screen_load(s_scr);
}

void badge_ui_set_field(badge_field_t field)
{
    const char *buf = badge_data_get(field);
    if (!bsp_lvgl_lock(300)) return;
    if (s_scr) {
        switch (field) {
        case BADGE_FIELD_NAME:   if (s_name_lbl)  lv_label_set_text(s_name_lbl, buf);  break;
        case BADGE_FIELD_TOP:    if (s_brand_lbl) lv_label_set_text(s_brand_lbl, buf); break;
        case BADGE_FIELD_TITLE:  if (s_title_lbl) lv_label_set_text(s_title_lbl, buf); break;
        case BADGE_FIELD_STATUS: if (s_tag_lbl)   lv_label_set_text(s_tag_lbl, buf);   break;
        default: break;
        }
        if (field == BADGE_FIELD_NAME || field == BADGE_FIELD_TITLE || field == BADGE_FIELD_STATUS)
            update_info_position();   // 文字宽度变化后重排居中
    }
    bsp_lvgl_unlock();
}

static void dock_select(int sel)
{
    s_dock_sel = sel;
    if (!bsp_lvgl_lock(300)) return;
    dock_draw();
    bsp_lvgl_unlock();
}

void badge_ui_dock_prev(void) { dock_select((s_dock_sel + DOCK_COUNT - 1) % DOCK_COUNT); }
void badge_ui_dock_next(void) { dock_select((s_dock_sel + 1) % DOCK_COUNT); }