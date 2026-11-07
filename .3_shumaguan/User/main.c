#include "stm32f10x.h"                  // Device header

int main(void)
	
{
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	 GPIO_InitTypeDef GPIO_Initstructure;
	 GPIO_Initstructure.GPIO_Mode = GPIO_Mode_Out_PP;
	 GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	 GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOA,&GPIO_Initstructure); 
	
	 GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1| GPIO_Pin_3|GPIO_Pin_4;
	 GPIO_Init(GPIOB,&GPIO_Initstructure); 
	
	while(1) 
	{
			
 	  GPIO_ResetBits(GPIOA,GPIO_Pin_4|GPIO_Pin_5);
	  GPIO_SetBits(GPIOA,GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_6|GPIO_Pin_7);
		
    GPIO_SetBits(GPIOB,GPIO_Pin_1);
		GPIO_ResetBits(GPIOB,GPIO_Pin_0|GPIO_Pin_3|GPIO_Pin_4);
	}
}
