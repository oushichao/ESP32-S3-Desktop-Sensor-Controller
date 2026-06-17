#include "nvs_flash.h"
#include "esp_log.h"

#include "NVS.h"
#include "UI/UI_data.h"
#include "config.h"
#include "IOT/WIFI_Init/WIFI_Init.h"

static const char* TAG ="NVS";


void NVS_Write_Str(char* key,char* str){
    nvs_handle  handle;
    if(!str||!key)return;
    if(nvs_open("config",NVS_READWRITE,&handle)!=ESP_OK){
        ESP_LOGE(TAG,"[%s]nvs open fail!!!",__func__);
        return ;
    }

    nvs_set_str(handle,key,str);

    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG,"[%s]nvs write success!!!",__func__);
}

void NVS_Write_int32_t(char* key,int32_t value){
    nvs_handle  handle;
    if(!key)return;
    if(nvs_open("config",NVS_READWRITE,&handle)!=ESP_OK){
        ESP_LOGE(TAG,"[%s]nvs open fail!!!",__func__);
        return ;
    }

    nvs_set_i32(handle,key,value);

    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG,"[%s]nvs write success!!!",__func__);
}

void NVS_Write_uint16_t(char* key,uint16_t value){
    nvs_handle  handle;
    if(!key)return;
    if(nvs_open("config",NVS_READWRITE,&handle)!=ESP_OK){
        ESP_LOGE(TAG,"[%s]nvs open fail!!!",__func__);
        return ;
    }

    nvs_set_u16(handle,key,value);

    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG,"[%s]nvs write success!!!",__func__);
}

void NVS_Write_uint8_t(char* key,uint8_t value){
    nvs_handle  handle;
    if(!key)return;
    if(nvs_open("config",NVS_READWRITE,&handle)!=ESP_OK){
        ESP_LOGE(TAG,"[%s]nvs open fail!!!",__func__);
        return ;
    }

    nvs_set_u8(handle,key,value);

    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG,"[%s]nvs write success!!!",__func__);
}

void NVS_Read_Config(){
    nvs_handle  handle;
    nvs_flash_init();
    if(nvs_open("config",NVS_READWRITE,&handle)!=ESP_OK){
        ESP_LOGE(TAG,"nvs open fail!!!");
        return ;
    }

    int32_t out_value;
    uint8_t out_value_0;

    if(nvs_get_i32(handle,"temp_thresh",&out_value)==ESP_OK){
        g_temp_threshold=out_value;
    }
    else{
        nvs_set_i32(handle,"temp_thresh",g_temp_threshold);
    }

    if(nvs_get_i32(handle,"humi_thresh",&out_value)==ESP_OK){
        g_humi_threshold=out_value;
    }
    else{
        nvs_set_i32(handle,"humi_thresh",g_humi_threshold);
    }

    if(nvs_get_u8(handle,"bl_level",&out_value_0)==ESP_OK){
        backlight=out_value_0;
    }
    else{
        nvs_set_u8(handle,"bl_level",backlight);
    }

    nvs_commit(handle);
    nvs_close(handle);
}
