/*==============================================================================
 *  HC-SR04 超声波测距 —— 入门注释版
 *
 *  说明：这份代码 = 你学过的几课拼起来
 *        1_project_temple  -> main 骨架、初始化顺序
 *        2_led             -> PA0 推挽输出（发触发脉冲）
 *        4_button          -> PA1 输入（读 ECHO 电平）
 *        6.interrupt       -> EXTI1 双边沿中断 + NVIC（核心）
 *        7_TIM_LED         -> TIM2 当微秒秒表
 *        13_chuankou       -> 串口 printf 打印距离
 *
 *  接线：TRIG -> PA0    ECHO -> PA1    串口 TX -> PA9   记得和板子共地
 *  串口助手：115200, 8, N, 1
 *
 *  工程设置（和你之前每一课一样）：
 *    Define:  USE_STDPERIPH_DRIVER, STM32F10X_MD
 *    Include: CMSIS 两个目录 + StdPeriph_Driver\inc
 *    库文件:  stm32f10x_rcc.c gpio.c tim.c exti.c usart.c misc.c
 *    勾选 Use MicroLIB（printf 才能用）
 *============================================================================*/

#include "stm32f10x.h"
#include <stdio.h>

/* 声速：1 微秒声波走 0.0343 厘米（空气 20 度时） */
#define SHENGSU   0.0343f

/*------------------------------------------------------------------------------
 *  三个全局变量
 *  为什么要写 volatile？
 *    因为它们在"中断函数"里被改，在"main"里被读。
 *    不加 volatile，编译器会以为 main 里没人改它，把你的判断优化掉，
 *    结果 main 永远等不到 —— 这是中断课老师强调过的点。
 *  这里都是 16 位/8 位，读写一次就完成，不用担心被打断（原子）。
 *---------------------------------------------------------------------------*/
volatile uint16_t kaishi  = 0;   /* 上升沿那一刻，秒表的读数            */
volatile uint16_t kuandu  = 0;   /* ECHO 高电平持续了多少微秒          */
volatile uint8_t  wancheng = 0;  /* 1 = 这次测量完成了                  */

/*==============================================================================
 *  第 1 块：秒表  —— 对应 7_TIM_LED
 *  那一课你让定时器定时去翻转 LED，这里只保留"计数器在跑"这个功能。
 *  72MHz / 72 = 1MHz，也就是计数器每加 1 = 过去了 1 微秒。
 *============================================================================*/
void miaobiao_init(void)
{
    TIM_TimeBaseInitTypeDef tb;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);  /* 开定时器时钟 */

    tb.TIM_Prescaler         = 72 - 1;   /* 预分频：72MHz -> 1MHz */
    tb.TIM_Period            = 65535;   /* 计到 65535 就归零重来 */
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tb);

    TIM_SetCounter(TIM2, 0);
    TIM_Cmd(TIM2, ENABLE);               /* 让秒表开始跑 */
}

/* 微秒延时：看一眼秒表，等它走够 us 个数 */
void delay_us(uint16_t us)
{
    uint16_t t0 = TIM_GetCounter(TIM2);
    while ((uint16_t)(TIM_GetCounter(TIM2) - t0) < us) {
        /* 空转等待 */
    }
}

/*==============================================================================
 *  第 2 块：串口  —— 对应 13_chuankou
 *  那一课的代码原样搬过来即可，一个字都不用改。
 *============================================================================*/
void chuankou_init(void)
{
    GPIO_InitTypeDef  gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_9;            /* TX */
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin   = GPIO_Pin_10;           /* RX */
    gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate            = 115200;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

/* printf 重定向：让 printf 的内容从串口发出去（13_chuankou 里的老朋友） */
int fputc(int ch, FILE *f)
{
    (void)f;
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, (uint8_t)ch);
    return ch;
}

/*==============================================================================
 *  第 3 块：引脚 + 中断  —— 对应 2_led + 4_button + 6.interrupt
 *  把"按键中断"那节课的五个步骤照抄，只换引脚和边沿：
 *    ①开时钟  ②配引脚  ③挂 EXTI 线  ④配边沿  ⑤给 NVIC
 *============================================================================*/
