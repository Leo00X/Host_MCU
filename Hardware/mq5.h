#ifndef _MQ5_H
#define	_MQ5_H

#include "ht32.h"
#include "delay.h"
#include "math.h"

#define MQ5_READ_TIMES	10  //MQ-2传感器ADC循环读取次数
uint16_t MQ5_ADC_Read(void);

uint16_t MQ5_GetData_PPM(void);
void MQ5_Init(void);

#endif

