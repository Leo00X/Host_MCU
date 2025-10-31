#include "bsp_usart.h"
#include <stdio.h>
#include <string.h>


/* 全局变量 (为保持兼容性而保留) ---------------------------------------------------------------*/
// USART0 相关
uc8  *gURTx_Ptr;
vu32 gURTx_Length = 0;
u8  *gURRx_Ptr;
vu32 gURRx_Length = 0;
vu32 gIsTxFinished = FALSE;
u8 gRx_Buffer[128];

// USART1 接收环形缓冲区
__ALIGN4 USART1_RxBuffer_TypeDef usart1_rx_buffer;


/*
========================================================================================================================
                                       新版底层硬件驱动 (私有函数)
========================================================================================================================
*/

/* 私有函数原型 ----------------------------------------------------------------------------------*/
// USART0
static void _USART0_CKCU_Config(void);
static void _USART0_GPIO_AFIO_Config(void);
static void _USART0_Peripheral_Config(void);
static void _USART0_NVIC_Config(void);

// USART1
static void _USART1_CKCU_Config(void);
static void _USART1_GPIO_AFIO_Config(void);
static void _USART1_Peripheral_Config(void);
static void _USART1_NVIC_Config(void);

/* USART0 底层实现 --------------------------------------------------------------------------------*/
static void _USART0_CKCU_Config(void) {
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  CKCUClock.Bit.AFIO    = 1;
  CKCUClock.Bit.PA      = 1;
  CKCUClock.Bit.USART0  = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);
}

static void _USART0_GPIO_AFIO_Config(void) {
  AFIO_GPxConfig(USART0_GPIO_ID, USART0_TX_AFIO_PIN, USART0_AFIO_MODE);
  AFIO_GPxConfig(USART0_GPIO_ID, USART0_RX_AFIO_PIN, USART0_AFIO_MODE);
}

static void _USART0_Peripheral_Config(void) {
  USART_InitTypeDef USART_InitStruct;
  USART_InitStruct.USART_BaudRate = 9600;
  USART_InitStruct.USART_WordLength = USART_WORDLENGTH_8B;
  USART_InitStruct.USART_StopBits = USART_STOPBITS_1;
  USART_InitStruct.USART_Parity = USART_PARITY_NO;
  USART_InitStruct.USART_Mode = USART_MODE_NORMAL;
  USART_Init(USART0_PORT, &USART_InitStruct);
  USART_TxCmd(USART0_PORT, ENABLE);
  USART_RxCmd(USART0_PORT, ENABLE);
}

static void _USART0_NVIC_Config(void) {
  // 保持原样，不开启中断
}

/* USART1 底层实现 --------------------------------------------------------------------------------*/
static void _USART1_CKCU_Config(void) {
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  CKCUClock.Bit.AFIO    = 1;
  CKCUClock.Bit.PA      = 1;
  CKCUClock.Bit.USART1  = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);
}

static void _USART1_GPIO_AFIO_Config(void) {
  AFIO_GPxConfig(USART1_GPIO_ID, USART1_TX_AFIO_PIN, USART1_AFIO_MODE);
  AFIO_GPxConfig(USART1_GPIO_ID, USART1_RX_AFIO_PIN, USART1_AFIO_MODE);
}

static void _USART1_Peripheral_Config(void) {
  USART_InitTypeDef USART_InitStruct;
  USART_InitStruct.USART_BaudRate = 9600;
  USART_InitStruct.USART_WordLength = USART_WORDLENGTH_8B;
  USART_InitStruct.USART_StopBits = USART_STOPBITS_1;
  USART_InitStruct.USART_Parity = USART_PARITY_NO;
  USART_InitStruct.USART_Mode = USART_MODE_NORMAL;
  USART_Init(USART1_PORT, &USART_InitStruct);
  USART_TxCmd(USART1_PORT, ENABLE);
  USART_RxCmd(USART1_PORT, ENABLE);
  USART_IntConfig(USART1_PORT, USART_INT_RXDR, ENABLE);
}

static void _USART1_NVIC_Config(void) {
  NVIC_EnableIRQ(USART1_IRQn);
}


