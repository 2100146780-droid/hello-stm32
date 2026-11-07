/*==============================================================================
 *  HC-SR04 超声波测距 + TB6612 直流电机（带小扇叶） = 智能风扇
 *
 *  功能：一边测距一边吹风
 *          距离 >= 40cm   -> 全速吹（占空比 100%）
 *        20cm ~ 40cm      -> 中速  （占空比 75%）
 *          距离 < 20cm    -> 慢吹  （占空比 50%）   ← 你要的效果
 *        测不到回波       -> 全速（前方空旷）
 *  同时把距离、脉宽、当前速度显示在 OLED 上。
 *
 *  ★ 关键：秒表在 TIM3，电机 PWM 在 TIM2，两个定时器分工不打架
 *       TIM2 -> 电机 PWM（PSC=720-1, ARR=100-1，1kHz）
 *       TIM3 -> 秒表    （PSC=72-1,  ARR=65535，1 格 = 1us）
 *
 *  接线：
 *        TRIG -> PA0             ECHO -> PA1
 *        TB6612： PWMA -> PA2 (TIM2_CH3，固定)
 *                 AIN1 -> PA4
 *                 AIN2 -> PA5
 *                 STBY -> 3.3V    （不接高电平，电机死活不转）
 *        OLED：   SCL -> PB8     SDA -> PB9
 *
 *  电源： 小扇叶虽然电流小，但仍建议电机单独用电池供电，
 *         并且电池的 GND 必须和单片机 GND 接在一起（共地）。
 *
 *  工程里需要： OLED.c / OLED.h （5_OLED 那课的文件）
 *============================================================================*/

#include "stm32f10x.h"
#include "OLED.h"
#include <stdio.h>          /* 只用整数 sprintf，不用 %f */

/*------------------------ 引脚（按你的接线改这里）--------------------------*/
#define TRIG_PORT       GPIOA
#define TRIG_PIN        GPIO_Pin_0

#define ECHO_PORT       GPIOA
#define ECHO_PIN        GPIO_Pin_1

#define AIN1_PORT       GPIOA
#define AIN1_PIN        GPIO_Pin_4
#define AIN2_PORT       GPIOA
#define AIN2_PIN        GPIO_Pin_5
/* PWMA 固定是 PA2 = TIM2_CH3 */

/*------------------------ 阈值与速度 ---------------------------------------*/
#define SHENGSU         0.0343f     /* 声速 cm/us */
#define TAIJIN_CM       20.0f       /* 小于这个距离 = 太近 -> 慢吹 */
#define ZHONG_CM        40.0f       /* 20~40cm = 中等 */

#define SPEED_JIN       50          /* 太近：占空比 50%  ← 你要的 */
#define SPEED_ZHONG     75          /* 中等：75% */
#define SPEED_YUAN      100         /* 远  ：全速 */

/*------------------------ 全局变量（中断与 main 共享）----------------------*/
volatile uint16_t kaishi   = 0;     /* 上升沿时的秒表读数 */
volatile uint16_t kuandu   = 0;     /* ECHO 高电平宽度(us) */
volatile uint8_t  wancheng = 0;     /* 1 = 本次测量完成 */

/*==============================================================================
 *  第 1 块：秒表  —— 对应 7_TIM_LED（定时器换成了 TIM3）
 *============================================================================*/
void miaobiao_init(void)
{
    TIM_TimeBaseInitTypeDef tb;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);   /* 是 TIM3 */

    tb.TIM_Prescaler         = 72 - 1;      /* 72MHz -> 1MHz，1 个数 = 1us */
    tb.TIM_Period            = 0xFFFF;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &tb);

    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
}

/* 微秒延时：盯着 TIM3 秒表等够 us 个数 */
void delay_us(uint16_t us)
{
    uint16_t t0 = TIM_GetCounter(TIM3);
    while ((uint16_t)(TIM_GetCounter(TIM3) - t0) < us) {
        /* 空转等待 */
    }
}

/*==============================================================================
 *  第 2 块：电机 PWM  —— 对应 10_PWM
 *  TIM2_CH3 输出方波到 PA2，改"占空比"就是改转速。
 *  72MHz / 720 = 100kHz，再数 100 个 = 1kHz 的 PWM。
 *  （如果电机有啸叫声，把 PSC 改成 72-1、ARR 保持 100-1，
 *    频率就变成 10kHz，人耳听着安静很多。）
 *============================================================================*/
void dianji_pwm_init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef       oc;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA2 复用推挽输出（10_PWM 课的 LED 用的是 PA0，这里必须换成 PA2，
       因为 PA0 已经给 TRIG 用了） */
    gpio.GPIO_Pin   = GPIO_Pin_2;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    TIM_InternalClockConfig(TIM2);

    tb.TIM_Prescaler         = 720 - 1;     /* PSC */
    tb.TIM_Period            = 100 - 1;     /* ARR：占空比范围 0~100 */
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tb);

    TIM_OCStructInit(&oc);                  /* 先把结构体填默认值 */
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 0;                 /* CCR = 0，上电先不转 */
    TIM_OC3Init(TIM2, &oc);                 /* 通道 3 = PA2 */

    TIM_Cmd(TIM2, ENABLE);
}

/*==============================================================================
 *  第 3 块：电机方向 + 调速  —— 对应 12_Motor
 *  风扇只需要正转，所以 AIN1 拉高、AIN2 拉低，一直保持不动。
 *============================================================================*/
