// main/badge.c —— 磨砂绿「心情电子名牌」(阶段 1)。
//
// 屏幕:ST7789P3 240x320,有效显示区约 y=50..285。
// 布局:右上角电量 → 姓名(大字) → 状态(主题色) → 英文小字 → 表情小人。
// 按键:上/下 切状态;OK 短按/长按 暂空。真关机用物理电源开关。
// 电源:10 分钟无操作自动进入深度睡眠(省电),任意键唤醒。
// 持久化:NVS 存姓名 / 当前状态(预留手机修改)。
//
// 线程规则:
//   - LVGL 对象只在 LVGL 锁内操作(见 apply_state)。
//   - 深度睡眠从 button 任务或 esp_timer 任务触发,均不持有 LVGL 锁。
//   - 自动关机计时用 esp_timer(独立任务),不占用 LVGL 任务。
#include "badge.h"
#include "badge_fonts.h"
#include "badge_avatar.h"
#include "bsp_display.h"    // bsp_lvgl_lock / unlock / backlight
#include "bsp_battery.h"
#include "bsp_pins.h"       // BSP_BTN_ADC_CHANNEL(唤醒引脚)
#include "lvgl.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "badge";

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
#define LAYOUT_INFO_X       122    // 右侧信息列左对齐起点
#define LAYOUT_NAME_Y       108    // 姓名(主,24px)
#define LAYOUT_DIV_Y        140    // 姓名下细分隔线(宽度 64)
#define LAYOUT_TITLE_Y      152    // 职位(次,14px)
#define LAYOUT_STATUS_Y     172    // 状态文字(辅,14px)
#define LAYOUT_STATUS_DOT_Y 177    // 状态前置圆点(6x6)
#define LAYOUT_DOCK_LINE    272    // dock 顶部分隔线
#define LAYOUT_DOCK_Y       274    // dock 背景起点

#define DOCK_COUNT  2          // 底部 dock 图标数

// ---------------------------------------------------------------------------
// NVS 持久化
// ---------------------------------------------------------------------------
#define NVS_NS       "badge"
#define NVS_KEY_NAME   "name"
#define NVS_KEY_TOP    "top"
#define NVS_KEY_TITLE  "title"
#define NVS_KEY_STATUS "status"

static nvs_handle_t s_nvs;

// ---------------------------------------------------------------------------
// 电源 / 计时
// ---------------------------------------------------------------------------
#define AUTO_OFF_MS    (3u * 60u * 1000u)   // 3 分钟无操作自动深度睡眠
#define BOOT_IGNORE_US (800u * 1000u)        // 开机前忽略按键,防唤醒键误触

static esp_timer_handle_t s_auto_timer;
static int64_t s_boot_us;                    // 开机时间
static int64_t s_last_activity_us;           // 最近一次按键时间

// ---------------------------------------------------------------------------
// UI 对象
// ---------------------------------------------------------------------------
static lv_obj_t *s_scr;
static lv_obj_t *s_brand_lbl;              // 顶部导航栏文字
static lv_obj_t *s_name_lbl;               // 姓名
static lv_obj_t *s_title_lbl;              // 职位(豆包大学)
static lv_obj_t *s_tag_lbl;                // 状态文字(主题色,前置圆点)
static lv_obj_t *s_avatar;                 // 左侧自定义像素形象
static lv_obj_t *s_batt_fill;              // 电量条填充块
static lv_obj_t *s_batt_txt;               // 电量百分比文字(图标右侧)
static lv_timer_t *s_batt_timer;           // 电量刷新
static lv_obj_t *s_dock;                   // 底部 dock 容器

static char    s_name[32]  = "李秋实";
static char    s_top[24]   = "FoloToy";     // 顶部导航栏文字
static char    s_title[32] = "豆包大学";     // 职位
static char    s_status[16]= "自由";        // 状态
static int     s_dock_sel = 0;             // 底部 dock 当前选中项
static bool    s_inited   = false;

// 像素块辅助:在 parent 内按本地坐标画一个纯色矩形。
static lv_obj_t *blk(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    return o;
}

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

