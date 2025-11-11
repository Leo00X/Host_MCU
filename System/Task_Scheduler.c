#include "Task_Scheduler.h"
#include <string.h> // 使用 memset
#include "app_globals.h"  // 引入全局数据中心
#include "led.h"          // 用于LED心跳
#include "dht11.h"        // 用于读取温湿度
#include "mq2.h"          // 用于读取烟雾
#include "mq5.h"          // 用于读取煤气
#include "uart.h"         // 用于处理语音模块
#include "hmi_serial.h"   // 用于更新串口屏
#include "esp8266.h"      // 用于处理ESP8266
#include "onenet.h"       // 用于处理OneNet协议
#include "iaq_algorithm.h" // 用于智能算法模块
//================================================================================
// 任务调度器所需的内部变量
//================================================================================

static Task_Flags_t g_task_flags; // 全局的任务标志变量
extern volatile bool g_command_ready;
extern u8 rbfData[20];
extern uint32_t g_tick_counter;
//================================================================================
// 任务处理函数的 “实现”
//================================================================================

/**
 * @brief  [任务] 读取主机本地传感器
 */
static void _Task_ReadLocalSensors(void)
{
	 uint8_t temp_i, temp_d, humi_i, humi_d; 
	 if (DHT11_Read_Data(&temp_i, &temp_d, &humi_i, &humi_d) == 0) 
    {
      g_system_data.host.env_data.humidity    = (float)humi_i + ((float)humi_d / 10.0f);
      g_system_data.host.env_data.temperature = (float)temp_i + ((float)temp_d / 10.0f);
    }
    g_system_data.host.env_data.smog_ppm    = (float)MQ2_GetData_PPM();
    g_system_data.host.env_data.gas_ppm      = (float)MQ5_GetData_PPM();
    // 可以在这里添加读取甲醛传感器的代码
}

/**
 * @brief  [任务] 更新HMI串口屏显示
 */
static void _Task_UpdateHMI(void)
{
	if(g_system_data.hmi_mode_request.new_EnvData_flag == true)
	{
	// 更新主机数据显示
	HMI_SetNumber("p0_home.n0", "val", (int)g_system_data.host.env_data.temperature);		//
	HMI_SetNumber("p0_home.n0", "val", (int)g_system_data.host.env_data.temperature);
	HMI_SetNumber("p0_home.n0", "val", (int)g_system_data.host.env_data.temperature);
	HMI_SetNumber("p0_home.n1", "val", (int)g_system_data.host.env_data.humidity);
	HMI_SetNumber("p0_home.n2", "val", (int)g_system_data.host.env_data.smog_ppm);
	// 更新从机1数据显示
	HMI_SetNumber("p1_env.n4", "val", (int)g_system_data.slaves[0].env_data.temperature);
	HMI_SetNumber("p1_env.n5", "val", (int)g_system_data.slaves[0].env_data.humidity);
	HMI_SetNumber("p1_env.n6", "val", (int)g_system_data.slaves[0].env_data.smog_ppm);
	HMI_SetNumber("p1_env.n7", "val", (int)g_system_data.slaves[0].env_data.co_ppm);
	HMI_SetNumber("p1_env.n8", "val", (int)g_system_data.slaves[0].env_data.aqi_ppm);
	// 更新从机2数据显示
	HMI_SetNumber("p1_env.n9", "val",  (int)g_system_data.slaves[1].env_data.temperature);
	HMI_SetNumber("p1_env.n10", "val", (int)g_system_data.slaves[1].env_data.humidity);
	HMI_SetNumber("p1_env.n11", "val", (int)g_system_data.slaves[1].env_data.smog_ppm);
	HMI_SetNumber("p1_env.n12", "val", (int)g_system_data.slaves[1].env_data.co_ppm);
	HMI_SetNumber("p1_env.n13", "val", (int)g_system_data.slaves[1].env_data.aqi_ppm);
	// 更新从机3数据显示
	HMI_SetNumber("p1_env.n14", "val", (int)g_system_data.slaves[2].env_data.temperature);
	HMI_SetNumber("p1_env.n15", "val", (int)g_system_data.slaves[2].env_data.humidity);
	HMI_SetNumber("p1_env.n16", "val", (int)g_system_data.slaves[2].env_data.smog_ppm);
	HMI_SetNumber("p1_env.n17", "val", (int)g_system_data.slaves[2].env_data.co_ppm);
	HMI_SetNumber("p1_env.n18", "val", (int)g_system_data.slaves[2].env_data.aqi_ppm);
	g_system_data.hmi_mode_request.new_EnvData_flag = false;
	}
	

}

/**
 * @brief  [任务] 处理 HMI 请求和系统核心逻辑
 * @note   此任务应在主循环中被 *高频* 调用。
 */
