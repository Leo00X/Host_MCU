#include "dht11.h"
#include "delay.h"
#include "ht32.h"
#include "math.h"
#include "usart.h"

//IO方向设置
#define DHT11_IO_IN()  {GPIO_DirectionConfig(HT_GPIOC, GPIO_PIN_0, GPIO_DIR_IN);GPIO_InputConfig(HT_GPIOC, GPIO_PIN_0,ENABLE);}
#define DHT11_IO_OUT() {GPIO_DirectionConfig(HT_GPIOC, GPIO_PIN_0, GPIO_DIR_OUT);GPIO_InputConfig(HT_GPIOC, GPIO_PIN_0,DISABLE);}

#define	DHT11_DQ_IN   GPIO_ReadInBit(HT_GPIOC,GPIO_PIN_0)
      

void DHT11_DQ_OUT(u8 x)
{
	if(x)GPIO_SetOutBits(HT_GPIOC,GPIO_PIN_0);
	else GPIO_ClearOutBits(HT_GPIOC,GPIO_PIN_0);
}

//复位DHT11
void DHT11_Rst(void)	   
{                 
	DHT11_IO_OUT(); 	//SET OUTPUT
    DHT11_DQ_OUT(0); 	//拉低DQ
    delay_ms(20);    	//拉低至少18ms
    DHT11_DQ_OUT(1); 	//DQ=1 
	delay_us(30);     	//主机拉高20~40us
}
//等待DHT11的回应
//返回1:未检测到DHT11的存在
//返回0:存在
u8 DHT11_Check(void) 	   
{   
	UsartPrintf(USART_DEBUG, "DHT11_Check(void)\r\n");
	u8 retry=0;
	DHT11_IO_IN();//SET INPUT	 
    while (DHT11_DQ_IN&&retry<100)//DHT11会拉低40~80us
	{
		retry++;
		delay_us(1);
	};	 
	if(retry>=100)return 1;
	else retry=0;
    while (!DHT11_DQ_IN&&retry<100)//DHT11拉低后会再次拉高40~80us
	{
		retry++;
		delay_us(1);
	};
	if(retry>=100)return 1;	    
	return 0;
}
//从DHT11读取一个位
//返回值：1/0
u8 DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
	while(DHT11_DQ_IN&&retry<100)//等待变为低电平
	{
		retry++;
		delay_us(1);
	}
	retry=0;
	while(!DHT11_DQ_IN&&retry<100)//等待变高电平
	{
		retry++;
		delay_us(1);
	}
	delay_us(40);//等待40us
	if(DHT11_DQ_IN)return 1;
	else return 0;		   
}
//从DHT11读取一个字节
//返回值：读到的数据
u8 DHT11_Read_Byte(void)    
{        
    u8 i,dat;
    dat=0;
	for (i=0;i<8;i++) 
	{
   		dat<<=1; 
	    dat|=DHT11_Read_Bit();
    }						    
    return dat;
}
//从DHT11读取一次数据
//temp:温度值(范围:0~50°)
//humi:湿度值(范围:20%~90%)
//返回值：0,正常;1,读取失败
u8 DHT11_Read_Data(u8 *temp,u8 *humi)    
{        
	UsartPrintf(USART_DEBUG, "DHT11_Read_Data\r\n");
 	u8 buf[5];
	u8 i;
	DHT11_Rst();
	if(DHT11_Check()==0)
	{
		for(i=0;i<5;i++)//读取40位数据
		{
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
		{
			*humi=buf[0];
			*temp=buf[2];
		}
	}else return 1;
	return 0;	    
}
//初始化DHT11的IO口 DQ 同时检测DHT11的存在
//返回1:不存在
//返回0:存在    	 
u8 DHT11_Init(void)
{	 
	//使能PC端口的时钟
	CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
	CKCUClock.Bit.PC         = 1;
//	CKCUClock.Bit.PB         = 1;
	CKCUClock.Bit.AFIO       = 1;
	CKCU_PeripClockConfig(CKCUClock, ENABLE);

	
//	GPIO_DirectionConfig(HT_GPIOB, GPIO_PIN_15, GPIO_DIR_OUT);
//	GPIO_ClearOutBits(HT_GPIOB, GPIO_PIN_15);
	//配置端口功能为GPIO
	AFIO_GPxConfig(GPIO_PC, GPIO_PIN_0, AFIO_FUN_GPIO);

	//配置IO口为输入模式                                                     
	GPIO_DirectionConfig(HT_GPIOC, GPIO_PIN_0, GPIO_DIR_IN);

	//使能输入
	GPIO_InputConfig(HT_GPIOC, GPIO_PIN_0,ENABLE);
			    
	DHT11_Rst();  //复位DHT11
	return DHT11_Check();//等待DHT11的回应
} 







