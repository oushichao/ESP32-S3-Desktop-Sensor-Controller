#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include <time.h>
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "esp_task_wdt.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "Device/LCD/LCD_Init.h"
#include "Device/LCD/FT6336_Touch.h"
#include "Device/BH1750/BH1750.h"
#include "Device/SHT30/SHT30.h"
#include "Device/Relay/Relay.h"
#include "UI/UI_main.h"
#include "UI/UI_data.h"
#include "IOT/WIFI_Init/WIFI_Init.h"
#include "IOT/Get_Weather_Time/Get_Time/Ntp_Time.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_HTTPS.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_Parse.h"
#include "IOT/OTA/OTA.h"
#include "MY_I2C/MY_I2C.h"
#include "NVS/NVS.h"
#include "config.h"
#include "FreeRTOS_Task/FreeRTOS_Task.h"
#include "IOT/EMQX/EMQX.h"

// pio run -t monitor
static const char* TAG="mian";

void app_main(){
    setenv("TZ", "CST-8", 1);
    tzset();
    ESP_LOGI(TAG, "当前版本: %s", Get_App_Version());
    /* ========== 0. NVS 初始化 ========== */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    /* ========== 1. OTA 队列 + NVS ========== */
    Ota_Task_Init();
    NVS_Read_Config();
    /* ========== 2. 硬件初始化 ========== */
    Relay_Init();
    BH1750_Init();
    SHT30_Init();
    /* ========== 3. LCD + LVGL + UI ========== */
    Lvgl_Start();
    lvgl_port_lock(portMAX_DELAY);
    UI_Init();
    lvgl_port_unlock();
    /* ========== 4. WiFi + 回滚检查 ========== */
    Wifi_Sta_Init();
    OTA_Rollback_Check();   // 等 WiFi 10s 验证固件
    /* ========== 5. EMQX初始化 ========== */
    EMQX_Start();
    /* ========== 5. FreeRTOS 任务 ========== */
    Start_FreeROTS_Task();
}

