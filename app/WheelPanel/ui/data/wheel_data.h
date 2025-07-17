#ifndef _WHEEL_DATA_H_
#define _WHEEL_DATA_H_

#include <stdint.h>
#include <stdbool.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================= 数据结构定义 ======================= */
typedef struct {
    uint8_t _wheel_mode;            // 0x06 整车驱动模式
    uint8_t _speed_level;           // 0x07 速度档位
}wheel_data_t;

wheel_data_t g_wheel_data;

/* ======================= 绑定结构定义 ======================= */
typedef struct 
{
    uint8_t type;
    uint8_t *value_ptr;

    lv_obj_t *lv_obj;

    const char **value_str_table;
    uint8_t * value_len;
}type_binding_t;

/* 字符串表 */
static const char *wheel_mode_str[] = {
    "经济", "标准", "运动", "助力", "自定义"
};

static const char *_speed_level_str[] = {
    "低速", "中速", "高速"
};

typedef union 
{
    uint8_t raw[1];    // 用于整体首发；
    struct 
    {
        uint8_t low_beam        :1;     // bit0 近光
        uint8_t high_beam       :1;     // bit1 远光
        uint8_t turn_left       :1;     // bit2 左传灯
        uint8_t ture_right      :1;     // bit3 右转灯
        uint8_t position_light  :1;     // bit4 位置灯
        uint8_t reserved        :3;     // bit5~7 保留

    } bits;
    
}wheel_state_t;

type_binding_t g_bindings[] = {
    {0x06, &g_wheel_data._wheel_mode, NULL, wheel_mode_str, 5},
    {0x07, &g_wheel_data._wheel_mode, NULL, wheel_mode_str, 3},
};

#ifdef __cplusplus
}
#endif

#endif