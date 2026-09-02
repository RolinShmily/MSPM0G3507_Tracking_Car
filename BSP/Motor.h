#ifndef __MOTOR_H
#define __MOTOR_H

#include "ti_msp_dl_config.h"

/* PWM 周期最大值 (与 SysConfig 中 Period=1000 对应) 与档位定义 */
#define MOTOR_PWM_PERIOD_MAX    1000
#define MOTOR_GEAR_MAX          10

/**
 * @brief 电机驱动模块初始化（启动 TIMG8 计数并初始化速度为 0）
 */
void Motor_Init(void);

/**
 * @brief 设置左电机 (A路) 速度与方向
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void L_MOTO_SetSpeed(int Speed);

/**
 * @brief 设置右电机 (B路) 速度与方向
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void R_MOTO_SetSpeed(int Speed);

/**
 * @brief 设置电机总体速度与方向 (同时控制 A 路与 B 路)
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void Motor_SetSpeed(int Speed);

#endif /* __MOTOR_H */
