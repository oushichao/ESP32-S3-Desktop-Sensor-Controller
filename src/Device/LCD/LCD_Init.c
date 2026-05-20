#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"


#include "Lvgl/Display/lv_port_display.h"
#include "LCD_Init.h"

static bool Ledc_Backlight_Init_State=false;
esp_lcd_panel_handle_t panel_handler=NULL;

void Backlight_Pwm_Init(){
    //定时器初始化
    ledc_timer_config_t ledc_timer={
        .clk_cfg=           LEDC_AUTO_CLK,
        .deconfigure=       false,
        .duty_resolution=   LEDC_TIMER_10_BIT,  //0~1023
        .freq_hz=           5000,               //符合人眼的刷新率
        .speed_mode=        LEDC_LOW_SPEED_MODE,
        .timer_num=         LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);
    //pwm初始化
    ledc_channel_config_t ledc_channel={
        .channel=           LEDC_CHANNEL_0,
        .duty=              1023,
        .gpio_num=          LCD_BL,
        .hpoint=            0,
        .timer_sel=         LEDC_TIMER_0,
        .speed_mode=        LEDC_LOW_SPEED_MODE
    };
    ledc_channel_config(&ledc_channel);
    Ledc_Backlight_Init_State=true;
}

void ST7789_Backlight_Set(uint8_t Backlight){
    if(!Ledc_Backlight_Init_State)return ;
    if(Backlight>100)Backlight=100;

    uint32_t duty=(uint32_t)Backlight*1023/100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
}

//ST7789初始化
void ST7789_Init(){
    //SPI总线初始化
    spi_bus_config_t bus_cfg={
        .mosi_io_num=       LCD_SPI_MOSI,
        .miso_io_num=       LCD_SPI_MISO,
        .sclk_io_num=       LCD_SPI_CLK,
        .max_transfer_sz=   LCD_HOR_RES*LCD_VER_RES*2+10,
        .quadwp_io_num=     GPIO_NUM_NC,
        .quadhd_io_num=     GPIO_NUM_NC
    };
    spi_bus_initialize(LCD_SPI_HOST,&bus_cfg,SPI_DMA_CH_AUTO);

    //创建panel IO(SPI接口适配器)
    //(通用面板操作：画点、关屏、旋转)
    esp_lcd_panel_io_spi_config_t io_cfg={
        .cs_gpio_num=LCD_SPI_CS,
        //高电平 = 后面发的字节是像素数据；低电平 = 后面发的字节是 LCD 寄存器命令
        .dc_gpio_num=LCD_SPI_DC,
        .pclk_hz=40*1000*1000,
        .trans_queue_depth=10,
        //命令长度。ST7789 寄存器地址固定 8 bit
        .lcd_cmd_bits=8,
        //参数长度。ST7789 寄存器值固定 8 bit
        .lcd_param_bits=8,
        //SPI 模式 0：时钟空闲低电平、上升沿采样。芯片手册规定
        .spi_mode=0,
        .user_ctx=NULL,
        //颜色传输完成回调。NULL = 不需要通知
        .on_color_trans_done=NULL
    };
    esp_lcd_panel_io_handle_t  io_handler=NULL;
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,&io_cfg,&io_handler);

    //创建st7789 panel
    //ST7789 初始化、分辨率、颜色格式
    esp_lcd_panel_dev_config_t  panel_cfg={
        .reset_gpio_num=LCD_SPI_RST,
        .rgb_ele_order=LCD_RGB_ELEMENT_ORDER_RGB,//RGB 字节顺序
        .bits_per_pixel=16,
        .vendor_config=NULL
    };

    esp_lcd_new_panel_st7789(io_handler,&panel_cfg,&panel_handler);



    //复位＋初始化
    esp_lcd_panel_reset(panel_handler);
    esp_lcd_panel_init(panel_handler);
    esp_lcd_panel_invert_color(panel_handler, true);

    //设置旋转方向，竖屏
    esp_lcd_panel_swap_xy(panel_handler,false); //默认xy轴方向
    //param-2 左右镜像,  param-3 上下镜像
    esp_lcd_panel_mirror(panel_handler,true,false);

    //开启显示
    esp_lcd_panel_disp_on_off(panel_handler,true);

    //初始化背光
    Backlight_Pwm_Init();
    ST7789_Backlight_Set(80);
}