#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>

void SHT30_Init();
void SHT30_Read_Data(float* temperature,float* humidity);