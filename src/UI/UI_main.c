#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdlib.h>

#include <stdbool.h>
#include <stdio.h>
#include "UI_main.h"
#include "UI_data.h"
#include "IOT/Get_Weather_Time/Get_Time/Ntp_Time.h"
#include "IOT/Get_Weather_Time/Get_Weather/Weather_Parse.h"
#include "IOT/OTA/OTA.h"


/* ==========  Seeting控制刷新   ========== */
lv_obj_t *label_tem_value   = NULL;
lv_obj_t *label_hum_value   = NULL;
lv_obj_t *label_back_value  = NULL;
lv_obj_t* progress_bar      = NULL;

/*===========  Home控制刷新  ============*/
lv_obj_t *led_wifi          =NULL;
lv_obj_t *led_mqtt          =NULL;
lv_obj_t *current_timer     =NULL;
lv_obj_t *current_temp      =NULL;
lv_obj_t *current_humi      =NULL; 
lv_obj_t *current_light     =NULL;
lv_obj_t *label_home_relay  =NULL;
lv_obj_t *weather           =NULL;

/*===========   Chart控制刷新  ==========*/
lv_obj_t *chart_temp        =NULL;
lv_obj_t *chart_humi        =NULL;
lv_obj_t *chart_light       =NULL;


extern int32_t g_temp_threshold;
extern int32_t g_humi_threshold;
extern float   g_temperature;
extern float   g_humidity;
extern uint16_t g_light;
extern bool    g_relay_state;
extern char    time_str[16];
extern char    current_weather[16];

/* 
    @brief 设置Home页的led显示颜色
    @param obj  操作的对象
    @param status   1:led显示绿色 0:led显示红色
*/
void set_led_status(lv_obj_t *obj, bool status){
    if (status)
        lv_led_set_color(obj, lv_color_hex(0x00FF00));
    else
        lv_led_set_color(obj, lv_color_hex(0xFF0000));
}

// static void slider_debug_cb(lv_event_t *e)
// {
//     lv_obj_t *slider = lv_event_get_current_target(e);
//     lv_event_code_t code = lv_event_get_code(e);
//     int32_t val = lv_slider_get_value(slider);
//     ESP_LOGI("SLIDER", "event=%d val=%ld", (int)code, (long)val);
// }


/* ===== 回调 ===== */
static void slider_released_tem(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_current_target(e);
    g_temp_threshold = (int32_t)lv_slider_get_value(slider);
    if (label_tem_value) {
        lv_label_set_text_fmt(label_tem_value, "%ld", (long)g_temp_threshold);
    }
}

static void slider_released_hum(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    g_humi_threshold = (int32_t)lv_slider_get_value(slider);
    if (label_hum_value) {
        lv_label_set_text_fmt(label_hum_value, "%ld", (long)g_humi_threshold);
    }
}

static void slider_released_light(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    backlight = (uint8_t)lv_slider_get_value(slider);
    if (label_back_value) {
        lv_label_set_text_fmt(label_back_value, "%u", (unsigned)backlight);
    }
}



static void switch_value_rel(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    g_relay_state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (label_home_relay) {
        lv_label_set_text(label_home_relay, g_relay_state ? "True" : "False");
    }
}


static void button_check_update(lv_event_t *e){// OTA
    lv_obj_t* bt=lv_event_get_target(e);
    // 显示进度条
    lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
}

