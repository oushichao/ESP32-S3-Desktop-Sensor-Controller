#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_touch.h"

#include "LCD_Init.h"
#include "UI/UI_data.h"
#include "FT6336_Touch.h"


void Backlight_Pwm_Init();
static lv_disp_t* LCD_Display_Init();
static lv_indev_t* Display_Indev_Init(lv_disp_t* disp);


/*传输层,负责CS，DC，SPI模式，时钟*/
esp_lcd_panel_io_handle_t io_handle=NULL;
/*应用层,开显示,设置窗口，画像素，旋转/镜像*/
esp_lcd_panel_handle_t    panel_handle=NULL;
lv_disp_t* disp         =   NULL; 
static const char* TAG  =   "LEDC_Init";
bool ledc_ok            =   false;     //背光初始化标志
esp_lcd_touch_handle_t touch_handle =   NULL;

/**
 * @brief   显示驱动:ili9341,触摸驱动:FT6336,绑定lvgl的初始化函数
 */
void Lvgl_Start(){
    //初始化lvgl
    lvgl_port_cfg_t lvgl_cfg= ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    // 初始化液晶屏 并添加LVGL接口
    disp=LCD_Display_Init();
    if(!disp){
        ESP_LOGE(TAG,"添加lvgl显示接口失败");
        return;
    }
    else ESP_LOGI(TAG,"添加lvgl显示接口成功!!!");

    //开启背光
    Backlight_Pwm_Init();
    Backlight_Set(backlight);

    //初始化触摸屏 并添加LVGL接口
    lv_indev_t *indev = Display_Indev_Init(disp);
    if (!indev) {
        ESP_LOGE(TAG, "添加lvgl触摸接口失败!!!");
        return;
    }
    ESP_LOGI(TAG, "添加lvgl触摸接口成功!!!");

    ESP_LOGI(TAG,"lcd的lvgl显示触摸驱动成功!!!");
}

/**
 * @brief // 触摸屏初始化+添加LVGL接口
 */
static lv_indev_t* Display_Indev_Init(lv_disp_t* disp){
    if(touch_handle==NULL){
        if(LCD_FT6336_Touch_Init(&touch_handle)!=ESP_OK){
            ESP_LOGE(TAG,"触摸驱动失败!!!!");
            return NULL;
        }
    }
    //添加lvgl接口
    lvgl_port_touch_cfg_t touch_cfg={
        .disp   =   disp,
        .handle =   touch_handle,
    };
    return lvgl_port_add_touch(&touch_cfg);
}

/**
 * @brief   lcd初始化+添加lvgl接口
 * @return  非NULL，成功,返回 LVGL display 对象指针 .NULL,失败        
 */
static lv_disp_t* LCD_Display_Init(){
    LCD_ili9341_Init();
    LCD_Set_Color(0xFFFF);
    //开启显示
    esp_lcd_panel_disp_on_off(panel_handle,true);
    ESP_LOGI(TAG,"添加屏幕!!!");

    lvgl_port_display_cfg_t disp_cfg={
        .io_handle      =   io_handle,
        .panel_handle   =   panel_handle,
        .buffer_size    =   LCD_HOR_RES*15,    //缓存
        .double_buffer  =   true,                       //开启双缓存
        .hres           =   LCD_HOR_RES,                //宽
        .vres           =   LCD_VER_RES,                //高
        .monochrome     =   false,                      //是否单色显示器
        .rotation={     //配置与LCD_ili9341_Init()一致
            .mirror_x   =   true,
            .mirror_y   =   false,
            .swap_xy    =   false,
        },      
        .flags={        //不能同时为true
            .buff_spiram=   false,                      //是否使用PSRAM
            .buff_dma   =   true,                       //是否使用DMA
        },
    };
    return lvgl_port_add_disp(&disp_cfg);
}

/**
 * @brief lcd背光初始化
 */
void Backlight_Pwm_Init(){
    ledc_timer_config_t ledc_timer={
        .clk_cfg        =   LEDC_AUTO_CLK,
        .duty_resolution=   LEDC_TIMER_10_BIT,  //10位分辨率,1023
        .deconfigure    =   false,    
        .freq_hz        =   5000,
        .speed_mode     =   LEDC_LOW_SPEED_MODE,
        .timer_num      =   LEDC_TIMER_0,
    };
    if(ledc_timer_config(&ledc_timer)!=ESP_OK){
        ESP_LOGE(TAG,"[%d]定时器初始化失败",__LINE__);
        return;
    }

    ledc_channel_config_t   ledc_channel={
        .channel        =   LEDC_CHANNEL_0,
        .timer_sel      =   LEDC_TIMER_0,
        .duty           =   1023,
        .gpio_num       =   LCD_BL,
        .speed_mode     =   LEDC_LOW_SPEED_MODE
    };
    if(ledc_channel_config(&ledc_channel)!=ESP_OK){
        ESP_LOGE(TAG,"[%d]定时器通道初始化失败",__LINE__);
        return;
    }
    ledc_ok=true;
    ESP_LOGI(TAG,"lcd背光初始化成功!!!");
}

