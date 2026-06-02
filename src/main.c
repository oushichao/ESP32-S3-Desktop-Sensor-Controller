#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include <time.h>

#include "Device/LCD/LCD_Init.h"
#include "Device/BH1750/BH1750.h"
#include "Device/SHT30/SHT30.h"
#include "Device/Relay_And_Led/Relay_And_Led.h"
#include "Lvgl/Task/lvgl_port_task.h"
#include "Lvgl/Touch/FT6336_Touch.h"
#include "Lvgl/Indev/lv_port_indev.h"
#include "UI/UI_main.h"
#include "UI/UI_data.h"
#include "IOT/Get_Weather_Time/Get_Time/Ntp_Time.h"
#include "IOT/WIFI_Init/WIFI_Init.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_HTTPS.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_Parse.h"
#include "config.h"

static const char *TAG = "main";

// pio run -t monitor


// /* 触摸测试任务 */
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

// void app_main(void)
// {
//     ESP_LOGI(TAG, "boot");
//     lv_init();
//     /* 1. LCD 硬件初始化（背光会亮，屏幕为初始状态） */
//     ILI9341_Init();

//     /* 2. 触摸初始化 */
//     FT6336_Touch_Init();

//     lvgl_port_start();
//     UI_init();
    

//     /* 3. 启动触摸测试任务 */
//     xTaskCreatePinnedToCore(
//         touch_test_task,
//         "touch_test",
//         4096,
//         NULL,
//         5,
//         NULL,
//         1);   /* Core 1 */

//     ESP_LOGI(TAG, "touch test running, please touch the screen");
// }


// void app_main(void){
//     SHT30_Init();
//     BH1750_Init();

//     while (1) {
//        SHT30_Read_Data(&g_temperature, &g_humidity);
//        BH1750_ReadData(&g_light);

//        int ti = (int)g_temperature;
//        int td = (int)((g_temperature - ti) * 10 + 0.5);
//        int hi = (int)g_humidity;
//        int hd = (int)((g_humidity - hi) * 10 + 0.5);

//        ESP_LOGI(TAG, "temp:%d.%d, hum:%d.%d, light:%u",
//                 ti, abs(td), hi, abs(hd), (unsigned)g_light);
//        vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }
void app_main(){
    setenv("TZ", "CST-8", 1);    //东八区
    tzset();

    //Wifi_Sta_Init();
    //OTA_Rollback_Check();   
    //NTP_Init();
    // char *json = Weather_HTTPS_Fetch_Now(CITY_ID, API_KEY);
    // if (json) {
    //     if (Weather_Parse_Now(json)) {
    //         ESP_LOGI("WEATHER", "天气: %s", current_weather);
    //     }  
    //     free(json);
    // }
     
    ILI9341_Init();
    FT6336_Touch_Init();

    /* 步骤 2：启动 LVGL（lv_init + tick + 显示驱动 + LVGL 任务） */
    if (!lvgl_port_start()) {
        ESP_LOGE(TAG, "lvgl_port_start failed");
        return;
    }
    lv_port_indev_init();

    /* 步骤 3：创建 UI。所有 LVGL API 调用必须在锁内进行。 */
    if (lvgl_port_lock(0)) {
        UI_init();
        lvgl_port_unlock();
    }
}

