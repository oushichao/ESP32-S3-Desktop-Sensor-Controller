#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "FreeRTOS_Task.h"

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

void Gui_Task(){

}

void Sensor_Task(){

}

void Network_Task(){

}

void OTA_Task(){

}

void Total_Task(){
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