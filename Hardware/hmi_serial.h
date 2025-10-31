// hmi_serial.h -- 串口屏通信模块头文件
#ifndef __HMI_SERIAL_H
#define __HMI_SERIAL_H


#include "ht32.h"
#include <stdio.h>     // 为了 vsnprintf, size_t
#include <stdarg.h>    // 为了 va_list 等可变参数处理
#include <stdbool.h>   // 为了使用 bool 类型

// --- 配置 ---
// 定义用于格式化命令的最大缓冲区大小
#define HMI_CMD_BUFFER_SIZE 128

// --- HMI -> Host 接收协议定义 ---

// 根据协议文档，数据帧为5字节定长 [cite: 14]
#define HMI_FRAME_LENGTH 5
// 帧头固定为 0xA5 [cite: 15]
#define HMI_FRAME_HEADER 0xA5

/**
 * @brief HMI 指令结构体
 * @note 用于存储从 HMI 接收并成功解析的指令。
 */
typedef struct {
    uint8_t function;  // 功能码 (Byte 1) [cite: 15]
    uint8_t target_id; // 目标ID (Byte 2) [cite: 15]
    uint8_t data;      // 数据 (Byte 3) [cite: 15]
} HMI_Command_t;


// --- 初始化 ---
/**
 * @brief 初始化串口屏通信。
 * @note 发送初始序列以确保串口屏准备就绪。
 * 假设底层的 UART (例如 USART1) 已经被初始化。
 */
void HMI_Init(void);

// --- 核心发送函数 ---
/**
 * @brief 向串口屏发送原始字符串 (不包含结束符)。
 * @param str 要发送的以 null 结尾的字符串。
 */
void HMI_SendStringRaw(const char *str);

/**
 * @brief 发送 3 字节的命令结束符 (0xFF 0xFF 0xFF)。
 */
void HMI_SendEndCommand(void);

/**
 * @brief 发送一个完整的命令字符串，后跟结束符。
 * @param cmd 要发送的以 null 结尾的命令字符串。
 */
void HMI_SendCommand(const char *cmd);

/**
 * @brief 发送一个格式化的命令字符串，后跟结束符。
 * 类似于 printf 的功能。
 * @param format 格式化字符串。
 * @param ...    匹配格式化字符串的可变参数。
 * @return 成功返回 0，缓冲区溢出错误返回 -1。
 */
int HMI_SendCommandF(const char *format, ...);

// --- 新增: 核心接收函数 ---

/**
 * @brief 处理从 USART 接收到的单个字节。
 * @note  此函数应在 USART 的接收中断服务程序 (ISR) 中或在轮询循环中被调用。
 * 它内部维护一个状态机，用于解析协议帧。
 * @param byte 从串口接收到的字节。
 */
void HMI_ReceiveByte(uint8_t byte);

/**
 * @brief 检查是否有新的、合法的指令已接收。
 * @note  此函数应在主循环或 HMI 处理任务中调用。
 * 如果一个完整的、校验通过的指令帧被接收，函数返回 true，
 * 并将解析出的指令内容复制到 cmd 指针指向的结构体中。
 * @param cmd 指向 HMI_Command_t 结构体的指针，用于存储解析出的指令。
 * @return 如果有新指令则返回 true，否则返回 false。
 */
bool HMI_GetCommand(HMI_Command_t *cmd);


// --- 便捷函数 (示例) ---
/**
 * @brief 设置串口屏控件的数值属性。
 *        示例: HMI_SetNumber("n0", "val", 123); // 发送 "n0.val=123\xff\xff\xff"
 * @param control_name 控件名称 (例如 "n0", "x0")。
 * @param property     属性名称 (例如 "val")。
 * @param value        要设置的整数值。
 * @return 成功返回 0，错误 (例如缓冲区溢出) 返回 -1。
 */
int HMI_SetNumber(const char *control_name, const char *property, int32_t value);

/**
 * @brief 设置串口屏控件的文本属性。
 *        自动在文本前后添加英文双引号 ""。
 *        示例: HMI_SetText("t0", "txt", "你好"); // 发送 "t0.txt=\"你好\"\xff\xff\xff"
 * @param control_name 控件名称 (例如 "t0", "b0")。
 * @param property     属性名称 (例如 "txt")。
 * @param text         要设置的以 null 结尾的字符串值。
 * @return 成功返回 0，错误 (例如缓冲区溢出) 返回 -1。
 */
int HMI_SetText(const char *control_name, const char *property, const char *text);

/**
 * @brief 使用格式化的文本设置串口屏控件的文本属性。
 *        自动在格式化后的文本前后添加英文双引号 ""。
 *        示例: HMI_SetTextF("t1", "txt", "值: %d", myValue);
 * @param control_name 控件名称。
 * @param property     属性名称。
 * @param format       文本的格式化字符串。
 * @param ...          匹配格式化字符串的可变参数。
 * @return 成功返回 0，错误 (例如缓冲区溢出) 返回 -1。
*/
int HMI_SetTextF(const char *control_name, const char *property, const char *format, ...);


// --- 可选: 添加常用命令的函数 ---
/**
 * @brief 使用指定颜色清除串口屏屏幕。
 * @param color 颜色名称字符串 (例如 "RED", "BLUE", "WHITE")。请使用你的串口屏支持的颜色名称。
 * @return 成功返回 0，错误返回 -1。
 */
int HMI_ClearScreen(const char *color);

/**
 * @brief 切换串口屏上当前显示的页面。
 * @param page_name 页面的名称或索引 (例如 "main", "page0")。
 * @return 成功返回 0，错误返回 -1。
*/
int HMI_ChangePage(const char *page_name);


#endif // __HMI_SERIAL_H
