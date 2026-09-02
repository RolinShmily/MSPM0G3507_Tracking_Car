#ifndef __TICK_H
#define __TICK_H

#include "ti_msp_dl_config.h"

/**
 * @brief 基于 SysTick 滴答定时器中断的毫秒延时函数
 * @param ms 延时的毫秒数
 */
void delay_ms(uint32_t ms);

/**
 * @brief 获取系统当前运行的毫秒数
 * @return 毫秒计数值
 */
uint32_t get_ticks(void);

/**
 * @brief 初始化 1 秒硬件定时器 (使能 NVIC 中断并启动计数)
 */
void Timer_1s_Init(void);

/**
 * @brief 查询 1 秒定时是否到达（读后自动清零）
 * @return 1: 1秒已到达, 0: 未到达
 */
uint8_t Timer_1s_GetFlag(void);

#endif /* __TICK_H */
