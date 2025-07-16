#ifndef _EM_HAL_UART_H_
#define _EM_HAL_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct _uart_dev_t{
    char *path;
    int speed;
    int bits;
    char parity;
    int stop;
} uart_dev_t;

// 打开串口(阻塞模式)
int em_hal_uart_open(uart_dev_t *dev);
// 关闭串口
int em_hal_uart_close(int fd);
int em_hal_uart_write(int fd, const char *send_buf, int lenth);
int em_hal_uart_read(int fd,char *rcv_buf, int lenth, int timeout);

#ifdef __cplusplus
}
#endif

#endif