// ---------------------------------------------------------------------------
// 深度睡眠
// ---------------------------------------------------------------------------
// 按键 GPIO0 有外部 10k 上拉:松开=高,按下=低。故用 GPIO0 低电平唤醒,
// 任意键按下即拉低 → 唤醒。ESP32-C3 深度睡眠 GPIO 唤醒支持 GPIO0~5。
static void enter_deep_sleep(void)
{
    if (s_nvs) nvs_commit(s_nvs);
    ESP_LOGI(TAG, "进入深度睡眠(3分钟无操作自动,任意键唤醒)");
    bsp_display_backlight(0);

    // 确保 GPIO0 在深度睡眠时作为数字输入并上拉,避免因内部/外部电阻干扰而误唤醒。
    // (按键松开时外部 10k 上拉到高,按下拉低;低电平唤醒只在真正按键时触发。)
    const gpio_config_t io = {
        .pin_bit_mask = (uint64_t)1 << BSP_BTN_ADC_CHANNEL,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    esp_deep_sleep_enable_gpio_wakeup((uint64_t)1 << BSP_BTN_ADC_CHANNEL, ESP_GPIO_WAKEUP_GPIO_LOW);
    esp_deep_sleep_start();                 // 不复返
}

// esp_timer 周期回调:超时无操作即关机。不在 LVGL 任务里,不碰 UI。
static void auto_off_cb(void *arg)
{
    (void)arg;
    int64_t now = esp_timer_get_time();
    if (now - s_last_activity_us >= (int64_t)AUTO_OFF_MS * 1000) {
        enter_deep_sleep();
    }
}

// ---------------------------------------------------------------------------
// NVS
// ---------------------------------------------------------------------------
// 读 NVS 字符串;为空或不存在则写入默认值。
static void nvs_load_str(const char *key, char *buf, size_t size, const char *def)
{
    size_t len = size;
    if (nvs_get_str(s_nvs, key, buf, &len) != ESP_OK || buf[0] == '\0') {
        strncpy(buf, def, size - 1);
        buf[size - 1] = 0;
        nvs_set_str(s_nvs, key, buf);
        nvs_commit(s_nvs);
    }
}

static void nvs_load(void)
{
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    if (nvs_open(NVS_NS, NVS_READWRITE, &s_nvs) != ESP_OK) { s_nvs = 0; return; }

    // 四个可自定义字段:默认值,预留 BLE 手机修改。
    nvs_load_str(NVS_KEY_NAME,   s_name,   sizeof(s_name),   "李秋实");
    nvs_load_str(NVS_KEY_TOP,    s_top,    sizeof(s_top),    "FoloToy");
    nvs_load_str(NVS_KEY_TITLE,  s_title,  sizeof(s_title),  "豆包大学");
    nvs_load_str(NVS_KEY_STATUS, s_status, sizeof(s_status), "自由");
}

// ---------------------------------------------------------------------------
// 初始化(幂等,须在 LVGL 锁内)
// ---------------------------------------------------------------------------
static void badge_init(void)
{
    if (s_inited) return;
    s_inited = true;

    nvs_load();

    // 诊断:确认上次是深度睡眠唤醒,以及唤醒源(排查"自动休眠后又亮起")
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI(TAG, "复位原因=%d", (int)reason);
    if (reason == ESP_RST_DEEPSLEEP) {
        ESP_LOGI(TAG, "深度睡眠唤醒,唤醒源=%d", (int)esp_sleep_get_wakeup_cause());
    }

    s_boot_us = esp_timer_get_time();
    s_last_activity_us = s_boot_us;

    const esp_timer_create_args_t a = {
        .callback = auto_off_cb,
        .name     = "badge_autooff",
    };
    if (esp_timer_create(&a, &s_auto_timer) == ESP_OK) {
        esp_timer_start_periodic(s_auto_timer, 1000 * 1000);   // 每秒检查
    }
}

// 底部 dock 占位图标:带边框的方块 + 中心圆点。选中项顶部加主题色指示条。
static void dock_icon(lv_obj_t *parent, int x, int y, bool sel)
{
    blk(parent, x, y, 34, 3, sel ? ACCENT : 0x2C4A3E);   // 顶部 3px 指示条
    uint32_t b = sel ? TXT_PRIMARY : 0x3D624F;
    uint32_t d = sel ? ACCENT : 0x3D624F;
    blk(parent, x, y + 9, 34, 30, b);
    blk(parent, x + 2, y + 11, 30, 26, DOCK_BG);
    blk(parent, x + 13, y + 19, 8, 8, d);
}

// 重建底部 dock(两个图标),高亮当前选中项。须持 LVGL 锁。
static void dock_draw(void)
{
    if (s_dock) { lv_obj_delete(s_dock); s_dock = NULL; }
    s_dock = lv_obj_create(s_scr);
    lv_obj_remove_flag(s_dock, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_dock, 0, LAYOUT_DOCK_Y);
    lv_obj_set_size(s_dock, 240, 46);
    lv_obj_set_style_bg_opa(s_dock, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_dock, 0, 0);
    lv_obj_set_style_pad_all(s_dock, 0, 0);
    dock_icon(s_dock, 72, 6, s_dock_sel == 0);
    dock_icon(s_dock, 132, 6, s_dock_sel == 1);
}

void badge_enter(void)
{
    badge_init();

    // 根屏:磨砂黑背景
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(BG_COLOR), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    // ===================== 头部导航栏 =====================
    // 左上:品牌(小字,弱);右上:电量。底部细分隔线。
    s_brand_lbl = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_brand_lbl, &badge_font_gb2312_small, 0);
    lv_obj_set_style_text_color(s_brand_lbl, lv_color_hex(TXT_MUTED), 0);
    lv_label_set_text(s_brand_lbl, s_top);
    lv_obj_set_pos(s_brand_lbl, LAYOUT_MARGIN_X, 13);

    // 右上:电量条(无尖尖),图标与数字留小间距
    blk(s_scr, 158, 14, 20, 10, TXT_MUTED);                  // 电池外框
    blk(s_scr, 160, 16, 16, 6, BG_COLOR);                    // 外框内芯
    s_batt_fill = blk(s_scr, 160, 16, 0, 6, BATT_NORMAL);    // 电量填充
    s_batt_txt = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_batt_txt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_batt_txt, lv_color_hex(TXT_MUTED), 0);
    lv_obj_set_pos(s_batt_txt, 186, 12);
    blk(s_scr, 0, LAYOUT_HEAD_LINE, 240, 2, LINE_DIV);       // 头部分隔线

    // ===================== 主体:左头像 + 右信息 =====================
    // 左侧头像:主视觉,垂直居中对齐信息列。
    s_avatar = lv_image_create(s_scr);
    lv_image_set_src(s_avatar, &badge_avatar);
    lv_obj_set_pos(s_avatar, LAYOUT_AVATAR_X, LAYOUT_AVATAR_Y);

    // 右侧信息列:左对齐。层级 姓名(主)> 职位(次)> 状态(辅)。
    s_name_lbl = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_name_lbl, &badge_font_gb2312, 0);
    lv_obj_set_style_text_color(s_name_lbl, lv_color_hex(TXT_PRIMARY), 0);
    lv_label_set_text(s_name_lbl, s_name);
    lv_obj_set_pos(s_name_lbl, LAYOUT_INFO_X, LAYOUT_NAME_Y);

    // 姓名下细分隔线:把"主"与"次"信息分组,强化层级。
    blk(s_scr, LAYOUT_INFO_X, LAYOUT_DIV_Y, 64, 2, LINE_DIV);

    s_title_lbl = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_title_lbl, &badge_font_gb2312_small, 0);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_hex(TXT_MUTED), 0);
    lv_label_set_text(s_title_lbl, s_title);
    lv_obj_set_pos(s_title_lbl, LAYOUT_INFO_X, LAYOUT_TITLE_Y);

    // 状态:前置主题色圆点指示 + 主题色文字(辅)。
    blk(s_scr, LAYOUT_INFO_X, LAYOUT_STATUS_DOT_Y, 6, 6, ACCENT);
    s_tag_lbl = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_tag_lbl, &badge_font_gb2312_small, 0);
    lv_obj_set_style_text_color(s_tag_lbl, lv_color_hex(ACCENT), 0);
    lv_label_set_text(s_tag_lbl, s_status);
    lv_obj_set_pos(s_tag_lbl, LAYOUT_INFO_X + 14, LAYOUT_STATUS_Y);

    // ===================== 底部 dock 功能区 =====================
    blk(s_scr, 0, LAYOUT_DOCK_LINE, 240, 2, LINE_DIV);       // dock 顶部分隔线
    blk(s_scr, 0, LAYOUT_DOCK_Y, 240, 46, DOCK_BG);          // dock 背景
    dock_draw();                            // 两个占位图标(高亮当前选中)

    update_battery();

    s_batt_timer = lv_timer_create(batt_tick, 1000, NULL);

    lv_screen_load(s_scr);
}

