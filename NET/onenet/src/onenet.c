/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	onenet.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.1
	*
	*	说明： 		与onenet平台的数据交互接口层
	*
	*	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
	*				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
//#include "stm32f10x.h"
#include "HT32.h"
//网络设备
#include "esp8266.h"

//协议文件
#include "onenet.h"
#include "mqttkit.h"

//算法
#include "base64.h"
#include "hmac_sha1.h"

//硬件驱动
#include "usart.h"
#include "delay.h"
//#include "beep.h"
//#include "fan.h"
#include "beep.h"

#include "app_globals.h"

//C库
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>        // (V8) 用于 isnan(), isinf()
#include "cJSON.h"

#define PROID			"sMFCi53wHH"
#define ACCESS_KEY		"b1NNSFA4d2d1QWtEVUxoQVlwVVVWZXNCcFVtM1JMY1E="
#define DEVICE_NAME		"air_01"

//#define PROID			"6Tn8NKV9sN"
//#define ACCESS_KEY		"OTd3cjJrSTdhdHZPY1VqNzFYZW1IMmtpY21MODlkZFc="
//#define DEVICE_NAME		"ht32"


char devid[16];

char key[48];


extern unsigned char esp8266_buf[1024];
extern uint32_t g_tick_counter;
static char g_onenet_tx_buf[1600];
/*
************************************************************
*	函数名称：	OTA_UrlEncode
*
*	函数功能：	sign需要进行URL编码
*
*	入口参数：	sign：加密结果
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		+			%2B
*				空格		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}

/*
************************************************************
*	函数名称：	OTA_Authorization
*
*	函数功能：	计算Authorization
*
*	入口参数：	ver：参数组版本号，日期格式，目前仅支持格式"2018-10-31"
*				res：产品id
*				et：过期时间，UTC秒值
*				access_key：访问密钥
*				dev_name：设备名
*				authorization_buf：缓存token的指针
*				authorization_buf_len：缓存区长度(字节)
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		当前仅支持sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//保存签名的Base64编码结果 和 URL编码结果
	char hmac_sha1_buf[64];							//保存签名
	char access_key_base64[64];						//保存access_key的Base64编码结合
	char string_for_signature[72];					//保存string_for_signature，这个是加密的key

//----------------------------------------------------参数合法性--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------将access_key进行Base64解码----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
//	UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------计算string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
//	UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------加密-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	

	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
//	UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------将加密结果进行Base64编码------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------将Base64编码结果进行URL编码---------------------------------------------------
	OTA_UrlEncode(sign_buf);
//	UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------计算Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
//	UsartPrintf(USART_DEBUG, "Token: %s\r\n", authorization_buf);
	
	return 0;

}

//==========================================================
//	函数名称：	OneNET_RegisterDevice
//
//	函数功能：	在产品中注册一个设备
//
//	入口参数：	access_key：访问密钥
//				pro_id：产品ID
//				serial：唯一设备号
//				devid：保存返回的devid
//				key：保存返回的key
//
//	返回参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
_Bool OneNET_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//加密的key
	
	send_ptr = malloc(send_len + 240);
	if(send_ptr == NULL)
		return result;
	
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n", "CONNECT"))
		delay_ms(500);
	
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, NULL,
							authorization_buf, sizeof(authorization_buf), 1);
	
	snprintf(send_ptr, 240 + send_len, "POST /mqtt/v1/devices/reg HTTP/1.1\r\n"
					"Authorization:%s\r\n"
					"Host:ota.heclouds.com\r\n"
					"Content-Type:application/json\r\n"
					"Content-Length:%d\r\n\r\n"
					"{\"name\":\"%s\"}",
	
					authorization_buf, 11 + strlen(DEVICE_NAME), DEVICE_NAME);
	
	ESP8266_SendData((unsigned char *)send_ptr, strlen(send_ptr));
	
	/*
	{
	  "request_id" : "f55a5a37-36e4-43a6-905c-cc8f958437b0",
	  "code" : "onenet_common_success",
	  "code_no" : "000000",
	  "message" : null,
	  "data" : {
		"device_id" : "589804481",
		"name" : "mcu_id_43057127",
		
	"pid" : 282932,
		"key" : "indu/peTFlsgQGL060Gp7GhJOn9DnuRecadrybv9/XY="
	  }
	}
	*/
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//等待平台响应
	
	if(data_ptr)
	{
		data_ptr = strstr(data_ptr, "device_id");
	}
	
	if(data_ptr)
	{
		char name[16];
		int pid = 0;
		
		if(sscanf(data_ptr, "device_id\" : \"%[^\"]\",\r\n\"name\" : \"%[^\"]\",\r\n\r\n\"pid\" : %d,\r\n\"key\" : \"%[^\"]\"", devid, name, &pid, key) == 4)
		{
			UsartPrintf(USART_DEBUG, "create device: %s, %s, %d, %s\r\n", devid, name, pid, key);
			result = 0;
		}
	}
	
	free(send_ptr);
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");
	
	return result;

}

