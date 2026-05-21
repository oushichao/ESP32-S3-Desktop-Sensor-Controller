#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <stdio.h>   
#include "esp_log.h"

#include "Ntp_Time.h"

static const char *TAG = "Ntp_Time";

void NTP_Init(){
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);//轮询模式
    //阿里云NTP服务器
    esp_sntp_setservername(0,"ntp.aliyun.com");
    esp_sntp_setservername(1,"ntp.aliyun.com");//备用
    esp_sntp_init();
    esp_sntp_set_sync_interval(3600*1000);
    for(int i=0;i<10;i++){
        if(sntp_get_sync_status()==SNTP_SYNC_STATUS_COMPLETED)return ;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG,"CONNECT FAIL!!!");
}

void Get_Time_Str(char* buf,size_t buflen){ //获取当前时间
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now,&timeinfo);
    if (timeinfo.tm_year < (2025 - 1900)) {
        snprintf(buf, buflen, "--:--:--");
        return;
    }
    strftime(buf,buflen,"%Y-%m-%d %H:%M:%S",&timeinfo);
}