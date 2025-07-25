#include <stdio.h>        //标准输入输出,如printf、scanf以及文件操作
#include <stdlib.h>        //标准库头文件，定义了五种类型、一些宏和通用工具函数
#include <unistd.h>        //定义 read write close lseek 等Unix标准函数
#include <sys/types.h>    //定义数据类型，如 ssiz e_t off_t 等
#include <sys/stat.h>    //文件状态
#include <fcntl.h>        //文件控制定义
#include <termios.h>    //终端I/O
#include <errno.h>        //与全局变量 errno 相关的定义
#include <getopt.h>        //处理命令行参数
#include <string.h>        //字符串操作
#include <time.h>        //时间
#include <sys/select.h>    //select函数
#include "em_hal_uart.h"


/**
 * @brief 
 * 
 * @param fd 
 * @param nSpeed 
 * @param nBits 
 * @param nParity 
 * @param nStop 
 * @return int 
 */
static int em_hal_setOpt(int fd, int nSpeed, int nBits, char nParity, int nStop)
{
    struct termios newtio, oldtio;

    // 保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息
    if (tcgetattr(fd, &oldtio) != 0)
    {
        printf("SetupSerial 1");
        return -1;
    }

    bzero(&newtio, sizeof(newtio));        //新termios参数清零
    newtio.c_cflag |= CLOCAL | CREAD;    //CLOCAL--忽略 modem 控制线,本地连线, 不具数据机控制功能, CREAD--使能接收标志
    // 设置数据位数
    newtio.c_cflag &= ~CSIZE;    //清数据位标志
    switch (nBits)
    {
        case 5:
            newtio.c_cflag |= CS5;
        break;
        case 6:
            newtio.c_cflag |= CS6;
        break;
        case 7:
            newtio.c_cflag |= CS7;
        break;
        case 8:
            newtio.c_cflag |= CS8;
        break;
        default:
            printf("Unsupported data size");
            return -1;
    }
    // 设置校验位
    switch (nParity)
    {
        case 'o':
        case 'O':                     //奇校验
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'e':
        case 'E':                     //偶校验
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'n':
        case 'N':                    //无校验
            newtio.c_cflag &= ~PARENB;
            break;
        default:
            printf("Unsupported parity");
            return -1;
    }
    // 设置停止位
    switch (nStop)
    {
        case 1:
            newtio.c_cflag &= ~CSTOPB;
        break;
        case 2:
            newtio.c_cflag |= CSTOPB;
        break;
        default:
            printf("Unsupported stop bits");
            return -1;
    }
    // 设置波特率 2400/4800/9600/19200/38400/57600/115200/230400
    switch (nSpeed)
    {
        case 1200:
            cfsetispeed(&newtio, B1200);
            cfsetospeed(&newtio, B1200);
            break;
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 19200:
            cfsetispeed(&newtio, B19200);
            cfsetospeed(&newtio, B19200);
            break;
        case 38400:
            cfsetispeed(&newtio, B38400);
            cfsetospeed(&newtio, B38400);
            break;
        case 57600:
            cfsetispeed(&newtio, B57600);
            cfsetospeed(&newtio, B57600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        case 230400:
            cfsetispeed(&newtio, B230400);
            cfsetospeed(&newtio, B230400);
            break;
        default:
            printf("Sorry, Unsupported baud rate, set default 9600!");
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }
    // 设置read读取最小字节数和超时时间
    newtio.c_cc[VTIME] = 1;     // 读取一个字符等待1*(1/10)s
    newtio.c_cc[VMIN] = 200;        // 读取字符的最少个数为1

    tcflush(fd,TCIFLUSH);         //清空缓冲区
    if (tcsetattr(fd, TCSANOW, &newtio) != 0)    //激活新设置
    {
        perror("SetupSerial 3");
            return -1;
    }
    return 0;
}

/**
 * @brief 
 * 
 * @param fd 
 * @param rcv_buf 
 * @param lenth 
 * @param timeout 
 * @return int 
 */
int em_hal_uart_read(int fd, char *rcv_buf, int lenth, int timeout)
{
    int len, fs_sel;
    fd_set fs_read;
    struct timeval time;

    time.tv_sec = timeout / 1000;              //set the rcv wait time
    time.tv_usec = timeout % 1000 * 1000;    //100000us = 0.1s

    FD_ZERO(&fs_read);        //每次循环都要清空集合，否则不能检测描述符变化
    FD_SET(fd, &fs_read);    //添加描述符

    // 超时等待读变化，>0：就绪描述字的正数目， -1：出错， 0 ：超时
    fs_sel = select(fd + 1, &fs_read, NULL, NULL, &time);
    if(fs_sel)
    {
        len = read(fd, rcv_buf, lenth);
        return len;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief 
 * 
 * @param fd 
 * @param send_buf 
 * @param lenth 
 * @return int 
 */
int em_hal_uart_write(int fd, const char *send_buf, int lenth)
{
    ssize_t ret = 0;

    ret = write(fd, send_buf, lenth);
    if (ret == lenth)
    {
        // printf("send data is %s", send_buf);
        return ret;
    }
    else
    {
        printf("write device error");
        tcflush(fd,TCOFLUSH);
        return -1;
    }
}

/**
 * @brief 
 * 
 * @param dev 
 * @return int 
 */
int em_hal_uart_open(uart_dev_t *dev)
{
    int fd;
    if(dev->path == NULL)
        return -1;
	//O_RDWR ： 可读可写
	//O_NOCTTY ：该参数不会使打开的文件成为该进程的控制终端。如果没有指定这个标志，那么任何一个 输入都将会影响用户的进程。
	//O_NDELAY ：这个程序不关心DCD信号线所处的状态,端口的另一端是否激活或者停止。如果用户不指定了这个标志，则进程将会一直处在睡眠状态，直到DCD信号线被激活。
    fd = open(dev->path, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
    {
        perror(dev->path);
        return -1;
    }
    // 设置串口阻塞， 0：阻塞， FNDELAY：非阻塞
    if (fcntl(fd, F_SETFL, 0) < 0)    //阻塞
         printf("fcntl failed!");

    if (isatty(fd) == 0)
    {
        printf("standard input is not a terminal device");
        close(fd);
        return -1;
    }
    else
    {
        printf("is a tty success!");
    }

    // 设置串口参数
    if (em_hal_setOpt(fd, dev->speed, dev->bits, dev->parity, dev->stop)== -1)    //设置8位数据位、1位停止位、无校验
    {
        printf("Set opt Error");
        close(fd);
        return -1;
    }

    tcflush(fd, TCIOFLUSH);    //清掉串口缓存
    return fd;
}

/**
 * @brief 
 * 
 * @param fd 
 * @return int 
 */
int em_hal_uart_close(int fd)
{
    return close(fd);
}

