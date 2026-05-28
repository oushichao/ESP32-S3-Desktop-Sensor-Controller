#pragma once

void NVS_Init();
void NVS_Write_Str(char* key,char* str);
void NVS_Write_int32_t(char* key,int32_t value);
void NVS_Write_uint16_t(char* key,uint16_t value);
void NVS_Write_uint8_t(char* key,uint8_t value);
void NVS_Read_Store_Data();
