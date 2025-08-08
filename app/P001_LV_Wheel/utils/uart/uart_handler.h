#ifndef _UART_HANDLER_H_
#define _UART_HANDLER_H_

#include "device_data.h"

/***********************************宏定义***********************************/

#ifdef SIMULATOR_LINUX
#define HANDLER_UART_NUM "/dev/ttyUSB0"
#else
#define HANDLER_UART_NUM "/dev/ttyS0"
#endif

#define	CRC_FLAG 0											//是否开启CRC校验

#define HANDLER_UART_BAUD_RATE 115200						//数据串口波特率
#define HANDLER_UART_TIMEOUTS 1000							//接收超时时间

#define VEHICLE 		sizeof(recv_Vehicle_data)			//车辆信息接收数据长度
#define LED 			sizeof(recv_LED_data)				//指示灯信息接收数据长度
#define WARN 			sizeof(recv_warn_data)				//故障信息接收数据长度
#define MAX_RECV_COUNT 	(VEHICLE+LED+WARN+3)				//接收数据总长度

/***********************************结构体定义***********************************/

//串口信息结构体
typedef struct uart_hardware_cfg
{
    unsigned int baudrate; 		/* 波特率 */
    unsigned char dbit;    		/* 数据位 */
    char parity;           		/* 奇偶校验 */
    unsigned char sbit;    		/* 停止位 */
} uart_cfg_t;

//车辆信息  
union  gui_Vehicle_Info_state_t
{
	uint8_t data[32];
	struct
	{
		uint32_t	VehicleSpeed	 				:	16;	//车速显示(km/h)
        uint32_t	TripMileage						:	16;	//小计里程(km)(10倍)
        //
        uint32_t	OdoMileage		 				:	32;	//总里程(km)
        //
		uint32_t	SurplusMileage	  				:	16;	//续航里程(km)
        uint32_t	DriveModeSwitchStatus			:	 8;	//驾驶模式         //0x00:经济 0x01:标准 0x02:运动 0x03:助力 0x04:自定义
        uint32_t    ThemeModeSwitchStatus           :	 4;	//主题模式         //0x00:白天 0x01:黑夜
        uint32_t    NitroBoosterStatus              :	 4;	//氮气加速模式      //0x00:未启动 0x01:启动
        //
        uint32_t	MainSOC							:	16;	//电量%
        uint32_t	MainPowerAverage				:	16;	//平均电耗(Kwh/100km)(10倍)
        //
        uint32_t	InsDisChargePower			    :	16;	//瞬时功率
		uint32_t	SurplusPower				    :	16;	//剩余功率
        //
        uint32_t    CurrentChargingPower            :   16; //当前充电功率
        uint32_t	RemainChrgTime				    :	10;	//充电剩余时间
        uint32_t	GearPosition					:	 4;	//档位显示 		 //F 无效显示“--”，P:0x0 R:0x1 N:0x2 D:0x3
		uint32_t	TireID			   				:	 2;	//轮位(0~1)		//0:前轮 1:后轮
        //
		uint32_t	TirePressure	  				:	16;	//胎压显示(TPMS)(10倍) 
		uint32_t	TireTemperature		 			:	 8;	//胎压温度(℃)
		uint32_t	TimeMinute						:	 8;	//时间分钟
        //
		uint32_t	TimeHour						:	 8;	//时间小时
        uint32_t	RSV								:	24;	//RSV

	}__attribute__((packed)) bit;
};


