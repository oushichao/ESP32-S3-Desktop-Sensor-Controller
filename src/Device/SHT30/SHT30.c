#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "MY_I2C/MY_I2C.h"
#include "driver/i2c_master.h"
#include "SHT30.h"

#define SHT30_MEASURE_CYCLE 0x2236      //循环模式（周期1Hz）
#define SHT30_ADDR  0x44            
#define SHT30_SDA   GPIO_NUM_1
#define SHT30_SCL   GPIO_NUM_2

static bool sht30_inited = false;
static i2c_master_dev_handle_t sht30_handle = NULL;

void SHT30_Init(){
    if(sht30_inited)return;
    I2C_Init(SHT30_SDA,SHT30_SCL,SHT30_ADDR,&sht30_handle);
    uint8_t cmd_buf[2] = {
        (SHT30_MEASURE_CYCLE >> 8) & 0xFF,
        SHT30_MEASURE_CYCLE & 0xFF
    };
    i2c_master_transmit(sht30_handle, cmd_buf, 2, pdMS_TO_TICKS(100));
    sht30_inited = true;
}
void SHT30_Read_Data(float* temperature,float* humidity){
    uint8_t data[6]={0};
    //[温度高8位][温度低8位][温度CRC][湿度高8位][湿度低8位][湿度CRC]----周期1s
    if(sht30_handle==NULL||temperature==NULL||humidity==NULL)return;
    i2c_master_receive(sht30_handle,data,6,pdMS_TO_TICKS(100));
    uint16_t raw_temp = (data[0] << 8) | data[1];
    uint16_t raw_humi = (data[3] << 8) | data[4];    

    *temperature=-45 + 175 * (raw_temp / 65535.0);
    *humidity=100 * (raw_humi / 65535.0);
    // # 温度计算（单位：℃）
    // temp = -45 + 175 * (raw_temp / 65535.0)

    // # 湿度计算（单位：%RH）
    // humidity = 100 * (raw_humidity / 65535.0)
}