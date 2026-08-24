// main/apps/games/games.c —— Passport OS V2 第 6 页 GAMES(游戏菜单)页。
//
// 交互(TASK-09 起):由共享 app_list 控制器驱动三态导航。
//   ENTRY 入口态:标题"GAMES" + 简介 + "OK TO ENTER";UP/DOWN 短按交全局翻页。
//   MENU 菜单态: REACTION / MEMORY / MORSE / CATCH,UP/DOWN 选择、OK 进入;长按 OK 返回入口。
//   CHILD 子屏: CATCH 接管现有 game.c(Pixel Catcher);其余 COMING SOON;长按 OK 返回菜单。
// 状态机与按键复用 app_list;CATCH 子屏的按键经由 child_key 交给 game_key。
#include "games.h"
#include "app_list.h"
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

static app_list_ctl_t *s_ctl;
static bool s_in_catch;          // 当前子屏是否为 CATCH(决定 exit_child 用 game_exit)

// 进入选中游戏子屏。CATCH 用 game_enter(接管全屏);其余 COMING SOON。
static bool enter_child_cb(void *ctx, int index)
{
    (void)ctx;
    if (index == GAME_CATCH) {
        s_in_catch = true;
        game_enter();
        ESP_LOGI(TAG, "start CATCH");
        return true;
    }

    s_in_catch = false;
    lv_obj_t *s = lv_obj_create(NULL);
    lv_obj_remove_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
    ds_header(s, s_game_names[index], bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);
    lv_obj_t *lbl = ui_label(s, "COMING SOON", &badge_font_gb2312_small,
                             DS_TEXT_SECONDARY);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 120);
    ds_footer(s, NULL, APP_PAGE_COUNT, APP_PAGE_GAMES);
    lv_screen_load(s);
    return true;
}

// 退出子屏:CATCH 用 game_exit(停循环删屏);占位屏随 LVGL 回收。
static void exit_child_cb(void *ctx)
{
    (void)ctx;
    if (s_in_catch) game_exit();
    s_in_catch = false;
}

// 子屏按键:CATCH 交给 game_key;game 结束(EXITED)时返回菜单。
static app_list_child_key_t child_key_cb(void *ctx, bsp_btn_t btn, bsp_btn_ev_t ev)
{
    (void)ctx;
    if (!s_in_catch) return APP_LIST_CHILD_KEY_NONE;

    game_key_result_t r = game_key(btn, ev);
    if (r == GAME_KEY_EXITED) {
        app_list_goto_menu(s_ctl);   // 游戏结束,回到本页菜单
        return APP_LIST_CHILD_KEY_OK;
    }
    return (r == GAME_KEY_NONE) ? APP_LIST_CHILD_KEY_NONE
                                : APP_LIST_CHILD_KEY_OK;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------
void games_enter(app_page_t page)
{
    (void)page;
    if (!s_ctl) {
        static const app_list_cfg_t cfg = {
            .title = "GAMES",
            .intro = "GAMES",
            .sub = "Reaction / Memory / Morse / Catch",
            .page = APP_PAGE_GAMES,
            .items = s_game_names,
            .item_count = GAMES_COUNT,
            .enter_child = enter_child_cb,
            .exit_child = exit_child_cb,
            .child_key = child_key_cb,
        };
        s_ctl = app_list_create(&cfg);
    }
    app_list_enter(s_ctl);
}

void games_exit(void)
{
    if (s_ctl) { app_list_destroy(s_ctl); s_ctl = NULL; }
}

bool games_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    return s_ctl ? app_list_key(s_ctl, btn, ev) : false;
}