#include "PID.h"

/**
 * @brief 初始化 PID 控制器
 */
void PID_Init(PID_t *pid, float kp, float ki, float kd, float out_min, float out_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    PID_Reset(pid);
}

/**
 * @brief 复位 PID 内部状态
 */
void PID_Reset(PID_t *pid)
{
    pid->e_k1 = 0.0f;
    pid->e_k2 = 0.0f;
    pid->out  = 0.0f;
}

/**
 * @brief 增量式 PID 单步计算
 */
int32_t PID_IncrementalStep(PID_t *pid, float target, float feedback)
{
    float e = target - feedback;

    /* 增量: du = Kp*[e(k)-e(k-1)] + Ki*e(k) + Kd*[e(k)-2e(k-1)+e(k-2)] */
    float du = pid->kp * (e - pid->e_k1)
             + pid->ki * e
             + pid->kd * (e - 2.0f * pid->e_k1 + pid->e_k2);

    /* 更新偏差历史 */
    pid->e_k2 = pid->e_k1;
    pid->e_k1 = e;

    /* 输出累加与限幅: 越限后饱和不再累加, 抑制积分饱和 */
    pid->out += du;
    if (pid->out > pid->out_max) {
        pid->out = pid->out_max;
    }
    if (pid->out < pid->out_min) {
        pid->out = pid->out_min;
    }

    return (int32_t)pid->out;
}

/**
 * @brief 修改 PID 参数
 */
void PID_SetTunings(PID_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}
