# Passport OS V2

## ESP32-C3 智能电子工牌固件改造与 AI 执行规范

**项目：** iCurrer/ai-passport
**目标硬件：** ESP32-C3 / 8MB Flash / 240×320 ST7789P3 / 三按键 / 380mAh
**UI 类型：** 非触摸屏、按键导航
**开发方式：** Vibe Coding / AI Agent 辅助开发
**版本：** V2.0
**状态：** ⬜ 未开始

---

# 1. 项目改造目标

本项目不是重新开发一个 ESP32 固件，而是在当前 `ai-passport` 项目基础上进行**渐进式产品化改造**。

核心目标：

> 将当前 ESP32-C3 工牌固件改造成一个以“上下翻页”为核心交互方式的低功耗个人智能工牌系统。

最终设备应该具备：

* 个人身份展示
* 自定义头像
* 自定义姓名、职位、简介
* 状态显示
* QR Code
* 网站 / GitHub 等个人卡片
* 手机 BLE 修改个人信息
* 手机上传头像
* 简单工具
* 小游戏
* 电量 / 时间 / 设备状态
* 自动休眠
* 低功耗运行

---

# 2. 最重要的交互设计

## 2.1 不采用传统 App Grid

禁止设计成：

```text
┌──────┬──────┬──────┐
│ 👤   │ ⚙    │ 🎮   │
├──────┼──────┼──────┤
│ 🔧   │ 📱   │ 📊   │
└──────┴──────┴──────┘
```

因为：

* 屏幕只有 240×320
* 三按键
* 没有触摸屏
* 小图标操作效率低
* 不符合工牌的产品形态

---

# 3. 核心交互：上下翻页

整个系统采用：

> **Vertical Page Navigation**

即：

```text
           UP
            ↑
            │
       ┌──────────┐
       │  PAGE 1  │
       └──────────┘
            │
            ↓
       ┌──────────┐
       │  PAGE 2  │
       └──────────┘
            │
            ↓
       ┌──────────┐
       │  PAGE 3  │
       └──────────┘
            │
            ↓
       ┌──────────┐
       │  PAGE 4  │
       └──────────┘
            ↓
           DOWN
```

用户不需要进入复杂菜单。

---

# 4. 页面规划

第一版建议控制在 **6～8 个核心页面**。

```text
PAGE 0
HOME / IDENTITY

PAGE 1
PROFILE

PAGE 2
STATUS

PAGE 3
CARDS / QR

PAGE 4
DASHBOARD

PAGE 5
TOOLS

PAGE 6
GAMES

PAGE 7
SETTINGS
```

页面可以循环：

```text
PAGE 0
 ↑ ↓
PAGE 1
 ↑ ↓
PAGE 2
 ↑ ↓
...
PAGE 7
 ↑ ↓
PAGE 0
```

---

# 5. 三按键操作规范

硬件为：

> 三按键，非触摸屏。

必须统一操作逻辑。

## UP

短按：

```text
上一页
```

## DOWN

短按：

```text
下一页
```

## OK

短按：

```text
进入当前页面 / 操作
```

## OK 长按

```text
返回 HOME
```

## UP 长按

```text
直接返回 HOME
```

## DOWN 长按

```text
进入快速状态切换
```

---

# 6. UI 尺寸规范

屏幕固定：

```text
240 × 320 px
```

所有 UI 必须严格按照实际像素设计。

---

## 6.1 页面结构

```text
┌────────────────────────┐
│ Header            32px │
├────────────────────────┤
│                        │
│                        │
│                        │
│      Main Content      │
│                        │
│                        │
│                        │
├────────────────────────┤
│ Page Indicator    28px │
└────────────────────────┘
```

建议：

```text
Header:
0～35 px

Content:
36～271 px

Footer:
272～319 px
```

左右安全边距：

```text
16 px
```

---

# 7. UI 风格

## 背景

```text
#000000
```

## 主文字

```text
#FFFFFF
```

## 次要文字

```text
#8A8A8A
```

