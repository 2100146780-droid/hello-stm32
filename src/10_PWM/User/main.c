/*==============================================================================
 *  HC-SR04 超声波 + 舵机    最终整合版（STM32F103 + 标准外设库）
 *
 *  ┌──────────────────────────────────────────────────────────────────────┐
 *  │  定时器分工（非常重要，不要改）                                       │
 *  │      TIM2_CH3 -> PA2 :  舵机 PWM，50Hz，ARR = 20000-1                │
 *  │      TIM3            :  测距秒表，1MHz，ARR = 65535                  │
 *  │  一个定时器只能有一套 PSC/ARR，所以这两个必须分开！                   │
 *  └──────────────────────────────────────────────────────────────────────┘
 *
 *  接线：
 *        TRIG        -> PA0
 *        ECHO        -> PA1
 *        舵机 橙/黄  -> PA2      （TIM2_CH3）
 *        舵机 红     -> 5V       ★ 必须 5V，不要接 3.3V
 *        舵机 棕/黑  -> GND      ★ 和单片机共地
 *        OLED        -> 引脚在 OLED.c 里定义，跟本文件无关
 *
 *  ★★★ 用之前先做一件事：把工程里这两个文件 Remove 掉 ★★★
 *        PWM.c   /  Motor.c      （它们也会配置 TIM2，会把舵机信号搞乱）
 *      方法：右键文件 -> Remove File from Group
 *      同理，如果你之前的 7_TIM_LED 代码里有 TIM2 的中断配置，也要清掉。
 *
 *  ★ 舵机在 STM32 上还是不动？按这个顺序查：
 *      1. TIM2 有没有被别的文件重复配置？（就是上面那件事）
 *      2. 舵机红线量出来是不是 5V？
 *      3. 单片机 GND 和舵机 GND 通了没有？
 *      4. 信号线是不是真的插在 PA2？
 *      5. 怀疑代码的话，看文件开头的 TEST_SERVO_ONLY 开关
 *============================================================================*/

#include "stm32f10x.h"
#include "OLED.h"
#include <stdio.h>

/*============================ 测试开关 ======================================*/
#define TEST_SERVO_ONLY  1
/*   1 = 只测舵机：开机后舵机自己 0->180->0 来回扫，不测距
 *       ← 先用这个确认"STM32 到底能不能驱动舵机"
 *   0 = 完整功能：测距 + 舵机指针（距离越近，角度越大）
 */
/*============================================================================*/

/*============================ 引脚 =========================================*/
#define TRIG_PORT       GPIOA
#define TRIG_PIN        GPIO_Pin_0

#define ECHO_PORT       GPIOA
#define ECHO_PIN        GPIO_Pin_1
/* 舵机信号固定在 PA2 = TIM2_CH3 */

/*============================ 参数 =========================================*/
#define SHENGSU         0.0343f     /* 声速 cm/us */

#define DUOJI_MIN_US    500         /* 0 度对应的脉宽 */
#define DUOJI_MAX_US    2500        /* 180 度对应的脉宽 */
#define DUOJI_MAX_ANGLE 180

#define JIN_CM          10.0f       /* 这个距离以内 -> 180 度 */
#define YUAN_CM         60.0f       /* 这个距离以外 -> 0 度   */

/*============================ 全局变量 =====================================*/
volatile uint16_t kaishi   = 0;     /* 上升沿时的秒表读数 */
volatile uint16_t kuandu   = 0;     /* ECHO 高电平宽度(us) */
volatile uint8_t  wancheng = 0;     /* 1 = 本次测量完成 */

/*==============================================================================
 *  第 1 块：秒表（TIM3）—— 对应 7_TIM_LED，注意是 TIM3 不是 TIM2
 *  72MHz / 72 = 1MHz，计数器每加 1 = 1 微秒
 *============================================================================*/
void miaobiao_init(void)
{
    TIM_TimeBaseInitTypeDef tb;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);    /* TIM3 ! */

    tb.TIM_Prescaler         = 72 - 1;
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
 *  第 2 块：舵机 PWM（TIM2_CH3 -> PA2）
 *  频率必须 50Hz（周期 20ms），CCR 直接填"微秒数"
 *============================================================================*/
