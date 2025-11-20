/**
  ******************************************************************************
  * @file    protocol_dl20.h
  * @author  HT32 Code Expert
  * @version V3.1 (Host, Refactored for E72 & app_globals V2.x)
  * @brief   主机端DL20通信协议的头文件.
  * - V3.0: 重构，与 E72 驱动层解耦。
  * - V3.1: 保持 API 接口不变。
  ******************************************************************************
  */
#ifndef __PROTOCOL_DL20_H
#define __PROTOCOL_DL20_H

// 包含你的“数据大脑”头文件，它定义了u8, u16, 结构体等
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
 * (这些应与你 app_globals.h 中的 SlaveEnvData_t 匹配，或保持独立)
 *==================================================================================================*/

#pragma pack(1)

/**
  * @brief 功能码 0x01 (上报传感器数据) 的数据区结构.
  * @note  这必须与从机(STM32)发送的结构完全一致.
  */
typedef struct {
    int16_t  temperature;   // 放大10倍 (例如 25.5°C -> 255)
    u16      humidity;      // 放大10倍 (例如 60.2% -> 602)
    u16      co_ppm;
    u16      smog_ppm;
    u16      aqi_ppm;       // 你的V2.1注释说移除了gas_ppm，所以我这里也保持10字节
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
 * 协议层 API 函数声明 (Public API)
 *==================================================================================================*/

/**
 * @brief (新) 初始化 DL20 协议层。
 * @note  必须在主程序中调用一次，以重置内部接收状态机。
 * @retval 无
 */
void Protocol_Init(void);

/**
 * @brief (新) 处理来自E72驱动的原始应用数据。
 * @note  这是新的数据入口。E72回调函数将调用此函数。
 * @param pData: 指向 E72 解包后的应用数据 (即 0xAA...0x55 帧的开始)
 * @param u16Len: 数据长度
 * @retval 无
 */
void Protocol_ProcessRxData(const u8* pData, u16 u16Len);

/**
 * @brief (修改) 向指定从机发送风扇控制命令。
 * @param u16DestAddr:  目标从机的Zigbee短地址
 * @param fan_speed:  风扇速度
 * @retval 无
 */
void Protocol_SendFanCommand(u16 u16DestAddr, u8 fan_speed);

/**
 * @brief (修改) 向指定从机发送报警器控制命令。
 * @param u16DestAddr:  目标从机的Zigbee短地址
 * @param alarm_on:   报警器状态 (1: On, 0: Off)
 * @retval 无
 */
void Protocol_SendAlarmCommand(u16 u16DestAddr, u8 alarm_on);


#endif /* __PROTOCOL_DL20_H */
