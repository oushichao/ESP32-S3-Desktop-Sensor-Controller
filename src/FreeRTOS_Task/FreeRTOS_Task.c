#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "FreeRTOS_Task.h"

#include "IOT/OTA/OTA.h"
#include "Device/BH1750/BH1750.h"
#include "Device/SHT30/SHT30.h"
#include "Device/Relay/Relay.h"
#include "Device/LCD/LCD_Init.h"
#include "UI/UI_data.h"
#include "UI/UI_main.h"
#include "IOT/OneNET/OneNET_dm.h"
#include "IOT/OneNET/OneNET_MQTT.h"
#include "IOT/WIFI_Init/WIFI_Init.h"
#include "IOT/Get_Weather_Time/Get_Time/Ntp_Time.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_Parse.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_HTTPS.h"
#include "config.h"

static const char* TAG="FreeRTOS_Task";
//总线任务
#define TOTAL_TASK_SIZE             2*1024
#define TOTAL_TASK_PRIO             5
TaskHandle_t    Total_Task_Handle;

//屏幕刷新，触摸输入
#define GUI_TASK_SIZE               8*1024
#define GUI_TASK_PRIO               2
TaskHandle_t    Gui_Task_Handle;

//传感器任务,数据发送给GUI与网络任务
#define SENSOR_TASK_SIZE            4*1024
#define SENSOR_TASK_PRIO            1
TaskHandle_t    Sensor_Task_Handle;

//事件驱动，MQTT发布订阅，HTTP天气请求，NTP同步任务
#define NETWORK_TASK_SIZE           8*1024
#define NETWORK_TASK_PRIO           3
TaskHandle_t    Network_Task_Handle;

//ota远程升级
#define OTA_TASK_SIZE               8*1024
#define OTA_TASK_PRIO               4
TaskHandle_t    OTA_Task_Handle;

//天气获取，时间同步
#define TIME_WEATHER_TASK_SIZE      4*1024
#define TIME_WEATHER_TASK_PRIO      5
TaskHandle_t  Time_Weather_Handle;

/* ==========  Seeting控制刷新   ======= */
extern lv_obj_t *label_tem_value;
extern lv_obj_t *label_hum_value;
extern lv_obj_t *label_back_value;
extern lv_obj_t* progress_bar;
/*===========  Home控制刷新  ===========*/
extern lv_obj_t *led_wifi;
extern lv_obj_t *led_mqtt;
extern lv_obj_t *current_timer;
extern lv_obj_t *current_temp;
extern lv_obj_t *current_humi; 
extern lv_obj_t *current_light;
extern lv_obj_t *label_home_relay;
extern lv_obj_t *weather;
/*===========   Chart控制刷新  =========*/
extern lv_obj_t *chart_temp;
extern lv_obj_t *chart_humi;
extern lv_obj_t *chart_light;
extern lv_chart_series_t *ser_tem;
extern lv_chart_series_t *ser_hum;
extern lv_chart_series_t *ser_light;

//wifi连接与mqtt连接的指示灯
extern bool wifi_state;
extern bool mqtt_state;

void Gui_Task(){
    TickType_t last_wake_time=xTaskGetTickCount();
    while(1){
        if (lvgl_port_lock(pdMS_TO_TICKS(100))) {
            // ---- 指示灯 ----
            if(led_wifi)set_led_status(led_wifi,wifi_state);
            if(led_mqtt)set_led_status(led_mqtt,mqtt_state);
            // ---- 时间 ----
            Get_Time_Str(time_str, 16);
            if (current_timer) {
                lv_label_set_text(current_timer, time_str);
            }
            // ---- 天气 ----
            if (weather) {
                lv_label_set_text(weather, current_weather);
            }
            // ---- 温度 ----
            if (current_temp) {
                int ti = (int)g_temperature;
                int td = (int)((g_temperature - ti) * 10 + 0.5);
                lv_label_set_text_fmt(current_temp, "%d.%d", ti, abs(td));
            }
            // ---- 湿度 ----
            if (current_humi) {
                int hi = (int)g_humidity;
                int hd = (int)((g_humidity - hi) * 10 + 0.5);
                lv_label_set_text_fmt(current_humi, "%d.%d", hi, abs(hd));
            }
            // ---- 光照 ----
            if (current_light) {
                lv_label_set_text_fmt(current_light, "%u", (unsigned)g_light);
            }
            // ---- 背光滑块同步 ----
            if (label_back_value) {
                lv_label_set_text_fmt(label_back_value, "%u", (unsigned)backlight);
                Backlight_Set(backlight);
            }
            // ---- 继电器状态 ----
            if (label_home_relay) {
                lv_label_set_text(label_home_relay, g_relay_state ? "True" : "False");
                static bool last_relay_state = false;
                if (g_relay_state != last_relay_state) {
                    Relay_Set(g_relay_state);
                    last_relay_state = g_relay_state;
                }

            }
            // ---- 温度折线图 ----
            if(chart_temp&&ser_tem){
                lv_chart_set_next_value(chart_temp, ser_tem, (int32_t)(g_temperature*10));
                lv_chart_refresh(chart_temp);
            }
            // ---- 湿度折线图 ----
            if(chart_humi&&ser_hum){
                lv_chart_set_next_value(chart_humi, ser_hum, (int32_t)(g_humidity*10));
                lv_chart_refresh(chart_humi);
            }
            // ---- 光强折线图 ----
            if(chart_light&&ser_light){
                lv_chart_set_next_value(chart_light, ser_light,(int32_t)(g_light));
                lv_chart_refresh(chart_light);
            }
            lvgl_port_unlock();

        }
        vTaskDelayUntil(&last_wake_time,pdMS_TO_TICKS(1000));
    }
}

