#include "bsp_uart0.h"
#include "ht32_board_config.h"

/* 私有变量 --------------------------------------------------------------------------------*/
static uart0_rx_callback s_rx_callback_uart0 = NULL;

/* 私有函数原型 ----------------------------------------------------------------------------*/
static void _BSP_UART0_CKCU_Config(void);
static void _BSP_UART0_GPIO_AFIO_Config(void);
static void _BSP_UART0_Peripheral_Config(void);
static void _BSP_UART0_NVIC_Config(void);

/**
  * @brief  初始化UART0的所有硬件组件 (时钟, GPIO, 外设, 中断)。
  */
void BSP_UART0_Init(void)
{
  _BSP_UART0_CKCU_Config();
  _BSP_UART0_GPIO_AFIO_Config();
  _BSP_UART0_Peripheral_Config();
  _BSP_UART0_NVIC_Config();
}

/**
  * @brief  注册一个当UART0接收到数据时要调用的回调函数。
  */
void BSP_UART0_Register_RX_Callback(uart0_rx_callback callback)
{
    s_rx_callback_uart0 = callback;
}

/**
  * @brief  通过 UART0 发送单个字节。
  */
void BSP_UART0_SendByte(uint8_t data)
{
  while (USART_GetFlagStatus(BSP_UART0_PORT, USART_FLAG_TXDE) == RESET);
  USART_SendData(BSP_UART0_PORT, (u16)data);
  while (USART_GetFlagStatus(BSP_UART0_PORT, USART_FLAG_TXC) == RESET);
}

/**
  * @brief  通过 UART0 发送一个数据缓冲区。
  */
void BSP_UART0_SendBuffer(const uint8_t* buffer, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        BSP_UART0_SendByte(buffer[i]);
    }
}

/**
  * @brief  配置HT_UART0相关的时钟。
  */
static void _BSP_UART0_CKCU_Config(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  CKCUClock.Bit.AFIO    = 1;
  CKCUClock.Bit.PB      = 1;
  CKCUClock.Bit.UART0   = 1; // 明确使能 UART0 时钟
  CKCU_PeripClockConfig(CKCUClock, ENABLE);
}

/**
  * @brief  配置HT_UART0 TX/RX引脚 (PB2/PB3) 的GPIO和AFIO。
  */
static void _BSP_UART0_GPIO_AFIO_Config(void)
{
    AFIO_GPxConfig(BSP_UART0_TX_GPIO_ID, BSP_UART0_TX_AFIO_PIN, BSP_UART0_AFIO_MODE);
    AFIO_GPxConfig(BSP_UART0_RX_GPIO_ID, BSP_UART0_RX_AFIO_PIN, BSP_UART0_AFIO_MODE);
    GPIO_PullResistorConfig(BSP_UART0_RX_GPIO_PORT, BSP_UART0_RX_GPIO_PIN, GPIO_PR_UP);
}

/**
  * @brief  配置HT_UART0外设。
  */
static void _BSP_UART0_Peripheral_Config(void)
{
  USART_InitTypeDef USART_InitStruct;
  USART_InitStruct.USART_BaudRate = BSP_UART0_BAUDRATE;
  USART_InitStruct.USART_WordLength = USART_WORDLENGTH_8B;
  USART_InitStruct.USART_StopBits = USART_STOPBITS_1;
  USART_InitStruct.USART_Parity = USART_PARITY_NO;
  USART_InitStruct.USART_Mode = USART_MODE_NORMAL;
  USART_Init(BSP_UART0_PORT, &USART_InitStruct);

  USART_TxCmd(BSP_UART0_PORT, ENABLE);
  USART_RxCmd(BSP_UART0_PORT, ENABLE);
  USART_IntConfig(BSP_UART0_PORT, USART_INT_RXDR, ENABLE);
}

/**
  * @brief  配置HT_UART0的NVIC。
  */
static void _BSP_UART0_NVIC_Config(void)
{
  NVIC_EnableIRQ(BSP_UART0_IRQn);
}

/**
  * @brief  UART0中断服务程序。
  */
void UART0_IRQHandler(void)
{
  if (USART_GetFlagStatus(BSP_UART0_PORT, USART_FLAG_RXDR) == SET)
  {
    uint8_t received_char = (uint8_t)USART_ReceiveData(BSP_UART0_PORT);
    if (s_rx_callback_uart0 != NULL)
    {
        s_rx_callback_uart0(received_char);
    }
  }

  if (USART_GetFlagStatus(BSP_UART0_PORT, USART_FLAG_OE) == SET) {
      (void)USART_ReceiveData(BSP_UART0_PORT);
      USART_ClearFlag(BSP_UART0_PORT, USART_FLAG_OE);
  }
  if (USART_GetFlagStatus(BSP_UART0_PORT, USART_FLAG_FE) == SET) {
      USART_ClearFlag(BSP_UART0_PORT, USART_FLAG_FE);
  }
}
