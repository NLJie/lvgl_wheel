/*
 * @Author: xiaozhi
 * @Date: 2024-09-30 00:21:03
 * @Last Modified by: xiaozhi
 * @Last Modified time: 2024-10-08 23:45:16
 */

#include <stdio.h>
#include <stdlib.h>
#include "cJSON/cJSON.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "http_manager.h"
#include "osal_thread.h"
#include "osal_queue.h"

static osal_queue_t net_queue = NULL;
static osal_thread_t net_thread = NULL;

static weather_callback_fun weather_callback_func = NULL;

/**
 * @brief 组装http请求的url。
 * @return int 0:成功
 */
static int assemble_url(char *host, char *path, char **url)
{
    int length = strlen(host) + strlen(path);
    char *tmp = calloc(length + 1, sizeof(char));
    strcpy(tmp, host);
    strcat(tmp, path);
    *url = tmp;
    return 0;
}

/**
 * @brief 循环被curl调用写入数据，直到完成所有数据写入后结束。
 *
 * @param data 单次需要写入的数据指针
 * @param size  1
 * @param nmemb 数据大小
 * @param userp
 * @return size_t 单次写入的数据大小；需要与curl传入的size*nmemb相等，否则认为写入失败。
 */
static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    http_resp_data_t *mem = (http_resp_data_t *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (ptr == NULL)
        return 0; /* out of memory! */

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), data, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    return realsize;
}

int http_request_method(char *host, char *path, char *request_json, char *method, char **response_json)
{
    CURL *curl = curl_easy_init();
    if (curl != NULL)
    {
        int ret = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        if (ret != 0)
        {
            return -1;
        }
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

        // 拼装http请求的url
        char *url; // 需要释放
        assemble_url(host, path, &url);

        // 设置http请求的url
        ret = curl_easy_setopt(curl, CURLOPT_URL, url);
        // 释放内存
        free(url);
        if (ret != 0)
        {
            printf("Set CURLOPT_URL failed !!!");
            return -1;
        }

        // 设置接收http请求的返回函数
        ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        if (ret != 0)
        {
            printf("Set CURLOPT_WRITEFUNCTION failed !!!");
            return -1;
        }

        // 定义返HTTP回数据的指针的存储对象
        http_resp_data_t response_data = {0};
        ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);
        if (ret != 0)
        {
            printf("Set CURLOPT_WRITEDATA failed !!!");
            return -1;
        }

        if (strcmp(method, "POST") == 0)
        {
            // The data pointed to is NOT copied by the library
            ret = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json);
            if (ret != 0)
            {
                printf("Set CURLOPT_POSTFIELDS failed !!!");
                return -1;
            }
            // 设置post提交
            ret = curl_easy_setopt(curl, CURLOPT_POST, 1);
            if (ret != 0)
            {
                printf("Set CURLOPT_POST failed !!!");
                return -1;
            }
        }

        ret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        if (ret != 0)
        {
            return -1;
        }

        // 设置http请求的header
        struct curl_slist *header = NULL;
        header = curl_slist_append(NULL, "Content-Type:application/json");
        if (header == NULL)
        {
            printf("Create the http header failed !!!");
            return -1;
        }

        ret = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
        if (ret != 0)
        {
            printf("Set CURLOPT_HTTPHEADER failed !!!");
            curl_slist_free_all(header);
            return -1;
        }
        // 发送http请求
        CURLcode code = curl_easy_perform(curl);
        // 释放http请求头
        curl_slist_free_all(header);
        // 处理http请求结果
        if (code != 0)
        {
            printf("Perform http request failed: %s\n  %d", curl_easy_strerror(code), code);
            ret = -1;
        }

        // http 请求成功后，处理http请求结果
        printf("The response data len: %ld\n", response_data.size);
        printf("The response data: %s\n", response_data.data);
        *response_json = response_data.data;
        curl_easy_cleanup(curl);
        return ret;
    }
}

int http_method_request(char *host,char *path, char *request_json, char *method, char **response_json)
{
    printf("reqest url:%s%s%s method:%s\n", host, path, request_json,method);
    return http_request_method(host, path, request_json, method, response_json);
}

