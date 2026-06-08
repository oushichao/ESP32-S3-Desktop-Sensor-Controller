#pragma once
#include <stdint.h>
#include <stdbool.h>

/* 阈值（由 LVGL slider 或云端下发修改） */
extern int32_t g_temp_threshold;
extern int32_t g_humi_threshold;

/* 传感器当前值 */
extern float   g_temperature;
extern float   g_humidity;
extern uint16_t g_light;

/* 继电器 */
extern bool    g_relay_state;

/* 背光强度 */
extern uint8_t backlight;

/* 版本号*/
extern char version[10] ;

/* 时间*/
extern char time_str[16];

/*天气*/
extern char current_weather[16];

/*指示灯状态*/
extern bool mqtt_state;
extern bool wifi_state;