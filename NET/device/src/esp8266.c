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

#include "uart.h"//2025年11月3日 调试
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
//	函数功能：	等待接收完成 (HT32 专家已修改)
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
	{
		esp8266_cntPre = 0; // 确保 pre 也为0
		return REV_WAIT;
	}
		
	if(esp8266_cnt == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
        // --- HT32 专家修改 ---
		// *** 移除了这里的 esp8266_cnt = 0; ***
        // 清除计数器的操作必须由数据处理函数在临界区内完成
        // 否则会导致数据还未被读取，缓冲区就被覆盖的竞态条件
        
		return REV_OK;								//返回接收完成标志
	}
		
	esp8266_cntPre = esp8266_cnt;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令 (HT32 专家已修改)
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		已增加临界区保护，解决竞态条件
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 200;

    // --- HT32 专家新增：本地缓冲区，用于安全处理数据 ---
    // 使用 static 避免堆栈溢出，在单线程环境中安全
    static char local_rx_buffer[sizeof(esp8266_buf)];
    u16 local_rx_len = 0;
    
    // --- HT32 专家修改：在发送命令前，先在临界区内清空缓冲区 ---
    NVIC_DisableIRQ(USART0_IRQn);
    ESP8266_Clear();
    esp8266_cntPre = 0; // 确保 WaitRecive 的状态也被重置
    NVIC_EnableIRQ(USART0_IRQn);

	Usart_SendString(HT_USART0, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
            local_rx_len = 0;
            
            //==============================================================
            // 1. 进入临界区 (只关闭 USART0 中断)
            //==============================================================
            NVIC_DisableIRQ(USART0_IRQn); // *** HT32 专家方案 ***

            //==============================================================
            // 2. 快速拷贝数据并清空全局缓冲区
            //==============================================================
            local_rx_len = esp8266_cnt;
            if (local_rx_len > 0)
            {
                // 确保拷贝长度不超过本地缓冲区大小
                if (local_rx_len >= sizeof(local_rx_buffer))
                {
                    local_rx_len = sizeof(local_rx_buffer) - 1;
                }
                memcpy(local_rx_buffer, esp8266_buf, local_rx_len);
            }
            
            // 在临界区内清空全局缓冲区和计数器
            ESP8266_Clear();
            esp8266_cntPre = 0; // 重置 WaitRecive 的前一个值
            
            //==============================================================
            // 3. 退出临界区 (立刻重新打开 USART0 中断)
            //==============================================================
            NVIC_EnableIRQ(USART0_IRQn);
            
            //==============================================================
            // 4. 在临界区之外，安全地处理本地副本
            //==============================================================
			if(local_rx_len > 0)
			{
                local_rx_buffer[local_rx_len] = '\0'; // 确保字符串结束

				if(strstr((const char *)local_rx_buffer, res) != NULL)		//如果检索到关键词
				{
					// 缓冲区已在临界区内清空
					return 0; // 成功
				}
                // 如果没检索到，会继续循环等待下一次 WaitRecive (如果超时未到)
			}
		}
		
		delay_ms(10);
	}
	
    // --- HT32 专家修改：超时后也要清空缓冲区，为下次做准备 ---
    NVIC_DisableIRQ(USART0_IRQn);
    ESP8266_Clear();
    esp8266_cntPre = 0;
    NVIC_EnableIRQ(USART0_IRQn);

	return 1; // 失败
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
//	说明：		(此函数依赖 ESP8266_SendCmd, 已自动获得安全保护)
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
    // 注意：ESP8266_SendCmd 内部已经包含了清空缓冲区的逻辑
    // 所以这里不需要再调用 ESP8266_Clear()
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		Usart_SendString(HT_USART0, data, len);		//发送设备连接请求数据
	}

}

//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据 (HT32 专家已修改)
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据 (指针指向静态本地缓冲区)
//
//	说明：		已增加临界区保护，解决竞态条件。
//				返回的指针指向一个*静态*缓冲区，数据在下次调用时会被覆盖。
//==========================================================

// --- HT32 专家新增：一个静态本地缓冲区，用于安全地处理和返回数据 ---
// 警告: 这使得该函数不可重入，但对于单线程轮询是标准做法
static char local_ipd_buffer[sizeof(esp8266_buf)];

unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
    u16 local_rx_len = 0;
    
    // --- HT32 专家修改：在函数开始时不清除本地缓冲区，
    // 因为如果超时，调用者可能还想访问上次的数据（尽管这不好）
    // 我们只在 *成功复制* 数据时才覆盖它。
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
		{
            local_rx_len = 0;

            //==============================================================
            // 1. 进入临界区 (只关闭 USART0 中断)
            //==============================================================
            NVIC_DisableIRQ(USART0_IRQn); // *** HT32 专家方案 ***

            //==============================================================
            // 2. 快速拷贝数据并清空全局缓冲区
            //==============================================================
            local_rx_len = esp8266_cnt;
            if (local_rx_len > 0)
            {
                // 确保拷贝长度不超过本地缓冲区大小
                if (local_rx_len >= sizeof(local_ipd_buffer))
                {
                    local_rx_len = sizeof(local_ipd_buffer) - 1;
                }
                memcpy(local_ipd_buffer, esp8266_buf, local_rx_len);
            }
            
            // 在临界区内清空全局缓冲区和计数器
            ESP8266_Clear();
            esp8266_cntPre = 0; // 重置 WaitRecive 的前一个值
            
            //==============================================================
            // 3. 退出临界区 (立刻重新打开 USART0 中断)
            //==============================================================
            NVIC_EnableIRQ(USART0_IRQn);

            //==============================================================
            // 4. 在临界区之外，安全地处理本地副本
            //==============================================================
			if(local_rx_len > 0)
			{
                local_ipd_buffer[local_rx_len] = '\0'; // 确保字符串结束

				ptrIPD = strstr((char *)local_ipd_buffer, "IPD,");				//搜索“IPD”头
				if(ptrIPD == NULL)											//如果没找到
				{
					uprintf("\"IPD\" not found\r\n");
                    // 数据包不是IPD，但已被消耗，继续循环等待下一次(如果超时未到)
				}
				else
				{
					ptrIPD = strchr(ptrIPD, ':');							//找到':'
					if(ptrIPD != NULL)
					{
						ptrIPD++;
						uprintf("\"IPD\" ok\r\n");
                        // 成功！ptrIPD 指向 local_ipd_buffer 内部
						return (unsigned char *)(ptrIPD);
					}
                    // 找到了 "IPD," 但没找到 ":"，格式错误，继续循环
				}
			}
            // 如果 local_rx_len == 0 (不太可能，因为WaitRecive OK了)，
            // 也会继续循环
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
