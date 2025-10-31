#ifndef __USART_H
#define __USART_H

#include "ht32f5xxxx_usart.h"
#include "ht32f5xxxx_gpio.h"
#include "ht32_board.h"
#include <stdbool.h> // 新增：为了使用 bool 类型

//=================USART0================esp8266//
#define USART_GPIO_GROUP             GPIO_PA
#define USART_TX_PIN                 GPIO_PIN_2
#define USART_RX_PIN                 GPIO_PIN_3
#define USART_AFIO_MODE              (AFIO_FUN_USART_UART)
#define USARTX_PORT                   HT_USART0

//===============USART1================//HMI
#define USART1_GPIO_GROUP  GPIO_PA
#define USART1_TX_PIN      GPIO_PIN_4
#define USART1_RX_PIN      GPIO_PIN_5
#define USART1_AFIO_MODE   (AFIO_FUN_USART_UART)
#define USART1_PORT        HT_USART1
#define USART_DEBUG		     HT_USART1

/*协议格式定义：
帧头 (Start of Frame - SOF): 1字节，用于标记一个数据帧的开始。例如：0xA5
设备ID (Device ID): 1字节，用于区分不同的设备。
命令 (Command): 1字节，用于定义操作（开/关）。
帧尾 (End of Frame - EOF): 1字节，用于标记一个数据帧的结束。例如：0x5A
*/

//协议举例
//A502015A   开启窗帘

/* 串口屏通信协议定义 */
#define SCREEN_SOF_MARKER         (0xA5) // 帧起始标记 (Start Of Frame)
#define SCREEN_EOF_MARKER         (0x5A) // 帧结束标记 (End Of Frame)
#define SCREEN_FRAME_LENGTH       (4)    // 帧长度 (SOF, DEV_ID, CMD, EOF)

/* 设备ID定义 */
#define DEVICE_ID_BEEP            (0x01) // 警报
#define DEVICE_ID_CURTAIN         (0x02) // 窗帘
#define DEVICE_ID_FAN             (0x03) // 风扇
#define DEVICE_ID_HONGWAI         (0x04) // 防盗 (红外)
#define DEVICE_ID_K210            (0x05) // 摄像头 (K210)
#define DEVICE_ID_REMOTE1         (0x06) // 遥控器1号
#define DEVICE_ID_REMOTE2         (0x07) // 遥控器2号
#define DEVICE_ID_REMOTE3         (0x08) // 遥控器3号	空调

#define DEVICE_ID_ASRPRO         (0x10) // 语音助手

/* 命令定义 */
#define COMMAND_OFF               (0x00) // 关闭命令
#define COMMAND_ON                (0x01) // 打开命令

// --- 原有函数声明 ---
void USART0_Configuration(void);
void USART1_Configuration(void);

// USART1 中断处理函数声明 (保持不变)
void USART1_IRQHandler(void); // 这个函数名需要和你的向量表及 ht32_board_config.h 中的定义一致

// --- 发送相关函数声明 (修正参数类型以匹配 .c 文件) ---
void Usart_Sendbyte(HT_USART_TypeDef* USARTx, u8 data); // 修改 USARTx 类型为指针
void Usart_SendArray(HT_USART_TypeDef* USARTx, u8 *array, u8 num); // 修改 USARTx 类型为指针, array 为指针
void Usart_SendStr(HT_USART_TypeDef* USARTx, char *str); // 修改 USARTx 类型为指针
void USART_Tx(HT_USART_TypeDef* USARTx, const char* TxBuffer, u32 length); // 修改 USARTx 类型为指针
void USART_Rx(const char* RxBuffer, u32 length); // 这个函数在你的 .c 文件中没有实现，如果不需要可以移除
void Int2Char_Send(HT_USART_TypeDef* USARTx, unsigned char N);
void USART_SEND_END(HT_USART_TypeDef* USARTx) ;

//串口调试信息输出函数 (修正参数类型)
void UsartPrintf(HT_USART_TypeDef* USARTx, char *fmt, ...); // 修改 USARTx 类型为指针
void Usart_SendString(HT_USART_TypeDef *USARTx, unsigned char *str, unsigned short len);


// --- 新增功能相关的声明 ---
#define USART1_RX_BUFFER_SIZE 64 // USART1接收环形缓冲区大小

typedef struct
{
  u8 buffer[USART1_RX_BUFFER_SIZE];
  volatile u16 write_pt;
  volatile u16 read_pt;
  volatile u16 cnt;
} USART1_RxBuffer_TypeDef;

void USART1_Init_RxBuffer(void);          // 新增：初始化USART1接收缓冲区
void USART1_ProcessReceivedCommands(void);// 新增：处理接收到的命令

// 设备控制函数原型 (新增)
void Control_Beep(u8 command);
void Control_Curtain(u8 command);
void Control_Fan(u8 command);
void Control_Hongwai(u8 command);
void Control_K210(u8 command);
// void Control_Remote1(u8 command); // 如果需要
// void Control_Remote2(u8 command); // 如果需要

#endif