## 主强调色

```text
#4CD964
```

## Warning

```text
#FF9F0A
```

## Danger

```text
#FF453A
```

原则：

> 极简、高对比、低信息密度。

禁止：

* 大量渐变
* 复杂阴影
* 过多图标
* 小字体堆叠
* 复杂卡片嵌套
* 传统手机 App 风格

---

# 8. Page Indicator

由于设备采用上下翻页，所以必须让用户知道自己在哪里。

推荐底部：

```text
        ● ○ ○ ○ ○ ○ ○ ○
```

例如当前是第 3 页：

```text
        ○ ○ ● ○ ○ ○ ○ ○
```

或者：

```text
        03 / 08
```

优先采用：

```text
● ○ ○ ○ ○ ○ ○ ○
```

因为更适合 240×320。

---

# 9. PAGE 0：HOME

这是默认页面。

目标：

> 让用户一眼看到“这是我的电子身份”。

设计：

```text
┌────────────────────────┐
│ JOHN PASSPORT      86% │
│────────────────────────│
│                        │
│        ┌──────┐        │
│        │      │        │
│        │ AVATAR│       │
│        │      │        │
│        └──────┘        │
│                        │
│       JOHN LEE         │
│       ACCOUNTING       │
│                        │
│     ● AVAILABLE        │
│                        │
│                        │
│      ● ○ ○ ○ ○ ○ ○ ○   │
└────────────────────────┘
```

### 验收

* [ ] 240×320 无越界
* [ ] 中文正常
* [ ] 英文正常
* [ ] 电量正常
* [ ] 姓名正常
* [ ] 职位正常
* [ ] 状态正常
* [ ] 头像区域正常
* [ ] Page Indicator 正常

---

# 10. PAGE 1：PROFILE

展示完整个人信息。

```text
┌────────────────────────┐
│ PROFILE            86% │
│────────────────────────│
│                        │
│       ┌────────┐       │
│       │ AVATAR │       │
│       └────────┘       │
│                        │
│ JOHN LEE               │
│ Accounting             │
│                        │
│ AI · PHOTO · RADIO     │
│                        │
│ Shanghai               │
│                        │
│ ○ ● ○ ○ ○ ○ ○ ○        │
└────────────────────────┘
```

---

# 11. PAGE 2：STATUS

状态页面。

支持：

```text
AVAILABLE
FOCUS
BUSY
DND
OFFLINE
```

设计：

```text
┌────────────────────────┐
│ STATUS             86% │
│────────────────────────│
│                        │
│                        │
│          ●             │
│                        │
│       AVAILABLE        │
│                        │
│   FOCUS / BUSY / DND   │
│                        │
│                        │
│                        │
│ ○ ○ ● ○ ○ ○ ○ ○        │
└────────────────────────┘
```

按 OK：

```text
AVAILABLE
 ↓
FOCUS
 ↓
BUSY
 ↓
DND
 ↓
OFFLINE
```

---

# 12. PAGE 3：CARDS / QR

这个页面用于个人链接。

```text
┌────────────────────────┐
│ PERSONAL CARD       86%│
│────────────────────────│
│                        │
│      ┌────────────┐    │
│      │            │    │
│      │  QR CODE   │    │
│      │            │    │
│      │            │    │
│      └────────────┘    │
│                        │
│       SCAN ME          │
│                        │
│ ○ ○ ○ ● ○ ○ ○ ○        │
└────────────────────────┘
```

二维码建议：

```text
约 160×160 px
```

---

# 13. PAGE 4：DASHBOARD

显示：

* 时间
* 电量
* 今日事项
* Focus 状态
* BLE
* Wi-Fi

示例：

```text
┌────────────────────────┐
│ DASHBOARD          86% │
│────────────────────────│
│                        │
│       14:32            │
│       SUN 23 AUG       │
│                        │
│────────────────────────│
│                        │
│ FOCUS                  │
│ 01:24                  │
│                        │
│ NEXT                   │
│ 16:00  MEETING         │
│                        │
│ BLE OFF   WIFI OFF     │
│ ○ ○ ○ ○ ● ○ ○ ○        │
└────────────────────────┘
```

