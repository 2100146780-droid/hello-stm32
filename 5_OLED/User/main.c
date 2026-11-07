#include "stm32f10x.h"                  // Device header

#include "LED.h"
#include "Key.h"
#include "OLED.h"
int main(void)
	
{
	Key_Init();          //B11
	OLED_Init();  //接6789口，gnd-6,vcc-7,SCL-8,SDA-9;
	
	int Number = 0;
	
	while(1)
	{
		
    OLED_ShowNum(1,1,2026,4);//第一行一列 2026 四位数
    OLED_ShowChar(1,5,'.');//显示符号
		OLED_ShowNum(1,6,6,1);
    OLED_ShowChar(1,7,'.');
		OLED_ShowNum(1,8,9,1);

		OLED_ShowString(2,1,"Hello");//显示字符串
		
		OLED_ShowNum(4,1,Number,1);
		
			if  (Key_GetFlag() == 1)
			{
				Number++;
			}

	}
}


