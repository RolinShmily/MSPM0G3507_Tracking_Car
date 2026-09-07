#ifndef __MOTOR_H
#define __MOTOR_H

#include "ti_msp_dl_config.h"

/* PWM 占空比最大值 (与 SysConfig 中 Period=1000 对应) 及挡位数量 */
#define MOTOR_PWM_PERIOD_MAX    1000
#define MOTOR_GEAR_MAX          10

/**
 * @brief 电机驱动模块初始化: 启动 TIMG8 并将初始速度置 0
 */
void Motor_Init(void);

/**
 * @brief 控制左电机 (A路) 速度与方向
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void L_MOTO_SetSpeed(int Speed);

/**
 * @brief 控制右电机 (B路) 速度与方向
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void R_MOTO_SetSpeed(int Speed);

/**
 * @brief 设置电机整体速度与方向 (同时作用于 A 路和 B 路)
 * @param Speed 速度值 (-1000 ~ +1000): >0 正转, <0 反转, =0 刹车停止
 */
void Motor_SetSpeed(int Speed);

/**
 * @brief 获取左电机 (A路) 当前速度值
 * @return int 当前电机速度值: >0 正转, <0 反转, 0 停止
 */
int L_MOTO_GetSpeed(void);

/**
 * @brief 获取右电机 (B路) 当前速度值
 * @return int 当前电机速度值: >0 正转, <0 反转, 0 停止
 */
int R_MOTO_GetSpeed(void);

/**
 * @brief 获取电机当前整体速度值
 * @return int 当前电机速度值: >0 正转, <0 反转, 0 停止
 */
int Motor_GetSpeed(void);

/**
 * @brief 获取电机当前方向 (取决于最近一次设置的速度极性)
 * @return 1 正转(Forward), -1 反转(Backward)
 */
int Motor_GetDirection(void);

#endif /* __MOTOR_H */
