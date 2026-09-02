#ifndef __PWM_H
#define __PWM_H

#include "ti_msp_dl_config.h"

/* PWM 周期最大值 (与 SysConfig 中配置的 Period = 1000 对应) */
#define PWM_PERIOD_MAX       1000

/**
 * @brief PWM 初始化函数（使能并启动 PWM_LED 定时器）
 */
void PWM_Init(void);

/**
 * @brief 设置 PWM 比较寄存器值 (CCR)
 * @param compare_value 比较值 (0 ~ PWM_PERIOD_MAX)
 */
void PWM_Set_CompareValue(uint32_t compare_value);

/**
 * @brief 设置 PWM 占空比 (0 ~ 100%)
 * @param duty 占空比百分比 (0 ~ 100)
 */
void PWM_Set_Duty(uint32_t duty);

#endif /* __PWM_H */
