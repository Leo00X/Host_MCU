#include "ht32.h"
#include "delay.h"
#include "mq5.h"


void MQ5_Init(void)
{

    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};//定义结构体，配置时钟
    CKCUClock.Bit.PA = 1;                       //开启GPIOA时钟
    CKCUClock.Bit.ADC = 1;                      //开启ADC时钟
    CKCUClock.Bit.AFIO = 1;                     //开启AFIO时钟
    CKCUClock.Bit.PDMA = 1;                     //开启PDMA时钟
    CKCU_PeripClockConfig(CKCUClock, ENABLE);   //使能外设时钟
		

		GPIO_DirectionConfig(HT_GPIOA,GPIO_PIN_7,GPIO_DIR_IN); //配置PD1/2/3为输入模式	
//		GPIO_PullResistorConfig(HT_GPIOA,GPIO_PIN_2,GPIO_PR_DOWN); //上拉
		GPIO_InputConfig(HT_GPIOA,GPIO_PIN_7,ENABLE);	
		
		
		ADC_Reset(HT_ADC);                          //重置ADC
    CKCU_SetADCPrescaler(CKCU_ADCPRE_DIV4);     //设置ADC时钟分频
   
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_7, AFIO_FUN_ADC); 

    ADC_RegularChannelConfig(HT_ADC, ADC_CH_7, 1);      //配置ADC规则组，在规则组的序列1写入通道2

//    ADC_RegularGroupConfig(HT_ADC, CONTINUOUS_MODE, 3, 1);  //配置ADC转换模式为连续转换，队列长度为1
		ADC_RegularGroupConfig(HT_ADC, ONE_SHOT_MODE, 2, 1); 
    ADC_RegularTrigConfig(HT_ADC, ADC_TRIG_SOFTWARE);       //配置ADC触发源软件触发
    ADC_SamplingTimeConfig(HT_ADC, 16);                     //配置ADC输入通道采样时间

    ADC_Cmd(HT_ADC, ENABLE);                                //使能ADC
//    ADC_SoftwareStartConvCmd(HT_ADC, ENABLE);               //软件触发ADC转换

}

uint16_t MQ5_ADC_Read(void)
{
	// 读取ADC0的通道1转换结果

//	u16  pa1_value = ((HT_ADC->DR[0] & 0x0FFF));
        ADC_SoftwareStartConvCmd(HT_ADC, ENABLE);               //软件触发ADC转换

    u16 pa1_value = ADC_GetConversionData(HT_ADC, ADC_REGULAR_DATA1);
	return pa1_value;

}



uint16_t MQ5_GetData(void)
{
	

	uint32_t  tempData = 0;
	for (uint8_t i = 0; i < MQ5_READ_TIMES; i++)
	{
		tempData += MQ5_ADC_Read();
		delay_ms(5);
	}

	tempData /= MQ5_READ_TIMES;
	return tempData;
}


uint16_t MQ5_GetData_PPM(void)
{

	uint16_t  tempData = 0;
	

	for (uint8_t i = 0; i < MQ5_READ_TIMES; i++)
	{
		tempData += MQ5_ADC_Read();
		delay_ms(5);
	}
	tempData /= MQ5_READ_TIMES;
	

	
uint16_t ppm =(tempData/4096.0*3.3)*210+ 10;
																		//计算出PPM的值
	return  ppm; 



}

