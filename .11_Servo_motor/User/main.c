#include "stm32f10x.h"                  // Device header
#include "Delay.h"
//PWM连PA2

// 定义舵机角度范围
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180

// 定义PWM周期(20ms)和脉冲宽度范围(0.5ms-2.5ms)
#define PWM_PERIOD 20000  // 20ms in microseconds 
#define PWM_MIN_PULSE 500  // 0.5ms -90°
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
	TIM_SetCompare3(TIM2, pulse_width);//TIM2通道3
}


int main(void)
{
	// 配置PWM输出引脚
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		
	GPIO_InitTypeDef GPIO_Initstructure;									
	GPIO_Initstructure.GPIO_Mode = GPIO_Mode_AF_PP;				// 复用推挽输出
	GPIO_Initstructure.GPIO_Pin = GPIO_Pin_2;							
	GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;				
	GPIO_Init(GPIOA, &GPIO_Initstructure);		

	// 配置定时器   1kHz
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	// 配置定时器参数
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	
	TIM_TimeBaseStructure.TIM_Period = PWM_PERIOD - 1;			// ARR
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;				// PSC
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);	
	
	// 配置PWM模式
	TIM_OCInitTypeDef TIM_OCStruct;
	TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCStruct.TIM_Pulse = PWM_MIN_PULSE - 1;					// CCR 
  TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM2, &TIM_OCStruct); //TIM2通道3

	// 开始运行定时器
	TIM_Cmd(TIM2, ENABLE);
	
	//设置舵机角度
	Servo_SetAngle(180);
	
	
	while (1)
	{
		
	}
	
}



