#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

#include "lv_port_display.h"
#include "lvgl.h"
#include "Device/LCD/LCD_Init.h" 

static const char *TAG = "lvgl_port";

extern esp_lcd_panel_handle_t panel_handler;   

#define DRAW_BUF_ROWS 32

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map){
    esp_lcd_panel_draw_bitmap(panel_handler,   
        area->x1, area->y1,
        area->x2 + 1, area->y2 + 1,
        (const void *)px_map);
    lv_display_flush_ready(disp);
}

lv_display_t *lvgl_port_display_init(esp_lcd_panel_handle_t panel){
    if (panel == NULL) {
        ESP_LOGE(TAG, "panel is NULL");
        return NULL;
    }

    lv_display_t *disp = lv_display_create(LCD_HOR_RES, LCD_VER_RES);
    if (disp == NULL) {
        ESP_LOGE(TAG, "lv_display_create failed");
        return NULL;
    }

    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    static uint8_t buf1[240 * DRAW_BUF_ROWS * sizeof(lv_color_t)];
    static uint8_t buf2[240 * DRAW_BUF_ROWS * sizeof(lv_color_t)];

    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG, "LVGL display ready");
    return disp;
}
