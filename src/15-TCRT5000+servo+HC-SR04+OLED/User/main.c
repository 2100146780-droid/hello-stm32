/*==============================================================================
 *  三合一整合项目：TCRT5000 + HC-SR04 + 舵机
 *  = 循迹 / 避障 / 指示 的三个传感器合体（STM32F103 + 标准外设库）
 *
 *  ┌─── 整体逻辑（舵机角度 = "危险程度指针"）────────────────────────┐
 *  │   优先级 1：TCRT5000 压在黑线上     -> 舵机 180 度（最右，警告） │
 *  │   优先级 2：超声波距离 < 10cm       -> 舵机 180 度               │
 *  │   优先级 3：距离 10~60cm            -> 角度线性 180 -> 0 度      │
 *  │   优先级 4：距离 > 60cm 或测不到     -> 舵机 0 度（最左，安全）   │
 *  └──────────────────────────────────────────────────────────────────┘
 *
 *  ┌─── 定时器/外设分配（不要改，改了会打架）────────────────────────┐
 *  │   TIM2_CH3 -> PA2 : 舵机 PWM，50Hz，ARR = 20000-1               │
 *  │   TIM3            : 测距秒表，1MHz，ARR = 65535                  │
 *  │   ADC            : 本文件没用（要读 AO 的话看 tcrt5000.c）       │
 *  └──────────────────────────────────────────────────────────────────┘
 *
 *  接线：
 *      HC-SR04： TRIG -> PA0        ECHO -> PA1
 *      舵机   ： 橙/黄(信号) -> PA2
 *                红(电源)    -> 独立 5V（★ 千万别接 STM32 板载 5V）
 *                棕/黑(地)   -> GND（和单片机、电源共地）
 *      TCRT5000：VCC -> 3.3V        GND -> GND
 *                DO  -> PA4         （AO 不用接，本文件不读模拟量）
 *      OLED   ： SCL -> PB8         SDA -> PB9
 *
 *  ★★★ 舵机供电的坑（实测过）★★★
 *      接 STM32 板子上的 5V 排针 -> 只会"吱吱响但不转"（电流不够）。
 *      必须用独立 5V：充电宝 / 5V2A 充电头 / 一块空闲的 Uno 的 5V 排针，
 *      并且这个电源的 GND 要和 STM32 的 GND 连起来。
 *
 *  ★ 用之前把 PWM.c / Motor.c 从工程里 Remove 掉（它们也配置 TIM2）
 *============================================================================*/

#include "stm32f10x.h"
#include "OLED.h"
#include <stdio.h>

/*============================ 测试开关 ======================================*/
#define TEST_MODE   0
/*  0 = 完整联动：黑线优先，其次按距离映射角度
 *  1 = 只测传感器：舵机固定 90 度不动，OLED 实时显示两个传感器读数
 *      ← 第一次接线先用这个，确认两个传感器都读得对
 */
/*============================================================================*/

/*============================ 引脚 =========================================*/
#define TRIG_PORT       GPIOA
#define TRIG_PIN        GPIO_Pin_0

#define ECHO_PORT       GPIOA
#define ECHO_PIN        GPIO_Pin_1
/* 舵机信号固定在 PA2 = TIM2_CH3 */

#define LINE_PORT       GPIOA
#define LINE_PIN        GPIO_Pin_4          /* TCRT5000 的 DO */

/*============================ 参数 =========================================*/
#define SHENGSU         0.0343f     /* 声速 cm/us */

#define DUOJI_MIN_US    500         /* 0 度 */
#define DUOJI_MAX_US    2500        /* 180 度 */
#define DUOJI_MAX_ANGLE 180

#define JIN_CM          10.0f       /* 这个距离以内 -> 180 度 */
#define YUAN_CM         60.0f       /* 这个距离以外 -> 0 度   */

/* TCRT5000 的 DO 在"检测到"时是高还是低？
 * 大多数模块（对着白纸/近物时）DO = 0，指示灯亮。
 * 拿不准就先烧 TEST_MODE = 1，对着白纸和黑线各看一次再定。 */
#define LINE_ACTIVE_LEVEL 0

/*============================ 全局变量 =====================================*/
volatile uint16_t kaishi   = 0;
volatile uint16_t kuandu   = 0;
volatile uint8_t  wancheng = 0;

/*==============================================================================
 *  第 1 块：秒表（TIM3）+ 延时
 *============================================================================*/
void miaobiao_init(void)
{
    TIM_TimeBaseInitTypeDef tb;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);    /* 是 TIM3 */

    tb.TIM_Prescaler         = 72 - 1;      /* 1MHz，1 个数 = 1us */
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
        /* 空转 */
    }
}

void delay_ms(uint16_t ms)
{
    while (ms--) {
        delay_us(1000);
    }
}

/*==============================================================================
 *  第 2 块：舵机 PWM（TIM2_CH3 -> PA2），50Hz
 *============================================================================*/
void duoji_pwm_init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef       oc;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_2;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    TIM_InternalClockConfig(TIM2);

    tb.TIM_Prescaler         = 72 - 1;      /* 1us 一格 */
    tb.TIM_Period            = 20000 - 1;   /* 20ms = 50Hz */
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tb);

    TIM_OCStructInit(&oc);
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 1500;
    TIM_OC3Init(TIM2, &oc);

    TIM_Cmd(TIM2, ENABLE);
}

