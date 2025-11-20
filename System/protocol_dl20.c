/**
  ******************************************************************************
  * @file    protocol_dl20.c
  * @author  HT32 Code Expert
  * @version V3.1 (Host, Refactored for E72 & app_globals V2.x)
  * @brief   主机端DL20通信协议的实现文件.
  * - V3.0: 重构，与 E72 驱动层解耦。
  * - V3.1: 
  * - 修正了 V3.0 中 HandleReceivedFrame 的拼写错误 ( '_' )。
  * - 移除了不必要的大小端转换 (HT_TO_LE_...)。
  * - (关键) 移除了对 SlaveEnv_t 的引用。
  * - (关键) 现在直接包含 "app_globals.h" 并更新全局的 "g_system_data" 结构体。
  * - 移除了 _GetSlaveEnv 函数。
  ******************************************************************************
  */
#include "protocol_dl20.h"
#include <string.h>   
#include "uart.h"     // 调试打印 (uprintf)

// !!! 关键变更: 
// 1. 包含 E72 驱动层 (用于发送)
#include "E72_ZigBee.h" 
// 2. 包含“数据大脑” (用于写入)
#include "app_globals.h"

/*==================================================================================================
 * 宏定义与私有类型 (Macros & Private Types)
 *==================================================================================================*/
#define RX_BUFFER_SIZE      (128)

typedef enum {
    STATE_WAITING_FOR_SOF,
    STATE_RECEIVING_DATA  
} RxState_t;

/*==================================================================================================
 * 私有变量 (Private Variables)
 *==================================================================================================*/
static RxState_t s_rx_state = STATE_WAITING_FOR_SOF;
static u8 s_rx_buffer[RX_BUFFER_SIZE];              
static u16 s_rx_count = 0;                          
static u8 s_expected_data_len = 0;                  // DL20协议中的Len字段

/*==================================================================================================
 * 外部全局变量 (Extern Global Variables)
 *==================================================================================================*/
 
/**
 * @brief "数据大脑" 的全局唯一实例。
 * @note  此变量必须定义在 app_globals.c 或 main.c 中。
 */
extern SystemData_t g_system_data;
extern uint32_t g_tick_counter;

/*==================================================================================================
 * 私有函数原型 (Private Function Prototypes)
 *==================================================================================================*/
static u8 CalculateChecksum(const u8* data, u8 len);
static int HandleReceivedFrame(u8* frame_data, u8 frame_len_field);
static u16 PackCommand(u8 slave_id, u8 func, const void* payload, u8 payload_len, u8* buffer);
// ( _GetSlaveEnv 函数已被移除 )

// (新) 内部状态机，用于处理单个字节
static void _Protocol_ParseByte(u8 data);

/*==================================================================================================
 * 公共 API 函数 (Public API Functions)
 *==================================================================================================*/

/**
 * @brief (新) 初始化 DL20 协议层。
 */
void Protocol_Init(void)
{
    s_rx_state = STATE_WAITING_FOR_SOF;
    s_rx_count = 0;
}

/**
 * @brief (新) 处理来自E72驱动的原始应用数据。
 */
void Protocol_ProcessRxData(const u8* pData, u16 u16Len)
{
    u16 i;
    for (i = 0; i < u16Len; i++)
    {
        // 将 E72 传来的数据，一个字节一个字节地“喂”给
        // 我们原有的状态机逻辑
        _Protocol_ParseByte(pData[i]);
    }
}

/**
 * @brief (修改) 向指定从机发送风扇控制命令。
 */
