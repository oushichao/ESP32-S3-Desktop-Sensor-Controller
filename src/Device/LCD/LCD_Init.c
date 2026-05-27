#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "LCD_Init.h"

static bool ledc_ok = false;
esp_lcd_panel_io_handle_t io_handle = NULL;
SemaphoreHandle_t color_done_sem = NULL;

static const uint8_t init_cmds[] = {
    0x01, 0, 5,           // SWRESET
    0x36, 1, 0, 0x48,     // MADCTL: MX=1 + BGR (旋转180°，修正显示方向)
    0x3A, 1, 0, 0x55,     // COLMOD: 16bit
    0x21, 0, 10,          // INVON: 反相显示
    0x11, 0, 120,         // SLPOUT
    0x29, 0, 20,          // DISPON
    0x00
};





volatile uint32_t cb_count = 0;

static bool color_trans_done(esp_lcd_panel_io_handle_t io,
                             esp_lcd_panel_io_event_data_t *edata,
                             void *user_ctx)
{
    cb_count++;
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(color_done_sem, &woken);
    return woken == pdTRUE;
}


static void ili9341_run_init(void)
{
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

static void backlight_pwm_init(void)
{
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

void ILI9341_Backlight_Set(uint8_t pct)
{
    if (!ledc_ok) return;
    if (pct > 100) pct = 100;
    uint32_t duty = (uint32_t)pct * 1023 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void ILI9341_Init(void)
{
    color_done_sem = xSemaphoreCreateBinary();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = LCD_SPI_MOSI,
        .miso_io_num = LCD_SPI_MISO,
        .sclk_io_num = LCD_SPI_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_HOR_RES * LCD_VER_RES * 2 + 10
    };
    spi_bus_initialize(LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = LCD_SPI_CS,
        .dc_gpio_num = LCD_SPI_DC,
        .pclk_hz = 10 * 1000 * 1000,          
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .on_color_trans_done = color_trans_done,  // DMA 完成回调
    };
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_cfg, &io_handle);

    if (LCD_SPI_RST >= 0) {
        gpio_set_direction(LCD_SPI_RST, GPIO_MODE_OUTPUT);
        gpio_set_level(LCD_SPI_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(LCD_SPI_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    ili9341_run_init();
    backlight_pwm_init();
    ILI9341_Backlight_Set(80);
}