//==========================================================
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

	unsigned char *dataPtr;
	
	char authorization_buf[160];
	
	_Bool status = 1;

	OneNET_Authorization("2018-10-31", PROID, 1777207163, ACCESS_KEY, DEVICE_NAME,
								authorization_buf, sizeof(authorization_buf), 0);
	
//	UsartPrintf(USART_DEBUG, "OneNET_DevLink\r\n"
//							"NAME: %s,	PROID: %s,	KEY:%s\r\n"
//                        , DEVICE_NAME, PROID, authorization_buf);
	
	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 1000, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//上传平台
		
		dataPtr = ESP8266_GetIPD(250);									//等待平台响应
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:UsartPrintf(USART_DEBUG, "Tips:	连接成功\r\n");status = 0;break;
					
					case 1:UsartPrintf(USART_DEBUG, "WARN:	连接失败：协议错误\r\n");break;
					case 2:UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法的clientid\r\n");break;
					case 3:UsartPrintf(USART_DEBUG, "WARN:	连接失败：服务器失败\r\n");break;
					case 4:UsartPrintf(USART_DEBUG, "WARN:	连接失败：用户名或密码错误\r\n");break;
					case 5:UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法链接(比如token非法)\r\n");break;
					
					default:UsartPrintf(USART_DEBUG, "ERR:	连接失败：未知错误\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//删包
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	
	return status;
	
}

extern int n1,n2,n3,n4,Fan_Status,k210_Status,cur_Status,hwflag; 
// extern 声明从各个Zigbee从机接收到的传感器数据
// 监测点 A (slave001)
extern int slave1_temp;
extern int slave1_humi;
extern float slave1_smog1;
extern float slave1_co1;
extern float slave1_aqi1;
extern int slave1_beep;
extern int slave1_breach_count;

// 监测点 B (slave002)
extern int slave2_temp;
extern int slave2_humi;
extern float slave2_smog2;
extern float slave2_co2;
extern float slave2_aqi2;
extern int slave2_beep;
extern int slave2_breach_count;

// 监测点 C (slave003)
extern int slave3_temp;
extern int slave3_humi;
extern float slave3_smog3;
extern float slave3_co3;
extern float slave3_aqi3;
extern int slave3_beep;
extern int slave3_breach_count;
// **新增：用于存储从机1发送过来的阈值**
extern int slave1_co_upper_threshold;
extern int slave1_smog_upper_threshold;
extern int slave1_temp_lower_threshold;
extern int slave1_temp_upper_threshold;
extern int slave1_humi_lower_threshold;
extern int slave1_humi_upper_threshold;
// **新增：用于存储从机2发送过来的阈值**
extern int slave2_co_upper_threshold;
extern int slave2_smog_upper_threshold;
extern int slave2_temp_lower_threshold;
extern int slave2_temp_upper_threshold;
extern int slave2_humi_lower_threshold;
extern int slave2_humi_upper_threshold;

