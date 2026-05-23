#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

#include "Weather_Parse.h"

static const char* TAG="Weather_Parse"; 
/*
    {
    "code": "200",
    "now": {
        "temp": "25",
        "feelsLike": "23",
        "text": "晴",
        "icon": "100",
        "windDir": "南风",
        "windScale": "2",
        "windSpeed": "8",
        "humidity": "60",
        "precip": "0.0",
        "pressure": "1010",
        "vis": "20",
        "cloud": "10",
        "dew": "16"
        }
    }
*/
bool Weather_Parse_Now(const char* json_str,char* out,size_t out_len){
    if(!json_str||!out||out_len==0)return false;
    //解析json,返回cjson树
    cJSON* root=cJSON_Parse(json_str);
    if(!root){
        ESP_LOGE(TAG,"JSON PARSE FAIL!!!");
        cJSON_Delete(root);
        return false;
    }

    //检查code
    cJSON* code =cJSON_GetObjectItem(root,"code");
    if(!code||strcmp(code->valuestring,"200")!=0||!cJSON_IsString(code)){
        ESP_LOGE(TAG,"API RETURN FAIL!!!");
        cJSON_Delete(root);
        return false;
    }

    //定位对象
    // cJSON* now=cJSON_GetObjectItem

    cJSON_Delete(root);
    return true;
}