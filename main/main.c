// main/main.c —— FoloToy AI Passport 名牌固件:初始化外设后直入磨砂黑名牌界面。
//
// 开机流程:I2C → 显示/LVGL → 背光 → 电池/按键 → badge_enter()。
// 名牌界面交互见 badge.c(上/下切状态、OK 长按关机、任意键唤醒、10 分钟自动关机)。
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "badge.h"
#include "ble.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "main";

// 按键回调运行在 button 组件的任务里。badge_key 内部自行管理 LVGL 锁与深度睡眠。
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    badge_key(btn, ev);
}

void app_main(void)
{
    ESP_LOGI(TAG, "FoloToy AI Passport badge 启动");

    bsp_i2c_init();

    // 屏幕是名牌的载体,失败就没法展示 —— 打清日志后退出。
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败,无法进入名牌界面");
        return;
    }
    bsp_display_backlight(70);

    // 软依赖:电池缺失不阻塞(名牌电量显示 --%);按键失败则无法交互。
    bsp_battery_init();
    bool btn_ok = (bsp_button_init(on_key, NULL) == ESP_OK);
    ESP_LOGI(TAG, "电池=%d 按键=%d", 1, btn_ok);

    ble_init();                        // 启动 BLE 广播 + GATT 服务

    if (bsp_lvgl_lock(1000)) { badge_enter(); bsp_lvgl_unlock(); }
    ESP_LOGI(TAG, "名牌界面已就绪");
}
