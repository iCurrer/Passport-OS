// main/app/app_list.c —— 列表页三态导航控制器实现（详见 app_list.h）。
//
// 状态机:
//   ENTRY →(OK)→ MENU →(OK)→ CHILD →(OK 长按)→ MENU →(OK 长按)→ ENTRY
//   ENTRY 态 UP/DOWN 短按不消费(交全局翻页),修复列表页无法翻页的问题。
//   长按:ENTRY=回首页(交全局);MENU/CHILD=返回上一级。
//
// 线程:所有 lv_* 在锁内调用;本文件不自行加锁。
#include "app_list.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include <stdlib.h>

typedef enum {
    APP_LIST_ST_ENTRY = 0,   // 入口态(默认)
    APP_LIST_ST_MENU,        // 菜单态
    APP_LIST_ST_CHILD,       // 子屏
} app_list_st_t;

struct app_list_ctl {
    app_list_cfg_t cfg;
    app_list_st_t  st;
    lv_obj_t      *scr;      // 当前该控制器持有的根(ENTRY/MENU),CHILD 时非 NULL 表示待清理的 MENU
    ds_dots_t      dots;
    lv_obj_t      *row_bg[8];    // 菜单行背景(≤8)
    lv_obj_t      *row_name[8];  // 菜单行名称
    lv_obj_t      *row_val[8];   // 菜单行右侧值(可空列)
    int            sel;          // 当前选中(菜单态)
};

// --------------------------------------------------------------------------
// 布局(与既有列表页一致,见 UI spec §11)
// --------------------------------------------------------------------------
#define LIST_X       16
#define LIST_W       208
#define ROW_H        44
#define ROW_Y0       56
#define ROW_GAP      48
#define LABEL_X      28
#define LABEL_Y_OFS  15
#define VALUE_X      150

#define ENTRY_TITLE_Y   96
#define ENTRY_SUB_Y     160
#define ENTRY_HINT_Y    208

// --------------------------------------------------------------------------
// 渲染
// --------------------------------------------------------------------------

static void refresh_sel(app_list_ctl_t *ctl);   // 前置声明

// 建屏幕根。
static lv_obj_t *scr_new(void)
{
    lv_obj_t *s = lv_obj_create(NULL);
    lv_obj_remove_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
    return s;
}

// 清理当前持有的根(仅 ENTRY/MENU 由控制器持有;CHILD 由页面回调构建并自行清理)。
static void clear_scr(app_list_ctl_t *ctl)
{
    if (ctl->scr) { lv_obj_delete(ctl->scr); ctl->scr = NULL; }
}

// 渲染 ENTRY 入口态。
static void render_entry(app_list_ctl_t *ctl)
{
    clear_scr(ctl);
    lv_obj_t *s = scr_new();
    ctl->scr = s;

    ds_header(s, ctl->cfg.title, bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    lv_obj_t *t = ui_label(s, ctl->cfg.intro ? ctl->cfg.intro : ctl->cfg.title,
                           &badge_font_gb2312, DS_TEXT_PRIMARY);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, ENTRY_TITLE_Y);

    if (ctl->cfg.sub && ctl->cfg.sub[0]) {
        lv_obj_t *sub = ui_label(s, ctl->cfg.sub, &badge_font_gb2312_small, DS_TEXT_SECONDARY);
        lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, ENTRY_SUB_Y);
    }

    lv_obj_t *h = ui_label(s, "OK TO ENTER", &badge_font_gb2312_small, DS_ACCENT);
    lv_obj_align(h, LV_ALIGN_TOP_MID, 0, ENTRY_HINT_Y);

    ds_footer(s, &ctl->dots, APP_PAGE_COUNT, ctl->cfg.page);
    lv_screen_load(s);
}

