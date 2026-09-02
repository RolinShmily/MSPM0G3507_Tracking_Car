#include "Motor.h"

/**
 * @brief 电机驱动模块初始化
 */
void Motor_Init(void)
{
    /* 1. 初始速度设置为 0 (刹车停止) */
    Motor_SetSpeed(0);

    /* 2. 启动 TIMG8 (PWM_MOTO) 定时器计数 */
    DL_TimerG_startCounter(PWM_MOTO_INST);
}

/**
 * @brief 设置左电机 (A路: AIN3-PB22, AIN4-PB23, PWM-PB15)
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void L_MOTO_SetSpeed(int Speed)
{
    /* 限幅保护 */
    if (Speed > MOTOR_PWM_PERIOD_MAX)  Speed = MOTOR_PWM_PERIOD_MAX;
    if (Speed < -MOTOR_PWM_PERIOD_MAX) Speed = -MOTOR_PWM_PERIOD_MAX;

    if (Speed > 0) {
        /* 正转：AIN3=1, AIN4=0, 输出正向 PWM */
        DL_GPIO_setPins(MOTO_PORT, MOTO_PIN_22_AIN3_PIN);
        DL_GPIO_clearPins(MOTO_PORT, MOTO_PIN_23_AIN4_PIN);
        DL_TimerG_setCaptureCompareValue(PWM_MOTO_INST, Speed, GPIO_PWM_MOTO_C0_IDX);
    }
    else if (Speed < 0) {
        /* 反转：AIN3=0, AIN4=1, 输出反向 PWM */
        DL_GPIO_setPins(MOTO_PORT, MOTO_PIN_23_AIN4_PIN);
        DL_GPIO_clearPins(MOTO_PORT, MOTO_PIN_22_AIN3_PIN);
        DL_TimerG_setCaptureCompareValue(PWM_MOTO_INST, -Speed, GPIO_PWM_MOTO_C0_IDX);
    }
    else {
        /* 刹车停止：AIN3=1, AIN4=1, PWM=0 */
        DL_GPIO_setPins(MOTO_PORT, MOTO_PIN_22_AIN3_PIN);
        DL_GPIO_setPins(MOTO_PORT, MOTO_PIN_23_AIN4_PIN);
        DL_TimerG_setCaptureCompareValue(PWM_MOTO_INST, 0, GPIO_PWM_MOTO_C0_IDX);
    }
}

/**
 * @brief 设置右电机 (B路: BIN3-PB25, BIN4-PB26, PWM-PB16)
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void R_MOTO_SetSpeed(int Speed)
{
    /* 限幅保护 */
    if (Speed > MOTOR_PWM_PERIOD_MAX)  Speed = MOTOR_PWM_PERIOD_MAX;
    if (Speed < -MOTOR_PWM_PERIOD_MAX) Speed = -MOTOR_PWM_PERIOD_MAX;

    if (Speed > 0) {
        /* 正转：BIN3=1, BIN4=0, 输出正向 PWM */
        DL_GPIO_setPins(MOTO_PORT, MOTO_PIN_25_BIN3_PIN);
        DL_GPIO_clearPins(MOTO_PORT, MOTO_PIN_26_BIN4_PIN);
        DL_TimerG_setCaptureCompareValue(PWM_MOTO_INST, Speed, GPIO_PWM_MOTO_C1_IDX);
    }
    else if (Speed < 0) {
        /* 反转：BIN3=0, BIN4=1, 输出反向 PWM */
        DL_GPIO_setPins(MOTO_PORT, MOTO_PIN_26_BIN4_PIN);
        DL_GPIO_clearPins(MOTO_PORT, MOTO_PIN_25_BIN3_PIN);
        DL_TimerG_setCaptureCompareValue(PWM_MOTO_INST, -Speed, GPIO_PWM_MOTO_C1_IDX);
    }
    else {
        /* 刹车停止：BIN3=1, BIN4=1, PWM=0 */
        DL_GPIO_setPins(MOTO_PORT, MOTO_PIN_25_BIN3_PIN);
        DL_GPIO_setPins(MOTO_PORT, MOTO_PIN_26_BIN4_PIN);
        DL_TimerG_setCaptureCompareValue(PWM_MOTO_INST, 0, GPIO_PWM_MOTO_C1_IDX);
    }
}

/**
 * @brief 设置电机总体速度与方向 (同时控制 A 路与 B 路)
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void Motor_SetSpeed(int Speed)
{
    L_MOTO_SetSpeed(Speed);
    R_MOTO_SetSpeed(Speed);
}
