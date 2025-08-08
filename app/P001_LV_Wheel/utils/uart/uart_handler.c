#define _GNU_SOURCE
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
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>


#include "lvgl.h"
#include "ui.h"


/***********************************全局变量初始化***********************************/

union  gui_Vehicle_Info_state_t recv_Vehicle_data;
union  gui_LED_state_t recv_LED_data;
union  gui_warn_state_t recv_warn_data;

bool main_flag = false;

/***********************************静态变量初始化***********************************/

static char * TAG = "UART";

static int dev_uart_fd;
static struct termios old_cfg;  // 用于保存终端的配置参数
static uart_dev_t dev;

static pthread_t uart_handlethread; // UI数据处理线程

// 互斥锁
pthread_mutex_t mutex_lvgl = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_data = PTHREAD_MUTEX_INITIALIZER;


/***********************************静态函数声明***********************************/

static void data_copy(uint8_t *data,uint8_t *copy_to_data,int start,int end);   //将data中从位置start开始到end结束的数据拷贝到copy_to_data里
static uint8_t CRC8_SAEJ1850_CAL(uint8_t buf[], uint8_t len);                   //CRC校验值计算
static void async_io_init(void);                                                //异步 I/O 初始化函数


/***********************************函数定义***********************************/

/**
 * 串口配置
 * @param cfg 一个 uart_cfg_t 结构体对象
 * @param fd 串口终端对应的文件描述符
 * @return 配置成功:0 配置失败:-1
 */
int uart_cfg(const uart_cfg_t *cfg,int fd)
{
    struct termios new_cfg = {0}; // 将 new_cfg 对象清零
    speed_t uart_speed;
    /* 设置为原始模式 */
    cfmakeraw(&new_cfg);
    /* 使能接收 */
    new_cfg.c_cflag |= CREAD;
    /* 设置波特率 */
    switch (cfg->baudrate)
    {
    case 1200:
        uart_speed = B1200;
        break;
    case 1800:
        uart_speed = B1800;
        break;
    case 2400:
        uart_speed = B2400;
        break;
    case 4800:
        uart_speed = B4800;
        break;
    case 9600:
        uart_speed = B9600;
        printf("%s:baud rate: 9600\n",TAG);
        break;
    case 19200:
        uart_speed = B19200;
        printf("%s:baud rate: 19200\n",TAG);
        break;
    case 38400:
        uart_speed = B38400;
        printf("%s:baud rate: 38400\n",TAG);
        break;
    case 57600:
        uart_speed = B57600;
        printf("%s:baud rate: 57600\n",TAG);
        break;
    case 115200:
        uart_speed = B115200;
        printf("%s:baud rate: 115200\n",TAG);
        break;
    case 230400:
        uart_speed = B230400;
        printf("%s:baud rate: 230400\n",TAG);
        break;
    case 460800:
        uart_speed = B460800;
        printf("%s:baud rate: 460800\n",TAG);
        break;
    case 500000:
        uart_speed = B500000;
        break;
    default: // 默认配置为 115200
        uart_speed = B115200;
        printf("%s:default baud rate: 115200\n",TAG);
        break;
    }
    if (0 > cfsetspeed(&new_cfg, uart_speed))
    {
        printf("%s:cfsetspeed error\n",TAG);
        return -1;
    }
    /* 设置数据位大小 */
    new_cfg.c_cflag &= ~CSIZE; // 将数据位相关的比特位清零
    switch (cfg->dbit)
    {
    case 5:
        new_cfg.c_cflag |= CS5;
        break;
    case 6:
        new_cfg.c_cflag |= CS6;
        break;
    case 7:
        new_cfg.c_cflag |= CS7;
        break;
    case 8:
        new_cfg.c_cflag |= CS8;
        break;
    default: // 默认数据位大小为 8
        new_cfg.c_cflag |= CS8;
        printf("%s:default data bit size: 8\n",TAG);
        break;
    }
    /* 设置奇偶校验 */
    switch (cfg->parity)
    {
    case 'N': // 无校验
        new_cfg.c_cflag &= ~PARENB;
        new_cfg.c_iflag &= ~INPCK;
        break;
    case 'O': // 奇校验
        new_cfg.c_cflag |= (PARODD | PARENB);
        new_cfg.c_iflag |= INPCK;
        break;
    case 'E': // 偶校验
        new_cfg.c_cflag |= PARENB;
        new_cfg.c_cflag &= ~PARODD; /* 清除 PARODD 标志，配置为偶校验 */
        new_cfg.c_iflag |= INPCK;
        break;
    default: // 默认配置为无校验
        new_cfg.c_cflag &= ~PARENB;
        new_cfg.c_iflag &= ~INPCK;
        printf("%s:default parity: N\n",TAG);
        break;
    }
    /* 设置停止位 */
    switch (cfg->sbit)
    {
    case 1: // 1 个停止位
        new_cfg.c_cflag &= ~CSTOPB;
        break;
    case 2: // 2 个停止位
        new_cfg.c_cflag |= CSTOPB;
        break;
    default: // 默认配置为 1 个停止位
        new_cfg.c_cflag &= ~CSTOPB;
        printf("%s:default stop bit size: 1\n",TAG);
        break;
    }
    /* 将 MIN 和 TIME 设置为 0 */
    new_cfg.c_cc[VTIME] = 0;//单位：十分之一秒及100ms
    new_cfg.c_cc[VMIN] = 0;//MAX_RECV_COUNT
    
    /* 清空缓冲区 */
    // if (0 >ioctl(fd, TCFLSH, 2))    //清空输入输出缓存
    if (0 > tcflush(fd, TCIOFLUSH))
    {
        printf("%s:tcflush error\n",TAG);
        return -1;
    }
    /* 写入配置、使配置生效 */
    if (0 > tcsetattr(fd, TCSANOW, &new_cfg))
    {
        printf("%s:tcsetattr erro\n",TAG);
        return -1;
    }
    /* 配置 OK 退出 */
    return 0;
}

