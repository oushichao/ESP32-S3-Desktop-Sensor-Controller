#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>

void BH1750_Init();
void BH1750_ReadData(uint16_t* lux);