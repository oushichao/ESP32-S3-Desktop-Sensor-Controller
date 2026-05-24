#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "lvgl.h"
#include "Device/LCD/LCD_Init.h"
#include "lv_port_display.h"

static const char *TAG = "lvgl_port";

extern esp_lcd_panel_io_handle_t io_handle;

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint8_t x_cmd[4] = { (area->x1 >> 8), (area->x1 & 0xFF),
                         (area->x2 >> 8), (area->x2 & 0xFF) };
    uint8_t y_cmd[4] = { (area->y1 >> 8), (area->y1 & 0xFF),
                         (area->y2 >> 8), (area->y2 & 0xFF) };
    esp_lcd_panel_io_tx_param(io_handle, 0x2A, x_cmd, 4);
    esp_lcd_panel_io_tx_param(io_handle, 0x2B, y_cmd, 4);
    esp_lcd_panel_io_tx_param(io_handle, 0x2C, px_map,
        (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1) * sizeof(lv_color_t));
    lv_display_flush_ready(disp);
}

lv_display_t *lvgl_port_display_init(void)
{
    lv_display_t *disp = lv_display_create(LCD_HOR_RES, LCD_VER_RES);
    if (!disp) { ESP_LOGE(TAG, "create display failed"); return NULL; }

    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    static uint8_t buf1[LCD_HOR_RES * 32 * sizeof(lv_color_t)];
    static uint8_t buf2[LCD_HOR_RES * 32 * sizeof(lv_color_t)];
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG, "display ready");
    return disp;
}
