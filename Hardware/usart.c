#include "usart.h"
 #include "esp8266.h" 
#include "delay.h"
 #include "HT32F52352_SK.H" 
#include "ht32_board_config.h" 
#include "hmi_serial.h"
#include "beep.h"
//C库
#include <stdarg.h>
#include <stdio.h>  // 新增：为了 vsnprintf
#include <string.h> // 新增：为了 memset

/* Global variables ----------------------------------------------------------------------------------------*/
// 这些全局变量看起来主要用于 USART0 的示例或特定用途，保持不变
uc8  *gURTx_Ptr;
vu32 gURTx_Length = 0;
u8  *gURRx_Ptr;
vu32 gURRx_Length = 0;
vu32 gIsTxFinished = FALSE;

/* Private variables ---------------------------------------------------------------------------------------*/
 uc8 gHelloString[] = "Hello, this is USART Tx/Rx FIFO example. Please enter 5 characters...\r\n"; // 如果不用可以注释
 u8 gTx_Buffer[128]; // 如果不用可以注释
 u8 gRx_Buffer[128]; // USART0 的接收缓冲，由 USART0_Configuration 初始化

/* 新增：USART1 接收缓冲区和帧解析状态变量 */
__ALIGN4 USART1_RxBuffer_TypeDef usart1_rx_buffer; // USART1 的环形缓冲区
static u8 frame_buffer_parser[SCREEN_FRAME_LENGTH]; // 用于暂存可能帧的临时缓冲区 (改名以防冲突)
static u8 frame_buffer_parser_idx = 0;              // frame_buffer_parser 当前的索引
typedef enum {
    PARSER_STATE_WAITING_FOR_SOF,  // 等待帧头状态
    PARSER_STATE_RECEIVING_DATA    // 接收数据状态
} FrameParserState_TypeDef;
static FrameParserState_TypeDef current_parser_state = PARSER_STATE_WAITING_FOR_SOF;


/**************************实现函数********************************************
函数说明：配置usart0串口 (保持你原有的不变)
*******************************************************************************/
void USART0_Configuration(void)
{
  gURRx_Ptr = gRx_Buffer; // 注意：这里 gRx_Buffer 需要在某处定义，你在上面声明了但未定义

	#if 0 // Use following function to configure the IP clock speed.
  // The UxART IP clock speed must be faster 16x then the baudrate.
  CKCU_SetPeripPrescaler(CKCU_PCLK_UxARTn, CKCU_APBCLKPRE_DIV2);
  #endif

	{
		CKCU_PeripClockConfig_TypeDef CKCUClock= {{0}};
		CKCUClock.Bit.AFIO = 1;
		CKCUClock.Bit.PA=1;
		CKCUClock.Bit.USART0=1;
		CKCU_PeripClockConfig(CKCUClock, ENABLE);
	}
	//HTCFG_UART_IPN
	GPIO_PullResistorConfig(HT_GPIOA, GPIO_PIN_3, GPIO_PR_UP);

  AFIO_GPxConfig(USART_GPIO_GROUP, USART_TX_PIN, AFIO_FUN_USART_UART);
  AFIO_GPxConfig(USART_GPIO_GROUP, USART_RX_PIN, AFIO_FUN_USART_UART);

  /*
		波特率： 115200
		长度：   8bits
		停止位： 1位
	  校验位： 无
	  模式：   正常模式
  */
	USART_InitTypeDef USART_InitStructure={0};

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WORDLENGTH_8B;
  USART_InitStructure.USART_StopBits = USART_STOPBITS_1;
  USART_InitStructure.USART_Parity = USART_PARITY_NO;
  USART_InitStructure.USART_Mode = USART_MODE_NORMAL;
  USART_Init(COM0_PORT, &USART_InitStructure);

  //Enable 中断设置
  NVIC_EnableIRQ(COM0_IRQn); // 确保 COM0_IRQn 已在 ht32_board_config.h 定义

	// Enable UxART Rx interrupt (仅接收中断)
  USART_IntConfig(COM0_PORT, USART_INT_RXDR , ENABLE);

  // 使能 COM0_PORT 发送和接收 (注意：你代码中写的是 COM1_PORT，应为 COM0_PORT)
  USART_TxCmd(COM0_PORT, ENABLE);
  USART_RxCmd(COM0_PORT, ENABLE);
}


