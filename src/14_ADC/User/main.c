//利用Debug读取数据
#include "stm32f10x.h"      
uint16_t AD_Value[2];//*********

void adc_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);		//开启ADC时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);	//开启GPIOA时钟
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);		//打开DMA时钟,是AHB总线的设备，所以要用AHB开启时钟函数

	//配置GPIO
	GPIO_InitTypeDef GPIO_Initstructure;
	GPIO_Initstructure.GPIO_Mode = GPIO_Mode_AIN;				//配置为模拟输入 ADC的专属模式，此时GPIO失效，防止干扰
	GPIO_Initstructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_Initstructure);

	//配置ADC规则组
	ADC_RegularChannelConfig(ADC1,ADC_Channel_1,1,ADC_SampleTime_55Cycles5);		//选择规则组的输入通道
	ADC_RegularChannelConfig(ADC1,ADC_Channel_2,2,ADC_SampleTime_55Cycles5);		//选择规则组的输入通道///****************

	//初始化ADC
	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;					//配置ADC工作模式（独立工作模式/双ADC模式）	配置成独立工作模式
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;				//数据对齐 （左对齐/右对齐）	配置成右对齐
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//*********
	//外部触发转换选择，外部触发源选择	这里选择不使用外部触发，因为这里用程序触发
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;					//连续转换模式（单次转换/连续转换） ENABLE是连续，DISABLE是单次
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;	//**********					//扫描转换模式（扫描模式（多通道）/非扫描模式（单通道））ENABLE就是扫描模式
	ADC_InitStructure.ADC_NbrOfChannel = 2;					//**********			//通道数目 指定扫描模式下总共用到几个通道		//这个参数只在扫描模式有用，非扫描模式下只有第一个序列有效
	ADC_Init(ADC1,&ADC_InitStructure);									//配置成单次转换、非扫描模式

	//配置DMA参数：
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;				//外设站点的起始地址 可以在函数名的括号里选择地址
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//外设站点的数据宽度 （Byte=uint8_t,HalfWord=uint16_t,Word=uint32_t）
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;			//外设站点的是否自增
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)AD_Value;					//******************//存储器站点的起始地址 可以在函数名的括号里选择地址
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;			//存储器站点的数据宽度
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;	//************					//存储器站点的是否自增
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;							//传输方向	(DST=外设站点作为目的地，传输方向就是从存储器到外设站点。SRC=外设站点作为源头，从外设站点到存储器)
	DMA_InitStructure.DMA_BufferSize = 2;		//**********								//缓冲区大小其实就是传输计数器 指定传输几次
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;								//传输模式，是否自动重装 （Circular=循环模式，传输计数器自动重装。Normal=正常模式，不自动重装，自减到0就停下）
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;								//是否是存储器到存储器，就是选择硬件触发还是软件触发(ENABLE=软件触发。DISABLE=硬件触发)
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;						//优先级 (VeryHigh,High,Medium,Low)
	DMA_Init(DMA1_Channel1,&DMA_InitStructure);									//选择DMA1的CH1

	DMA_Cmd(DMA1_Channel1,ENABLE);	//使能DMA,使其开始转运
	ADC_DMACmd(ADC1,ENABLE);		//开启DMA触发信号

	//开启ADC
	ADC_Cmd(ADC1,ENABLE);

	//校准ADC
	ADC_ResetCalibration(ADC1);							//复位校准
	while (ADC_GetCalibrationStatus(ADC1) == SET);		//返回复位校准状态	如果未完成校准就一直在while循环里等待
	ADC_StartCalibration(ADC1);							//启动校准
	while (ADC_GetCalibrationStatus(ADC1) == SET);		//获取校准状态 等待校准完成

	ADC_SoftwareStartConvCmd(ADC1,ENABLE);				
//打开软件触发********* 用于触发ADC进行转换
}

int main(void)
{
	adc_init();

	while (1)
	{

	}

}

