#define onleyUart0 // 请确保这个宏定义符合您的项目需求
#ifdef onleyUart0

#include "uart.h"
#include <string.h> // 用于 strcmp
#include <stdio.h>  // 用于 snprintf
#include <stdbool.h> // 如果 uart.h 没有包含 bool 类型


// --- 全局变量定义 ---
u8 rbfData[20]; // 用于存储从ASRPRO接收到的指令的缓冲区
volatile bool g_command_ready = FALSE; // 命令接收完成标志, FALSE 为初始值

// --- 宏定义 ---
#define CMD_FROM_ASRPRO "DATA" // 定义期望从ASRPRO接收的指令

// 您已有的定义
#define MAX_BUFFER_SIZE 100 // 这个宏似乎没有在提供的代码片段中使用
#define __ALIGN4 __align(4) // Keil MDK 特有的对齐方式

//-----------------------------------------------------------------------------
__ALIGN4 _UART0_STRUCT rxd_comm0;
__ALIGN4 _UART0_STRUCT txd_comm0;

// 外部变量声明 (在 main.c 中定义的传感器数据)
// 这些变量由 Process_ASRPRO_Command_And_Respond 函数使用
extern int n1;
extern int n2;
extern int n3;
extern int n4;
//-----------------------------------------------------------------------------
void UART0_Configuration(void)
{
    USART_InitTypeDef USART_InitStruct; //定义在前，避免报错

    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.UART0   = 1;
    CKCUClock.Bit.AFIO    = 1;
    CKCUClock.Bit.PB      = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    GPIO_PullResistorConfig(HT_GPIOB, GPIO_PIN_3, GPIO_PR_UP);
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_2, AFIO_FUN_USART_UART);
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_3, AFIO_FUN_USART_UART);

    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_WordLength = USART_WORDLENGTH_8B;
    USART_InitStruct.USART_StopBits = USART_STOPBITS_1;
    USART_InitStruct.USART_Parity = USART_PARITY_NO;
    USART_InitStruct.USART_Mode = USART_MODE_NORMAL;

    USART_Init(HT_UART0, &USART_InitStruct);

    USART_RxCmd(HT_UART0, ENABLE);
    USART_TxCmd(HT_UART0, ENABLE);

    USART_IntConfig(HT_UART0, USART_INT_RXDR, ENABLE);
    NVIC_EnableIRQ(UART0_IRQn);
    UART0_init_buffer();
//  uprintf("UART0 Configured for ASRPRO.\r\n");
    UsartPrintf(USART_DEBUG, "UART0 Configured for ASRPRO.\r\n"); // 使用 USART_DEBUG 进行调试输出
}

//-----------------------------------------------------------------------------
void UART0_init_buffer (void)
{
    rxd_comm0.read_pt = 0;
    rxd_comm0.write_pt = 0;
    rxd_comm0.cnt = 0;

    txd_comm0.read_pt = 0;
    txd_comm0.write_pt = 0;
    txd_comm0.cnt = 0;
}

//-----------------------------------------------------------------------------
void UART0_IRQHandler(void)
{
    if (USART_GetFlagStatus(HT_UART0, USART_FLAG_TXC) == SET)
    {
        USART_ClearFlag(HT_UART0, USART_FLAG_TXC);
        if (!txd_comm0.cnt)
        {
            USART_IntConfig(HT_UART0, USART_INT_TXC, DISABLE);
        }
        else
        {
            u16 i;
            for (i = 0; i < 1; i++)
            {
                if (txd_comm0.cnt > 0)
                {
                    USART_SendData(HT_UART0, txd_comm0.buffer[txd_comm0.read_pt]);
                    txd_comm0.read_pt = (txd_comm0.read_pt + 1) % UART0_BUF_SIZE;
                    txd_comm0.cnt--;
                }
                if (!txd_comm0.cnt)
                    break;
            }
        }
    }

    if (USART_GetFlagStatus(HT_UART0, USART_FLAG_RXDR) == SET)
    {
        if (rxd_comm0.cnt < UART0_BUF_SIZE)
        {
            rxd_comm0.buffer[rxd_comm0.write_pt] = USART_ReceiveData(HT_UART0);
            rxd_comm0.write_pt = (rxd_comm0.write_pt + 1) % UART0_BUF_SIZE;
            rxd_comm0.cnt++;
        }
        else
        {
            volatile u16 tempData = USART_ReceiveData(HT_UART0);
            uprintf("UART0 RX Buffer Overflow!\r\n"); // 通过 uprintf 输出到 UART0
        }
    }
}

//-----------------------------------------------------------------------------
static uint8_t rbfIndex = 0;

