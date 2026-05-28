#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

#include "Device/LCD/LCD_Init.h"
#include "Lvgl/Task/lvgl_port_task.h"
#include "Lvgl/Touch/FT6336_Touch.h"
#include "UI/UI_main.h"
#include "UI/UI_data.h"
#include "Lvgl/Indev/lv_port_indev.h"
#include "IOT/Get_Weather_Time/Get_Time/Ntp_Time.h"
#include "IOT/WIFI_Init/WIFI_Init.h"


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

// /* 在屏幕正中央显示白色 "test" 字样 */
// static void ui_show_test(void)
// {
//     /* 1. 把活动屏幕背景设为黑色 */
//     lv_obj_t *scr = lv_screen_active();
//     lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
//     lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

//     /* 2. 创建 label，文字白色，居中 */
//     lv_obj_t *label = lv_label_create(scr);
//     lv_label_set_text(label, "test");
//     lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
//     lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);


//     lv_obj_center(label);

// }

void app_main(void)
{
    Wifi_Sta_Init();
    NTP_Init();

    /* 步骤 1：先初始化 LCD 硬件（SPI、面板、背光、信号量） */
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
        //ui_show_test();
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "ui created");
}
