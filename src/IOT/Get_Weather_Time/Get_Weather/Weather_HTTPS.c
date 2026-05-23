#include <string.h>
#include <stdlib.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <stdio.h>

#include "Weather_HTTPS.h"
#include "config.h"

static const char* TAG="Weather_HTTPS";
//接收缓冲区
typedef struct{
    char* data;     //缓冲区指针
    size_t len;     //已接受字符数
    size_t cap;     //当前容量
}receive_buf;

static esp_err_t _http_event_handler(esp_http_client_event_t* evt){
    if(!evt)return ESP_FAIL;
    receive_buf* buf=(receive_buf*)evt->user_data;
    if(!buf)return ESP_FAIL;
    switch (evt->event_id){
    case HTTP_EVENT_ON_DATA:{
        size_t need=buf->len+evt->data_len+1;//留给'\0'
        if(need>buf->cap){
            size_t new_cap=buf->cap*2;
            if(new_cap<need)new_cap=need;
            char* p=realloc(buf->data,new_cap);//扩容
            if(!p){
                ESP_LOGE(TAG,"REALLOC FAIL!!!");
                return ESP_FAIL;
            }
            buf->data=p;
            buf->cap=new_cap;
        }
        memcpy(buf->data+buf->len,evt->data,evt->data_len);
        buf->len+=evt->data_len;
        buf->data[buf->len]='\0';
        break;
    }
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG,"HTTP TRANSMIT SUCCESS!!!");
        break;
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG,"HTTP EVENT ERROR!!!");
        break;
    default:
        break;
    }
    return ESP_OK;
}

char* Weather_HTTPS_Fetch_Now(const char* city_id,const char* api_key){
    if(!city_id||!api_key)return NULL;
    char url[256];//拼接url
    int n=  snprintf(
                url,
                sizeof(url),
                "https://%s:%d%s?location=%s&key=%s",
                API_HOST,
                API_PORT,
                API_PATH,
                city_id,
                api_key
            );
    if(n>=sizeof(url)){
        ESP_LOGW(TAG,"[%s:%d] URL过长",__func__,__LINE__);
        return  NULL;
    }

    //接收缓存区
    receive_buf store_buf={
        .data=malloc(512),
        .len=0,
        .cap=512
    };
    if(store_buf.data==NULL){
        ESP_LOGE(TAG,"[%s:%d] malloc_init FAIL!!!",__func__,__LINE__);
        return NULL;
    }
    store_buf.data[0]='\0';

    //配置http客户端
    esp_http_client_config_t cfg={
        .url=url,
        .method=HTTP_METHOD_GET,
        .timeout_ms=10000,
        .event_handler=_http_event_handler,
        .crt_bundle_attach=esp_crt_bundle_attach,//系统证书验证TLS
        .user_data=&store_buf,
        .buffer_size=512
    };
    esp_http_client_handle_t client_handle=esp_http_client_init(&cfg);
    if(client_handle==NULL){
        ESP_LOGE(TAG,"[%s:%d] esp_http_client_init FAIL!!!",__func__,__LINE__);
        free(store_buf.data);
        return NULL;
    }

    //执行请求
    if(esp_http_client_perform(client_handle)!=ESP_OK){
        esp_http_client_cleanup(client_handle);
        ESP_LOGE(TAG,"[%s:%d] REQUEST FAIL!!!",__func__,__LINE__);
        free(store_buf.data);
        return NULL;
    }

    //检查状态码
    int state= esp_http_client_get_status_code(client_handle);
    esp_http_client_cleanup(client_handle);
    if(state!=200){
        ESP_LOGE(TAG,"[%s:%d] ERROR!!!",__func__,__LINE__);
        free(store_buf.data);
        return NULL;
    }
    char* result=realloc(store_buf.data,store_buf.len+1);
    return result ? result : store_buf.data;
}
