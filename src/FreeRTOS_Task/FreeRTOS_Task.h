#pragma once

typedef struct{
    float temperature;
    float humidity;
    uint16_t light;
}sensor_data_t;

void Start_FreeROTS_Task();