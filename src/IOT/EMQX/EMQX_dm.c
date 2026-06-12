#include "mqtt_client.h"

#include "EMQX_dm.h"
#include "UI/UI_data.h"
#include "cJSON.h"
#include "NVS/NVS.h"
#include "FreeRTOS_Task/FreeRTOS_Task.h"

extern QueueHandle_t sensor_net_queue;
extern sensor_data_t net_data;

//上行请求
cJSON* EMQX_Data_Upload(){
    xQueueReceive(sensor_net_queue,&net_data,0);
    cJSON*root =cJSON_CreateObject();
    cJSON_AddNumberToObject(root,"temperature",net_data.temperature);
    cJSON_AddNumberToObject(root,"humidity",net_data.humidity);
    cJSON_AddNumberToObject(root,"light",net_data.light);
    return root;
}

//下行请求
void EMQX_Cmd_Handle(cJSON* root){
    cJSON* relay=cJSON_GetObjectItem(root,"relay");
    if(cJSON_IsBool(relay))
        g_relay_state=cJSON_IsTrue(relay);

    cJSON* temp =cJSON_GetObjectItem(root,"temp_thresh");
    if(cJSON_IsNumber(temp)){
        g_temp_threshold=(int32_t)temp->valueint;
        NVS_Write_int32_t("temp_thresh",g_temp_threshold);
    }

    cJSON* humi =cJSON_GetObjectItem(root,"humi_thresh");
    if(cJSON_IsNumber(humi)){
        g_humi_threshold=(int32_t)humi->valueint;
        NVS_Write_int32_t("humi_thresh",g_humi_threshold);
    }
    
}