#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "Key_I.h"


//int main(void)
//{
//	
//	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
//	 
//	 GPIO_InitTypeDef GPIO_Initstructure;
//	 GPIO_Initstructure.GPIO_Mode = GPIO_Mode_AF_PP;//需要复用推挽输出
//	 GPIO_Initstructure.GPIO_Pin = GPIO_Pin_2;
//	 GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
//	 GPIO_Init(GPIOA,&GPIO_Initstructure); 
//	
//	 GPIO_Initstructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	 GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3;
//	 GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
//	 GPIO_Init(GPIOA,&GPIO_Initstructure); 
//	
//    //配置LED的GPIO输出 1khz
//    LED_Init();
//    //打开定时器时钟
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
//    
//    //配置定时器基础参数 PSC固定不变
//    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
//    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
//    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
//	  TIM_TimeBaseStructure.TIM_Period = 1000 - 1;//ARR
//    TIM_TimeBaseStructure.TIM_Prescaler = 72-1;//PSC
//    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);
//    
//		//配置PWM模式 通用定时器只需要配置这四个
//		TIM_OCInitTypeDef TIM_OCStruct;
//		TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;//基本不变
//		TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;//基本不变
//	  TIM_OCStruct.TIM_Pulse = 500  - 1;   //CCR占空比50% 500CRR/1000ARR
//	  TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_High; //先输出高电平
//		TIM_OC3Init(TIM2,&TIM_OCStruct);
//		
//		//开启定时器
//    TIM_Cmd(TIM2,ENABLE);
//		
//		
//		   while(1)
//    {
//			GPIO_ResetBits(GPIOA,GPIO_Pin_0);
//      GPIO_SetBits(GPIOA,GPIO_Pin_1);  
//      GPIO_SetBits(GPIOA,GPIO_Pin_3);
//      Delay_s(3);       
//      GPIO_SetBits(GPIOA,GPIO_Pin_0);
//      GPIO_ResetBits(GPIOA,GPIO_Pin_1);  
//      GPIO_SetBits(GPIOA,GPIO_Pin_3);
//			Delay_s(3);   
//			GPIO_ResetBits(GPIOA,GPIO_Pin_0);
//      GPIO_ResetBits(GPIOA,GPIO_Pin_1);  
//      GPIO_SetBits(GPIOA,GPIO_Pin_3);
//			Delay_s(3);   
//    }
//}




// 定义舵机角度范围
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180

// 定义PWM周期(20ms)和脉冲宽度范围(0.5ms-2.5ms)
#define PWM_PERIOD 20000  // 20ms in microseconds
#define PWM_MIN_PULSE 500  // 0.5ms
#define PWM_MAX_PULSE 2500  // 2.5ms

void Servo_SetAngle(uint8_t angle)
{
    uint16_t pulse_width;
    
    // 限制角度范围
    if(angle > SERVO_MAX_ANGLE)
        angle = SERVO_MAX_ANGLE;
    
    // 计算脉冲宽度(线性映射)
    pulse_width = PWM_MIN_PULSE + (angle * (PWM_MAX_PULSE - PWM_MIN_PULSE)) / SERVO_MAX_ANGLE;
    
    // 设置PWM占空比
    TIM_SetCompare3(TIM2, pulse_width);
}


int main(void)
{
        // 配置PWM输出引脚
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);                
        GPIO_InitTypeDef GPIO_Initstructure;                                                                        
        GPIO_Initstructure.GPIO_Mode = GPIO_Mode_AF_PP;    // 复用推挽输出
        GPIO_Initstructure.GPIO_Pin = GPIO_Pin_2;                  //A2                                      
        GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;                                
        GPIO_Init(GPIOA, &GPIO_Initstructure);                

        // 配置定时器   1kHz
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
        // 配置定时器参数
        TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
        TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;                
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;        
        TIM_TimeBaseStructure.TIM_Period = PWM_PERIOD - 1;    // ARR
        TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;        // PSC
        TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);        
        
        // 配置PWM模式
        TIM_OCInitTypeDef TIM_OCStruct;
        TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;
        TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
        TIM_OCStruct.TIM_Pulse = PWM_MIN_PULSE - 1;        // CCR 
        TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_High;
        TIM_OC3Init(TIM2, &TIM_OCStruct); 

        // 开始运行定时器
        TIM_Cmd(TIM2, ENABLE);
        
        //设置舵机角度
        Servo_SetAngle(90);
        
        
        while (1)
        {
                
        }
        
}

