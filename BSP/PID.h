#ifndef __PID_H
#define __PID_H

#include <stdint.h>

/**
 * @file    PID.h
 * @brief   增量式 PID 控制器 (Velocity-form PID)
 *
 * 增量式算法公式:
 *   du(k) = Kp*[e(k) - e(k-1)] + Ki*e(k) + Kd*[e(k) - 2*e(k-1) + e(k-2)]
 *   u(k)  = u(k-1) + du(k)
 *
 * 特性:
 *   - 历史误差保存少 (只需保存最近两次历史误差)
 *   - 控制增量平滑无突变
 *   - 输出饱和限幅后自动停止累加，天然抑制积分饱和
 */
typedef struct {
    float    kp;                 /* 比例系数 */
    float    ki;                 /* 积分系数 */
    float    kd;                 /* 微分系数 */
    float    e_k1;               /* 上一次偏差 e(k-1) */
    float    e_k2;               /* 上上次偏差 e(k-2) */
    float    out;                /* 当前累加输出 u(k) */
    float    out_min;            /* 输出限幅下限 */
    float    out_max;            /* 输出限幅上限 */
} PID_t;

/**
 * @brief 初始化 PID 控制器
 * @param pid      控制器实例
 * @param kp       比例系数
 * @param ki       积分系数
 * @param kd       微分系数
 * @param out_min  输出下限
 * @param out_max  输出上限
 */
void PID_Init(PID_t *pid, float kp, float ki, float kd, float out_min, float out_max);

/**
 * @brief 复位 PID 内部状态 (清零历史偏差与累加输出)
 * @param pid 控制器实例
 */
void PID_Reset(PID_t *pid);

/**
 * @brief 增量式 PID 单步计算
 * @param pid      控制器实例
 * @param target   目标设定值
 * @param feedback 传感器实际反馈值
 * @return int32_t 限幅后的控制量输出 u(k)
 */
int32_t PID_IncrementalStep(PID_t *pid, float target, float feedback);

/**
 * @brief 在线修改 PID 控制参数
 * @param pid 控制器实例
 * @param kp  比例系数
 * @param ki  积分系数
 * @param kd  微分系数
 */
void PID_SetTunings(PID_t *pid, float kp, float ki, float kd);

#endif /* __PID_H */
