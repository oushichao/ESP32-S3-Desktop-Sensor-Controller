#include <stdio.h>
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_http_client.h"

#include "UI/UI_data.h"
#include "OTA.h"
#include "IOT/WIFI_Init/WIFI_Init.h"
#include "config.h"

static const char *TAG = "OTA";

extern EventGroupHandle_t wifi_ev;
QueueHandle_t ota_progress_queue = NULL;

/**
 * @brief   获取当前的版本号，可通过cmakelist修改版本号
 * @return  返回当前的版本号
 */
const char* Get_App_Version(){
    static char app_version[32]={0};
    if(app_version[0]==0){
        //获取当前运行分区的信息,比如ota0或者ota1
        const esp_partition_t* running = esp_ota_get_running_partition();
        esp_app_desc_t app_desc;
        esp_ota_get_partition_description(running, &app_desc);
        snprintf(app_version, sizeof(app_version), "%s", app_desc.version);
    }
    return app_version;
}
/**
 * @brief   执行当前分区的操作,可用于扩展后续的功能
 * @param   vaild 1表示自检通过 固件有效 0表示自检失败 触发回滚
 */
void Set_App_Vaild(bool vaild){
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;     //存放当前分区的OTA状态
    if(esp_ota_get_state_partition(running, &ota_state)==ESP_OK){
        if(ota_state==ESP_OTA_IMG_PENDING_VERIFY){                  //待验证状态
            if(vaild){
                ESP_LOGI(TAG, "自检通过，标记固件有效，取消回滚");
                //otdata中的状态改为valid,取消回滚
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "自检失败，回滚到上一个固件并重启！");
                //当前分区标记位invaild，otadata指向旧固件分区同时重启
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        } else {
            ESP_LOGI(TAG, "非待验证状态(%d)，无需处理", ota_state);
        }
    }
}
/**
 * @brief   检查分区的状态
 */
void OTA_Rollback_Check(){
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;

    // 不是待验证状态（如普通启动），直接返回
    if(esp_ota_get_state_partition(running, &ota_state)!=ESP_OK
       || ota_state != ESP_OTA_IMG_PENDING_VERIFY){
        return;
    }

    ESP_LOGI(TAG, "检测到待验证的新固件，开始自检...");

    // 自检：等 WiFi 连上，10 秒超时
    EventBits_t bits = xEventGroupWaitBits(
        wifi_ev, WIFI_CONNECTED_BIT,
        pdFALSE, pdTRUE, pdMS_TO_TICKS(10000));

    Set_App_Vaild((bits & WIFI_CONNECTED_BIT) ? 1 : 0);
}

/* ==================== OTA 下载任务 ==================== */
/**
 * @brief   覆盖式的队列用于传递当前下载进度
 */
void Ota_Task_Init(){
    ota_progress_queue = xQueueCreate(2, sizeof(ota_progress_t));
}

/**
 * @brief   发送下载进度
 * @param percent   下载百分比
 * @param statue    状态文本
 */
void Ota_Send_Progress(int percent, const char *status){
    ota_progress_t p;
    p.percent = percent;
    strncpy(p.status, status, sizeof(p.status) - 1);
    p.status[sizeof(p.status) - 1] = '\0';
    xQueueOverwrite(ota_progress_queue, &p);    //队列满的话直接覆盖
}

static void ota_download_task(){
    Ota_Send_Progress(0, "Checking...");

    /* 等 WiFi（5 秒超时） */
    EventBits_t bits = xEventGroupWaitBits(
        wifi_ev, WIFI_CONNECTED_BIT,
        pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
    if (!(bits & WIFI_CONNECTED_BIT)) {
        Ota_Send_Progress(0, "WiFi not connected");
        vTaskDelete(NULL);
        return;
    }

    /* 配置 HTTP 客户端 */
    esp_http_client_config_t http_cfg = {
        .url = OTA_FW_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 15000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        Ota_Send_Progress(0, "No update available");
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }

    int content_len = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200 || content_len <= 0) {
        Ota_Send_Progress(0, "No update available");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }

    const esp_partition_t *update_part = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t ota_handle;
    err = esp_ota_begin(update_part, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        Ota_Send_Progress(0, "OTA init failed");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }

    int total_read = 0;
    uint8_t buf[1024];
    int last_pct = -1;

    while (total_read < content_len) {
        int rlen = esp_http_client_read(client, (char *)buf, sizeof(buf));
        if (rlen <= 0) break;
        esp_ota_write(ota_handle, buf, rlen);
        total_read += rlen;

        int pct = (int)((long long)total_read * 100 / content_len);
        if (pct != last_pct) {
            last_pct = pct;
            Ota_Send_Progress(pct, "Downloading...");
        }

        vTaskDelay(1);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        Ota_Send_Progress(0, "Verification failed");
        vTaskDelete(NULL);
        return;
    }

    Ota_Send_Progress(100, "Verifying...");
    vTaskDelay(pdMS_TO_TICKS(500));

    err = esp_ota_set_boot_partition(update_part);
    if (err != ESP_OK) {
        Ota_Send_Progress(0, "Boot partition fail");
        vTaskDelete(NULL);
        return;
    }

    Ota_Send_Progress(100, "Update complete");
    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_restart();
}