extern int slave3_co_upper_threshold;
extern int slave3_smog_upper_threshold;
extern int slave3_temp_lower_threshold;
extern int slave3_temp_upper_threshold;
extern int slave3_humi_lower_threshold;
extern int slave3_humi_upper_threshold;
/**
  * @brief  填充发送到OneNet服务器的数据缓冲区。
  * @brief  (安全修复版) 使用 snprintf 防止缓冲区溢出。
  * @param  buf: 指向目标缓冲区的指针，用于存储格式化后的JSON字符串。
  * @retval 填充后JSON字符串的长度。如果缓冲区不足，将返回0。
  * @note   此版本跟踪写入的长度，确保不会超出1600字节的限制。
  */
uint16_t OneNet_FillBuf(char *buf)
{
    // 定义缓冲区的总大小，必须与 OneNet_SendData 中的 buf 大小一致！
    const size_t buffer_size = 1600;
    
    // 'p' 是一个指向当前写入位置的指针
    char *p = buf;
    
    // 'remaining' 是缓冲区的剩余空间
    size_t remaining = buffer_size;
    
    // 'written' 用于接收 snprintf 的返回值（实际写入的字符数）
    int written = 0;
    int i;

    // (V9-Mod) 内部自增 ID
    static unsigned long s_internal_message_id = 0;
    s_internal_message_id++;

    // 1. 初始化主JSON结构
    written = snprintf(p, remaining, "{\"id\":\"%lu\",\"version\":\"1.0\",\"params\":{", s_internal_message_id);
    // 检查写入是否成功或是否会溢出
    if (written < 0 || written >= remaining) return 0;
    p += written;
    remaining -= written;

    // 2. 添加主机本地传感器数据
    const char *json_float_template = "\"%s\":{\"value\":%.1f},";
    const char *json_int_template = "\"%s\":{\"value\":%d},";

    // --- 主机数据 ---
    written = snprintf(p, remaining, json_float_template, "host_temperature", g_system_data.host.env_data.temperature);
    if (written < 0 || written >= remaining) return 0;
    p += written; remaining -= written;

    written = snprintf(p, remaining, json_float_template, "host_humidity", g_system_data.host.env_data.humidity);
    if (written < 0 || written >= remaining) return 0;
    p += written; remaining -= written;

    written = snprintf(p, remaining, json_float_template, "host_smog_ppm", g_system_data.host.env_data.smog_ppm);
    if (written < 0 || written >= remaining) return 0;
    p += written; remaining -= written;

    written = snprintf(p, remaining, json_float_template, "host_gas_ppm", g_system_data.host.env_data.gas_ppm);
    if (written < 0 || written >= remaining) return 0;
    p += written; remaining -= written;
    
    written = snprintf(p, remaining, json_float_template, "host_pm25_value", g_system_data.host.env_data.pm25_value);
    if (written < 0 || written >= remaining) return 0;
    p += written; remaining -= written;

    written = snprintf(p, remaining, json_float_template, "host_formaldehyde_ppb", g_system_data.host.env_data.formaldehyde_ppb);
    if (written < 0 || written >= remaining) return 0;
    p += written; remaining -= written;

    written = snprintf(p, remaining, json_int_template, "global_fan_mode", g_system_data.hmi_mode_request.mode_request);
    if (written < 0 || written >= remaining) return 0;
    p += written; remaining -= written;

    // 3. 循环添加所有在线从机的数据
    for (i = 0; i < MAX_SLAVES; i++)
    {
//        if (!g_system_data.slaves[i].info.is_online)
//        {
//            continue;
//        }

        char identifier[32]; // 临时存储标识符，如 "slave1_temperature"

        // --- 从机温度 ---
        sprintf(identifier, "slave%d_temperature", i + 1);
        written = snprintf(p, remaining, json_float_template, identifier, g_system_data.slaves[i].env_data.temperature);
        if (written < 0 || written >= remaining) return 0;
        p += written; remaining -= written;

        // --- 从机湿度 ---
        sprintf(identifier, "slave%d_humidity", i + 1);
        written = snprintf(p, remaining, json_float_template, identifier, g_system_data.slaves[i].env_data.humidity);
        if (written < 0 || written >= remaining) return 0;
        p += written; remaining -= written;

        // --- 从机CO ---
        sprintf(identifier, "slave%d_co_ppm", i + 1);
        written = snprintf(p, remaining, json_float_template, identifier, g_system_data.slaves[i].env_data.co_ppm);
        if (written < 0 || written >= remaining) return 0;
        p += written; remaining -= written;

        // --- 从机烟雾 ---
        sprintf(identifier, "slave%d_smog_ppm", i + 1);
        written = snprintf(p, remaining, json_float_template, identifier, g_system_data.slaves[i].env_data.smog_ppm);
        if (written < 0 || written >= remaining) return 0;
        p += written; remaining -= written;

        // --- 从机AQI ---
        sprintf(identifier, "slave%d_aqi_ppm", i + 1);
        written = snprintf(p, remaining, json_float_template, identifier, g_system_data.slaves[i].env_data.aqi_ppm);
        if (written < 0 || written >= remaining) return 0;
        p += written; remaining -= written;

        // --- 从机独立风速 ---
        sprintf(identifier, "slave%d_fan_speed", i + 1);
        written = snprintf(p, remaining, json_int_template, identifier, g_system_data.slaves[i].control.target_fan_speed);
        if (written < 0 || written >= remaining) return 0;
        p += written; remaining -= written;
    }

    // 4. 移除最后一个多余的逗号，并结束JSON结构
    size_t current_len = p - buf;
    if (current_len > 0 && buf[current_len - 1] == ',')
    {
        p--; // 指针回退一个字符
        remaining++;
    }
    
    // 闭合 params 和 root 对象
    written = snprintf(p, remaining, "}}");
    if (written < 0 || written >= remaining) return 0;
    
    // 返回最终字符串的实际长度
    return (uint16_t)(p - buf + written);
}

