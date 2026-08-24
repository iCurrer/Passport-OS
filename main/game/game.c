// main/game/game.c —— 像素接宝石(Pixel Catcher)。
// 玩法:底部篮子接住落下的宝石,躲避红色炸弹。
// 操控:UP=左移, DOWN=右移, OK短按=暂未使用, OK长按=返回名牌。
// 速度:从慢到快渐进,约 30 分才到中等难度。
//
// 线程规则:
//   - LVGL 对象只在 LVGL 锁内操作。
//   - 游戏循环用 lv_timer(30fps,跑在 LVGL 任务,已持锁)。
//   - game_key 从 button 任务经 badge_key 转发,内部加锁。
#include "game.h"
#include "ui_pixel.h"
#include "badge_fonts.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "game";

// ---------------------------------------------------------------------------
// 主题色
// ---------------------------------------------------------------------------
#define GAME_BG        0x1E352C
#define GAME_BORDER    0x2A473B
#define BASKET_COLOR   0x4CD964   // 篮子:充电绿
#define GEM_GREEN      0x4CD964   // 宝石绿(+10 分)
#define GEM_BLUE       0x4CA0D9   // 宝石蓝(+20 分)
#define GEM_GOLD       0xF0C030   // 宝石金(+50 分)
#define GEM_RED        0xE04545   // 炸弹(-1 命)
#define SCORE_COLOR    0xF4F8F5
#define LIFE_COLOR     0x4CD964
#define LIFE_LOST      0x3D624F
#define LVL_COLOR      0xAFC0B6
#define OVER_COLOR     0xE04545
#define TIP_COLOR      0xAFC0B6

// ---------------------------------------------------------------------------
// 游戏参数
// ---------------------------------------------------------------------------
#define SCREEN_W        240
#define SCREEN_H        320

#define BASKET_W        32
#define BASKET_H        10
#define BASKET_Y        290       // 篮子 y 坐标
#define BASKET_SPEED    16        // 每帧移动像素(屏幕 240px,约 15 次横跨)

#define GEM_SIZE        8         // 宝石/炸弹 8x8 像素
#define GEM_SPAWN_Y     20        // 生成 y 坐标
#define GEM_MAX         10        // 同时最多宝石

#define INIT_LIVES      3
#define BASE_SPEED      1         // 基础下落速度(像素/帧)
#define MAX_SPEED       4         // 最大下落速度
#define SPEED_UP_SCORE  30        // 每 N 分 +1 速度
#define SPAWN_INTERVAL  40        // 基础生成间隔(帧)

// ---------------------------------------------------------------------------
// 宝石/炸弹
// ---------------------------------------------------------------------------
typedef struct {
    lv_obj_t *obj;
    int x, y;
    int speed;
    int points;          // 正数=宝石分值, -1=炸弹
    bool active;
} gem_t;

// ---------------------------------------------------------------------------
// 游戏状态
// ---------------------------------------------------------------------------
static struct {
    lv_obj_t *scr;
    lv_obj_t *basket;
    lv_obj_t *score_lbl;
    lv_obj_t *level_lbl;
    lv_obj_t *life_blocks[INIT_LIVES];
    lv_obj_t *over_lbl;
    lv_obj_t *tip_lbl;
    lv_timer_t *timer;
    gem_t gems[GEM_MAX];
    int basket_x;
    int score;
    int lives;
    int speed;
    int spawn_tick;
    bool running;
    bool over;
} s;

// ---------------------------------------------------------------------------
// 内部
// ---------------------------------------------------------------------------

// 生成一个随机宝石(80% 宝石, 20% 炸弹)
static void spawn_gem(void)
{
    for (int i = 0; i < GEM_MAX; i++) {
        if (s.gems[i].active) continue;
        int r = rand() % 100;
        uint32_t color;
        int points;
        if (r < 10)       { color = GEM_RED;   points = -1; }  // 10% 炸弹
        else if (r < 20)  { color = GEM_GOLD;  points = 50; }  // 10% 金
        else if (r < 45)  { color = GEM_BLUE;  points = 20; }  // 25% 蓝
        else              { color = GEM_GREEN; points = 10; }  // 55% 绿

        int x = 10 + (rand() % (SCREEN_W - GEM_SIZE - 20));
        s.gems[i].x = x;
        s.gems[i].y = GEM_SPAWN_Y;
        s.gems[i].speed = s.speed;
        s.gems[i].points = points;
        s.gems[i].active = true;
        s.gems[i].obj = ui_block(s.scr, x, GEM_SPAWN_Y, GEM_SIZE, GEM_SIZE, color);
        return;
    }
}

