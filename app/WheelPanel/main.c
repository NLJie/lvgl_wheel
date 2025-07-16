/*
 * @Author: xiaozhi 
 * @Date: 2024-09-24 23:32:16 
 * @Last Modified by: xiaozhi
 * @Last Modified time: 2024-09-25 02:49:32
 */
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include "lvgl.h"
#include "font_conf.h"
#include "page_conf.h"
#include "device_data.h"
#include "ble_mesh.h"
#include "uart_handler.h"
#include "utils.h"
#include "http_manager.h"
#include "audio_player_async.h"

extern void lv_port_disp_init(bool is_disp_orientation);
extern void lv_port_indev_init(void);

static uint32_t time_till_next = 1;

int main() {
    // 信号处理初始化
    // system_signal_init();
    // 初始化设备状态
    // init_device_state();
    // 初始化 LVGL 图形库
    lv_init();
    
    // 获取设备状态
    device_state_t *state = get_device_state();
    lv_port_disp_init(state->is_disp_orientation);
    lv_port_indev_init();
    // 初始化 字体资源
    FONT_INIT();
    // 启动设备定时器
    // device_timer_init();
    // 初始化异步音频播放功能
    // init_async_audio_player();
    // 启动 WI-FI 控制器
    // wpa_manager_open();
    // 启动一个HTTP 请求处理
    // http_request_create();
    // 初始化 蓝牙Mesh
    // ble_mesh_init();
    uart_handler_init();
    // 初始化主界面页面
    init_page_main();

    while (1) {
        time_till_next = lv_task_handler();
		usleep(time_till_next*1000);
    }

    return 0;
}