static void http_save_user_body(cJSON **base)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "applianceId", cJSON_CreateString("testid"));
    cJSON_AddItemToObject(root, "num", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "userName", cJSON_CreateString("xiaozhi"));
    *base = root;
}

void http_add_data()
{
    char get_path[100] = "/data/add";
    cJSON *root;
    http_save_user_body(&root);
    char *postdata = cJSON_Print(root);
    cJSON_Delete(root);
    char *response_json_str;
    http_method_request("https://api.seniverse.com",get_path, postdata, "POST", &response_json_str);
    if (response_json_str == NULL)
    {
        printf("request fail \n");
    }
}

void parseWeatherData(const char *json_data) {
    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        fprintf(stderr, "Error parsing JSON data.\n");
        return;
    }
    // 获取 results 数组
    cJSON *results = cJSON_GetObjectItem(root, "results");
    if (!results || !cJSON_IsArray(results)) {
        fprintf(stderr, "Invalid JSON format: missing 'results' array.\n");
        cJSON_Delete(root);
        return;
    }
    int num_results = cJSON_GetArraySize(results);
    if (num_results <= 0) {
        fprintf(stderr, "No results found.\n");
        cJSON_Delete(root);
        return;
    }
    // 处理第一个结果
    cJSON *result = cJSON_GetArrayItem(results, 0);
    if (!result) {
        fprintf(stderr, "Invalid JSON format: missing first result.\n");
        cJSON_Delete(root);
        return;
    }
    // 获取 location 对象
    cJSON *location = cJSON_GetObjectItem(result, "location");
    if (!location || !cJSON_IsObject(location)) {
        fprintf(stderr, "Invalid JSON format: missing 'location' object.\n");
        cJSON_Delete(root);
        return;
    }
    // 获取 now 对象
    cJSON *now = cJSON_GetObjectItem(result, "now");
    if (!now || !cJSON_IsObject(now)) {
        fprintf(stderr, "Invalid JSON format: missing 'now' object.\n");
        cJSON_Delete(root);
        return;
    }
    // 获取 last_update 字段
    cJSON *last_update = cJSON_GetObjectItem(result, "last_update");
    if (!last_update || !cJSON_IsString(last_update)) {
        fprintf(stderr, "Invalid JSON format: missing 'last_update' string.\n");
        cJSON_Delete(root);
        return;
    }
    // 打印 location 字段
    printf("Location ID: %s\n", cJSON_GetObjectItem(location, "id")->valuestring);
    printf("Location Name: %s\n", cJSON_GetObjectItem(location, "name")->valuestring);
    printf("Country: %s\n", cJSON_GetObjectItem(location, "country")->valuestring);
    printf("Path: %s\n", cJSON_GetObjectItem(location, "path")->valuestring);
    printf("Timezone: %s\n", cJSON_GetObjectItem(location, "timezone")->valuestring);
    printf("Timezone Offset: %s\n", cJSON_GetObjectItem(location, "timezone_offset")->valuestring);
    // 打印 now 字段
    printf("Current Weather: %s\n", cJSON_GetObjectItem(now, "text")->valuestring);
    printf("Weather Code: %s\n", cJSON_GetObjectItem(now, "code")->valuestring);
    printf("Temperature: %s\n", cJSON_GetObjectItem(now, "temperature")->valuestring);
    // 打印 last_update 字段
    printf("Last Update: %s\n", last_update->valuestring);

    char weather_info[50];
    memset(weather_info, 0, sizeof(weather_info));
    strcat(weather_info, cJSON_GetObjectItem(location, "name")->valuestring);
    strcat(weather_info, " ");
    strcat(weather_info, cJSON_GetObjectItem(now, "text")->valuestring);
    strcat(weather_info, " ");
    strcat(weather_info, cJSON_GetObjectItem(now, "temperature")->valuestring);
    strcat(weather_info, "°C");
    printf("weather_info = %s\n",weather_info);
    if(weather_callback_func != NULL)
        weather_callback_func(weather_info);
    cJSON_Delete(root);
}


