#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lvgl.h"
#include "lwip/sys.h"
#include "esp_netif_ip_addr.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "UI/UI_main.h"
#include "config.h"
#include "IOT/WIFI_Init/WIFI_Init.h"
#include "UI/UI_data.h"
#include "Device/LCD/LCD_Init.h"






// static const char* TAG  ="LEDC_Init";
// bool ledc_ok            =false;     //背光初始化标志


// /*传输层,负责CS，DC，SPI模式，时钟*/
// esp_lcd_panel_io_handle_t io_handle=NULL;

// /*应用层,开显示,设置窗口，画像素，旋转/镜像*/
// esp_lcd_panel_handle_t    panel_handle=NULL;

// void Backlight_Pwm_Init(){
//     ledc_timer_config_t ledc_timer={
//         .clk_cfg        =   LEDC_AUTO_CLK,
//         .duty_resolution=   LEDC_TIMER_10_BIT,  //10位分辨率,1023
//         .deconfigure    =   false,    
//         .freq_hz        =   5000,
//         .speed_mode     =   LEDC_LOW_SPEED_MODE,
//         .timer_num      =   LEDC_TIMER_0,
//     };
//     ledc_timer_config(&ledc_timer);

//     ledc_channel_config_t   ledc_channel={
//         .channel        =   LEDC_CHANNEL_0,
//         .timer_sel      =   LEDC_TIMER_0,
//         .duty           =   1023,
//         .gpio_num       =   LCD_BL,
//         .speed_mode     =   LEDC_LOW_SPEED_MODE
//     };
//     ledc_channel_config(&ledc_channel);
//     ledc_ok=true;
// }

// void Backlight_Set(uint8_t duty){
//     if(!ledc_ok){
//         ESP_LOGE(TAG,"背光未初始化!!");
//         return ;
//     }
//     if(duty>100)duty=100;
//     uint32_t bl_duty=(uint32_t)duty*1023/100;
//     ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,bl_duty);
//     ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
// }

// void ILI9341_Init(){
//     spi_bus_config_t    bus_config_t={
//         .miso_io_num        =   LCD_SPI_MISO,
//         .mosi_io_num        =   LCD_SPI_MOSI,
//         .quadhd_io_num      =   GPIO_NUM_NC,
//         .quadwp_io_num      =   GPIO_NUM_NC,
//         /*单次spi传输最大字节数，对于屏幕为240*320，
//         RGB565每像素为2字节,10为余量            */
//         .max_transfer_sz    =   240*320*2+10,
//         .sclk_io_num        =   LCD_SPI_CLK   
//     };
//     //自动选择DMA通道
//     spi_bus_initialize(LCD_SPI_HOST,&bus_config_t,SPI_DMA_CH_AUTO);

//     esp_lcd_panel_io_spi_config_t   io_config_t={
//         .cs_gpio_num        =   LCD_SPI_CS,     //片选引脚
//         .dc_gpio_num        =   LCD_SPI_DC,     //数据/命令选择脚
//         .pclk_hz            =   10*1000*1000,   //时钟频率
//         .trans_queue_depth  =   10,             //队列深度，可并发处理请求
//         .lcd_cmd_bits       =   8,              //参数字段位宽。固定为8
//         .spi_mode           =   0,              //模式0
//         .lcd_param_bits     =   8,              //参数字段位宽，这里为8
//         .on_color_trans_done=   NULL            //颜色传输完成回调
//     };
//     esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,&io_config_t,&io_handle);

//     //面板配置
//     esp_lcd_panel_dev_config_t  panel_fig={
//         .reset_gpio_num     =   LCD_SPI_RST,
//         .rgb_ele_order      =   LCD_RGB_ELEMENT_ORDER_BGR,  //ili9341默认GRB
//         .bits_per_pixel     =   16,                         //RGB565
//     };
//     //面板实例
//     esp_lcd_new_panel_ili9341(io_handle,&panel_fig,&panel_handle);

//     //复位加初始化
//     esp_lcd_panel_reset(panel_handle);
//     esp_lcd_panel_init(panel_handle);
    
//     //颜色不反转
//     esp_lcd_panel_invert_color(panel_handle,false);

//     //旋转方向
//     esp_lcd_panel_swap_xy(panel_handle,false);
//     //param_1   左右翻转    param_2 垂直不翻转
//     esp_lcd_panel_mirror(panel_handle,true,false);
    
//     //开启显示
//     esp_lcd_panel_disp_on_off(panel_handle,true);

//     Backlight_Pwm_Init();
//     Backlight_Set(80);
// }








// #define MAX_RETRY   5

// static const char* TAG ="WIFI_INIT";
// static uint8_t reconnect=0;
// EventGroupHandle_t   wifi_ev;

// esp_netif_t *esp_netif;     //网络接口句柄,指定查询的网络接口

// void wifi_event_handler(void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data){
//     if(event_base==WIFI_EVENT){
//         switch(event_id){   //wifi_event_t类型
//             case    WIFI_EVENT_STA_START:
//                 esp_wifi_connect();
//                 break;
//             case    WIFI_EVENT_STA_DISCONNECTED:
//                 if(++reconnect<=MAX_RETRY){
//                     ESP_LOGI(TAG,"尝试重连,次数为%d/%d",reconnect,MAX_RETRY);
//                     esp_wifi_connect();
//                 }
//                 break;
//             case    WIFI_EVENT_STA_CONNECTED:
//                 reconnect=0;
//                 ESP_LOGI(TAG,"连接成功!!!!");
//                 break;
//             default:
//                 break;
//         }
//     }
//     else if(event_base==IP_EVENT){
//         switch (event_id){  //ip_event_t类型
//             case    IP_EVENT_STA_GOT_IP:{
//                 xEventGroupSetBits(wifi_ev,BIT0);   //确保wifi连接再进行MQTT
//                 esp_netif_ip_info_t ip_info;
//                 esp_netif_get_ip_info(esp_netif, &ip_info);
//                 ESP_LOGI(TAG, "获取的ip地址为:" IPSTR, IP2STR(&ip_info.ip));
//                 break;
//             }
//             default:
//                 break;
//         }
//     }
// }

// void wifi_init(){
//     esp_err_t ret = nvs_flash_init();                       //NVS
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         //页面不兼容或者已满,满足OTA回滚要求
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }                                       
//     esp_netif_init();                                       //tcp/ip
//     esp_event_loop_create_default();                        //循环事件组

//     esp_netif=esp_netif_create_default_wifi_sta();          //tcp/ip绑定sta
//     wifi_init_config_t config=WIFI_INIT_CONFIG_DEFAULT();
//     esp_wifi_init(&config);                                 //默认wifi配置
//     esp_wifi_set_mode(WIFI_MODE_STA);                       //设置sta模式

//     //设置回调
//     static esp_event_handler_instance_t  wifi_event;
//     static esp_event_handler_instance_t  ip_event;
//     esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event_handler,NULL,&wifi_event);
//     esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event_handler,NULL,&ip_event);

//     //配置sta模式
//     wifi_config_t wifi_config={
//         .sta={
//             .ssid=WIFI_SSID,
//             .password=WIFI_PASSWORD
//         }
//     };

//     esp_wifi_set_config(WIFI_IF_STA,&wifi_config);          //配置写入sta模式
//     esp_wifi_start();                                       //wifi开始工作
// }