#ifndef _PWM_H_
#define _PWM_H_

#include "stm32f10x.h"                  // Device header

void PWM_Init(void);                          // 初始化 TIM2 PWM（PA0=LED1，PA1=LED2）
void PWM_SetLED1_Brightness(uint8_t percent); // 设置 LED1 亮度，0~100，0=灭 100=最亮
void PWM_SetLED2_Brightness(uint8_t percent); // 设置 LED2 亮度，0~100

#endif