// ---------------------------------------------------------------------------
// 按键(可从 button 任务调用,内部管理锁与深度睡眠)
// ---------------------------------------------------------------------------
void badge_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    // 开机前短暂忽略按键:唤醒瞬间用户可能仍按着键,避免误触。
    if (esp_timer_get_time() - s_boot_us < (int64_t)BOOT_IGNORE_US) return;

    s_last_activity_us = esp_timer_get_time();

    // OK 短按/长按暂空:真关机由物理电源开关负责。
    if (ev != BSP_BTN_CLICK) return;

    // 上/下:切换底部 dock 菜单选中项
    if (btn == BSP_BTN_UP)        { s_dock_sel = (s_dock_sel + DOCK_COUNT - 1) % DOCK_COUNT; }
    else if (btn == BSP_BTN_DOWN) { s_dock_sel = (s_dock_sel + 1) % DOCK_COUNT; }
    else return;

    if (!bsp_lvgl_lock(300)) return;
    dock_draw();
    bsp_lvgl_unlock();
}

// ---------------------------------------------------------------------------
// BLE 可自定义字段的读写接口
// ---------------------------------------------------------------------------
const char *badge_get_text(badge_field_t field)
{
    switch (field) {
    case BADGE_FIELD_NAME:   return s_name;
    case BADGE_FIELD_TOP:    return s_top;
    case BADGE_FIELD_TITLE:  return s_title;
    case BADGE_FIELD_STATUS: return s_status;
    default:                 return "";
    }
}

