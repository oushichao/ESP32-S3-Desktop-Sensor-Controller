#include "driver/gpio.h"

#include "Relay_And_Led.h"
#include "MY_I2C/MY_I2C.h"

void Led_Init(){
    gpio_reset_pin(LED_GPIO);   //清空旧配置
    gpio_set_direction(LED_GPIO,GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO,0);
}

void Relay_Init(){
    gpio_reset_pin(RELAY_GPIO);   //清空旧配置
    gpio_set_direction(RELAY_GPIO,GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_GPIO,0);    
}