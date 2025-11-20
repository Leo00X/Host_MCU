#include "Task_Scheduler.h"
#include "app_globals.h"
#include "bftm.h"
#include "usart.h"		//usart1串口屏  usart0 ESP8266
#include "uart.h"			//uart0   ASR-PRO语音模块
#include "bsp_uart.h"	//UART1,DL-20的zigbee联系从机
#include "protocol_dl20.h"
#include "led.h"			
#include "delay.h"
#include "dht11.h"		//温湿度
#include "mq2.h"			//烟雾
#include "hmi_serial.h"	//串口屏
#include "mq5.h"			//煤气
#include "bsp_uart.h"
#include "beep.h"
#include "esp8266.h"
#include "onenet.h"
#include "string.h"
#define ESP8266_ONENET_INFO		"AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"
//extern volatile bool g_command_ready;
//extern u8 rbfData[20];

//主机环境监测参数
u8 temperature;  	            //  主机温度
u8 humidity;          //  主机温度
int host_temp = 29;        //  主机温度
int host_humi = 21;        //  主机湿度
int host_smog = 1;        //  主机烟雾
int host_co = 12;        //  主机CO
int HCHO;				//主机甲醛参数
uint8_t temp_int, humi_int;
//作废参数
int n4 = 29; 
int n1 = 21;  
int n2 = 1;     
int n3 = 12; 

/* --- 用于存储从各个Zigbee从机接收到的传感器数据的全局变量 --- */
// 监测点 A (slave001 - ID 0x01)
int slave1_temp;
int slave1_humi;
int slave1_smog1 = 10;
int slave1_co1 = 10;
int slave1_aqi1 = 10;
int slave1_beep;
int slave1_breach_count;
// **新增：用于存储从机1发送过来的阈值**
int slave1_co_upper_threshold = 50;
int slave1_smog_upper_threshold = 50;
int slave1_temp_lower_threshold = 5;
int slave1_temp_upper_threshold = 40;
int slave1_humi_lower_threshold = 10;
int slave1_humi_upper_threshold = 90;


// 监测点 B (slave002 - ID 0x02)
int slave2_temp;
int slave2_humi;
//int slave2_smog;
int slave2_smog = 10;
int slave2_smog2 = 10;
int slave2_co2;
int slave2_aqi2;
int slave2_beep;
int slave2_breach_count;
// **新增：用于存储从机2发送过来的阈值**
int slave2_co_upper_threshold = 50;
int slave2_smog_upper_threshold = 50;
int slave2_temp_lower_threshold = 5;
int slave2_temp_upper_threshold = 40;
int slave2_humi_lower_threshold = 10;
int slave2_humi_upper_threshold = 90;


// 监测点 C (slave003 - ID 0x03)
int slave3_temp;
int slave3_humi;
int slave3_smog3;
int slave3_co3;
int slave3_aqi3;
int slave3_beep;
int slave3_breach_count;
// **新增：用于存储从机3发送过来的阈值**
int slave3_co_upper_threshold = 50;
int slave3_smog_upper_threshold = 50;
int slave3_temp_lower_threshold = 5;
int slave3_temp_upper_threshold = 40;
int slave3_humi_lower_threshold = 10;
int slave3_humi_upper_threshold = 90;
/* --- 全局变量结束 --- */

void Hardware_Init(void)
{
	
	USART1_Configuration();						//USART1,串口屏
	USART0_Configuration();						//USART0，驱动ESP8266用
	UART0_Configuration();						//UART0，ASRPRO语音模块用
//	BSP_UART_Init();								//UART1,DL-20的zigbee联系从机
	E72_Init();										// 初始化 E72 驱动 (它会自动初始化 bsp_uart.c)
	Protocol_Init();
	MQ2_Init();
	MQ5_Init();
   HMI_Init();	
	Led_Init();;        //LED初始化
	DHT11_Init();   //温度初始化
	beep_init();
	UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
}
 int main(void)
 {	

	unsigned char *dataPtr = NULL;

	Hardware_Init();//硬件初始化
	Scheduler_Init();
	bftm_Init(1000);
	APP_GLOBALS_Init();
	 
	UsartPrintf(USART_DEBUG, "Connect wifi Server...\r\n");
	ESP8266_Init();					//初始化ESP8266

	UsartPrintf(USART_DEBUG, "Connect MQTTs Server...\r\n");

	while(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))
		delay_ms(500);
	UsartPrintf(USART_DEBUG, "Connect MQTT Server Success\r\n");

	while(OneNet_DevLink())			//接入OneNET
	{
		ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT");
		delay_ms(500);
	}
		
	OneNET_Subscribe();
	UsartPrintf(USART_DEBUG, "main Success\r\n");
    
	while(1) 
	{		
		Scheduler_Run();
		
	}				
     
}





