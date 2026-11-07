//使用哪个串口也是要看引脚定义图的USART1_RX URAST1_TX pa9pa10

#include "uart.h"

uint8_t Serial_RxBuffer[100];
uint8_t Serial_TxBuffer[100];

char Serial_RxPacket[100];
uint8_t Serial_RxFlag = 0;
uint16_t Serial_RxLength = 0;

void uart_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);		//开启串口时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);		//开启GPIOA时钟

	//初始化GPIO引脚：
	GPIO_InitTypeDef GPIO_Initstructure;
	GPIO_Initstructure.GPIO_Mode = GPIO_Mode_AF_PP;		//TX复用推挽输出
	GPIO_Initstructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_Initstructure);
	GPIO_Initstructure.GPIO_Mode = GPIO_Mode_IPU;			//RX上拉输入
	GPIO_Initstructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_Initstructure);

    //初始化USART：
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;    //波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;     //基本不变 叫硬件流控制
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                     //配置成串口模式
	USART_InitStructure.USART_Parity = USART_Parity_No;                         //校验位（No=无校验。Odd=奇校验。Even=偶校验
	USART_InitStructure.USART_StopBits = USART_StopBits_1;                      //停止位	（参数可以选择0_5、1、1_5、2）
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;                 //字长 （8b=8位。9b=9位）不需要校验就选择8位
	USART_Init(USART1, &USART_InitStructure);//串口一 ****

    //初始化中断：
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//基本不变 
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn; //这里串口一*********************************
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    // 配置DMA
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//先打开DMA时钟
    // Rx
    DMA_InitTypeDef DMA_InitStructure; //定义结构体
    DMA_DeInit(DMA1_Channel5);          //是取消初始化 先处于一个关闭状态 UART1 Rx 对应了 DAM1 CH5****不同串口不一样
		
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;  
		//外设地址，是从什么地址挪到什么地址，串口一接受地址就是这样 基本不变   
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)Serial_RxBuffer;
		//这个是个数组 需要定一个一个全局变量，一定要是全局变量，因为其他地方还想调用 可以改更大 在开头//内存地址，就是要把串口地址数据接受的放在哪里
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//方向：从外设到内存  从串口到地址
		
    DMA_InitStructure.DMA_BufferSize = 100;//缓存区大小就是数组的大小 在开头对应***************************************
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设地址不递增 每次串口接收数据都会放到一个固定的位置
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//内设地址是要递增的为了防止被覆盖
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; //因为我们初始化USART的时候字长是8位数据
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;//也是八位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//基本不变
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;//选择优先级
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;//非内存到内存
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);//调用
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);//用来打开DMA
    DMA_Cmd(DMA1_Channel5, ENABLE);//用来打开DMA
    DMA_ClearFlag(DMA1_FLAG_TC5 | DMA1_FLAG_HT5 | DMA1_FLAG_TE5);   // 清标志
    // Tx
    DMA_DeInit(DMA1_Channel4);//也是要固定的******************注意问清楚串口几对应几
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)Serial_TxBuffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;//和rx不一样 传输方向
    DMA_InitStructure.DMA_BufferSize = 0; // 动态设置 先不管
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    DMA_Cmd(DMA1_Channel4, ENABLE);//打开*************************888

    // 打开串口
    USART_Cmd(USART1, ENABLE);
}
//void Serial_3_DMA_TX_Init(void)
//{
//    DMA_InitTypeDef DMA_InitStructure;

//    DMA_DeInit(DMA1_Channel2);
//    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
//    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)Serial_3_TxBuffer;
//    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
//    DMA_InitStructure.DMA_BufferSize = 0; // 动态设置
//    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
//    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//    DMA_Init(DMA1_Channel2, &DMA_InitStructure);

//    USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
//}

// 串口中断函数
void USART1_IRQHandler(void)
{
    // 检查 IDLE 中断
    if (USART_GetITStatus(USART1, USART_IT_IDLE) == SET)
    {
        (void)USART1->SR;
        (void)USART1->DR;

        // 关闭 DMA 担心读取过程中把DMA给改变了
        DMA_Cmd(DMA1_Channel5, DISABLE);

        // 计算本次接收长度
        uint16_t data_len = 100 - DMA1_Channel5->CNDTR;

        // 清空目标缓冲区并拷贝数据
        if (data_len > 0)
        {
            memset(Serial_RxPacket, 0, 100);  // 清空整个目标数组
            memcpy(Serial_RxPacket, Serial_RxBuffer, data_len); // 拷贝接收到的数据
            Serial_RxPacket[data_len] = '\0';
            Serial_RxLength = data_len;  // 记录数据长度**********************
            Serial_RxFlag = 1;           // 标记有新数据*********************
        }

        // 重装 DMA 环形缓冲，继续接收
        DMA1_Channel5->CNDTR = 100;
        DMA_Cmd(DMA1_Channel5, ENABLE);

        // 清 IDLE 中断标志（已经在读 DR 时清了，这里可选）
        USART_ClearITPendingBit(USART1, USART_IT_IDLE);
        return;
    }
}

// 发送一个字节
void Serial_SendByte(uint8_t Byte)
{
	// 把数据写到 DMA 发送缓冲区
    Serial_TxBuffer[0] = Byte;

    // 关闭通道，更新发送长度、地址，再打开通道
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA1_Channel4->CNDTR = 1;                              // 只发 1 个字节
    DMA1_Channel4->CMAR  = (uint32_t)&Serial_TxBuffer[0];  // 内存地址
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

// 发送一个数组  参数：Array待发送数组的地址（数组名）  Length数组的长度
void Serial_SendArray(uint8_t *Array,uint16_t Length)
{
	// 拷贝数据到 DMA 发送缓冲区
    for (uint16_t i = 0; i < Length; i++)
    {
        Serial_TxBuffer[i] = Array[i];
    }

    // 关闭 DMA，更新长度，再使能
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA1_Channel4->CNDTR = Length;
    DMA1_Channel4->CMAR = (uint32_t)Serial_TxBuffer;
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

// 发送一个字符串 参数：String待发送的字符串地址
void Serial_SendString(char*String)
{
	uint16_t length = strlen(String);
    if (length == 0 || length > 100) return;  // 边界保护

    // 把整个字符串拷贝到 DMA 缓冲区
    memcpy(Serial_TxBuffer, String, length);

    // 关闭通道，更新发送长度、地址，再打开通道
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA1_Channel4->CNDTR = length;                             // 字符串长度
    DMA1_Channel4->CMAR  = (uint32_t)Serial_TxBuffer;          // 缓冲区首地址
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

void Serial_Printf(char *format, ...)
{
	char String[100];
	va_list arg;
	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);
	Serial_SendString(String);
}


