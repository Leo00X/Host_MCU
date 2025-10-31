#ifndef __BSP_USART_H
#define __BSP_USART_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "ht32.h"
#include <stdbool.h>
#include <stdarg.h>

/* 硬件配置常量 ---------------------------------------------------------------------------------*/
// --- USART0 (PA2/PA3) ---
#define USART0_GPIO_PORT              (HT_GPIOA)
#define USART0_GPIO_ID                (GPIO_PA)
#define USART0_TX_PIN                 (GPIO_PIN_2)
#define USART0_RX_PIN                 (GPIO_PIN_3)
#define USART0_TX_AFIO_PIN            (AFIO_PIN_2)
#define USART0_RX_AFIO_PIN            (AFIO_PIN_3)
#define USART0_AFIO_MODE              (AFIO_FUN_USART_UART)
#define USART0_PORT                   (HT_USART0)
#define USART0_IRQn                   (USART0_IRQn)

// --- USART1 (PA4/PA5) ---
#define USART1_GPIO_PORT              (HT_GPIOA)
#define USART1_GPIO_ID                (GPIO_PA)
#define USART1_TX_PIN                 (GPIO_PIN_4)
#define USART1_RX_PIN                 (GPIO_PIN_5)
#define USART1_TX_AFIO_PIN            (AFIO_PIN_4)
#define USART1_RX_AFIO_PIN            (AFIO_PIN_5)
#define USART1_AFIO_MODE              (AFIO_FUN_USART_UART)
#define USART1_PORT                   (HT_USART1)
#define USART1_IRQn                   (USART1_IRQn)

#define USART_DEBUG_PORT              (HT_USART1) // 定义调试串口

/* 缓冲区定义 (保留) ----------------------------------------------------------------------------*/
#define USART1_RX_BUFFER_SIZE         (64)

typedef struct
{
  u8 buffer[USART1_RX_BUFFER_SIZE];
  vu16 write_pt;
  vu16 read_pt;
} USART1_RxBuffer_TypeDef;


/* 导出函数 (保留原有接口不变) -------------------------------------------------------------------*/
// --- 底层初始化 ---
void USART0_Configuration(void);
void USART1_Configuration(void);

// --- 数据发送功能 ---
void Usart_Sendbyte(HT_USART_TypeDef* USARTx, u8 data);
void Usart_SendArray(HT_USART_TypeDef* USARTx, u8 *array, u8 num);
void Usart_SendStr(HT_USART_TypeDef* USARTx, char *str);
void USART_SEND_END(HT_USART_TypeDef* USARTx);
void Int2Char_Send(HT_USART_TypeDef* USARTx, unsigned char N);
void Usart_SendString(HT_USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

// --- 格式化打印 ---
void UsartPrintf(HT_USART_TypeDef* USARTx, char *fmt, ...);

// --- USART1 接收相关 ---
bool USART1_Is_Rx_Data_Available(void);
u8   USART1_Read_Rx_Data(void);


#ifdef __cplusplus
}
#endif

#endif
