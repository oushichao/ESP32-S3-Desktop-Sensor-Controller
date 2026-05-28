#include <stdio.h>
#include "esp_ota_ops.h"

#include "UI/UI_data.h"
#include "OTA.h"

//回滚功能暂未开启！！！！

const char* Get_App_Version(){
    static char app_version[32]={0};
    if(app_version[0]==0){
        //返回当前运行的分区信息
        const esp_partition_t* running= esp_ota_get_running_partition();
        esp_app_desc_t app_desc;
        //根据分区信息获取app_desc_t
        esp_ota_get_partition_description(running,&app_desc);
        snprintf(app_version,sizeof(app_version),"%s",app_desc.version);
    }
    return app_version;
}


void Set_App_Vaild(int vaild){
    const esp_partition_t* running= esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if(esp_ota_get_state_partition(running,&ota_state)==ESP_OK){
        if(ota_state==ESP_OTA_IMG_PENDING_VERIFY){
            if(vaild){
                //取消回滚
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else{
                //回滚重启
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}