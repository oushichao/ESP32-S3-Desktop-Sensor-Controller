#pragma once
#include "lvgl.h"
#include "esp_lcd_panel_ops.h"

#define LCD_SPI_HOST        SPI2_HOST
#define LCD_SPI_MISO        GPIO_NUM_NC
#define LCD_BL              GPIO_NUM_8
#define LCD_SPI_CLK         GPIO_NUM_15
#define LCD_SPI_MOSI        GPIO_NUM_7
#define LCD_SPI_DC          GPIO_NUM_6
#define LCD_SPI_RST         GPIO_NUM_5
#define LCD_SPI_CS          GPIO_NUM_4

// LCD 序号	功能	接 ESP32 GPIO
// 1	VCC（电源正）	3.3V 或 5V
// 2	GND（电源地）	GND
// 3	CS（片选）	GPIO 4
// 4	RST（复位）	GPIO 5
// 5	DC（数据/命令）	GPIO 6
// 6	MOSI	GPIO 7
// 7	SCLK	GPIO 15
// 8	BL（背光）	GPIO 8
// 9	MISO	不接（悬空）
// 10	TOUCH SCL	GPIO 9
// 11	TOUCH RST	不接 或接任意空闲 GPIO
// 12	TOUCH SDA	GPIO 10
// 13	TOUCH INT	GPIO 11
// 14	SD CS	不接（悬空）


#define LCD_HOR_RES         240
#define LCD_VER_RES         320

extern esp_lcd_panel_io_handle_t io_handle;
extern esp_lcd_panel_handle_t    panel_handle;

void LCD_ili9341_Init();
void Backlight_Set(uint8_t duty);
void LCD_Set_Color(uint16_t color);
void LCD_Fill_Rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void Lvgl_Start();
