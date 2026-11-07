#include "stm32f10x.h"                  // Device header

#include "LED.h"
#include "Delay.h"

int main(void)
	
{
	LED_Init();
	
	
	while(1)
	{
		//A0推挽输出，led正极接GPIO负极接GND
		GPIO_SetBits(GPIOA,GPIO_Pin_0);//置1
		
	 
	}


}
