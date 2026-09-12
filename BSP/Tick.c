#include "Tick.h"
#include "Key.h"

/* 滴答定时器毫秒计数变量 (每 1ms 中断累加 1) */
static volatile uint32_t g_systick_ms = 0;

/**
 * @brief SysTick 1ms 中断服务函数
 */
void SysTick_Handler(void)
{
    g_systick_ms++;
    /* 1ms 周期调用按键状态机处理函数 */
    Key_Tick_Handler();
}

/**
 * @brief 基于滴答定时器中断的毫秒延时函数
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