void dianji_fangxiang_init(void)
{
    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin   = AIN1_PIN | AIN2_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(AIN1_PORT, &gpio);

    GPIO_SetBits(AIN1_PORT, AIN1_PIN);       /* AIN1 = 1 */
    GPIO_ResetBits(AIN2_PORT, AIN2_PIN);     /* AIN2 = 0 -> 正转 */
}

/* 调速：speed 取 0~100（0 = 停，100 = 全速）
   风扇不用倒转，所以这里只给正数；以后想让它"吸气"就传负数 */
void dianji_setspeed(int16_t speed)
{
    if (speed > 100)  speed = 100;
    if (speed < -100) speed = -100;

    if (speed >= 0) {
        GPIO_SetBits(AIN1_PORT, AIN1_PIN);
        GPIO_ResetBits(AIN2_PORT, AIN2_PIN);
        TIM_SetCompare3(TIM2, (uint16_t)speed);
    } else {
        GPIO_ResetBits(AIN1_PORT, AIN1_PIN);
        GPIO_SetBits(AIN2_PORT, AIN2_PIN);
        TIM_SetCompare3(TIM2, (uint16_t)(-speed));
    }
}

/*==============================================================================
 *  第 4 块：引脚 + 中断  —— 对应 2_led + 4_button + 6.interrupt
 *============================================================================*/
void yinjiao_zhongduan_init(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    /* TRIG：推挽输出 */
    gpio.GPIO_Pin   = TRIG_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TRIG_PORT, &gpio);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

    /* ECHO：浮空输入 */
    gpio.GPIO_Pin  = ECHO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(ECHO_PORT, &gpio);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);

    exti.EXTI_Line    = EXTI_Line1;
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;   /* 双边沿 */
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    nvic.NVIC_IRQChannel                   = EXTI1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 1;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}

/*==============================================================================
 *  第 5 块：中断服务函数  —— 对应 6.interrupt
 *  只记录时间戳，不要在这里面刷屏或开电机
 *============================================================================*/
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        uint16_t xianzai = TIM_GetCounter(TIM3);       /* 是 TIM3 */

        if (GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) == Bit_SET) {
            kaishi = xianzai;                          /* 上升沿：记起点 */
        } else {
            kuandu   = (uint16_t)(xianzai - kaishi);   /* 下降沿：算宽度 */
            wancheng = 1;
        }

        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}

/*==============================================================================
 *  第 6 块：main
 *============================================================================*/
int main(void)
{
    float    juli;
    uint16_t t0;
    uint16_t zhengshu;
    uint16_t xiaoshu;
    int16_t  mubiao_sudu;       /* 这一轮该用多少速度（0~100） */
    char     buf[20];

    /* ---- 初始化 ---- */
    OLED_Init();
    miaobiao_init();            /* TIM3 秒表 */
    dianji_pwm_init();          /* TIM2 电机 PWM */
    dianji_fangxiang_init();    /* 电机方向 */
    yinjiao_zhongduan_init();   /* TRIG / ECHO 中断 */

    /* ---- 屏幕上的固定文字，只写一次 ---- */
    OLED_ShowString(1, 1, "Ultrasonic Fan");
    OLED_ShowString(2, 1, "Dist:");
    OLED_ShowString(3, 1, "Pulse:");
    OLED_ShowString(3, 12, "us");
    OLED_ShowString(4, 1, "Speed:");

    while (1)
    {
        /* ---- ① 喊一声 ---- */
        wancheng = 0;
        GPIO_SetBits(TRIG_PORT, TRIG_PIN);
        delay_us(10);
        GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

        /* ---- ② 等回声 ---- */
        t0 = TIM_GetCounter(TIM3);
        while (wancheng == 0) {
            if ((uint16_t)(TIM_GetCounter(TIM3) - t0) > 30000) {   /* 30ms 超时 */
                break;
            }
        }

        /* ---- ③ 按距离决定风扇转速 ---- */
        if (wancheng == 0)
        {
            /* 没测到回波 = 前方很空旷，让风扇全速吹 */
            mubiao_sudu = SPEED_YUAN;
            OLED_ShowString(2, 6, "--.-- cm   ");
            OLED_ShowNum(3, 7, 0, 5);
        }
        else
        {
            juli = (float)kuandu * SHENGSU / 2.0f;

            /* 距离显示：拆成整数 + 小数，避开 %f */
            zhengshu = (uint16_t)juli;
            xiaoshu  = (uint16_t)((juli - (float)zhengshu) * 100.0f + 0.5f);
            if (xiaoshu >= 100) { zhengshu++; xiaoshu = 0; }
            sprintf(buf, "%u.%02u cm   ", zhengshu, xiaoshu);
            OLED_ShowString(2, 6, buf);
            OLED_ShowNum(3, 7, kuandu, 5);

            if (juli < TAIJIN_CM) {
                mubiao_sudu = SPEED_JIN;        /* 太近 -> 占空比 50% */
            }
            else if (juli < ZHONG_CM) {
                mubiao_sudu = SPEED_ZHONG;      /* 中等 -> 75% */
            }
            else {
                mubiao_sudu = SPEED_YUAN;       /* 远 -> 100% */
            }
        }

        /* ---- ④ 调速 + 显示 ---- */
        dianji_setspeed(mubiao_sudu);
        sprintf(buf, "%3d%%   ", mubiao_sudu);   /* 注意 %% 才是打印一个百分号 */
        OLED_ShowString(4, 7, buf);

        delay_us(60000);        /* 100ms 一轮（要 >60ms，避免余波干扰） */
    }
}