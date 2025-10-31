#ifndef _MQ2_H
#define	_MQ2_H

#include "ht32.h"
#include "delay.h"
#include "math.h"

#define MQ2_READ_TIMES	10  //MQ-2传感器ADC循环读取次数
uint16_t MQ2_ADC_Read(void);

float MQ2_GetData_PPM(void);
void MQ2_Init(void);

#endif


