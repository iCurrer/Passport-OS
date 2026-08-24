idf_component_register(
    SRCS "src/ui_pixel.c"
         "src/ds_widgets.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl esp_lvgl_port
)