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

#define MAX_RETRY   5

static const char* TAG ="WIFI_INIT";
static uint8_t reconnect=0;
EventGroupHandle_t   wifi_ev;

esp_netif_t *esp_netif;     //网络接口句柄,指定查询的网络接口

void Wifi_Event_Handler(void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data){
    if(event_base==WIFI_EVENT){
        switch(event_id){   //wifi_event_t类型
            case    WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case    WIFI_EVENT_STA_DISCONNECTED:
                if(++reconnect<=MAX_RETRY){
                    ESP_LOGI(TAG,"尝试重连,次数为%d/%d",reconnect,MAX_RETRY);
                    esp_wifi_connect();
                }
                break;
            case    WIFI_EVENT_STA_CONNECTED:
                reconnect=0;
                ESP_LOGI(TAG,"连接成功!!!!");
                break;
            default:
                break;
        }
    }
    else if(event_base==IP_EVENT){
        switch (event_id){  //ip_event_t类型
            case    IP_EVENT_STA_GOT_IP:{
                xEventGroupSetBits(wifi_ev,WIFI_CONNECTED_BIT);   //确保wifi连接再进行MQTT
                esp_netif_ip_info_t ip_info;
                esp_netif_get_ip_info(esp_netif, &ip_info);
                ESP_LOGI(TAG, "获取的ip地址为:" IPSTR, IP2STR(&ip_info.ip));
                break;
            }
            default:
                break;
        }
    }
}

void Wifi_Sta_Init(){
    wifi_ev=xEventGroupCreate();
    esp_err_t ret = nvs_flash_init();                       //NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        //页面不兼容或者已满,满足OTA回滚要求
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }                                       
    esp_netif_init();                                       //tcp/ip
    esp_event_loop_create_default();                        //循环事件组

    esp_netif=esp_netif_create_default_wifi_sta();          //tcp/ip绑定sta
    wifi_init_config_t config=WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);                                 //默认wifi配置
    esp_wifi_set_mode(WIFI_MODE_STA);                       //设置sta模式

    //设置回调
    static esp_event_handler_instance_t  wifi_event;
    static esp_event_handler_instance_t  ip_event;
    esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,Wifi_Event_Handler,NULL,&wifi_event);
    esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,Wifi_Event_Handler,NULL,&ip_event);

    //配置sta模式
    wifi_config_t wifi_config={
        .sta={
            .ssid=WIFI_SSID,
            .password=WIFI_PASSWORD
        }
    };

    esp_wifi_set_config(WIFI_IF_STA,&wifi_config);          //配置写入sta模式
    esp_wifi_start();                                       //wifi开始工作
}

