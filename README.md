<p align="center">
  <img src="docs/assets/passport-os-v2.jpg" alt="Passport OS V2" width="180" style="border-radius:12px"/>
</p>

<h1 align="center">Passport OS V2</h1>

<h3 align="center">低功耗个人智能电子工牌固件 · ESP32-C3</h3>

<p align="center">
  <em>个人身份 · 上下翻页 · BLE 自定义 · 头像同步 · 深色极简</em>
</p>

<p align="center">
  <a href="#"><img alt="版本" src="https://img.shields.io/badge/版本-v2.0.0-4CD964.svg"/></a>
  <a href="LICENSE"><img alt="协议" src="https://img.shields.io/badge/license-MIT-blue.svg"/></a>
  <a href="README.en.md"><img alt="双语" src="https://img.shields.io/badge/docs-中文%20%7C%20English-8A8A8A.svg"/></a>
  <a href="#"><img alt="芯片" src="https://img.shields.io/badge/MCU-ESP32--C3-E7352C.svg"/></a>
  <a href="#"><img alt="屏幕" src="https://img.shields.io/badge/Display-240%C3%97320%20ST7789-FF9F0A.svg"/></a>
  <a href="#"><img alt="蓝牙" src="https://img.shields.io/badge/BLE-NimBLE%20GATT-4CA0D9.svg"/></a>
  <a href="#"><img alt="SDK" src="https://img.shields.io/badge/ESP--IDF-5.5-green.svg"/></a>
  <a href="#"><img alt="LVGL" src="https://img.shields.io/badge/LVGL-9.5-purple.svg"/></a>
</p>

> 这是一套基于 **ESP32-C3** 的**低功耗个人智能工牌**固件（原 FoloToy AI Passport 渐进式产品化改造而来）。
> 它以「**上下翻页**」为核心交互，把一块 240×320 屏幕变成你的电子身份名片——展示姓名、职位、状态、二维码，支持头像与个人资料经手机 BLE 修改，化繁为简、深色极简、单屏极低功耗。

简体中文 | [English](README.en.md)

---

## 快速认知

| 项目 | 说明 |
| --- | --- |
| 主控 | ESP32-C3（8 MB Flash，**无 PSRAM**，内部 RAM 有限） |
| 屏幕 | ST7789P3，240×320 竖屏 RGB565，SPI2 @ 40 MHz |
| 交互 | 三按键（`UP` / `DOWN` / `OK`），非触摸，共用 GPIO0 ADC 分压 |
| 音频 | ES8311（I2S0 全双工），与本项目主体 UI 无关 |
| 电量 | CW2017 电量计（运行时可缺省，缺失时显示 `--%`） |
| SDK | ESP-IDF 5.5.x，LVGL 9.5 |
| 系统 | **Vertical Page Navigation** —— 8 页循环上下翻页，App Router 统一管理全局按键 |

---

## 核心交互：上下翻页

整个系统不是手机 App 图标网格，而是**纵向翻页**：

```text
         UP / DOWN          UP / DOWN
              ↑                   ↑
     ┌───────────────┐   ┌───────────────┐
     │   HOME        │   │  PROFILE      │  ... 循环至 HOME
     └───────────────┘   └───────────────┘
```

### 三按键语义（App Router 统一，禁止页面各自管理全局按键）

| 按键 | 短按 | 长按 |
| --- | --- | --- |
| `UP` | 上一页（循环） | 直接返回 HOME |
| `DOWN` | 下一页（循环） | 快速状态切换 |
| `OK` | 进入 / 操作当前页 | 返回 HOME |

页面列表页内（TOOLS / GAMES / SETTINGS）短按 **UP/DOWN** 优先用于选择、**OK** 进入/操作；长按始终走全局语义。

### 8 个页面

```text
PAGE 0  HOME        身份名片：头像 + 姓名 + 职位 + 状态
PAGE 1  PROFILE     完整个人资料：姓名 / 职位 / 状态 / 简介（不含链接文本）
PAGE 2  STATUS      快速状态：AVAILABLE / FOCUS / BUSY / DND / OFFLINE
PAGE 3  CARDS/QR    个人二维码（动态生成，内容取「二维码内容」字段，不展示链接）
PAGE 4  DASHBOARD   仪表盘：运行时长(UPTIME) / FOCUS 会话计时 / BLE / WIFI（诚实数据，不造假时钟）
PAGE 5  TOOLS       工具：TIMER / STOPWATCH / CALCULATOR / MORSE
PAGE 6  GAMES       游戏：REACTION / MEMORY / MORSE / CATCH
PAGE 7  SETTINGS    设置：BLE 开关 / 休眠超时 / 固件版本
```

每页底部都有 **Page Indicator**（`● ○ ○ ○ ○ ○ ○ ○`）标示当前所在页。

---

## 特性