void Protocol_SendFanCommand(u16 u16DestAddr, u8 fan_speed)
{
    u8 tx_buffer[32]; // 存储 0xAA...0x55 帧
    u16 frame_len;
    FanCommandPayload_t payload;
    
    payload.fan_speed = fan_speed;
    
    // 1. 打包你的“应用层” (DL20) 协议帧
    //    注意：slave_id (0x01) 在 E72 模式下可能意义不大, 但协议需要
    frame_len = PackCommand(0x01, FUNC_MASTER_CMD_FAN, &payload, sizeof(payload), tx_buffer);
    
    // 2. 调用 E72 "传输层" API 发送
    //    目标端口(Endpoint)通常是 0x01 (根据 E72 规范)
    E72_SendTransparentData(u16DestAddr, 0x01, tx_buffer, frame_len);
}

/**
 * @brief (修改) 向指定从机发送报警器控制命令。
 */
void Protocol_SendAlarmCommand(u16 u16DestAddr, u8 alarm_on)
{
    u8 tx_buffer[32]; // 存储 0xAA...0x55 帧
    u16 frame_len;
    AlarmCommandPayload_t payload;
    
    payload.alarm_on = alarm_on;
    
    frame_len = PackCommand(0x01, FUNC_MASTER_CMD_ALARM, &payload, sizeof(payload), tx_buffer);
    
    E72_SendTransparentData(u16DestAddr, 0x01, tx_buffer, frame_len);
}

/*==================================================================================================
 * 内部（私有）函数 (Internal (Private) Functions)
 *==================================================================================================*/

/**
 * @brief (新) 内部状态机，处理单个字节。
 * @note  这段逻辑是从你原有的 V2.4 UART_Rx_Callback 中剥离出来的。
 */
static void _Protocol_ParseByte(u8 data)
{
    if (s_rx_state == STATE_WAITING_FOR_SOF)
    {
        if (data == DL20_FRAME_SOF)
        {
            s_rx_state = STATE_RECEIVING_DATA;
            s_rx_count = 0;
            // 注意：我们不把SOF放入缓冲区，缓冲区从Len字段开始
        }
    }
    else if (s_rx_state == STATE_RECEIVING_DATA)
    {
        if (s_rx_count == 0) // 第一个字节: Len 字段
        {
            s_expected_data_len = data;
            s_rx_buffer[s_rx_count++] = data;
        }
        else // 后续字节: SlaveID, Func, Payload, FCS, EOF
        {
            s_rx_buffer[s_rx_count++] = data;
            
            // 检查是否收到了包含EOF在内的完整帧
            // 缓冲区总字节数 = Len(1) + Data(s_expected_data_len) + FCS(1) + EOF(1)
            if (s_rx_count >= (s_expected_data_len + 3))
            {
                // 检查 EOF (位于 s_rx_count - 1)
                if (s_rx_buffer[s_rx_count - 1] == DL20_FRAME_EOF)
                {
                    // 检查 FCS (位于 s_rx_count - 2)
                    u8 received_fcs = s_rx_buffer[s_rx_count - 2];
                    
                    // 校验和计算范围: [Len]...[Payload]
                    // 长度为: Len(1) + SlaveID(1) + Func(1) + Payload(N) = 1 + s_expected_data_len
                    u8 calculated_fcs = CalculateChecksum(s_rx_buffer, s_expected_data_len + 1); 
                    
                    if (received_fcs == calculated_fcs)
                    {
                        // 帧完美接收！
                        HandleReceivedFrame(s_rx_buffer, s_expected_data_len);
                    }
                    else {
                        uprintf("DL20 ERR: Checksum fail! Rcv:0x%02X Calc:0x%02X\r\n", received_fcs, calculated_fcs);
                    }
                }
                else {
                    uprintf("DL20 ERR: EOF mismatch! Got:0x%02X\r\n", s_rx_buffer[s_rx_count - 1]);
                }
                
                // 无论成功与否，都复位状态机
                s_rx_state = STATE_WAITING_FOR_SOF;
            }
        }
        
        // 缓冲区溢出保护
        if (s_rx_count >= RX_BUFFER_SIZE)
        {
            uprintf("DL20 ERR: Buffer overflow!\r\n");
            s_rx_state = STATE_WAITING_FOR_SOF;
        }
    }
}

