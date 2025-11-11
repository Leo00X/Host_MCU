#include "iaq_algorithm.h"
#include "app_globals.h" // 再次包含，用于访问 g_system_data 实体

//================================================================================
// 内部辅助函数 (定义为 static, 外部不可见)
//================================================================================

// --- 1. 定义污染物的“危险权重 (W)” ---
#define W_PM25      (5)  // 主机 PM2.5 权重
#define W_CO        (10) // 从机 CO 权重 (最高)
#define W_SMOKE     (8)  // 通用 烟雾 权重 (次高)
#define W_VOC       (4)  // 从机 MQ135(VOCs) 权重
#define W_TEMP      (7)  // 【新增】温度舒适度权重 (低)
#define W_HUMI      (2)  // 【新增】湿度舒适度权重 (低)


// --- 2. 定义“污染评分”的阈值 ---

/**
 * @brief 将主机 PM2.5 (假设为 gas_ppm) 读数归一化为 0-10 分
 */
static uint8_t _Normalize_PM25(float ppm_val)
{
    if (ppm_val >= 150.0f) return 10; // 严重污染
    if (ppm_val >= 75.0f)  return 6;  // 中度污染
    if (ppm_val >= 35.0f)  return 3;  // 轻度污染
    return 0; // 优良
}

/**
 * @brief 将从机 CO (MQ7) 读数归一化为 0-10 分
 */
static uint8_t _Normalize_CO(float ppm_val)
{
    if (ppm_val >= 15.0f) return 10; // 危险
    if (ppm_val >= 5.0f)  return 5;  // 警告
    return 0; // 安全
}

/**
 * @brief 将通用烟雾 (MQ2) 读数归一化为 0-10 分
 */
static uint8_t _Normalize_Smog(float ppm_val)
{
    if (ppm_val >= 300.0f) return 10; // 有烟雾
    return 0; // 无烟雾
}

/**
 * @brief 将从机空气质量 (MQ135/VOCs，假设为 aqi_ppm) 归一化为 0-10 分
 */
static uint8_t _Normalize_VOC(float ppm_val)
{
    if (ppm_val >= 200.0f) return 8; // 差
    if (ppm_val >= 50.0f)  return 4; // 中
    return 0; // 优
}

/**
 * @brief 【新增】将温度 (DHT11) 归一化为“不舒适评分”
 * @note  我们假设 18-26°C 是舒适区 (0分)
 */
static uint8_t _Normalize_Temperature(float temp)
{
    if (temp > 30.0f) return 5; // 过热
    if (temp < 15.0f) return 3; // 过冷
    return 0; // 舒适
}

/**
 * @brief 【新增】将湿度 (DHT11) 归一化为“不舒适评分”
 * @note  我们假设 40-60% 是舒适区 (0分)
 */
static uint8_t _Normalize_Humidity(float humi)
{
    if (humi > 75.0f) return 5; // 过湿 (易滋生霉菌)
    if (humi < 30.0f) return 3; // 过干
    return 0; // 舒适
}


/**
 * @brief 根据“区域综合污染指数” (Total_IAQ_Score) 决定风扇转速
 */
static uint8_t _Get_Required_Speed(uint16_t total_iaq_score)
{
    // 优先级判断：
    // 只要有 CO (10分*W_CO(10)=100) 或 烟雾 (10分*W_SMOKE(8)=80) 触发
    if (total_iaq_score >= 80) 
    {
        return 100; // 100% 全速运转 (高)
    }
    // PM2.5 严重污染 (10分*W_PM25(5)=50)
    else if (total_iaq_score >= 30)
    {
        return 60;  // 60% 中速运转
    }
    // PM2.5 轻度污染 (3分*W_PM25(5)=15)
    // 或 仅温湿度不适 (例如 5分*W_TEMP(2) + 5分*W_HUMI(2) = 20)
    else if (total_iaq_score >= 15)
    {
        return 30;  // 30% 低速通风 (舒适度调节)
    }
    else
    {
        return 0;   // 0% 关闭
    }
}


//================================================================================
// 对外接口函数 (在 .h 文件中声明)
//================================================================================

/**
 * @brief [对外接口] 执行一次 IAQ 智能决策算法.
 */
void IAQ_RunDecisionAlgorithm(void)
{
    uint16_t iaq_score_host;
    uint16_t iaq_score_slave[MAX_SLAVES];
    
    uint8_t speed_required_host;
    uint8_t speed_required_slave[MAX_SLAVES];

    // --- 1. 计算【主机区域】的污染指数和所需风速 ---
    // (主机传感器: PM2.5, 烟雾, 温湿度)
    iaq_score_host = (uint16_t)(_Normalize_PM25(g_system_data.host.env_data.pm25_value) * W_PM25) +
                     (uint16_t)(_Normalize_Smog(g_system_data.host.env_data.smog_ppm) * W_SMOKE) +
                     (uint16_t)(_Normalize_Temperature(g_system_data.host.env_data.temperature) * W_TEMP) + // 【新增】
                     (uint16_t)(_Normalize_Humidity(g_system_data.host.env_data.humidity) * W_HUMI);         // 【新增】
    
    speed_required_host = _Get_Required_Speed(iaq_score_host);

    // --- 2. 独立计算【每个从机区域】的污染指数和所需风速 ---
    for (int i = 0; i < MAX_SLAVES; i++)
    {
        // (从机传感器: CO, 烟雾, VOCs, 温湿度)
        iaq_score_slave[i] = (uint16_t)(_Normalize_CO(g_system_data.slaves[i].env_data.co_ppm) * W_CO) +
                             (uint16_t)(_Normalize_Smog(g_system_data.slaves[i].env_data.smog_ppm) * W_SMOKE) +
                             (uint16_t)(_Normalize_VOC(g_system_data.slaves[i].env_data.aqi_ppm) * W_VOC) +
                             (uint16_t)(_Normalize_Temperature(g_system_data.slaves[i].env_data.temperature) * W_TEMP) + // 【新增】
                             (uint16_t)(_Normalize_Humidity(g_system_data.slaves[i].env_data.humidity) * W_HUMI);         // 【新增】
        
        speed_required_slave[i] = _Get_Required_Speed(iaq_score_slave[i]);
    }

    // --- 3. 【核心】全局权衡与决策 ---
    for (int i = 0; i < MAX_SLAVES; i++)
    {
        uint8_t final_speed = 0;

        if (speed_required_host > speed_required_slave[i])
        {
            final_speed = speed_required_host; // 听主机的
        }
        else
        {
            final_speed = speed_required_slave[i]; // 听从机自己的
        }

        // 写入决策结果
		  g_system_data.hmi_fan_requests[i].new_request_flag = true;
        g_system_data.slaves[i].control.target_fan_speed = final_speed;
    }
}
