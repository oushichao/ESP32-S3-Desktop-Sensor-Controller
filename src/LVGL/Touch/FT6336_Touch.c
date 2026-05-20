#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "FT6336_Touch.h"
#include "Device/LCD/LCD_Init.h"

/* ============= FT6336 寄存器定义 ============= */
/* ==================== FT6336 触摸屏寄存器地址定义 ==================== */
// 0x01 寄存器：手势状态寄存器 - 存储触摸手势（如滑动、单击、双击等）
#define FT6336_REG_GESTURE          0x01
// 0x02 寄存器：触摸状态寄存器 - 存储当前有效触摸点数（0=无触摸，1=单点，2=双点）
#define FT6336_REG_TD_STATUS        0x02
// 0x03 寄存器：第一个触摸点 X 坐标高8位（包含触摸有效标志位 + X坐标高位数据）
#define FT6336_REG_P1_XH            0x03
// 0x04 寄存器：第一个触摸点 X 坐标低8位（与高8位组合成完整X坐标）
#define FT6336_REG_P1_XL            0x04
// 0x05 寄存器：第一个触摸点 Y 坐标高8位（Y坐标高位数据）
#define FT6336_REG_P1_YH            0x05
// 0x06 寄存器：第一个触摸点 Y 坐标低8位（与高8位组合成完整Y坐标）
#define FT6336_REG_P1_YL            0x06

static  i2c_master_bus_handle_t i2c_bus_handle=NULL;
static  i2c_master_dev_handle_t i2c_dev_handle=NULL;
static SemaphoreHandle_t touch_sem=NULL;
static gpio_isr_t isr_handler=NULL;

void FT6336_Touch_Init(){
    touch_sem=xSemaphoreCreateBinary();
    if(!touch_sem)return ;

    //初始化i2c总线
    i2c_master_bus_config_t bus_cfg={
        .scl_io_num=                    TOUCH_I2C_SCL,
        .sda_io_num=                    TOUCH_I2C_SDA,
        .i2c_port=                      TOUCH_I2C_PORT,
        .clk_source=                    I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt= 7,
        .flags.enable_internal_pullup=  true             
    };
    i2c_new_master_bus(&bus_cfg,&i2c_bus_handle);

    //添加设备
    i2c_device_config_t dev_cfg={
        .dev_addr_length=   I2C_ADDR_BIT_LEN_7,
        .device_address=    FT6336_ADDR,
        .scl_speed_hz=      40000
    };
    i2c_master_bus_add_device(i2c_bus_handle,&dev_cfg,&i2c_dev_handle);

    //配置中断引脚
    gpio_config_t   int_cfg={
        .mode=              GPIO_MODE_INPUT,
        .pin_bit_mask=      (1ULL<<TOUCH_INT),
        .pull_up_en=        GPIO_PULLUP_ENABLE,
        .pull_down_en=      GPIO_PULLDOWN_DISABLE,
        .intr_type=         GPIO_INTR_NEGEDGE
    };
    gpio_config(&int_cfg);

    //注册ISR
    gpio_install_isr_service(0);        //不分配额外RAM,安装 GPIO 中断服务程序
    gpio_isr_handler_add(TOUCH_INT,isr_handler,NULL);//指定 GPIO 引脚 添加中断处理函数（绑定中断回调）

    //探测
    uint8_t status=0;
    uint8_t reg=FT6336_REG_TD_STATUS;
    i2c_master_transmit_receive(
        i2c_dev_handle,   // ① I2C设备句柄（你的FT6336遥控器）
        &reg,             // ② 要读取的寄存器地址：FT6336_REG_TD_STATUS (0x02)
        1,                // ③ 发送的地址长度：1个字节
        &status,          // ④ 读取到的数据存到这个变量里
        1,                // ⑤ 读取的数据长度：1个字节
        100               // ⑥ 超时时间：100ms（超时就放弃）
    );
}

void FT6336_Touch_Read(){

}