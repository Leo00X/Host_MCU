#include "led.h"
#include "HT32.h"
#include "ht32f5xxxx_gpio.h"
void Led_Init(void)
{
		CKCU_PeripClockConfig_TypeDef LEDCLOCK ={{0}};
		LEDCLOCK.Bit.PC = 1; //使用PC
		
		//CKCU_PeripClockConfig(CKCU_PeripClockConfig_TypeDef Clock, ControlStatus Cmd);
		CKCU_PeripClockConfig(LEDCLOCK,ENABLE); //使能时钟
		//GPIO_DirectionConfig(HT_GPIO_TypeDef* HT_GPIOx, u16 GPIO_PIN_nBITMAP, GPIO_DIR_Enum GPIO_DIR_INorOUT);
		GPIO_DirectionConfig(HT_GPIO_PORT,HT_GPIO_PIN,GPIO_DIR_OUT); //配置GPIO PC15为输出模式
		//GPIO_WriteOutBits(HT_GPIO_TypeDef* HT_GPIOx, u16 GPIO_PIN_nBITMAP, FlagStatus Status);
		GPIO_WriteOutBits(HT_GPIO_PORT,HT_GPIO_PIN,SET); //将LED1 LED2 熄灭	
}

void LED1_Toggle(void)
{
    // 使用库函数直接翻转 PC15 引脚的输出电平
    if( GPIO_ReadOutBit(HT_GPIOC, GPIO_PIN_14) == 1)
			GPIO_WriteOutBits(HT_GPIO_PORT,GPIO_PIN_14,RESET);
	 else
			GPIO_WriteOutBits(HT_GPIO_PORT,GPIO_PIN_14,SET);
}

/**
  * @brief  翻转LED2的亮灭状态.
  * @retval None
  */
void LED2_Toggle(void)
{
    // 使用库函数直接翻转 PC15 引脚的输出电平
    if( GPIO_ReadOutBit(HT_GPIOC, GPIO_PIN_15) == 0)
			GPIO_WriteOutBits(HT_GPIO_PORT,GPIO_PIN_15,SET);
	 else
			GPIO_WriteOutBits(HT_GPIO_PORT,GPIO_PIN_15,RESET);
}
