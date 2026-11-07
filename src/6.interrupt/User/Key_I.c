#include "stm32f10x.h"                  // 引入STM32F10x标准库寄存器头文件
#include "LED.h"                        // 引入LED驱动自定义头文件，包含LED翻转函数LED_Turn()
//轮询：你每隔 1 秒就看一眼手机，看有没有消息；
//外部中断：手机放一边做别的事，来消息手机响铃打断你，你再看消息。
/**
 * @brief  外部中断0初始化函数，PA0引脚上拉输入，下降沿触发外部中断
 * @param  无
 * @retval 无
 */
void Key_I_Init(void)
{
    // 1. 开启GPIOA端口时钟、AFIO复用功能时钟（外部中断必须开启AFIO时钟）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	 //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);

    
    GPIO_InitTypeDef GPIO_Initstructure;// 定义GPIO初始化结构体变量
    GPIO_Initstructure.GPIO_Mode =GPIO_Mode_IPU;// 设置引脚模式：上拉输入模式IPU
    GPIO_Initstructure.GPIO_Pin = GPIO_Pin_0;// 指定配置引脚：PA0//改*********
    GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;// 输入模式下Speed参数无实际作用，标准库规范填写50MHz
    GPIO_Init(GPIOA,&GPIO_Initstructure); // 将上述配置写入GPIOA寄存器，完成PA0初始化

//    GPIO_Initstructure.GPIO_Mode =GPIO_Mode_IPU;// 设置引脚模式：上拉输入模式IPU
//    GPIO_Initstructure.GPIO_Pin = GPIO_Pin_1;// 指定配置引脚：PA0//改*********
//    GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;// 输入模式下Speed参数无实际作用，标准库规范填写50MHz
//    GPIO_Init(GPIOB,&GPIO_Initstructure); // 将上述配置写入GPIOA寄存器，完成PA0初始化
    // 2. AFIO配置：将EXTI0外部中断线路映射到GPIOA第0号引脚,上面配置哪个引脚下面也配置哪个 
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource0);

    // 3. EXTI外部中断线配置
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;//选择外部中断线0*******
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;//打开中断线
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	// 模式选择：中断模式（区别于事件模式Event）
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//改上升沿还是下降沿,Falling
		//下降沿的话，按下的时候触发中断
		//上升沿的话，松开的时候触发中断
	
//    EXTI_InitStructure.EXTI_Line = EXTI_Line6;//选择外部中断线1*******
//    EXTI_InitStructure.EXTI_LineCmd = ENABLE;//打开中断线
//    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	// 模式选择：中断模式（区别于事件模式Event）
//    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    
    EXTI_Init(&EXTI_InitStructure);// 写入EXTI寄存器，完成外部中断线0配置

    // 4. NVIC中断优先级分组与中断通道配置
    // 设置NVIC优先级分组为2：2位抢占优先级，2位子优先级
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//配置中断分组（每个工程只有一个分组）
    NVIC_InitTypeDef NVIC_InitStructure;//定义结构体
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;//选择中断线*******
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//打开中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//1是最大的优先级，4优先级最小 // 设置抢占优先级为1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; // 设置子优先级为1
	
   
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;//选择中断线*******
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//打开中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =2;//1是最大的优先级，4优先级最小 // 设置抢占优先级为1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2; // 设置子优先级为1
    // 将优先级配置写入NVIC寄存器
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief  EXTI0外部中断服务函数，中断触发后自动进入该函数
 * @note   函数名固定不可修改，启动文件已绑定EXTI0中断入口
 */
void EXTI0_IRQHandler(void)//每个通道这个名字是固定的********
{
	  //中断标志判断：if(EXTI_GetITStatus(EXTI_Line0) == SET)如果是单独通断线的话就不需要，如果不是的话就需要
    
    //中间是中断函数的具体内容
	   LED_Turn();// 按键中断触发，调用LED翻转函数，实现按键控制LED亮灭切换
   
    EXTI_ClearITPendingBit(EXTI_Line0);// 必须手动清除EXTI0中断挂起标志位，否则会重复进入中断
	
}//这个中断程序不需要再.h文件中声明，他是自动执行的
//void EXTI9_5_IRQHandler(void)//每个通道这个名字是固定的********
//{
//	  if(EXTI_GetITStatus(EXTI_Line6) == SET)如果是单独通断线的话就不需要，如果不是的话就需要
//    {
//    //中间是中断函数的具体内容
//	   LED_Turn();// 按键中断触发，调用LED翻转函数，实现按键控制LED亮灭切换
//    }
//    EXTI_ClearITPendingBit(EXTI_Line6);// 必须手动清除EXTI0中断挂起标志位，否则会重复进入中断
//	
//}//这个中断程序不需要再.h文件中声明，他是自动执行的
	