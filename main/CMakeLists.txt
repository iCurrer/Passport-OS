idf_component_register(
    SRCS "main.c"
         "badge.c"
         "ble.c"
         "badge_font_gb2312.c"
         "badge_font_gb2312_small.c"
         "badge_avatar.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer nvs_flash bt
)