void duoji_pwm_init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef       oc;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    /* TIM2 只给舵机用 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_2;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;      /* 必须复用推挽 */
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    TIM_InternalClockConfig(TIM2);

    tb.TIM_Prescaler         = 72 - 1;      /* 1MHz，1 个数 = 1us */
    tb.TIM_Period            = 20000 - 1;   /* 20ms = 50Hz */
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tb);

    TIM_OCStructInit(&oc);
    oc.TIM_OCMode      = TIM_OCMode_PWM1;       /* 必须 PWM1 */
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;   /* 必须 High */
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 1500;                  /* 1.5ms -> 90 度 */
    TIM_OC3Init(TIM2, &oc);                     /* 通道 3 = PA2 */

    TIM_Cmd(TIM2, ENABLE);
}

/* 设定角度 0~180 度 */
void Servo_SetAngle(uint8_t angle)
{
    uint16_t pulse;

    if (angle > DUOJI_MAX_ANGLE) {
        angle = DUOJI_MAX_ANGLE;
    }

    /* 先转 32 位再乘，避免 16 位平台上溢出 */
    pulse = DUOJI_MIN_US
          + (uint16_t)((uint32_t)angle * (DUOJI_MAX_US - DUOJI_MIN_US)
                       / DUOJI_MAX_ANGLE);

    TIM_SetCompare3(TIM2, pulse);
}

/*==============================================================================
 *  第 3 块：TRIG / ECHO + EXTI1 中断（只在完整功能模式需要）
 *============================================================================*/
#if (TEST_SERVO_ONLY == 0)

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

/* 中断里只记录时间戳（用的是 TIM3） */
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

#endif  /* TEST_SERVO_ONLY == 0 */

/*==============================================================================
 *  main
 *============================================================================*/
int main(void)
{
#if (TEST_SERVO_ONLY == 0)
    float    juli;
    uint16_t t0;
    uint16_t zhengshu;
    uint16_t xiaoshu;
    uint8_t  jiaodu;
#endif
    int16_t  angle;             /* 扫掠用的临时变量 */
    char     buf[20];

    /* ---- 初始化 ---- */
    OLED_Init();
    miaobiao_init();            /* TIM3 秒表 */
    duoji_pwm_init();           /* TIM2 舵机 PWM */

#if (TEST_SERVO_ONLY == 0)
    yinjiao_zhongduan_init();   /* TRIG / ECHO 中断 */
    OLED_ShowString(1, 1, "Ultrasonic Servo");
    OLED_ShowString(2, 1, "Dist:");
    OLED_ShowString(3, 1, "Pulse:");
    OLED_ShowString(3, 12, "us");
    OLED_ShowString(4, 1, "Angle:");
#else
    OLED_ShowString(1, 1, "Servo Test     ");
    OLED_ShowString(2, 1, "Angle:");
    OLED_ShowString(3, 1, "PA2 = TIM2_CH3 ");
    OLED_ShowString(4, 1, "sweep 0 <-> 180");
#endif

    /* ---- 开机先把舵机摆到中间 ---- */
    Servo_SetAngle(90);
    delay_ms(1000);

#if (TEST_SERVO_ONLY == 1)
    /*==================================================================
     *  只测舵机模式：0 <-> 180 平滑来回扫
     *  舵机能跟着扫  -> STM32 的 PWM 完全正常，可以切换 TEST_SERVO_ONLY 为 0
     *  舵机纹丝不动  -> 看文件开头的 5 条排查清单
     *================================================================*/
    while (1)
    {
        for (angle = 0; angle <= 180; angle += 2) {
            Servo_SetAngle((uint8_t)angle);
            sprintf(buf, "%3d", (int)angle);
            OLED_ShowString(2, 7, buf);
            delay_ms(20);
        }

        for (angle = 180; angle >= 0; angle -= 2) {
            Servo_SetAngle((uint8_t)angle);
            sprintf(buf, "%3d", (int)angle);
            OLED_ShowString(2, 7, buf);
            delay_ms(20);
        }
    }

#else
    /*==================================================================
     *  完整功能：测距 + 舵机指针（越近角度越大）
     *================================================================*/
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
            if ((uint16_t)(TIM_GetCounter(TIM3) - t0) > 30000) {   /* 30ms */
                break;
            }
        }

        /* ---- ③ 算距离 -> 角度 ---- */
        if (wancheng == 0)
        {
            jiaodu = 90;                            /* 没测到，回中间 */
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

            /* 距离越近，角度越大 */
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

        /* ---- ④ 转舵机 + 显示 ---- */
        Servo_SetAngle(jiaodu);
        sprintf(buf, "%3d", jiaodu);
        OLED_ShowString(4, 7, buf);
        OLED_ShowString(4, 11, "deg");

        delay_ms(60);       /* 一轮约 100ms，也给余波消散留时间 */
    }
#endif
}