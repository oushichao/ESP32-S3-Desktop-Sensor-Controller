#pragma once

#define LCD_SPI_HOST        SPI2_HOST           // SPI2
#define LCD_SPI_CLK         GPIO_NUM_12         // SCLK
#define LCD_SPI_MOSI        GPIO_NUM_11         // MOSI (SDA)
#define LCD_SPI_MISO        GPIO_NUM_NC         // MISO
#define LCD_SPI_CS          GPIO_NUM_10         // CS
#define LCD_SPI_DC          GPIO_NUM_9          // 数据/命令切换
#define LCD_SPI_RST         GPIO_NUM_46         // 硬件复位
#define LCD_BL              GPIO_NUM_45         // 背光，

/* ===== 分辨率 ===== */
#define LCD_HOR_RES         240
#define LCD_VER_RES         320

void ST7789_Init();