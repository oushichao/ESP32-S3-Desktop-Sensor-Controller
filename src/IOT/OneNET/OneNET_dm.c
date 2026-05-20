#include "driver/gpio.h"
#include "OneNET_dm.h"
#include "driver/ledc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "OneNET_MQTT.h"
#include "UI/UI_data.h"   // ← 新增


extern int32_t g_temp_threshold;
extern int32_t g_humi_threshold;
extern float   g_temperature;
extern float   g_humidity;
extern int32_t g_light;
extern bool    g_relay_state;


void OneNET_Property_Handle(cJSON *root){
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
    cJSON *param_js = cJSON_GetObjectItem(root, "params");
    if (param_js) {
        cJSON *name_js = param_js->child;
        while (name_js) {
            if (strcmp(name_js->string, "temp_threshold") == 0) {
                g_temp_threshold = (int32_t)cJSON_GetNumberValue(name_js);
            }
            else if (strcmp(name_js->string, "relay_switch") == 0) {
                g_relay_state = cJSON_IsTrue(name_js) ? true : false;
            }
            else if (strcmp(name_js->string, "humi_threshold") == 0) {
                g_humi_threshold = (int32_t)cJSON_GetNumberValue(name_js);
            }
            name_js = name_js->next;
        }
    }
}

cJSON *OneNET_Property_Upload(void){
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
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", "123");
    cJSON_AddStringToObject(root, "version", "1.0");
    cJSON *param_js = cJSON_AddObjectToObject(root, "params");

    cJSON *t = cJSON_AddObjectToObject(param_js, "temperature");
    cJSON_AddNumberToObject(t, "value", g_temperature);

    cJSON *h = cJSON_AddObjectToObject(param_js, "humidity");
    cJSON_AddNumberToObject(h, "value", g_humidity);

    cJSON *l = cJSON_AddObjectToObject(param_js, "light");
    cJSON_AddNumberToObject(l, "value", g_light);

    cJSON *r = cJSON_AddObjectToObject(param_js, "relay_state");
    cJSON_AddBoolToObject(r, "value", g_relay_state);

    return root;
}
