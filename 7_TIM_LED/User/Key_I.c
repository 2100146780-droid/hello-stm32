#include "stm32f10x.h"                  // Device header
#include "LED.h"
void Key_I_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);

		//≈‰÷√GPIO
	  GPIO_InitTypeDef GPIO_Initstructure;
		GPIO_Initstructure.GPIO_Mode =GPIO_Mode_IPU;
		GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1;
		GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA,&GPIO_Initstructure);
	
	//≈‰÷√AFIO
		GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource0);
			
	
		//≈‰÷√EXTI
		EXTI_InitTypeDef EXTI_InitStructure;
		EXTI_InitStructure.EXTI_Line = EXTI_Line0;
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
		EXTI_Init(&EXTI_InitStructure);
	
	//≈‰÷√NVIC
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
	// °¬‘≈–∂œ
		LED_Turn();
		EXTI_ClearITPendingBit(EXTI_Line0);
}
	
uint8_t Key_Read(void)
{
    if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0) == 0)
    {
        //ºÚµ•œ˚∂∂—” ±
        for(uint32_t i=0;i<72000;i++);
        if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0) == 0)
        {
            while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)==0); //µ»¥˝À…ø™
            return 1;
        }
    }
    return 0;
}