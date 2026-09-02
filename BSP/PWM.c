#include "PWM.h"

/**
 * @brief PWM 初始化函数
 */
void PWM_Init(void)
{
    /* 启动 PWM_LED (TIMG6) 定时器计数 */
    DL_TimerG_startCounter(PWM_LED_INST);
}

/**
 * @brief 设置 PWM 比较寄存器值 (CCR)
 * @param compare_value 比较值 (0 ~ PWM_PERIOD_MAX)
 */
void PWM_Set_CompareValue(uint32_t compare_value)
{
    if (compare_value > PWM_PERIOD_MAX) {
        compare_value = PWM_PERIOD_MAX;
    }
    DL_TimerG_setCaptureCompareValue(PWM_LED_INST, compare_value, GPIO_PWM_LED_C1_IDX);
}

/**
 * @brief 设置 PWM 占空比 (0 ~ 100%)
 * @param duty 占空比百分比 (0 ~ 100)
 */
void PWM_Set_Duty(uint32_t duty)
{
    if (duty > 100) {
        duty = 100;
    }
    /* 将百分比映射至 0 ~ PWM_PERIOD_MAX */
    uint32_t compare_val = (duty * PWM_PERIOD_MAX) / 100;
    PWM_Set_CompareValue(compare_val);
}
