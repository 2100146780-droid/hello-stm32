#include "stm32f10x.h"
#include "OLED.h"
#include "Key_I.h"

// 计时状态枚举
typedef enum
{
    STATE_IDLE,    // 空闲清零
    STATE_RUN,     // 计时运行
    STATE_STOP     // 计时暂停
} RunState;

RunState g_state = STATE_IDLE;
uint32_t g_time_ms = 0;    // 总毫秒计时
uint8_t g_key_flag = 0;    // 按键触发标志

// 函数声明
void TIM2_Init_1ms(void);

int main(void)
{
    // 外设初始化
    OLED_Init();
    OLED_Clear();
    Key_I_Init();    // 调用独立按键驱动初始化
    TIM2_Init_1ms();

    OLED_ShowString(1, 1, "StopWatch");
    OLED_ShowString(2, 1, "Time:000000 ms");

    while(1)
    {
        // 检测按键触发
        if(g_key_flag == 1)
        {
            g_key_flag = 0;
            switch(g_state)
            {
                case STATE_IDLE:
                    g_state = STATE_RUN;
                    break;
                case STATE_RUN:
                    g_state = STATE_STOP;
                    break;
                case STATE_STOP:
                    g_state = STATE_IDLE;
                    g_time_ms = 0;
                    break;
            }
        }
        // 刷新毫秒数值
        OLED_ShowNum(2, 7, g_time_ms, 6);
    }
}

// TIM2 72MHz系统时钟，1ms定时中断
void TIM2_Init_1ms(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // 预分频71，计数999 → 72000分频 = 1ms
    TIM_TimeBaseStruct.TIM_Prescaler = 71;
    TIM_TimeBaseStruct.TIM_Period = 999;
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    // NVIC配置
    NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

// TIM2中断服务函数，1ms累加计时
void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        if(g_state == STATE_RUN)
        {
            g_time_ms++;
        }
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}