# FoloToy AI Passport — 磨砂黑电子名牌

简体中文 | [English](README.md)

为 **FoloToy AI Passport** 开发的固件，把 240×320 屏幕变成一块**磨砂黑电子名牌**。上电即显示一块干净的个人名牌——姓名、职位、状态、电量，采用克制、高对比的布局；可通过蓝牙在不重新烧录的情况下改字；3 分钟无操作自动深度睡眠省电，任意键唤醒。

本仓库是一个可运行、经过硬件验证的基线。板级驱动放在 `components/bsp` 中、以稳定 API 暴露；名牌应用放在 `main`。完整硬件上下文与排障知识见 [`docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md`](docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md)。

## 特性

- **磨砂黑名牌 UI**，信息层级清晰：大号姓名（24px 白色）为主元素，小号职位/状态（14px）为次级，单一强调色（`0x5B8DEF` 柔蓝）仅用于状态圆点和 dock 选中项。
- **三键交互** —— `UP`/`DOWN` 切换底部 dock；`OK` 预留。
- **BLE 自定义（NimBLE）** —— 手机 App 可通过 GATT 实时修改 姓名/顶部文字/职位/状态，无需重新烧录。
- **NVS 持久化** —— 四个可自定义字段掉电不丢。
- **深度睡眠省电** —— 3 分钟无操作自动深睡，任意键唤醒（GPIO0 低电平唤醒）。
- **电量显示** —— CW2017 电量计，低于 20% 变橙、低于 10% 变红；电量计缺失时降级显示 `--%`。

## 界面布局

```text
┌─────────────────────────────┐
│ FoloToy            ▯ 96%    │  顶部:品牌 + 电量
├─────────────────────────────┤
│             ┌──────────┐    │
│             │          │    │
│  [头像]     │ 张三     │    │  姓名(24px 白色,主)
│             │ ──────── │    │
│             │ 豆包大学  │    │  职位(14px 灰色)
│             │ ● 自由    │    │  状态(14px 强调色+圆点)
│             └──────────┘    │
├─────────────────────────────┤
│      ▮        │              │  底部 dock(UP/DOWN 切换)
└─────────────────────────────┘
```

- 头部：品牌（左）+ 电量条与百分比（右），下方细分隔线。
- 主体：左侧像素头像，与右侧信息列垂直居中；信息列左对齐于同一条参考线。
- 底部 dock：两个占位页签，选中项顶部有 3px 强调色指示条。

## 三键交互

| 按键 | 动作 |
| --- | --- |
| `UP` | 切换到上一个 dock 页签 |
| `DOWN` | 切换到下一个 dock 页签 |
| `OK` | 预留（短按/长按暂空，真关机走物理电源开关） |

## BLE 自定义（NimBLE）

- **广播名：** `FoloToy-Badge`
- **GATT 服务：** `0xFFE0`
- **特性（可读可写）**，16 位 UUID：

| UUID | 字段 | NVS key |
| --- | --- | --- |
| `0xFFE1` | 姓名 | `name` |
| `0xFFE2` | 顶部文字 | `top` |
| `0xFFE3` | 职位 | `title` |
| `0xFFE4` | 状态 | `status` |

写入特性会即时刷新 LVGL 标签并持久化到 NVS。安卓端 UUID 为 `0000FFEx-0000-1000-8000-00805F9B34FB`。

## 硬件能力契约（BSP）

| 能力 | 已确认实现 | 应用接口 | 边界 |
| --- | --- | --- | --- |
| 显示 | ST7789P3，240 × 320 竖屏 RGB565，SPI2 @ 40 MHz；LEDC 背光 | `bsp_display_*`、`bsp_lvgl_*` | ESP32-C3 无 PSRAM；单小 DMA 缓冲；无 MISO/触摸/TE |
| 输入 | `UP`/`DOWN`/`OK` 共用 GPIO0 ADC 电阻分压 | `bsp_button_init()`、`bsp_button_read_mv()` | 回调运行在按键任务中，不能阻塞 |
| 音频 | ES8311，I2S0 全双工 PCM（播放+录音） | `bsp_audio_*` | PCM 读写阻塞 → 工作任务；切格式必须 close/open |
| 电池 | CW2017 SOC 与电压 | `bsp_battery_*` | 运行时可缺省；精度取决于电芯/profile |
| 共享总线 | ES8311 与 CW2017 共用 I2C0 | `bsp_i2c_*` | 复用 BSP 持有的总线，不在同端口新建第二条总线 |
| 日志/烧录 | ESP32-C3 原生 USB Serial/JTAG | ESP-IDF console | GPIO18/19 给 USB；UART0 TX GPIO21 与背光冲突 |

所有引脚、地址、面板参数与按键电压窗口只在 [`components/bsp/include/bsp_pins.h`](components/bsp/include/bsp_pins.h) 定义。

## 中文字库说明

- 姓名用 24px 全量中文字库；职位/状态/顶部用 14px 全量中文字库。
- 当前内置字库覆盖 **GB2312 一级常用字**，以控制 Flash 体积（整个固件约 2.1 MB）。通过 BLE 写入超出该字集的中文会显示为缺字。如需更广覆盖，原始全量字库备份在 `_font_backup/`（不入库）。

## 构建与烧录

ESP-IDF 5.5.x（已知环境 5.5.3）：

```bash
idf.py set-target esp32c3     # 新 checkout 时才需要
idf.py build
idf.py -p COM5 flash          # 把 COM5 换成你的端口
idf.py -p COM5 monitor
```

自定义分区表（`partitions.csv`）把 app 分区扩大到 4 MB 以容纳中文字库。

## 真机验收清单

- USB Serial/JTAG 有稳定启动日志，无重启循环或 watchdog 复位。
- 显示方向、颜色、边缘与背光正确，强调色渲染正常。
- `UP`/`DOWN` 切换 dock 选中项（强调色指示条跟随）。
- BLE：手机可发现 `FoloToy-Badge`；读写四个特性后 UI 更新且重启后仍在。
- 电量读数合理，CW2017 缺失时降级显示 `--%`。
- 3 分钟无操作后深睡，任意键唤醒。
- 反复切换/BLE 写入不持续泄漏任务、对象或堆。

## 项目结构

```text
components/bsp/include/  BSP 公开 API 与 bsp_pins.h 硬件事实
components/bsp/src/      显示、按键、音频、电池、共享 I2C 实现
main/                    名牌应用(main.c、badge.c、ble.c、字体、头像)
docs/                    agent 硬件开发指南
sdkconfig.defaults       ESP32-C3、USB console、Flash、LVGL 默认配置
partitions.csv           自定义 4 MB app 分区
AGENTS.md                编码、验证与贡献规则
```