void Sensor_Task(){    
    TickType_t last_wake_time=xTaskGetTickCount();
    while(1){
        BH1750_ReadData(&g_light);
        SHT30_Read_Data(&g_temperature,&g_humidity);
        ESP_LOGI(TAG,"Tem:%d.Hum:%d.Light:%d",g_temperature,g_humidity,g_light);
        vTaskDelayUntil(&last_wake_time,pdMS_TO_TICKS(1000));
    }
}

void Network_Task(){
    xEventGroupWaitBits(wifi_ev,WIFI_CONNECTED_BIT,pdFALSE,pdTRUE,portMAX_DELAY);
    xEventGroupWaitBits(wifi_ev, MQTT_CONNECT_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    TickType_t last_wake_time=xTaskGetTickCount();
    while(1){
        cJSON* property_js=OneNET_Property_Upload();
        char* data=cJSON_PrintUnformatted(property_js);
        Connect_Post_Data(data);
        cJSON_Delete(property_js);
        cJSON_free(data);    
        vTaskDelayUntil(&last_wake_time,pdMS_TO_TICKS(1000));
    }
}

void OTA_Task(){
    xEventGroupWaitBits(wifi_ev,WIFI_CONNECTED_BIT,pdFALSE,pdTRUE,portMAX_DELAY);
    xEventGroupWaitBits(wifi_ev, OTA_DOWNLOAD_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}

static void Time_Weather_Task(){
    xEventGroupWaitBits(wifi_ev, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    TickType_t last_wake_time=xTaskGetTickCount();

    NTP_Init();
    Get_Time_Str(time_str,sizeof(time_str));
    ESP_LOGI(TAG,"当前时间为:%s",time_str);
    while(1){
        char *json = Weather_HTTPS_Fetch_Now(CITY_ID, API_KEY);
        if (json) {
            if (Weather_Parse_Now(json)) {
                ESP_LOGI("WEATHER", "天气: %s", current_weather);
            }
            free(json);
        }
        vTaskDelayUntil(&last_wake_time,pdMS_TO_TICKS(3600*1000));
    }
}

void Total_Task(){
    xTaskCreate(
        Time_Weather_Task,
        "Time_Weather_Task",
        TIME_WEATHER_TASK_SIZE,
        NULL,
        TIME_WEATHER_TASK_PRIO,
        &Time_Weather_Handle
    );
    xTaskCreate( 
        Gui_Task,
        "gui_task",
        GUI_TASK_SIZE,
        NULL,
        GUI_TASK_PRIO,
        &Gui_Task_Handle
    );

    xTaskCreate( 
        Sensor_Task,
        "sensor_task",
        SENSOR_TASK_SIZE,
        NULL,
        SENSOR_TASK_PRIO,
        &Sensor_Task_Handle
    );

    xTaskCreate( 
        Network_Task,
        "Network_Task",
        NETWORK_TASK_SIZE,
        NULL,
        NETWORK_TASK_PRIO,
        &Network_Task_Handle
    );

    xTaskCreate( 
        OTA_Task,
        "OTA_Task",
        OTA_TASK_SIZE,
        NULL,
        OTA_TASK_PRIO,
        &OTA_Task_Handle
    );

    vTaskDelete(NULL);
}

void Start_FreeROTS_Task(){
    xTaskCreate( 
        Total_Task,
        "Total_Task",
        TOTAL_TASK_SIZE,
        NULL,
        TOTAL_TASK_PRIO,
        &Total_Task_Handle
    );
}