void Servo_SetAngle(uint8_t angle)
{
    uint16_t pulse;

    if (angle > DUOJI_MAX_ANGLE) {
        angle = DUOJI_MAX_ANGLE;
    }

    pulse = DUOJI_MIN_US
          + (uint16_t)((uint32_t)angle * (DUOJI_MAX_US - DUOJI_MIN_US)
                       / DUOJI_MAX_ANGLE);

    TIM_SetCompare3(TIM2, pulse);
}

/*==============================================================================
 *  第 3 块：超声波 TRIG / ECHO + EXTI1
 *============================================================================*/
void chaoshengbo_init(void)
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
 *  第 4 块：TCRT5000 的黑线检测（DO 数字输入）
 *  对应 4_button 那一课的"读引脚"，
 *  注意要配成上拉输入：模块里的 LM393 是开漏输出，需要上拉。
 *============================================================================*/
void tcrt5000_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin   = LINE_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;        /* 上拉输入 */
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LINE_PORT, &gpio);
}

/* 读原始电平，返回 0 或 1 */
uint8_t line_raw(void)
{
    return (GPIO_ReadInputDataBit(LINE_PORT, LINE_PIN) == Bit_SET) ? 1 : 0;
}

/* 判断"是否检测到黑线"：1 = 检测到 */
uint8_t line_detected(void)
{
    return (line_raw() == LINE_ACTIVE_LEVEL) ? 1 : 0;
}

/* 带确认的版本：连续 3 次都检测到才算数，避免抖动误判 */
uint8_t line_detected_stable(void)
{
    uint8_t i;

    for (i = 0; i < 3; i++) {
        if (!line_detected()) {
            return 0;
        }
        delay_ms(1);
    }

    return 1;
}

/*==============================================================================
 *  第 5 块：测一次距离（返回 cm，-1 表示超时）
 *============================================================================*/
float chaoshengbo_measure(void)
{
    uint16_t t0;

    wancheng = 0;

    GPIO_SetBits(TRIG_PORT, TRIG_PIN);
    delay_us(10);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

    t0 = TIM_GetCounter(TIM3);
    while (wancheng == 0) {
        if ((uint16_t)(TIM_GetCounter(TIM3) - t0) > 30000) {    /* 30ms */
            return -1.0f;
        }
    }

    return (float)kuandu * SHENGSU / 2.0f;
}

/*==============================================================================
 *  main
 *============================================================================*/
int main(void)
{
    float    juli;
    uint8_t  line;
    uint8_t  jiaodu;
    uint16_t zhengshu;
    uint16_t xiaoshu;
    char     buf[20];

    /* ---- 初始化 ---- */
    OLED_Init();
    miaobiao_init();        /* TIM3 秒表 */
    duoji_pwm_init();       /* TIM2 舵机 */
    chaoshengbo_init();     /* TRIG / ECHO 中断 */
    tcrt5000_init();        /* 黑线检测 */

    OLED_ShowString(1, 1, "LineFollow+Ultra");
    OLED_ShowString(2, 1, "Dist:");
    OLED_ShowString(3, 1, "Line:");
    OLED_ShowString(4, 1, "Angle:");

    Servo_SetAngle(90);
    delay_ms(800);

    while (1)
    {
        /* ---- ① 读两个传感器 ---- */
        line = line_detected_stable();
        juli = chaoshengbo_measure();

        /* ---- ② 距离显示 ---- */
        if (juli < 0.0f) {
            OLED_ShowString(2, 6, "--.-- cm   ");
        } else {
            zhengshu = (uint16_t)juli;
            xiaoshu  = (uint16_t)((juli - (float)zhengshu) * 100.0f + 0.5f);
            if (xiaoshu >= 100) { zhengshu++; xiaoshu = 0; }
            sprintf(buf, "%u.%02u cm   ", zhengshu, xiaoshu);
            OLED_ShowString(2, 6, buf);
        }

        /* ---- ③ 黑线状态 + 脉宽显示 ---- */
        sprintf(buf, "%s %4u us", line ? "YES" : "NO ", kuandu);
        OLED_ShowString(3, 6, buf);

#if (TEST_MODE == 1)
        /*--------------------------------------------------------------
         *  只测传感器模式：舵机不动，专心看两个传感器读数对不对
         *      把 TCRT5000 分别对着白纸和黑线，看 Line 变不变
         *      把手在超声波前面晃，看 Dist 变不变
         *-------------------------------------------------------------*/
        Servo_SetAngle(90);
        OLED_ShowString(4, 7, " 90");
        OLED_ShowString(4, 11, "test");
#else
        /*--------------------------------------------------------------
         *  完整联动
         *-------------------------------------------------------------*/
        if (line) {
            /* 黑线优先：转到"警告"位置 */
            jiaodu = 180;
        }
        else if (juli < 0.0f) {
            /* 没测到障碍：安全位置 */
            jiaodu = 0;
        }
        else if (juli <= JIN_CM) {
            jiaodu = 180;
        }
        else if (juli >= YUAN_CM) {
            jiaodu = 0;
        }
        else {
            /* 10cm -> 180 度，60cm -> 0 度，中间线性 */
            jiaodu = (uint8_t)(180.0f
                   - (juli - JIN_CM) * 180.0f / (YUAN_CM - JIN_CM));
        }

        Servo_SetAngle(jiaodu);
        sprintf(buf, "%3d", jiaodu);
        OLED_ShowString(4, 7, buf);
        OLED_ShowString(4, 11, "deg");
#endif

        delay_ms(60);       /* 一轮约 100ms，也给超声波余波消散留时间 */
    }
}