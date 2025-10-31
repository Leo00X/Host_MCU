#include "bsp_uart.h"
#include "ht32_board_config.h"

/* 私有变量 --------------------------------------------------------------------------------*/
// 定义一个静态的函数指针变量，用于存储上层注册的回调函数
static uart_rx_callback s_rx_callback = NULL;

/* 私有函数原型 ----------------------------------------------------------------------------*/
static void _BSP_USART_CKCU_Config(void);
static void _BSP_USART_GPIO_AFIO_Config(void);
static void _BSP_USART_Peripheral_Config(void);
static void _BSP_USART_NVIC_Config(void);

/**
  * @brief  初始化UART1的所有硬件组件 (时钟, GPIO, 外设, 中断)。
  * @retval 无
  */
void BSP_UART_Init(void)
{
  _BSP_USART_CKCU_Config();
  _BSP_USART_GPIO_AFIO_Config();
  _BSP_USART_Peripheral_Config();
  _BSP_USART_NVIC_Config();
}

/**
  * @brief  注册一个当UART接收到数据时要调用的回调函数。
  * @param  callback: 指向回调函数的指针。
  * @retval 无
  */
void BSP_UART_Register_RX_Callback(uart_rx_callback callback)
{
    s_rx_callback = callback;
}

/**
  * @brief  通过 UART1 发送单个字节。
  * @param  data: 要发送的字节。
  * @retval 无
  */
void BSP_UART_SendByte(uint8_t data)
{
  while (USART_GetFlagStatus(BSP_USART_PORT, USART_FLAG_TXDE) == RESET);
  USART_SendData(BSP_USART_PORT, (u16)data);
  while (USART_GetFlagStatus(BSP_USART_PORT, USART_FLAG_TXC) == RESET);
}

/**
  * @brief  通过 UART1 发送一个数据缓冲区。
  * @param  buffer: 指向数据缓冲区的指针。
  * @param  length: 要发送的数据长度。
  * @retval 无
  */
void BSP_UART_SendBuffer(const uint8_t* buffer, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        BSP_UART_SendByte(buffer[i]);
    }
}

/**
  * @brief  配置HT_UART1相关的时钟。
  */
static void _BSP_USART_CKCU_Config(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  CKCUClock.Bit.AFIO    = 1;
  CKCUClock.Bit.PB      = 1;
  CKCUClock.Bit.UART1   = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);
}

/**
  * @brief  配置HT_UART1 TX/RX引脚 (PB4/PB5) 的GPIO和AFIO。
  */
static void _BSP_USART_GPIO_AFIO_Config(void)
{
    AFIO_GPxConfig(BSP_USART_TX_GPIO_ID, BSP_USART_TX_AFIO_PIN, BSP_USART_AFIO_MODE);
    AFIO_GPxConfig(BSP_USART_RX_GPIO_ID, BSP_USART_RX_AFIO_PIN, BSP_USART_AFIO_MODE);
    GPIO_PullResistorConfig(BSP_USART_RX_GPIO_PORT, BSP_USART_RX_GPIO_PIN, GPIO_PR_UP);
}

/**
  * @brief  配置HT_UART1外设。
  */
static void _BSP_USART_Peripheral_Config(void)
{
  USART_InitTypeDef USART_InitStruct;
  USART_InitStruct.USART_BaudRate = BSP_USART_BAUDRATE;
  USART_InitStruct.USART_WordLength = USART_WORDLENGTH_8B;
  USART_InitStruct.USART_StopBits = USART_STOPBITS_1;
  USART_InitStruct.USART_Parity = USART_PARITY_NO;
  USART_InitStruct.USART_Mode = USART_MODE_NORMAL;
  USART_Init(BSP_USART_PORT, &USART_InitStruct);

  USART_TxCmd(BSP_USART_PORT, ENABLE);
  USART_RxCmd(BSP_USART_PORT, ENABLE);
  USART_IntConfig(BSP_USART_PORT, USART_INT_RXDR, ENABLE);
}

/**
  * @brief  配置HT_UART1的NVIC。
  */
static void _BSP_USART_NVIC_Config(void)
{
  NVIC_EnableIRQ(BSP_USART_IRQn);
}

/**
  * @brief  UART1中断服务程序。
  * @note   此函数现在是底层驱动的一部分。
  * 它只负责接收数据并通过回调函数传递给上层，不再处理任何协议逻辑。
  */
void UART1_IRQHandler(void)
{
  if (USART_GetFlagStatus(BSP_USART_PORT, USART_FLAG_RXDR) == SET)
  {
    uint8_t received_char = (uint8_t)USART_ReceiveData(BSP_USART_PORT);
    
    // 如果上层应用已经注册了回调函数，则调用它
    if (s_rx_callback != NULL)
    {
        s_rx_callback(received_char);
    }
  }

  // 错误标志位处理保持不变
  if (USART_GetFlagStatus(BSP_USART_PORT, USART_FLAG_OE) == SET) {
      (void)USART_ReceiveData(BSP_USART_PORT);
      USART_ClearFlag(BSP_USART_PORT, USART_FLAG_OE);
  }
  if (USART_GetFlagStatus(BSP_USART_PORT, USART_FLAG_FE) == SET) {
      USART_ClearFlag(BSP_USART_PORT, USART_FLAG_FE);
  }
}
