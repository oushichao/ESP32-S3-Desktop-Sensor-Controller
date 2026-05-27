#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lvgl.h"
#include "Device/LCD/LCD_Init.h"
#include "lv_port_display.h"

static const char *TAG = "lvgl_port";

extern esp_lcd_panel_io_handle_t io_handle;
extern SemaphoreHandle_t color_done_sem;

/* 部分刷新模式下，每次绘制的行数。
 * 240(宽) * 40(行) * 2(RGB565) = 19200 字节，单缓冲 < 20KB，双缓冲 < 40KB */
#define DRAW_BUF_ROWS   40

/* RGB565 每像素字节数。LVGL v9.2 传入 flush 的 px_map 已是 RGB565 字节流，
 * 不要用 sizeof(lv_color_t)（在 v9 里是 3 字节结构体，会算错） */
#define BYTES_PER_PX    2

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    size_t w = area->x2 - area->x1 + 1;
    size_t h = area->y2 - area->y1 + 1;
    size_t px_count = w * h;
    size_t bytes = px_count * BYTES_PER_PX;

    uint8_t x_cmd[4] = { (uint8_t)(area->x1 >> 8), (uint8_t)(area->x1 & 0xFF),
                         (uint8_t)(area->x2 >> 8), (uint8_t)(area->x2 & 0xFF) };
    uint8_t y_cmd[4] = { (uint8_t)(area->y1 >> 8), (uint8_t)(area->y1 & 0xFF),
                         (uint8_t)(area->y2 >> 8), (uint8_t)(area->y2 & 0xFF) };

    /* 直接在原 buffer 上做就地字节交换：
     * LVGL 渲染输出小端 RGB565，ILI9341 期望大端，需要交换高低字节。
     * 因为绘制缓冲已是 DMA-capable（分配时用了 MALLOC_CAP_DMA），可直接交给 SPI 发送，
     * 省掉一次 malloc/memcpy，提高性能。 */
    for (size_t i = 0; i < px_count; i++) {
        uint8_t tmp = px_map[i * 2];
        px_map[i * 2]     = px_map[i * 2 + 1];
        px_map[i * 2 + 1] = tmp;
    }

    esp_lcd_panel_io_tx_param(io_handle, 0x2A, x_cmd, 4);
    esp_lcd_panel_io_tx_param(io_handle, 0x2B, y_cmd, 4);
    esp_lcd_panel_io_tx_color(io_handle, 0x2C, px_map, bytes);

    /* 等待 DMA 传输完成（由 on_color_trans_done 中断释放信号量） */
    xSemaphoreTake(color_done_sem, portMAX_DELAY);

    lv_display_flush_ready(disp);
}

lv_display_t *lvgl_port_display_init(void)
{
    lv_display_t *disp = lv_display_create(LCD_HOR_RES, LCD_VER_RES);
    if (!disp) {
        ESP_LOGE(TAG, "lv_display_create failed");
        return NULL;
    }

    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    size_t buf_size = LCD_HOR_RES * DRAW_BUF_ROWS * BYTES_PER_PX;
    uint8_t *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    uint8_t *buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "draw buffer alloc failed (buf1=%p buf2=%p, need %u B each)",
                 buf1, buf2, (unsigned)buf_size);
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        return NULL;
    }

    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG, "display ready: %dx%d, draw_buf=%u B x2",
             LCD_HOR_RES, LCD_VER_RES, (unsigned)buf_size);
    return disp;
}
