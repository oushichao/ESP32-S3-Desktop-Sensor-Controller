#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include <stdbool.h>
#include <stdio.h>
#include "UI_main.h"
#include "UI_data.h"
 
static const char* TAG="UI_TEST";

extern int32_t g_temp_threshold;
extern int32_t g_humi_threshold;
extern float   g_temperature;
extern float   g_humidity;
extern int32_t g_light;
extern bool    g_relay_state;

/* ===== 辅助函数 ===== */
void set_led_status(lv_obj_t *obj, bool status){
    if (status)
        lv_led_set_color(obj, lv_color_hex(0x00FF00));
    else
        lv_led_set_color(obj, lv_color_hex(0xFF0000));
}

/* ===== 回调 ===== */
static void slider_value_tem(lv_event_t *e){
    g_temp_threshold = lv_slider_get_value(lv_event_get_target(e));
}

static void slider_value_hum(lv_event_t *e){
    g_humi_threshold = lv_slider_get_value(lv_event_get_target(e));
}

static void slider_value_light(lv_event_t *e){
    g_light = lv_slider_get_value(lv_event_get_target(e));
}

static void switch_value_rel(lv_event_t *e){
    g_relay_state = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
}

static void button_check_update(lv_event_t *e){
    // OTA
}

/* ===== UI 入口 ===== */
void UI_init(void){
    lv_obj_t *tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_BOTTOM);

    lv_obj_t *tab_home    = lv_tabview_add_tab(tabview, "Home");
    lv_obj_t *tab_chart   = lv_tabview_add_tab(tabview, "Chart");
    lv_obj_t *tab_setting = lv_tabview_add_tab(tabview, "Setting");

    /* ---- Home 页 ---- */
    lv_obj_t *label_wifi = lv_label_create(tab_home);
    lv_label_set_text(label_wifi, "WIFI");
    lv_obj_set_pos(label_wifi, 10, 10);

    lv_obj_t *led_wifi = lv_led_create(tab_home);
    lv_obj_set_size(led_wifi, 10, 10);
    lv_obj_set_pos(led_wifi, 55, 10);
    set_led_status(led_wifi, false);
    lv_led_on(led_wifi);

    lv_obj_t *label_mqtt = lv_label_create(tab_home);
    lv_label_set_text(label_mqtt, "MQTT");
    lv_obj_set_pos(label_mqtt, 120, 10);

    lv_obj_t *led_mqtt = lv_led_create(tab_home);
    lv_obj_set_size(led_mqtt, 10, 10);
    lv_obj_set_pos(led_mqtt, 170, 10);
    set_led_status(led_mqtt, false);
    lv_led_on(led_mqtt);

    // 时间
    lv_obj_t *label_timer = lv_label_create(tab_home);
    lv_label_set_text(label_timer, "Current-Timer:");
    lv_obj_set_pos(label_timer, 10, 50);
    char buf[32];
    snprintf(buf, 32, "%d", 11);
    lv_obj_t *current_timer = lv_label_create(tab_home);
    lv_label_set_text(current_timer, buf);
    lv_obj_set_pos(current_timer, 125, 50);

    // 温度
    lv_obj_t *label_temp = lv_label_create(tab_home);
    lv_label_set_text(label_temp, "Temp:");
    lv_obj_set_pos(label_temp, 10, 90);
    lv_obj_t *current_temp = lv_label_create(tab_home);
    lv_label_set_text(current_temp, buf);
    lv_obj_set_pos(current_temp, 60, 90);

    // 湿度
    lv_obj_t *label_humi = lv_label_create(tab_home);
    lv_label_set_text(label_humi, "Humi:");
    lv_obj_set_pos(label_humi, 10, 130);
    lv_obj_t *current_humi = lv_label_create(tab_home);
    lv_label_set_text(current_humi, buf);
    lv_obj_set_pos(current_humi, 60, 130);

    // 光照
    lv_obj_t *label_light = lv_label_create(tab_home);
    lv_label_set_text(label_light, "Light:");
    lv_obj_set_pos(label_light, 10, 170);
    lv_obj_t *current_light = lv_label_create(tab_home);
    lv_label_set_text(current_light, buf);
    lv_obj_set_pos(current_light, 60, 170);

    // 继电器
    lv_obj_t *label_relay = lv_label_create(tab_home);
    lv_label_set_text(label_relay, "Relay:");
    lv_obj_set_pos(label_relay, 10, 210);
    lv_obj_t *current_relay = lv_label_create(tab_home);
    snprintf(buf, 32, "%s", g_relay_state ? "True" : "False");
    lv_label_set_text(current_relay, buf);
    lv_obj_set_pos(current_relay, 60, 210);
