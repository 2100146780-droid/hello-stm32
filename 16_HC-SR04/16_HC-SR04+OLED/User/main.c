/*==============================================================================
 *  HC-SR04 超声波测距 —— OLED 显示版
 *
 *  跟 hcsr04_simple.c 的区别：只换了"第 2 块"。
 *     原来：第 2 块 = 串口 + fputc + printf   （显示在电脑上）
 *     现在：第 2 块 = OLED_Init + OLED_ShowXXX（显示在板子上）
 *
 *  好处：不需要串口模块、不需要 fputc、不需要勾 MicroLIB。
 *
 *  接线：  TRIG -> PA0      ECHO -> PA1
 *          OLED  SCL -> PB8  SDA -> PB9   （江协科技 OLED 库的默认接线）
 *
 *  需要你的工程里有 5_OLED 那一课的两个文件： OLED.c  OLED.h
 *  并且把 OLED.c 加入工程（Add Existing Files to Group）。
 *
 *  用到的 OLED 库函数（5_OLED 课里的那套，行/列坐标）：
 *      OLED_Init(void);                                  初始化
 *      OLED_ShowString(行, 列, "字符串");                 行 1~4，列 1~16
 *      OLED_ShowNum(行, 列, 数字, 显示几位);              数字位数不足会补 0
 *============================================================================*/

#include "stm32f10x.h"
#include "OLED.h"
#include <stdio.h>      /* 只为了用 sprintf 拼字符串，不用 %f */

/* 声速：1 微秒声波走 0.0343 厘米（空气 20 度时） */
#define SHENGSU   0.0343f

/*------------------------------------------------------------------------------
 *  三个全局变量（中断里写、main 里读，所以必须 volatile）
 *---------------------------------------------------------------------------*/
volatile uint16_t kaishi   = 0;   /* 上升沿那一刻，秒表的读数     */
volatile uint16_t kuandu   = 0;   /* ECHO 高电平持续了多少微秒   */
volatile uint8_t  wancheng = 0;   /* 1 = 这次测量完成了           */

/*==============================================================================
 *  第 1 块：秒表  —— 对应 7_TIM_LED
 *  72MHz / 72 = 1MHz，计数器每加 1 = 过去了 1 微秒。
 *  ? 别用 7_TIM_LED 里的 PSC=7200-1（那是 100us 一格，测距精度会很差）。
 *============================================================================*/
void miaobiao_init(void)
{
    TIM_TimeBaseInitTypeDef tb;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    tb.TIM_Prescaler         = 72 - 1;   /* 72MHz -> 1MHz */
    tb.TIM_Period            = 0xFFFF;   /* 65535，让它自由地一遍遍数 */
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tb);

    TIM_SetCounter(TIM2, 0);
    TIM_Cmd(TIM2, ENABLE);
}

/* 微秒延时：盯着秒表等够 us 个数（详见前面的讲解） */
void delay_us(uint16_t us)
{
    uint16_t t0 = TIM_GetCounter(TIM2);
    while ((uint16_t)(TIM_GetCounter(TIM2) - t0) < us) {
        /* 空转等待 */
    }
}

/*==============================================================================
 *  第 2 块：显示  —— 对应 5_OLED
 *  原来这里是一堆串口代码，现在只要一句 OLED_Init() 就够了。
 *  （所以我们把这个函数整块删掉了，直接在 main 里初始化）
 *============================================================================*/

/*==============================================================================
 *  第 3 块：引脚 + 中断  —— 对应 2_led + 4_button + 6.interrupt
 *============================================================================*/
void yinjiao_zhongduan_init(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    /* ① 开时钟（AFIO 一定要开，否则 EXTI 挂不上，中断永远不来） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    /* ② PA0 推挽输出 —— 2_led 的写法，用来发触发脉冲 */
    gpio.GPIO_Pin   = GPIO_Pin_0;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);

    /* ② PA1 浮空输入 —— 4_button 的写法，用来读 ECHO 电平 */
    gpio.GPIO_Pin  = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    /* ③ 把 PA1 接到 EXTI1 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);

    /* ④ 双边沿触发：变高进一次，变低再进一次 */
    exti.EXTI_Line    = EXTI_Line1;
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    /* ⑤ 告诉 NVIC 我要用这条中断 */
    nvic.NVIC_IRQChannel                   = EXTI1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 1;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}

