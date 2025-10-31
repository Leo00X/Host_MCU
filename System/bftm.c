#include "HT32.h"
#include "bftm.h"
#include "led.h"
#include "Task_Scheduler.h"
#include "app_globals.h"

extern uint32_t g_tick_counter;
void bftm_Init(u32 nus) 
{	
  
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
  CKCUClock.Bit.BFTM0      = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);
  

  NVIC_EnableIRQ(BFTM0_IRQn);//使能中断
                                                 
  BFTM_SetCompare(HT_BFTM0, SystemCoreClock/1000000*nus);//设置比较器值
  BFTM_SetCounter(HT_BFTM0, 0);
  BFTM_IntConfig(HT_BFTM0, ENABLE);
  BFTM_EnaCmd(HT_BFTM0, ENABLE);//使能计数器

}



//bftm中断函数
void BFTM0_IRQHandler(void)
{
	Scheduler_Tick();
	
	BFTM_ClearFlag(HT_BFTM0);
}

/**
 * @brief  获取自系统启动以来经过的毫秒数.
 * @return u32: 当前的系统滴答计数值 (单位: ms).
 */
u32 Get_SystemTick(void)
{
  return g_tick_counter;
}

