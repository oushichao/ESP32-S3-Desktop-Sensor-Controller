#include "ili9341.h"

#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ili9341";

/* ILI9341 寄存器 */
#define ILI9341_SWRESET   0x01
#define ILI9341_SLPOUT    0x11
#define ILI9341_NORON     0x13
#define ILI9341_INVOFF    0x20
#define ILI9341_DISPON    0x29
#define ILI9341_CASET     0x2A
#define ILI9341_PASET     0x2B
#define ILI9341_RAMWR     0x2C
#define ILI9341_MADCTL    0x36
#define ILI9341_PIXFMT    0x3A

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_BGR 0x08

static spi_device_handle_t s_spi;
static uint16_t s_width = LCD_HOR_RES;
static uint16_t s_height = LCD_VER_RES;

/* ========== GPIO / SPI 底层 ========== */

static void cs_low(void)
{
    gpio_set_level(LCD_SPI_CS, 0);
}

static void cs_high(void)
{
    gpio_set_level(LCD_SPI_CS, 1);
}

static esp_err_t spi_send(const uint8_t *data, size_t len)
{
    if (len == 0) {
        return ESP_OK;
    }
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    return spi_device_polling_transmit(s_spi, &t);
}

/** 发送单字节命令（DC=0） */
static esp_err_t write_command(uint8_t cmd)
{
    cs_low();
    gpio_set_level(LCD_SPI_DC, 0);
    esp_err_t ret = spi_send(&cmd, 1);
    cs_high();
    return ret;
}

/** 发送命令 + 参数（同一 CS 周期内完成，ILI9341 必须这样） */
static esp_err_t write_cmd_data(uint8_t cmd, const uint8_t *data, size_t len)
{
    cs_low();
    gpio_set_level(LCD_SPI_DC, 0);
    esp_err_t ret = spi_send(&cmd, 1);
    if (ret == ESP_OK && data && len > 0) {
        gpio_set_level(LCD_SPI_DC, 1);
        ret = spi_send(data, len);
    }
    cs_high();
    return ret;
}

/** 设置窗口后连续写像素（RAMWR 与像素数据 CS 不断开） */
static esp_err_t write_pixels_continuous(const uint8_t *data, size_t len)
{
    cs_low();
    gpio_set_level(LCD_SPI_DC, 0);
    uint8_t cmd = ILI9341_RAMWR;
    esp_err_t ret = spi_send(&cmd, 1);
    if (ret == ESP_OK) {
        gpio_set_level(LCD_SPI_DC, 1);
        ret = spi_send(data, len);
    }
    cs_high();
    return ret;
}

static esp_err_t set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 += LCD_X_OFFSET;
    x1 += LCD_X_OFFSET;
    y0 += LCD_Y_OFFSET;
    y1 += LCD_Y_OFFSET;

    uint8_t col[4] = {
        (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
        (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF),
    };
    uint8_t row[4] = {
        (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
        (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF),
    };

    esp_err_t ret = write_cmd_data(ILI9341_CASET, col, 4);
    if (ret != ESP_OK) {
        return ret;
    }
    return write_cmd_data(ILI9341_PASET, row, 4);
}

static void hardware_reset(void)
{
    gpio_set_level(LCD_SPI_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_SPI_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static uint8_t madctl_value(ili9341_rotate_t rotate)
{
    /* 常见 2.4 寸 ILI9341 竖屏用 0x48 (MX|BGR)，横屏用 0x28 (MV|BGR) */
    switch (rotate) {
    case ILI9341_ROTATE_90:
        return MADCTL_MV | MADCTL_BGR;
    case ILI9341_ROTATE_180:
        return MADCTL_MY | MADCTL_BGR;
    case ILI9341_ROTATE_270:
        return MADCTL_MV | MADCTL_MX | MADCTL_MY | MADCTL_BGR;
    case ILI9341_ROTATE_0:
    default:
        return MADCTL_MX | MADCTL_BGR;
    }
}

static esp_err_t update_rotation(ili9341_rotate_t rotate)
{
    uint8_t mad = madctl_value(rotate);
    esp_err_t ret = write_cmd_data(ILI9341_MADCTL, &mad, 1);
    if (ret != ESP_OK) {
        return ret;
    }

    if (rotate == ILI9341_ROTATE_90 || rotate == ILI9341_ROTATE_270) {
        s_width = LCD_VER_RES;
        s_height = LCD_HOR_RES;
    } else {
        s_width = LCD_HOR_RES;
        s_height = LCD_VER_RES;
    }
    return ESP_OK;
}

/** ILI9341 标准初始化序列（Adafruit / ESP-IDF 通用版） */
static esp_err_t panel_init(ili9341_rotate_t rotate)
{
    esp_err_t ret;

    hardware_reset();

    ret = write_command(ILI9341_SWRESET);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(150));

    /* 关闭显示反转 */
    ret = write_command(ILI9341_INVOFF);
    if (ret != ESP_OK) return ret;

    /* 像素格式 16bit RGB565 */
    const uint8_t pixfmt = 0x55;
    ret = write_cmd_data(ILI9341_PIXFMT, &pixfmt, 1);
    if (ret != ESP_OK) return ret;

    ret = update_rotation(rotate);
    if (ret != ESP_OK) return ret;

    /* Power Control */
    const uint8_t pwctrl1[] = { 0x00, 0x15, 0x80 };
    ret = write_cmd_data(0xC0, pwctrl1, sizeof(pwctrl1));
    if (ret != ESP_OK) return ret;

    const uint8_t pwctrl2[] = { 0x00 };
    ret = write_cmd_data(0xC1, pwctrl2, sizeof(pwctrl2));
    if (ret != ESP_OK) return ret;

    const uint8_t vcom = 0x7F;
    ret = write_cmd_data(0xC5, &vcom, 1);
    if (ret != ESP_OK) return ret;

    /* Frame Rate */
    const uint8_t frctrl[] = { 0x00, 0x1A };
    ret = write_cmd_data(0xB1, frctrl, sizeof(frctrl));
    if (ret != ESP_OK) return ret;

    /* Gamma 校正（各 15 字节） */
    const uint8_t gamma_p[] = {
        0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20, 0x22,
        0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02
    };
    ret = write_cmd_data(0xE0, gamma_p, sizeof(gamma_p));
    if (ret != ESP_OK) return ret;

    const uint8_t gamma_n[] = {
        0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29, 0x2E,
        0x30, 0x3E, 0x3F, 0x3D, 0x07, 0x0B, 0x06
    };
    ret = write_cmd_data(0xE1, gamma_n, sizeof(gamma_n));
    if (ret != ESP_OK) return ret;

    /* 退出睡眠 */
    ret = write_command(ILI9341_SLPOUT);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(120));

    /* 正常显示模式 */
    ret = write_command(ILI9341_NORON);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(10));

    /* 开显示 */
    ret = write_command(ILI9341_DISPON);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(20));

    return ESP_OK;
}