/**************************实现函数********************************************
函数说明：配置HT_USATR1串口
作者：LIANG WENFA
日期：2021年1月26日
(修改：增加接收中断使能)
*******************************************************************************/
void USART1_Configuration(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock= {{0}};
  CKCUClock.Bit.AFIO = 1;
  CKCUClock.Bit.PA=1;
  COM1_CLK(CKCUClock)  = 1;  //开启COM1 时钟 (你注释写的是COM0，但代码是COM1_CLK，保持COM1)
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  GPIO_PullResistorConfig(HT_GPIOA, GPIO_PIN_5, GPIO_PR_UP);

  AFIO_GPxConfig(USART1_GPIO_GROUP, USART1_TX_PIN, AFIO_FUN_USART_UART);
  AFIO_GPxConfig(USART1_GPIO_GROUP, USART1_RX_PIN, AFIO_FUN_USART_UART);

  /*
		波特率： 9600
		长度：   8bits
		停止位： 1位
	  校验位： 无
	  模式：   正常模式
  */
	USART_InitTypeDef USART_InitStructure={0};

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WORDLENGTH_8B;
  USART_InitStructure.USART_StopBits = USART_STOPBITS_1;
  USART_InitStructure.USART_Parity = USART_PARITY_NO;
  USART_InitStructure.USART_Mode = USART_MODE_NORMAL;
  USART_Init(COM1_PORT, &USART_InitStructure); // 确保 COM1_PORT 已在 ht32_board_config.h 定义为 USART1_PORT

  // --- 新增：使能 USART1 接收中断 ---
  USART_IntConfig(COM1_PORT, USART_INT_RXDR, ENABLE);
  NVIC_EnableIRQ(COM1_IRQn); // 确保 COM1_IRQn 已在 ht32_board_config.h 定义
  // --- 新增结束 ---

  // 使能 COM1_PORT 发送和接收
  USART_TxCmd(COM1_PORT, ENABLE);
  USART_RxCmd(COM1_PORT, ENABLE);
}

//==========================================================
//	函数名称：	USART0_IRQHandler (你原有的，保持注释状态或根据需要实现)
//==========================================================
/*
void USART0_IRQHandler(void)
{
  // ... 你原有的 USART0 中断处理逻辑 ...
  // 注意：如果 USART0 的中断用于接收数据给 gURRx_Ptr，则需要确保其正确性
  if (USART_GetFlagStatus(COM0_PORT , USART_FLAG_RXDR))
  {
    // DBG_IO1_LO();
    // 确保 gURRx_Length 不会超出 gRx_Buffer 的界限
    if (gURRx_Length < sizeof(gRx_Buffer)) {
        gURRx_Ptr[gURRx_Length++] = USART_ReceiveData(COM0_PORT);
    } else {
        // 缓冲区满处理
        (void)USART_ReceiveData(COM0_PORT); // 丢弃数据以清除中断标志
    }
    // DBG_IO1_HI();
  }
  // ... 其他中断标志处理 ...
}
*/

/**************************实现函数********************************************
函数说明：初始化USART1的接收环形缓冲区 (新增)
*******************************************************************************/
void USART1_Init_RxBuffer(void)
{
    usart1_rx_buffer.read_pt = 0;
    usart1_rx_buffer.write_pt = 0;
    usart1_rx_buffer.cnt = 0;
    frame_buffer_parser_idx = 0;
    current_parser_state = PARSER_STATE_WAITING_FOR_SOF;
    memset(usart1_rx_buffer.buffer, 0, USART1_RX_BUFFER_SIZE);
    memset(frame_buffer_parser, 0, SCREEN_FRAME_LENGTH);
}

