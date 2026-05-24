#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#define WIFI_SSID         "HONOR 50"            
#define WIFI_PASSWORD     "12345678"
#define WIFI_MAX_RETRY    5

extern EventGroupHandle_t   wifi_ev;

void Wifi_Sta_Init();