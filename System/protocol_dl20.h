/**
  ******************************************************************************
  * @file    protocol_dl20.h
  * @author  MCU Code Master
  * @version V2.1 (Host)
  * @brief   主机端DL20通信协议的头文件.
  * - V2.0: 结构体与 app_globals.h (V2.0) 同步.
  * - V2.1: 临时移除 gas_ppm 以兼容旧版10字节负载的从机协议.
  ******************************************************************************
  */
#ifndef __PROTOCOL_DL20_H
#define __PROTOCOL_DL20_H

#include "app_globals.h"

/*==================================================================================================
 * 协议常量定义 (Constants)
 *==================================================================================================*/
#define DL20_FRAME_SOF                  (0xAA)
#define DL20_FRAME_EOF                  (0x55)

/*==================================================================================================
 * 功能码定义 (Function Codes)
 *==================================================================================================*/
#define FUNC_REPORT_SENSOR_DATA         (0x01)
#define FUNC_MASTER_CMD_ALARM           (0x20)
#define FUNC_MASTER_CMD_FAN             (0x21)

/*==================================================================================================
 * 协议帧的数据区 (Payload) 结构体定义
 *==================================================================================================*/

#pragma pack(1)

/**
  * @brief 功能码 0x01 (上报传感器数据) 的数据区结构.
  */
typedef struct {
    int16_t  temperature;
    u16      humidity;
    u16      co_ppm;
    u16      smog_ppm;
    // u16      gas_ppm;  // 临时移除以匹配10字节负载
    u16      aqi_ppm;
} SensorPayload_t;

/**
  * @brief 功能码 0x20 (主机控制报警器) 的数据区结构.
  */
typedef struct {
    u8 alarm_on;
} AlarmCommandPayload_t;

/**
  * @brief 功能码 0x21 (主机控制风扇) 的数据区结构.
  */
typedef struct {
    u8 fan_speed;
} FanCommandPayload_t;

#pragma pack()

/*==================================================================================================
 * 协议层 API 函数声明 (Public Functions)
 *==================================================================================================*/
void DL20_Init(void);
int DL20_ParseReceivedData(SystemData_t* system_data);
u16 DL20_PackAlarmCommand(u8 slave_id, u8 alarm_on, u8* buffer);
u16 DL20_PackFanCommand(u8 slave_id, u8 fan_speed, u8* buffer);

#endif /* __PROTOCOL_DL20_H */