/**************************实现函数********************************************
函数说明：USART1接收中断服务函数 (修改为你提供的COM1_IRQHandler)
修改内容：将接收到的数据存入环形缓冲区
*******************************************************************************/
void USART1_IRQHandler(void) 
{
	
    // 检查是否是接收中断
    if (USART_GetFlagStatus(COM1_PORT, USART_FLAG_RXDR)) 
    {
        // 读取接收到的数据以清除中断标志
        u8 data = USART_ReceiveData(COM1_PORT);

        // 将接收到的字节直接送入 HMI 协议状态机进行处理
        // 这将替代下面所有旧的环形缓冲区代码
        HMI_ReceiveByte(data);
    }
    // 保留对其他中断标志（如溢出错误）的处理，这是好习惯
    if (USART_GetFlagStatus(COM1_PORT, USART_FLAG_OE) == SET) {
        (void)USART_ReceiveData(COM1_PORT); // 通过读取数据来清除标志
        USART_ClearFlag(COM1_PORT, USART_FLAG_OE);
    }
}


/**************************实现函数********************************************
函数说明：处理接收到的命令帧并执行操作 (新增)
*******************************************************************************/
// 设备操作函数的具体实现（占位）


static void ExecuteParsedCommand(u8 device_id, u8 cmd_code)
{
    UsartPrintf(USART_DEBUG, "Executing: DEV_ID=0x%02X, CMD=0x%02X\r\n", device_id, cmd_code);
    switch (device_id)
    {
       
        case DEVICE_ID_REMOTE1:
		  {
			  UsartPrintf(USART_DEBUG, "Remote1: DEV_ID=0x%02X, CMD=0x%02X\r\n", device_id, cmd_code);
            // Control_Remote1(cmd_code); // 如果有实现
			  beep_key();
		  }           
			  break;
        case DEVICE_ID_REMOTE2:
		  {
            UsartPrintf(USART_DEBUG, "Remote2: DEV_ID=0x%02X, CMD=0x%02X\r\n", device_id, cmd_code);
            // Control_Remote2(cmd_code); // 如果有实现
			  beep_key();
		  } 
			  break;
        case DEVICE_ID_REMOTE3:
		  {
            UsartPrintf(USART_DEBUG, "Remote3: DEV_ID=0x%02X, CMD=0x%02X\r\n", device_id, cmd_code);
            // Control_Remote3(cmd_code); // 如果有实现
			  beep_key();
		  } 
			  break;
        case DEVICE_ID_ASRPRO:
		  {
            UsartPrintf(USART_DEBUG, "DEVICE_ID_ASRPRO: DEV_ID=0x%02X, CMD=0x%02X\r\n", device_id, cmd_code);
            // Control_Remote2(cmd_code); // 如果有实现
			  beep_key();
		  } 
			  break;
        default:
            UsartPrintf(USART_DEBUG, "Unknown Device ID: 0x%02X\r\n", device_id);
            break;
    }
}

