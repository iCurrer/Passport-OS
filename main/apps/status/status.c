// main/apps/status/status.c —— Passport OS V2 第 2 页 STATUS(状态)页。
//
// 布局(参考 docs/UI_DESIGN_SPEC.md §8):
//   Header(0-35)      :ds_header("STATUS" + 电量)
//   Content(36-271)   :当前状态大字(24px 强调) → 强调分隔线 → 5 档可选状态列表
//                      当前档高亮(强调),其余灰字(次要)
//   Footer(272-319)   :Page Indicator(STATUS=第 2 点实心)
//
// 交互:
//   - OK 短按 / DOWN 长按(快速状态切换)→ status_cycle():切下一档并写 NVS。
// 状态文案存 NVS(BADGE_FIELD_STATUS),5 档预设:AVAILABLE→FOCUS→BUSY→DND→OFFLINE。
#include "status.h"
#include "badge_data.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stddef.h>
#include <string.h>

static const char *TAG = "status";

// 5 档预设状态(英文,UI 文字规范)。与 NVS status 字段对应。
#define STATUS_COUNT  5
static const char *s_presets[STATUS_COUNT] = {
    "AVAILABLE", "FOCUS", "BUSY", "DND", "OFFLINE",
};

// 布局 y(Content 36-271)
#define STATUS_BIG_Y    86
#define STATUS_DIV_Y    120
#define STATUS_LIST_Y   142
#define STATUS_ROW_GAP  22

static lv_obj_t *s_scr;
static ds_dots_t s_dots;
static lv_obj_t *s_big;                      // 当前状态大字
static lv_obj_t *s_rows[STATUS_COUNT];       // 5 档列表标签

// 找到当前状态在预设中的下标;不在预设(如旧默认"自由")返回 -1。
static int find_index(const char *st)
{
    for (int i = 0; i < STATUS_COUNT; i++) {
        if (st && s_presets[i][0] && strcmp(st, s_presets[i]) == 0) return i;
    }
    return -1;
}

// 刷新大字与列表高亮。须在 LVGL 锁内;页面不在场时无对象可刷,直接返回。
static void refresh_ui(void)
{
    if (!s_scr) return;
    const char *cur = badge_data_get(BADGE_FIELD_STATUS);
    int idx = find_index(cur);
    if (s_big) lv_label_set_text(s_big, cur);
    for (int i = 0; i < STATUS_COUNT; i++) {
        if (!s_rows[i]) continue;
        uint32_t c = (i == idx) ? DS_ACCENT : DS_TEXT_SECONDARY;
        lv_obj_set_style_text_color(s_rows[i], lv_color_hex(c), 0);
    }
}

void status_cycle(void)
{
    int cur = find_index(badge_data_get(BADGE_FIELD_STATUS));
    int next = (cur < 0) ? 0 : (cur + 1) % STATUS_COUNT;   // 不在预设→跳到 AVAILABLE
    badge_data_set(BADGE_FIELD_STATUS, s_presets[next]);
    ESP_LOGI(TAG, "status -> %s", s_presets[next]);
    refresh_ui();
}

void status_enter(app_page_t page)
{
    (void)page;

    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    ds_header(s_scr, "STATUS", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    // 当前状态大字
    s_big = ui_label(s_scr, badge_data_get(BADGE_FIELD_STATUS),
                     &badge_font_gb2312, DS_ACCENT);
    lv_obj_align(s_big, LV_ALIGN_TOP_MID, 0, STATUS_BIG_Y);

    // 强调分隔线
    ui_block(s_scr, 120 - 36, STATUS_DIV_Y, 72, 2, DS_ACCENT);

    // 5 档可选状态列表(当前档高亮,其余灰字)
    const char *cur = badge_data_get(BADGE_FIELD_STATUS);
    int cur_idx = find_index(cur);
    for (int i = 0; i < STATUS_COUNT; i++) {
        s_rows[i] = ui_label(s_scr, s_presets[i], &badge_font_gb2312_small,
                             (i == cur_idx) ? DS_ACCENT : DS_TEXT_SECONDARY);
        lv_obj_align(s_rows[i], LV_ALIGN_TOP_MID, 0, STATUS_LIST_Y + i * STATUS_ROW_GAP);
    }

    // Footer:Page Indicator(STATUS=第 2 点实心)
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_STATUS);

    lv_screen_load(s_scr);
}

void status_exit(void)
{
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    s_big = NULL;
    for (int i = 0; i < STATUS_COUNT; i++) s_rows[i] = NULL;
}