#pragma once
#include <stdio.h>

#define OTA_DOWNLOAD_BIT    BIT2

/* ===== OTA 下载进度 ===== */
typedef struct {
    uint8_t  percent;       /* 0–100 */
    char status[32];    
} ota_progress_t;
/* "Checking..." 
/ "Downloading..." 
/ "Verifying..." 
/ "Update complete" 
/ "No update available" 
/ "Download failed" */

extern QueueHandle_t ota_progress_queue;

void Ota_Task_Init();       /* app_main 初始化阶段调用一次 */
const char* Get_App_Version();
void Set_App_Vaild(bool vaild);
void OTA_Rollback_Check();
void Ota_Send_Progress(uint8_t percent, const char *status);
