#pragma once
#include <stdio.h>

#define OTA_DOWNLOAD_BIT    BIT2

/* ===== OTA 下载进度 ===== */
typedef struct {
    uint8_t  percent;       /* 0–100 */
    char status[32];    /* "Checking..." / "Downloading..." / "Verifying..." / "Update complete" / "No update available" / "Download failed" */
} ota_progress_t;

extern QueueHandle_t ota_progress_queue;

void ota_task_init(void);       /* app_main 初始化阶段调用一次 */
void ota_trigger_start(void);   /* 按钮回调中调用 */


const char* Get_App_Version();
void Set_App_Vaild(int vaild);
void OTA_Rollback_Check(void);
