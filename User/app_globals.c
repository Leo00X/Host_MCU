/**
 ******************************************************************************
 * @file    app_globals.c
 * @author  MCU Code Master (HT32 Code Expert)
 * @version V2.0 (Host)
 * @brief   全局数据结构的实体定义与初始化.
 ******************************************************************************
 */
#include "app_globals.h"

/*==================================================================================================
 * 全局变量定义 (Global Variable Definitions)
 *==================================================================================================*/

/**
 * @brief 定义并初始化全局的“数据大脑”变量.
 * @note  使用 C99 风格的初始化方式，可以在编译时将所有数据清零，
 * 这在嵌入式系统中是推荐的最佳实践。
 */
SystemData_t g_system_data = {0};
uint32_t g_tick_counter = 0;


/*==================================================================================================
 * 函数定义 (Function Definitions)
 *==================================================================================================*/

/**
 * @brief 初始化所有全局数据到安全的默认状态.
 * @param None
 * @retval None
 */
void APP_GLOBALS_Init(void)
{
    u8 i;
    //----------------------------------------------------------------
    // 1. 初始化主机节点数据
    //----------------------------------------------------------------
    g_system_data.host.env_data.temperature       = 0.0f;
    g_system_data.host.env_data.humidity          = 0.0f;
    g_system_data.host.env_data.smog_ppm          = 0.0f;
    g_system_data.host.env_data.gas_ppm           = 0.0f;
    g_system_data.host.env_data.pm25_value           = 0.0f;
    g_system_data.host.env_data.formaldehyde_ppb  = 0.0f;
    
    // 设置一个默认的主机报警阈值
    g_system_data.host.thresholds.smog_thresh         = 50.0f; // 示例值
    g_system_data.host.thresholds.gas_thresh          = 20.0f; // 示例值
    g_system_data.host.thresholds.formaldehyde_thresh = 50.0f; // 示例值
    g_system_data.host.thresholds.pm25_value_thresh   = 20.0f; // 示例值
	
	
	
	 g_system_data.hmi_mode_request.mode_request = 0x00;
	 g_system_data.slaves[0].info.zigbee_short_addr = 0x1234;
	 g_system_data.slaves[1].info.zigbee_short_addr = 0x5678;
	 g_system_data.slaves[2].info.zigbee_short_addr = 0xABCD;

    //----------------------------------------------------------------
    // 2. 初始化所有从机节点数据
    //----------------------------------------------------------------
    for (i = 0; i < MAX_SLAVES; i++)
    {
        g_system_data.slaves[i].info.is_online = false; // 默认所有从机离线
        g_system_data.slaves[i].info.last_update_tick = 0;

        g_system_data.slaves[i].env_data.temperature = 0.0f;
        g_system_data.slaves[i].env_data.humidity    = 0.0f;
        g_system_data.slaves[i].env_data.co_ppm      = 0.0f;
        g_system_data.slaves[i].env_data.smog_ppm    = 0.0f;
        g_system_data.slaves[i].env_data.aqi_ppm     = 0.0f;
        
        // 设置一个默认的从机报警阈值
        g_system_data.slaves[i].thresholds.co_thresh   = 15.0f; // 示例值
        g_system_data.slaves[i].thresholds.smog_thresh = 60.0f; // 示例值

		 
		         // --- 新增: 初始化 HMI 从机风速请求 ---
        g_system_data.hmi_fan_requests[i].new_request_flag = false;
        g_system_data.hmi_fan_requests[i].fan_command = 0;
		 
		  g_system_data.slaves[i].control.target_fan_speed = 0;
    }

    //----------------------------------------------------------------
    // 3. 初始化系统状态
    //----------------------------------------------------------------
    g_system_data.status.is_wifi_connected   = false;
    g_system_data.status.is_onenet_connected = false;
    g_system_data.status.system_alarm_level  = 0; // 0代表系统正常
}
