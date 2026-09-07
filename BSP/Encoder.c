#include "Encoder.h"

/* 编码器脉冲计数器 (GROUP1 中断中更新) */
static volatile int32_t Encode_CNT_RL = 0;   /* 左轮: OB3 触发计数, OA3 判向 */
static volatile int32_t Encode_CNT_RR = 0;   /* 右轮: OA4 触发计数, OB4 判向 */

/* 左右轮 1s 窗口转速 (RPM, SysTick 上下文中计算) */
static volatile int32_t g_rl_speed_rpm = 0;
static volatile int32_t g_rr_speed_rpm = 0;

/* 左右轮最近一个 10ms 采样窗的脉冲增量 (PID 快速反馈源) */
static volatile int32_t g_l_delta10ms = 0;
static volatile int32_t g_r_delta10ms = 0;

/**
 * @brief 编码器模块初始化: 清零计数器, 使能 GPIO 中断
 */
void Encoder_Init(void)
{
    /* 清零脉冲计数器 */
    Encode_CNT_RL = 0;
    Encode_CNT_RR = 0;

    /* OA3/OA4 位于 GPIOA, OB3/OB4 位于 GPIOB, 共享 GROUP1 中断 */
    NVIC_EnableIRQ(Encoder_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(Encoder_GPIOB_INT_IRQN);
}

/**
 * @brief 外部中断模拟编码器信号 (GROUP1: GPIOA + GPIOB)
 *        左轮: OB3 上升沿触发, 读 OA3 电平判向
 *        右轮: OA4 上升沿触发, 读 OB4 电平判向
 *        注: 正转时两轮计数均递增 (符号与 OLED/串口显示方向一致)
 */
void GROUP1_IRQHandler(void)
{
    /* 读取触发的 GPIOA 和 GPIOB 中断状态 */
    uint32_t PortA_interrupt = DL_GPIO_getRawInterruptStatus(Encoder_OA4_PORT, Encoder_OA4_PIN);
    uint32_t PortB_interrupt = DL_GPIO_getRawInterruptStatus(Encoder_OB3_PORT, Encoder_OB3_PIN);

    /* 处理左轮 (OB3 触发中断) */
    if (PortB_interrupt & Encoder_OB3_PIN)
    {
        if (DL_GPIO_readPins(Encoder_OA3_PORT, Encoder_OA3_PIN) & Encoder_OA3_PIN)
            Encode_CNT_RL++;    /* 正转 */
        else
            Encode_CNT_RL--;    /* 反转 */
    }

    /* 处理右轮 (OA4 触发中断) */
    if (PortA_interrupt & Encoder_OA4_PIN)
    {
        if (DL_GPIO_readPins(Encoder_OB4_PORT, Encoder_OB4_PIN) & Encoder_OB4_PIN)
            Encode_CNT_RR++;    /* 正转 */
        else
            Encode_CNT_RR--;    /* 反转 */
    }

    /* 清除编码器两个端口所有已触发的中断标志 */
    DL_GPIO_clearInterruptStatus(Encoder_OA3_PORT, Encoder_OA3_PIN | Encoder_OA4_PIN);
    DL_GPIO_clearInterruptStatus(Encoder_OB3_PORT, Encoder_OB3_PIN | Encoder_OB4_PIN);
}

/**
 * @brief 编码器周期处理 (由 SysTick_Handler 每 1ms 调用)
 *        - 每 10ms: 统计一次脉冲增量 (速度采样)
 *        - 每 1s : 将累计脉冲换算为输出轴转速
 *          RPM = 每秒脉冲数 * 60 / (减速比 * PPR)
 */
void Encoder_Tick_Handler(void)
{
    static uint16_t ms_cnt = 0;        /* 1ms -> 10ms 分频 */
    static uint16_t sample_cnt = 0;    /* 10ms -> 1s 分频 */
    static int32_t last_rl = 0, last_rr = 0;
    static int32_t acc_rl = 0, acc_rr = 0;

    if (++ms_cnt < ENCODER_SAMPLE_MS) {
        return;
    }
    ms_cnt = 0;

    /* 累计本 10ms 窗口内的脉冲增量 */
    int32_t now_rl = Encode_CNT_RL;
    int32_t now_rr = Encode_CNT_RR;
    g_l_delta10ms = now_rl - last_rl;
    g_r_delta10ms = now_rr - last_rr;
    acc_rl += g_l_delta10ms;
    acc_rr += g_r_delta10ms;
    last_rl = now_rl;
    last_rr = now_rr;

    /* 每 1s 换算一次转速 (带符号, 符号即方向) */
    if (++sample_cnt >= ENCODER_SAMPLES_PER_SEC) {
        sample_cnt = 0;
        g_rl_speed_rpm = (acc_rl * 60) / (ENCODER_GEAR_RATIO * ENCODER_PPR);
        g_rr_speed_rpm = (acc_rr * 60) / (ENCODER_GEAR_RATIO * ENCODER_PPR);
        acc_rl = 0;
        acc_rr = 0;
    }
}

/**
 * @brief 获取左轮 1s 窗口转速 (RPM, 带符号)
 */
int Encoder_GetLRPM(void)
{
    return g_rl_speed_rpm;
}

/**
 * @brief 获取右轮 1s 窗口转速 (RPM, 带符号)
 */
int Encoder_GetRRPM(void)
{
    return g_rr_speed_rpm;
}

/**
 * @brief 获取左轮最近一个 10ms 采样窗的脉冲增量 (带符号)
 */
int32_t Encoder_GetL10msDelta(void)
{
    return g_l_delta10ms;
}

/**
 * @brief 获取右轮最近一个 10ms 采样窗的脉冲增量 (带符号)
 */
int32_t Encoder_GetR10msDelta(void)
{
    return g_r_delta10ms;
}
