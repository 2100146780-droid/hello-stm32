#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "Key_I.h"

// 全局保存PSC值，统一计算
#define TIM2_PSC_VAL    (7200 - 1)
#define TIM2_CNT_FREQ   10000U   // 预分频后计数频率 10000Hz

// 函数：设置TIM2中断定时时间（单位：秒，支持小数如0.5s、2s）
void SetTIM2_Time(float sec)
{
    uint16_t arr_val;
    // 计算ARR
    arr_val = (uint16_t)(sec * TIM2_CNT_FREQ - 1);
    
    // 防止数值越界（TIM2是16位定时器，最大ARR=65535）
    if(arr_val > 65535)
    {
        arr_val = 65535;
    }
    if(arr_val < 0)
    {
        arr_val = 0;
    }
    
    // 修改自动重装载寄存器ARR
    TIM_SetAutoreload(TIM2, arr_val);
    // 产生更新事件，立刻把新ARR载入影子寄存器生效
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);
}

int main(void)
{
    //配置LED的GPIO输出
    LED_Init();
    //打开定时器时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
    
    //配置定时器基础参数 PSC固定不变
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Prescaler = TIM2_PSC_VAL;
    // 先默认1秒初始化
    TIM_TimeBaseStructure.TIM_Period = 10000 - 1;
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);
    
    //配置NVIC
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
    
    //打开更新中断
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
    //开启定时器
    TIM_Cmd(TIM2,ENABLE);
    
    while(1)
    {
   
        SetTIM2_Time(1.0f);  // 1秒翻转一次
				Delay_s(5);
        SetTIM2_Time(0.5f);  // 0.5秒翻转一次
		  	Delay_s(5);
        SetTIM2_Time(2.0f);  // 2秒翻转一次
        Delay_s(5);
       
       
       
        
    }
}

//定时器2中断服务函数
void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2,TIM_IT_Update)!= RESET)
    {
        TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
        
        //翻转PA1 LED
        if(GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_1) == 1)
        {
            GPIO_ResetBits(GPIOA,GPIO_Pin_1);
        }
        else
        {
            GPIO_SetBits(GPIOA,GPIO_Pin_1);
        }
    }
}