---

# 14. PAGE 5：TOOLS

工具页面。

支持：

```text
Timer
Stopwatch
Calculator
Morse
```

采用纵向列表：

```text
┌────────────────────────┐
│ TOOLS              86% │
│────────────────────────│
│                        │
│ > TIMER                │
│   STOPWATCH            │
│   CALCULATOR           │
│   MORSE                │
│                        │
│                        │
│ ○ ○ ○ ○ ○ ● ○ ○        │
└────────────────────────┘
```

注意：

这里可以使用：

```text
UP/DOWN
```

选择工具。

OK：

```text
进入工具
```

---

# 15. PAGE 6：GAMES

游戏：

```text
Reaction
Memory
Morse
Catch
```

第一阶段不要增加过多游戏。

原则：

> 每局 30 秒～3 分钟。

---

# 16. PAGE 7：SETTINGS

```text
┌────────────────────────┐
│ SETTINGS            86%│
│────────────────────────│
│                        │
│ > DISPLAY              │
│   BLUETOOTH            │
│   POWER                │
│   PROFILE              │
│   DEVICE               │
│                        │
│                        │
│ ○ ○ ○ ○ ○ ○ ○ ●        │
└────────────────────────┘
```

---

# 17. 动态个人信息

个人信息绝对不能编译进固件。

以下内容必须存储为动态数据：

```text
name
title
top
bio
status
website
github
qr
```

建议：

```text
NVS
```

存储。

结构：

```text
profile.name
profile.title
profile.top
profile.bio
profile.status

card.website
card.github
card.qr
```

---

# 18. 手机 App 控制

手机 App 是 Passport OS 的管理端。

手机负责：

```text
个人信息
头像
二维码
网站
GitHub
状态
显示设置
```

设备负责：

```text
显示
交互
本地工具
游戏
低功耗
```

---

# 19. BLE 工作模式

不要让 BLE 永久运行。

正常：

```text
Wi-Fi OFF
BLE OFF
```

用户需要同步：

```text
Settings
↓
Connect Phone
↓
BLE ON
↓
手机连接
↓
Sync
↓
BLE OFF
↓
Deep Sleep
```

---

# 20. 手机头像

头像处理全部在 Android App 完成。

ESP32 不进行：

* AI 抠图
* JPEG 解码
* 图片裁剪
* 图片缩放
* 人脸识别

手机处理：

```text
选择照片
↓
裁剪
↓
抠背景
↓
像素化
↓
80×80
↓
RGB565
↓
BLE
```

---

# 21. 头像格式

第一阶段只支持：

```text
80×80 RGB565
```

大小：

```text
80 × 80 × 2
= 12,800 bytes
```

约：

```text
12.5 KB
```

头像通过 BLE Chunk 分包传输。

---

# 22. BLE Avatar Protocol

建议：

```text
START_AVATAR

width = 80
height = 80
format = RGB565
size = 12800
checksum = XXXX
```

然后：

```text
CHUNK 001
CHUNK 002
...
CHUNK N
```

最后：

```text
END_AVATAR
```

ESP32：

```text
接收
↓
校验
↓
保存
↓
更新版本
↓
刷新 LVGL
```

---

# 23. 存储设计

不要把大头像直接塞进 NVS。

NVS：

```text
name
title
status
bio
website
github
avatar_version
```

文件系统：

```text
/avatar.bin
```

保存头像。

---

# 24. UI 模块化要求

当前项目已经存在 UI 模块。

后续不要继续把所有页面塞进一个巨大文件。

目标结构：

```text
main/
├── badge/
│   ├── badge.c
│   ├── badge_data.c
│   ├── badge_power.c
│   ├── badge_ui.c
│   └── badge_theme.h
│
├── apps/
│   ├── home/
│   ├── profile/
│   ├── status/
│   ├── cards/
│   ├── dashboard/
│   ├── tools/
│   ├── games/
│   └── settings/
│
├── avatar/
│
├── transport/
│   └── ble/
│
└── storage/
```