/**
 * @brief 发送数据的主函数 (V7-fix)
 * @note  (Mod-10-fix) 修复 OneNet_FillBuf 调用参数和逻辑判断
 */
void OneNet_SendData(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};

    // 缓冲区大小
    char buf[2600];

    short body_len = 0, i = 0;

    memset(buf, 0, sizeof(buf));

    // ==========================================================
    // (Mod-10) 修正 1: 
    // OneNet_FillBuf 只接受 1 个参数 (char *buf)
    // 并且它返回 uint16_t 的长度
    // ==========================================================
    uint16_t fill_status = OneNet_FillBuf(buf); 

    // ==========================================================
    // (Mod-10) 修正 2: 
    // fill_status > 0 代表成功 (返回的是字符串长度)
    // fill_status == 0 代表填充失败
    // ==========================================================
    if(fill_status > 0) // 大于 0 代表填充成功
    {
        // 填充成功后，fill_status 就是字符串的实际长度
        body_len = fill_status; 
        
        if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)
        {
            for(; i < body_len; i++)
                mqttPacket._data[mqttPacket._len++] = buf[i];
            
            ESP8266_SendData(mqttPacket._data, mqttPacket._len);
            UsartPrintf(USART_DEBUG, "Send OK\r\n");

			  // (安全修复 V2): 不再使用 UsartPrintf 的 %s 打印长字符串，避免其内部缓冲区溢出
				UsartPrintf(USART_DEBUG, "\r\n--- OneNET JSON Payload ---\r\n");

				// 使用循环，手动将 buf 中的每个字符通过串口发送出去
				for (i = 0; buf[i] != '\0'; i++)
				{
					 // !! 注意：请将下面的函数名替换为您项目中实际使用的“发送单个字节”的函数 !!
					 Usart_Sendbyte(USART_DEBUG, buf[i]); 
				}

				UsartPrintf(USART_DEBUG, "\r\n---------------------------------\r\n");
            MQTT_DeleteBuffer(&mqttPacket);
        }
        else
        {
            UsartPrintf(USART_DEBUG, "WARN:    EDP_NewBuffer Failed\r\n");
        }
    }
    else
    {
        // 填充失败 (cJSON 错误或缓冲区太小)
        UsartPrintf(USART_DEBUG, "ERROR: OneNet_FillBuf failed!\r\n");
    }
}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}

