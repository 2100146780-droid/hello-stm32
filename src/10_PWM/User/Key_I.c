
#include "stm32f10x.h"                  // Device header
#include "LED.h"                 //引入led头文件
void Key_I_Init(void)//函数：按键外部中断初始化函数，main 里调用一次就行，只初始化一次硬件。
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//开启GPIOA外设时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//AFIO：复用功能 IO。EXTI 外部中断需要 AFIO 来把 PA0 映射到 EXTI0 中断线，**EXTI 必须开 AFIO 时钟！**

		//配置GPIO
	  GPIO_InitTypeDef GPIO_Initstructure;
		GPIO_Initstructure.GPIO_Mode =GPIO_Mode_IPU;
		GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0;
		GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA,&GPIO_Initstructure);
	
	//配置AFIO
		GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource0);
			
	
		//配置EXTI
		EXTI_InitTypeDef EXTI_InitStructure;
		EXTI_InitStructure.EXTI_Line = EXTI_Line0;
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
		EXTI_Init(&EXTI_InitStructure);
	
	//配置NVIC
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
		NVIC_InitTypeDef NVIC_InitStructure;
		NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
		NVIC_Init(&NVIC_InitStructure);
	
}

void EXTI0_IRQHandler(void)
{
	//省略判断
		LED_Turn();
		EXTI_ClearITPendingBit(EXTI_Line0);
}
/*
#include "stm32f10x.h"                  // 引入STM32标准库头文件，GPIO、EXTI、NVIC函数都在这里
#include "LED.h"                       // 引入LED头文件，用来调用LED翻转函数LED_Turn()

// PA0外部中断初始化函数，main函数调用一次即可
void Key_EXTI0_Init(void)
{
	// 开启GPIOA外设时钟，STM32外设默认关闭时钟，不开时钟GPIO无法工作
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	// 开启AFIO复用IO时钟，EXTI外部中断必须开启AFIO才能映射引脚
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	// 定义GPIO初始化结构体，存放GPIO配置参数
	GPIO_InitTypeDef GPIO_Initstructure;
	GPIO_Initstructure.GPIO_Mode = GPIO_Mode_IPU;        // 设置PA0为上拉输入：无按键时默认高电平
	GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0;            // 指定配置PA0引脚
	GPIO_Init(GPIOA, &GPIO_Initstructure);               // 将上面配置写入寄存器，GPIO配置生效
	
	// AFIO配置：把EXTI0中断线映射到PA0引脚
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
			
	// 定义EXTI外部中断初始化结构体
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;           // 选择EXTI0中断线
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;             // 使能EXTI0这条中断线
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;   // 设置为中断模式（另一种是事件模式）
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // 下降沿触发：高电平变低电平瞬间触发中断
	EXTI_Init(&EXTI_InitStructure);                       // EXTI配置写入寄存器，配置生效
	
	// NVIC中断分组配置：分组2，2位抢占优先级、2位子优先级，整个工程只需要调用一次
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	// 定义NVIC中断控制器结构体
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;                 // 选择EXTI0对应的中断通道
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                   // 使能EXTI0中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;        // 抢占优先级1，数字越小优先级越高
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;               // 子优先级1，同抢占优先级时决定响应顺序
	NVIC_Init(&NVIC_InitStructure);                                   // NVIC配置生效
}

// EXTI0中断服务函数，名字固定不能修改，中断触发CPU自动进入，不能手动调用
void EXTI0_IRQHandler(void)
{
	// 判断是否是EXTI0产生的中断标志，SET代表有中断请求
	if(EXTI_GetITStatus(EXTI_Line0) == SET)
	{
		LED_Turn();                          // 翻转LED状态，亮灭切换
		EXTI_ClearITPendingBit(EXTI_Line0); // 清除EXTI0中断挂起标志，必须写！不清会反复进中断
	}
}
*/