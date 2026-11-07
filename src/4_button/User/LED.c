#include "stm32f10x.h"                  // Device header
 
 void LED_Init(void)
 {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	 
	 GPIO_InitTypeDef GPIO_Initstructure;
	 GPIO_Initstructure.GPIO_Mode = GPIO_Mode_Out_PP;
	 GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0;
	 GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
		
	 GPIO_Init(GPIOB,&GPIO_Initstructure); 
 
 }
 
 void LED_ON(void)
 {
		GPIO_ResetBits(GPIOA,GPIO_Pin_0 );
 
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
void LED_Turn(void)
{
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0) == 1)//B0口如果是1
	{
		GPIO_ResetBits(GPIOB,GPIO_Pin_0);//熄灭
	}
		else
		{
		 GPIO_SetBits(GPIOB,GPIO_Pin_0);
		}
}