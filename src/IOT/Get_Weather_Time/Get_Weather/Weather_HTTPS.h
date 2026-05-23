#pragma once
#include<stddef.h>

#define API_HOST    "devapi.qweather.com"
#define API_PATH    "/v7/weather/now"
#define API_PORT    443
#define CITY_ID     101270101

char* Weather_HTTPS_Fetch_Now(const char* city_id,const char* api_key);