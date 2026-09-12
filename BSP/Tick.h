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

#endif /* __TICK_H */