/* ===== UI 入口 ===== */
void UI_init(void){

    lv_obj_t *tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_BOTTOM);


    lv_obj_t *tab_home    = lv_tabview_add_tab(tabview, "Home");
    lv_obj_t *tab_chart   = lv_tabview_add_tab(tabview, "Chart");
    lv_obj_t *tab_setting = lv_tabview_add_tab(tabview, "Setting");


    /* ---- Home 页 ---- */

    //WIFI指示
    lv_obj_t *label_wifi = lv_label_create(tab_home);
    lv_label_set_text(label_wifi, "WIFI");
    lv_obj_set_pos(label_wifi, 10, 10);

    led_wifi = lv_led_create(tab_home);
    lv_obj_set_size(led_wifi, 10, 10);
    lv_obj_set_pos(led_wifi, 70, 15);
    set_led_status(led_wifi, false);
    lv_led_on(led_wifi);

    //mqtt指示
    lv_obj_t *label_mqtt = lv_label_create(tab_home);
    lv_label_set_text(label_mqtt, "MQTT");
    lv_obj_set_pos(label_mqtt, 120, 10);

    led_mqtt = lv_led_create(tab_home);
    lv_obj_set_size(led_mqtt, 10, 10);
    lv_obj_set_pos(led_mqtt, 185, 15);
    set_led_status(led_mqtt, false);
    lv_led_on(led_mqtt);

    // 时间和天气
    Get_Time_Str(time_str,(size_t)32);
    current_timer = lv_label_create(tab_home);
    lv_obj_set_pos(current_timer, 10, 50);
    lv_label_set_text(current_timer,time_str);
    
    weather=lv_label_create(tab_home);
    lv_obj_set_pos(weather, 100, 50);
    lv_label_set_text(weather,current_weather);
    

    // 温度
    lv_obj_t *label_temp = lv_label_create(tab_home);
    lv_label_set_text(label_temp, "Temp:");
    lv_obj_set_pos(label_temp, 10, 90);
    current_temp = lv_label_create(tab_home);

    int temp_int = (int)g_temperature;
    int temp_dec = (int)((g_temperature - temp_int) * 10 + 0.5);
    lv_label_set_text_fmt(current_temp, "%d.%d", temp_int, abs(temp_dec));
    lv_obj_set_pos(current_temp, 80, 90);

    // 湿度
    lv_obj_t *label_humi = lv_label_create(tab_home);
    lv_label_set_text(label_humi, "Humi:");
    lv_obj_set_pos(label_humi, 10, 130);
    current_humi = lv_label_create(tab_home);
    
    int humi_int = (int)g_humidity;
    int humi_dec = (int)((g_humidity - humi_int) * 10 + 0.5);
    lv_label_set_text_fmt(current_humi, "%d.%d", humi_int, abs(humi_dec));
    lv_obj_set_pos(current_humi, 80, 130);

    // 光照
    lv_obj_t *label_light = lv_label_create(tab_home);
    lv_label_set_text(label_light, "Light:");
    lv_obj_set_pos(label_light, 10, 170);
    current_light = lv_label_create(tab_home);

    lv_label_set_text_fmt(current_light, "%u", (unsigned)g_light);
    lv_obj_set_pos(current_light, 80, 170);

    // 继电器
    lv_obj_t *label_relay = lv_label_create(tab_home);
    lv_label_set_text(label_relay, "Relay:");
    lv_obj_set_pos(label_relay, 10, 210);

    label_home_relay = lv_label_create(tab_home);
    lv_label_set_text(label_home_relay, g_relay_state ? "True" : "False");
    lv_obj_set_pos(label_home_relay, 80, 210);


    /* ---- Chart 页 ---- */
    int32_t fake_temp[30] = {
        -20, -15, -10, -5, 0, 5, 10, 15,
         25,  35,  45, 55, 60, 55, 45, 35,
         20,  10,   0, -10, -15, -10, 5, 15,
         25,  35,  40, 30, 20, 10
    };

    //温度曲线
    chart_temp = lv_chart_create(tab_chart);
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

    lv_obj_t *temp_x = lv_label_create(tab_chart);
    lv_label_set_text(temp_x, "Temp/Time");
    lv_obj_align_to(temp_x, chart_temp, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    //湿度曲线
    chart_humi = lv_chart_create(tab_chart);
    lv_obj_set_pos(chart_humi, 10, 85);
    lv_obj_set_size(chart_humi, 200, 60);
    lv_chart_set_type(chart_humi, LV_CHART_TYPE_LINE);
    lv_obj_t *humi_x = lv_label_create(tab_chart);
    lv_label_set_text(humi_x, "Humi/Time");
    lv_obj_align_to(humi_x, chart_humi, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    //光强曲线
    chart_light = lv_chart_create(tab_chart);
    lv_obj_set_pos(chart_light, 10, 170);
    lv_obj_set_size(chart_light, 200, 60);
    lv_chart_set_type(chart_light, LV_CHART_TYPE_LINE);
    lv_obj_t *light_x = lv_label_create(tab_chart);
    lv_label_set_text(light_x, "Light/Time");
    lv_obj_align_to(light_x, chart_light, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

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

    label_tem_value = lv_label_create(tab_setting);
    lv_obj_set_pos(label_tem_value, 170, 0);
    lv_label_set_text_fmt(label_tem_value, "%ld", (long)g_temp_threshold);

    // 湿度阈值
    lv_obj_t *humi_limit = lv_slider_create(tab_setting);
    lv_obj_set_pos(humi_limit, 5, 75);
    lv_obj_set_size(humi_limit, 200, 15);
    lv_slider_set_range(humi_limit, 0, 100);
    lv_slider_set_value(humi_limit, g_humi_threshold, LV_ANIM_OFF);
    lv_obj_t *humi_limit_label = lv_label_create(tab_setting);
    lv_obj_set_pos(humi_limit_label, 0, 50);
    lv_label_set_text(humi_limit_label, "Hum_Threshold:");

    label_hum_value = lv_label_create(tab_setting);
    lv_obj_set_pos(label_hum_value, 170, 50);
    lv_label_set_text_fmt(label_hum_value, "%ld", (long)g_humi_threshold);


    // 继电器
    lv_obj_t *relay_status_obj = lv_switch_create(tab_setting);
    lv_obj_set_pos(relay_status_obj, 135, 105);
    lv_obj_set_size(relay_status_obj, 70, 15);
    lv_obj_t *relay_label = lv_label_create(tab_setting);
    if (g_relay_state) {
        lv_obj_add_state(relay_status_obj, LV_STATE_CHECKED);
    }
    lv_obj_set_pos(relay_label, 0, 100);
    lv_label_set_text(relay_label, "Relay_Status:");

    // 背光
    lv_obj_t *back_light = lv_slider_create(tab_setting);
    lv_obj_set_pos(back_light, 5, 160);
    lv_obj_set_size(back_light, 200, 15);
    lv_slider_set_range(back_light, 0, 100);
    lv_slider_set_value(back_light, backlight, LV_ANIM_OFF);
    lv_obj_t *back_label = lv_label_create(tab_setting);
    lv_obj_set_pos(back_label, 0, 135);
    lv_label_set_text(back_label, "Back_Light:");

    label_back_value = lv_label_create(tab_setting);
    lv_obj_set_pos(label_back_value, 130, 135);
    lv_label_set_text_fmt(label_back_value, "%ld", (long)backlight);


    // OTA
    lv_obj_t *ota = lv_btn_create(tab_setting);
    lv_obj_set_pos(ota, 160, 190);
    lv_obj_set_size(ota, 20, 20);
    lv_obj_set_style_radius(ota, 40, 0);
    lv_obj_t *ota_label = lv_label_create(tab_setting);
    lv_obj_set_pos(ota_label, 0, 190);
    lv_label_set_text(ota_label, "Check Update:");
    //进度条
    progress_bar =lv_bar_create(tab_setting);
    lv_obj_set_pos(progress_bar,0,215);
    lv_obj_set_size(progress_bar,100,20);
    lv_bar_set_range(progress_bar,0,100);
    lv_bar_set_value(progress_bar,0,LV_ANIM_OFF);
    lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);

    /* 绑定回调 */
    lv_obj_add_event_cb(temp_limit,       slider_released_tem,   LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(humi_limit,       slider_released_hum,   LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(back_light,       slider_released_light, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(relay_status_obj, switch_value_rel,      LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ota,              button_check_update, LV_EVENT_CLICKED, NULL);

}
