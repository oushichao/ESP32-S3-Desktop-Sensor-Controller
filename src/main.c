#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include <time.h>
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

#include "Device/LCD/LCD_Init.h"
#include "Device/BH1750/BH1750.h"
#include "Device/SHT30/SHT30.h"
#include "Device/LCD/FT6336_Touch.h"
#include "Device/Relay_And_Led/Relay_And_Led.h"
#include "UI/UI_main.h"
#include "UI/UI_data.h"
#include "IOT/Get_Weather_Time/Get_Time/Ntp_Time.h"
#include "IOT/WIFI_Init/WIFI_Init.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_HTTPS.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_Parse.h"
#include "config.h"

// pio run -t monitor
//static const char* TAG="mian";


// static void network_task(void *arg)
// {
//     // 等待 WiFi 连上
//     xEventGroupWaitBits(wifi_ev, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

//     NTP_Init();
//     Get_Time_Str(time_str,16);
//     ESP_LOGI(TAG,"当前时间为:%s",time_str);
//     char *json = Weather_HTTPS_Fetch_Now(CITY_ID, API_KEY);
//     if (json) {
//         if (Weather_Parse_Now(json)) {
//             ESP_LOGI("WEATHER", "天气: %s", current_weather);
//         }
//         free(json);
//     }

//     vTaskDelete(NULL);
// }

void app_main()
{
    setenv("TZ", "CST-8", 1);
    tzset();

    //Wifi_Sta_Init();

    // 屏幕 + UI
    Lvgl_Start();
    lvgl_port_lock(portMAX_DELAY);
    UI_init();
    lvgl_port_unlock();

    //网络操作在后台任务，不阻塞
    //xTaskCreate(network_task, "network", 16384, NULL, 5, NULL);  // 8KB → 16KB
}


// void app_main(void)
// {
//     setenv("TZ", "CST-8", 1);    /* 东八区 */
//     tzset();
//     //Wifi_Sta_Init();
//     //OTA_Rollback_Check();
//     // NTP_Init();
//     /* 1. 硬件初始化 */
//     ILI9341_Init();
//     FT6336_Touch_Init();
//     /* 2. 启动 LVGL 任务（之后 lv_timer_handler 会周期跑） */
//     lvgl_port_start();
//     /* 3. indev + UI 必须在锁内创建，且必须确保真的执行到 ！
//           ——这里用 do/while 重试，确保拿到锁，绝不被静默跳过 */
//     while (!lvgl_port_lock(0)) {
//         vTaskDelay(pdMS_TO_TICKS(10));   /* 没拿到锁就等 10ms 再试 */
//     }
//     lv_port_indev_init();
//     UI_init();
//     lvgl_port_unlock();
//     ESP_LOGI(TAG, "indev + UI init done");   /* 看到这句才算成功 */
// }


/* 触摸测试任务 */
// static void touch_test_task(void *arg)
// {
//     uint16_t x = 0, y = 0;
//     bool pressed = false;
//     bool last_pressed = false;

//     while (1) {
//         FT6336_Touch_Read(&x, &y, &pressed);

//         if (pressed) {
//             /* 持续按下时每次都打印；如果只想边沿打印，把下面 if 改成
//                (pressed && !last_pressed) */
//             ESP_LOGI(TAG, "TOUCH  x=%3u  y=%3u", x, y);
//         } else if (last_pressed) {
//             ESP_LOGI(TAG, "TOUCH release");
//         }

//         last_pressed = pressed;
//         vTaskDelay(pdMS_TO_TICKS(50));
//     }
// }

// void app_main(void){
//     setenv("TZ", "CST-8", 1);    //东八区
//     tzset();
    
//     //Wifi_Sta_Init();
//     //OTA_Rollback_Check();   
//     //NTP_Init();
//     // char *json = Weather_HTTPS_Fetch_Now(CITY_ID, API_KEY);
//     // if (json) {
//     //     if (Weather_Parse_Now(json)) {
//     //         ESP_LOGI("WEATHER", "天气: %s", current_weather);
//     //     }  
//     //     free(json);
//     // }
//     ILI9341_Init();
//     FT6336_Touch_Init();
    
//     lvgl_port_start();   

//     if (lvgl_port_lock(0)) {
//         lv_port_indev_init();
//         lvgl_port_unlock();
//     }

//     if (lvgl_port_lock(0)) {
//         UI_init();
//         lvgl_port_unlock();
//     }

//     // xTaskCreatePinnedToCore(
//     //     touch_test_task, "touch_test", 4096, NULL, 5, NULL, 1);
// }




