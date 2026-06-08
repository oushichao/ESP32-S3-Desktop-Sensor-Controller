#pragma once
#include <stdio.h>

#define OTA_DOWNLOAD_BIT    BIT2

/* ===== OTA 下载进度 ===== */
typedef struct {
    uint8_t  percent;       /* 0–100 */
    char status[32];    /* "Checking..." / "Downloading..." / "Verifying..." / "Update complete" / "No update available" / "Download failed" */
} ota_progress_t;

extern QueueHandle_t ota_progress_queue;

void Ota_Task_Init();       /* app_main 初始化阶段调用一次 */
const char* Get_App_Version();
void Set_App_Vaild(bool vaild);
void OTA_Rollback_Check();
void Ota_Send_Progress(int percent, const char *status);
