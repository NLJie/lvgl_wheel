#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <pthread.h>

#include "osal_queue.h"

OSAL_RESULT_T osal_queue_create(osal_queue_t *qhandle,char *name, uint32_t msgsize,uint32_t queue_length)
{
	int fd = 0;
	key_t key = 0;
	msg_handle_t* msg_handle = NULL;
	char fileName[64];
	memset(fileName, 0, 64);
	sprintf(fileName, "%s/%s", M_FIFO_NAME_PREFIX, name);
	msg_handle = malloc(sizeof(msg_handle_t));
	if(msg_handle == NULL){
		printf("osal queue malloc error");
		return OSAL_ERROR;
	}
	if(access(M_FIFO_NAME_PREFIX, F_OK) != 0) {
		mkdir(M_FIFO_NAME_PREFIX, 0777);
	}
	if(access(fileName, F_OK) != 0) {
		unlink(fileName);
		fd = open(fileName, O_RDONLY|O_NONBLOCK|O_CREAT, 0666);
		if(fd == -1){
			printf("osal queue open error");
			return OSAL_ERROR;
		}
		close(fd);
	}
	key = ftok(fileName, M_QUEUE_ID);
	if(key == -1){
		printf("osal queue ftok error");
		goto error;
	}
	
	msg_handle->msg_id = msgget(key, IPC_CREAT | 0666);
	if(msg_handle->msg_id == -1){
		printf("osal queue msgget error");
		goto error;
	}
	msg_handle->msgsize = msgsize;
	*qhandle = msg_handle;
	return OSAL_SUCCESS;
	
error:
	free(msg_handle);
	return OSAL_ERROR;
}

OSAL_RESULT_T osal_queue_send(osal_queue_t *qhandle, void *msg, uint32_t len, uint32_t msecs)
{
	int ret = OSAL_ERROR;
	msg_t* qmsg = NULL;
	msg_handle_t *msg_handle = *qhandle;
	if(msg_handle != NULL){
		qmsg = malloc(sizeof(int) + msg_handle->msgsize);
		if(qmsg != NULL){
			memset(qmsg, 0, sizeof(int) + msg_handle->msgsize);
			qmsg->type = 1;
			memcpy(qmsg->data, msg, msg_handle->msgsize);
			ret = msgsnd(msg_handle->msg_id, qmsg, msg_handle->msgsize, IPC_NOWAIT);
			if(ret == 0){
				ret = OSAL_SUCCESS;
			}
			else{
				printf("osal queue msgsnd error");
				ret = OSAL_ERROR;
			}
		}
	}

	if(qmsg != NULL){
		free(qmsg);
	}
	
	return ret;
}

static void cleanup_handler(void *arg) {
    void *mem = *(void **)arg;
    free(mem);
	mem = NULL;
}

OSAL_RESULT_T osal_queue_recv(osal_queue_t *qhandle, void *msg, uint32_t msecs)
{
	int ret = OSAL_ERROR;
	msg_t* qmsg;
	msg_handle_t *msg_handle = *qhandle;
	if(msg_handle != NULL){
		qmsg = malloc(sizeof(int) + msg_handle->msgsize);
		if(qmsg != NULL){
			pthread_cleanup_push(cleanup_handler, &qmsg);
			memset(qmsg, 0, sizeof(int) + msg_handle->msgsize);
			ret = msgrcv(msg_handle->msg_id, qmsg, msg_handle->msgsize, 0, IPC_NOWAIT);
			if(ret != -1){
				memcpy(msg, qmsg->data, msg_handle->msgsize);
				ret = OSAL_SUCCESS;
			}
			else{
				if(errno != ENOMSG){
					printf("osal queue msgrcv error");
				}				
				ret = OSAL_ERROR;
			}
			pthread_cleanup_pop(1);
		}
	}
	return ret;
}

OSAL_RESULT_T osal_queue_delete(osal_queue_t *qhandle)
{
	msg_handle_t *msg_handle = *qhandle;
	if(msg_handle != NULL){
		msgctl(msg_handle->msg_id, IPC_RMID, NULL);
		free(msg_handle);
	}else{
		return OSAL_ERROR;
	}
    return OSAL_SUCCESS;
}
