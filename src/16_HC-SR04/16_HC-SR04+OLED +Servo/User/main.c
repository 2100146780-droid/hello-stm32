/*==============================================================================
 *  HC-SR04 超声波测距 + 舵机  =  指针式测距仪
 *
 *  功能：距离越近，舵机指针角度越大
 *          距离 >= 60cm  -> 0 度
 *          距离 <= 10cm  -> 180 度
 *          中间          -> 线性对应（10cm->180度，60cm->0度）
 *        测不到回波      -> 回到 90 度
 *  上电时会自动 0->90->180->90 扫一遍（自检，证明舵机接线正确）。
 *
 *  ★ 舵机不需要 TB6612！ 它只需要三根线：电源 5V、地 GND、信号线。
 *    TB6612 那块可以完全撤掉，电池也不需要了。
 *
 *  接线：
 *        TRIG  -> PA0
 *        ECHO  -> PA1
 *        舵机： 橙/黄(信号) -> PA2   ← 就是原来 PWMA 那个脚
 *               红(电源)   -> 5V    ← 必须 5V，不要接 3.3V！
 *               棕/黑(地)  -> GND   ← 和单片机共地
 *        OLED： SCL -> PB8       SDA -> PB9
 *
 *  ★ 舵机 PWM 的规则和电机完全不同：
 *        电机看"占空比"（高低电平的比例）决定转速
 *        舵机看"脉冲宽度"（高电平持续多久）决定角度
 *        周期固定 20ms（50Hz），高电平 0.5ms~2.5ms 对应 0~180 度
 *        1.0ms -> 0 度左右，1.5ms -> 90 度，2.0ms -> 180 度左右
 *    （不同舵机略有差异，如果转到头还在"吱吱"响，就是超范围了）
 *
 *  定时器分工：
 *        TIM2_CH3 (PA2) -> 舵机 PWM，50Hz
 *        TIM3           -> 测距秒表，1MHz
 *============================================================================*/

#include "stm32f10x.h"
#include "OLED.h"
#include <stdio.h>

/*------------------------ 引脚 ---------------------------------------------*/
#define TRIG_PORT       GPIOA
#define TRIG_PIN        GPIO_Pin_0

#define ECHO_PORT       GPIOA
#define ECHO_PIN        GPIO_Pin_1
/* 舵机信号固定在 PA2 = TIM2_CH3 */

/*------------------------ 参数 ---------------------------------------------*/
#define SHENGSU         0.0343f     /* 声速 cm/us */
#define JIN_CM          10.0f       /* 这个距离以内 -> 180 度 */
#define YUAN_CM         60.0f       /* 这个距离以外 -> 0 度   */

/* 舵机脉宽范围（微秒）。SG90 一般 500~2500，保守一点可用 600~2400 */
#define DUOJI_MIN_US    500
#define DUOJI_MAX_US    2500

/*------------------------ 全局变量 -----------------------------------------*/
volatile uint16_t kaishi   = 0;
volatile uint16_t kuandu   = 0;
volatile uint8_t  wancheng = 0;

/*==============================================================================
 *  第 1 块：秒表（TIM3）  —— 对应 7_TIM_LED
 *============================================================================*/
void miaobiao_init(void)
{
    TIM_TimeBaseInitTypeDef tb;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    tb.TIM_Prescaler         = 72 - 1;      /* 72MHz -> 1MHz，1 个数 = 1us */
    tb.TIM_Period            = 0xFFFF;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &tb);

    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
}

void delay_us(uint16_t us)
{
    uint16_t t0 = TIM_GetCounter(TIM3);
    while ((uint16_t)(TIM_GetCounter(TIM3) - t0) < us) {
        /* 空转等待 */
    }
}

void delay_ms(uint16_t ms)
{
    while (ms--) {
        delay_us(1000);
    }
}

/*==============================================================================
 *  第 2 块：舵机 PWM（TIM2_CH3 -> PA2）  —— 对应 10_PWM，但参数完全不同
 *  72MHz / 72 = 1MHz  ->  1 个数 = 1us
 *  数 20000 个 = 20000us = 20ms  ->  正好 50Hz
 *  所以 CCR 直接填"微秒数"：CCR=1500 就是 1.5ms，对应 90 度
 *============================================================================*/
void duoji_pwm_init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef       oc;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA2 复用推挽输出 */
    gpio.GPIO_Pin   = GPIO_Pin_2;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    TIM_InternalClockConfig(TIM2);

    tb.TIM_Prescaler         = 72 - 1;      /* 1MHz */
    tb.TIM_Period            = 20000 - 1;   /* 20ms = 50Hz */
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tb);

    TIM_OCStructInit(&oc);
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 1500;              /* 1.5ms -> 先摆到 90 度 */
    TIM_OC3Init(TIM2, &oc);                 /* 通道 3 = PA2 */

    TIM_Cmd(TIM2, ENABLE);
}

