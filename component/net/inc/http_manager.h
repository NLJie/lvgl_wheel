/**
 * @file http_manager.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-05
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef _HTTP_MANAGER_H
#define _HTTP_MANAGER_H

typedef enum
{
    NET_GET_WEATHER = 0,
    NET_GET_TIME,
}NET_COMM_ID;

typedef struct
{
    NET_COMM_ID id;
    char host[100];
    char path[100];
    char data[50];
    char type[10];
    int loop_flag;
}net_obj;

typedef struct
{
    char *data;
    size_t size;
} http_resp_data_t;

typedef void (* weather_callback_fun)(char* str);

int http_request_create(void);

void http_get_weather(char *key,char *city);

void http_get_time(void);

void http_get_weather_async(char *key,char *city);

void http_set_weather_callback(weather_callback_fun func);

void http_get_time_async();

#endif