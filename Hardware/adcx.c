#include "adcx.h"



void ADC_init(void)
{
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};//定义结构体，配置时钟
    CKCUClock.Bit.PA = 1;                       //开启GPIOA时钟
    CKCUClock.Bit.ADC = 1;                      //开启ADC时钟
    CKCUClock.Bit.AFIO = 1;                     //开启AFIO时钟
    CKCUClock.Bit.PDMA = 1;                     //开启PDMA时钟
    CKCU_PeripClockConfig(CKCUClock, ENABLE);   //使能外设时钟

    ADC_Reset(HT_ADC);                          //重置ADC
    CKCU_SetADCPrescaler(CKCU_ADCPRE_DIV4);     //设置ADC时钟分频
   
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_1, AFIO_FUN_ADC); 

    ADC_RegularChannelConfig(HT_ADC, ADC_CH_1, 0);      //配置ADC规则组，在规则组的序列1写入通道1

    ADC_RegularGroupConfig(HT_ADC, CONTINUOUS_MODE, 1, 1);  //配置ADC转换模式为连续转换，队列长度为1
    ADC_RegularTrigConfig(HT_ADC, ADC_TRIG_SOFTWARE);       //配置ADC触发源软件触发
    ADC_SamplingTimeConfig(HT_ADC, 16);                     //配置ADC输入通道采样时间

    ADC_Cmd(HT_ADC, ENABLE);                                //使能ADC
    ADC_SoftwareStartConvCmd(HT_ADC, ENABLE);               //软件触发ADC转换

}