- **深色极简 UI**（V2 design-system）：黑底 `#000000`、主文字 `#FFFFFF`、强调色 `#4CD964`，低信息密度、高对比。
- **上下翻页个人身份**：一键就能翻看自己的名片、状态、二维码与个人信息，无需进入复杂菜单。
- **可自定义个人资料**：姓名、顶部文字、职位、状态、简介全部存 **NVS**，经 BLE 修改后**掉电不丢**，**不编译进固件**。
- **个人二维码**：CARDS 页动态生成 QR（依赖 `espressif/qrcode`），内容取自用户自定义的「二维码内容」字段（如微信/网址链接），页面不展示链接文本。
- **头像上传（BLE）**：手机端裁剪/缩放/转 RGB565 后经 BLE 分块上传，ESP32 接收 → CRC32 校验 → 保存到 SPIFFS（`/avatar.bin`，80×80 RGB565）→ 立即刷新显示。
- **3 键导航**：全部按键逻辑由 **App Router** 统一分发，符合三键语义表。
- **低功耗**：可配置闲置超时（30s/1m/2m/5m/never）自动 **Deep Sleep**，任意键唤醒；BLE 与 Wi-Fi 默认关，仅在需要同步时打开。
- **电量显示**：CW2017 SOC，低于 20% 变橙、低于 10% 变红，电量计缺失时降级显示 `--%`。

---

## BLE 自定义与头像（NimBLE）

- **广播名**：`FoloToy-Badge`；**GATT 服务** `0xFFE0`
- **个人资料特性**（可读可写，16 位 UUID，写入即持久化 NVS）：

| UUID | 字段 | NVS key |
| --- | --- | --- |
| `0xFFE1` | 姓名 | `name` |
| `0xFFE2` | 顶部文字 | `top` |
| `0xFFE3` | 职位 | `title` |
| `0xFFE4` | 状态 | `status` |
| `0xFFE5` | 简介 | `bio` |
| `0xFFE7` | 二维码内容 | `qr` |

- **头像特性**（只写）：

| UUID | 用途 |
| --- | --- |
| `0xFFE8` | `AV_CTRL` 头像控制：`START <size> <crc32>` 开始 / `CANCEL` 中止 |
| `0xFFE9` | `AV_DATA` 头像数据（分块写入，收满后 CRC32 校验，匹配则保存） |

> 安卓端 UUID 形如 `0000FFEx-0000-1000-8000-00805F9B34FB`。手机 App 见 [`android_app/`](android_app/)（含 240×320 实时预览与头像裁剪上传）。

---

## 硬件能力契约（BSP）

| 能力 | 已确认实现 | 应用接口 | 边界 |
| --- | --- | --- | --- |
| 显示 | ST7789P3，240 × 320 竖屏 RGB565，SPI2 @ 40 MHz；LEDC 背光 | `bsp_display_*`、`bsp_lvgl_*` | ESP32-C3 无 PSRAM；单小 DMA 缓冲；无 MISO/触摸/TE |
| 输入 | `UP`/`DOWN`/`OK` 共用 GPIO0 ADC 电阻分压 | `bsp_button_init()`、`bsp_button_read_mv()` | 回调运行在按键任务中，不能阻塞 |
| 音频 | ES8311，I2S0 全双工 PCM（与本项目主体 UI 无关） | `bsp_audio_*` | PCM 读写阻塞 → 工作任务；切格式必须 close/open |
| 电池 | CW2017 SOC 与电压 | `bsp_battery_*` | 运行时可缺省；精度取决于电芯/profile |
| 共享总线 | ES8311 与 CW2017 共用 I2C0 | `bsp_i2c_*` | 复用 BSP 持有的总线，不在同端口新建第二条总线 |
| 日志/烧录 | ESP32-C3 原生 USB Serial/JTAG | ESP-IDF console | GPIO18/19 给 USB；UART0 TX GPIO21 与背光冲突 |

所有引脚、地址、面板参数与按键电压窗口只在 [`components/bsp/include/bsp_pins.h`](components/bsp/include/bsp_pins.h) 定义。

---

## 中文字库说明

- 姓名用 24px 全量中文字库；职位/状态/顶部等用 14px 全量中文字库（`main/assets/badge_font_gb2312.c` / `badge_font_gb2312_small.c`）。
- 字库面向 **LVGL 9.5** 用 `lv_font_conv` 生成，覆盖 **GB2312（6763 常用字）+ ASCII `0x20-0x7E`**。通过 BLE 写入超出该字集的字符会显示为缺字。
- ⚠️ LVGL 8 与 9.5 修改了 `lv_font_t.bitmap_format` 语义；用 LVGL 8 生成的字库在 9.5 下会**所有文字全空白**（布局/图片/声音仍正常）。请用 `scripts/gen_badge_fonts.py` 重新生成并确认 `bitmap_format=1` 且含 ASCII 范围。

---

## 构建与烧录

ESP-IDF 5.5.x：

```bash
idf.py set-target esp32c3     # 新 checkout 时才需要
idf.py build
idf.py -p COM5 flash          # 把 COM5 换成你的端口
idf.py -p COM5 monitor
```

