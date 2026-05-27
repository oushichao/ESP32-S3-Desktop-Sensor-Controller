#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

/* ========== 引脚与屏幕参数 ========== */
#define LCD_SPI_HOST        SPI2_HOST
#define LCD_SPI_MISO        GPIO_NUM_NC
#define LCD_BL              GPIO_NUM_8
#define LCD_SPI_CLK         GPIO_NUM_5
#define LCD_SPI_MOSI        GPIO_NUM_7
#define LCD_SPI_DC          GPIO_NUM_6
#define LCD_SPI_RST         GPIO_NUM_4
#define LCD_SPI_CS          GPIO_NUM_1
#define LCD_HOR_RES         240
#define LCD_VER_RES         320

/* ILI9341 内部 GRAM 偏移（部分 2.4 寸屏需要，默认 0） */
#define LCD_X_OFFSET        0
#define LCD_Y_OFFSET        0

/* RGB565 颜色 */
#define ILI9341_COLOR_BLACK       0x0000
#define ILI9341_COLOR_WHITE       0xFFFF
#define ILI9341_COLOR_RED         0xF800
#define ILI9341_COLOR_GREEN       0x07E0
#define ILI9341_COLOR_BLUE        0x001F
#define ILI9341_COLOR_YELLOW      0xFFE0
#define ILI9341_COLOR_CYAN        0x07FF
#define ILI9341_COLOR_MAGENTA     0xF81F

typedef enum {
    ILI9341_ROTATE_0   = 0,
    ILI9341_ROTATE_90  = 1,
    ILI9341_ROTATE_180 = 2,
    ILI9341_ROTATE_270 = 3,
} ili9341_rotate_t;

esp_err_t ili9341_init(ili9341_rotate_t rotate);
void ili9341_backlight(bool on);
esp_err_t ili9341_set_rotation(ili9341_rotate_t rotate);
esp_err_t ili9341_fill_screen(uint16_t color);
esp_err_t ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
esp_err_t ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
uint16_t ili9341_get_width(void);
uint16_t ili9341_get_height(void);

static inline uint16_t ili9341_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
