// components/ui/include/ds_tokens.h
//
// Passport OS V2 design-system tokens：配色 / 骨架间距 / 排版尺寸。
// 纯头文件，只定义常量，供各页面与 ds_* 原语复用 —— 是 V2 深色极简规范的单一来源。
// 迁移期与旧 ui_pixel 浅色风格并存、互不依赖；页面改造到对应 TASK 时以本文件为准。
// 对应规范见 docs/UI_DESIGN_SPEC.md §1。屏幕固定 240x320。
#pragma once

#include <stdint.h>

// ============================================================================
// 配色（V2 深色极简）
// ============================================================================
#define DS_BG              0x000000   // 背景（纯黑）
#define DS_TEXT_PRIMARY    0xFFFFFF   // 主文字
#define DS_TEXT_SECONDARY  0x8A8A8A   // 次要文字
#define DS_ACCENT          0x4CD964   // 强调（状态激活/选中/Page Indicator 当前点）
#define DS_WARN            0xFF9F0A   // Warning（低电量）
#define DS_DANGER          0xFF453A   // Danger（很低电量/错误/卸载）
#define DS_LINE            0x1F1F1F   // 分隔线 / 灰卡边线
#define DS_CARD            0x111111   // 卡片背景

// ============================================================================
// 骨架（Header 0–35 / Content 36–271 / Footer 272–319）
// ============================================================================
#define DS_SCREEN_W         240
#define DS_SCREEN_H         320
#define DS_MARGIN_X         16         // 左右安全边距
#define DS_HEADER_H         35         // Header 0–35
#define DS_CONTENT_Y        36         // Content 36–271
#define DS_CONTENT_H        236
#define DS_FOOTER_Y         272        // Footer 272–319
#define DS_FOOTER_H         48
#define DS_HEADER_LINE_Y    34         // Header 底部分隔线

// Header 内元素
#define DS_HDR_BRAND_X      16         // 左侧标题
#define DS_HDR_BRAND_Y      13
#define DS_HDR_BATT_X       158        // 电量外框
#define DS_HDR_BATT_Y       14
#define DS_HDR_BATT_W       20
#define DS_HDR_BATT_H       10
#define DS_HDR_BATT_CORE_X  160        // 电量内芯
#define DS_HDR_BATT_CORE_Y  16
#define DS_HDR_BATT_CORE_W  16
#define DS_HDR_BATT_CORE_H  6
#define DS_HDR_BATT_PCT_X   186        // 电量百分比文字
#define DS_HDR_BATT_PCT_Y   12

// ============================================================================
// Page Indicator（Footer，见 spec §4）
// ============================================================================
#define DS_DOT_SIZE         6          // 圆点直径
#define DS_DOT_GAP          12         // 圆点间距
#define DS_DOT_Y            293        // 垂直居中于 296（6px 圆点 → y=293）
#define DS_PAGES_MAX        8          // 页面最多 8 个（plan §4 / §5）

// ============================================================================
// 排版尺寸（像素字高；具体 lv_font 由调用方按页面传入）
// ============================================================================
#define DS_FONT_SIZE_TITLE  24         // 页面主标题/姓名（badge_font_gb2312, 24px）
#define DS_FONT_SIZE_BODY   14         // 正文/标签（badge_font_gb2312_small, 14px）
#define DS_FONT_SIZE_NUM    14         // 数字/百分比（lv_font_montserrat_14, 14px）