/* ========== 公开 API ========== */

esp_err_t ili9341_init(ili9341_rotate_t rotate)
{
    esp_err_t ret;

    /* DC / RST / CS / BL 全部手动控制 */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LCD_SPI_DC) | (1ULL << LCD_SPI_RST)
                      | (1ULL << LCD_SPI_CS) | (1ULL << LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio config failed");

    gpio_set_level(LCD_SPI_DC, 1);
    gpio_set_level(LCD_SPI_RST, 1);
    cs_high();
    ili9341_backlight(false);

    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_SPI_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_HOR_RES * LCD_VER_RES * 2 + 8,
    };
    ret = spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize: %s", esp_err_to_name(ret));
        return ret;
    }

    /* CS 由软件控制，不用硬件 CS */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 26 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(LCD_SPI_HOST, &devcfg, &s_spi), TAG, "add device failed");

    ESP_RETURN_ON_ERROR(panel_init(rotate), TAG, "panel init failed");

    /* 初始化后立即刷一帧白色，验证 GRAM 写入 */
    ili9341_backlight(true);
    ret = ili9341_fill_screen(ILI9341_COLOR_BLACK);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "first fill failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "init ok %ux%u", s_width, s_height);
    return ESP_OK;
}

void ili9341_backlight(bool on)
{
    gpio_set_level(LCD_BL, on ? 1 : 0);
}

esp_err_t ili9341_set_rotation(ili9341_rotate_t rotate)
{
    return update_rotation(rotate);
}

uint16_t ili9341_get_width(void)
{
    return s_width;
}

uint16_t ili9341_get_height(void)
{
    return s_height;
}

esp_err_t ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0 || x >= s_width || y >= s_height) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t x1 = x + w - 1;
    uint16_t y1 = y + h - 1;
    if (x1 >= s_width)  x1 = s_width - 1;
    if (y1 >= s_height) y1 = s_height - 1;

    esp_err_t ret = set_addr_window(x, y, x1, y1);
    if (ret != ESP_OK) {
        return ret;
    }

    uint32_t pixel_count = (uint32_t)(x1 - x + 1) * (y1 - y + 1);
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);

    /* 512 字节缓冲 = 256 像素 */
    uint8_t chunk[512];
    for (size_t i = 0; i < sizeof(chunk); i += 2) {
        chunk[i] = hi;
        chunk[i + 1] = lo;
    }

    while (pixel_count > 0) {
        size_t batch = pixel_count;
        if (batch > sizeof(chunk) / 2) {
            batch = sizeof(chunk) / 2;
        }
        size_t byte_len = batch * 2;

        ret = write_pixels_continuous(chunk, byte_len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "write pixels failed: %s", esp_err_to_name(ret));
            return ret;
        }
        pixel_count -= batch;
    }

    return ESP_OK;
}

esp_err_t ili9341_fill_screen(uint16_t color)
{
    return ili9341_fill_rect(0, 0, s_width, s_height, color);
}

esp_err_t ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    return ili9341_fill_rect(x, y, 1, 1, color);
}
