#include "tcrt5000.h"


/* ------------------------------------------------------------------
 * 引脚表:从左到右
 * 注意:用 DO(数字输出),不接 AO;模块 DO 经 LM393 整形,
 *       模块板上一般自带 10k 上拉,这里再开内部上拉更保险。
 * ------------------------------------------------------------------ */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} tcrt_pin_t;

static const tcrt_pin_t s_pin[TCRT_NUM] = {
    { GPIOA, GPIO_Pin_0 },      /* 0 最左 */
    { GPIOA, GPIO_Pin_1 },      /* 1 左   */
    { GPIOA, GPIO_Pin_2 },      /* 2 中   */
    { GPIOA, GPIO_Pin_3 },      /* 3 右   */
    { GPIOA, GPIO_Pin_4 },      /* 4 最右 */
};

/* 权重:负 = 偏左,正 = 偏右,中间 0。乘 10 是为了给 PID 留分辨率 */
static const int16_t s_weight[TCRT_NUM] = { -20, -10, 0, 10, 20 };

static uint8_t s_black[TCRT_NUM];   /* 消抖后的结果 */
static uint8_t s_cand [TCRT_NUM];   /* 上一次的原始结果 */
static uint8_t s_first = 1;         /* 首次采样直接采信 */

static uint8_t s_mask  = 0;
static int16_t s_error = 0;
static uint8_t s_lost  = 1;

/* ------------------------------------------------------------------ */

void TCRT_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    uint16_t mask_a = 0, mask_b = 0, mask_c = 0;
    uint8_t  i;

    /* 按端口分组,日后把某一路挪到别的端口也不用改初始化代码 */
    for (i = 0; i < TCRT_NUM; i++) {
        if      (s_pin[i].port == GPIOA) mask_a |= s_pin[i].pin;
        else if (s_pin[i].port == GPIOB) mask_b |= s_pin[i].pin;
        else                             mask_c |= s_pin[i].pin;
    }

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;      /* 上拉输入 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    if (mask_a) { GPIO_InitStructure.GPIO_Pin = mask_a; GPIO_Init(GPIOA, &GPIO_InitStructure); }
    if (mask_b) { GPIO_InitStructure.GPIO_Pin = mask_b; GPIO_Init(GPIOB, &GPIO_InitStructure); }
    if (mask_c) { GPIO_InitStructure.GPIO_Pin = mask_c; GPIO_Init(GPIOC, &GPIO_InitStructure); }

    s_first = 1;
    s_lost  = 1;
}

uint8_t TCRT_ReadRaw(uint8_t id)
{
    uint8_t level;

    if (id >= TCRT_NUM) {
        return 0;
    }

    level = GPIO_ReadInputDataBit(s_pin[id].port, s_pin[id].pin);

    return (level == TCRT_BLACK_LEVEL) ? 1 : 0;
}

void TCRT_Update(void)
{
    uint8_t i, now, cnt = 0;
    int16_t sum = 0;

    s_mask = 0;

    for (i = 0; i < TCRT_NUM; i++) {
        now = TCRT_ReadRaw(i);

        /* 连续两个周期读数一致才更新,滤掉毛刺(代价是 1 个周期的延迟) */
        if (s_first || now == s_cand[i]) {
            s_black[i] = now;
        }
        s_cand[i] = now;

        if (s_black[i]) {
            s_mask |= (uint8_t)(1u << i);
            sum    += s_weight[i];
            cnt++;
        }
    }

    s_first = 0;

    if (cnt > 0) {
        s_error = (int16_t)(sum / (int16_t)cnt);   /* 黑线质心 */
        s_lost  = 0;
    } else {
        s_lost  = 1;                               /* 丢线,保留上次误差用于找线 */
    }
}

uint8_t TCRT_Get(uint8_t id)
{
    return (id < TCRT_NUM) ? s_black[id] : 0;
}

uint8_t TCRT_GetMask(void)
{
    return s_mask;
}

int16_t TCRT_GetError(void)
{
    return s_error;
}

uint8_t TCRT_IsLost(void)
{
    return s_lost;
}