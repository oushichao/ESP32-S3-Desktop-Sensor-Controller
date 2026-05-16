#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>

esp_err_t OneNET_Start();
esp_err_t Connect_Post_Data(const char*data);

