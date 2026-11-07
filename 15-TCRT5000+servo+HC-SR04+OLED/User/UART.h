#ifndef _UART_H_
#define _UART_H_

#include "stm32f10x.h"                  // Device header

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern char Serial_RxPacket[];//可以在main函数中调用了
extern uint8_t Serial_RxFlag;

void uart_init(void);

void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array,uint16_t Length);
void Serial_SendString(char*String);
void Serial_Printf(char *format, ...);

#endif
