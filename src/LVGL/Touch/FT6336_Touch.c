#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#include "FT6336_Touch.h"
#include "Device/LCD/LCD_Init.h"

static const char *TAG = "FT6336";

#define FT6336_REG_GESTURE      0x01
#define FT6336_REG_TD_STATUS    0x02
#define FT6336_REG_P1_XH        0x03
#define FT6336_REG_P1_XL        0x04
#define FT6336_REG_P1_YH        0x05
#define FT6336_REG_P1_YL        0x06

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t i2c_dev_handle = NULL;

void FT6336_Touch_Init(void)
{
    /* 1. 创建 I²C 主机总线 */
    i2c_master_bus_config_t bus_cfg = {
        .scl_io_num        = TOUCH_I2C_SCL,
        .sda_io_num        = TOUCH_I2C_SDA,
        .i2c_port          = TOUCH_I2C_PORT,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &i2c_bus_handle));

    /* 2. 添加 FT6336 从机 */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = FT6336_ADDR,
        .scl_speed_hz    = 400000,   /* FT6336 支持 400 kHz */
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &i2c_dev_handle));

    /* 3. INT 引脚配置为输入（方案 A：不再用中断，仅保留管脚配置，
          以便将来如需低功耗唤醒时使用）*/
    gpio_config_t int_cfg = {
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << TOUCH_INT),
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&int_cfg);

    ESP_LOGI(TAG, "Touch init done (polling mode, I2C @400kHz)");
}

void FT6336_Touch_Read(uint16_t *x, uint16_t *y, bool *pressed)
{
    *pressed = false;

    if (!i2c_dev_handle) {
        return;
    }

    /* 1. 读触摸点数寄存器 TD_STATUS */
    uint8_t  status = 0;
    uint8_t  reg_st = FT6336_REG_TD_STATUS;
    esp_err_t err = i2c_master_transmit_receive(
        i2c_dev_handle, &reg_st, 1, &status, 1, 100);
    if (err != ESP_OK) {
        return;  /* I²C 失败，视为未按下，避免幽灵触摸 */
    }

    if ((status & 0x0F) == 0) {
        return;  /* 没有手指按下 */
    }

    /* 2. 连续读 4 个坐标字节：XH, XL, YH, YL */
    uint8_t reg  = FT6336_REG_P1_XH;
    uint8_t data[4] = {0};
    err = i2c_master_transmit_receive(
        i2c_dev_handle, &reg, 1, data, 4, 100);
    if (err != ESP_OK) {
        return;
    }

    uint16_t x_raw = ((uint16_t)(data[0] & 0x0F) << 8) | data[1];
    uint16_t y_raw = ((uint16_t)(data[2] & 0x0F) << 8) | data[3];

    if (x_raw >= LCD_HOR_RES) x_raw = LCD_HOR_RES - 1;
    if (y_raw >= LCD_VER_RES) y_raw = LCD_VER_RES - 1;

    *x = x_raw;
    *y = y_raw;
    *pressed = true;
}