ESP_LOGI(TAG,"%d",__LINE__);
    /* ---- Chart 页 ---- */
    int32_t fake_temp[30] = {
        -20, -15, -10, -5, 0, 5, 10, 15,
         25,  35,  45, 55, 60, 55, 45, 35,
         20,  10,   0, -10, -15, -10, 5, 15,
         25,  35,  40, 30, 20, 10
    };

    lv_obj_t *chart_temp = lv_chart_create(tab_chart);
    lv_obj_set_pos(chart_temp, 10, 0);
    lv_obj_set_size(chart_temp, 200, 60);
    lv_chart_set_type(chart_temp, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart_temp, 30);
    lv_chart_set_range(chart_temp, LV_CHART_AXIS_PRIMARY_Y, -20, 60);
    lv_chart_series_t *ser = lv_chart_add_series(chart_temp,
        lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    for (int j = 0; j < 30; j++)
        lv_chart_set_next_value(chart_temp, ser, fake_temp[j]);
    lv_chart_refresh(chart_temp);
ESP_LOGI(TAG,"%d",__LINE__);
    lv_obj_t *temp_x = lv_label_create(tab_chart);
    lv_label_set_text(temp_x, "Temp/Time");
    lv_obj_align_to(temp_x, chart_temp, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
ESP_LOGI(TAG,"%d",__LINE__);
    lv_obj_t *chart_humi = lv_chart_create(tab_chart);
    lv_obj_set_pos(chart_humi, 10, 85);
    lv_obj_set_size(chart_humi, 200, 60);
    lv_chart_set_type(chart_humi, LV_CHART_TYPE_LINE);
    lv_obj_t *humi_x = lv_label_create(tab_chart);
    lv_label_set_text(humi_x, "Humi/Time");
    lv_obj_align_to(humi_x, chart_humi, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
ESP_LOGI(TAG,"%d",__LINE__);
    lv_obj_t *chart_light = lv_chart_create(tab_chart);
    lv_obj_set_pos(chart_light, 10, 170);
    lv_obj_set_size(chart_light, 200, 60);
    lv_chart_set_type(chart_light, LV_CHART_TYPE_LINE);
    lv_obj_t *light_x = lv_label_create(tab_chart);
    lv_label_set_text(light_x, "Light/Time");
    lv_obj_align_to(light_x, chart_light, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
ESP_LOGI(TAG,"%d",__LINE__);
    /* ---- Setting 页 ---- */
    // 温度阈值
    lv_obj_t *temp_limit = lv_slider_create(tab_setting);
    lv_obj_set_pos(temp_limit, 5, 25);
    lv_obj_set_size(temp_limit, 200, 15);
    lv_slider_set_range(temp_limit, -40, 125);
    lv_slider_set_value(temp_limit, g_temp_threshold, LV_ANIM_OFF);
    lv_obj_t *temp_limit_label = lv_label_create(tab_setting);
    lv_obj_set_pos(temp_limit_label, 0, 0);
    lv_label_set_text(temp_limit_label, "Tem_Threshold:");
ESP_LOGI(TAG,"%d",__LINE__);
    snprintf(buf, 32, "%ld", g_temp_threshold);
    lv_obj_t *tem_value_label = lv_label_create(tab_setting);
    lv_obj_set_pos(tem_value_label, 125, 0);
    lv_label_set_text(tem_value_label, buf);
ESP_LOGI(TAG,"%d",__LINE__);
    // 湿度阈值
    lv_obj_t *humi_limit = lv_slider_create(tab_setting);
    lv_obj_set_pos(humi_limit, 5, 70);
    lv_obj_set_size(humi_limit, 200, 15);
    lv_slider_set_range(humi_limit, 0, 100);
    lv_slider_set_value(humi_limit, g_humi_threshold, LV_ANIM_OFF);
    lv_obj_t *humi_limit_label = lv_label_create(tab_setting);
    lv_obj_set_pos(humi_limit_label, 0, 50);
    lv_label_set_text(humi_limit_label, "Hum_Threshold:");
ESP_LOGI(TAG,"%d",__LINE__);;
    snprintf(buf, 32, "%ld", g_humi_threshold);
    lv_obj_t *hum_value_label = lv_label_create(tab_setting);
    lv_obj_set_pos(hum_value_label, 125, 50);
    lv_label_set_text(hum_value_label, buf);
ESP_LOGI(TAG,"%d",__LINE__);
    // 继电器
    lv_obj_t *relay_status_obj = lv_switch_create(tab_setting);
    lv_obj_set_pos(relay_status_obj, 110, 100);
    lv_obj_set_size(relay_status_obj, 100, 20);
    lv_obj_t *relay_label = lv_label_create(tab_setting);
    lv_obj_set_pos(relay_label, 0, 100);
    lv_label_set_text(relay_label, "Relay_Status:");
ESP_LOGI(TAG,"%d",__LINE__);
    // 背光
    lv_obj_t *back_light = lv_slider_create(tab_setting);
    lv_obj_set_pos(back_light, 5, 175);
    lv_obj_set_size(back_light, 200, 15);
    lv_slider_set_range(back_light, 0, 100);
    lv_slider_set_value(back_light, g_light, LV_ANIM_OFF);
    lv_obj_t *back_label = lv_label_create(tab_setting);
    lv_obj_set_pos(back_label, 0, 150);
    lv_label_set_text(back_label, "Back_Light:");
ESP_LOGI(TAG,"%d",__LINE__);
    snprintf(buf, 32, "%ld", g_light);
    lv_obj_t *back_value_label = lv_label_create(tab_setting);
    lv_obj_set_pos(back_value_label, 100, 150);
    lv_label_set_text(back_value_label, buf);
ESP_LOGI(TAG,"%d",__LINE__);
    // OTA
    lv_obj_t *ota = lv_btn_create(tab_setting);
    lv_obj_set_pos(ota, 130, 210);
    lv_obj_set_size(ota, 40, 40);
    lv_obj_set_style_radius(ota, 40, 0);
    lv_obj_t *ota_label = lv_label_create(tab_setting);
    lv_obj_set_pos(ota_label, 0, 220);
    lv_label_set_text(ota_label, "Check Update:");
ESP_LOGI(TAG,"%d",__LINE__);
    /* 绑定回调 */
    lv_obj_add_event_cb(temp_limit,       slider_value_tem,   LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(humi_limit,       slider_value_hum,   LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(back_light,       slider_value_light, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(relay_status_obj, switch_value_rel,   LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ota,              button_check_update, LV_EVENT_CLICKED, NULL);
}