// 碰撞检测
static bool rect_hit(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

// 收集宝石
static void collect_gem(int idx)
{
    gem_t *g = &s.gems[idx];
    if (g->points < 0) {
        // 炸弹:扣命
        s.lives--;
    } else {
        // 宝石:加分
        s.score += g->points;
    }

    // 更新生命显示
    for (int i = 0; i < INIT_LIVES; i++) {
        lv_obj_set_style_bg_color(s.life_blocks[i],
            lv_color_hex(i < s.lives ? LIFE_COLOR : LIFE_LOST), 0);
    }

    // 更新分数与速度
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", s.score);
    lv_label_set_text(s.score_lbl, buf);
    s.speed = BASE_SPEED + (s.score / SPEED_UP_SCORE);
    if (s.speed > MAX_SPEED) s.speed = MAX_SPEED;

    // 删除宝石对象
    lv_obj_delete(g->obj);
    g->obj = NULL;
    g->active = false;

    // 检查游戏结束
    if (s.lives <= 0) {
        s.over = true;
        s.running = false;
        s.over_lbl = ui_label(s.scr, "GAME OVER", &badge_font_gb2312, OVER_COLOR);
        lv_obj_center(s.over_lbl);
        lv_obj_set_y(s.over_lbl, 120);
        s.tip_lbl = ui_label(s.scr, "OK=return", &badge_font_gb2312_small, TIP_COLOR);
        lv_obj_center(s.tip_lbl);
        lv_obj_set_y(s.tip_lbl, 160);
    }
}

// 游戏主循环(~30fps)
static void game_tick(lv_timer_t *t)
{
    (void)t;
    if (!s.running) return;

    // 生成宝石
    s.spawn_tick++;
    int interval = SPAWN_INTERVAL - s.speed * 3;
    if (interval < 15) interval = 15;
    if (s.spawn_tick >= interval) {
        s.spawn_tick = 0;
        spawn_gem();
    }

    // 更新宝石位置
    for (int i = 0; i < GEM_MAX; i++) {
        if (!s.gems[i].active) continue;
        s.gems[i].y += s.gems[i].speed;
        lv_obj_set_pos(s.gems[i].obj, s.gems[i].x, s.gems[i].y);

        // 与篮子碰撞
        if (rect_hit(s.basket_x, BASKET_Y, BASKET_W, BASKET_H,
                     s.gems[i].x, s.gems[i].y, GEM_SIZE, GEM_SIZE)) {
            collect_gem(i);
            if (s.over) return;
            continue;
        }

        // 超出屏幕底部
        if (s.gems[i].y > SCREEN_H) {
            lv_obj_delete(s.gems[i].obj);
            s.gems[i].obj = NULL;
            s.gems[i].active = false;
        }
    }

    // 更新关卡显示
    int lvl = s.speed;
    char buf[8];
    snprintf(buf, sizeof(buf), "LV.%d", lvl);
    lv_label_set_text(s.level_lbl, buf);
}

// 移动篮子
static void move_basket(int dx)
{
    if (s.over) return;
    int nx = s.basket_x + dx;
    if (nx < 5) nx = 5;
    if (nx > SCREEN_W - BASKET_W - 5) nx = SCREEN_W - BASKET_W - 5;
    s.basket_x = nx;
    lv_obj_set_pos(s.basket, nx, BASKET_Y);
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

void game_enter(void)
{
    memset(&s, 0, sizeof(s));
    s.lives = INIT_LIVES;
    s.speed = BASE_SPEED;
    s.running = true;
    s.basket_x = (SCREEN_W - BASKET_W) / 2;

    // 游戏屏幕
    s.scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s.scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s.scr, lv_color_hex(GAME_BG), 0);
    lv_obj_set_style_border_width(s.scr, 0, 0);
    lv_obj_set_style_pad_all(s.scr, 0, 0);

    // 顶部信息栏
    s.score_lbl = ui_label(s.scr, "0", &badge_font_gb2312, SCORE_COLOR);
    lv_obj_set_pos(s.score_lbl, 22, 8);

    s.level_lbl = ui_label(s.scr, "LV.1", &badge_font_gb2312_small, LVL_COLOR);
    lv_obj_set_pos(s.level_lbl, 100, 10);

    for (int i = 0; i < INIT_LIVES; i++) {
        s.life_blocks[i] = ui_block(s.scr, 198 - i * 16, 12, 10, 10, LIFE_COLOR);
    }

    ui_block(s.scr, 0, 34, SCREEN_W, 2, GAME_BORDER);   // 顶部信息栏分隔线(避开 24px 分数)

    // 篮子
    s.basket = ui_block(s.scr, s.basket_x, BASKET_Y, BASKET_W, BASKET_H, BASKET_COLOR);

    lv_screen_load(s.scr);

    s.timer = lv_timer_create(game_tick, 33, NULL);  // ~30fps

    ESP_LOGI(TAG, "game started");
}

void game_exit(void)
{
    if (s.timer) { lv_timer_delete(s.timer); s.timer = NULL; }
    s.running = false;

    for (int i = 0; i < GEM_MAX; i++) {
        if (s.gems[i].obj) { lv_obj_delete(s.gems[i].obj); s.gems[i].obj = NULL; }
        s.gems[i].active = false;
    }

    if (s.scr) { lv_obj_delete(s.scr); s.scr = NULL; }
    s.basket = NULL;
    s.score_lbl = NULL;
    s.level_lbl = NULL;
    s.over_lbl = NULL;
    s.tip_lbl = NULL;
    memset(s.life_blocks, 0, sizeof(s.life_blocks));

    ESP_LOGI(TAG, "game exited");
}

game_key_result_t game_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    // 须由调用方持 LVGL 锁(Router 委托契约)。

    // Game Over:OK 短按结束游戏(调用方重建菜单);其它键吞掉
    if (s.over) {
        if (ev == BSP_BTN_CLICK && btn == BSP_BTN_OK) {
            game_exit();
            return GAME_KEY_EXITED;
        }
        return GAME_KEY_CONSUMED;
    }

    // 游戏操作:仅响应 CLICK
    if (ev != BSP_BTN_CLICK) return GAME_KEY_NONE;

    switch (btn) {
    case BSP_BTN_UP:   move_basket(-BASKET_SPEED); break;
    case BSP_BTN_DOWN: move_basket( BASKET_SPEED); break;
    case BSP_BTN_OK:   /* OK 短按暂保留,未来可做特殊技能 */ break;
    default: break;
    }
    return GAME_KEY_CONSUMED;
}