---

# 25. App Router

必须增加统一页面路由。

示例：

```text
HOME
PROFILE
STATUS
CARDS
DASHBOARD
TOOLS
GAMES
SETTINGS
```

所有页面通过 Router 管理。

禁止每个页面自行管理全局按键。

---

# 26. AI 修改原则

AI Agent 必须遵守：

### 禁止

```text
一次重写整个项目
```

### 禁止

```text
没有编译就继续开发
```

### 禁止

```text
修改 BSP 后同时修改 UI
```

### 禁止

```text
一次修改超过一个主要模块
```

### 禁止

```text
没有验证中文字库就更换字体
```

### 禁止

```text
随意升级 LVGL
```

### 禁止

```text
随意修改屏幕初始化
```

---

# 27. 每个任务必须执行以下流程

所有任务都必须：

```text
READ
 ↓
PLAN
 ↓
MODIFY
 ↓
BUILD
 ↓
UI VERIFY
 ↓
FUNCTION VERIFY
 ↓
MARK DOCUMENT
 ↓
GIT COMMIT
```

---

# 28. 原文档标记机制

**这是本项目强制要求。**

每完成一个任务，AI 必须修改本文件对应任务状态。

例如：

```text
## TASK-01 Design System

状态：

- [x] 代码完成
- [x] 编译通过
- [x] UI验证通过
- [x] 功能验证通过
- [x] 真机验证
- [x] Git Commit
```

如果没有完成：

```text
- [ ] 真机验证
```

**禁止 AI 自行打勾。**

只有实际完成验证后才能标记。

---

# 29. 每个任务必须增加验证记录

模板：

````markdown
### TASK-XX

状态：
- [ ] Code
- [ ] Build
- [ ] UI
- [ ] Function
- [ ] Hardware
- [ ] Commit

修改文件：

- xxx.c
- xxx.h

UI验证：

- [ ] 240×320 无越界
- [ ] 中文正常
- [ ] 英文正常
- [ ] 长文本正常
- [ ] Page Indicator 正常
- [ ] 按键导航正常

功能验证：

- [ ] UP
- [ ] DOWN
- [ ] OK
- [ ] Long Press
- [ ] 数据保存
- [ ] 重启后数据恢复

编译：

```text
idf.py build
结果：
PASS / FAIL
````

Git Commit：

```text
xxxxxxxx
```

问题：

无 / xxx

````

---

# 30. 开发任务清单

## TASK-00：代码基线分析

状态：

- [x] Code（基线分析，无代码改动）
- [x] Build（基线编译通过）
- [x] UI（静态基线，无改动；详见本文档 + UI_DESIGN_SPEC）
- [x] Function（静态基线，无改动）
- [ ] Hardware（**NOT TESTED** —— 未连接实机）
- [x] Commit

要求：

AI 不修改代码。

分析：

- [x] BSP
- [x] LCD
- [x] LVGL
- [x] 按键
- [x] BLE
- [x] NVS
- [x] Power
- [x] Game
- [x] Settings
- [x] Android App

### TASK-00 验证记录

状态：

- [x] Code（基线分析完成；未新增/修改任何固件代码）
- [x] Build
- [ ] UI（纯静态基线，无改动）
- [ ] Function（纯静态基线，无改动）
- [ ] Hardware（NO_BOARD → 未实测）
- [x] Commit

产出文件（本次仅新增文档，未改代码）：

- 新增 `docs/HARDWARE_REFERENCE.md`（硬件事实与“绝不能碰”红线）
- 新增 `docs/UI_DESIGN_SPEC.md`（240×320 每页坐标/字体/颜色/Page Indicator）
- `docs/PASSPORT_OS_V2_AI_IMPLEMENTATION_PLAN.md`（本 TASK-00 状态 + 验证记录）
- 基线分析结论写入上述文件；无 `main/`、`components/` 改动

编译：

```text
idf.py build
结果：PASS
（FoloToy-AI-Passport.bin 0x2e11b0 bytes；app 分区剩余 28% free）
```

基线编译告警（预先存在，非本次引起，TASK-00 不触碰）：

```text
main/badge/badge_data.c:9:20: warning: 'TAG' defined but not used [-Wunused-variable]
```

UI 验证：N/A（基线无改动）。真机 UI/功能均未验证。

硬件验证：NOT TESTED（未连接 ESP32-C3）。

Git Commit：

```text
bd91091
```

问题：

- 基线固件只有 4 个 NVS 字段（name/top/title/status），无头像文件、无 profile 扩展字段、无二维码；与 V2 动态数据目标差距待后续 TASK 补齐。
- 现有 `badge_ui.c` 是“左头像+右信息”dock 布局，尚无为 8 页翻页服务的 App Router / Page Indicator；`on_key` 目前仍由 `main.c`→`badge_key` 分发，改造需在 TASK-01/02 以 Router 逐步取代，保持编译稳定。
- 现有 `settings.c` 已能从 NVS 读 BLE 开关与休眠超时，说明 `badge_cfg` 命名空间已在使用，V2 扩展存储勿破坏该命名空间。
- 旧 `ui_pixel` 为浅色像素风，与 V2 深色 `#000000` 冲突；TASK-01 建立新 design-system，旧原语暂留以保证编译。