- 自定义分区表（`partitions.csv`）：`factory` app 4 MB（容纳中文字库）+ `storage`（SPIFFS 2 MB，存放 `/avatar.bin`），烧录时需连同分区表一起写入。

---

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
├── include/ds_tokens.h           V2 配色/骨架/排版 token（深色极简规范）
├── include/ds_widgets.h          设计系统原语（ds_header / ds_footer / ds_page_dots）
├── src/ds_widgets.c
├── include/ui_pixel.h            旧版像素原语（ui_block / ui_label / ui_header_bar / ui_battery_pct）
└── src/ui_pixel.c

main/                             应用层
├── main.c                        入口：初始化硬件 → Router
├── app/                          App Router（全局按键分发 + 8 页循环导航）
│   ├── app_router.h / .c
│   └── app_pages.h               8 页模型
├── apps/                         各页面（每页独立模块）
│   ├── home/  profile/  status/  cards/
│   ├── dashboard/  tools/  games/  settings/
│   └── ...
├── badge/                        名牌数据/电源/UI（NVS 字段模型、深睡定时、唤醒）
├── avatar/                       头像存储（SPIFFS 挂载 + /avatar.bin save/load）
├── transport/ble.c               NimBLE GATT（资料 + 头像上传）
├── game/                         像素游戏（CATCH）
├── settings/                     旧设置（保留）
└── assets/                       静态资源（中文字库、头像素材）

android_app/                     安卓管理端（240×320 预览 + 头像裁剪上传）
partitions.csv                    factory(4MB) + storage(SPIFFS 2MB)
sdkconfig.defaults                ESP32-C3、USB console、Flash、LVGL 默认配置
docs/                             Passport OS V2 实施计划 / UI 设计规范 / 硬件参考 / 硬件开发指南
tests/                            主机端纯逻辑自检脚本
```

### 架构与依赖方向

```text
┌──────────────────────────────────────────────────────────────┐
│                       应用层                                 │
│  main.c ──► app/app_router（全局按键 + 8 页循环导航）        │
│               ├─► apps/（home/profile/status/cards/dashboard/│
│               │      tools/games/settings）                  │
│               ├─► badge/(数据/电源)  transport/ble + avatar  │
├──────────────────────────────────────────────────────────────┤
│                     UI 组件库                                │
│  components/ui/（ds_tokens/ds_widgets —— V2 设计系统；       │
│                  ui_pixel —— 旧版像素原语，纯 LVGL）         │
├──────────────────────────────────────────────────────────────┤
│                     硬件抽象层                              │
│  components/bsp/（显示、按键、音频、电池、I2C）            │
└──────────────────────────────────────────────────────────────┘
```

依赖方向：`main → ui → bsp`。`ui` 层是纯 LVGL，对硬件一无所知。每个页面（`apps/*`）实现 `enter` / `exit` /（可选）`key` / `action` 接口，统一挂到 Router 的页面表上，页面自身不接管全局按键。

---

## 真机验收清单

- USB Serial/JTAG 有稳定启动日志，无重启循环或 watchdog 复位。
- 显示方向、颜色、边缘与背光正确，深色极简配色与强调色渲染正常。
- `UP`/`DOWN` 8 页循环翻页，Page Indicator 跟随；`OK` 进入/操作当前页；长按 `UP`/`OK` 返回 HOME、长按 `DOWN` 快速切换状态。
- 各页面内容正确：HOME 头像/姓名/职位/状态；PROFILE 完整资料；STATUS 五档切换；CARDS 二维码可扫描；DASHBOARD 诚实数据；TOOLS/GAMES 列表选择与工具/游戏；SETTINGS 开关生效。
- 个人资料：BLE 写入后 UI 即时更新，重启后仍在。
- 头像：BLE 上传 → CRC 校验 → Flash 保存 → 重启恢复 → 正常显示。
- 电量读数合理，CW2017 缺失时降级显示 `--%`。
- 配置的闲置超时后深睡，任意键唤醒。
- 反复翻页/BLE 写入不持续泄漏任务、对象或堆。

---

## 文档

| 文档 | 用途 |
| --- | --- |
| [`docs/PASSPORT_OS_V2_AI_IMPLEMENTATION_PLAN.md`](docs/PASSPORT_OS_V2_AI_IMPLEMENTATION_PLAN.md) | Passport OS V2 实施计划与任务状态（唯一权威） |
| [`docs/UI_DESIGN_SPEC.md`](docs/UI_DESIGN_SPEC.md) | 240×320 每页坐标/字体/颜色/Page Indicator 设计规范 |
| [`docs/HARDWARE_REFERENCE.md`](docs/HARDWARE_REFERENCE.md) | 硬件事实与“绝不能碰”红线 |
| [`docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md`](docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md) | 完整硬件上下文与排障知识 |
| [`AGENTS.md`](AGENTS.md) | 编码、验证与贡献规则 |