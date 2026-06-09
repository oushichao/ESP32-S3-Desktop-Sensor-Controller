#pragma once
#include "driver/gpio.h"

#define RELAY_GPIO  GPIO_NUM_1

void Relay_Init();
void Relay_Set(bool on);