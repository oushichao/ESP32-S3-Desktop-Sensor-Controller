#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include <stdbool.h>

#include "LCD_Init.h"

static bool ledc_ok = false;
esp_lcd_panel_io_handle_t io_handle = NULL;

/* ILI9341 初始化序列 */
static const uint8_t init_cmds[] = {
    0x01, 0, 0x80,   // software reset
    0x28, 0, 0x80,   // display off
    0x3A, 1, 0x55,   // pixel format: 16bit
    0x36, 1, 0x08,   // MADCTL: BGR order
    0x11, 0, 0x80,   // sleep out
    0x29, 0, 0x80,   // display on
    0x00                // end
};

static void ili9341_send_cmd(uint8_t cmd){
    esp_lcd_panel_io_tx_param(io_handle, cmd, NULL, 0);
}

static void ili9341_send_data(const uint8_t *data, size_t len){
    esp_lcd_panel_io_tx_param(io_handle, 0, data, len);
}

static void ili9341_run_init(void){
    int i = 0;
    while (init_cmds[i]) {
        uint8_t cmd  = init_cmds[i++];
        uint8_t len  = init_cmds[i++];
        uint8_t wait = init_cmds[i++];
        if (len) {
            esp_lcd_panel_io_tx_param(io_handle, cmd, (void *)&init_cmds[i], len);
            i += len;
        } else {
            esp_lcd_panel_io_tx_param(io_handle, cmd, NULL, 0);
        }
        if (wait) vTaskDelay(pdMS_TO_TICKS(wait));
    }
}

static void backlight_pwm_init(void){
    ledc_timer_config_t ledc_timer = {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 1023,
        .gpio_num = LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
    ledc_ok = true;
}

void ILI9341_Backlight_Set(uint8_t pct){
    if (!ledc_ok) return;
    if (pct > 100) pct = 100;
    uint32_t duty = (uint32_t)pct * 1023 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void ILI9341_Init(void){
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = LCD_SPI_MOSI,
        .miso_io_num = LCD_SPI_MISO,
        .sclk_io_num = LCD_SPI_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_HOR_RES * LCD_VER_RES * 2 + 10
    };
    spi_bus_initialize(LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

    /* Panel IO 适配器（CS + DC 控制） */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = LCD_SPI_CS,
        .dc_gpio_num = LCD_SPI_DC,
        .pclk_hz = 20 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
    };
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_cfg, &io_handle);

    /* 硬件复位 */
    if (LCD_SPI_RST >= 0) {
        gpio_set_direction(LCD_SPI_RST, GPIO_MODE_OUTPUT);
        gpio_set_level(LCD_SPI_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(LCD_SPI_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    /* 发初始化序列 */
    ili9341_run_init();

    /* 背光 */
    backlight_pwm_init();
    ILI9341_Backlight_Set(80);

    /* 清屏为白色 */
    uint16_t *buf = malloc(LCD_HOR_RES * LCD_VER_RES * 2);
    if (buf) {
        for (int i = 0; i < LCD_HOR_RES * LCD_VER_RES; i++) buf[i] = 0xFFFF;

        uint8_t x_cmd[4] = {0, 0, 0, LCD_HOR_RES - 1};
        uint8_t y_cmd[4] = {0, 0, 0, LCD_VER_RES - 1};

        esp_lcd_panel_io_tx_param(io_handle, 0x2A, x_cmd, 4);
        esp_lcd_panel_io_tx_param(io_handle, 0x2B, y_cmd, 4);
        esp_lcd_panel_io_tx_param(io_handle, 0x2C, (void *)buf, LCD_HOR_RES * LCD_VER_RES * 2);
        free(buf);
    }
}
