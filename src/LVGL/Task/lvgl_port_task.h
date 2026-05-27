#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* LVGL 互斥锁。在 LVGL 任务以外的地方操作 LVGL API（创建/修改控件等）时，
 * 必须先 lvgl_port_lock()，操作完调 lvgl_port_unlock()。 */
bool lvgl_port_lock(uint32_t timeout_ms);
void lvgl_port_unlock(void);

/* 启动 LVGL：初始化 lv_init、注册 tick、创建 LVGL 任务。
 * 内部会调用 lvgl_port_display_init()。
 * 返回 true 表示成功。 */
bool lvgl_port_start(void);


