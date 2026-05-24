#pragma once
#include "lvgl.h"

#define LCD_SPI_HOST        SPI2_HOST
#define LCD_SPI_CLK         GPIO_NUM_12
#define LCD_SPI_MOSI        GPIO_NUM_11
#define LCD_SPI_MISO        GPIO_NUM_NC
#define LCD_SPI_CS          GPIO_NUM_10
#define LCD_SPI_DC          GPIO_NUM_9
#define LCD_SPI_RST         GPIO_NUM_13
#define LCD_BL              GPIO_NUM_14

#define LCD_HOR_RES         240
#define LCD_VER_RES         320

void ILI9341_Init(void);
void ILI9341_Backlight_Set(uint8_t pct);
