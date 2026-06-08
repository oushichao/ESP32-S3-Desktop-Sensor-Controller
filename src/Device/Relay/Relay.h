#pragma once
#include "driver/gpio.h"

#define RELAY_GPIO  GPIO_NUM_5
#define LED_GPIO    GPIO_NUM_38

void Relay_Init();
void Relay_Set(bool on);