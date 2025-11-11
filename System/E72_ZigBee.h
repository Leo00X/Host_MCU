#ifndef __E72_ZIGBEE_H
#define __E72_ZIGBEE_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "ht32.h"
#include "bsp_uart.h" // 包含你已有的UART驱动头文件
#include <stdbool.h>

/*
 ********************************************************************************************************
 * 串口功能移植宏定义
 * (根据你的 bsp_uart.c/h 文件进行配置)
 ********************************************************************************************************
 */

/**
 * @brief E72 模组所连接的串口。
 * @note  BSP_USART_PORT 已经在 bsp_uart.h 中定义为 HT_UART1
 */
#define E72_UART_PORT                   (BSP_USART_PORT)

/**
 * @brief 初始化 E72 模组所用的串口。
 */
#define E72_UART_INIT()                 BSP_UART_Init()

/**
 * @brief E72 模组串口发送单个字节。
 * @note  (已修正) 适配 bsp_uart.c, 假设 BSP_UART_SendByte 只需1个参数 (data)，
 * 并在其内部硬编码了 BSP_USART_PORT。
 */
#define E72_UART_SEND_BYTE(data)        BSP_UART_SendByte(data)

/**
 * @brief E72 模组串口注册接收回调函数。
 */
#define E72_UART_REGISTER_RX_CALLBACK(cb) BSP_UART_Register_RX_Callback(cb)


/*
 ********************************************************************************************************
 * E72 HEX 协议帧结构宏定义 (根据 亿佰特ZigBee3.0模组HEX命令标准规范_V1.7.pdf P6)
 ********************************************************************************************************
 */
#define E72_FRAME_HEADER                (0x55)    // 帧头
#define E72_MIN_FRAME_LEN               (5)       // 最短帧: 55 + Len(1) + Type(1) + Cmd(2) + FCS(1) = 6字节 (Len=3)
#define E72_MAX_PAYLOAD_LEN             (250)     // 估算的最大Payload长度
#define E72_MAX_FRAME_LEN               (E72_MAX_PAYLOAD_LEN + 6) // 估算的最大帧长度 (Header + Len + Type + Cmd_H + Cmd_L + ...Payload + FCS)


/*
 ********************************************************************************************************
 * E72 命令类型 (Type) (规范 P8)
 ********************************************************************************************************
 */
#define E72_CMD_TYPE_QUERY              (0x01)    // 查询命令 (MCU -> 模组)
#define E72_CMD_TYPE_SET                (0x02)    // 设置命令 (MCU -> 模组)
#define E72_CMD_TYPE_COMMON_RSP         (0x80)    // 通用应答 (模组 -> MCU)
#define E72_CMD_TYPE_QUERY_RSP          (0x81)    // 查询应答 (模组 -> MCU)
#define E72_CMD_TYPE_SET_RSP            (0x82)    // 设置应答 (模组 -> MCU)
#define E72_CMD_TYPE_NOTIFY             (0x83)    // 主动上报 (模组 -> MCU)
#define E72_CMD_TYPE_PASSTHROUGH        (0x0A)    // 透传发送 (MCU -> 模组)
#define E72_CMD_TYPE_PASSTHROUGH_RSP    (0x8A)    // 透传接收 (模组 -> MCU)

/*
 ********************************************************************************************************
 * E72 常用命令码 (Cmd ID) (规范 P9+)
 ********************************************************************************************************
 */
#define E72_CMD_ID_RESET                (0x0000)  // 重启模组
#define E72_CMD_ID_GET_VERSION          (0x0001)  // 读取版本号
#define E72_CMD_ID_GET_STATUS           (0x0002)  // 查询模组当前状态
#define E72_CMD_ID_SET_UART_BAUD        (0x0010)  // 设置串口波特率
#define E72_CMD_ID_SET_WORK_MODE        (0x0020)  // 设置工作模式 (透传/HEX)
#define E72_CMD_ID_PERMIT_JOIN          (0x0030)  // 允许/禁止入网
#define E72_CMD_ID_GET_LOCAL_INFO       (0x0040)  // 读取本地网络信息
#define E72_CMD_ID_PASSTHROUGH_DATA     (0x0001)  // 透传数据 (注意：此时Type=0x0A或0x8A)


/*
 ********************************************************************************************************
 * 上层应用回调函数指针类型定义
 ********************************************************************************************************
 */

/**
 * @brief 接收到一帧完整且校验通过的E72 HEX数据的回调函数。
 * @param u8Type:     帧类型 (E72_CMD_TYPE_*)
 * @param u16Cmd:     命令ID (E72_CMD_ID_*)
 * @param pPayload:   指向Payload数据的指针
 * @param u8PayloadLen: Payload的长度
 */
typedef void (*e72_frame_rx_callback_t)(u8 u8Type, u16 u16Cmd, const u8* pPayload, u8 u8PayloadLen);


/*
 ********************************************************************************************************
 * E72 驱动核心功能函数原型
 ********************************************************************************************************
 */

/**
 * @brief 初始化E72 Zigbee驱动。
 * @note  此函数将初始化硬件UART并注册内部的串口接收回调。
 * @retval 无
 */
void E72_Init(void);

/**
 * @brief 注册一个上层回调函数，当收到E72模组的完整数据帧时将调用该函数。
 * @param callback: 指向回调函数的指针。
 * @retval 无
 */
void E72_Register_Frame_Callback(e72_frame_rx_callback_t callback);

/**
 * @brief (新) 向Zigbee网络中的指定节点发送“透传”数据。
 * @note  这封装了 E72_CMD_TYPE_PASSTHROUGH (0x0A) 和 E72_CMD_ID_PASSTHROUGH_DATA (0x0001)
 *
 * @param u16DestAddr:  目标设备的短地址 (例如: 0x1234, 广播: 0xFFFF)
 * @param u8Endpoint:   目标端口 (通常是 0x01)
 * @param pData:        指向要发送的“应用层”数据 (即你的 0xAA...0x55 帧)
 * @param u8DataLen:    应用层数据的长度
 * @retval 无
 */
void E72_SendTransparentData(u16 u16DestAddr, u8 u8Endpoint, const u8* pData, u8 u8DataLen);

/**
 * @brief (底层) 向E72模组发送一帧HEX指令。
 * @param u8Type:     帧类型 (E72_CMD_TYPE_*)
 * @param u16Cmd:     命令ID (E72_CMD_ID_*)
 * @param pPayload:   指向Payload数据的指针 (如果无Payload，请传入 NULL)
 * @param u8PayloadLen: Payload的长度 (如果无Payload，请传入 0)
 * @retval 无
 */
void E72_SendHexCommand(u8 u8Type, u16 u16Cmd, const u8* pPayload, u8 u8PayloadLen);


#ifdef __cplusplus
}
#endif

#endif /* __E72_ZIGBEE_H */
