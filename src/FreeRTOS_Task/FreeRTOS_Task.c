#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "FreeRTOS_Task.h"

// static void lvgl_task(void *arg)
// {
//     while (1) {
//         lv_timer_handler();
//         vTaskDelay(pdMS_TO_TICKS(5));
//     }
// }

// void app_main(void)
// {
//     lv_init();
//     ST7789_Init();
//     lv_port_display_init(panel_handler);

//     /* UI 控件（从 VS2022 复制过来） */

//     xTaskCreate(lvgl_task, "lvgl", 4096 * 4, NULL, 5, NULL);
//     // app_main 可以 return 或干别的
// }
