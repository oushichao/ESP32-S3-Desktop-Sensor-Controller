#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_ft5x06.h"

#include "FT6336_Touch.h"
#include "Device/LCD/LCD_Init.h"

static const char *TAG = "FT6336";

/**
 * @brief   FT6336的LCD触摸驱动
 * @param   触摸句柄
 */
esp_err_t LCD_FT6336_Touch_Init(esp_lcd_touch_handle_t* ret_touch){
    //I2C硬件总线驱动
    i2c_master_bus_config_t bus_config={
        .scl_io_num     =   TOUCH_I2C_SCL,
        .sda_io_num     =   TOUCH_I2C_SDA,
        .i2c_port       =   TOUCH_I2C_PORT,
        .clk_source     =   I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt  =7,
        .flags.enable_internal_pullup = true,   
    };
    i2c_master_bus_handle_t bus_handle;
    i2c_new_master_bus(&bus_config,&bus_handle);

    //触摸初始化
    esp_lcd_touch_config_t th_cfg={
        .x_max          =   LCD_HOR_RES,
        .y_max          =   LCD_VER_RES,
        .rst_gpio_num   =   GPIO_NUM_NC,
        .int_gpio_num   =   TOUCH_INT,
        .levels={
            .interrupt  =   false,
            .reset      =   false,
        },
        .flags={
            .mirror_x   =   false,
            .mirror_y   =   false,
            .swap_xy    =   false
        },
    };

    esp_lcd_panel_io_handle_t  tp_io_handle=NULL;
    //一键填充I2C的配置
    esp_lcd_panel_io_i2c_config_t   tp_io_config=ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    //创建传输层
    esp_lcd_new_panel_io_i2c(bus_handle,&tp_io_config,&tp_io_handle);
    //传入触摸驱动
    esp_err_t ret = esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &th_cfg, ret_touch);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FT6336 驱动失败: %s", esp_err_to_name(ret));
        return ret;                           
    }
    ESP_LOGI(TAG,"FT6336驱动完成!!!");
    return ESP_OK;
}