// 更新字段:写 NVS + 刷新 UI。可从 BLE/NimBLE 任务调用(内部加 LVGL 锁)。
void badge_update_text(badge_field_t field, const char *s)
{
    if (!s) return;

    // BLE 写入也算活动,重置自动休眠计时
    s_last_activity_us = esp_timer_get_time();

    char *buf = NULL; size_t size = 0; const char *key = NULL;
    switch (field) {
    case BADGE_FIELD_NAME:   buf = s_name;   size = sizeof(s_name);   key = NVS_KEY_NAME;   break;
    case BADGE_FIELD_TOP:    buf = s_top;    size = sizeof(s_top);    key = NVS_KEY_TOP;    break;
    case BADGE_FIELD_TITLE:  buf = s_title;  size = sizeof(s_title);  key = NVS_KEY_TITLE;  break;
    case BADGE_FIELD_STATUS: buf = s_status; size = sizeof(s_status); key = NVS_KEY_STATUS; break;
    default:                 return;
    }

    strncpy(buf, s, size - 1);
    buf[size - 1] = 0;

    // 持久化到 NVS
    if (s_nvs) {
        nvs_set_str(s_nvs, key, buf);
        nvs_commit(s_nvs);
    }

    // 刷新 LVGL 标签(加锁)
    if (!bsp_lvgl_lock(300)) return;
    if (s_scr) {
        switch (field) {
        case BADGE_FIELD_NAME:   if (s_name_lbl)  lv_label_set_text(s_name_lbl, buf);  break;
        case BADGE_FIELD_TOP:    if (s_brand_lbl) lv_label_set_text(s_brand_lbl, buf); break;
        case BADGE_FIELD_TITLE:  if (s_title_lbl) lv_label_set_text(s_title_lbl, buf); break;
        case BADGE_FIELD_STATUS: if (s_tag_lbl)   lv_label_set_text(s_tag_lbl, buf);   break;
        default: break;
        }
    }
    bsp_lvgl_unlock();
}
