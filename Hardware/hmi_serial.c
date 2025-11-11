// hmi_serial.c -- 串口屏通信模块源文件 (适用于 HT32)
#include "hmi_serial.h"
#include "usart.h"      // 包含 HT32 的 USART 头文件 (确保这是你的 HT32 USART 定义)
#include "delay.h"      // 为了使用 delay_ms (确保这是 HT32 的延时函数)
#include <stdio.h>      // 为了使用 vsnprintf, snprintf
#include <stdarg.h>     // 为了使用 va_list, va_start, va_end
#include <stdint.h>     // 包含标准整数类型
#include <stdbool.h>    // 为了使用 bool 类型

// --- 接收状态机所需私有变量 ---

// 接收状态枚举
typedef enum {
    HMI_RX_STATE_WAIT_HEADER,  // 等待帧头 (0xA5)
    HMI_RX_STATE_RECEIVING,    // 正在接收数据
} HMI_ReceiveState_t;

static HMI_ReceiveState_t g_rx_state = HMI_RX_STATE_WAIT_HEADER; // 当前接收状态
static uint8_t g_rx_buffer[HMI_FRAME_LENGTH];                    // 接收缓冲区
static uint8_t g_rx_count = 0;                                   // 当前已接收字节数

static volatile bool g_new_cmd_available = false; // 新指令标志
static HMI_Command_t g_last_cmd;                  // 最后一条有效指令

// --- 私有辅助函数 ---

/**
 * @brief 通过 HT32 的 USART1 (COM1_PORT) 发送单个字节。
 * @param byte 要发送的字节。
 */
static void HMI_SendByte(uint8_t byte) {
    // 使用 HT32 的 USART 发送函数，目标是 COM1_PORT (HT_USART1)
    USART_SendData(COM1_PORT, byte);

    // 重要: 等待发送数据寄存器为空 (Transmit Data Register Empty)
    // 注意：HT32 使用 USART_FLAG_TXDE，STM32 使用 USART_FLAG_TXE
    while (USART_GetFlagStatus(COM1_PORT, USART_FLAG_TXDE) == RESET);
}

/**
 * @brief (私有) 处理接收到的完整数据帧。
 * 进行校验和检查，如果合法则更新指令。
 */
static void HMI_ProcessFrame(void) {
    // 1. 计算校验和
    // 校验和是前4个字节的累加和 [cite: 16]
    uint8_t checksum = (g_rx_buffer[0] + g_rx_buffer[1] + g_rx_buffer[2] + g_rx_buffer[3]) & 0xFF;

    // 2. 验证校验和
    if (checksum == g_rx_buffer[4]) {
        // 校验成功，解析数据
        g_last_cmd.function  = g_rx_buffer[1];
        g_last_cmd.target_id = g_rx_buffer[2];
        g_last_cmd.data      = g_rx_buffer[3];
        g_new_cmd_available  = true; // 设置新指令标志
    }
    // 如果校验失败，则静默丢弃该帧，等待下一帧的帧头
}


// --- 公共函数实现 ---

void HMI_Init(void) {
    // 确保在调用此函数之前，HT32 的 USART1 (COM1_PORT) 已经被正确初始化
    // 例如调用了 USART1_Configuration()
    delay_ms(200);        // 原始代码推荐的初始延时
    HMI_SendEndCommand(); // 发送结束符序列作为启动信号
    delay_ms(200);        // 启动序列后的延时

    // 初始化接收状态
    g_rx_state = HMI_RX_STATE_WAIT_HEADER;
    g_rx_count = 0;
    g_new_cmd_available = false;
}

void HMI_SendStringRaw(const char *str) {
    while (*str) {
        HMI_SendByte((uint8_t)*str++); // 逐字节发送
    }
}

void HMI_SendEndCommand(void) {
    HMI_SendByte(0xFF); // 发送第一个结束符
    HMI_SendByte(0xFF); // 发送第二个结束符
    HMI_SendByte(0xFF); // 发送第三个结束符
}

void HMI_SendCommand(const char *cmd) {
    HMI_SendStringRaw(cmd); // 先发送命令字符串
    HMI_SendEndCommand();   // 再发送结束符
}