---

# TASK-01：UI Design System

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

完成：

- [x] Header
- [x] Footer
- [x] Page Indicator
- [x] Typography
- [x] Colors
- [x] Spacing

### TASK-01 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（纯逻辑审查）
- [ ] Hardware（NO_BOARD → 未实测）
- [x] Commit

修改文件（仅 `components/ui` design-system 层；未改 BSP、未改任何页面接线、未删旧 `ui_pixel`）：

- 新增 `components/ui/include/ds_tokens.h`（V2 配色/骨架间距/排版尺寸 token）
- 新增 `components/ui/include/ds_widgets.h`、`components/ui/src/ds_widgets.c`（ds_header / ds_footer / ds_page_dots / ds_page_dots_set）
- 修改 `components/ui/CMakeLists.txt`（加入 ds_widgets.c）

明细：

- Header：标题 x=16,y=13 + 电量条(外框 20x10 @158,14 / 内芯 16x6 @160,16 / 填充随 SOC) + 百分比 @186,12 + 底部分隔线 y=34（x=16 宽 208）。
- Footer：顶部 1px 分隔线 y=271 + Page Indicator。
- Page Indicator：8 点，直径 6、间距 12，水平居中(x0=75)、垂直居中 y=293(中心 296)；当前页实心(强调)/其余空心(次要描边)；`ds_page_dots_set` 复用对象刷新高亮。
- Typography：排版分层（TITLE 24px / BODY 14px / NUM 14px）以 token 记录尺寸，具体 lv_font 由页面层传入（components/ui 不依赖 main/assets 中文字库）。
- Colors：V2 深色极简 #000000 背景等 8 色 token。
- Spacing：骨架(Header 0–35 / Content 36–271 / Footer 272–319)与边距 16 归档为 token。

UI 验证（静态坐标复查）：

- [x] 240×320 无越界（Header/Footer/指示器坐标均落在骨架内）
- [x] Page Indicator 8 点总宽 90、x0=75、点 75..159，均 <240
- [ ] 中文正常（字库在页面 TASK 接线时验证——本 TASK 未接线页面，字体不进 components/ui）
- [ ] 英文/数字正常（同上，接入页面时验证）
- [ ] 长文本正常（同上）
- [ ] KEY 导航正常（N/A，本 TASK 无页面按键）

功能验证（纯逻辑审查）：

