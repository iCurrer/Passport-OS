# FoloToy AI Passport — 磨砂黑电子名牌

简体中文 | [English](README.md)

为 **FoloToy AI Passport** 开发的固件，把 240×320 屏幕变成一块**磨砂黑电子名牌**。上电即显示一块干净的个人名牌——姓名、职位、状态、电量，采用克制、高对比的布局；可通过蓝牙在不重新烧录的情况下改字。底部 dock 可进入内置的**像素游戏**和**设置页**（息屏时间、蓝牙开关、固件版本）。为省电，在可配置的闲置超时（默认 7 分钟）后自动深度睡眠，任意键唤醒。

本仓库是一个可运行、经过硬件验证的基线。固件采用清晰的**三层架构**：硬件抽象（`components/bsp`）、可复用 UI 原语（`components/ui`）、应用逻辑（`main/`——`badge/`、`game/`、`settings/`、`transport/`、`assets/`）。完整硬件上下文与排障知识见 [`docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md`](docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md)。

## 特性

- **磨砂黑名牌 UI**，信息层级清晰：大号姓名（24px 白色）为主元素，小号职位/状态（14px）为次级，单一强调色（`0x4CD964` 绿）仅用于状态圆点和 dock 选中项。
- **三键交互** —— `UP`/`DOWN` 切换底部 dock；`OK` 短按进入选中页面，`OK` 长按返回名牌。
- **像素游戏** —— dock 内置"接宝石、躲炸弹"小游戏；`UP`/`DOWN` 移动篮子，得分越高难度越大。
- **设置页** —— 息屏时间（30 秒 / 1 / 2 / 5 分钟 / 永不）、蓝牙开关（NVS 持久化）、固件版本。
- **BLE 自定义（NimBLE）** —— 手机 App 可通过 GATT 实时修改 姓名/顶部文字/职位/状态，无需重新烧录。
- **NVS 持久化** —— 四个可自定义字段、息屏时间、蓝牙开关状态均掉电不丢。
- **深度睡眠省电** —— 可配置闲置超时（默认 7 分钟）自动深睡，任意键唤醒（GPIO0 低电平唤醒）。
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
- 底部 dock：两个实际入口（游戏图标 + 设置图标），选中项顶部有 3px 强调色指示条。

## 三键交互

| 按键 | 名牌（首页） | 游戏 | 设置 |
| --- | --- | --- | --- |
| `UP` / `DOWN` | 切换 dock 选中项 | 移动篮子左 / 右 | 移动选中项上 / 下 |
| `OK`（短按） | 进入选中页面 | 预留（未来技能） | 切换选中项的值 |
| `OK`（长按） | — | 返回名牌 | 返回名牌 |

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
- 字库（`main/assets/badge_font_gb2312.c`、`main/assets/badge_font_gb2312_small.c`）用 `lv_font_conv` 面向 **LVGL 9.5** 生成，覆盖 **GB2312（6763 常用字）+ ASCII `0x20-0x7E`**。通过 BLE 写入超出该字集的字符会显示为缺字。
- ⚠️ LVGL 8 与 9.5 修改了 `lv_font_t.bitmap_format` 的语义；用 LVGL 8 生成的字库在 9.5 下会**所有文字全空白**（布局/图片/声音仍正常），不易排查。请用与目标 LVGL 一致的 `lv_font_conv` 重新生成（`scripts/gen_badge_fonts.py`），并确认 `bitmap_format=1`（COMPRESSED）且包含 ASCII 范围 `0x20-0x7E`。

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
- `UP`/`DOWN` 切换 dock 选中项（强调色指示条跟随）；`OK` 进入选中页面，长按 `OK` 返回。
- 像素游戏：篮子可移动，宝石/炸弹计分正确，Game Over 与退出均干净返回名牌。
- 设置页：息屏时间与蓝牙开关即时生效且重启后仍在；版本号正确渲染。
- BLE：手机可发现 `FoloToy-Badge`；读写四个特性后 UI 更新且重启后仍在。
- 电量读数合理，CW2017 缺失时降级显示 `--%`。
- 可配置闲置超时（默认 7 分钟）后深睡，任意键唤醒。
- 反复切换/BLE 写入不持续泄漏任务、对象或堆。

