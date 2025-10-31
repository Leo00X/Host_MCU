#ifndef __DHT11_H
#define __DHT11_H 
#include "ht32.h"   


u8 DHT11_Init(void);
// [专家修改] 更改函数声明以包含小数部分
// u8 DHT11_Read_Data(u8 *temp,u8 *humi); // <-- 旧的声明
u8 DHT11_Read_Data(u8 *temp_int, u8 *temp_dec, u8 *humi_int, u8 *humi_dec); // <-- 新的声明

u8 DHT11_Read_Byte(void);
u8 DHT11_Read_Bit(void);
u8 DHT11_Check(void);
void DHT11_Rst(void);    

#endif
