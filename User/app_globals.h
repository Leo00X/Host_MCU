#ifndef __APP_GLOBALS_H
#define __APP_GLOBALS_H

#include "ht32.h"
#include <stdbool.h> // 使用标准的布尔类型

/*==================================================================================================
 * 宏定义 (Macros)
 *==================================================================================================*/

/**
 * @brief 定义系统支持的最大从机数量.
 */
#define MAX_SLAVES                                  (3)
#define TEST_WIFI                            		 (0)
#define ALL_SLAVE_ID   										0xAA      // 全部从机

/*==================================================================================================
 * 新增: HMI 控制请求数据结构 (HMI Control Request Data Structures)
 *==================================================================================================*/

/**
 * @brief 存放来自 HMI 的全局模式设置请求.
 */
typedef struct {
    bool new_request_flag;      // 新请求标志位: true表示有新请求
    u8   mode_request;          // 请求的模式 (0x00:自动, 0x01:手动, 0x02:睡眠)
} HMI_ModeRequest_t;

/**
 * @brief 存放来自 HMI 的从机风速控制请求.
 */
typedef struct {
    bool new_request_flag;      // 新请求标志位: true表示有新请求
    u8   fan_command;           // 请求的风速命令 (0x80:增加, 0x81:减少)
} HMI_FanRequest_t;


/*==================================================================================================
 * 数据结构定义 (Data Structures)
 *==================================================================================================*/
/**
 * @brief 从机控制状态与指令结构体.
 */
typedef struct {
    // --- 决策结果 (由 "智能决策" 任务写入) ---
    u8 target_fan_speed;        // 目标风扇转速 (0-100)
    bool target_alarm_status;   // 目标蜂鸣器报警器状态 (false: OFF, true: ON)

    // --- 发送记录 (由 "指令派发" 任务维护) ---
    u8 last_sent_fan_speed;     // 【新增】上次已发送的风扇转速指令
    bool last_sent_alarm_status;// 【新增】上次已发送的报警器状态指令
    
} SlaveControl_t;

/**
 * @brief 从机的环境数据结构体.
 * @note  V2.0新增: 精确定义从机所包含的传感器数据.
 */
typedef struct {
    float temperature;      // 温度 (单位: °C)
    float humidity;         // 湿度 (单位: %RH)
    float co_ppm;           // CO浓度 (单位: ppm)
    float smog_ppm;         // 烟雾浓度 (单位: ppm)
    float aqi_ppm;          // 空气质量指数 (单位: ppm)
} SlaveEnvData_t;

/**
 * @brief 主机的环境数据结构体.
 * @note  V2.0新增: 精确定义主机所包含的传感器数据.
 */
typedef struct {
    float temperature;      // 温度 (单位: °C)
    float humidity;         // 湿度 (单位: %RH)
    float smog_ppm;         // 烟雾浓度 (单位: ppm)
    float gas_ppm;          // 燃气浓度 (单位: ppm)
    float formaldehyde_ppb; // 甲醛浓度 (单位: ppb)
    float pm25_value; 		 // PM2.5浓度 (单位:μg/m³​ (微克/立方米)  mg/m³ (毫克/立方米) )
} HostEnvData_t;

/**
 * @brief 通用的报警阈值与超限计数结构体.
 */
typedef struct {
    float co_thresh;
    float smog_thresh;
    float aqi_thresh;
    float gas_thresh;
    float formaldehyde_thresh; // 仅主机使用
    float pm25_value_thresh; // 仅主机使用
	
	u16 co_exceed_count;        // CO 超限次数  从机
	u16 smog_exceed_count;      // 烟雾超限次数 主机+从机
	u16 aqi_exceed_count;       // AQI 超限次数 从机
	u16 gas_exceed_count;       // 燃气超限次数 主机
	u16 formaldehyde_exceed_count; // 甲醛超限次数 主机
	u16 pm25_value_exceed_count; // PM25超限次数 主机
	
} Thresholds_t;

/**
 * @brief 从机节点信息与状态结构体.
 */
typedef struct {
    bool is_online;         // 在线状态: true-在线, false-离线
    u32  last_update_tick;  // 最后一次收到数据的时间戳 (可用于超时判断)
} SlaveInfo_t;

/**
 * @brief V2.0新增: 从机节点聚合结构体.
 * @note  将一个从机的所有相关数据封装在一起，便于管理.
 */
typedef struct {
    SlaveInfo_t     info;
    SlaveEnvData_t  env_data;
    Thresholds_t    thresholds;
	 SlaveControl_t		  control;
} SlaveNode_t;

/**
 * @brief V2.0新增: 主机节点聚合结构体.
 * @note  封装主机自身的所有数据.
 */
typedef struct {
    HostEnvData_t   env_data;
    Thresholds_t    thresholds;
} HostNode_t;


/**
 * @brief 系统全局状态结构体.
 */
typedef struct {
    bool is_wifi_connected;
    bool is_onenet_connected;
    u8   system_alarm_level; // 0:正常, 1:一级警报, 2:二级警报...
} SystemStatus_t;

/**
 * @brief 主机的“数据大脑” (System Data Center).
 * @note  V2.0重构: 这是整个系统的唯一事实来源 (Single Source of Truth).
 * 结构更清晰，分为从机节点、主机节点和系统状态三大块。
 */
typedef struct {
    // 1. 从机节点数据区 (Array of Slave Nodes)
    SlaveNode_t     slaves[MAX_SLAVES];
    // 2. 本机节点数据区 (Host Node Data)
    HostNode_t      host;
    // 3. 系统状态区 (System Status)
    SystemStatus_t  status;
    // 4. 新增: HMI 请求区 (HMI Request Area)
    HMI_ModeRequest_t hmi_mode_request;         // <-- 新增: 存放全局模式请求
    HMI_FanRequest_t  hmi_fan_requests[MAX_SLAVES]; // <-- 新增: 存放对每个从机的风速请求

} SystemData_t;


extern SystemData_t g_system_data;

void APP_GLOBALS_Init(void);


#endif /* __APP_GLOBALS_H */