void http_get_weather(char *key,char *city){
    char *response_json_str;
    char get_path[100];
    snprintf(get_path, sizeof(get_path), "/v3/weather/now.json?key=%s&location=%s&language=zh-Hans&unit=c", key,city);
    http_method_request("https://api.seniverse.com",get_path, "", "GET", &response_json_str);
    if (response_json_str != NULL)
    {
        // printf("response ---> %s\n",response_json_str);
        parseWeatherData(response_json_str);
    }
}

#include "em_hal_time.h"

static void set_time_str(char *str){
	char year_str[5], month_str[3], day_str[3], hour_str[3], minute_str[3], second_str[3];
    // 分割字符串
    char *token = strtok((char *)str, "-T:");
    strncpy(year_str, token, 4);  // 年
    token = strtok(NULL, "-T:");
    strncpy(month_str, token, 2);  // 月
    token = strtok(NULL, "-T:");
    strncpy(day_str, token, 2);  // 日
    token = strtok(NULL, "-T:");
    strncpy(hour_str, token, 2);  // 时
    token = strtok(NULL, "-T:");
    strncpy(minute_str, token, 2);  // 分
    token = strtok(NULL, "+");
    strncpy(second_str, token, 2);  // 秒
    // 转换为整数
    int year = atoi(year_str);
    int month = atoi(month_str);
    int day = atoi(day_str);
    int hour = atoi(hour_str);
    int minute = atoi(minute_str);
    int second = atoi(second_str);
	em_hal_time_set_time(year,month,day,hour,minute,second);
}

void parseDateTimeData(const char *json_data) {
    printf("parseDateTimeData\n");
    cJSON *root = cJSON_Parse(json_data);

    if (!root) {
        fprintf(stderr, "Error parsing JSON data.\n");
        return;
    }

    // 获取各个字段
    cJSON *utc_offset = cJSON_GetObjectItem(root, "utc_offset");
    cJSON *timezone = cJSON_GetObjectItem(root, "timezone");
    cJSON *day_of_week = cJSON_GetObjectItem(root, "day_of_week");
    cJSON *day_of_year = cJSON_GetObjectItem(root, "day_of_year");
    cJSON *datetime = cJSON_GetObjectItem(root, "datetime");
    cJSON *utc_datetime = cJSON_GetObjectItem(root, "utc_datetime");
    cJSON *unixtime = cJSON_GetObjectItem(root, "unixtime");
    cJSON *raw_offset = cJSON_GetObjectItem(root, "raw_offset");
    cJSON *week_number = cJSON_GetObjectItem(root, "week_number");
    cJSON *dst = cJSON_GetObjectItem(root, "dst");
    cJSON *abbreviation = cJSON_GetObjectItem(root, "abbreviation");
    cJSON *dst_offset = cJSON_GetObjectItem(root, "dst_offset");
    cJSON *dst_from = cJSON_GetObjectItem(root, "dst_from");
    cJSON *dst_until = cJSON_GetObjectItem(root, "dst_until");
    cJSON *client_ip = cJSON_GetObjectItem(root, "client_ip");

    // 检查字段是否存在
    if (!utc_offset || !timezone || !day_of_week || !day_of_year ||
        !datetime || !utc_datetime || !unixtime || !raw_offset ||
        !week_number || dst == NULL || !abbreviation || dst_offset == NULL ||
        dst_from == NULL || dst_until == NULL || !client_ip) {
        fprintf(stderr, "Invalid JSON format.\n");
        cJSON_Delete(root);
        return;
    }

    // 打印各个字段
    printf("UTC Offset: %s\n", utc_offset->valuestring);
    printf("Timezone: %s\n", timezone->valuestring);
    printf("Day of Week: %d\n", day_of_week->valueint);
    printf("Day of Year: %d\n", day_of_year->valueint);
    printf("Datetime: %s\n", datetime->valuestring);
    printf("UTC Datetime: %s\n", utc_datetime->valuestring);
    printf("Unixtime: %lld\n", (long long)unixtime->valuedouble);
    printf("Raw Offset: %lld\n", (long long)raw_offset->valuedouble);
    printf("Week Number: %d\n", week_number->valueint);
    printf("DST: %s\n", dst->valueint ? "true" : "false");
    printf("Abbreviation: %s\n", abbreviation->valuestring);
    printf("DST Offset: %lld\n", (long long)dst_offset->valuedouble);
    printf("DST From: %s\n", dst_from ? dst_from->valuestring : "null");
    printf("DST Until: %s\n", dst_until ? dst_until->valuestring : "null");
    printf("Client IP: %s\n", client_ip->valuestring);
    set_time_str(datetime->valuestring);
    // 清理资源
    cJSON_Delete(root);
}

