/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	esp8266.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.0
	*
	*	说明： 		ESP8266的简单驱动
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
//#include "stm32f10x.h"
#include "HT32.h"
//网络设备驱动
#include "esp8266.h"

//硬件驱动
#include "delay.h"
#include "usart.h"

//C库
#include <string.h>
#include <stdio.h>


#define ESP8266_WIFI_INFO		"AT+CWJAP=\"IoT-605\",\"A605A605\"\r\n"
//#define ESP8266_WIFI_INFO		"AT+CWJAP=\"LiangXing\",\"8265398.\"\r\n"
//#define ESP8266_WIFI_INFO		"AT+CWJAP=\"A301\",\"301301301\"\r\n"
//#define ESP8266_WIFI_INFO		"AT+CWJAP=\"Xiaomi 14 Ultra\",\"704554363\"\r\n"
//#define ESP8266_WIFI_INFO		"AT+CWJAP=\"sheng\",\"15724174685\"\r\n"
//#define ESP8266_WIFI_INFO		"AT+CWJAP=\"Mi\",\"12345678\"\r\n"

unsigned char esp8266_buf[1024];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;


//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		esp8266_cnt = 0;							//清0接收计数
			
		return REV_OK;								//返回接收完成标志
	}
		
	esp8266_cntPre = esp8266_cnt;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 200;

	Usart_SendString(HT_USART0, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear();									//清空缓存
				
				return 0;
			}
		}
		
		delay_ms(10);
	}
	
	return 1;

}

//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear();								//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		Usart_SendString(HT_USART0, data, len);		//发送设备连接请求数据
	}

}

//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//搜索“IPD”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
//				UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
//				UsartPrintf(USART_DEBUG, "\"IPD\" ok\r\n");
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		
		delay_ms(5);													//延时等待
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针

}

//==========================================================
//	函数名称：	ESP8266_Init
//
//	函数功能：	初始化ESP8266 Wi-Fi 模块
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		通过发送一系列 AT 指令来配置 ESP8266 模块，
//              并在 OLED 屏幕上显示当前步骤状态。
//              每个步骤会重试直到成功接收到预期响应。
//==========================================================
void ESP8266_Init(void)
{
	// 调用自定义函数 ESP8266_Clear()，可能用于清空用于接收 ESP8266 数据的串口缓冲区，
	// 确保后续接收命令响应时不受旧数据干扰。
	ESP8266_Clear();

	// 下面这行被注释掉了，原来可能是用于通过调试串口打印状态信息
//	UsartPrintf(USART_DEBUG, "1. AT\r\n");

	// 清除 OLED 屏幕内容
//	OLED_Clear();
	// 在 OLED 的 (0,0) 位置以字号 8 显示 "1.AT..."，提示用户正在执行第一步：发送 AT 测试指令
//	OLED_ShowString(0,0,"1.AT...",8);
	// 循环发送 "AT\r\n" 指令给 ESP8266，并检查响应中是否包含 "OK"
	// ESP8266_SendCmd 函数可能在成功（收到"OK"）时返回 0，失败或超时时返回非 0
	// 如果失败 (返回值非0)，则延时 500ms 后重试，直到成功为止
	while(ESP8266_SendCmd("AT\r\n", "OK"))
		delay_ms(500); // 使用 delay_ms 进行延时

	// 下面这行被注释掉了，原来可能是用于通过调试串口打印状态信息
//	UsartPrintf(USART_DEBUG, "2. CWMODE\r\n");

	// 在 OLED 的第 3 行 (0,2) 以字号 8 显示 "2.CWMODE..."，提示正在设置 Wi-Fi 模式
//	OLED_ShowString(0,2,"2.CWMODE...",8);
	// 循环发送 "AT+CWMODE=1\r\n" 指令，将 ESP8266 设置为 Station 模式 (客户端模式)
	// 等待 "OK" 响应，失败则延时 500ms 重试
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
		delay_ms(500);

	// 下面这行被注释掉了，原来可能是用于通过调试串口打印状态信息
//	UsartPrintf(USART_DEBUG, "3. AT+CWDHCP\r\n");

	// 在 OLED 的第 5 行 (0,4) 以字号 8 显示 "3.AT+CWDHCP..."，提示正在配置 DHCP
//	OLED_ShowString(0,4,"3.AT+CWDHCP...",8);
	// 循环发送 "AT+CWDHCP=1,1\r\n" 指令，启用 Station 模式和 SoftAP 模式的 DHCP 客户端功能
	// (虽然这里只用了 Station 模式，但指令同时设置了两者)
	// 等待 "OK" 响应，失败则延时 500ms 重试
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
		delay_ms(500);

	// 下面这行被注释掉了，原来可能是用于通过调试串口打印状态信息
//	UsartPrintf(USART_DEBUG, "4. CWJAP\r\n");

	// 在 OLED 的第 7 行 (0,6) 以字号 8 显示 "4.CWJAP..."，提示正在连接 Wi-Fi
//	OLED_ShowString(0,6,"4.CWJAP...",8);
	// 循环发送连接 Wi-Fi AP 的指令。ESP8266_WIFI_INFO 可能是一个宏或字符串，包含类似 'AT+CWJAP="ssid","password"\r\n' 的内容
	// 等待响应中包含 "GOT IP"，表示成功连接 Wi-Fi 并获取到 IP 地址
	// 失败则延时 500ms 重试
	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP"))
		delay_ms(500);

	// 通过调试串口 (USART_DEBUG) 打印最终状态信息，表示 ESP8266 初始化和配置完成
	UsartPrintf(USART_DEBUG, "5. ESP8266 Init OK\r\n");

//	// 清除 OLED 屏幕内容
//	OLED_Clear();
//	// 在 OLED 的 (0,0) 位置以字号 16 显示最终成功信息 "ESP8266 Init OK"
//	OLED_ShowString(0,0,"ESP8266 Init OK",16);
	// 短暂延时 500ms，让用户能看到屏幕上的成功信息
	delay_ms(500);

} // 函数结束

//==========================================================
//	函数名称：	USART0_IRQHandler
//
//	函数功能：	串口2收发中断
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void USART0_IRQHandler(void)
{

	if(USART_GetFlagStatus(COM0_PORT, USART_FLAG_RXDR) != RESET) //接收中断
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //防止串口被刷爆
		esp8266_buf[esp8266_cnt++] = COM0_PORT->DR;
		
		USART_ClearFlag(COM0_PORT, USART_FLAG_RSADD);
	}

}
