// main/ble.h —— 名牌 BLE 服务。
#pragma once

#include <stdbool.h>

// 初始化 NimBLE:注册 GATT 服务并根据 NVS 中的开关状态决定是否广播。app_main 里调用一次。
void ble_init(void);

// 停止/重新开始广播(供设置页蓝牙开关),状态持久化到 NVS。
void ble_stop(void);
void ble_restart(void);

// 查询蓝牙开关是否开启(供设置页初始化显示)。
bool ble_is_enabled(void);
