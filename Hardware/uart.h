 /************************************************************************************************************
 * 
 *	合泰LOG_UART0.h  
 *
 ************************************************************************************************************/
 
#ifndef __UART_H
#define __UART_H
 
 
 
//-----------------------------------------------------------------------------
#include "ht32.h"
#include <stdarg.h>  // 包含变参函数相关的头文件
//#include "PDMA.h"
#include "USART.h"
#include <stdbool.h> // 如果 uart.c 中会用到 bool 且 uart.h 是主要包含点
//-----------------------------------------------------------------------------
#define UART0_BUF_SIZE 256
#define UART0_FIFO_LEN 1
// #define CMD_FROM_ASRPRO "DATA"
 
//-----------------------------------------------------------------------------
typedef struct
{
  u8 buffer[UART0_BUF_SIZE];
  u16 write_pt;
  u16 read_pt;
  u16 cnt;
}_UART0_STRUCT;
 
//-----------------------------------------------------------------------------
void UART0_Configuration(void);  				// 波特率 GPIO口 等初始化
void UART0_init_buffer (void);
void UART0_analyze_data(void);
void receiveString(void);								// 接受字符串
void UART0_tx_data(u8 *pt, u8 len);
void UART0_test(void);
void UART0_tx_string(const char *str);	//字符串发送函数
void uprintf(const char *str, ...); 		//类printf 串口发送
void Process_ASRPRO_Command_And_Respond(void);

//-----------------------------------------------------------------------------
#endif
