#ifndef __PID_H
#define __PID_H

#include <stdint.h>

/**
 * 增量式 PID 控制器 (参考《电子系统综合实践》报告 3.2 节)
 *
 * 增量式公式:
 *   du(k) = Kp*[e(k)-e(k-1)] + Ki*e(k) + Kd*[e(k)-2*e(k-1)+e(k-2)]
 *   u(k)  = u(k-1) + du(k)   (限幅)
 *
 * 特点: 只需保存最近两次偏差; 输出增量不会突变;
 *       输出限幅后增量自动停止累积, 天然抑制积分饱和
 */
typedef struct {
    float    kp;                 /* 比例系数 */
    float    ki;                 /* 积分系数 */
    float    kd;                 /* 微分系数 */
    float    e_k1;               /* 上一次偏差 e(k-1) */
    float    e_k2;               /* 上上次偏差 e(k-2) */
    float    out;                /* 输出累积量 u(k) */
    float    out_min;            /* 输出下限 */
    float    out_max;            /* 输出上限 */
} PID_t;

/**
 * @brief 初始化 PID 控制器
 * @param pid  控制器实例
 * @param kp   比例系数
 * @param ki   积分系数
 * @param kd   微分系数
 * @param min  输出下限
 * @param max  输出上限
 */
void PID_Init(PID_t *pid, float kp, float ki, float kd, float out_min, float out_max);

/**
 * @brief 复位 PID 内部状态 (偏差历史与输出累积清零)
 * @param pid  控制器实例
 */
void PID_Reset(PID_t *pid);

/**
 * @brief 增量式 PID 一步运算
 * @param pid      控制器实例
 * @param target   目标值 (设定值)
 * @param feedback 反馈值 (实测值)
 * @return int32_t 限幅后的控制输出 u(k)
 */
int32_t PID_IncrementalStep(PID_t *pid, float target, float feedback);

/**
 * @brief 修改 PID 参数 (保留偏差历史与输出累积)
 */
void PID_SetTunings(PID_t *pid, float kp, float ki, float kd);

#endif /* __PID_H */
