#pragma once
#include "lvgl.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"

lv_display_t *lvgl_port_display_init(esp_lcd_panel_handle_t panel);