/**************************实现函数********************************************
函数说明：处理接收到的命令帧并执行操作
功能：从 USART1 接收环形缓冲区读取数据，根据定义的协议格式 (SOF, ID, CMD, EOF)
      解析出完整的命令帧，如果帧有效，则调用对应的设备操作函数。
注意：此函数应在主循环中不断调用，以非阻塞方式处理缓冲区中的数据。
*******************************************************************************/
void USART1_ProcessReceivedCommands(void)
{
    u8 current_byte; // 用于存储当前从环形缓冲区读取的字节

    // 循环处理环形缓冲区中所有可用数据
    // usart1_rx_buffer.cnt > 0 表示缓冲区非空
    while (usart1_rx_buffer.cnt > 0)
    {
        // 从环形缓冲区读取当前字节 (读指针指向的位置)
        current_byte = usart1_rx_buffer.buffer[usart1_rx_buffer.read_pt];
        // 更新读指针，环形前进 (使用取模运算确保不超出缓冲区大小)
        usart1_rx_buffer.read_pt = (usart1_rx_buffer.read_pt + 1) % USART1_RX_BUFFER_SIZE;

        // 以下操作修改了被中断服务函数也访问的共享变量 usart1_rx_buffer.cnt
        // 为了防止主循环与中断之间发生竞争条件，需要先关闭中断
        __ISB(); // 指令同步屏障，确保之前的内存访问完成
        __disable_irq(); // 关闭全局中断
        usart1_rx_buffer.cnt--; // 缓冲区数据计数减一
        __enable_irq();  // 恢复全局中断

        // 使用状态机解析接收到的字节
        // current_parser_state 记录当前处于帧解析的哪个阶段
        switch (current_parser_state)
        {
            // 状态 1: 正在等待接收帧头 (SOF)
            case PARSER_STATE_WAITING_FOR_SOF:
                // 如果当前字节是帧头标记
                if (current_byte == SCREEN_SOF_MARKER)
                {
                    frame_buffer_parser[0] = current_byte; // 将帧头存入临时帧缓冲区第一个位置
                    frame_buffer_parser_idx = 1;         // 重置临时帧缓冲区的写入索引为1 (已存入1个字节)
                    current_parser_state = PARSER_STATE_RECEIVING_DATA; // 状态切换到接收数据阶段
                }
                // 如果不是帧头，则丢弃当前字节，继续等待 SOF (状态保持不变)
                break;

            // 状态 2: 已经收到帧头，正在接收后续数据
            case PARSER_STATE_RECEIVING_DATA:
                // 如果临时帧缓冲区尚未存满整个帧 (长度小于 SCREEN_FRAME_LENGTH)
                if (frame_buffer_parser_idx < SCREEN_FRAME_LENGTH)
                {
                    // 将当前字节存入临时帧缓冲区，并索引加一
                    frame_buffer_parser[frame_buffer_parser_idx++] = current_byte;
                }

                // 如果临时帧缓冲区已经存满了整个帧 (达到了 SCREEN_FRAME_LENGTH 长度)
                if (frame_buffer_parser_idx >= SCREEN_FRAME_LENGTH)
                {
                    // 检查接收到的最后一个字节是否是帧尾 (EOF)
                    if (frame_buffer_parser[SCREEN_FRAME_LENGTH - 1] == SCREEN_EOF_MARKER)
                    {
                        // 帧尾正确，认为是一个有效的完整帧
                        u8 device_id = frame_buffer_parser[1]; // 提取帧中的设备ID (第二个字节)
                        u8 command   = frame_buffer_parser[2]; // 提取帧中的命令 (第三个字节)
                        ExecuteParsedCommand(device_id, command); // 调用函数执行对应的设备操作
                    }
                    else
                    {
                        // 帧尾错误，认为是一个无效帧
                        // 打印错误信息和接收到的帧内容，方便调试
                        UsartPrintf(USART_DEBUG, "USART1 Frame Error: Invalid EOF. Expected 0x%02X, Got 0x%02X. Frame: %02X %02X %02X %02X\r\n",
                                    SCREEN_EOF_MARKER, frame_buffer_parser[SCREEN_FRAME_LENGTH - 1],
                                    frame_buffer_parser[0],frame_buffer_parser[1],frame_buffer_parser[2],frame_buffer_parser[3]);
                    }
                    // 无论帧是否有效，处理完一个完整长度的候选帧后，都需要重置解析状态
                    frame_buffer_parser_idx = 0; // 重置临时帧缓冲区索引
                    current_parser_state = PARSER_STATE_WAITING_FOR_SOF; // 状态切换回等待帧头阶段，准备接收下一帧
                }
                break;
        }
    }
}


/**************************实现函数********************************************
函数说明：FIFO (修正参数类型以匹配 .h 文件)
*******************************************************************************/
void USART_Tx(HT_USART_TypeDef* USARTx, const char* TxBuffer, u32 length) // USARTx 改为指针
{
  u32 i; // int 改为 u32

  for (i = 0; i < length; i++)
  {
    // 等待 TXDE (Transmit Data Empty) 比 TXC (Transmit Complete) 更常用作发送前等待
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXDE) == RESET);
    USART_SendData(USARTx, (u8)TxBuffer[i]); // 明确转换为 u8
  }
  // 如果需要确保所有数据都已物理发送出去，则等待TXC:
  // while (USART_GetFlagStatus(USARTx, USART_FLAG_TXC) == RESET);
}

