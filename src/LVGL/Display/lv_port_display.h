#pragma once
#include "lvgl.h"
#include "driver/gpio.h"



/* ===== 分辨率 ===== */
#define LCD_HOR_RES         240
#define LCD_VER_RES         320

void lv_port_display_init(void);

