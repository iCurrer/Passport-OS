// main/badge/badge_theme.h —— 名牌应用共用主题常量与布局参数。
// 供 badge_ui / settings / 未来子页面共享,避免各页重复定义。
#pragma once

// 主题色(磨砂绿)
#define BADGE_BG_COLOR        0x1E352C   // 背景
#define BADGE_TXT_PRIMARY     0xF4F8F5   // 主文字(近白微绿)
#define BADGE_TXT_MUTED       0xAFC0B6   // 次要文字(灰绿)
#define BADGE_ACCENT          0x4CD964   // 主题强调色(状态圆点/dock 选中)
#define BADGE_LINE_DIV        0x2A473B   // 分隔线色(深绿)
#define BADGE_DOCK_BG         0x16241C   // 底部 dock 背景

// 电量条颜色
#define BADGE_BATT_NORMAL 0xF4F8F5
#define BADGE_BATT_WARN   0xF0A030      // 低电量(<20%)
#define BADGE_BATT_LOW    0xE04545      // 很低电量(<10%)

// 布局
#define BADGE_MARGIN_X     22
#define BADGE_HEAD_LINE    40           // 顶部分隔线 y
