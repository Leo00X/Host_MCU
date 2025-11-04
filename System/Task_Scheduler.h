#ifndef __TASK_SCHEDULER_H
#define __TASK_SCHEDULER_H

#include <stdbool.h> // 引入 bool 类型, 用于定义标志位

//================================================================================
// 步骤 1: 为您的新风机主机项目定义 "任务清单"
//================================================================================
typedef struct {
    // --- 周期性任务 ---
    bool run_task_read_local_sensors;   // [周期] 读取主机本地传感器的任务标志
    bool run_task_update_hmi;           // [周期] 更新串口屏显示的任务标志
    bool run_task_cloud_upload;         // [周期] 上传数据到OneNet云平台的任务标志
    
    // --- 实时性任务 (尽可能快地执行) ---
    bool run_task_process_voice;        // [实时] 处理语音模块指令的任务标志
    bool run_task_process_cloud_down;   // [实时] 处理OneNet下行数据的任务标志
    bool run_task_process_hmi_down;   // [实时] 处理HMI下行数据的任务标志
	 bool run_task_process_hmi_up;   // [实时] 处理HMI上行数据的任务标志
    
    // --- 调试/状态指示任务 ---
    bool run_task_led_toggle;           // [周期] LED闪烁, 作为系统心跳指示
	 bool run_task_intelligent_control;  // 【新增】智能联动决策任务
	 bool run_task_command_dispatch;     // 【新增】向下行指令派发任务

} Task_Flags_t;


//================================================================================
// 调度器核心函数声明 (通常不需要修改这里)
//================================================================================

/**
  * @brief  初始化任务调度器.
  * @note   此函数会清空所有任务标志, 准备开始新一轮调度.
  */
void Scheduler_Init(void);

/**
  * @brief  任务调度器的“时钟滴答”函数.
  * @note   这个函数必须在一个周期性的地方被调用, 例如一个1ms的定时器中断中.
  * 它负责根据设定的时间间隔, 设置上面结构体中的各个任务标志位.
  */
void Scheduler_Tick(void);

/**
  * @brief  运行任务调度器.
  * @note   这个函数应该在 main 函数的 while(1) 循环中被唯一地、反复地调用.
  * 它会检查所有任务标志位, 如果某个标志位被置位, 就执行对应的任务, 然后清除该标志.
  */
void Scheduler_Run(void);


#endif /* __TASK_SCHEDULER_H */
