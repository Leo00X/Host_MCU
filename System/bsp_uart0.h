#ifndef __BSP_UART0_H
#define __BSP_UART0_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "ht32.h"
#include <stdbool.h>

/* UART0 配置常量 (根据手册和你的截图修正) ---------------------------------------------------*/
#define BSP_UART0_PORT              (HT_UART0)
#define BSP_UART0_IRQn              (UART0_IRQn)

#define BSP_UART0_TX_GPIO_PORT      (HT_GPIOB)
#define BSP_UART0_RX_GPIO_PORT      (HT_GPIOB)
#define BSP_UART0_TX_GPIO_ID        (GPIO_PB)
#define BSP_UART0_RX_GPIO_ID        (GPIO_PB)
#define BSP_UART0_TX_GPIO_PIN       (GPIO_PIN_2)
#define BSP_UART0_RX_GPIO_PIN       (GPIO_PIN_3)
#define BSP_UART0_TX_AFIO_PIN       (AFIO_PIN_2)
#define BSP_UART0_RX_AFIO_PIN       (AFIO_PIN_3)

// 使用你原始代码中更具可读性的宏定义，功能与 AFIO_MODE_6 在此场景下相同
#define BSP_UART0_AFIO_MODE         (AFIO_FUN_USART_UART) 

#define BSP_UART0_BAUDRATE          (9600) // 恢复为你原始代码中的波特率

/* 类型定义：接收回调函数指针 -------------------------------------------------------------*/
typedef void (*uart0_rx_callback)(uint8_t received_data);

/* 导出函数 ---------------------------------------------------------------------------------*/
void BSP_UART0_Init(void);
void BSP_UART0_SendByte(uint8_t data);
void BSP_UART0_SendBuffer(const uint8_t* buffer, uint32_t length);
void BSP_UART0_Register_RX_Callback(uart0_rx_callback callback);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART0_H */