void UART0_analyze_data(void)
{
    u8 tmp;

    // 新增：过时/不完整指令的超时处理逻辑
    if (rxd_comm0.cnt == 0 && rbfIndex > 0 && !g_command_ready) {
//      uprintf("UART0_analyze_data: Stale partial cmd (len %d), resetting rbfIndex and rbfData.\r\n", rbfIndex);
        rbfIndex = 0;
        if (sizeof(rbfData) > 0) { 
            rbfData[0] = '\0'; 
        }
    }

    if (rxd_comm0.cnt > 0 && !g_command_ready) { 
        // uprintf("Analyze: rxd_comm0.cnt = %d, g_ready = %d, rbfIndex = %d\r\n", rxd_comm0.cnt, g_command_ready, rbfIndex);
    }

    while (rxd_comm0.cnt > 0 && !g_command_ready)
    {
        tmp = rxd_comm0.buffer[rxd_comm0.read_pt];

        rxd_comm0.read_pt = (rxd_comm0.read_pt + 1) % UART0_BUF_SIZE;
        __disable_irq();
        rxd_comm0.cnt--;
        __enable_irq();

//      uprintf("RX Char: %c (ASCII: %d)\r\n", tmp, tmp);

        if (tmp == '\n' || tmp == '\r')
        {
//          uprintf("EOL detected! rbfIndex = %d\r\n", rbfIndex);
            if (rbfIndex > 0)
            {
                rbfData[rbfIndex] = '\0';
//              uprintf("Cmd in rbfData before set TRUE: [%s]\r\n", (char*)rbfData);
                g_command_ready = TRUE;
//              uprintf("g_command_ready SET TRUE!\r\n");
            }
            else
            {
//              uprintf("Empty EOL detected, rbfIndex reset.\r\n");
                if (sizeof(rbfData) > 0) { 
                    rbfData[0] = '\0';
                }
            }
            rbfIndex = 0; 
        }
        else if (rbfIndex < (sizeof(rbfData) - 1))
        {
            rbfData[rbfIndex++] = tmp;
        }
        else
        {
//          uprintf("rbfData overflow! rbfIndex reset.\r\n");
            rbfIndex = 0;
            if (sizeof(rbfData) > 0) { 
                rbfData[0] = '\0';
            }
        }
    }
}

//-----------------------------------------------------------------------------
void UART0_tx_data(u8 *pt, u8 len)
{
    u16 iSend = 0;
    while(len--)
    {
        while (txd_comm0.cnt >= UART0_BUF_SIZE);

        __disable_irq();
        txd_comm0.buffer[txd_comm0.write_pt] = pt[iSend++];
        txd_comm0.write_pt = (txd_comm0.write_pt + 1) % UART0_BUF_SIZE;
        txd_comm0.cnt++;
        __enable_irq();
    }

    if(txd_comm0.cnt > 0)
    {
        USART_IntConfig(HT_UART0, USART_INT_TXC, ENABLE);
        if (USART_GetFlagStatus(HT_UART0, USART_FLAG_TXDE) == SET && txd_comm0.cnt > 0)
        {
            USART_SendData(HT_UART0, txd_comm0.buffer[txd_comm0.read_pt]);
            txd_comm0.read_pt = (txd_comm0.read_pt + 1) % UART0_BUF_SIZE;
            __disable_irq();
            txd_comm0.cnt--;
            __enable_irq();
            if (txd_comm0.cnt == 0) {
                 USART_IntConfig(HT_UART0, USART_INT_TXC, DISABLE);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void UART0_test(void)
{
    u8 i,test_array[8];
    for(i=0; i<8; i++)
    {
        test_array[i] = i;
    }
    UART0_tx_data(test_array, 8);
}

//-----------------------------------------------------------------------------
#define LOG_do
void UART0_tx_string(const char *str)
{
    #ifdef LOG_do
    while (*str) {
        UART0_tx_data((u8*)str, 1);
        str++;
    }
    #endif
}

//-----------------------------------------------------------------------------
void uprintf(const char *str, ...)
{
    #ifdef LOG_do
    char buffer[256];
    va_list args;

    va_start(args, str);
    vsnprintf(buffer, sizeof(buffer), str, args);
    va_end(args);

    UART0_tx_string(buffer);
    #endif
}

//-----------------------------------------------------------------------------
void Process_ASRPRO_Command_And_Respond(void)
{
//  uprintf("Process_ASRPRO_Command_And_Respond called.\r\n");
//  uprintf("DEBUG ASRPRO CMD RX: [%s]\r\n", (char*)rbfData);

    if (strcmp((const char*)rbfData, CMD_FROM_ASRPRO) == 0)
    {
//      uprintf("CMD '[%s]' matched! Sending data to ASRPRO...\r\n", CMD_FROM_ASRPRO);

        char response_buffer[64];
        // 修改点：对 n2 和 n3 使用 %03d 进行格式化，确保输出为3位数，不足则补零
//        snprintf(response_buffer, sizeof(response_buffer), "t%dTh%dHs%03dSm%03dM\n",
//                 n4, n1, n2, n3);

//      uprintf("ASRPRO TX: %s", response_buffer); // 通过 UART0 发送给 ASRPRO
        UART0_tx_string(response_buffer);       // 通过 UART0 发送给 ASRPRO
        UsartPrintf(USART_DEBUG, response_buffer); // 通过 USART_DEBUG (例如串口1) 打印一份，方便调试
    }
    else
    {
        uprintf("DEBUG Unknown ASRPRO CMD: [%s] vs Expected: [%s]\r\n", (char*)rbfData, CMD_FROM_ASRPRO);
    }
}
//-----------------------------------------------------------------------------
#endif // onleyUart0