//指示灯
union	gui_LED_state_t
{
	uint8_t data[3];
	struct 
	{
		//
		uint8_t		LeftTurnLightSts				    :	1;	//左转向灯
		uint8_t		RightTurnLightSts				    :	1;	//右转向灯
		uint8_t		HighLightSts					    :	1;	//远光灯
		uint8_t		LowLightSts					        :	1;	//近光灯
		uint8_t		PositionLightSts				    :	1;	//示宽灯(位置灯)
		uint8_t		DaytimeRunningLightSts		        :	1;	//日行灯(昼行灯)
		uint8_t		ReadyLightSts			            :	1;	//Ready指示灯
        uint8_t		TPMSLightSts						:	1;	//胎压故障指示灯
		//
        uint8_t		AutoHoldLightSts				    :	1;	//AutoHold功能状态指示灯
        uint8_t		LogoLightSts				        :	1;	//Logo灯
        uint8_t		SideShoringLightSts				    :	1;	//边撑灯
        uint8_t		LocationLightSts				    :	1;	//定位指示灯
        uint8_t		NetSignalLightSts				    :	1;	//网络信号显示
        uint8_t		CenControlConnectLightSts			:	1;	//中控连接指示
        uint8_t		CruiseLightSts				        :	1;	//定速巡航指示灯
        uint8_t		AutoLowLightSts				        :	1;	//自动近光灯
		//
        uint8_t		LowBatteryLightSts				    :	1;	//低电量报警灯
        uint8_t		PowerSystemErrLightSts				:	1;	//动力系统故障灯
        uint8_t		MotorErrLightSts				    :	1;	//电机故障灯
        uint8_t		BatteryHeatLightSts				    :	1;	//动力电池热失控指示灯
        uint8_t		ChargeLightSts				        :	1;	//充电故障指示灯
		uint8_t		RSV									:   3;	//RSV
	}__attribute__((packed)) bit;
};

//故障信息，文字提示
union gui_warn_state_t
{
	uint8_t data[3];
	struct
	{
		//
		uint8_t		ReadyWarn							:	1;	//整车未READY
		uint8_t		TPMSNoLearningSts					:	1;	//胎压传感器未学习
		uint8_t		TPMSUnderPressureSts				:	1;	//胎压过低
		uint8_t		TPMSOverPressureSts					:	1;	//胎压过高
		uint8_t		TPMSTireLeakageSts					:	1;	//胎压快速漏气
		uint8_t		TPMSSystemFaultSts					:	1;	//胎压系统故障
		uint8_t		TPMSSensorLostSts					:	1;	//胎压传感器丢失
		uint8_t		TPMSBatteryPwrLowSts				:	1;	//胎压传感器电池电量低
		//
		uint8_t		EngineReleaseSig					:	1;	//防盗认证失败
		uint8_t		LowBatteryWarn						:	1;	//动力电池电量低
		uint8_t		CruiseActiveSts						:	1;	//定速巡航已激活
		uint8_t		MotorErrSts							:	1;	//驱动电机故障
        uint8_t		LocationWarn				    	:	1;	//定位失败
        uint8_t		NetSignalWarn				    	:	1;	//网络连接失败
        uint8_t		CenControlConnectWarn				:	1;	//中控连接失败
        uint8_t		PowerSystemErrWarn					:	1;	//动力系统故障灯
		//
        uint8_t		MotorErrWarn				    	:	1;	//驱动电机故障
        uint8_t		BatteryHeatWarn				    	:	1;	//动力电池热失控
		uint8_t		ChargingWarn						:	1;	//充电中
		uint8_t		ChargeCompleteWarn					:	1;	//充电完成
		uint8_t		ChargeFaultWarn						:	1;	//充电故障
		uint8_t		RSV									:	3;	//RSV
	}__attribute__((packed)) bit;
};

/***********************************全局变量***********************************/

extern pthread_mutex_t mutex_lvgl;							//lvgl线程互斥锁
extern union  gui_Vehicle_Info_state_t 	recv_Vehicle_data;	//车辆信息
extern union  gui_LED_state_t 			recv_LED_data;		//指示灯
extern union  gui_warn_state_t 			recv_warn_data;		//故障信息  文字提示

extern bool main_flag;

/***********************************函数声明***********************************/

void uart_handler_init(void);           // 初始化串口处理功能

#endif

