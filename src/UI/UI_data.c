#include "UI_data.h"
#include "stdlib.h"

int32_t g_temp_threshold    = 30;
int32_t g_humi_threshold    = 70;
float   g_temperature       = 30.0f;
float   g_humidity          = 70.0f;
uint16_t g_light            = 0;
bool    g_relay_state       = false;
uint8_t backlight           = 80;
char version[10]            = "1.0.0";
char time_str[16]           = "--:--:--";
char current_weather[16]    = " ";