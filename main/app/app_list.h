// main/app/app_list.h —— 列表页三态导航控制器（ENTRY → MENU → CHILD）。
//
// 背景:TOOLS/GAMES/SETTINGS 三个列表页需要同时支持「上下翻页」和「列表内选择」。
// 早期实现把短按 UP/DOWN 全部用于列表选择,导致这几个页面没法短按翻页(TASK-08/09/13 已知取舍)。
// 本控制器统一为:
//   ENTRY 入口态(默认,占位展示标题+简介+"OK to enter")
//     - UP/DOWN 短按 不消费 → 交给 Router 全局翻页(修复翻页问题)
//     - OK 短按   → 进入 MENU
//     - OK/UP 长按→ 不消费 → Router 回首页
//   MENU  菜单态(列表选择)
//     - UP/DOWN 短按 选择上一项/下一项(消费)
//     - OK 短按   → 进入 CHILD(或原地操作 SETTINGS)
//     - OK 长按   → 返回上一级(ENTRY)
//   CHILD 子屏(工具/游戏;SETTINGS 无子屏,OK 原地操作)
//     - 子屏 key 由页面回调处理,内部/调用方持锁
//     - OK 长按   → 返回上一级(MENU)
//
// 复用方式:三个页面各自持有一个 app_list_ctl_t,把本页文案/子屏回调填进 app_list_cfg_t,
// 由控制器统一驱动状态机与按键,页面不再各写一份重复逻辑。Router 仅需把 key 事件交给页面
// 页面再把短按/长按一并委托给 app_list_key。
//
// 线程:所有 lv_* 调用须在 bsp_lvgl_lock 内(由 Router 或调用方持锁)。
#pragma once

#include "lvgl.h"
#include "app_pages.h"
#include "bsp_button.h"

#ifdef __cplusplus
extern "C" {
#endif

// 子屏 key 分发结果
typedef enum {
    APP_LIST_CHILD_KEY_NONE = 0,  // 未消费(交给全局,如长按回首页)
    APP_LIST_CHILD_KEY_OK,        // 已消费
} app_list_child_key_t;

// 页面提供的回调;ctx 为页面私有上下文(通常 NULL 或本页状态指针)。
typedef struct {
    // 入口态标题(Header 左文案)与简介/提示(正文)。
    const char *title;        // Header 标题,如 "TOOLS"/"GAMES"/"SETTINGS"
    const char *intro;        // 入口正文第一行(强调色大字)
    const char *sub;          // 入口正文第二行(次要色小字),可空
    app_page_t  page;         // 所在页(用于 Page Indicator)
    const char *const *items; // 菜单项文案(MENU 态列表)
    int         item_count;   // 菜单项数量

    // 菜单行右侧值文案(可空)。SETTINGS 用(BLE ON/OFF、SLEEP 标签、版本)。
    const char *(*value_cb)(void *ctx, int index);

    // 子屏构建:OK 进入某项时调用,负责创建并 lv_screen_load 子屏。
    // 须持锁。返回 true=已进入子屏(状态切 CHILD);false=原地操作(SETTINGS 用,留在 MENU)。
    bool (*enter_child)(void *ctx, int index);
    // 子屏销毁:退出 CHILD 时调用,清理子屏对象/定时器。可空。
    void (*exit_child)(void *ctx);
    // 子屏按键分发:CHILD 态其它 key 交到这里。返回消费与否。可空(默认不消费)。
    app_list_child_key_t (*child_key)(void *ctx, bsp_btn_t btn, bsp_btn_ev_t ev);

    void *ctx;
} app_list_cfg_t;

typedef struct app_list_ctl app_list_ctl_t;

// 创建控制器(不渲染)。cfg 复制到内部。
app_list_ctl_t *app_list_create(const app_list_cfg_t *cfg);

// 销毁并释放。
void app_list_destroy(app_list_ctl_t *ctl);

// 进入本页并渲染 ENTRY 入口态(复位状态机)。须持锁。
void app_list_enter(app_list_ctl_t *ctl);

// 从 CHILD 返回 MENU(子屏自身探测到退出时调用,如 game 结束)。须持锁。
void app_list_goto_menu(app_list_ctl_t *ctl);

// 按键分发:CLICK/LONG 都交给控制器统一处理。返回是否已消费。
// Router 委托给页面后,页面直接转发本函数。须持锁。
bool app_list_key(app_list_ctl_t *ctl, bsp_btn_t btn, bsp_btn_ev_t ev);

// 当前所在状态(调试/日志用)。
int app_list_state(const app_list_ctl_t *ctl);

#ifdef __cplusplus
}
#endif