/**
 * @brief (已移除) _GetSlaveEnv(u8 slave_id)
 */

static u8 CalculateChecksum(const u8* data, u8 len)
{
    u8 checksum = 0;
    for (u8 i = 0; i < len; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief  (V3.1 关键修正) 处理一帧校验通过的 DL20 数据。
 * @note   此函数现在直接更新全局的 "g_system_data" 结构体。
 */
static int HandleReceivedFrame(u8* frame_data, u8 frame_len_field)
{
    u8 slave_id = frame_data[1];  // [0]是Len
    u8 func_code = frame_data[2];
    u8* payload = &frame_data[3];
    u8 payload_len = frame_len_field - 2; // Len - SlaveID(1) - Func(1)
    
    // (V3.1 修正) 检查 Slave ID 是否有效，并获取指向“数据大脑”中对应节点的指针
    if (slave_id == 0 || slave_id > MAX_SLAVES)
    {
        uprintf("DL20 ERR: Invalid Slave ID %d.\r\n", slave_id);
        return -1;
    }
    
    // (V3.1 修正) 直接获取指向 g_system_data 中从机节点的指针
    // (假设 slave_id 是 1-based, 对应数组索引 0-based)
    SlaveNode_t* target_slave = &g_system_data.slaves[slave_id - 1];

    // (V3.1 修正) 更新 "数据大脑" 中的在线状态
    target_slave->info.is_online = true;
    target_slave->info.last_update_tick = g_tick_counter; // (假设 g_tick_counter 是你的全局 SysTick 计数器)

    if (func_code == FUNC_REPORT_SENSOR_DATA)
    { 
        if (payload_len != sizeof(SensorPayload_t))
        {
            uprintf("DL20 ERR: Payload len mismatch for SENSOR_DATA (ID:%d). Exp:%d, Got:%d\r\n", 
                    slave_id, sizeof(SensorPayload_t), payload_len);
            return -1;
        }
        
        SensorPayload_t* pSensor = (SensorPayload_t*)payload;
        
        // (V3.1 修正) 将数据写入 "数据大脑" 的 env_data 字段
        // (假设 STM32 和 HT32 都是小端模式，无需转换)
        target_slave->env_data.temperature = (float)(pSensor->temperature) / 10.0f;
        target_slave->env_data.humidity    = (float)(pSensor->humidity) / 10.0f;
        target_slave->env_data.co_ppm      = pSensor->co_ppm;
        target_slave->env_data.smog_ppm    = pSensor->smog_ppm;
        target_slave->env_data.aqi_ppm     = pSensor->aqi_ppm;

        uprintf("DL20: Rcvd data from Slave %d. T:%.1f H:%.1f\r\n",
                slave_id, target_slave->env_data.temperature, target_slave->env_data.humidity);
        
        return slave_id;
    }
    else
    {
        uprintf("DL20 WARN: Unknown function code 0x%02X from Slave %d.\r\n", func_code, slave_id);
    }
    
    return -1;
}

static u16 PackCommand(u8 slave_id, u8 func, const void* payload, u8 payload_len, u8* buffer)
{
    u8 frame_len_field = 1 + 1 + payload_len; // SlaveID + Func + Payload
    u16 total_len = 6 + payload_len; // SOF+Len+Data+FCS+EOF

    buffer[0] = DL20_FRAME_SOF;
    buffer[1] = frame_len_field;
    buffer[2] = slave_id;
    buffer[3] = func;
    if (payload_len > 0)
    {
        memcpy(&buffer[4], payload, payload_len);
    }
    
    // (V3.0 修正) 校验和的计算范围是 [Len]...[Payload]
    // 总长度是 1 (Len) + frame_len_field
    buffer[4 + payload_len] = CalculateChecksum(&buffer[1], frame_len_field + 1);
    
    buffer[5 + payload_len] = DL20_FRAME_EOF;
    
    return total_len; // 返回总帧长
}
