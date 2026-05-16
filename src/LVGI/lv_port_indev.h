#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===== FT6336 I2C 引脚（按实际接线修改） ===== */
#define TOUCH_I2C_PORT      I2C_NUM_0
#define TOUCH_I2C_SDA       5
#define TOUCH_I2C_SCL       6
#define TOUCH_INT           4       // 不用填 -1
#define TOUCH_RST           3       // 不用填 -1

void lv_port_indev_init(void);

#ifdef __cplusplus
}
#endif
