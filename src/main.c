#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>

#include "config.h"
#include "Device/LCD/LCD_Init.h"
#include "Lvgl/Display/lv_port_display.h"
#include "Lvgl/Touch/FT6336_Touch.h"
#include "Lvgl/Indev/lv_port_indev.h"
#include "UI/UI_main.h"
#include "IOT/WIFI_Init/WIFI_Init.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_HTTPS.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_Parse.h"
#include "IOT/Get_Weather_Time/Get_Time/Ntp_Time.h"

//pio run -t monitor
static const char* TAG="main";
extern EventGroupHandle_t wifi_ev;

void app_main(){
    setenv("TZ", "CST-8", 1);   // CST = 中国标准时间，UTC+8
    tzset();
    Wifi_Sta_Init();
    xEventGroupWaitBits(wifi_ev, BIT0, pdFALSE, pdTRUE, portMAX_DELAY);
    NTP_Init();
    char current_weather[512];
    char current_time[256];
    Get_Time_Str(current_time,256);
    ESP_LOGI(TAG,"TIME:%s",current_time);
    char* cjson_str=Weather_HTTPS_Fetch_Now(CITY_ID,API_KEY);
    if(cjson_str){
        if(Weather_Parse_Now(cjson_str,current_weather,512)){
            ESP_LOGI(TAG,"WEATHER:%s",current_weather);
        }
        free(cjson_str);
    }
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// void app_main(void)
// {
//     lv_init();
//     ILI9341_Init();
//     lvgl_port_display_init();

//     lv_obj_t *scr = lv_screen_active();
//     lv_obj_set_style_bg_color(scr, lv_color_hex(0xFF0000), 0);

//     lv_obj_t *label = lv_label_create(scr);
//     lv_label_set_text(label, "TEST");
//     lv_obj_center(label);

//     while (1) {
//         lv_timer_handler();
//         vTaskDelay(pdMS_TO_TICKS(5));
//     }
// }