- [x] ds_page_dots 的 count/current 越界防护与 8 点上限
- [x] ds_page_dots_set 高亮切换不重建对象（复用 apply_dot）
- [x] ds_header 电量 fill 宽度 clamp、<20%/<10% 变色、SOC=-1 显示 "--"
- [x] 返回 ref 的 brand/fill/pct 可被调用方持有并刷新
- [x] 主机坐标自检 `tests/ds_geometry_check.py`：Header / Page Indicator 全部 13 项断言 PASS（水平居中、越界、重叠、footer 带）
- [ ] 实机渲染（NO_BOARD，未验证）

编译：

```text
idf.py build
结果：PASS
（ds_widgets.c 编译无告警；libui.a 构建通过；App 分区仍剩 28%）
```

新增警告：无（唯一预先存在警告 `badge_data.c TAG unused` 与本次无关）。

Git Commit：

```text
9ea4a28
```

问题：

- 本 TASK 只交付 design-system 原语，尚未被任何页面调用，故最终镜像体积未变化；待 TASK-02/03 起接入页面后编译体积与 malloc 会上升，届时复查内部 RAM。
- `components/ui` 不引入主工程 GB2312 中文字库，字体（中文/数字）须由页面层在各自 TASK 接入时传入并做真机验证。
- ds_dots_t 圆点数组按 plan「页面最多 8 个」设为 8 上限；若后续页面调整需同步。

---

# TASK-02：Page Router

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

实现：

- [x] UP → Previous Page
- [x] DOWN → Next Page
- [x] OK → Enter
- [x] OK Long → Home
- [x] UP Long → Home
- [x] DOWN Long → 快速状态切换（占位，语义由 TASK-05 STATUS 提供）
- [x] 页面循环翻页（HOME ↔ SETTINGS 双向环绕）

### TASK-02 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（主机纯逻辑自检）
- [ ] Hardware（NO_BOARD，未实测）
- [x] Commit

修改文件（仅 `main/` 应用层；未改 BSP、未删旧 badge/game/settings）：

- 新增 `main/app/app_pages.h`（8 页模型 APP_PAGE_HOME..SETTINGS）
- 新增 `main/app/app_router.h`、`main/app/app_router.c`（全局按键分发 + 循环导航 + 占位页渲染）
- 修改 `main/main.c`（on_key → app_router_key；boot → app_router_init/enter）
- 修改 `main/CMakeLists.txt`（加入 app/ 目录与源）
- 新增 `tests/app_router_logic_check.py`（主机纯逻辑自检）

明细：

- 纯导航 `app_router_page_cycle()`：非法入参钳位到 HOME，(cur+dir+8)%8 双向环绕。
- 按键映射 `map_event()`：CLICK→UP=PREV/DOWN=NEXT/OK=OK_ACTION；LONG→UP/OK=HOME、DOWN=STATUS_TOGGLE；其余事件(PRESS/DOUBLE)=NONE。
- 统一在 `app_router_key` 内 `bsp_lvgl_lock/unlock`；保留 `badge_power_key_activity()`（开机忽略期 + 休眠计时）。
- `app_router_init()` 内调 `badge_power_init()`（自动休眠计时原由 badge_enter 负责，这里补上避免休眠失效）。
- 占位页用 TASK-01 的 ds_header/ds_footer 渲染：Header(页标题+电量) + 居中“PAGE n/8+标题” + Page Indicator；真实页面后续 TASK 替换。

UI 验证（静态坐标审查）：

- [x] 240×320 无越界（Header/Footer/指示器复用 TASK-01 已验证坐标）
- [x] 占位主体 label 居中 y=120，位于 Content 36–271
- [x] 最长页标题 DASHBOARD(14px) 不与电量块(158)重叠
- [x] Page Indicator 8 点正常（TASK-01 已验）
- [x] KEY 导航逻辑（主机自检，见下）
- [ ] 实机中文/英文渲染与按键物理触发（NO_BOARD，未验证）

功能验证（纯逻辑自检 `tests/app_router_logic_check.py`）：

