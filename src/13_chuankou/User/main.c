#include "stm32f10x.h"                  // Device header

#include "uart.h"
#include "Delay.h"
#include <string.h>

int main(void)
{
	uart_init();
   //uint8_t temp = OX66
	// Serial_SendByte(temp);//发送字节
	//用hex模式
	
	// char temp = 'Z';
	// Serial_SendByte((uint8_t)temp);
  //用文本模式
	
	 uint8_t temp[10] = "ABCDEFG";
	 Serial_SendArray(temp, 10);//发送数组 需要写长度

	// char temp[10] = "SCY";
	// Serial_SendString(temp);//发送字符串 不需要写长度

	// uint8_t temp1 = 6;
	// float temp_2 = 1.234f;
	// Serial_Printf("temp1 = %d\r\ntemp2 = %f\r\n", temp1, temp_2);//prinf是可以输出变量的

	// Serial_Printf("OK");
//发送数据要放到while1外面
	while(1)
	{
		if (Serial_RxFlag == 1)
		{
			// 接收到新数据

			// Serial_Printf("OK");
			// Delay_ms(10);
			// Serial_SendArray((uint8_t *)Serial_RxPacket, 5);

			// if (Serial_RxPacket[0] == '1')
			// {
			// 	Serial_Printf("One");
			// }
			// else
			// {
			// 	Serial_Printf("ELSE");
			// }

			
//			
//			if(strcmp(Serial_RxPacket, "AAA") == 0)
//			{
//				Serial_Printf("aaa");
//			}
//			else
//			{
//				Serial_Printf("ELSE");
//			}

//			Serial_RxFlag = 0;
//		}
	}
}

