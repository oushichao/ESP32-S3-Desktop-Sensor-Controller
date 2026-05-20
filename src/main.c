#include "lvgl.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"


#include "IOT/WIFI_Init/WIFI_Init.h"
#include "FreeRTOS_Task/FreeRTOS_Task.h"
#include "Device/LCD/LCD_Init.h"
#include "Lvgl/Display/lv_port_display.h"
#include "Lvgl/Display/lv_port_display.h"


extern esp_lcd_panel_handle_t panel_handler;

void app_main(void)
{
    lv_init();
    ST7789_Init();
    lvgl_port_display_init(panel_handler);

    /* UI 控件（从 VS2022 复制过来） */

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}