void yinjiao_zhongduan_init(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    /* ① 开时钟：GPIOA 和 AFIO（AFIO 不开，EXTI 挂不上去，中断永远不来）*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    /* ② 配 PA0：推挽输出 —— 就是 2_led 里点亮 LED 的那种配置 */
    gpio.GPIO_Pin   = GPIO_Pin_0;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);       /* 先给低电平，模块要求 */

    /* ② 配 PA1：浮空输入 —— 就是 4_button 里读按键的那种配置 */
    gpio.GPIO_Pin  = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    /* ③ 把 PA1 这根线接到 EXTI1 上（和按键课里接 EXTI_Line14 一个道理）*/
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);

    /* ④ 配边沿：双边沿 = 变高也进中断，变低也进中断
     *    按键课里用的是 EXTI_Trigger_Falling（只在按下时进） */
    exti.EXTI_Line    = EXTI_Line1;
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    /* ⑤ 告诉 NVIC：这条中断我要用 */
    nvic.NVIC_IRQChannel                   = EXTI1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 1;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}

/*==============================================================================
 *  第 4 块：中断服务函数  —— 对应 6.interrupt 里的按键中断函数
 *  函数名必须叫 EXTI1_IRQHandler（PA1 对应 1 号线），
 *  和你按键课里用 EXTI15_10_IRQHandler 是一个规矩：
 *  名字在启动文件里规定好了，写错就永远进不来。
 *
 *  这里只做两件事：看表、记数。
 *  千万不要在这里面写 printf 或者延时，会把后面的边沿错过。
 *============================================================================*/
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)      /* 确认是这条线触发的 */
    {
        uint16_t xianzai = TIM_GetCounter(TIM2);    /* 看一眼秒表 */

        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == Bit_SET)
        {
            kaishi = xianzai;                       /* ECHO 变高：记起点 */
        }
        else
        {
            kuandu   = (uint16_t)(xianzai - kaishi); /* ECHO 变低：终点-起点 */
            wancheng = 1;                            /* 告诉 main：测完了 */
        }

        EXTI_ClearITPendingBit(EXTI_Line1);          /* 必须清，不清会反复重入 */
    }
}

/*==============================================================================
 *  第 5 块：main  —— 对应 1_project_temple 的骨架
 *============================================================================*/
int main(void)
{
    float juli;              /* 距离，单位厘米 */
    uint16_t t0;

    miaobiao_init();         /* 秒表 */
    chuankou_init();         /* 串口 */
    yinjiao_zhongduan_init();/* 引脚 + 中断 */

    printf("\r\nkaishi ceju...\r\n");

    while (1)
    {
        /* ---- 第一步：喊一声（TRIG 拉高 10us 再拉低）---- */
        wancheng = 0;                                  /* 清掉上一次的结果 */
        GPIO_SetBits(GPIOA, GPIO_Pin_0);
        delay_us(10);
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);

        /* ---- 第二步：等回声（等中断把 wancheng 置 1）---- */
        t0 = TIM_GetCounter(TIM2);
        while (wancheng == 0)
        {
            /* 30ms 还没回来，说明前方没东西或超量程，不傻等 */
            if ((uint16_t)(TIM_GetCounter(TIM2) - t0) > 30000) {
                break;
            }
        }

        /* ---- 第三步：算距离并打印 ---- */
        if (wancheng == 1)
        {
            printf("huibo = %u us\r\n", kuandu);        /* 先打印原始脉宽，方便调试 */
            juli = (float)kuandu * SHENGSU / 2.0f;     /* 距离 = 时间x声速 / 2 */
            printf("juli  = %.2f cm\r\n\r\n", juli);
        }
        else
        {
            printf("chaoshi: meiyou shoudao huibo\r\n\r\n");
        }

        /* ---- 第四步：歇一会儿再测下一次（间隔要大于 60ms）---- */
        delay_us(60000);
    }
}