/**
 * @brief lcd背光亮度设置,背光亮度调节依靠PWM
 * @param duty 背光亮度范围0~100
 */
void Backlight_Set(uint8_t duty){
    if(!ledc_ok){
        ESP_LOGE(TAG,"背光未初始化!!");
        return ;
    }
    if(duty>100)duty=100;
    uint32_t bl_duty=(uint32_t)duty*1023/100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,bl_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
}

/**
 * @brief lcd初始化，显示驱动ili9341
 */
void LCD_ili9341_Init(){
    spi_bus_config_t    bus_config_t={
        .miso_io_num        =   LCD_SPI_MISO,
        .mosi_io_num        =   LCD_SPI_MOSI,
        .quadhd_io_num      =   GPIO_NUM_NC,
        .quadwp_io_num      =   GPIO_NUM_NC,
        /*单次spi传输最大字节数，对于屏幕为240*320，
        RGB565每像素为2字节,10为余量            */
        .max_transfer_sz    =   240*320*sizeof(uint16_t)+10,
        .sclk_io_num        =   LCD_SPI_CLK   
    };
    //自动选择DMA通道
    spi_bus_initialize(LCD_SPI_HOST,&bus_config_t,SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_spi_config_t   io_config_t={
        .cs_gpio_num        =   LCD_SPI_CS,     //片选引脚
        .dc_gpio_num        =   LCD_SPI_DC,     //数据/命令选择脚
        .pclk_hz            =   10*1000*1000,   //时钟频率
        .trans_queue_depth  =   10,             //队列深度，可并发处理请求
        .lcd_cmd_bits       =   8,              //参数字段位宽。固定为8
        .spi_mode           =   0,              //模式0
        .lcd_param_bits     =   8,              //参数字段位宽，这里为8
        .on_color_trans_done=   NULL            //颜色传输完成回调
    };
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,&io_config_t,&io_handle);

    // 初始化液晶屏驱动芯片ili9341
    esp_lcd_panel_dev_config_t  panel_fig={
        .reset_gpio_num     =   LCD_SPI_RST,
        .rgb_ele_order      =   LCD_RGB_ELEMENT_ORDER_RGB,  
        .bits_per_pixel     =   16,                         //RGB565
    };
    //面板实例
    esp_lcd_new_panel_ili9341(io_handle,&panel_fig,&panel_handle);

    //复位加初始化
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    
    //颜色是否反转
    esp_lcd_panel_invert_color(panel_handle,true);
    //旋转方向
    esp_lcd_panel_swap_xy(panel_handle,false);
    //param_1   左右翻转    param_2 垂直不翻转
    esp_lcd_panel_mirror(panel_handle,true,false);
}

/***
 *@brief 设置整个页面的颜色
 *@param color  RGB565颜色
 */
void LCD_Set_Color(uint16_t color){
    uint16_t *buffer = heap_caps_malloc(LCD_HOR_RES * sizeof(uint16_t), 
                                         MALLOC_CAP_8BIT);
    if (NULL == buffer){
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }

    //esp32小端，但是ili9341是大端,需要交换顺序
    uint16_t swapped = ((color & 0xFF) << 8) | (color >> 8);

    for (size_t i = 0; i < LCD_HOR_RES; i++)
        buffer[i] = swapped;
    for (int y = 0; y < LCD_VER_RES; y++)
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, LCD_HOR_RES, y+1, buffer);
    free(buffer);
}

/**
 * @brief  在 LCD 上画一个纯色矩形
 * @param  x, y  矩形左上角坐标
 * @param  w, h  矩形宽高（像素）
 * @param  color RGB565 原始颜色，函数内部自动处理字节序
 */
void LCD_Fill_Rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color){
    uint16_t *line_buf = heap_caps_malloc(w * sizeof(uint16_t), MALLOC_CAP_8BIT);
    if (!line_buf) {
        ESP_LOGE(TAG, "fill_rect: malloc failed");
        return;
    }

    uint16_t swapped = (color << 8) | (color >> 8);
    for (int i = 0; i < w; i++)
        line_buf[i] = swapped;

    for (int row = 0; row < h; row++)
        esp_lcd_panel_draw_bitmap(panel_handle, x, y + row, x + w, y + row + 1, line_buf);

    free(line_buf);
}

