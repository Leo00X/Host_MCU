#ifndef __IAQ_ALGORITHM_H
#define __IAQ_ALGORITHM_H

#include "app_globals.h" // 必须包含全局数据结构，因为算法需要读写 g_system_data

/**
 * @brief [对外接口] 执行一次 IAQ 智能决策算法.
 * @note  此函数会被 _Task_IntelligentControl 任务周期性调用。
 * 它会读取 g_system_data 中的传感器值，
 * 并把决策结果（风扇转速）写回 g_system_data。
 */
void IAQ_RunDecisionAlgorithm(void);

#endif // __IAQ_ALGORITHM_H