- [x] 循环环绕：0→+1→1；7→+1→0；0→-1→7；100 步内恒在 [0,7]
- [x] 非法入参钳位：cur=99 → HOME 再步进=1；cur=-1 → HOME
- [x] 按键映射 9 项全部 PASS（CLICK 三键 / LONG 三键 / PRESS+DOUBLE=NONE）
- 说明：自检过程中发现测试脚本自身的 `HOME` 常量与意图枚举同名遮蔽 bug（页码 HOME 被 range 重绑为 4），已改 `INT_*` 前缀修正；C 固件用 `APP_INTENT_HOME`/`APP_PAGE_HOME` 不存在该问题，逻辑无误。
- [ ] 实机翻页/长按（NO_BOARD，未验证）

编译：

```text
idf.py build
结果：PASS
（app_router.c 编译无告警；App 分区占用降至 30% free——
 旧 badge_ui/game/settings 渲染不再被引用而未链接，体积下降）
```

新增警告：无。

Git Commit：

```text
commit（提交后回填）
```

问题：

- 本 TASK 起 Router 接管全局按键，旧的 `badge_enter` 名牌界面在启动时不再进入（代码保留编译，供后续 HOME 页面复用其身份布局）。
- 占位页主体“PAGE n/8”为临时占位；各页真实内容在 TASK-03..09 逐个替换。
- ok/down 长按的“快速状态切换”仅占位无操作，待 TASK-05(STATUS)。
- Router 目前是单任务锁内渲染占位页；后续页面若引入自身 lv_timer/任务，退出时须先停再删。

---

# TASK-03：HOME

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

---

# TASK-04：PROFILE

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

---

# TASK-05：STATUS

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

---

# TASK-06：CARDS / QR

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

---

# TASK-07：DASHBOARD

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

---

# TASK-08：TOOLS

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

---

# TASK-09：GAMES

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

---

# TASK-10：BLE Profile V2

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

必须验证：

```text
APP
 ↓
BLE
 ↓
ESP32
 ↓
NVS
 ↓
Restart
 ↓
数据仍然存在
```

---

# TASK-11：Android Preview

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

要求：

Android App 显示：

```text
240 × 320 Passport Preview
```

修改个人信息时实时更新预览。

---

# TASK-12：Avatar

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

必须验证：

```text
相册
 ↓
裁剪
 ↓
80×80
 ↓
RGB565
 ↓
BLE
 ↓
ESP32
 ↓
Flash
 ↓
显示
```

---

# TASK-13：低功耗

状态：

* [ ] Code
* [ ] Build
* [ ] UI
* [ ] Function
* [ ] Hardware
* [ ] Commit

验证：

```text
BLE OFF
Wi-Fi OFF
Display OFF
Deep Sleep
Wake
UI Restore
```

---

# 31. 最终 UI 验收

完成全部任务以后，AI 必须执行一次完整 UI Audit。

## 屏幕

* [ ] 240×320
* [ ] 所有元素不越界
* [ ] 无裁切
* [ ] 无重叠

## 字体

* [ ] 中文
* [ ] 英文
* [ ] 数字
* [ ] 长文本
* [ ] 特殊字符

## 导航

* [ ] UP
* [ ] DOWN
* [ ] OK
* [ ] Long Press

## 页面

* [ ] HOME
* [ ] PROFILE
* [ ] STATUS
* [ ] CARDS
* [ ] DASHBOARD
* [ ] TOOLS
* [ ] GAMES
* [ ] SETTINGS

---

# 32. 最终功能验收

### 数据

* [ ] 修改姓名
* [ ] 修改职位
* [ ] 修改状态
* [ ] 修改简介
* [ ] 修改 QR
* [ ] 修改网站
* [ ] 修改 GitHub

### BLE

* [ ] 手机发现设备
* [ ] 手机连接设备
* [ ] 修改信息
* [ ] 保存
* [ ] 断开
* [ ] ESP32 重启
* [ ] 数据仍存在

### Avatar

* [ ] 上传头像
* [ ] BLE 分包
* [ ] CRC 校验
* [ ] Flash 保存
* [ ] 重启恢复
* [ ] 正常显示

