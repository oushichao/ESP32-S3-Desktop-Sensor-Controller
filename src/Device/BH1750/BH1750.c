#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "MY_I2C/MY_I2C.h"
#include "driver/i2c_master.h"
#include "BH1750.h"


#define BH1750_ADDR                 0x23             //0x5C或者0x23，取决于板上ADDR的引脚
#define BH1750_SDA                  GPIO_NUM_1
#define BH1750_SCL                  GPIO_NUM_2
const uint8_t BH1750_MEASURE_CYCLE_LOW =  0x12;      //连续测量低分辨率

static bool bh1750_inited = false;
static i2c_master_dev_handle_t bh1750_handle = NULL;

void BH1750_Init(){
    if(bh1750_inited)return;
    I2C_Init(BH1750_SDA,BH1750_SCL,BH1750_ADDR,&bh1750_handle);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_master_transmit(bh1750_handle,&BH1750_MEASURE_CYCLE_LOW,1,pdMS_TO_TICKS(100));
    bh1750_inited = true;
}

void BH1750_ReadData(uint16_t* lux){
    if(lux==NULL||bh1750_handle==NULL)return ;
    uint8_t data[2]={0};
    i2c_master_receive(bh1750_handle,data,2,pdMS_TO_TICKS(100));
    *lux=(data[0]<<8|data[1])/1.2;
}
//计算公式:Lux=RawValue/12