/**
 * 将data中从位置start开始到end结束的数据拷贝到copy_to_data里
 * @param data 拷贝数据的对象
 * @param copy_to_data 拷贝数据的目标对象
 * @param start 数据的起始位置（下标）
 * @param end 数据的结束位置（下标）
 */
static void data_copy(uint8_t *data,uint8_t *copy_to_data,int start,int end)
{
    int n = 0;
    for(start;start<=end;start++)
    {
        copy_to_data[n] = data[start];
        n++;
    }
}

/**
 * CRC校验值计算
 * @param buf 需要CRC校验的数据数组
 * @param len 数组中的数据长度
 * @return 计算出的CRC校验值
 */
static uint8_t CRC8_SAEJ1850_CAL(uint8_t buf[], uint8_t len)
{
    uint8_t i, j;
    uint8_t u8_poly;
    uint8_t u8_crc8;

    u8_crc8 = 0xFF;
    u8_poly = 0x1D;

    for (i = 0; i < len; i++)
    {
        u8_crc8 ^= buf[i];
        for (j = 0; j < 8; j++)
        {
            if ((u8_crc8 & 0x80) != 0)
            {
                u8_crc8 <<= 1;
                u8_crc8 ^= u8_poly;
            }
            else
            {
                u8_crc8 <<= 1;
            }
        }
    }
    u8_crc8 ^= (uint8_t)0xFF;
    return u8_crc8;
}

/**
 * 车辆信息处理函数
 */