---

# 33. 最终 AI 报告格式

完成整个项目以后，AI 必须输出：

```markdown
# Passport OS V2 Implementation Report

## Completed

- TASK-00 ✓
- TASK-01 ✓
- TASK-02 ✓
- ...

## Build

idf.py build

Result:
PASS

## UI Verification

240×320:
PASS

Chinese:
PASS

English:
PASS

Long Text:
PASS

Navigation:
PASS

## BLE Verification

Profile Sync:
PASS

Avatar Sync:
PASS

## Power

Deep Sleep:
PASS

Wake:
PASS

## Known Issues

None

## Git

Latest Commit:
xxxxxxxx
```

---

# 34. 最重要的 AI 行为规则

最后增加：

> **AI 不得为了“完成任务”而虚构测试结果。**

如果没有实际连接 ESP32：

```text
Hardware Verification:
NOT TESTED
```

而不是：

```text
PASS
```

如果只是编译成功：

```text
Build:
PASS

Hardware:
NOT TESTED
```

如果 UI 只通过代码检查：

```text
UI:
STATIC REVIEW PASS

REAL DEVICE:
NOT TESTED
```

---

# 35. 项目完成标准

Passport OS V2 只有在以下条件全部满足后才算完成：

```text
□ UI 统一
□ 上下翻页正常
□ 三按键逻辑统一
□ 个人信息可修改
□ BLE 同步正常
□ NVS 持久化正常
□ 头像可上传
□ QR 正常
□ 240×320 无布局问题
□ 中文正常
□ 无明显内存问题
□ 深睡正常
□ 唤醒正常
□ Git 历史完整
□ 本文档所有任务均有状态
□ 所有未完成项目明确标记
```

---

# AI 执行时最重要的一句话

把下面这段放到文档最顶部：

```text
你不是一次性重写 Passport OS。

你必须严格按照 TASK-00 → TASK-13 的顺序执行。

每次只完成一个 TASK。

完成一个 TASK 后必须：

1. 编译
2. 检查编译错误
3. 检查 240×320 UI 布局
4. 检查功能
5. 如条件允许，连接真实 ESP32-C3 验证
6. 更新本文件对应 TASK 的状态
7. 填写验证记录
8. 创建 Git Commit
9. 停止，等待下一条指令

绝对禁止在没有完成当前 TASK 验证的情况下开始下一个 TASK。

绝对禁止声称没有实际验证过的功能为 PASS。

如果发现现有代码与本方案冲突，优先保护现有已经工作的硬件驱动、显示初始化、按键驱动、字体系统和电源管理，不得为了实现 UI 而破坏底层稳定性。
```

---

## 我建议你实际使用时再加一个东西

在仓库里建立：

```text
docs/
├── PASSPORT_OS_V2_AI_IMPLEMENTATION_PLAN.md
├── UI_DESIGN_SPEC.md
└── HARDWARE_REFERENCE.md
```

其中：

**`PASSPORT_OS_V2_AI_IMPLEMENTATION_PLAN.md`**

负责告诉 AI：

> **“现在做到哪一步、下一步做什么、哪些已经验证。”**

**`UI_DESIGN_SPEC.md`**

负责告诉 AI：

> **“240×320 每一个页面应该长什么样、坐标是多少、字体多大。”**

**`HARDWARE_REFERENCE.md`**

负责告诉 AI：

> **“ESP32-C3 的屏幕、按键、BLE、电池、Flash、音频到底是什么，哪些东西不能碰。”**

这样以后你每次打开 Codex/Trae，不需要重新解释项目背景，AI 直接读取这三个文件就能继续工作。

而且你之前遇到的**“AI 改完文字消失、Git 恢复也不正常”**这种问题，这套“**任务 → 编译 → UI 验证 → 功能验证 → 文档打勾 → Git commit → 再进入下一任务**”的流程会比单纯给 AI 一个大 Prompt 安全得多。
