#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lvgl.h"
#include "lwip/sys.h"

#include "WIFI_Init.h"
#include "UI/UI_main.h"

extern lv_obj_t *led_wifi;
extern lv_obj_t *led_mqtt;

static const char* TAG ="WIFI_Init";
EventGroupHandle_t   wifi_ev;   //确保wifi连接再连接mqtt
static uint8_t retry_count=0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data){
    if(event_base==WIFI_EVENT){      //处理WIFI相关事件
        switch(event_id){
            case    WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi驱动启动完成:开始向路由器发起连接");
                esp_wifi_connect();  // 主动向目标路由器发起WiFi连接请求
                break;
            case    WIFI_EVENT_STA_DISCONNECTED:
                if(retry_count<WIFI_MAX_RETRY){
                    //警告日记
                    ESP_LOGW(TAG, "WiFi连接断开,尝试重连");
                    retry_count++;
                    esp_wifi_connect();
                    ESP_LOGI(TAG, "重连尝试次数：%d/%d", retry_count, WIFI_MAX_RETRY);
                }
                else{
                    ESP_LOGE(TAG, "超过最大重试次数,WiFi连接失败");
                    retry_count=0;
                }
                break;
            case    WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG,"WIFI连接成功!!!");
                break;
            default:    
                break;    
        }
    }
    else if(event_base==IP_EVENT){      //处理IP相关事件
        switch (event_id){
            // 事件：成功获取到IP地址（WiFi连接最终成功标志）
            case    IP_EVENT_STA_GOT_IP:
                // 提取事件携带的IP信息
                ip_event_got_ip_t* ip_info = (ip_event_got_ip_t*)event_data;
                xEventGroupSetBits(wifi_ev,BIT0);
                ESP_LOGI(TAG, "WiFi连接成功!获取到IP地址:" IPSTR, IP2STR(&ip_info->ip_info.ip));
                retry_count = 0;              
                break;
            default:
                break;
        }
    }                            
}

void Wifi_Sta_Init(){
    wifi_ev=xEventGroupCreate();
    retry_count=0;
// ===================== 第一部分：前置必选初始化（步骤1-3）======================
    ESP_ERROR_CHECK(nvs_flash_init());                          //初始化NVS
    ESP_ERROR_CHECK(esp_netif_init());                          //初始化TCP/IP协议栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());           //初始化循环事件组
// ===================== 第二部分：WiFi核心通用初始化（步骤4-7）===================   
    esp_netif_create_default_wifi_sta();                        //STA/AP模块与LWIP协议栈连接
    wifi_init_config_t wifi_init_sta=WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_sta);                              // 初始化WiFi底层驱动
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));          // 设置WiFi工作模式
    static esp_event_handler_instance_t  wifi_event_instance;   //注册事件回调处理函数
    static esp_event_handler_instance_t  ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,                 // 事件类型
        ESP_EVENT_ANY_ID,           // 监听所有WiFi事件ID
        &wifi_event_handler,        // 绑定的回调处理函数
        NULL,                       // 自定义传参，此处为空
        &wifi_event_instance        // 事件实例句柄，用于后续注销
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,        // 仅监听STA模式获取IP事件
        &wifi_event_handler,
        NULL,
        &ip_event_instance
    ));   
// ================ 第三部分：模式专属配置与启动（步骤8-9）==============
    wifi_config_t wifi_config={     //配置STA模式专属运行参数
        .sta={
            .ssid=WIFI_SSID,
            .password=WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&wifi_config));        
    ESP_ERROR_CHECK(esp_wifi_start());                  //启动WiFi驱动
}