static void _Task_ProcessLogic(void)
{
    // 1. 检查模式切换请求
    if (g_system_data.hmi_mode_request.new_request_flag == true)
    {
		 //更新OneNET
		 g_task_flags.run_task_cloud_upload = true;
		 
       uprintf("Processing Mode Change Request: %d\r\n", g_system_data.hmi_mode_request.mode_request);
		 HMI_SendCommandF("p0_home.v_fan_mode.val=%d",g_system_data.hmi_mode_request.mode_request);
		 HMI_SendCommandF("p0_home.v_fan_mode.val=%d",g_system_data.hmi_mode_request.mode_request);
		 HMI_SendCommandF("p0_home.v_fan_mode.val=%d",g_system_data.hmi_mode_request.mode_request);
		 
       g_system_data.hmi_mode_request.new_request_flag = false;
    }
	 
	 if(!(0x01 == g_system_data.hmi_mode_request.mode_request))
	 {
	 HMI_SetNumber("p2_ctrl.fan_speed_id_1", "val", (int)g_system_data.slaves[0].control.target_fan_speed);
	 HMI_SetNumber("p2_ctrl.fan_speed_id_1", "val", (int)g_system_data.slaves[0].control.target_fan_speed);
	 HMI_SetNumber("p2_ctrl.fan_speed_id_2", "val", (int)g_system_data.slaves[1].control.target_fan_speed);
	 HMI_SetNumber("p2_ctrl.fan_speed_id_3", "val", (int)g_system_data.slaves[2].control.target_fan_speed);
	 }
	 if(g_system_data.hmi_fan_requests[0].new_request_flag == true || g_system_data.hmi_fan_requests[1].new_request_flag == true || g_system_data.hmi_fan_requests[2].new_request_flag == true )
	 {	 
		//更新OneNET
		g_task_flags.run_task_cloud_upload = true;
		 
		HMI_SetNumber("p2_ctrl.fan_speed_id_1", "val", (int)g_system_data.slaves[0].control.target_fan_speed);
		HMI_SetNumber("p2_ctrl.fan_speed_id_1", "val", (int)g_system_data.slaves[0].control.target_fan_speed);
		HMI_SetNumber("p2_ctrl.fan_speed_id_2", "val", (int)g_system_data.slaves[1].control.target_fan_speed);
		HMI_SetNumber("p2_ctrl.fan_speed_id_3", "val", (int)g_system_data.slaves[2].control.target_fan_speed);
		 
		 
		g_system_data.hmi_fan_requests[0].new_request_flag = false; 
		g_system_data.hmi_fan_requests[1].new_request_flag = false; 
		g_system_data.hmi_fan_requests[2].new_request_flag = false; 
		
	 }
}

/**
 * @brief HMI 串口屏处理任务
 * @note  此任务应在主循环中被周期性调用。
 * 它负责检查来自 HMI 的新指令并将其分发到 g_system_data 的请求区。
 */
// 正确的版本
static void _Task_HMI_Process(void)
{
    HMI_Command_t hmi_cmd; 
    if (HMI_GetCommand(&hmi_cmd))
    {
        switch (hmi_cmd.function)    // 根据功能码进行分发
        {
            case 0x01:    // 功能码 0x01: 模式选择
                // 目标必须是主机 (ID 0x00)
                if (hmi_cmd.target_id == 0x00) {
                    // 更新 g_system_data 中的全局模式请求
						  g_system_data.hmi_mode_request.mode_request ++;
						  if(g_system_data.hmi_mode_request.mode_request > 2){g_system_data.hmi_mode_request.mode_request = 0 ;}
                    g_system_data.hmi_mode_request.new_request_flag = true; // 设置请求标志		
						  HMI_SendCommandF("p0_home.v_fan_mode.val=%d",g_system_data.hmi_mode_request.mode_request);
						  HMI_SendCommandF("p0_home.v_fan_mode.val=%d",g_system_data.hmi_mode_request.mode_request);
						  HMI_SendCommandF("p0_home.v_fan_mode.val=%d",g_system_data.hmi_mode_request.mode_request);
						 uprintf("mode: %2d\r\n",g_system_data.hmi_mode_request.mode_request);
                }
                break;
            case 0x02:		// 功能码 0x02: 风速控制
                // 目标必须是有效的从机ID (1, 2, or 3)
                if (hmi_cmd.target_id >= 1 && hmi_cmd.target_id <= MAX_SLAVES) {
                    u8 slave_index = hmi_cmd.target_id - 1;
                    // 写入对指定从机的控制请求
//                   g_system_data.hmi_fan_requests[slave_index].fan_command = hmi_cmd.data;
						  g_system_data.slaves[slave_index].control.target_fan_speed = hmi_cmd.data;
                    g_system_data.hmi_fan_requests[slave_index].new_request_flag = true; // 设置请求标志
						 uprintf("ID: %u. Target Fan Speed: %3d.\r\n",hmi_cmd.target_id,g_system_data.slaves[slave_index].control.target_fan_speed);
                }
					 // 目标全部从机的控制请求
					 else if(ALL_SLAVE_ID == hmi_cmd.target_id)
					 {
							for (int slave_i = 0; slave_i < MAX_SLAVES; slave_i++)
							 {
								  // 写入决策结果
								  g_system_data.slaves[slave_i].control.target_fan_speed = hmi_cmd.data;
								  g_system_data.hmi_fan_requests[slave_i].new_request_flag = true; // 设置请求标志
						 		  uprintf("ID: %u. Target Fan Speed: %3d.\r\n",slave_i + 1,g_system_data.slaves[slave_i].control.target_fan_speed);
							 }
					 }
                break;
            default:
                // 收到未定义的功能码，可以选择忽略或记录日志
                break;
        }
    }
}

