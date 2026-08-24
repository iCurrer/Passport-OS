// main/apps/cards/cards.c —— Passport OS V2 第 3 页 CARDS / QR(个人链接)页。
//
// 布局(参考 docs/UI_DESIGN_SPEC.md §9):
//   Header(0-35)      :ds_header("CARDS" + 电量)
//   Content(36-271)   :二维码(约 ≤116px,居中) → 副标题("SCAN ME")
//   Footer(272-319)   :Page Indicator(CARDS=第 3 点实心)
//
// 二维码内容来自 NVS 的 QR 字段(用户自定义,微信/网址等),仅用于生成二维码,
// 页面不展示二维码内容/任何链接文本。
// 用 espressif/qrcode 动态生成;无 PSRAM:渲染成单个 RGB565 bitmap(≤约27KB),
// 仅在 CARDS 页驻留时占用,退出即释放(见 cards_exit)。
#include "cards.h"
#include "badge_data.h"
#include "badge_fonts.h"
#include "bsp_battery.h"
#include "ds_tokens.h"
#include "ds_widgets.h"
#include "ui_pixel.h"
#include "qrcode.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "cards";

// 二维码渲染尺寸上限(px),控制无 PSRAM 下位图缓冲 ≤ 约27KB。
#define QR_MAX_PX        116
#define QR_MIN_MODULE_PX 3      // 每模块最小像素(保持可扫描)
#define QR_MODULE_PX     4      // 常用每模块像素
#define QR_Y             48     // 二维码顶部 y(Content 36-271)
#define QR_CAPTION_Y     200    // 副标题 y

static lv_obj_t        *s_scr;
static ds_dots_t        s_dots;
static uint8_t         *s_qr_buf;   // RGB565 bitmap
static lv_image_dsc_t   s_qr_dsc;   // LVGL image 描述符(静态,需驻留)
static lv_obj_t        *s_qr_img;

// 二维码生成回调(esp_qrcode_generate 同步调用):把模块矩阵画进 s_qr_buf 并建 lv_image。
static void qr_display_cb(esp_qrcode_handle_t qrcode, void *user_data)
{
    (void)user_data;
    if (!s_scr) return;

    int size = esp_qrcode_get_size(qrcode);          // 模块边长(21..177)
    if (size <= 0) return;

    // 在 RAM 预算内尽量放大:px = size * module_px ≤ QR_MAX_PX
    int module_px = QR_MAX_PX / size;
    if (module_px < QR_MIN_MODULE_PX) module_px = QR_MIN_MODULE_PX;
    int px = size * module_px;
    uint32_t n = (uint32_t)px * (uint32_t)px * 2;    // RGB565

    s_qr_buf = malloc(n);
    if (!s_qr_buf) {
        ESP_LOGE(TAG, "QR 位图分配失败(%u B)", (unsigned)n);
        return;
    }

    uint16_t *p = (uint16_t *)s_qr_buf;
    for (int y = 0; y < px; y++) {
        int my = y / module_px;
        for (int x = 0; x < px; x++) {
            int mx = x / module_px;
            bool black = esp_qrcode_get_module(qrcode, mx, my);
            *p++ = black ? 0x0000 : 0xFFFF;          // 黑模块 / 白背景
        }
    }

    memset(&s_qr_dsc, 0, sizeof(s_qr_dsc));
    s_qr_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_qr_dsc.header.cf    = LV_COLOR_FORMAT_RGB565;
    s_qr_dsc.header.w     = px;
    s_qr_dsc.header.h     = px;
    s_qr_dsc.data_size    = n;
    s_qr_dsc.data         = s_qr_buf;

    s_qr_img = lv_image_create(s_scr);
    lv_image_set_src(s_qr_img, &s_qr_dsc);
    lv_obj_align(s_qr_img, LV_ALIGN_TOP_MID, 0, QR_Y);
    ESP_LOGI(TAG, "QR %d x %d (module_px=%d, buf=%u B)", size, size, module_px, (unsigned)n);
}

// 二维码内容:来自用户自定义的 QR 字段(微信/网址等)。只用于生成二维码,不展示文本。
static const char *qr_content(void)
{
    const char *qr = badge_data_get(BADGE_FIELD_QR);
    if (qr && qr[0]) return qr;
    return NULL;
}

void cards_enter(app_page_t page)
{
    (void)page;

    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    ds_header(s_scr, "CARDS", bsp_battery_soc(),
              &badge_font_gb2312_small, &lv_font_montserrat_14);

    const char *content = qr_content();
    if (content) {
        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
        cfg.display_func_with_cb = qr_display_cb;
        cfg.user_data = NULL;
        if (esp_qrcode_generate(&cfg, content) != ESP_OK) {
            ESP_LOGE(TAG, "QR 生成失败");
            // 内容过长等:显示占位文本
            lv_obj_t *err = ui_label(s_scr, "QR TOO LONG", &badge_font_gb2312_small,
                                     DS_DANGER);
            lv_obj_align(err, LV_ALIGN_TOP_MID, 0, QR_Y + 40);
        }
    } else {
        lv_obj_t *no = ui_label(s_scr, "NO CARD DATA", &badge_font_gb2312_small,
                                DS_TEXT_SECONDARY);
        lv_obj_align(no, LV_ALIGN_TOP_MID, 0, QR_Y + 60);
    }

    // 副标题:固定提示,不展示二维码内容/链接原文
    lv_obj_t *cap_lbl = ui_label(s_scr, "SCAN ME", &badge_font_gb2312_small,
                                 DS_TEXT_SECONDARY);
    lv_obj_align(cap_lbl, LV_ALIGN_TOP_MID, 0, QR_CAPTION_Y);

    // Footer:Page Indicator(CARDS=第 3 点实心)
    ds_footer(s_scr, &s_dots, APP_PAGE_COUNT, APP_PAGE_CARDS);

    lv_screen_load(s_scr);
}

void cards_exit(void)
{
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    s_qr_img = NULL;
    if (s_qr_buf) { free(s_qr_buf); s_qr_buf = NULL; }
}