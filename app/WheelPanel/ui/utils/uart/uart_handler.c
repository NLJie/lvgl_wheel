#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>
#include "uart_handler.h"
#include "em_hal_uart.h"
#include "device_data.h"

#include "lvgl.h"
#include "image_conf.h"

extern lv_obj_t * bg_img;
extern lv_obj_t * wheel_mode;

#ifdef SIMULATOR_LINUX
#define HANDLER_UART_NUM "/dev/ttyUSB0"
#else
#define HANDLER_UART_NUM "/dev/ttyS0"
#endif

#define HANDLER_UART_BAUD_RATE 115200
#define HANDLER_UART_TIMEOUTS 1000

#define UART_RECV_MAX_LINES 30
#define UART_RECV_MAX_LINES_LENGTH 100
#define UART_RECV_MAX_MSG_LENGTH 200

typedef struct
{
    long type; // 消息类型
    int len;
    unsigned char buffer[UART_RECV_MAX_MSG_LENGTH];
} uart_msg_t;

static int dev_uart_fd;
static uart_dev_t dev;
static CONNECT_STATE_E uart_state = CONNECTING;

// 消息队列id
static int msgid;
// 互斥量
static pthread_mutex_t mutex;
// 线程
static pthread_t uart_readthread;   // 读取串口接收数据，发送到消息队列
static pthread_t uart_handlethread; // 读取消息队列数据，进行处理

static void read_uart_msg_to_queue()
{
    int ret = 1;
    // unsigned char hex_array[] = {0xaa, 0x03, 0x04, 0x00, 0x55};
    uart_msg_t msg;
    msg.type = 1;
    msg.len = UART_RECV_MAX_MSG_LENGTH;
    ret = em_hal_uart_read(dev_uart_fd, msg.buffer, msg.len, HANDLER_UART_TIMEOUTS);
    if (ret >= 0)
    {
        msg.len = ret;
        msg.buffer[msg.len] = '\0';

        // 串口接收
        // printf("receive msg len %d, HEX data:  \n", ret);
        // for (int i = 0; i < ret; i++)
        // {
        //     printf(" %02X", (unsigned char)msg.buffer[i]);
        // }
        // printf("  , buffer size: %ld \n", sizeof(msg.buffer));
        

        ret = msgsnd(msgid, &msg, msg.len + sizeof(int), 0);
        if (ret == -1)
        {
            printf("queue msgsnd error");
            return;
        }
    }
}

/**
 * 串口消息处理
 */
static void handle_uart_msg()
{
    int ret;
    uart_msg_t msg;
    memset(msg.buffer, 0, sizeof(msg.buffer));
    ret = msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 1, 0);
    if (ret == -1)
    {
        printf("queue msgrcv error");
        return;
    }
    msg.buffer[msg.len] = '\0';

    // printf("receive msg len %d, HEX data:  \n", ret);
    // for (int i = 0; i < msg.len; i++)
    // {
    //     printf(" %02X", (unsigned char)msg.buffer[i]);
    // }
    // printf("  , buffer size: %ld \n", sizeof(msg.buffer));

    // 判断帧头
    if(msg.buffer[0] != 0xAA || msg.buffer[1] != 0x55){
        printf("Invalid frame header\n");
        return;
    }

    uint8_t type = msg.buffer[2];
    uint8_t value = msg.buffer[3];

    printf("Frama type = 0x%02X, value = 0x%02X\n", type, value);

    if (type == 0x05)
    {
        if (value == 0x01)
        {
            lv_img_set_src(bg_img,GET_IMAGE_PATH("g_bg_white.png"));
        }else if (value == 0x02)
        {
            lv_img_set_src(bg_img,GET_IMAGE_PATH("g_bg_black.png"));
        }
        
    }

    if (type == 0x06)
    {
        if (value == 0x01)
        {
            lv_label_set_text(wheel_mode,"经济");
        }else if (value == 0x02)
        {
            lv_label_set_text(wheel_mode,"标准");
        }else if (value == 0x03)
        {
            lv_label_set_text(wheel_mode,"运动");
        }else if (value == 0x04)
        {
            lv_label_set_text(wheel_mode,"助力");
        }else if (value == 0x05)
        {
            lv_label_set_text(wheel_mode,"X");
        }
    }

    if (type == 0x07)
    {
        if (value == 0x01)
        {
            lv_label_set_text(wheel_mode,"经济");
        }else if (value == 0x02)
        {
            lv_label_set_text(wheel_mode,"标准");
        }else if (value == 0x03)
        {
            lv_label_set_text(wheel_mode,"运动");
        }else if (value == 0x04)
        {
            lv_label_set_text(wheel_mode,"助力");
        }else if (value == 0x05)
        {
            lv_label_set_text(wheel_mode,"X");
        }
    }

}

// 串口读数据线程函数
static void *uart_readthread_function(void *arg)
{
    printf("uart mesh readthread_function run!\n");
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while (1)
    {
        read_uart_msg_to_queue();
        usleep(10);
    }
    pthread_exit(NULL);
}

// 串口数据处理线程
static void *uart_handlethread_function(void *arg)
{
    printf("uart mesh handlethread_function run!\n");
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    // 1. 与 MCU 通讯 初始化
    // init_at_device();
    while (1)
    {
        handle_uart_msg();
        usleep(10);
    }
    pthread_exit(NULL);
}

void uart_handler_init()
{
    int ret = 0;
    dev.speed = HANDLER_UART_BAUD_RATE;
    dev.path = HANDLER_UART_NUM;
    dev.bits = 8;
    dev.parity = 'N';
    dev.stop = 1;
    dev_uart_fd = em_hal_uart_open(&dev);

    // 创建消息队列
    msgid = msgget((key_t)123, IPC_CREAT | 0666);
    // 创建互斥量
    ret = pthread_mutex_init(&mutex, NULL);
    if (ret != 0)
    {
        printf("Error: pthread_mutex_init() failed\n");
        return;
    }
    // 创建接收和处理线程，接收和处理分离，避免处理耗时丢数据
    ret = pthread_create(&uart_readthread, NULL, uart_readthread_function, NULL);
    if (ret != 0)
    {
        printf("Error: pthread_create() uart_mesh_readthread failed\n");
        return;
    }
    ret = pthread_create(&uart_handlethread, NULL, uart_handlethread_function, NULL);
    if (ret != 0)
    {
        printf("Error: pthread_create() uart_mesh_handlethread failed\n");
        return;
    }
}