void Vehicle_func()
{
    /*车速显示*///recv_Vehicle_data.bit.VehicleSpeed
    int recv_speed = (int)recv_Vehicle_data.bit.VehicleSpeed;
    lv_label_set_text_fmt(ui_Label_speed_value, "%d" ,recv_speed);

    /*里程显示(总里程)*///recv_Vehicle_data.bit.OdoMileage
        if(recv_Vehicle_data.bit.OdoMileage==0xFFFFFFFF)
        {
            lv_label_set_text(ui_Label_odo_value, "00000");
        }
        else
        {
            int odo_value = (int)recv_Vehicle_data.bit.OdoMileage;
            if(odo_value>99999)
            {
                odo_value = 99999;
            }
            lv_label_set_text_fmt(ui_Label_odo_value,"%d",odo_value);
        }

    /*里程显示(小计里程)*///recv_Vehicle_data.bit.TripMileage
        // printf("TripMileage--0x%x\n",recv_Vehicle_data.bit.TripMileage);
        if(recv_Vehicle_data.bit.TripMileage==0xFFFF)
        {
            lv_label_set_text(ui_Label_trip_value,"000.0");
        }
        else
        {
            int trip_value = (int)recv_Vehicle_data.bit.TripMileage;
            lv_label_set_text_fmt(ui_Label_trip_value,"%d.%01d",trip_value/10,trip_value%10);
        }
    
    /*续航里程*///recv_Vehicle_data.bit.SurplusMileage
        // printf("SurplusMileage--0x%x\n",recv_Vehicle_data.bit.SurplusMileage);
        if(recv_Vehicle_data.bit.SurplusMileage==0xFFFF)
        {
            lv_label_set_text(ui_Label_value_endurance_mileage, "000 km");
        }
        else
        {
            int endurance_mileage_value = (int)recv_Vehicle_data.bit.SurplusMileage;
            if(endurance_mileage_value>999)
            {
                endurance_mileage_value = 999;
            }
            lv_label_set_text_fmt(ui_Label_value_endurance_mileage,"%d km",endurance_mileage_value);
        }

    /*主题模式*///recv_Vehicle_data.bit.ThemeModeSwitchStatus
        // printf("ThemeModeSwitchStatus--0x%x\n",recv_Vehicle_data.bit.ThemeModeSwitchStatus);
        if(recv_Vehicle_data.bit.ThemeModeSwitchStatus==0x0)
        {
            lv_obj_add_state(ui_Button_test, LV_STATE_CHECKED);
            lv_event_send(ui_Button_test, LV_EVENT_VALUE_CHANGED, NULL);
        }
        else
        {
            lv_obj_clear_state(ui_Button_test, LV_STATE_CHECKED);
            lv_event_send(ui_Button_test, LV_EVENT_VALUE_CHANGED, NULL);
        }
}

/**
 * 指示灯信息处理函数
 */
void LED_func()
{
    /*左转向灯*/if(recv_LED_data.bit.LeftTurnLightSts){lv_obj_clear_flag( ui_Image_left, LV_OBJ_FLAG_HIDDEN);}
            else{lv_obj_add_flag( ui_Image_left, LV_OBJ_FLAG_HIDDEN);}
    /*右转向灯*/if(recv_LED_data.bit.RightTurnLightSts){lv_obj_clear_flag( ui_Image_right, LV_OBJ_FLAG_HIDDEN);}
            else{lv_obj_add_flag( ui_Image_right, LV_OBJ_FLAG_HIDDEN);}
}

/**
 * 故障信息处理函数
 */
void Warn_func()
{
    // /*整车未READY 1*/if(recv_warn_data.bit.ReadyWarn){lv_label_set_text(ui_Label_warning_info, "整车未READY!");}
    // /*胎压传感器未学习 2*/if(recv_warn_data.bit.TPMSNoLearningSts){lv_label_set_text(ui_Label_warning_info, "胎压传感器未学习!");}
}

#define BUFFER_SIZE 128                     // 缓冲区大小
static uint8_t buffer[BUFFER_SIZE] = {0};   // 缓冲区
static int buffer_len = 0;                  // 缓冲区当前长度
static int packet_start = 0;                // 是否开始接收数据包

/**
 * 信号处理函数，当串口有数据可读时，会跳转到该函数执行
 */
static void io_handler(int sig, siginfo_t *info, void *context)
{
    uint8_t buf[MAX_RECV_COUNT] = {0};      // 临时缓冲区
    int ret = 0;
    int n;
    if (SIGRTMIN != sig)
        return;
    /* 判断串口是否有数据可读 */
    if(POLL_IN == info->si_code)
    {
        ret = em_hal_uart_read(dev_uart_fd, buf, MAX_RECV_COUNT, HANDLER_UART_TIMEOUTS); // 一次最多读字节数据
        // ret = read(dev_uart_fd, buf, MAX_RECV_COUNT); // 一次最多读字节数据
        
        // printf("%s:ret:%d,MAX_RECV_COUNT:%ld\n",TAG,ret,MAX_RECV_COUNT);
        do {
            pthread_mutex_lock(&mutex_data);

            if(ret==0 || ret != MAX_RECV_COUNT)
            {
                break;
            }

            if(buf[0] != 0xFF && buf[MAX_RECV_COUNT-1] != 0xEE)
            {
                break;
            }
#if CRC_FLAG
            uint8_t crc_value = CRC8_SAEJ1850_CAL(&buf[1],MAX_RECV_COUNT-3);
            if(crc_value != buf[MAX_RECV_COUNT-2])
            {
                printf("%s:crc error,crc value is 0x%hhx,recv crc value is 0x%hhx\n",TAG,crc_value,buf[MAX_RECV_COUNT-2]);
                break;
            }
#endif
            data_copy(buf,recv_Vehicle_data.data,1,VEHICLE);
            data_copy(buf,recv_LED_data.data,VEHICLE+1,VEHICLE+LED);
            data_copy(buf,recv_warn_data.data,VEHICLE+LED+1,VEHICLE+LED+WARN);

            // printf("%s:[ ",TAG);
            // for (n = 0; n < ret; n++)
            // {
            //     printf("0x%hhx ", buf[n]);
            // }
            // printf("]\n");
            main_flag = true;
        }while(0);

        pthread_mutex_unlock(&mutex_data);
    }
}

