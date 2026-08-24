// main/main.c —— Passport OS V2 智能工牌固件入口。
// 开机流程:I2C → 显示/LVGL → 背光 → 电池/按键 → app_router(上下翻页)。
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "app_router.h"
#include "ble.h"
#include "nvs_flash.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "main";

// 按键回调运行在 button 组件的任务里。app_router_key 内部自行管理 LVGL 锁与深度睡眠。
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    app_router_key(btn, ev);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Passport OS V2 启动");

    // NVS 必须先初始化:BLE 开关/休眠超时等配置在 ble_init 与 badge_power 里读取。
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    bsp_i2c_init();

    // 屏幕是 UI 的载体,失败就没法展示 —— 打清日志后退出。
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败,无法进入界面");
        return;
    }
    bsp_display_backlight(70);

    // 软依赖:电池缺失不阻塞(电量显示 --%);按键失败则无法交互。
    bsp_battery_init();
    bool btn_ok = (bsp_button_init(on_key, NULL) == ESP_OK);
    ESP_LOGI(TAG, "电池=1 按键=%d", btn_ok);

    ble_init();                        // 启动 BLE 广播 + GATT 服务

    // 全局页面路由接管导航(app_router_init 内部会启动自动休眠计时)。
    app_router_init();
    if (bsp_lvgl_lock(1000)) { app_router_enter(); bsp_lvgl_unlock(); }
    ESP_LOGI(TAG, "界面已就绪");
}
