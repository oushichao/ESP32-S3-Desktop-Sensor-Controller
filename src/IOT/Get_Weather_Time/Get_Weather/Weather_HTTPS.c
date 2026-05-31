#include <string.h>
#include <stdlib.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <stdio.h>
#include "miniz.h"


#include "Weather_HTTPS.h"
#include "config.h"

static const char* TAG="Weather_HTTPS";
//接收缓冲区
typedef struct{
    char* data;     //缓冲区指针
    size_t len;     //已接受字符数
    size_t cap;     //当前容量
}receive_buf;

static char *gzip_decompress(const uint8_t *src, size_t src_len, size_t *out_len)
{
    if (src_len < 10 || src[0] != 0x1F || src[1] != 0x8B) {
        // 不是 gzip，原样返回
        *out_len = src_len;
        char *ret = malloc(src_len + 1);
        if (ret) { memcpy(ret, src, src_len); ret[src_len] = '\0'; }
        return ret;
    }

    // 跳过 gzip 头部
    size_t pos = 10;           // 固定 10 字节基础头
    uint8_t flags = src[3];
    if (flags & 0x04) {        // FEXTRA
        if (pos + 2 > src_len) return NULL;
        uint16_t xlen = src[pos] | (src[pos+1] << 8);
        pos += 2 + xlen;
    }
    if (flags & 0x08) {        // FNAME
        while (pos < src_len && src[pos] != 0) pos++;
        pos++;
    }
    if (flags & 0x10) {        // FCOMMENT
        while (pos < src_len && src[pos] != 0) pos++;
        pos++;
    }
    if (flags & 0x02) pos += 2; // FHCRC

    // 解压 deflate 数据
    tinfl_decompressor inflator;
    tinfl_init(&inflator);

    size_t dst_cap = (src_len - pos) * 4;
    if (dst_cap < 1024) dst_cap = 1024;
    uint8_t *dst = malloc(dst_cap);
    if (!dst) return NULL;

    size_t src_off = pos, dst_off = 0;
    tinfl_status status;
    do {
        size_t src_rem = src_len - src_off;
        size_t dst_rem = dst_cap - dst_off;
        status = tinfl_decompress(&inflator, src + src_off, &src_rem,
                                dst, dst + dst_off, &dst_rem,
                                TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

        src_off = src_len - src_rem;
        dst_off = dst_cap - dst_rem;
        if (status == TINFL_STATUS_HAS_MORE_OUTPUT) {
            dst_cap *= 2;
            uint8_t *tmp = realloc(dst, dst_cap);
            if (!tmp) { free(dst); return NULL; }
            dst = tmp;
        }
    } while (status == TINFL_STATUS_HAS_MORE_OUTPUT);

    if (status != TINFL_STATUS_DONE) {
        free(dst);
        return NULL;
    }

    dst[dst_off] = '\0';
    *out_len = dst_off;
    return (char *)dst;
}



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
                "https://%s:%d%s?location=%s&key=%s&lang=en",
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
    esp_http_client_set_header(client_handle, "Accept-Encoding", "identity");//禁用gzip压缩
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
        ESP_LOGE(TAG,"[%s:%d] HTTP STATE:%d",__func__,__LINE__,state);
        free(store_buf.data);
        return NULL;
    }

    size_t json_len = 0;
    char *json = gzip_decompress((const uint8_t *)store_buf.data, store_buf.len, &json_len);
    free(store_buf.data);
    if (!json) {
        ESP_LOGE(TAG, "gzip decompress failed");
        return NULL;
    }
    char *result = realloc(json, json_len + 1);
    return result ? result : json;
}