/**
 * @brief  [任务] 上传数据到OneNet云平台
 */
static void _Task_CloudUpload(void)
{
    // 您原来使用 timeCount 的逻辑现在由调度器自动管理
    OneNet_SendData();
    ESP8266_Clear();
}

/**
 * @brief  [任务] 处理语音模块指令
 */
static void _Task_ProcessVoice(void)
{
    UART0_analyze_data(); // 持续分析串口数据
    
    // 您原来处理 g_command_ready 的逻辑
    if (g_command_ready)
    {
        g_command_ready = FALSE; // 清除标志
        Process_ASRPRO_Command_And_Respond();
    }
}

/**
 * @brief  [任务] 处理OneNet下行数据
 */
static void _Task_ProcessCloudDownlink(void)
{
    char *dataPtr = ESP8266_GetIPD(0);
    if(dataPtr != NULL)
    {
        OneNet_RevPro(dataPtr);
//		  uprintf("dataPtr != NULL\r\n");
    }
}



/**
 * @brief [任务] 智能联动决策任务
 * @note  此任务现在是一个“调度者”，它只负责调用真正的算法实现。
 * 所有复杂的逻辑都封装在 iaq_algorithm.c 中。
 */
static void _Task_IntelligentControl(void)
{
	if(!(0x01 == g_system_data.hmi_mode_request.mode_request))
	{
		IAQ_RunDecisionAlgorithm();    // 只需调用算法模块的接口函数
//		uprintf("IAQ Algorithm...\r\n");
	}
}


/**
 * @brief  [任务] 指令派发中心 (最终版)
 * @brief  遍历所有从机，检查其“目标状态”与“上次发送状态”是否一致.
 * @brief  如果不一致，则调用 DL20 协议函数打包，并通过串口发送新的控制指令.
 */
static void _Task_CommandDispatch(void)
{
    uint8_t tx_buffer[32]; // 用于打包的临时发送缓冲区
    uint16_t frame_len;
    
//    uprintf("SCHEDULER: Running Command Dispatch...\r\n"); // 调试时可以取消此行注释

    // 遍历所有从机节点
    for (int i = 0; i < MAX_SLAVES; i++)
    {
        // 如果从机不在线，则跳过，不发送任何指令
//        if (g_system_data.slaves[i].info.is_online == false)
//        {
//            continue;
//        }

        uint8_t slave_id = i + 1;

        // ----------- 检查1: 风扇速度指令是否需要更新 -----------
        if (g_system_data.slaves[i].control.target_fan_speed != g_system_data.slaves[i].control.last_sent_fan_speed)
        {
            uint8_t speed_cmd = g_system_data.slaves[i].control.target_fan_speed;
            
            uprintf("DISPATCH: Slave %d fan speed changed! Target: %3d%%, Last Sent: %3d%%\r\n", 
                    slave_id, speed_cmd, g_system_data.slaves[i].control.last_sent_fan_speed);
            
            // 步骤1：调用你的 DL20 协议函数进行打包
            frame_len = DL20_PackFanCommand(slave_id, speed_cmd, tx_buffer);
            
            if (frame_len > 0)
            {
                // 步骤2：【关键修改】调用你的实际串口发送函数，将打包好的数据发送给Zigbee模块
                // 请根据你的 uart 驱动，修改下面的函数名和第一个参数 (串口号)
                // 常见函数名可能为 Uart_SendBuffer, Usart_Send_Buffer, bsp_uart_send 等
                BSP_UART_SendBuffer(tx_buffer, frame_len); // <<-- 请确认此处的串口号和函数名
                
//                uprintf("DISPATCH: Sent fan command to Slave %d via UART.\r\n", slave_id);
                
                // 步骤3：发送成功后，立即更新“上次发送”的记录，防止重复发送
                g_system_data.slaves[i].control.last_sent_fan_speed = speed_cmd;
					 delay_ms(70);
					
            }
        }
        

    }
}

        // ----------- 检查2: 报警器指令是否需要更新 (同理) -----------
