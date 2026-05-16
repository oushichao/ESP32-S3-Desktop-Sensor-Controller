#include "lvgl.h"
void LVGL_Init(){
    lv_init();
    lv_obj_t * sw = lv_switch_create(lv_screen_active());
    lv_obj_center(sw);
}