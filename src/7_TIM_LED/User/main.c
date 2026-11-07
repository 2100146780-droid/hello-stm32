/*delay会阻塞main函数，定时器本质上是中断，定时器不会。

当定时器定时一秒时，在这一秒内，其他程序是可以正常运行的，一秒结束之后会进入中断函数，才会
运行中断函数主要是定时如：1秒钟一次，2秒钟一次。或者是波形的形成，简称TIM,timer。

基本定时器：支持内部时钟，向上计数
通用定时器：最常用，PWM,输入捕获，输出比较
高级定时器：支持互补PWM输出，常用于电机控制
低功耗定时器：在低功耗模式下工作

在stm32中*******
通用定时器：TIM2,TIM3,TIM4
高级定时器：TIM1

PSC:系统的时钟称为主频，PSC=2 则把主频除以2 从而实现对主频速度的限制
作用：把主频降低 
CNT:CNT计数，是主频降低之后每走一步都+1
ARR:ARR是计数设置的终点值，当CNT计数到ARR值之后，才开始进入中断模式


*/

#include "stm32f10x.h"                  // Device header
#include "LED.h"
#include "Key_I.h"

/**
 * @brief 设置TIM2更新中断周期，只修改ARR，PSC固定7199不变
 * @param sec 中断周期，单位：秒，范围0.0001 ~ 6.5536
 */
void TIM2_SetTimerPeriod(float sec)
{
    uint16_t arr;
    //限幅防止溢出
    if(sec < 0.0001f)
    {
        sec = 0.0001f;
    }
    if(sec > 6.5536f)
    {
        sec = 6.5536f;
    }
    arr = (uint16_t)(sec * 10000.0f - 1);
		//影子寄存器：写入 ARR 不会立刻生效，等下一次计数溢出才加载新值。
    //想要**立即生效**，写完 ARR 加一句手动更新事件：
    TIM_SetAutoreload(TIM2, arr);
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);

}

int main(void)
	
{
	
  LED_Init();//配置LED的GPIO输出
	Key_I_Init();
	//配置定时器 定时一秒

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);	//打开定时器TIM2时钟
	//配置定时器参数:
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;//定义结构体
	TIM_TimeBaseStructure.TIM_Period = 10000-1;            //ARR *******
	TIM_TimeBaseStructure.TIM_Prescaler = 7200-1;          //PSC *******这两个数控制闪烁的速度
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;//默认不分配 相当于PSC 时钟分频，意思是说出来PSC还有另一个，一般用不太到
	//其中TIM_CKD_DIV1为不分频，TIM_CKD_DIV2为2分频，TIM_CKD_DIV4为4分频，一共三个固定
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//计数模式 ：4321 or 1234向上分频 一般为向上计数
	//TIM_TimeBaseStructure.TIM_RepetitionCounter = //高级定时器的 ： 
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);//将结构体调用到TIM2中
	
	//只有用到定时器中断的时候才用配置NVIC
		NVIC_InitTypeDef NVIC_InitStructure;
		NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;//中断通道：打开TIM2中断  ********
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
		NVIC_Init(&NVIC_InitStructure);
	//打开定时器中断
		TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
		//开始运行定时器
		TIM_Cmd(TIM2,ENABLE);
		
	while(1)
	{
		 if(Key_Read() == 1)
	{
		TIM2_SetTimerPeriod(0.3f);
	}
  
	}
}
//定时器中断函数
void TIM2_IRQHandler(void)//这个定时器中断函数的名字是固定的不能随便改，每一个都是固定的
{
		if(TIM_GetITStatus(TIM2,TIM_IT_Update)!= RESET)//判断标志位
		{
			TIM_ClearITPendingBit(TIM2,TIM_IT_Update);//清空标志位
			
			//执行中断内容
//				 if(GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_1) == 1)//读取当前GPIO输出是1还是0
//			  {
//					GPIO_ResetBits(GPIOA,GPIO_Pin_1);
//			  }
//				 else
//				{
//				GPIO_SetBits(GPIOA,GPIO_Pin_1);
//				}
//			
			 LED_Turn();
			
		}

}

//AI使用：比如 帮我生成一个STM32F103C8T6芯片的定时器代码，使用TIM3定时器，实现每秒钟发起一次定时器中断