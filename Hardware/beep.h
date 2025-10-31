#ifndef __BEEP_H
#define __BEEP_H 			   
#include "ht32.h"
#include "delay.h"

void beep_init(void);
void beep(u8 a);
void beep_on(void);
void beep_off(void);
void play_alarm_pattern_blocking(const unsigned int* pattern_ms, int repetitions);
void process_dynamic_alarms();
void beep_key(void);


#endif

