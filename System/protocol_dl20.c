/**
  ******************************************************************************
  * @file    protocol_dl20.c
  * @author  MCU Code Master
  * @version V2.4 (Host)
  * @brief   主机端DL20通信协议的实现文件.
  * - V2.3: 修正了 PackCommand 函数中校验和与EOF的写入位置错误.
  * - V2.4: 移除了对 gas_ppm 的解析逻辑, 以兼容10字节负载的从机.
  ******************************************************************************
  */
#include "protocol_dl20.h"
#include "bsp_uart.h" 
#include <string.h>   
#include "uart.h"     //调试打印

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
static u8 s_expected_data_len = 0;                  

// --- 函数原型声明 ---
static void DL20_ReceiveByte_Callback(u8 received_byte);
static int  DL20_ParseFrame(const u8* buffer, u16 len, SystemData_t* system_data);

/*==================================================================================================
 * 私有辅助函数 (Private Helper Functions)
 *==================================================================================================*/
static void ResetRxStateMachine(void)
{
    s_rx_state = STATE_WAITING_FOR_SOF;
    s_rx_count = 0;
    s_expected_data_len = 0;
}

static u8 CalculateChecksum(const u8* data, u8 len)
{
    u8 checksum = 0;
    for (u8 i = 0; i < len; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

/*==================================================================================================
 * 公共 API 函数实现 (Public API Functions)
 *==================================================================================================*/
void DL20_Init(void)
{
    uprintf("DL20 INFO: Protocol Layer Initialized.\r\n");
    ResetRxStateMachine();
    BSP_UART_Register_RX_Callback(DL20_ReceiveByte_Callback);
}

static void DL20_ReceiveByte_Callback(u8 received_byte)
{
    switch (s_rx_state)
    {
        case STATE_WAITING_FOR_SOF:
        {
            if (received_byte == DL20_FRAME_SOF)
            {
                ResetRxStateMachine();
                s_rx_buffer[s_rx_count++] = received_byte;
                s_rx_state = STATE_RECEIVING_DATA;
            }
            break;
        }
        case STATE_RECEIVING_DATA:
        {
            if (s_rx_count >= RX_BUFFER_SIZE)
            {
                uprintf("DL20 ERR: RX buffer overflow. Resetting state machine.\r\n");
                ResetRxStateMachine();
                return;
            }
            s_rx_buffer[s_rx_count++] = received_byte;
            if (s_rx_count == 2)
            {
                s_expected_data_len = received_byte;
            }
            u16 total_expected_len = 4 + s_expected_data_len;
            if (s_rx_count >= 4 && s_rx_count == total_expected_len)
            {
                if (s_rx_buffer[s_rx_count - 1] == DL20_FRAME_EOF)
                {
                    uprintf("DL20 INFO: Full frame received (%d bytes), parsing...\r\n", s_rx_count);
                    DL20_ParseFrame(s_rx_buffer, s_rx_count, &g_system_data);
                }
                else
                {
                    uprintf("DL20 ERR: Invalid EOF. Expected 0x55, got 0x%02X.\r\n", s_rx_buffer[s_rx_count - 1]);
                }
                ResetRxStateMachine();
            }
            break;
        }
    }
}

static int DL20_ParseFrame(const u8* buffer, u16 len, SystemData_t* system_data)
{
    uprintf("DL20 DEBUG: Parsing frame: ");
    for(int i = 0; i < len; i++) {
        uprintf("%02X ", buffer[i]);
    }
    uprintf("\r\n");

    if (len < 7 || buffer[0] != DL20_FRAME_SOF || buffer[len - 1] != DL20_FRAME_EOF)
    {
        uprintf("DL20 ERR: Basic frame format error (SOF/EOF/MinLen).\r\n");
        return -1;
    }
    
    u8 frame_len_field = buffer[1];
    if (frame_len_field != len - 4)
    {
        uprintf("DL20 ERR: Length field mismatch. Header says %d, actual is %d.\r\n", frame_len_field, len - 4);
        return -1;
    }

    u8 checksum_calculated = CalculateChecksum(&buffer[1], frame_len_field);
    u8 checksum_received = buffer[len - 2];
    if (checksum_received != checksum_calculated)
    {
        uprintf("DL20 ERR: Checksum mismatch! Received 0x%02X, Calculated 0x%02X\r\n", checksum_received, checksum_calculated);
        return -1;
    }

    u8 slave_id = buffer[2];
    u8 func_code = buffer[3];
    const u8* payload_ptr = &buffer[4];
    u8 payload_len = frame_len_field - 2;

    if (slave_id < 1 || slave_id > MAX_SLAVES)
    {
        uprintf("DL20 ERR: Invalid Slave ID: %d.\r\n", slave_id);
        return -1;
    }
    
    SlaveEnvData_t* target_slave_env = &system_data->slaves[slave_id - 1].env_data;

    if (func_code == FUNC_REPORT_SENSOR_DATA)
    {
        if (payload_len != sizeof(SensorPayload_t))
        {
            uprintf("DL20 ERR: Payload length mismatch for sensor data. Expected %d, got %d.\r\n", sizeof(SensorPayload_t), payload_len);
            return -1;
        }
        
        SensorPayload_t* p = (SensorPayload_t*)payload_ptr;
        
        target_slave_env->temperature = (float)p->temperature / 10.0f;
        target_slave_env->humidity    = (float)p->humidity / 10.0f;
        target_slave_env->co_ppm      = (float)p->co_ppm / 10.0f;
        target_slave_env->smog_ppm    = (float)p->smog_ppm / 10.0f;
        // target_slave_env->gas_ppm     = (float)p->gas_ppm / 10.0f; // 临时移除
        target_slave_env->aqi_ppm     = (float)p->aqi_ppm / 10.0f;
        
        uprintf("DL20 INFO: Parsed sensor data from Slave %d successfully.\r\n", slave_id);
        uprintf("  Temp:%.1f Humi:%.1f CO:%.1f Smog:%.1f AQI:%.1f\r\n",
                target_slave_env->temperature, target_slave_env->humidity,
                target_slave_env->co_ppm, target_slave_env->smog_ppm,
                target_slave_env->aqi_ppm);
        
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
    u8 frame_len_field = 1 + 1 + payload_len;
    u16 total_len = 6 + payload_len;

    buffer[0] = DL20_FRAME_SOF;
    buffer[1] = frame_len_field;
    buffer[2] = slave_id;
    buffer[3] = func;
    if (payload_len > 0)
    {
        memcpy(&buffer[4], payload, payload_len);
    }
    buffer[4 + payload_len] = CalculateChecksum(&buffer[1], frame_len_field);
    buffer[5 + payload_len] = DL20_FRAME_EOF;
    
    uprintf("DL20 DEBUG: Packing command for Slave %d, Func 0x%02X. Total len: %d\r\n", slave_id, func, total_len);
    
    return total_len;
}

u16 DL20_PackAlarmCommand(u8 slave_id, u8 alarm_on, u8* buffer)
{
    AlarmCommandPayload_t p = { .alarm_on = alarm_on };
    return PackCommand(slave_id, FUNC_MASTER_CMD_ALARM, &p, sizeof(p), buffer);
}

u16 DL20_PackFanCommand(u8 slave_id, u8 fan_speed, u8* buffer)
{
    FanCommandPayload_t p = { .fan_speed = fan_speed };
    return PackCommand(slave_id, FUNC_MASTER_CMD_FAN, &p, sizeof(p), buffer);
}

