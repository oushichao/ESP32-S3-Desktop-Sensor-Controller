#include "mqtt_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

#include "EMQX.h"
#include "config.h"
#include "EMQX_dm.h"
#include "IOT/WIFI_Init/WIFI_Init.h"
#include "UI/UI_data.h"

esp_mqtt_client_handle_t  emqx_handle=NULL;
static const char* TAG="MQEX";
extern EventGroupHandle_t wifi_ev;

/**
 * @brief 订阅服务端平台消息
 */
static void EMQX_Subscribe(){
    if(esp_mqtt_client_subscribe_single(emqx_handle,"esp32s3/envmonitor/cmd",1)>=0)
        ESP_LOGI(TAG,"订阅服务端消息成功!!!!");
    else
        ESP_LOGI(TAG,"订阅服务端消息失败");
}

/**
 * @brief 客户端发送传感器数据 qos:0,retain:0
 */
void EMQX_Post_Sensor(){
    cJSON* root=EMQX_Data_Upload();
    char* data=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if(data){
        if(esp_mqtt_client_publish(emqx_handle,"esp32s3/envmonitor/data",data,strlen(data),0,0)>=0)
            ESP_LOGI(TAG,"传感器数据发送成功!!!");
        else
            ESP_LOGI(TAG,"传感器数据发送失败!!!");
        cJSON_free(data);
    }
}

/**
 * @brief 客户端发送继电器状态 qos:1,retain:1
 */
void EMQX_Post_Relay(){
    char pay_load[32];
    snprintf(pay_load,sizeof(pay_load),"{\"relay\":%s}",g_relay_state ? "true":"false");
    if(esp_mqtt_client_publish(emqx_handle,"esp32s3/envmonitor/relay",pay_load,strlen(pay_load),1,1)>=0)
        ESP_LOGI(TAG,"继电器状态发送成功!!!");
    else
        ESP_LOGI(TAG,"继电器状态发送失败!!!");   
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    // 强转为MQTT事件指针
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG,"EMQT成功连接!!!!");
            EMQX_Subscribe();
            xEventGroupSetBits(wifi_ev,EMQX_CONNECT_BIT);
            break;

        case MQTT_EVENT_DATA:{
            char topic_buf[128]={0},data_buf[128]={0};
            uint16_t topic_len=event->topic_len < 127 ? event->topic_len : 127;
            uint16_t data_len=event->data_len < 127 ? event->data_len : 127;
            memcpy(topic_buf,event->topic,topic_len);
            memcpy(data_buf,event->data,data_len);
            ESP_LOGI(TAG,"收到：%s -> %s",topic_buf,data_buf);
            cJSON* root=cJSON_Parse(data_buf);
            if(root){
                EMQX_Cmd_Handle(root);
                cJSON_Delete(root);
            }
            break;
        }

        default:
            ESP_LOGI(TAG,"未知错误 id:%d",event_id);
            break;
    }
}

esp_err_t EMQX_Start(){
    esp_mqtt_client_config_t    emqx_config={
        .broker.address.uri         =   EMQX_HOST,
        .broker.address.port        =   EMQX_PORT,
        .credentials.client_id      =   EMQX_CLIENT_ID,

        //遗嘱消息
        .session.last_will.topic    =   "esp32s3/envmonitor/status",
        .session.last_will.msg      =   "offline",
        .session.last_will.qos      =   1,
        .session.last_will.retain   =   1,
         
        .session.keepalive          =   60,

        .network.disable_auto_reconnect =   false,
    };
    emqx_handle = esp_mqtt_client_init(&emqx_config);
    esp_mqtt_client_register_event(emqx_handle,ESP_EVENT_ANY_ID,mqtt_event_handler,NULL);
    return esp_mqtt_client_start(emqx_handle);
}

