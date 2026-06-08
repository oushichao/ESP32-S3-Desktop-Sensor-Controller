#include "driver/gpio.h"
#include <stdbool.h>  

#include "Relay.h"
#include "MY_I2C/MY_I2C.h"


void Relay_Init(){
    gpio_reset_pin(RELAY_GPIO);   //清空旧配置
    gpio_set_direction(RELAY_GPIO,GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_GPIO,0);    
}

void Relay_Set(bool on){
     gpio_set_level(RELAY_GPIO,on);
}