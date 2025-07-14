#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osal_thread.h"
#include "osal_queue.h"
#include "em_hal_audio.h"

typedef enum
{
    AUDIO_COMM_ID_START,
    AUDIO_COMM_ID_STOP,
}AUDIO_COMM_ID;

typedef struct
{
    AUDIO_COMM_ID id;
    char file_name[256];
}audio_obj;

static osal_queue_t audio_queue = NULL;
static osal_queue_t stop_audio_queue = NULL;

static osal_thread_t audio_thread = NULL;
static osal_thread_t stop_audio_thread = NULL;

static void handle_audio(osal_queue_t *audio_queue)
{
    int ret = OSAL_ERROR;
    
    audio_obj obj;
    memset(&obj, 0, sizeof(audio_obj));

    ret = osal_queue_recv(audio_queue, (void*)&obj, 100);
    if (ret == OSAL_SUCCESS)
    {
        AUDIO_COMM_ID id = obj.id;
        printf("========>%d",id);
        switch(id)
        {
            case AUDIO_COMM_ID_START:
            em_play_audio(obj.file_name);
                break;
            default:
                break;
        }
    }
}

static void stop_handle_audio(osal_queue_t *stop_audio_queue)
{
    int ret = OSAL_ERROR;
    
    audio_obj obj;
    memset(&obj, 0, sizeof(audio_obj));

    ret = osal_queue_recv(stop_audio_queue, (void*)&obj, 100);
    if (ret == OSAL_SUCCESS)
    {
        AUDIO_COMM_ID id = obj.id;
        printf("========>%d",id);
        switch(id)
        {
            case AUDIO_COMM_ID_STOP:
                em_stop_play_audio();
                break;
            default:
                break;
        }
    }
}

static void* audio_thread_fun(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while(1)
    {
        handle_audio(&audio_queue);
        osal_thread_sleep(1);
    }
}

static void* stop_audio_thread_fun(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while(1)
    {
        stop_handle_audio(&stop_audio_queue);
        osal_thread_sleep(1);
    }
}

void start_play_audio_async(char *url)
{
    audio_obj obj;  
    memset(&obj, 0, sizeof(audio_obj));
    obj.id = AUDIO_COMM_ID_START;
    strcpy(obj.file_name, url);
    int ret = osal_queue_send(&audio_queue, &obj, sizeof(audio_obj), 1000);
    if(ret == OSAL_ERROR)
    {
        printf("queue send error");
    }
}

void stop_play_audio_async(void)
{
    audio_obj obj;  
    memset(&obj, 0, sizeof(audio_obj));
    obj.id = AUDIO_COMM_ID_STOP;
    int ret = osal_queue_send(&stop_audio_queue, &obj, sizeof(audio_obj), 1000);
    if(ret == OSAL_ERROR)
    {
        printf("queue send error");
    }
}

int init_async_audio_player(void)
{
    int ret = OSAL_ERROR;

    ret = osal_queue_create(&audio_queue,"audio_queue",sizeof(audio_obj),50);
    if(ret == OSAL_ERROR)
    {
        printf("create queue error");
        return -1;
    }   
    ret = osal_thread_create(&audio_thread,audio_thread_fun, NULL);
    if(ret == OSAL_ERROR)
    {
        printf("create thread error");
        return -1;
    }

    ret = osal_queue_create(&stop_audio_queue,"stop_audio_queue",sizeof(audio_obj),50);
    if(ret == OSAL_ERROR)
    {
        printf("create queue error");
        return -1;
    }   
    ret = osal_thread_create(&stop_audio_thread,stop_audio_thread_fun, NULL);
    if(ret == OSAL_ERROR)
    {
        printf("create thread error");
        return -1;
    }
    return 0;
}