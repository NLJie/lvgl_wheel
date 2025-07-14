#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include "osal_thread.h"

OSAL_RESULT_T osal_thread_create(osal_thread_t *thandle,void* (*thread_fun)(void *), void *arg)
{
	OSAL_RESULT_T ret = 0;
	pthread_t *tid = (pthread_t *)malloc(sizeof(pthread_t));
	if(tid == NULL)
	{
		printf("osal thread malloc error");
        return OSAL_ERROR;
	}
	*thandle = tid;
    ret = pthread_create(tid, NULL, thread_fun, arg);
    if (ret != 0)
    {
        printf("osal thread pthread_create error");
        return OSAL_ERROR;
    }
	return OSAL_SUCCESS;
}

OSAL_RESULT_T osal_thread_cancel(void **thandle)
{
	if((thandle != NULL) && (*thandle != NULL))
	{
		pthread_t* p = *(pthread_t **)thandle;
		pthread_cancel(*p);
	}
	return OSAL_SUCCESS;
}

void osal_thread_join(osal_thread_t *thandle, void **thread_return){
	if((thandle != NULL) && (*thandle != NULL)){
		pthread_t* p = *(pthread_t **)thandle;
		pthread_join(*p,thread_return);
		free(*thandle);
	}
}

OSAL_RESULT_T osal_thread_delete(osal_thread_t *thandle)
{
	if((thandle != NULL) && (*thandle != NULL))
	{
		free(*thandle);
	}
	pthread_exit(NULL);
	return OSAL_SUCCESS;
}

void osal_thread_sleep(int32_t msecs)
{
	usleep(msecs * 1000);
}
