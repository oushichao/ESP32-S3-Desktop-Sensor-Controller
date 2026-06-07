#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "MY_I2C.h"
#include "driver/i2c_master.h"

static i2c_master_bus_handle_t bus_handle=NULL;

void I2C_Init(gpio_num_t SDA,gpio_num_t SCL,uint8_t address,i2c_master_dev_handle_t* dev_handle){
    if(bus_handle==NULL){
        i2c_master_bus_config_t bus_config={        //总线配置
            .clk_source=I2C_CLK_SRC_DEFAULT,        //时钟源，系统默认
            .sda_io_num=SDA,                        
            .scl_io_num=SCL,
            .glitch_ignore_cnt=7,                   //干扰毛刺过滤，7个时钟周期的杂波过滤
            .flags.enable_internal_pullup=true,     //内部上拉使能
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config,&bus_handle));
    }

    //添加设备
    i2c_device_config_t dev_config={            //从设备配置
        .dev_addr_length=I2C_ADDR_BIT_LEN_7,    //地址位宽
        .device_address=address,                //设备i2c地址
        .scl_speed_hz=10*100*100,                   //通信速度
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle,&dev_config,dev_handle));
}

esp_err_t I2C_Write_Word(uint8_t reg_addr,uint16_t data,i2c_master_dev_handle_t dev_handle){
    if(dev_handle == NULL) return ESP_ERR_INVALID_STATE;
    uint8_t write_buf[3]={
        reg_addr,
        (data>>8)&0xFF,
        data&0xFF,
    };
    return i2c_master_transmit(dev_handle,write_buf,3,pdMS_TO_TICKS(100));
}

esp_err_t I2C_Read(uint8_t reg_addr,uint8_t* read_buf,uint8_t read_size,i2c_master_dev_handle_t dev_handle){
    if(read_buf==NULL||read_size==0)return ESP_ERR_INVALID_ARG;
    if(dev_handle == NULL) return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit_receive(dev_handle,&reg_addr,1,read_buf,read_size,pdMS_TO_TICKS(100));
}

esp_err_t I2C_Read_Word(uint8_t reg_addr, uint16_t *data,i2c_master_dev_handle_t dev_handle){
    if(dev_handle == NULL) return ESP_ERR_INVALID_STATE;
    uint8_t buf[2] = {0};
    esp_err_t ret = I2C_Read(reg_addr, buf, 2,dev_handle);
    if (ret == ESP_OK) {
        *data = (buf[0] << 8) | buf[1]; // 合并为16位
    }
    return ret;
}