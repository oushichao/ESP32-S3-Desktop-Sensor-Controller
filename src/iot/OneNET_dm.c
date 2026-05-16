#include "driver/gpio.h"
#include "OneNET_dm.h"
#include "driver/ledc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "OneNET_MQTT.h"

bool relay_switch=false;     //继电器开关控制
float temp_threshold=30.0;
float humi_threshold=70.0;

float temperature=30.0;
float humidity=70.0;
int32_t light=0;
bool relay_state=false;      //继电器当前状态
void OneNET_Property_Handle(cJSON*root){   //解析处理云端下发给你的指令
    /*  下行格式        
    {   
        "id": "123",
        "version": "1.0",
        "params": {
            "relay_switch":true,
            "temp_threshold":30,
            "humi_threshold":70
        }
    }
    */
    cJSON* param_js = cJSON_GetObjectItem(root, "params");     
    if (param_js) {
        cJSON* name_js = param_js->child; 
        while (name_js) {
            if (strcmp(name_js->string, "temp_threshold") == 0) {
                temp_threshold = cJSON_GetNumberValue(name_js);
            }
            else if (strcmp(name_js->string, "relay_switch") == 0) {
                relay_switch = cJSON_IsTrue(name_js) ? true : false;
                relay_state = relay_switch;
            }
            else if (strcmp(name_js->string, "humi_threshold") == 0) {
                humi_threshold = cJSON_GetNumberValue(name_js);
            }
            name_js = name_js->next;
        }
    }
}

cJSON* OneNET_Property_Upload(){          //生成要上报给云端的传感器数据 JSON
    /*  上行格式
    {   
        "id": "123",
        "version": "1.0",
        "params": {
            "temperature":{
                "value":30
            },    
            "humidity":{
                "value":70
            },
            "light":{
                "value":0
            },
            "relay_state":{
                "value":true
            },
        }
    }
    */
    cJSON*root=cJSON_CreateObject();
    cJSON_AddStringToObject(root,"id","123");
    cJSON_AddStringToObject(root,"version","1.0");
    cJSON* param_js=cJSON_AddObjectToObject(root,"params");
    //温度
    cJSON* temperature_js=cJSON_AddObjectToObject(param_js,"temperature");
    cJSON_AddNumberToObject(temperature_js,"value",temperature);
    //湿度
    cJSON* humidity_js=cJSON_AddObjectToObject(param_js,"humidity");
    cJSON_AddNumberToObject(humidity_js,"value",humidity);
    //亮度
    cJSON* light_js=cJSON_AddObjectToObject(param_js,"light");
    cJSON_AddNumberToObject(light_js,"value",light);
    //继电器状态
    cJSON* relay_state_js=cJSON_AddObjectToObject(param_js,"relay_state");
    cJSON_AddBoolToObject(relay_state_js,"value",relay_state); 
    return root;
}