/*==============================================================================
 *  第 4 块：中断服务函数  —— 对应 6.interrupt 的按键中断
 *  只做"看表、记数"，绝对不要在这里面刷 OLED（刷屏很慢，会错过边沿）
 *============================================================================*/
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        uint16_t xianzai = TIM_GetCounter(TIM2);

        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == Bit_SET) {
            kaishi = xianzai;                          /* 变高：记起点 */
        } else {
            kuandu   = (uint16_t)(xianzai - kaishi);   /* 变低：算宽度 */
            wancheng = 1;                              /* 通知 main */
        }

        EXTI_ClearITPendingBit(EXTI_Line1);            /* 必须清标志 */
    }
}

/*==============================================================================
 *  第 5 块：main  —— 对应 1_project_temple
 *============================================================================*/
int main(void)
{
    float    juli;
    uint16_t t0;
    uint16_t zhengshu;      /* 距离的整数部分，例如 23 */
    uint16_t xiaoshu;       /* 距离的小数部分，例如 47（表示 .47） */
    char     buf[20];       /* 临时字符串缓冲区 */

    /* ---- 初始化 ---- */
    OLED_Init();                 /* 5_OLED 那一课的初始化 */
    miaobiao_init();             /* 秒表 */
    yinjiao_zhongduan_init();    /* 引脚 + 中断 */

    /* ---- 先画好固定的文字（只画一次，后面只改数字）---- */
    OLED_ShowString(1, 1, "HC-SR04 Ranging");
    OLED_ShowString(2, 1, "Dist:");
    OLED_ShowString(3, 1, "Pulse:");
    OLED_ShowString(3, 12, "us");
    OLED_ShowString(4, 1, "Status:");

    while (1)
    {
        /* ---- ① 喊一声：TRIG 拉高 10us 再拉低 ---- */
        wancheng = 0;
        GPIO_SetBits(GPIOA, GPIO_Pin_0);
        delay_us(10);
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);

        /* ---- ② 等回声 ---- */
        t0 = TIM_GetCounter(TIM2);
        while (wancheng == 0) {
            if ((uint16_t)(TIM_GetCounter(TIM2) - t0) > 30000) {  /* 30ms 超时 */
                break;
            }
        }

        /* ---- ③ 算距离并显示 ---- */
        if (wancheng == 1)
        {
            juli = (float)kuandu * SHENGSU / 2.0f;      /* 距离 = 时间 x 声速 / 2 */

            /* 拆成整数和小数两部分，用整数格式化拼字符串。
               这样就不依赖 %f，也不用管 MicroLIB 的设置。 */
            zhengshu = (uint16_t)juli;
            xiaoshu  = (uint16_t)((juli - (float)zhengshu) * 100.0f + 0.5f);
            if (xiaoshu >= 100) {          /* 像 19.999 四舍五入成 20.00 的情况 */
                zhengshu++;
                xiaoshu = 0;
            }
            sprintf(buf, "%u.%02u cm   ", zhengshu, xiaoshu);   /* 后面几个空格用来盖掉旧内容 */

            OLED_ShowString(2, 6, buf);                 /* 第 2 行显示距离 */
            OLED_ShowNum(3, 7, kuandu, 5);              /* 第 3 行显示原始脉宽 */
            OLED_ShowString(4, 8, "OK     ");
        }
        else
        {
            OLED_ShowString(2, 6, "--.-- cm   ");
            OLED_ShowNum(3, 7, 0, 5);
            OLED_ShowString(4, 8, "Timeout");
        }

        /* ---- ④ 歇一会儿再测下一次（间隔要大于 60ms）---- */
        delay_us(60000);
    }
}