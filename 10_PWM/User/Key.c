#include "stm32f10x.h"                  // 标准库寄存器、外设宏定义头文件
#include "Delay.h"                      // 自己写的毫秒延时函数头文件

// 按键PB11初始化函数
void Key_Init(void)
{
	// 开启GPIOB外设时钟（APB2总线）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	// 定义GPIO初始化结构体
	GPIO_InitTypeDef GPIO_Initstructure;
	// 模式：上拉输入（按键一端接地，按下引脚变低电平0）
	GPIO_Initstructure.GPIO_Mode = GPIO_Mode_IPU;
	// 选择引脚PB11
	GPIO_Initstructure.GPIO_Pin = GPIO_Pin_11;
	// 输入模式下Speed参数无实际作用，习惯填50MHz
	GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
	// 把配置写入GPIOB端口
	GPIO_Init(GPIOB,&GPIO_Initstructure); 
}

// 获取按键按下标志：按下返回1，无操作返回0
uint8_t Key_GetFlag(void)
{
	uint8_t Flag = 0;
	// 检测引脚电平为低=按键按下
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11) == 0)
	{
		Delay_ms(10);         // 消抖延时10ms
		while(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11) == 0); // 等待松手
		Delay_ms(10);         // 松手消抖
		Flag = 1;             // 标记有一次有效单击
	}
	return Flag;
}