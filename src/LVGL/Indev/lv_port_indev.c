#include "lvgl.h"
#include "Lvgl/Touch/FT6336_Touch.h"
#include "esp_log.h"

//LVGL定期调用
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint16_t last_x = 0, last_y = 0;   // ← 记住上一次坐标
    uint16_t x = 0, y = 0;
    bool pressed = false;

    FT6336_Touch_Read(&x, &y, &pressed);

    if (pressed) {
        last_x = x;
        last_y = y;
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI("INDEV", "x=%u y=%u", x, y);
    } else {
        /* 释放时坐标必须保持上一次的值，不能给 (0,0) */
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}


void lv_port_indev_init(void){
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
}
// main() 主循环
//   └─ lv_timer_handler()                     ← 每 5ms 调一次
//        └─ LVGL 内部时钟
//             └─ 检查输入设备定时器（30ms 间隔）
//                  └─ 遍历所有 lv_indev_t
//                       └─ 调用 touch_read_cb(indev, &data)   ← 你的回调
//                            └─ FT6336_Touch_Read → I2C 读芯片 → 取坐标
//                            └─ data.point.x = ...   ← 你填
//                            └─ data.state = ...
//                       └─ LVGL 拿 data 去匹配控件
//                            └─ 坐标落在按钮上 → 触发 LV_EVENT_CLICKED
//                            └─ 坐标在 slider 上拖动 → 触发 LV_EVENT_VALUE_CHANGED

