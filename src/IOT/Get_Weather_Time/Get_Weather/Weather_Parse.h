#pragma once

#include <stdbool.h>
#include <time.h>

/*
    @brief 解析和风天气返回的天气数据

    @param json_str 获取的原始字符串
    @param out      天气状况文本
    @param out_len  缓冲区大小

    @return true 解析成功   false 解析失败
*/
bool Weather_Parse_Now(const char* json_str,char* out,size_t out_len);