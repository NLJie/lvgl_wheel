#ifndef _OSAL_QUEUE_H_
#define _OSAL_QUEUE_H_

#include <stdint.h>

#include "osal_conf.h"

#define M_QUEUE_ID 1

#define M_FIFO_NAME_PREFIX "/tmp"

typedef struct{
	int type;
	uint8_t data[0];
}msg_t;

typedef struct{
	int32_t msgsize;
	int msg_id;
}msg_handle_t;

OSAL_RESULT_T osal_queue_create(osal_queue_t *qhandle, char *name, uint32_t msgsize, uint32_t queue_length);

OSAL_RESULT_T osal_queue_send(osal_queue_t *qhandle, void *msg, uint32_t len, uint32_t msecs);

OSAL_RESULT_T osal_queue_recv(osal_queue_t *qhandle, void *msg, uint32_t msecs);

OSAL_RESULT_T osal_queue_delete(osal_queue_t *qhandle);

#endif

