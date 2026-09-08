#include "Tick.h"
#include "Key.h"
#include "Encoder.h"
#include "SpeedCtrl.h"
#include "Track.h"

/* 滴答定时器累积计数变量 (每 1ms 中断累加 1) */
static volatile uint32_t g_systick_ms = 0;

/* 1 秒硬件定时器中断标志位 */
static volatile uint8_t g_timer_1s_flag = 0;

/**
 * @brief SysTick 1ms 中断服务函数
 */
void SysTick_Handler(void)
{
    static uint8_t s_tick_10ms = 0;

    g_systick_ms++;
    /* 1ms 周期调用按键状态扫描处理函数 */
    Key_Tick_Handler();
    /* 1ms 周期调用编码器处理: 10ms 速度采样, 1s 转速换算 */
    Encoder_Tick_Handler();

    /* 10ms 分频调用循迹处理: 运行在速度闭环控制之前，保证 100Hz 确定性采样与零抖动 */
    if (++s_tick_10ms >= 10) {
        s_tick_10ms = 0;
        Track_Process();
    }
    /* 1ms 周期调用速度闭环处理: 内部 10ms 执行一次增量式 PID */
    SpeedCtrl_Tick_Handler();
}

/**
 * @brief TIMER_0 (TIMG0) 硬件通用定时器中断服务函数
 */
void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST)) {
        case DL_TIMERG_IIDX_ZERO:
            g_timer_1s_flag = 1; /* 置位标志位 */
            break;
        default:
            break;
    }
}

/**
 * @brief 初始化 1 秒硬件定时器 (使能 NVIC 中断并启动 TIMG0 计数)
 */
void Timer_1s_Init(void)
{
    /* 使能 TIMER_0 (TIMG0) 在 NVIC 中的中断响应 */
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);

    /* 启动 TIMG0 定时器计数 */
    DL_TimerG_startCounter(TIMER_0_INST);
}

/**
 * @brief 轮询 1 秒定时是否到达（读取后自动清零）
 * @return 1: 1秒已到达, 0: 未到达
 */
uint8_t Timer_1s_GetFlag(void)
{
    if (g_timer_1s_flag) {
        g_timer_1s_flag = 0; /* 清除标志位 */
        return 1;
    }
    return 0;
}

/**
 * @brief 基于滴答定时器中断的毫秒级延时函数
 * @param ms 需要延时的毫秒数
 */
void delay_ms(uint32_t ms)
{
    uint32_t start_time = g_systick_ms;
    while ((g_systick_ms - start_time) < ms) {
        /* 等待 SysTick 中断累加计数值 */
    }
}

/**
 * @brief 获取系统当前运行的毫秒数
 * @return 毫秒计数值
 */
uint32_t get_ticks(void)
{
    return g_systick_ms;
}