//        if (g_system_data.slaves[i].control.target_alarm_status != g_system_data.slaves[i].control.last_sent_alarm_status)
//        {
//            bool alarm_cmd = g_system_data.slaves[i].control.target_alarm_status;

//            uprintf("DISPATCH: Slave %d alarm status changed! Target: %d\r\n", slave_id, alarm_cmd);
//            
//            frame_len = DL20_PackAlarmCommand(slave_id, alarm_cmd, tx_buffer);
//            if (frame_len > 0)
//            {
//                // 同样，调用你的实际串口发送函数
//                BSP_UART_SendBuffer(tx_buffer, frame_len); // <<-- 请确认此处的串口号和函数名

//                uprintf("DISPATCH: Sent alarm command to Slave %d via UART.\r\n", slave_id);

//                // 更新记录
//                g_system_data.slaves[i].control.last_sent_alarm_status = alarm_cmd;
//            }
//        }

/**
 * @brief  [任务] LED闪烁 (系统心跳)
 */
static void _Task_LedToggle(void)
{
    LED1_Toggle();
}


//================================================================================
// 调度器核心函数的 “实现”
//================================================================================

/**
 * @brief  初始化任务调度器.
 */
void Scheduler_Init(void)
{
    // 清空所有任务标志
    g_task_flags = (Task_Flags_t){0};
//	 // 初始化“数据中心”
//    memset(&g_app_data, 0, sizeof(AppData_t));
	 HMI_SendCommandF("rest");
	 HMI_SendCommandF("rest");
}

/**
 * @brief  任务调度器的“时钟滴答”函数, 在1ms中断中调用.
 */
void Scheduler_Tick(void)
{
		g_tick_counter++;

		// --- 设置实时性任务标志 ---
		// 这些任务我们希望每次主循环都检查, 以获得最快响应
		//    g_task_flags.run_task_process_voice = true;//接收语音控制
		g_task_flags.run_task_process_cloud_down = true;//接收OneNet指令并回应
		g_task_flags.run_task_process_hmi_down = true;//接收显示屏控制 
		if (g_tick_counter % 800 == 0){g_task_flags.run_task_process_hmi_up = true;} // HMI上行数据更新的任务标志每500ms
		if (g_tick_counter % 1000 == 0){g_task_flags.run_task_led_toggle = true;} // LED闪烁每1000ms
		if (g_tick_counter % 5000 == 100){g_task_flags.run_task_intelligent_control = true;}//智能管理
		if (g_tick_counter % 5000 == 0){g_task_flags.run_task_read_local_sensors = true;}//主机数据采集(5秒) 
		if (g_tick_counter % 1000 == 10){g_task_flags.run_task_command_dispatch = true;}//发送管理指令 
		if (g_tick_counter % 5000 == 0){g_task_flags.run_task_update_hmi = true; g_system_data.hmi_mode_request.new_EnvData_flag = true;}//显示屏更新 
		if (g_tick_counter % 5000 == 100){g_task_flags.run_task_cloud_upload = true;}//网络数据更新
}

/**
 * @brief  运行任务调度器, 在main的while(1)中调用.
 */
void Scheduler_Run(void)
{
		if (g_task_flags.run_task_read_local_sensors)   { _Task_ReadLocalSensors();       g_task_flags.run_task_read_local_sensors = false;    }
		if (g_task_flags.run_task_update_hmi)           { _Task_UpdateHMI();              g_task_flags.run_task_update_hmi = false;           }
		if (g_task_flags.run_task_cloud_upload)         { _Task_CloudUpload();            g_task_flags.run_task_cloud_upload = false;         }
		if (g_task_flags.run_task_process_voice)        { _Task_ProcessVoice();           g_task_flags.run_task_process_voice = false;        }
		if (g_task_flags.run_task_process_cloud_down)   { _Task_ProcessCloudDownlink();   g_task_flags.run_task_process_cloud_down = false;   }
		if (g_task_flags.run_task_process_hmi_down)     { _Task_HMI_Process();            g_task_flags.run_task_process_hmi_down = false;     }
		if (g_task_flags.run_task_led_toggle)           { _Task_LedToggle();              g_task_flags.run_task_led_toggle = false;           }
		if (g_task_flags.run_task_intelligent_control)  { _Task_IntelligentControl();     g_task_flags.run_task_intelligent_control = false;  }
		if (g_task_flags.run_task_command_dispatch)     { _Task_CommandDispatch();        g_task_flags.run_task_command_dispatch = false;     }
		if (g_task_flags.run_task_process_hmi_up)       { _Task_ProcessLogic();           g_task_flags.run_task_process_hmi_up = false;     }
}
