#include "stm32f10x.h"                  // Device header

#include "LED.h"
#include "Key.h"

int main(void)
	
{
	Key_Init();          //B11
	LED_Init();
	
	while(1)
	{
		//GPIO_SetBits(GPIOB,GPIO_Pin_0);
			if  (Key_GetFlag() == 1)
			{
				LED_Turn();
			}

	}
}


