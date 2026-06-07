#pragma once
#include "driver/gpio.h"

#define TOUCH_I2C_PORT      I2C_NUM_0
#define TOUCH_I2C_SCL       GPIO_NUM_9
#define TOUCH_I2C_SDA       GPIO_NUM_10
#define TOUCH_INT           GPIO_NUM_11

esp_err_t LCD_FT6336_Touch_Init(esp_lcd_touch_handle_t* ret_touch);
