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
#include "game.h"
#include "settings.h"                 // 像素游戏入口
#include "lvgl.h"

// ---------------------------------------------------------------------------
// 主题色 + 布局(集中定义于 badge_theme.h,此处 include)
// ---------------------------------------------------------------------------
#include "badge_theme.h"

// ---------------------------------------------------------------------------
// 布局常量(名牌主页专用)
// ---------------------------------------------------------------------------

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
#define LAYOUT_DOCK_LINE    250    // dock 顶部分隔线
#define LAYOUT_DOCK_Y       252    // dock 背景起点

#define DOCK_COUNT  2              // 底部 dock 图标数
#define DOCK_SLOT_W 56             // 单个槽位宽(图标+文字居中基准)
#define DOCK_SLOT_H 56             // 单个槽位高
#define DOCK_GAP    24             // 槽位间距

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
    uint32_t c = BADGE_BATT_NORMAL;
    if (soc >= 0 && soc < 10)      c = BADGE_BATT_LOW;
    else if (soc >= 0 && soc < 20) c = BADGE_BATT_WARN;
    lv_obj_set_style_bg_color(s_batt_fill, lv_color_hex(c), 0);

    // 电量数字:取整到十位(100/90/80...),不显示精确值
    char buf[16];
    ui_battery_pct(soc, buf, sizeof(buf));
    lv_label_set_text(s_batt_txt, buf);
}

static void batt_tick(lv_timer_t *t) { (void)t; update_battery(); }

// 底部 dock 图标(使用 LVGL 内置符号 + 文字标签)。
// 每个槽位是一个透明容器,图标/文字通过 lv_obj_align 在槽内居中,
// 避免之前 set_pos 把左上角当锚点导致整体右偏。
// 索引 0=游戏(LV_SYMBOL_PLAY), 索引 1=设置(LV_SYMBOL_SETTINGS)。
// 选中时符号/文字变主题色,底部加 3px 指示条。
static void draw_dock_icon(lv_obj_t *parent, int idx, bool sel)
{
    const char *sym   = (idx == 0) ? LV_SYMBOL_PLAY : LV_SYMBOL_SETTINGS;
    const char *label = (idx == 0) ? "Game" : "Set";
    uint32_t c = sel ? BADGE_ACCENT : 0x3D624F;

    // 透明槽容器(作为图标/文字对齐基准,整组居中于屏幕)
    lv_obj_t *slot = lv_obj_create(parent);
    lv_obj_remove_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(slot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(slot, 0, 0);
    lv_obj_set_style_pad_all(slot, 0, 0);
    int total = DOCK_COUNT * DOCK_SLOT_W + (DOCK_COUNT - 1) * DOCK_GAP;
    lv_obj_set_pos(slot, (240 - total) / 2 + idx * (DOCK_SLOT_W + DOCK_GAP), 0);
    lv_obj_set_size(slot, DOCK_SLOT_W, DOCK_SLOT_H);

    // 图标符号(水平居中,偏上;用 20px 符号字体放大显示)
    lv_obj_t *icon = lv_label_create(slot);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(c), 0);
    lv_label_set_text(icon, sym);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -13);

    // 文字标签(水平居中,偏下)
    lv_obj_t *lbl = ui_label(slot, label, &badge_font_gb2312_small, c);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 12);

    // 选中:底部 3px 主题色指示条
    if (sel) ui_block(slot, 0, DOCK_SLOT_H - 3, DOCK_SLOT_W, 3, BADGE_ACCENT);
}

// 重建底部 dock(多个图标),高亮当前选中项。须持 LVGL 锁。
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
    for (int i = 0; i < DOCK_COUNT; i++) {
        draw_dock_icon(s_dock, i, s_dock_sel == i);
    }
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
    // 幂等:重复进入(如从子页面返回)前清理上次的定时器与 screen,避免泄漏。
    if (s_batt_timer) { lv_timer_delete(s_batt_timer); s_batt_timer = NULL; }
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }

    // 根屏:磨砂黑背景
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(BADGE_BG_COLOR), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    // ===================== 头部导航栏(复用 ui_header_bar) =====================
    ui_header_result_t hdr = ui_header_bar(s_scr, badge_data_get(BADGE_FIELD_TOP),
                                           bsp_battery_soc(),
                                           BADGE_TXT_MUTED, BADGE_BG_COLOR,
                                           BADGE_BATT_NORMAL, BADGE_LINE_DIV,
                                           &badge_font_gb2312_small, &lv_font_montserrat_14);
    s_brand_lbl = hdr.brand;    // 持有引用,供 badge_ui_set_field 更新品牌文字
    s_batt_fill = hdr.fill;
    s_batt_txt  = hdr.pct;      // 电量百分比文字(供 s_batt_timer 定时刷新)

    // ===================== 主体:左头像 + 右信息 =====================
    // 左侧头像:主视觉,垂直居中对齐信息列。
    s_avatar = lv_image_create(s_scr);
    lv_image_set_src(s_avatar, &badge_avatar);
    lv_obj_set_pos(s_avatar, LAYOUT_AVATAR_X, LAYOUT_AVATAR_Y);

    // 右侧信息列:以 INFO_CENTER_X 为轴水平居中。层级 姓名(主)> 职位(次)> 状态(辅)。
    s_name_lbl = ui_label(s_scr, badge_data_get(BADGE_FIELD_NAME), &badge_font_gb2312, BADGE_TXT_PRIMARY);

    // 姓名下细分隔线:与姓名同宽,随姓名长度自动伸缩。
    s_div = ui_block(s_scr, INFO_CENTER_X, LAYOUT_DIV_Y, 1, 2, BADGE_LINE_DIV);

    s_title_lbl = ui_label(s_scr, badge_data_get(BADGE_FIELD_TITLE), &badge_font_gb2312_small, BADGE_TXT_MUTED);

    // 状态:前置主题色圆点指示 + 主题色文字(辅)。
    s_tag_dot = ui_block(s_scr, INFO_CENTER_X, LAYOUT_STATUS_DOT_Y, 6, 6, BADGE_ACCENT);
    s_tag_lbl = ui_label(s_scr, badge_data_get(BADGE_FIELD_STATUS), &badge_font_gb2312_small, BADGE_ACCENT);

    update_info_position();   // 统一居中排版(姓名/下划线/职位/状态)

    // ===================== 底部 dock 功能区 =====================
    ui_block(s_scr, 0, LAYOUT_DOCK_LINE, 240, 2, BADGE_LINE_DIV);       // dock 顶部分隔线
    ui_block(s_scr, 0, LAYOUT_DOCK_Y, 240, 320 - LAYOUT_DOCK_Y, BADGE_DOCK_BG);   // dock 背景(延伸到屏幕底)
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

void badge_ui_dock_enter(void)
{
    // 子页面 enter 会创建 LVGL screen,必须在 LVGL 锁内调用。
    if (s_dock_sel == 0) {
        g_badge_sub = BADGE_SUB_GAME;
        if (!bsp_lvgl_lock(300)) return;
        game_enter();   // 进入像素游戏
        bsp_lvgl_unlock();
    } else if (s_dock_sel == 1) {
        g_badge_sub = BADGE_SUB_SETTINGS;
        if (!bsp_lvgl_lock(300)) return;
        settings_enter();   // 进入设置页面
        bsp_lvgl_unlock();
    }
}