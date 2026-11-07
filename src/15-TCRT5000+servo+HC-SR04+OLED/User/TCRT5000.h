#ifndef __TCRT5000_H
#define __TCRT5000_H

#include "stm32f10x.h"

/* 传感器数量(改这里 + s_pin[] + s_weight[] 即可扩到 7/8 路) */
#define TCRT_NUM            5

/*
 * 黑线对应的电平。
 * 先烧 CALIB_MODE 用串口实测:
 *   黑线时读到 0 -> 填 0;黑线时读到 1 -> 填 1
 */
#define TCRT_BLACK_LEVEL    0

void    TCRT_Init(void);

/* 立即读一次,返回 1 = 检测到黑线,0 = 白底。无消抖,标定/调试用 */
uint8_t TCRT_ReadRaw(uint8_t id);

/* 采样 + 消抖 + 计算误差。每个控制周期调用一次 */
void    TCRT_Update(void);

/* 以下取值都在 TCRT_Update() 之后有效 */
uint8_t TCRT_Get(uint8_t id);      /* 消抖后的单路结果,1 = 黑线 */
uint8_t TCRT_GetMask(void);        /* 位图,bit0 = 最左 ... bit4 = 最右 */
int16_t TCRT_GetError(void);       /* 加权质心误差:负 = 偏左,正 = 偏右,范围约 ±20 */
uint8_t TCRT_IsLost(void);         /* 1 = 五路全白,丢线 */

#endif /* __TCRT5000_H */