/**************************实现函数********************************************
函数说明：发送一个字节 (修正参数类型)
*******************************************************************************/
void Usart_Sendbyte(HT_USART_TypeDef* USARTx, u8 data) // USARTx 改为指针
{
	USART_SendData(USARTx, data);
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TXDE) == RESET);
}

/**************************实现函数********************************************
函数说明：发送数组 (修正参数类型和实现)
*******************************************************************************/
void Usart_SendArray(HT_USART_TypeDef* USARTx, u8 *array, u8 num) // USARTx 改为指针, array 改为指针
{
	u8 i;
	for( i = 0;i < num;i++)
	{
		Usart_Sendbyte(USARTx, array[i]); // 使用 array[i] 而不是 array++ 后解引用
	}
}

/**************************实现函数********************************************
函数说明：发送字符串 (保持你原有的，但 .h 中已修正为指针)
*******************************************************************************/
void Usart_SendStr(HT_USART_TypeDef* USARTx, char *str) // USARTx 已经是正确的指针类型
{
	uint8_t i;
	for(i = 0;str[i] != '\0';i++)
	{
		Usart_Sendbyte(USARTx,str[i]);
	}
}

/****************************以下部分为淘晶驰屏幕串口函数*******************************/
// (保持你原有的不变)
void USART_SEND_END(HT_USART_TypeDef* USARTx)   //发送结束符
{
	 Usart_Sendbyte(USARTx,0xFF);
	 Usart_Sendbyte(USARTx,0xFF);
	 Usart_Sendbyte(USARTx,0xFF);
}

void Int2Char_Send(HT_USART_TypeDef* USARTx,unsigned char N) //将温度数值分解为 个位，十位，百位  ，并转换为字符发送出去
{
  unsigned char g,s,b,l;
  l=N;
  b=l/100;
  l=l%100;
  s=l/10;
  l=l%10;
  g=l;

  if(b==0)
  {
		if(s==0); // 十位为 0 就不发送了 (这个分号表示空语句，可以移除)
		else Usart_Sendbyte(USARTx,s+48);
  }
  else
	{
   	Usart_Sendbyte(USARTx,b+48);
	  Usart_Sendbyte(USARTx,s+48);
  }
   Usart_Sendbyte(USARTx,g+48);
}

void Usart_SendString(HT_USART_TypeDef *USARTx, unsigned char *str, unsigned short len)
{
	unsigned short count = 0;
	for(; count < len; count++)
	{
		USART_SendData(USARTx, str[count]); // 修改：使用 str[count] 而不是 *str++
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TXDE) == RESET);
	}
}

/*
************************************************************
*	函数名称：	UsartPrintf
*
*	函数功能：	格式化打印信息，便于调试
*
*	入口参数：	USARTx：串口组
*				fmt：不定长参
*
*	返回参数：	无
*
*	说明：
************************************************************
*/
void UsartPrintf(HT_USART_TypeDef* USARTx, char *fmt,...) // USARTx 改为指针
{
	// 缓冲区大小可根据需要调整，296通常足够
	char UsartPrintfBuf[296]; // 类型改为 char
	va_list ap;
	// unsigned char *pStr = UsartPrintfBuf; // pStr 可以直接用 char*

	va_start(ap, fmt);
	vsnprintf(UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap); // 直接使用 UsartPrintfBuf
	va_end(ap);

    // 使用 Usart_SendStr 发送格式化后的字符串，更简洁
    Usart_SendStr(USARTx, UsartPrintfBuf);
    /*
	while(*pStr != 0)
	{
		USART_SendData(USARTx, *pStr++);
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TXDE) == RESET);
	}
    */
}