int HMI_SendCommandF(const char *format, ...) {
    char buffer[HMI_CMD_BUFFER_SIZE]; // 定义缓冲区
    va_list args;                     // 定义可变参数列表
    int len;                          // 存储格式化后的长度

    va_start(args, format);           // 初始化可变参数列表
    // 使用 vsnprintf 防止缓冲区溢出，更安全
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);                     // 结束可变参数列表处理

    if (len < 0 || len >= sizeof(buffer)) {
        // 处理错误：缓冲区太小或编码错误
        return -1; // 返回错误码
    }

	//==============================================================
	// 1. 进入临界区 (保持不变)
	//==============================================================
//    __disable_irq();

    // 发送格式化后的字符串和结束符
    HMI_SendStringRaw(buffer);
    HMI_SendEndCommand();

	//==============================================================
	// 2. 退出临界区 (保持不变)
	//==============================================================
//    __enable_irq();

    return 0; // 返回成功码
}

// --- 新增: 接收协议实现 ---

void HMI_ReceiveByte(uint8_t byte) {
    switch (g_rx_state) {
        case HMI_RX_STATE_WAIT_HEADER:
            // 等待帧头
            if (byte == HMI_FRAME_HEADER) {
                g_rx_buffer[0] = byte;
                g_rx_count = 1;
                g_rx_state = HMI_RX_STATE_RECEIVING;
            }
            break;

        case HMI_RX_STATE_RECEIVING:
            // 接收数据部分
            g_rx_buffer[g_rx_count++] = byte;

            // 检查是否已接收完整帧
            if (g_rx_count >= HMI_FRAME_LENGTH) {
                // 接收满5字节，处理该帧 [cite: 14]
                HMI_ProcessFrame();
                
                // 重置状态机，准备接收下一帧
                g_rx_state = HMI_RX_STATE_WAIT_HEADER;
                g_rx_count = 0;
            }
            break;
    }
}

bool HMI_GetCommand(HMI_Command_t *cmd) {
    if (g_new_cmd_available) {
        // 如果有新指令
        if (cmd != NULL) {
            // 拷贝指令到用户提供的结构体
            *cmd = g_last_cmd;
        }
        // 清除标志，表示指令已被读取
        g_new_cmd_available = false;
        return true;
    }
    return false;
}


// --- 便捷发送函数实现 ---

int HMI_SetNumber(const char *control_name, const char *property, int32_t value) {
    // 格式: control.property=value
    // 例如: "n0.val=123"
    // 注意 HT32 的 printf 可能不支持 %ld 直接打印 int32_t，如果遇到问题，
    // 可能需要先转换为 long 或者使用更具体的打印格式。
    // 但 snprintf 通常能较好地处理标准类型。
    return HMI_SendCommandF("%s.%s=%ld", control_name, property, (long)value); // 显式转换为 long
}

int HMI_SetText(const char *control_name, const char *property, const char *text) {
    // 格式: control.property="text"
    // 例如: "t0.txt=\"你好\""
    return HMI_SendCommandF("%s.%s=\"%s\"", control_name, property, text);
}

int HMI_SetTextF(const char *control_name, const char *property, const char *format, ...)
{
    char text_buffer[HMI_CMD_BUFFER_SIZE / 2]; // 为格式化的文本分配一个合理的缓冲区
    char command_buffer[HMI_CMD_BUFFER_SIZE];  // 最终命令的缓冲区
    va_list args;
    int text_len; // 格式化后文本的长度
    int cmd_len;  // 最终命令的长度

    // 1. 格式化文本部分
    va_start(args, format);
    text_len = vsnprintf(text_buffer, sizeof(text_buffer), format, args);
    va_end(args);

    if (text_len < 0 || text_len >= sizeof(text_buffer)) {
        return -1; // 文本格式化错误或缓冲区溢出
    }

    // 2. 格式化最终的命令字符串: control.property="formatted_text"
    cmd_len = snprintf(command_buffer, sizeof(command_buffer), "%s.%s=\"%s\"", control_name, property, text_buffer);

     if (cmd_len < 0 || cmd_len >= sizeof(command_buffer)) {
        return -1; // 命令格式化错误或缓冲区溢出
    }

    // 3. 发送命令
    HMI_SendStringRaw(command_buffer);
    HMI_SendEndCommand();

    return 0; // 返回成功
}


int HMI_ClearScreen(const char *color)
{
    // 格式: cls COLOR
    // 例如: cls RED
    return HMI_SendCommandF("cls %s", color);
}

int HMI_ChangePage(const char *page_name)
{
     // 格式: page page_name
     // 例如: page main
     return HMI_SendCommandF("page %s", page_name);
}