/**
 * 异步 I/O 初始化函数
 */
static void async_io_init(void)
{
    struct sigaction sigatn;
    /* 为实时信号 SIGRTMIN 注册信号处理函数 */
    sigatn.sa_sigaction = io_handler; // 当串口有数据可读时，会跳转到 io_handler 函数
    sigatn.sa_flags = SA_SIGINFO;
    sigemptyset(&sigatn.sa_mask);
    sigaction(SIGRTMIN, &sigatn, NULL);

    int flag;
    /* 使能异步 I/O */
    flag = fcntl(dev_uart_fd, F_GETFL); //先获取原来的 flag
    flag |= O_ASYNC;//将 O_ASYNC 标志添加到 flag ,使能异步 I/O 事件
    fcntl(dev_uart_fd, F_SETFL, flag);//重新设置 flag
    /* 设置异步 I/O 的所有者 */
    fcntl(dev_uart_fd, F_SETOWN, getpid());
    /* 指定实时信号 SIGRTMIN 作为异步 I/O 通知信号 */
    fcntl(dev_uart_fd, F_SETSIG, SIGRTMIN);
}

// UI数据处理线程
static void *uart_handlethread_function(void *arg)
{
    pthread_detach(pthread_self());//将本线程设置为分离属性
    while (1)
    {
        if(main_flag)
        {
            pthread_mutex_lock(&mutex_lvgl);
            Vehicle_func();//车辆信息
            LED_func();//指示灯
            Warn_func();//故障信息
            pthread_mutex_unlock(&mutex_lvgl);
            main_flag = false;
        }
        usleep(10);
    }
    pthread_exit(NULL);
}


void uart_handler_init(void)
{
    uart_cfg_t cfg = {0};
    char *device = HANDLER_UART_NUM;
    cfg.baudrate = HANDLER_UART_BAUD_RATE;
    printf("%s:dev:%s\n",TAG,device);
    if (NULL == device)
    {
        printf("%s:Error: the device must be set!\n",TAG);
        goto err;
    }
    /* 串口初始化 */
    /* 打开串口终端 */
    dev_uart_fd = open(device, O_RDWR | O_NOCTTY);
    if (0 > dev_uart_fd)
    {
        printf("%s:open error: %s\n",TAG, device);
        goto err;
    }
    /* 获取串口当前的配置参数 */
    if (0 > tcgetattr(dev_uart_fd, &old_cfg))
    {
        printf("%s:tcgetattr error\n",TAG);
        close(dev_uart_fd);
        goto err;
    }

    /* 串口配置 */
    if (uart_cfg(&cfg,dev_uart_fd))
    {
#if LOG_SWITCH
        printf("%s:uart_cfg error\n",TAG);
#endif
        tcsetattr(dev_uart_fd, TCSANOW, &old_cfg); // 恢复到之前的配置
        close(dev_uart_fd);
        goto err;
    }

    async_io_init();

    int ret = pthread_create(&uart_handlethread, NULL, uart_handlethread_function, NULL);
    if (ret != 0)
    {
        printf("%s:Error: pthread_create() uart_mesh_handlethread failed\n",TAG);
        goto err;
    }
    printf("%s:MAX_RECV_COUNT:%ld VEHICLE:%ld LED:%ld warn:%ld\n",TAG,MAX_RECV_COUNT,VEHICLE,LED,WARN);

err:
    // uart_handler_init();
    return;
}