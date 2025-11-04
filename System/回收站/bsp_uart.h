#ifndef __BSP_UART_H
#define __BSP_UART_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "ht32.h"
#include <stdbool.h>

/* UART 配置常量 (从旧的 zigbee.h 迁移并通用化) -------------------------------------------*/
#define BSP_USART_PORT              (HT_UART1)
#define BSP_USART_IRQn              (UART1_IRQn)

#define BSP_USART_TX_GPIO_PORT      (HT_GPIOB)
#define BSP_USART_RX_GPIO_PORT      (HT_GPIOB)
#define BSP_USART_TX_GPIO_ID        (GPIO_PB)
#define BSP_USART_RX_GPIO_ID        (GPIO_PB)
#define BSP_USART_TX_GPIO_PIN       (GPIO_PIN_4)
#define BSP_USART_RX_GPIO_PIN       (GPIO_PIN_5)
#define BSP_USART_TX_AFIO_PIN       (AFIO_PIN_4)
#define BSP_USART_RX_AFIO_PIN       (AFIO_PIN_5)
#define BSP_USART_AFIO_MODE         (AFIO_MODE_6) // AFIO_MODE_6 for UART1 on PB4/PB5

#define BSP_USART_BAUDRATE          (9600)

/* 类型定义：接收回调函数指针 -------------------------------------------------------------*/
// 定义一个函数指针类型，用于注册一个当UART接收到数据时被调用的函数
typedef void (*uart_rx_callback)(uint8_t received_data);

/* 导出函数 ---------------------------------------------------------------------------------*/
void BSP_UART_Init(void);
void BSP_UART_SendByte(uint8_t data);
void BSP_UART_SendBuffer(const uint8_t* buffer, uint32_t length);
void BSP_UART_Register_RX_Callback(uart_rx_callback callback);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_H */
