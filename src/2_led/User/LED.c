#include "stm32f10x.h"                  // Device header
 
 void LED_Init(void)
 {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//改
	 
	 GPIO_InitTypeDef GPIO_Initstructure;//定义结构体 结构体类型+结构体名称 配置结构体
	 GPIO_Initstructure.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出模式-改
	 GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0;//引脚-改
	 GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;//速度
		
	 GPIO_Init(GPIOA,&GPIO_Initstructure); //这是一个函数，将结构体参数配置给GPIOA
 
 }
 
 void LED_ON(void)
 {
		GPIO_ResetBits(GPIOA,GPIO_Pin_0 );//设置低电平
 
 }
 void LED_Set(uint8_t LED_Num,uint8_t LED_State)
 {
  switch(LED_Num)
	  {
		case 1:
			switch(LED_State)
			{
				case 0:{
				GPIO_SetBits(GPIOA,GPIO_Pin_0);
				break;}
				case 1:{
				GPIO_ResetBits(GPIOA,GPIO_Pin_0);
					break;}
				}
		case 2:
			switch(LED_State)
			{
				case 0:{
				GPIO_SetBits(GPIOA,GPIO_Pin_1);
				break;}
				case 1:{
				GPIO_ResetBits(GPIOA,GPIO_Pin_1);
					break;}
			}
	
	       
      }
	  
}	 		 
