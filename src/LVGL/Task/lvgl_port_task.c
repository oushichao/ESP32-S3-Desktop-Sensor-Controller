#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "Lvgl/Display/lv_port_display.h"
#include "lvgl_port_task.h"

static const char *TAG = "lvgl_task";

/* LVGL 不是线程安全的，所有 LVGL API 调用都必须在持有此锁的情况下进行 */
static SemaphoreHandle_t lvgl_mutex = NULL;

#define LVGL_TASK_STACK_SIZE    (8 * 1024)
#define LVGL_TASK_PRIORITY      5
#define LVGL_TASK_CORE          1            /* 跑在 Core 1，避开 WiFi/系统主任务 */
#define LVGL_TASK_MAX_DELAY_MS  30
#define LVGL_TASK_MIN_DELAY_MS  1

/* LVGL tick 回调：LVGL 内部会调用它来获取毫秒数。
 * esp_timer_get_time() 返回微秒，单调递增，从启动开始计时。 */
static uint32_t lvgl_tick_get_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

bool lvgl_port_lock(uint32_t timeout_ms)
{
    if (!lvgl_mutex) return false;
    TickType_t to = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mutex, to) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    if (!lvgl_mutex) return;
    xSemaphoreGiveRecursive(lvgl_mutex);
}

static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started on core %d", xPortGetCoreID());
    uint32_t delay_ms = LVGL_TASK_MAX_DELAY_MS;

    while (1) {
        if (lvgl_port_lock(0)) {
            delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }
        if (delay_ms > LVGL_TASK_MAX_DELAY_MS) delay_ms = LVGL_TASK_MAX_DELAY_MS;
        if (delay_ms < LVGL_TASK_MIN_DELAY_MS) delay_ms = LVGL_TASK_MIN_DELAY_MS;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

bool lvgl_port_start(void)
{
    /* 1. 创建互斥锁（递归类型，允许同一任务多次加锁） */
    lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    if (!lvgl_mutex) {
        ESP_LOGE(TAG, "create mutex failed");
        return false;
    }

    /* 2. 初始化 LVGL 核心 */
    lv_init();

    /* 3. 注册 tick 回调（LVGL v9 推荐方式，无需额外定时器） */
    lv_tick_set_cb(lvgl_tick_get_cb);

    /* 4. 初始化显示驱动 */
    if (lvgl_port_display_init() == NULL) {
        ESP_LOGE(TAG, "display init failed");
        return false;
    }

    /* 5. 创建 LVGL 任务，绑定到 Core 1 */
    BaseType_t r = xTaskCreatePinnedToCore(
        lvgl_task, "lvgl",
        LVGL_TASK_STACK_SIZE, NULL,
        LVGL_TASK_PRIORITY, NULL,
        LVGL_TASK_CORE);
    if (r != pdPASS) {
        ESP_LOGE(TAG, "create lvgl task failed");
        return false;
    }

    ESP_LOGI(TAG, "lvgl port started");
    return true;
}
