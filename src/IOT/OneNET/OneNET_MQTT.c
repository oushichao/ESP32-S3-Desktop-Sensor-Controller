#include "mqtt_client.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cJSON.h"

#include "OneNET_MQTT.h"
#include "OneNET_Token.h"
#include "OneNET_dm.h"
#include "config.h"

#define TAG "onenet"

esp_mqtt_client_handle_t    mqtt_handler=NULL;
extern EventGroupHandle_t   wifi_ev;

static void OneNET_Property_Ack(const char* id,int code,const char* msg);
static void OneNET_Subscribe();


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data){
    // 强转为MQTT事件指针
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(wifi_ev, BIT1);
            OneNET_Subscribe();
            //上报数据,为了数据同步
            cJSON* property_js=OneNET_Property_Upload();           
            char*data=cJSON_PrintUnformatted(property_js);//将一个cjson对象转为json格式字符串
            Connect_Post_Data(data);
            cJSON_free(data);
            cJSON_Delete(property_js);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:   //onenet平台下行任何数据都会进入
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            char topic_buf[128] = {0};
            char data_buf[256]  = {0}; 
                
            int tlen = (event->topic_len < 127) ? event->topic_len : 127;
            int dlen = (event->data_len < 255) ? event->data_len : 255;
            //补'\0' 
            memcpy(topic_buf, event->topic, tlen);
            memcpy(data_buf,  event->data,  dlen);

            printf("TOPIC: %s\r\n", topic_buf);
            printf("DATA: %s\r\n", data_buf);
            if(strstr(topic_buf,"property/set")){   //下行数据处理
                cJSON*property=cJSON_Parse(data_buf);//将json格式字符串转为一个cjson对象
                OneNET_Property_Handle(property);
                cJSON* id_js=cJSON_GetObjectItem(property,"id");
                
                //200是成功码,其他为失败码
                OneNET_Property_Ack(cJSON_GetStringValue(id_js),200,"success");
                cJSON_Delete(property);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            ESP_LOGI(TAG, "Other event id: %d", event_id);
            break;
    }
}

//定义函数：启动OneNET物联网平台连接，返回ESP32标准错误码
esp_err_t OneNET_Start(){
    esp_mqtt_client_config_t    mqtt_config;
    memset(&mqtt_config,0,sizeof(esp_mqtt_client_config_t));
    mqtt_config.broker.address.uri="mqtt://mqtts.heclouds.com";
    mqtt_config.broker.address.port=1883;

    mqtt_config.credentials.client_id=ONENET_DEVICE_NAME;   //设备名称
    mqtt_config.credentials.username=ONENET_PRODUCT_ID;     //产品ID
    static char token[256];
    dev_token_generate(
        token,                      //存放生成的token
        SIG_METHOD_SHA256,          //加密方式
        2074859487,                 //token过期时间
        ONENET_PRODUCT_ID,          //产品ID
        ONENET_DEVICE_NAME,         //设备名称
        ONENET_PRODUCT_ACCESS_KEY   //产品访问密钥        
    );
    //把生成的Token，设置为MQTT连接密码
    mqtt_config.credentials.authentication.password=token;

    //遗嘱消息
    mqtt_config.session.last_will.topic="device/status";
    mqtt_config.session.last_will.msg="offline";
    mqtt_config.session.last_will.qos=1;
    mqtt_config.session.last_will.retain=1;

    //初始化MQTT客户端
    mqtt_handler=esp_mqtt_client_init(&mqtt_config);

    //事件回调函数,自动调用 mqtt_event_handler
    esp_mqtt_client_register_event(mqtt_handler,ESP_EVENT_ANY_ID,mqtt_event_handler,NULL);
    //连接OneNET服务器
    return  esp_mqtt_client_start(mqtt_handler);
}

/*
=============================设备属性设置==============================
=============================下行响应主题=============================
设备->平台 回复属性查询,平台下发指令需要回复,被动回复
1.消息id号 2.结果状态码 3.错误信息
*/
static void OneNET_Property_Ack(const char* id,int code,const char* msg){
    char topic[128];  //拼接onenet平台规定的:固定应答主题
    snprintf(topic,128,"$sys/%s/%s/thing/property/set_reply",ONENET_PRODUCT_ID,ONENET_DEVICE_NAME);
    cJSON* replay_js=cJSON_CreateObject();
    cJSON_AddStringToObject(replay_js,"id",id);
    cJSON_AddNumberToObject(replay_js,"code",code);
    cJSON_AddStringToObject(replay_js,"msg",msg);
    char*data=cJSON_PrintUnformatted(replay_js);    //转化为压缩的可发送的json文本
    esp_mqtt_client_publish(mqtt_handler,topic,data,strlen(data),1,0);
    cJSON_free(data);
    cJSON_Delete(replay_js);
}


//订阅上行响应，下行请求主题
static void OneNET_Subscribe(){
    char topic[128]; 
    //订阅上报属性回复主题,上行响应主题
    snprintf(topic,128,"$sys/%s/%s/thing/property/post/reply",ONENET_PRODUCT_ID,ONENET_DEVICE_NAME);
    esp_mqtt_client_subscribe_single(mqtt_handler,topic,1);
    //下行请求主题
    snprintf(topic,128,"$sys/%s/%s/thing/property/set",ONENET_PRODUCT_ID,ONENET_DEVICE_NAME);
    esp_mqtt_client_subscribe_single(mqtt_handler,topic,1);
}

//上行请求主题
esp_err_t Connect_Post_Data(const char*data){
    char topic[128];
    snprintf(topic,128,"$sys/%s/%s/thing/property/post",ONENET_PRODUCT_ID,ONENET_DEVICE_NAME);
    ESP_LOGI(TAG,"Upload topic:%s,payload:%s",topic,data);
    return esp_mqtt_client_publish(mqtt_handler,topic,data,strlen(data),0,0);
}