// main/apps/games/games.c —— Passport OS V2 第 6 页 GAMES(游戏菜单)页。
//
// 列表(参考 docs/UI_DESIGN_SPEC.md §12):REACTION / MEMORY / MORSE / CATCH。
// 交互同 TOOLS:列表内 UP/DOWN 选择、OK 进入;CATCH 启动现有 game.c(Pixel Catcher)。
// 全局语义:短按在页面局部优先,消费不了才翻页;长按全局(OK/UP 回 HOME)。
// 本 TASK 把 CATCH 接入现有 game.c,其余游戏 COMING SOON 占位。
#include "games.h"
#include "game.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "games";

#define GAMES_COUNT 4
enum { GAME_REACTION = 0, GAME_MEMORY, GAME_MORSE, GAME_CATCH };
static const char *s_game_names[GAMES_COUNT] = {
    "REACTION", "MEMORY", "MORSE", "CATCH",
};

// 列表布局(同 TOOLS)
#define LIST_X       16
#define LIST_W       208
#define ROW_H        44
#define ROW_Y0       56
#define ROW_GAP      48
#define LABEL_X      28
#define LABEL_Y_OFS  15

typedef enum { GAMES_LIST, GAMES_CATCH, GAMES_PLACEHOLDER } games_mode_t;

static games_mode_t s_mode = GAMES_LIST;
static int          s_sel = 0;
static lv_obj_t    *s_scr;              // 列表/占位屏
static ds_dots_t    s_dots;
static lv_obj_t    *s_row_bg[GAMES_COUNT];    // 列表行背景
static lv_obj_t    *s_row_name[GAMES_COUNT];  // 列表行名称

static void refresh_sel(void);   // 前置声明

static void build_list_screen(void)
{
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    ds_header(s_scr, "GAMES", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    // 行对象只建一次;选择刷新仅改样式
    for (int i = 0; i < GAMES_COUNT; i++) {
        int y = ROW_Y0 + i * ROW_GAP;
        s_row_bg[i] = ui_block(s_scr, LIST_X, y, LIST_W, ROW_H, DS_BG);
        s_row_name[i] = ui_label(s_scr, s_game_names[i], &badge_font_gb2312_small,
                                 DS_TEXT_SECONDARY);
        lv_obj_set_pos(s_row_name[i], LABEL_X, y + LABEL_Y_OFS);
    }
    refresh_sel();

    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_GAMES);
    lv_screen_load(s_scr);
}

// 原地刷新选中高亮(不重建对象)
static void refresh_sel(void)
{
    for (int i = 0; i < GAMES_COUNT; i++) {
        bool sel = (i == s_sel);
        lv_obj_set_style_bg_color(s_row_bg[i], lv_color_hex(sel ? DS_CARD : DS_BG), 0);
        lv_obj_set_style_text_color(s_row_name[i],
                                    lv_color_hex(sel ? DS_ACCENT : DS_TEXT_SECONDARY), 0);
    }
}

static void enter_game(int idx)
{
    lv_obj_delete(s_scr); s_scr = NULL;

    if (idx == GAME_CATCH) {
        s_mode = GAMES_CATCH;
        game_enter();                 // 现有 Pixel Catcher 全屏接管
        ESP_LOGI(TAG, "start CATCH");
        return;
    }

    // 占位游戏屏
    s_mode = GAMES_PLACEHOLDER;
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);
    ds_header(s_scr, s_game_names[idx], bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);
    lv_obj_t *lbl = ui_label(s_scr, "COMING SOON", &badge_font_gb2312_small,
                             DS_TEXT_SECONDARY);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 120);
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_GAMES);
    lv_screen_load(s_scr);
}

void games_enter(app_page_t page)
{
    (void)page;
    s_mode = GAMES_LIST;
    s_sel = 0;
    build_list_screen();
}

void games_exit(void)
{
    if (s_mode == GAMES_CATCH) game_exit();   // 停止游戏循环并删其屏
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    s_mode = GAMES_LIST;
}

bool games_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (ev != BSP_BTN_CLICK) return false;   // 长按仍走全局(返回 HOME 等)

    if (s_mode == GAMES_CATCH) {
        game_key_result_t r = game_key(btn, ev);
        if (r == GAME_KEY_EXITED) {
            s_mode = GAMES_LIST;
            build_list_screen();             // 游戏结束,回到菜单
        }
        return true;                          // 游戏内按键一律消费
    }

    if (s_mode == GAMES_LIST) {
        switch (btn) {
        case BSP_BTN_UP:   s_sel = (s_sel + GAMES_COUNT - 1) % GAMES_COUNT; break;
        case BSP_BTN_DOWN: s_sel = (s_sel + 1) % GAMES_COUNT;                break;
        case BSP_BTN_OK:   enter_game(s_sel); return true;
        default: return false;
        }
        refresh_sel();   // 原地刷新高亮
        return true;
    }

    // 占位游戏屏:按键不消费,交给全局(UP/DOWN 翻页离开,OK 长按回 HOME)
    return false;
}