void http_get_time(){
    char *response_json_str;
    http_method_request("http://worldtimeapi.org","/api/timezone/Asia/Shanghai", "", "GET", &response_json_str);
    if (response_json_str != NULL)
    {
        parseDateTimeData(response_json_str);
    }
}

static void handle_net_request(osal_queue_t *net_queue)
{
    int ret = OSAL_ERROR;
    
    net_obj obj;
    memset(&obj, 0, sizeof(net_obj));
    char *response_json_str;

    ret = osal_queue_recv(net_queue, (void*)&obj, 100);
    if (ret == OSAL_SUCCESS)
    {
        NET_COMM_ID id = obj.id;
        switch(id)
        {
            case NET_GET_TIME:
                printf("handle NET_GET_TIME");
                http_method_request(obj.host,obj.path,obj.data,obj.type,&response_json_str);
                if (response_json_str != NULL){
                    parseDateTimeData(response_json_str);
                    free(response_json_str);
                }
                break;
            case NET_GET_WEATHER:
                printf("handle NET_GET_WEATHER");
                http_method_request(obj.host,obj.path,obj.data,obj.type,&response_json_str);
                if (response_json_str != NULL){
                    parseWeatherData(response_json_str);
                    free(response_json_str);
                }
                break;
            default:
                break;
        }
    }
}

static void* net_thread_fun(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while(1)
    {
        handle_net_request(&net_queue);
        osal_thread_sleep(500);
    }
}

void http_get_time_async()
{
    net_obj obj;    
    memset(&obj, 0, sizeof(net_obj));
    strcpy(obj.host,"http://worldtimeapi.org");
    strcpy(obj.path,"/api/timezone/Asia/Shanghai");
    obj.id = NET_GET_TIME;
    strcpy(obj.data,"");
    strcpy(obj.type,"GET");
    int ret = osal_queue_send(&net_queue, &obj, sizeof(net_obj), 1000);
    if(ret == OSAL_ERROR)
    {
        printf("queue send error");
    }
}

void http_get_weather_async(char *key,char *city){
    net_obj obj;    
    memset(&obj, 0, sizeof(net_obj));
    strcpy(obj.host,"https://api.seniverse.com");
    sprintf(obj.path, "/v3/weather/now.json?key=%s&location=%s&language=zh-Hans&unit=c", key,city);
    obj.id = NET_GET_WEATHER;
    strcpy(obj.data,"");
    strcpy(obj.type,"GET");
    int ret = osal_queue_send(&net_queue, &obj, sizeof(net_obj), 1000);
    if(ret == OSAL_ERROR)
    {
        printf("queue send error");
    }
}

void http_set_weather_callback(weather_callback_fun func){
	weather_callback_func = func;						
}

int http_request_create()
{
    int ret = OSAL_ERROR;
    ret = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (ret != 0)
    {
        return -1;
    }
    ret = osal_queue_create(&net_queue,
                                "net_queue",
                                sizeof(net_obj),
                                50);
    if(ret == OSAL_ERROR)
    {
        printf("create queue error");
        return -1;
    }   
    
    ret = osal_thread_create(&net_thread,net_thread_fun, NULL);
    if(ret == OSAL_ERROR)
    {
        printf("create thread error");
        return -1;
    }
    return 0;
}

int http_request_destory()
{
    curl_global_cleanup();
    return 0;
}
