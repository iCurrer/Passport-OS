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
- [ ] 实机中文/英文渲染与按键物理触发（部分验证：固件已烧录并正常启动，显示/电池/按键初始化通过；屏幕内容与按键导航未实际交互）

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
17c4480
```

问题：

- 本 TASK 起 Router 接管全局按键，旧的 `badge_enter` 名牌界面在启动时不再进入（代码保留编译，供后续 HOME 页面复用其身份布局）。
- 占位页主体“PAGE n/8”为临时占位；各页真实内容在 TASK-03..09 逐个替换。
- ok/down 长按的“快速状态切换”仅占位无操作，待 TASK-05(STATUS)。
- Router 目前是单任务锁内渲染占位页；后续页面若引入自身 lv_timer/任务，退出时须先停再删。

---

# TASK-03：HOME

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-03 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（纯逻辑审查）
- [ ] Hardware（部分验证：已烧录启动正常，显示/电池/按键初始化通过；屏幕内容与按键导航未实际交互）
- [x] Commit

修改文件（仅 `main/` 应用层；未改 BSP、未改 ui 层、未动 TASK-02 已验逻辑）：

- 新增 `main/apps/home/home.h`、`main/apps/home/home.c`（V2 HOME 页渲染）
- 修改 `main/app/app_router.c`（引入按页渲染表 s_pages，HOME 用真实渲染、其余页占位；init 补 badge_data_init）
- 修改 `main/CMakeLists.txt`（加入 apps/home）

明细：

- HOME 用 TASK-01 design-system 渲染：Header(顶部文字 top + 电量) + 头像(现有 80x157 素材等比缩至约 56x110,居中上段) + 姓名(24px) + 职位(14px) + 状态(色点+文字成组居中) + Footer Page Indicator(HOME=第 0 点实心)。
- 数据显示动态字段来自 badge_data(NVS):top/name/title/status,绝不写死进固件。
- Router 改为按页表 `{build,destroy}`:TASK-03 仅 HOME 槽位用 home*,其余槽位仍用占位渲染;后续页面 TASK 逐个替换,无需改动导航与按键逻辑。
- `app_router_init()` 新增 `badge_data_init()`(原由 badge_enter 负责),确保 NVS 字段在 Router 启动时加载。

UI 验证（静态坐标审查，Content 36–271）：

- [x] 头像 TOP_MID y=56,约 56x110(scale 180),y 56..166 居中不越界
- [x] 姓名 24px y 178..202、职位 14px y 212..226、状态 14px y 244..258 —— 均在 Content 内,不触 Footer(272)
- [x] Header / Page Indicator 复用 TASK-01/02 已验坐标
- [x] 背景 DS_BG 黑色、主文字/强调色符合 V2 配色
- [ ] 中文/英文/长文本实机渲染（NO_BOARD，未验证；长字段无换行，NAME/TITLE 过长会横向溢出——沿用旧行为，后续 UI audit 处理）
- [ ] 头像显示与字号实机效果（NO_BOARD）

功能验证（纯逻辑审查）：

- [x] HOME 页 enter 构建并加载独立 screen,exit 删除 screen(键/渲染均在 Router 锁内)
- [x] 动态字段读取走 badge_data(NVS),默认值回退(李秋实/豆包大学/自由/FoloToy)
- [x] 占位页保留,未实现页仍可翻页显示
- [ ] 实机翻页到 HOME/退出 HOME(OSC_BOARD,未验证)

编译：

```text
idf.py build
结果：PASS
（home.c 编译无告警;.bin=0x2da940,App 分区剩 29%）
```

新增警告：无。

Git Commit：

```text
a508738
```

问题：

- 头像仍为现有全身素材的等比缩放,非规范 80×80;真正的 80×80 自定义头像文件在 **TASK-12(Avatar)** 接入。
- 运行时头像缩放(scale=180)对像素风素材较清晰,但若后续换高分辨率需复核渲染与 RAM。
- NAME/TITLE 长文本无自动换行,超宽会横向溢出(与旧 badge 行为一致);列入最终 UI audit。

---

# TASK-04：PROFILE

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-04 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（纯逻辑审查）
- [ ] Hardware（已烧录；PROFILE 页内容未实机确认）
- [x] Commit

修改文件（仅 `main/` 应用层；未改 BSP、未改 ui 层、未动已验导航/按键逻辑）：

- 新增 `main/apps/profile/profile.h`、`main/apps/profile/profile.c`（V2 PROFILE 页渲染）
- 修改 `main/badge/badge.h`（badge_field_t 新增 BIO/WEBSITE/GITHUB）
- 修改 `main/badge/badge_data.c`（新增 3 个 NVS 字段与读写；nvs_load_str 增加空默认值防回写）
- 修改 `main/app/app_router.c`（页面表 PROFILE 槽位换成 profile_enter/exit）
- 修改 `main/CMakeLists.txt`（加入 apps/profile）

明细：

- PROFILE 用 ds_header/ds_footer 渲染：Header("PROFILE"+电量) + **纯文字档案**(无头像)：姓名(24px)→职位(14px)→强调分隔线→状态(色点+文字)→简介/网站/GitHub(14px)。
- 采用垂直累计 y 游标,字段空时自动收起不留空行;状态/分隔线保持视觉节奏。
- 数据全部来自 badge_data(NVS)：name/title/status/bio/website/github；**空字段自动跳过**。
- 数据模型扩展：badge_field_t 新增 bio/website/github 三字段并持久化 NVS；nvs_load_str 空默认值不再每次开机回写 NVS。
- 按用户反馈去掉 PROFILE 头像,仅文字展示(原版用全身素材缩略,已移除)。

UI 验证（静态坐标审查，Content 36–271）：

- [x] 纯文字布局：姓名 24px y56..80、职位 98、分隔线 122、状态 148、简介 178、网站 204、GitHub 230..244 —— 全部在场时末尾仍 <271,不触 Footer(272)
- [x] 空字段自动收起,不留空行/重叠
- [ ] 中文/英文/长文本实机渲染（未实机确认；长 bio/website 无换行会横向溢出,沿用旧行为）

功能验证（纯逻辑审查）：

- [x] 新字段 NVS 读/写与默认空值逻辑（badge_data_get/set 全覆盖）
- [x] PROFILE enter/exit 构建/销毁独立 screen
- [x] Router 表仅替换 PROFILE 槽位,其余页不变
- [ ] 实机翻页到 PROFILE（未实机确认）

编译：

```text
idf.py build
结果：PASS
（profile.c 编译无告警；.bin=0x2dac60，App 分区剩 29%）
```

新增警告：无。

Git Commit：

```text
f643b3c
```

问题：

- PROFILE 头像暂用全身素材缩略显示,非规范头像文件;TASK-12 接入真 80×80 头像。
- 新增的 bio/website/github 字段尚未暴露给 BLE 手机侧(当前 BLE 只暴露 name/top/title/status);BLE 字段扩展在后续 BLE/同步 TASK。
- 长 bio/website 无自动换行,超宽横向溢出;列入最终 UI audit。

---

# TASK-05：STATUS

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-05 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（主机纯逻辑自检）
- [ ] Hardware（已烧录；STATUS 页内容与按键未实机确认）
- [x] Commit

修改文件（仅 `main/` 应用层；未改 BSP、未改 ui 层、未动已验导航/按键逻辑）：

- 新增 `main/apps/status/status.h`、`main/apps/status/status.c`（V2 STATUS 页 + status_cycle）
- 修改 `main/app/app_router.c`（页面表加 action 回调;STATUS 槽位用 status*;OK_ACTION 转发;DOWN 长按 → status_cycle）
- 修改 `main/CMakeLists.txt`（加入 apps/status）
- 新增 `tests/status_cycle_check.py`（主机纯逻辑自检）

明细：

- STATUS 页：ds_header("STATUS"+电量) + 当前状态大字(24px 强调) + 强调分隔线 + 5 档列表(当前档强调/其余灰字) + Footer 指示器。
- 5 档预设 `AVAILABLE→FOCUS→BUSY→DND→OFFLINE`;`status_cycle()` 切下一档并写 NVS(BADGE_FIELD_STATUS),在场时同步刷新页面。
- Router：页面表新增 `action`(OK 短按"操作当前页");STATUS 页 action=status_cycle;**DOWN 长按(APP_INTENT_STATUS_TOGGLE)全局调用 status_cycle**——补齐 TASK-02 预留的快速状态切换。
- 兼容：当前状态不在预设(如旧默认"自由")时 find_index=-1,首次切换跳到 AVAILABLE。
- 线程：status_cycle 假定调用方已持 LVGL 锁(由 router 持锁调用);页面不在场时仅更新 NVS。

UI 验证（静态坐标审查，Content 36–271）：

- [x] 大字 24px y86..110、分隔线 120、5 行列表 y142..230 步进 22 —— 末尾 244 <271,不触 Footer(272)
- [x] 列表高亮逻辑(当前档强调/其余灰)
- [ ] 实机状态大字与列表渲染（未实机确认）
- [ ] 实机 OK 短按 / DOWN 长按切换效果（未实机确认）

功能验证（纯逻辑自检 `tests/status_cycle_check.py`）：

- [x] find_index：预设 0/4、非预设("自由")/空/None → -1
- [x] next：AVAILABLE→FOCUS→…→OFFLINE→AVAILABLE(环绕);非预设/空 → AVAILABLE(0)
- [x] 5 步循环回到起点
- [ ] 实机按键循环切换（未实机确认）

编译：

```text
idf.py build
结果：PASS
（status.c 编译无告警；.bin=0x2db140，App 分区剩 29%）
```

新增警告：无。

Git Commit：

```text
0bd82c2
```

问题：

- 状态文案改为 5 档英文预设并覆盖旧默认"自由";HOME/PROFILE 也读同一 status 字段,会同步显示新档。
- 长状态文案无自动换行(预设为短英文,风险低)。
- action 回调当前仅 STATUS 页使用;其余页为 NULL,后续页 TASK 按需填充。

---

# TASK-06：CARDS / QR

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-06 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（纯逻辑审查 + 编译）
- [ ] Hardware（已烧录；CARDS 页二维码实机未确认）
- [x] Commit

修改文件（`main/` 应用层 + 新增组件依赖；未改 BSP、未改 ui 层）：

- 新增 `main/apps/cards/cards.h`、`main/apps/cards/cards.c`（V2 CARDS/QR 页 + 动态二维码渲染）
- 新增 `main/idf_component.yml`（依赖 `espressif/qrcode: ^0.2.0`）
- 修改 `main/CMakeLists.txt`（REQUIRES 加 espressif__qrcode；SRCS/INCLUDE_DIRS 加 apps/cards）
- 修改 `main/app/app_router.c`（CARDS 槽位用 cards_enter/exit）
- 拉取到 `managed_components/espressif__qrcode`（基于 Nayuki qrcodegen，轻量，无 PSRAM 友好）

明细：

- 二维码内容来自 NVS：BADGE_FIELD_WEBSITE，空则回退 GITHUB，全空则显示 "NO CARD DATA" + "SCAN ME"。
- 用 `esp_qrcode_generate` 动态编码，`display_func_with_cb` 回调里把模块矩阵画进**单个 RGB565 bitmap**（不是数百个小块,省对象省 RAM）。
- 位图尺寸自适应：px = size × module_px ≤ 116px,缓冲 ≤ 约27KB;**仅 CARDS 页驻留时占用,退出 cards_exit 即 free**。
- 副标题显示二维码内容(网站/GitHub)。
- 内容过长导致 generate 失败时显示 "QR TOO LONG"(Danger)。

UI 验证（静态坐标审查，Content 36–271）：

- [x] 二维码居中 y48,≤116px(48..164)、副标题 y200 —— 均在 Content 内,不触 Footer(272)
- [ ] 二维码实机可扫描性（未实机确认；需先设置 website 才生成二维码）
- [ ] 中文/英文/长文本副标题实机渲染（未实机确认；长 URL 会横向溢出,沿用旧行为）

功能验证：

- [x] 编译通过(依赖 espressif__qrcode 正常拉取与链接)
- [x] 位图缓冲分配/释放路径(cards_exit 释放)
- [x] 内容选择逻辑:website→github→无(纯逻辑可读)
- [ ] 实机二维码生成与扫描（未实机确认；当前 website 为空,显示占位）

编译：

```text
idf.py build
结果：PASS
（cards.c 编译通过;依赖 espressif__qrcode 拉取成功;.bin=0x2de080,App 分区剩 28%）
```

新增警告：无（以实际构建输出为准）。

Git Commit：

```text
60b23ce
```

问题：

- 二维码内容当前依赖 website/github NVS 字段;二者默认空 → 实机会显示占位。待 BLE 暴露 website 后可写入真实链接再验证二维码。
- 位图缓冲 ≤27KB 在无 PSRAM 下可接受,但若与其它大分配叠加需复核 free 堆;page 内无并发大分配。
- 完整 vCard 名片二维码(含姓名/职位等多字段)为后续增强;当前仅编码单个链接。

---

# TASK-07：DASHBOARD

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-07 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（纯逻辑审查）
- [ ] Hardware（已烧录；DASHBOARD 页实机未确认）
- [x] Commit

修改文件（仅 `main/` 应用层；未改 BSP、未改 ui 层）：

- 新增 `main/apps/dashboard/dashboard.h`、`main/apps/dashboard/dashboard.c`（V2 DASHBOARD 页 + 每秒刷新定时器）
- 修改 `main/app/app_router.c`（DASHBOARD 槽位用 dashboard_enter/exit）
- 修改 `main/CMakeLists.txt`（加入 apps/dashboard）

明细：

- **诚实无假数据**：本机无 RTC、Wi-Fi/BLE 默认关，无可靠墙钟来源 → 不造假时间/日期。
- 展示真实状态：**UPTIME**（esp_timer 运行时长,大号数字实时刷新）、本页 **FOCUS** 会话计时、**BLE/WIFI** 开关、NEXT 日程占位 "NONE"。
- 每秒用 `lv_timer`(LVGL 任务内)刷新 UPTIME 与 FOCUS;**退出先 `lv_timer_del` 再删对象**(红线)。

UI 验证（静态坐标审查，Content 36–271）：

- [x] UPTIME 大号数字 y56、标签 86、分隔线 112、4 行状态 y132..198(步进 22) —— 均在 Content 内,不触 Footer(272)
- [ ] 实机 UPTIME/FOCUS 秒级刷新与数字渲染（未实机确认）
- [ ] 实机 BLE/WIFI 状态显示（未实机确认）

功能验证（纯逻辑审查）：

- [x] lv_timer 创建/销毁路径(dashboard_exit 先停再删)
- [x] UPTIME 秒→HH:MM:SS、FOCUS 秒→MM:SS 格式换算
- [x] BLE 状态读 ble_is_enabled();WIFI 未启用恒 OFF(诚实)
- [ ] 实机定时器刷新与退出无泄漏（未实机确认）

编译：

```text
idf.py build
结果：PASS
（dashboard.c 编译通过;.bin=0x2e40f0,App 分区剩 28%）
```

新增警告：无。

Git Commit：

```text
242521a
```

问题：

- 无 RTC → 无墙钟时间/日期,以 UPTIME 代替;若后续要真时间需接 SNTP(Wi-Fi,违反默认关)或外部 RTC。
- NEXT 日程无数据源,占位 "NONE";未来可从 NVS 读取日程或接入 BLE 同步。
- FOCUS 为"本页会话"计时,非真实工作会话;语义可后续调整。

---

# TASK-08：TOOLS

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-08 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（主机纯逻辑自检）
- [ ] Hardware（已烧录；TOOLS 页与秒表实机未确认）
- [x] Commit

修改文件（仅 `main/` 应用层；未改 BSP、未改 ui 层）：

- 新增 `main/apps/tools/tools.h`、`main/apps/tools/tools.c`（V2 TOOLS 列表页 + 秒表工具）
- 修改 `main/app/app_router.c`（页面表加 `key` 局部按键回调；短按优先交给页面消费；TOOLS 槽位用 tools*）
- 修改 `main/CMakeLists.txt`（加入 apps/tools）
- 新增 `tests/tools_list_check.py`（主机纯逻辑自检）

明细：

- TOOLS 列表：TIMER/STOPWATCH/CALCULATOR/MORSE；UP/DOWN 选择(高亮,选中=卡片底+强调文字)，OK 进入工具。
- **交互模型扩展**：Router 页面表新增 `key` 回调——短按先交给当前页局部消费(列表选择/工具操作)，消费不了才走全局翻页；**长按仍全局**(OK/UP 回 HOME、DOWN 快速状态)。
- STOPWATCH 已实现(OK 启动/停止,秒表 MM:SS 每秒刷新);TIMER/CALCULATOR/MORSE 为 COMING SOON 占位屏。
- 工具屏 UP/DOWN 走全局翻页离开;OK 长按回 HOME。

UI 验证（静态坐标审查，Content 36–271）：

- [x] 列表 4 行 y56..200(步进 48,行高 44) —— 末行 244 <271,不触 Footer
- [x] 秒表大字 y100、提示 y152
- [x] 选中高亮(卡片底 #111111 / 强调文字)与未选中(次要色)区分
- [ ] 实机列表高亮移动与秒表计时渲染（未实机确认）

功能验证（纯逻辑自检 `tests/tools_list_check.py`）：

- [x] 列表选择 UP/DOWN 环绕(4x DOWN 回 0,4x UP 从 3 回 3)
- [x] 秒表 OK 启停与累计逻辑、lv_timer 创建/销毁(tools_exit 先停再删)
- [x] Router 短按委托:页面 key 消费优先,长按仍全局
- [ ] 实机按键列表选择/进入秒表/启停（未实机确认）

编译：

```text
idf.py build
结果：PASS
（tools.c 编译通过;.bin=0x2e48c0,App 分区剩 28%）
```

新增警告：无。

Git Commit：

```text
722b9f2
```

问题：

- 列表页内 UP/DOWN 短按被用于选择,故在该页无法用短按直接翻页;离开列表页用长按(OK/UP=HOME)或进入工具后 UP/DOWN 翻页。此为"页面局部按键优先"设计取舍,待真机确认是否符合预期。
- 秒表无"归零"操作(仅启停),可后续加 RESET。
- 其余工具(TIMER/CALCULATOR/MORSE)为占位,后续 TASK/迭代实现。

---

# TASK-09：GAMES

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-09 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（纯逻辑审查）
- [ ] Hardware（已烧录；GAMES 页与 CATCH 游戏实机未确认）
- [x] Commit

修改文件（`main/` 应用层;含对现有 game.c 的适配;未改 BSP、未改 ui 层）：

- 新增 `main/apps/games/games.h`、`main/apps/games/games.c`（V2 GAMES 列表页 + CATCH 接入）
- 修改 `main/game/game.h`、`main/game/game.c`（适配 Router:移除 badge.h 依赖;game_key 返回结果且不再自加锁,由调用方持锁）
- 修改 `main/app/app_router.c`（GAMES 槽位用 games_enter/exit/games_key）
- 修改 `main/CMakeLists.txt`（加入 apps/games）

明细：

- GAMES 列表：REACTION/MEMORY/MORSE/**CATCH**;UP/DOWN 选择(高亮同 TOOLS),OK 进入。
- **CATCH 接入现有 game.c(Pixel Catcher)**：game_key 改为返回 `GAME_KEY_CONSUMED/EXITED/NONE`,去掉内部锁(符合 Router 委托契约);游戏结束 OK → `game_exit()` 并返回 EXITED,GAMES 页重建菜单。
- 其余游戏 COMING SOON 占位。
- 全局语义:短按页面局部优先;长按全局(OK/UP 回 HOME)。

UI 验证（静态坐标审查，Content 36–271）：

- [x] 列表 4 行 y56..200(步进 48,行高 44) —— 末行 244 <271,不触 Footer
- [x] 选中高亮(卡片底/强调文字)与未选中(次要色)
- [ ] 实机 GAMES 列表高亮移动与 CATCH 游戏画面（未实机确认）

功能验证（纯逻辑审查）：

- [x] GAMES 选择 UP/DOWN 环绕(复用 TOOLS 自检逻辑)
- [x] game_key 契约:消耗/退出/未消费;GAMES 页在 EXITED 后重建菜单
- [x] games_exit 先 game_exit(停游戏循环)再删屏(红线)
- [ ] 实机 CATCH 操作(UP/DOWN 移篮、接宝石)与 Game Over 返回菜单（未实机确认）

编译：

```text
idf.py build
结果：PASS
（game.c 适配 + games.c 编译通过;.bin=0x2e54d0,App 分区剩 28%）
```

新增警告：无。

Git Commit：

```text
6f91d88
```

问题：

- game.c 游戏屏仍用旧磨砂绿主题色(非 V2 纯黑),为遗留游戏模块;如需统一配色可后续调整。
- 列表页内 UP/DOWN 短按用于选择,离开用长按(与 TOOLS 一致)。
- REACTION/MEMORY/MORSE 为占位,后续迭代实现。

---

# TASK-10：PROFILE SYNC / BLE 数据

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-10 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（无 UI 改动）
- [x] Function（纯逻辑审查）
- [ ] Hardware（已烧录；BLE 实机写→重启持久化未确认）
- [x] Commit

修改文件（仅 `main/transport/ble.c`；未改 BSP、未改 ui 层、未动其它模块）：

- `main/transport/ble.c`：GATT 服务从 4 个特性扩到 7 个(name/top/title/status/**bio/website/github**)。
- 新增 UUID：BIO 0xFFE5、WEBSITE 0xFFE6、GITHUB 0xFFE7。
- 写缓冲 `buf[40] → buf[64]`,容纳最长字段(bio/website/github 各 48 字节)。

明细：

- 新特性走同一 `on_char_access`,arg 对应 badge_field_t;写入经 badge_update_text → badge_data_set 持久化 NVS + badge_ui_set_field。
- 持久化链路沿用既有 NVS 机制:badge_data_init 开机载入,写后 nvs_commit;重启仍在(继承自 name/top/title/status 已验证路径)。
- 读路径返回内部静态缓冲,安卓可读。

UI 验证：N/A(无 UI 改动)。

功能验证（纯逻辑审查）：

- [x] 7 个特性全部 READ|WRITE,arg 一一对应 badge_field_t
- [x] 写缓冲 64 覆盖 48 字节最长字段,无截断
- [x] NVS 持久化链路完整(badge_data_set 写+commit,init 读)
- [ ] 实机 APP→BLE→ESP32→NVS→Restart→数据仍在 全链路（未实机确认,需手机 + 重启）
- [ ] 安卓端新增字段读写（TASK-11 Android Preview 实现）

编译：

```text
idf.py build
结果：PASS
（ble.c 编译通过;.bin=0x2e5540,App 分区剩 28%）
```

新增警告：无。

Git Commit：

```text
292138a
```

问题：

- 安卓端目前只暴露 name/top/title/status;bio/website/github 的手机侧界面在 **TASK-11(Android Preview)** 添加。
- 实机"写→重启→仍在"链路需真机 + 手机 App 验证,当前未执行。

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

- [x] Code
- [ ] Build（**NOT TESTED** —— 本机无 Android SDK / Java,未编译）
- [x] UI（静态审查）
- [x] Function（静态审查）
- [ ] Hardware（未实机）
- [x] Commit

要求：

Android App 显示：

```text
240 × 320 Passport Preview
```

修改个人信息时实时更新预览。

### TASK-11 验证记录

状态：

- [x] Code（Kotlin 改动完成）
- [ ] Build（**NOT TESTED**:本机无 Android SDK(路径不存在)且无 Java,无法 gradle 编译）
- [x] UI（静态代码审查）
- [x] Function（静态代码审查）
- [ ] Hardware（未实机,需手机装 APK 测试）
- [x] Commit

修改文件：

- `android_app/app/src/main/java/com/folotoy/badge/MainActivity.kt`

明细：

- 新增资料字段 UUID：CHR_BIO 0xFFE5、CHR_WEBSITE 0xFFE6、CHR_GITHUB 0xFFE7（与固件 ble.c 一致）。
- 表单新增「简介 / 网站 / GitHub」三个输入框。
- **240×320 Passport Preview**：`PassportPreviewView`(inner View,onMeasure 保持 4:3 比例)绘制仿 HOME 页预览(顶部文字/电量占位/头像占位/姓名/职位/状态/8 点指示器)。
- 7 个输入框挂 `TextWatcher` → `invalidate()`,编辑即实时刷新预览;读取回填也会触发刷新。
- read/write 队列加入新字段,onCharacteristicRead 回填新增字段。

UI 验证（静态审查）：

- [x] PassportPreviewView 按 4:3 比例缩放(240:320),onMeasure 保证
- [x] 预览绘制坐标参照 240px 参考宽度,等比例放大
- [x] 7 字段 TextWatcher 刷新预览
- [ ] 实机预览渲染效果（未实机）

功能验证（静态审查）：

- [x] 新字段 UUID 与固件一致(0xFFE5-0xFFE7)
- [x] read/write 队列与回填逻辑全覆盖新字段
- [ ] 实机 BLE 连接读写(含新字段)（未实机）

编译：

```text
本机无 Android SDK / Java,未执行 gradle 编译。
Build: NOT TESTED
```

Git Commit：

```text
893e6b9
```

问题：

- 需在有 Android SDK + JDK 的环境跑 `gradlew assembleDebug` 编译,并在真机安装验证预览与 BLE 新字段读写。
- 预览中电量为静态占位"86%",未接真实电量;头像为灰块占位(真头像流程在 TASK-12)。

## 当前开发备注（非任务清单）

### 临时 BLE 覆盖（开发/联调）
- **现象**：开机后 BLE 未广播、手机无法连接。根因：NVS `ble_on=0`（旧固件设置页关闭过），而 ble_init 会读取该值关闭广播。
- **处理（临时）**：`main/transport/ble.c` ble_init 暂时强制 `s_ble_enabled = true`（开机即广播），便于手机连接联调。
- **⚠ 最终 V2 行为**：BLE 默认关 + 设置页开关（TASK-13），同步完即关并深睡。届时恢复读取 NVS `ble_on` 并配合 TASK-13 的开关 UI。
- 验证：boot 日志 `NimBLE: GAP procedure initiated: advertise` + `BLE 已初始化并开始广播` 已确认。

---

# TASK-12：Avatar

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

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

### TASK-12 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（主机纯逻辑自检）
- [ ] Hardware（已烧录；头像上传/显示实机未确认）
- [x] Commit

修改文件：

- `partitions.csv`：新增 `storage`(spiffs,0x410000,2MB) 分区
- 新增 `main/avatar/avatar_storage.h/.c`（SPIFFS 挂载 + `/avatar.bin` save/load/has）
- `main/transport/ble.c`：新增头像上传 GATT（AV_CTRL 0xFFE8 / AV_DATA 0xFFE9，分块 + CRC32 校验）
- `main/apps/home/home.c`：优先渲染 `/avatar.bin`(80x80 RGB565)，无则回退内置素材
- `main/app/app_router.c`：init 调 avatar_storage_init
- `main/CMakeLists.txt`：加入 avatar_storage.c + `spiffs` REQUIRES
- 新增 `tests/avatar_transfer_check.py`

明细：

- **存储**：SPIFFS 挂到 `/spiffs`，头像文件 `/avatar.bin`(80x80 RGB565=12,800B)；首次/损坏自动格式化。
- **BLE 上传协议**：写 `AV_CTRL "START <size> <crc32>"` 开始分配缓冲；`AV_DATA` 分块写入；收满后 `esp_crc32_le` 校验，匹配则 `avatar_storage_save`；`CANCEL` 中止。缓冲上限 64KB 防滥用。
- **显示**：HOME 页有 `/avatar.bin` 则渲染 80x80 用户头像，否则回退内置 80x157 素材缩放。
- 头像的裁剪/缩放/RGB565 全部在 Android 端完成（TASK-11 预览侧），ESP32 只接收-校验-保存-刷新。

UI 验证（静态坐标审查）：

- [x] 用户头像 80x80 置于 HOME_AVATAR_Y=56(56..136)，不触姓名(178)
- [x] 无头像时回退内置素材
- [ ] 实机头像显示效果（未实机确认）

功能验证（主机自检 `tests/avatar_transfer_check.py`）：

- [x] 12,800B 分块装配完整一致
- [x] CRC 匹配(esp_crc32_le==zlib.crc32)、损坏字节不匹配
- [x] 超长块 clamp、空块忽略、@244B 需 53 块
- [ ] 实机 BLE 上传→CRC→Flash→显示 全链路（未实机确认,需手机 App 实现头像裁剪/上传）

编译：

```text
idf.py build
结果：PASS
（avatar_storage.c/ble.c/home.c 编译通过;分区表含 spiffs;.bin=0x2f17f0,App 分区剩 26%）
```

Git Commit：

```text
0313900
```

问题：

- 实机上传全链路需手机 App 实现头像裁剪→RGB565→分块上传(Android 端已在 `MainActivity.kt` 实现;**真机发现根因:未协商 ATT MTU 使 244B 分块走长写被 NimBLE 拒(rc=6 Insufficient Resources),数据到不了固件。修复:App 连接后 `requestMtu(517)` + 按协商 MTU 算分块,协商失败回退 20B。**)待编译 APK 实机验证。
- 上传缓冲 malloc 12,800B(无 PSRAM)在联调时瞬时占用;已限上限并传输结束即 free。
- 分区表新增 storage 分区,需与固件一起烧录(partition-table.bin 已包含)。

---

# TASK-13：SETTINGS

状态：

- [x] Code
- [x] Build
- [x] UI
- [x] Function
- [ ] Hardware
- [x] Commit

### TASK-13 验证记录

状态：

- [x] Code
- [x] Build
- [x] UI（静态坐标审查）
- [x] Function（主机纯逻辑自检）
- [ ] Hardware（已烧录；SETTINGS 页与低功耗链路实机未确认）
- [x] Commit

修改文件：

- 新增 `main/apps/settings/settings_page.h/.c`（V2 SETTINGS 页:BLE 开关 / SLEEP 超时 / VERSION）
- `main/transport/ble.c`：恢复 ble_init 读取 NVS ble_on(默认开);SETTINGS 页开关经 ble_stop/ble_restart 持久化
- `main/app/app_router.c`：SETTINGS 槽位用 settings_page_enter/exit/key
- `main/CMakeLists.txt`：加入 apps/settings
- 新增 `tests/settings_logic_check.py`

明细：

- SETTINGS 列表：**BLE**(OK 开关,经 ble_stop/ble_restart 写 NVS)、**SLEEP**(OK 循环 30s/1m/2m/5m/never,经 badge_power_set_timeout)、**VERSION**(只读 v2.0.0)。
- UP/DOWN 选择(高亮同 TOOLS),OK 开关/循环;长按全局回 HOME。
- **低功耗**：BLE 默认开、可在 SETTINGS 关闭并持久化(NVS ble_on);重启后读回。深睡/唤醒/显示关闭复用既有 badge_power。

UI 验证（静态坐标审查）：

- [x] 3 行列表 y56..152(步进 48,行高 44) —— 末行 196 <271,不触 Footer
- [x] 右值显示(BLE ON/OFF、SLEEP 标签、版本)
- [ ] 实机 SETTINGS 显示与操作（未实机确认）

功能验证（主机自检 `tests/settings_logic_check.py`）：

- [x] find_sleep_idx 匹配/未知回退、SLEEP 循环(含 wrap 到 30s)
- [x] 列表 UP/DOWN 选择环绕(3 项)
- [x] BLE 开关经 ble_stop/ble_restart 持久化 NVS 逻辑
- [ ] 实机 BLE 开关切换、休眠超时切换（未实机确认）

编译：

```text
idf.py build
结果：PASS
（settings_page.c 编译通过;.bin=0x2f1d90,App 分区剩 26%）
```

Git Commit：

```text
f00dc35
```

问题：

- 恢复 NVS 读后,你板子上旧的 `ble_on=0` 会让 BLE 开机为关;请在 SETTINGS 页把它打开(会持久化)。
- 低功耗全链路(BLE OFF/Wi-Fi OFF/Display OFF/Deep Sleep/Wake/UI Restore)大多复用 badge_power 既有实现,真机待验证。

---

# TASK-13：低功耗

状态：

- [x] Code（复用既有 badge_power 深睡/唤醒;BLE 关闭经 SETTINGS 开关持久化）
- [x] Build
- [x] UI（N/A,无新 UI 除 SETTINGS 开关）
- [x] Function
- [ ] Hardware（未实机验证深睡→唤醒→UI 恢复链路）
- [x] Commit

验证：

```text
BLE OFF      ← SETTINGS 页关闭并持久化
Wi-Fi OFF    ← 未启用
Display OFF  ← 深睡前关闭(既有 badge_power)
Deep Sleep   ← 无操作超时(既有 badge_power)
Wake         ← GPIO0 低电平唤醒(既有)
UI Restore   ← 唤醒重建当前页
```

记录：

- BLE 关闭：SETTINGS 页 BLE 项 OK 关闭 → ble_stop() 写 NVS `ble_on=0`,重启读回关闭。
- Wi-Fi OFF：固件未初始化 Wi-Fi,恒关。
- Display OFF / Deep Sleep / Wake：`badge_power`(TASK 基线)已实现——深睡前关背光 + LCD DISP_OFF + 提交 NVS,GPIO0 低电平唤醒,开机 800ms 忽略按键。
- UI Restore：唤醒即冷启动重建,Router 渲染 HOME。
- **Hardware：NOT TESTED** —— 深睡功耗与唤醒链路需真机实测(当前未测量)。

---

# 31. 最终 UI 验收

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
