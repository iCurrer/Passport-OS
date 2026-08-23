// main/badge/badge_power.c —— 名牌电源管理:自动关机计时 + 深度睡眠 + 按键唤醒。
//
// 线程规则:
//   - 自动关机计时用 esp_timer(独立任务),不占用 LVGL 任务。
//   - 深度睡眠从 button 任务或 esp_timer 任务触发,均不持有 LVGL 锁。
#include "badge_power.h"
#include "badge_data.h"          // badge_data_commit:深睡前提交 NVS
#include "bsp_display.h"         // bsp_display_backlight / panel
#include "esp_lcd_panel_ops.h"   // esp_lcd_panel_disp_on_off(深睡关屏省电)
#include "bsp_pins.h"            // BSP_BTN_ADC_CHANNEL(唤醒引脚)
#include "esp_sleep.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "badge_power";

#define DEFAULT_TIMEOUT_S  (7u * 60u)          // 默认 7 分钟
#define BOOT_IGNORE_US     (800u * 1000u)       // 开机前忽略按键,防唤醒键误触
#define NVS_KEY_TIMEOUT    "sleep_to"

static esp_timer_handle_t s_auto_timer;
static int64_t s_boot_us;                    // 开机时间
static int64_t s_last_activity_us;           // 最近一次按键时间
static uint32_t s_timeout_s = DEFAULT_TIMEOUT_S;  // 当前超时秒数

// 进入深度睡眠:提交 NVS、关屏、配置 GPIO0 低电平唤醒。不复返。
static void enter_deep_sleep(void)
{
    badge_data_commit();
    ESP_LOGI(TAG, "进入深度睡眠(%lu秒无操作)", (unsigned long)s_timeout_s);
    bsp_display_backlight(0);

    // 关闭 LCD 面板显示,降低深睡时面板静态漏电
    esp_lcd_panel_handle_t panel = bsp_display_panel();
    if (panel) esp_lcd_panel_disp_on_off(panel, false);

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
    if (s_timeout_s == 0) return;   // 永不自动休眠
    int64_t now = esp_timer_get_time();
    if (now - s_last_activity_us >= (int64_t)s_timeout_s * 1000000) {
        enter_deep_sleep();
    }
}

void badge_power_init(void)
{
    if (s_auto_timer) return;

    // 从 NVS 读取上次设置的休眠超时
    nvs_handle_t nvs;
    if (nvs_open("badge_cfg", NVS_READONLY, &nvs) == ESP_OK) {
        uint32_t v = 0;
        if (nvs_get_u32(nvs, NVS_KEY_TIMEOUT, &v) == ESP_OK) {
            s_timeout_s = v;
        }
        nvs_close(nvs);
    }
    ESP_LOGI(TAG, "休眠超时=%lu秒%s", (unsigned long)s_timeout_s,
             s_timeout_s == 0 ? "(永不)" : "");

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

bool badge_power_key_activity(void)
{
    // 开机前短暂忽略按键:唤醒瞬间用户可能仍按着键,避免误触。
    if (esp_timer_get_time() - s_boot_us < (int64_t)BOOT_IGNORE_US) return false;
    s_last_activity_us = esp_timer_get_time();
    return true;
}

void badge_power_activity(void)
{
    s_last_activity_us = esp_timer_get_time();
}

void badge_power_set_timeout(uint32_t seconds)
{
    s_timeout_s = seconds;
    nvs_handle_t nvs;
    if (nvs_open("badge_cfg", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u32(nvs, NVS_KEY_TIMEOUT, seconds);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
    ESP_LOGI(TAG, "休眠超时已设为 %lu秒", (unsigned long)seconds);
}

uint32_t badge_power_get_timeout(void)
{
    return s_timeout_s;
}