// 渲染 MENU 菜单态(复用列表布局)。仅在进入 MENU/从 CHILD 返回 MENU 时重建。
static void render_menu(app_list_ctl_t *ctl)
{
    clear_scr(ctl);
    lv_obj_t *s = scr_new();
    ctl->scr = s;

    ds_header(s, ctl->cfg.title, bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    for (int i = 0; i < ctl->cfg.item_count; i++) {
        int y = ROW_Y0 + i * ROW_GAP;
        ctl->row_bg[i]   = ui_block(s, LIST_X, y, LIST_W, ROW_H, DS_BG);
        ctl->row_name[i] = ui_label(s, ctl->cfg.items[i], &badge_font_gb2312_small,
                                    DS_TEXT_SECONDARY);
        lv_obj_set_pos(ctl->row_name[i], LABEL_X, y + LABEL_Y_OFS);
        if (ctl->cfg.value_cb) {
            ctl->row_val[i] = ui_label(s, ctl->cfg.value_cb(ctl->cfg.ctx, i),
                                       &badge_font_gb2312_small, DS_TEXT_PRIMARY);
            lv_obj_set_pos(ctl->row_val[i], VALUE_X, y + LABEL_Y_OFS);
        } else {
            ctl->row_val[i] = NULL;
        }
    }
    refresh_sel(ctl);
    ds_footer(s, &ctl->dots, APP_PAGE_COUNT, ctl->cfg.page);
    lv_screen_load(s);
}

// 刷新列表选中高亮(原地改样式,不重建对象)。
static void refresh_sel(app_list_ctl_t *ctl)
{
    for (int i = 0; i < ctl->cfg.item_count; i++) {
        bool sel = (i == ctl->sel);
        lv_obj_set_style_bg_color(ctl->row_bg[i], lv_color_hex(sel ? DS_CARD : DS_BG), 0);
        lv_obj_set_style_text_color(ctl->row_name[i],
                                    lv_color_hex(sel ? DS_ACCENT : DS_TEXT_SECONDARY), 0);
        if (ctl->row_val[i] && ctl->cfg.value_cb) {
            lv_label_set_text(ctl->row_val[i], ctl->cfg.value_cb(ctl->cfg.ctx, i));
        }
    }
}

// --------------------------------------------------------------------------
// 公开 API
// --------------------------------------------------------------------------
app_list_ctl_t *app_list_create(const app_list_cfg_t *cfg)
{
    app_list_ctl_t *ctl = calloc(1, sizeof(*ctl));
    if (!ctl) return NULL;
    ctl->cfg = *cfg;
    ctl->st = APP_LIST_ST_ENTRY;
    ctl->scr = NULL;
    ctl->sel = 0;
    return ctl;
}

void app_list_destroy(app_list_ctl_t *ctl)
{
    if (!ctl) return;
    // 若在 CHILD,先让页面销毁子屏。
    if (ctl->st == APP_LIST_ST_CHILD && ctl->cfg.exit_child) {
        ctl->cfg.exit_child(ctl->cfg.ctx);
    }
    clear_scr(ctl);
    free(ctl);
}

void app_list_enter(app_list_ctl_t *ctl)
{
    if (!ctl) return;
    ctl->st = APP_LIST_ST_ENTRY;
    ctl->sel = 0;
    render_entry(ctl);
}

void app_list_goto_menu(app_list_ctl_t *ctl)
{
    if (!ctl) return;
    // 从 CHILD 返回 MENU:若子屏仍未清理(如 game 退出时由回调自己删了屏),通知页面清理。
    if (ctl->st == APP_LIST_ST_CHILD && ctl->cfg.exit_child) {
        ctl->cfg.exit_child(ctl->cfg.ctx);
    }
    ctl->st = APP_LIST_ST_MENU;
    render_menu(ctl);
}

bool app_list_key(app_list_ctl_t *ctl, bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (!ctl) return false;

    switch (ctl->st) {
    case APP_LIST_ST_ENTRY:
        if (ev == BSP_BTN_CLICK) {
            if (btn == BSP_BTN_OK) {
                // 进入 MENU。
                ctl->st = APP_LIST_ST_MENU;
                render_menu(ctl);
                return true;
            }
            // UP/DOWN 短按→不消费(交全局翻页)
            return false;
        }
        // 长按:ENTRY 态 UP/OK 应回首页 —— 不消费,交全局。
        return false;

    case APP_LIST_ST_MENU:
        if (ev == BSP_BTN_CLICK) {
            switch (btn) {
            case BSP_BTN_UP:
                ctl->sel = (ctl->sel + ctl->cfg.item_count - 1) % ctl->cfg.item_count;
                refresh_sel(ctl);
                return true;
            case BSP_BTN_DOWN:
                ctl->sel = (ctl->sel + 1) % ctl->cfg.item_count;
                refresh_sel(ctl);
                return true;
            case BSP_BTN_OK:
                if (ctl->cfg.enter_child && ctl->cfg.enter_child(ctl->cfg.ctx, ctl->sel)) {
                    // 进入子屏成功:删掉 MENU 根,交付 CHILD 态。
                    ctl->st = APP_LIST_ST_CHILD;
                    clear_scr(ctl);
                    return true;
                }
                // 原地操作(SETTINGS 开关/循环):刷新值列。
                if (ctl->cfg.value_cb) refresh_sel(ctl);
                return true;
            default:
                return false;
            }
        }
        // 长按 OK → 返回上一级(ENTRY)。
        if (ev == BSP_BTN_LONG && btn == BSP_BTN_OK) {
            ctl->st = APP_LIST_ST_ENTRY;
            render_entry(ctl);
            return true;
        }
        // 其它长按(UP/DOWN)不消费 → 交全局(回首页/DOWN 快速状态)。
        return false;

    case APP_LIST_ST_CHILD:
        // 长按 OK → 返回上一级(MENU)。
        if (ev == BSP_BTN_LONG && btn == BSP_BTN_OK) {
            app_list_goto_menu(ctl);
            return true;
        }
        // 子屏其它 key 交给页面回调。
        if (ctl->cfg.child_key) {
            return ctl->cfg.child_key(ctl->cfg.ctx, btn, ev) != APP_LIST_CHILD_KEY_NONE;
        }
        return false;

    default:
        return false;
    }
}

int app_list_state(const app_list_ctl_t *ctl)
{
    return ctl ? (int)ctl->st : -1;
}