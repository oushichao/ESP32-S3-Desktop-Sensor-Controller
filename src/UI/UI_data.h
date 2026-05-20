#pragma once
#include <stdint.h>
#include <stdbool.h>

/* 阈值（由 LVGL slider 或云端下发修改） */
extern int32_t g_temp_threshold;
extern int32_t g_humi_threshold;

/* 传感器当前值 */
extern float   g_temperature;
extern float   g_humidity;
extern int32_t g_light;

/* 继电器 */
extern bool    g_relay_state;