/*
========================================================================================================================
                                       保留的公共接口 (兼容旧代码)
========================================================================================================================
*/

void USART0_Configuration(void)
{
  _USART0_CKCU_Config();
  _USART0_GPIO_AFIO_Config();
  _USART0_Peripheral_Config();
  _USART0_NVIC_Config();
  gURRx_Length = 128;
  gURRx_Ptr = gRx_Buffer;
}

void USART1_Configuration(void)
{
  _USART1_CKCU_Config();
  _USART1_GPIO_AFIO_Config();
  _USART1_Peripheral_Config();
  _USART1_NVIC_Config();
  memset(&usart1_rx_buffer, 0, sizeof(usart1_rx_buffer));
}


/*
========================================================================================================================
                                保留的应用/工具函数 (已修正错误)
========================================================================================================================
*/

void Usart_Sendbyte(HT_USART_TypeDef* USARTx, u8 data)
{
  USART_SendData(USARTx, data);
  while(USART_GetFlagStatus(USARTx, USART_FLAG_TXDE) == RESET);
}

void Usart_SendArray(HT_USART_TypeDef* USARTx, u8 *array, u8 num)
{
  u8 i;
  for(i=0; i<num; i++)
  {
    Usart_Sendbyte(USARTx, array[i]);
  }
  // 修正: 使用 USART_FLAG_TXC 等待发送完成
  while(USART_GetFlagStatus(USARTx, USART_FLAG_TXC) == RESET);
}

void Usart_SendStr(HT_USART_TypeDef* USARTx, char *str)
{
  u32 i = 0;
  do {
    Usart_Sendbyte(USARTx, *(str+i));
    i++;
  } while(*(str+i) != '\0'); // 修正: 使用 '\0'

  // 修正: 使用 USART_FLAG_TXC 等待发送完成
  while(USART_GetFlagStatus(USARTx, USART_FLAG_TXC) == RESET);
}

void UsartPrintf(HT_USART_TypeDef* USARTx, char *fmt,...)
{
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Usart_SendStr(USARTx, buffer);
}

void USART_SEND_END(HT_USART_TypeDef* USARTx)
{
   Usart_Sendbyte(USARTx, 0x0D);
   Usart_Sendbyte(USARTx, 0x0A);
}

void Int2Char_Send(HT_USART_TypeDef* USARTx, unsigned char N)
{
  unsigned char g,s,b;
  b = N / 100;
  s = (N % 100) / 10;
  g = N % 10;

  if (b != 0) Usart_Sendbyte(USARTx, b + '0');
  if (b != 0 || s != 0) Usart_Sendbyte(USARTx, s + '0');
  Usart_Sendbyte(USARTx, g + '0');
}

void Usart_SendString(HT_USART_TypeDef *USARTx, unsigned char *str, unsigned short len)
{
  unsigned short count = 0;
  for(; count < len; count++)
  {
    USART_SendData(USARTx, str[count]);
    while(USART_GetFlagStatus(USARTx, USART_FLAG_TXDE) == RESET);
  }
}

/*
========================================================================================================================
                                       中断服务及接收处理 (保留逻辑)
========================================================================================================================
*/

void USART1_IRQHandler(void)
{
  if (USART_GetFlagStatus(USART1_PORT, USART_FLAG_RXDR) == SET)
  {
    u8 received_data = (u8)USART_ReceiveData(USART1_PORT);
    usart1_rx_buffer.buffer[usart1_rx_buffer.write_pt] = received_data;
    usart1_rx_buffer.write_pt = (usart1_rx_buffer.write_pt + 1) % USART1_RX_BUFFER_SIZE;
  }
}

bool USART1_Is_Rx_Data_Available(void)
{
  return (usart1_rx_buffer.read_pt != usart1_rx_buffer.write_pt);
}

u8 USART1_Read_Rx_Data(void)
{
  u8 data;
  data = usart1_rx_buffer.buffer[usart1_rx_buffer.read_pt];
  usart1_rx_buffer.read_pt = (usart1_rx_buffer.read_pt + 1) % USART1_RX_BUFFER_SIZE;
  return data;
}