//==========================================================
//	函数名称：	OneNET_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	char topic_buf[56];
	const char *topic = topic_buf;
	
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);
	
	//UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}






//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[800];
	int num = 0;
	
	cJSON *raw_json, *params_json;
    cJSON *s1cup_json, *s1sup_json, *s1tlo_json, *s1tup_json, *s1hlo_json, *s1hup_json;
    cJSON *s2cup_json, *s2sup_json, *s2tlo_json, *s2tup_json, *s2hlo_json, *s2hup_json;
    cJSON *s3cup_json, *s3sup_json, *s3tlo_json, *s3tup_json, *s3hlo_json, *s3hup_json;
	
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:																//接收的Publish消息
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				char *data_ptr = NULL;
				
//				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
//																	cmdid_topic, topic_len, req_payload, req_len);
				
				raw_json = cJSON_Parse(req_payload);
				params_json = cJSON_GetObjectItem(raw_json,"params");
				
			// --- 解析从机1的阈值 (Parse Slave 1 Thresholds) ---

			// slave1_co_upper_threshold
			s1cup_json = cJSON_GetObjectItem(params_json, "slave1_co_upper_threshold");
			if (s1cup_json != NULL) {
			  // For robustness, consider adding cJSON_IsNumber(s1cup_json) check here
			  slave1_co_upper_threshold = s1cup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave1_co_upper_threshold = %d\r\n", slave1_co_upper_threshold);
			}

			// slave1_smog_upper_threshold
			s1sup_json = cJSON_GetObjectItem(params_json, "slave1_smog_upper_threshold");
			if (s1sup_json != NULL) {
			  slave1_smog_upper_threshold = s1sup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave1_smog_upper_threshold = %d\r\n", slave1_smog_upper_threshold);
			}

			// slave1_temp_lower_threshold
			s1tlo_json = cJSON_GetObjectItem(params_json, "slave1_temp_lower_threshold");
			if (s1tlo_json != NULL) {
			  slave1_temp_lower_threshold = s1tlo_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave1_temp_lower_threshold = %d\r\n", slave1_temp_lower_threshold);
			}

			// slave1_temp_upper_threshold
			s1tup_json = cJSON_GetObjectItem(params_json, "slave1_temp_upper_threshold");
			if (s1tup_json != NULL) {
			  slave1_temp_upper_threshold = s1tup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave1_temp_upper_threshold = %d\r\n", slave1_temp_upper_threshold);
			}

			// slave1_humi_lower_threshold
			s1hlo_json = cJSON_GetObjectItem(params_json, "slave1_humi_lower_threshold");
			if (s1hlo_json != NULL) {
			  slave1_humi_lower_threshold = s1hlo_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave1_humi_lower_threshold = %d\r\n", slave1_humi_lower_threshold);
			}

			// slave1_humi_upper_threshold
			s1hup_json = cJSON_GetObjectItem(params_json, "slave1_humi_upper_threshold");
			if (s1hup_json != NULL) {
			  slave1_humi_upper_threshold = s1hup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave1_humi_upper_threshold = %d\r\n", slave1_humi_upper_threshold);
			}

			// --- 解析从机2的阈值 (Parse Slave 2 Thresholds) ---

			// slave2_co_upper_threshold
			s2cup_json = cJSON_GetObjectItem(params_json, "slave2_co_upper_threshold");
			if (s2cup_json != NULL) {
			  slave2_co_upper_threshold = s2cup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave2_co_upper_threshold = %d\r\n", slave2_co_upper_threshold);
			}

			// slave2_smog_upper_threshold
			s2sup_json = cJSON_GetObjectItem(params_json, "slave2_smog_upper_threshold");
			if (s2sup_json != NULL) {
			  slave2_smog_upper_threshold = s2sup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave2_smog_upper_threshold = %d\r\n", slave2_smog_upper_threshold);
			}

			// slave2_temp_lower_threshold
			s2tlo_json = cJSON_GetObjectItem(params_json, "slave2_temp_lower_threshold");
			if (s2tlo_json != NULL) {
			  slave2_temp_lower_threshold = s2tlo_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave2_temp_lower_threshold = %d\r\n", slave2_temp_lower_threshold);
			}

			// slave2_temp_upper_threshold
			s2tup_json = cJSON_GetObjectItem(params_json, "slave2_temp_upper_threshold");
			if (s2tup_json != NULL) {
			  slave2_temp_upper_threshold = s2tup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave2_temp_upper_threshold = %d\r\n", slave2_temp_upper_threshold);
			}

			// slave2_humi_lower_threshold
			s2hlo_json = cJSON_GetObjectItem(params_json, "slave2_humi_lower_threshold");
			if (s2hlo_json != NULL) {
			  slave2_humi_lower_threshold = s2hlo_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave2_humi_lower_threshold = %d\r\n", slave2_humi_lower_threshold);
			}

			// slave2_humi_upper_threshold
			s2hup_json = cJSON_GetObjectItem(params_json, "slave2_humi_upper_threshold");
			if (s2hup_json != NULL) {
			  slave2_humi_upper_threshold = s2hup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave2_humi_upper_threshold = %d\r\n", slave2_humi_upper_threshold);
			}

			// --- 解析从机3的阈值 (Parse Slave 3 Thresholds) ---

			// slave3_co_upper_threshold
			s3cup_json = cJSON_GetObjectItem(params_json, "slave3_co_upper_threshold");
			if (s3cup_json != NULL) {
			  slave3_co_upper_threshold = s3cup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave3_co_upper_threshold = %d\r\n", slave3_co_upper_threshold);
			}

			// slave3_smog_upper_threshold
			s3sup_json = cJSON_GetObjectItem(params_json, "slave3_smog_upper_threshold");
			if (s3sup_json != NULL) {
			  slave3_smog_upper_threshold = s3sup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave3_smog_upper_threshold = %d\r\n", slave3_smog_upper_threshold);
			}

			// slave3_temp_lower_threshold
			s3tlo_json = cJSON_GetObjectItem(params_json, "slave3_temp_lower_threshold");
			if (s3tlo_json != NULL) {
			  slave3_temp_lower_threshold = s3tlo_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave3_temp_lower_threshold = %d\r\n", slave3_temp_lower_threshold);
			}

			// slave3_temp_upper_threshold
			s3tup_json = cJSON_GetObjectItem(params_json, "slave3_temp_upper_threshold");
			if (s3tup_json != NULL) {
			  slave3_temp_upper_threshold = s3tup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave3_temp_upper_threshold = %d\r\n", slave3_temp_upper_threshold);
			}

			// slave3_humi_lower_threshold
			s3hlo_json = cJSON_GetObjectItem(params_json, "slave3_humi_lower_threshold");
			if (s3hlo_json != NULL) {
			  slave3_humi_lower_threshold = s3hlo_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave3_humi_lower_threshold = %d\r\n", slave3_humi_lower_threshold);
			}

			// slave3_humi_upper_threshold
			s3hup_json = cJSON_GetObjectItem(params_json, "slave3_humi_upper_threshold");
			if (s3hup_json != NULL) {
			  slave3_humi_upper_threshold = s3hup_json->valuedouble;
			  UsartPrintf(USART_DEBUG, "slave3_humi_upper_threshold = %d\r\n", slave3_humi_upper_threshold);
			}
			
			
			
							cJSON_Delete(raw_json);
				UsartPrintf(USART_DEBUG, "cJSON_Delete(raw_json)  OK");
			}
			
		case MQTT_PKT_PUBACK:														//发送Publish消息，平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
			
		break;
		
		case MQTT_PKT_SUBACK:																//发送Subscribe消息的Ack
		
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
		
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//清空缓存
	
	if(result == -1)
		return;
	
	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}
}
