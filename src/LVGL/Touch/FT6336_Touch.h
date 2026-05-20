#pragma once
#include "lvgl.h"
#include "driver/gpio.h"

#define TOUCH_I2C_PORT      I2C_NUM_0
#define TOUCH_I2C_SDA       GPIO_NUM_0
#define TOUCH_I2C_SCL       GPIO_NUM_1
#define TOUCH_INT           GPIO_NUM_2
#define TOUCH_RST           GPIO_NUM_NC

#define FT6336_ADDR         0x38

void FT6336_Touch_Init();

void FT6336_Touch_Read();


