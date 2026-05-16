#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#include "driver/i2c_master.h"
void I2C_Init(gpio_num_t SDA,gpio_num_t SCL,uint8_t address,i2c_master_dev_handle_t* dev_handle);
esp_err_t I2C_Write_Word(uint8_t reg_addr,uint16_t data,i2c_master_dev_handle_t dev_handle);
esp_err_t I2C_Read(uint8_t reg_addr,uint8_t* read_buf,uint8_t read_size,i2c_master_dev_handle_t dev_handle);
esp_err_t I2C_Read_Word(uint8_t reg_addr, uint16_t *data,i2c_master_dev_handle_t dev_handle);