## 项目结构

```text
components/bsp/                  硬件抽象层（BSP）
├── include/bsp_pins.h            引脚、地址、面板参数单一事实来源
├── include/bsp_display.h         显示与 LVGL 端口 API
├── include/bsp_button.h          ADC 三按键 API
├── include/bsp_audio.h           ES8311 音频编解码 API
├── include/bsp_battery.h         CW2017 电量计 API
├── include/bsp_i2c.h             共享 I2C0 总线 API
└── src/                          驱动实现

components/ui/                    可复用 UI 原语库（跨应用共享）
├── include/ui_pixel.h            ui_block()、ui_label()、ui_header_bar()、ui_battery_pct()
└── src/ui_pixel.c

main/                             应用层
├── main.c                        入口：初始化硬件 → badge_enter()
├── badge/                        名牌应用（模块化、分层）
│   ├── badge.h                   公开 API + badge_sub_t（子页面路由状态）
│   ├── badge.c                   门面：编排子模块，把按键路由到 game/settings
│   ├── badge_data.h / .c          数据模型：基于 NVS 的字段存储（姓名/顶部/职位/状态）
│   ├── badge_power.h / .c         电源：可配置深睡定时器、按键唤醒
│   ├── badge_ui.h / .c            LVGL 布局：导航栏、dock、字段刷新
│   └── badge_theme.h             共用颜色/布局常量（主题）
├── game/                         像素游戏页
│   └── game.h / .c                “接宝石，躲炸弹”小游戏（enter / exit / key）
├── settings/                     设置页
│   └── settings.h / .c            息屏时间、蓝牙开关、版本（enter / exit / key）
├── transport/                     通信层
│   └── ble.h / .c                 NimBLE GATT 服务（广播 + 读写特性）
├── assets/                        静态资源
│   ├── badge_fonts.h              字体声明
│   ├── badge_font_gb2312.c        24px 中文字库（GB2312）
│   ├── badge_font_gb2312_small.c  14px 中文字库（GB2312）
│   └── badge_avatar.h / .c        像素头像图像数据
└── CMakeLists.txt                 构建：源文件、头文件目录、组件依赖

sdkconfig.defaults                 ESP32-C3、USB console、Flash、LVGL 默认配置
partitions.csv                     自定义 4 MB app 分区
AGENTS.md                          编码、验证与贡献规则
docs/                              AI 硬件开发指南
```

### 架构与依赖方向

```
┌──────────────────────────────────────────────────────────────┐
│                     应用层                                   │
│  main.c ──► badge/ （门面 → 数据、电源、UI、主题）        │
│              ├─► game/（像素游戏）   └─► settings/（设置页） │
│              transport/ble.c （NimBLE GATT）                 │
├──────────────────────────────────────────────────────────────┤
│                   UI 组件库                                  │
│  components/ui/ （ui_block、ui_label、ui_header_bar、        │
│                  ui_battery_pct —— 纯 LVGL）                 │
├──────────────────────────────────────────────────────────────┤
│                   硬件抽象层                                 │
│  components/bsp/ （显示、按键、音频、电池、I2C）            │
└──────────────────────────────────────────────────────────────┘
```

依赖方向：`main → ui → bsp`。`ui` 层是纯 LVGL，对硬件一无所知。`badge` 子模块按职责拆分：`data`（NVS 存储）、`power`（休眠定时器）、`ui`（LVGL 布局）、`theme`（共用常量）—— 全部由 `badge.c` 门面协调。dock 页面（`game/`、`settings/`）实现 `enter`/`exit`/`key` 接口，由 `badge_ui_dock_enter()` 从 dock 启动，并复用 `components/ui` 绘制。新应用同样可复用 `components/ui`，无需重复实现像素原语。