/* 设定角度：0 ~ 180 度
   把角度换算成脉宽（500us~2500us），再写进 CCR。
   注意先把 angle 转成 32 位再乘，不然 180*2000 超出 16 位会算错。 */
void duoji_set_angle(uint8_t angle)
{
    uint16_t ccr;

    if (angle > 180) angle = 180;

    ccr = DUOJI_MIN_US
        + (uint16_t)((uint32_t)angle * (DUOJI_MAX_US - DUOJI_MIN_US) / 180);

    TIM_SetCompare3(TIM2, ccr);
}

/*==============================================================================
 *  第 3 块：引脚 + 中断（TRIG / ECHO）  —— 对应 2_led + 4_button + 6.interrupt
 *============================================================================*/
void yinjiao_zhongduan_init(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    gpio.GPIO_Pin   = TRIG_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TRIG_PORT, &gpio);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

    gpio.GPIO_Pin  = ECHO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(ECHO_PORT, &gpio);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);

    exti.EXTI_Line    = EXTI_Line1;
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    nvic.NVIC_IRQChannel                   = EXTI1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 1;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}

/*==============================================================================
 *  第 4 块：中断服务函数  —— 对应 6.interrupt
 *============================================================================*/
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        uint16_t xianzai = TIM_GetCounter(TIM3);

        if (GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) == Bit_SET) {
            kaishi = xianzai;
        } else {
            kuandu   = (uint16_t)(xianzai - kaishi);
            wancheng = 1;
        }

        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}

/*==============================================================================
 *  第 5 块：main
 *============================================================================*/
int main(void)
{
    float    juli;
    uint16_t t0;
    uint16_t zhengshu;
    uint16_t xiaoshu;
    uint8_t  jiaodu;
    char     buf[20];

    /* ---- 初始化 ---- */
    OLED_Init();
    miaobiao_init();            /* TIM3 秒表 */
    duoji_pwm_init();           /* TIM2 舵机 PWM */
    yinjiao_zhongduan_init();   /* TRIG / ECHO 中断 */

    OLED_ShowString(1, 1, "Ultrasonic Servo");
    OLED_ShowString(2, 1, "Dist:");
    OLED_ShowString(3, 1, "Pulse:");
    OLED_ShowString(3, 12, "us");
    OLED_ShowString(4, 1, "Angle:");

    /* ---- 开机自检：让舵机扫一遍 ----
       这一步不依赖超声波。如果舵机能从左边转到中间再转到右边，
       说明舵机的电源、地、信号线全部接对了。 */
    OLED_ShowString(2, 6, "selftest...  ");
    duoji_set_angle(0);     delay_ms(800);
    duoji_set_angle(90);    delay_ms(800);
    duoji_set_angle(180);   delay_ms(800);
    duoji_set_angle(90);    delay_ms(800);

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
            if ((uint16_t)(TIM_GetCounter(TIM3) - t0) > 30000) {
                break;
            }
        }

        /* ---- ③ 算距离，换算成角度 ---- */
        if (wancheng == 0)
        {
            jiaodu = 90;                            /* 没测到，回到中间 */
            OLED_ShowString(2, 6, "--.-- cm   ");
            OLED_ShowNum(3, 7, 0, 5);
        }
        else
        {
            juli = (float)kuandu * SHENGSU / 2.0f;

            zhengshu = (uint16_t)juli;
            xiaoshu  = (uint16_t)((juli - (float)zhengshu) * 100.0f + 0.5f);
            if (xiaoshu >= 100) { zhengshu++; xiaoshu = 0; }
            sprintf(buf, "%u.%02u cm   ", zhengshu, xiaoshu);
            OLED_ShowString(2, 6, buf);
            OLED_ShowNum(3, 7, kuandu, 5);

            /* 距离 -> 角度：越近角度越大 */
            if (juli <= JIN_CM) {
                jiaodu = 180;
            }
            else if (juli >= YUAN_CM) {
                jiaodu = 0;
            }
            else {
                jiaodu = (uint8_t)(180.0f
                       - (juli - JIN_CM) * 180.0f / (YUAN_CM - JIN_CM));
            }
        }

        /* ---- ④ 转舵机 + 显示角度 ---- */
        duoji_set_angle(jiaodu);
        sprintf(buf, "%3d", jiaodu);
        OLED_ShowString(4, 7, buf);
        OLED_ShowString(4, 11, "deg");

        /* 舵机转动要一点时间，也给超声波留出余波消散的时间 */
        delay_ms(60);
    }
}