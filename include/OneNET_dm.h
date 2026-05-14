#pragma once
#include "cJSON.h"

extern bool relay_switch;
extern float temp_threshold;
extern float humi_threshold;

extern float temperature;
extern float humidity;
extern int32_t light;
extern bool relay_state;

void OneNET_Property_Handle(cJSON*property);//处理下行数据
cJSON* OneNET_Property_Upload();//设备数据上报给onenet