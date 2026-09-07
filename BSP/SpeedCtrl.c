#include "SpeedCtrl.h"
#include "PID.h"
#include "Encoder.h"
#include "Motor.h"

/* 目标转速限幅: 防止串口误输入导致电机全速飞车 */
#define SPEEDCTRL_TARGET_MAX    400     /* 输出轴 RPM 上限 (减速比 1:20, 电机轴约 8000 RPM) */

/* PID 输出限幅: 与 PWM 满量程一致 */
#define SPEEDCTRL_OUT_MIN       (-(float)MOTOR_PWM_PERIOD_MAX)
#define SPEEDCTRL_OUT_MAX       ((float)MOTOR_PWM_PERIOD_MAX)

/* PID 反馈换算: 10ms 脉冲增量 -> 输出轴 RPM
 * RPM = 增量(脉冲/10ms) * 1000/10 * 60 / (减速比 * PPR)
 *     = 增量 * 6000 / (ENCODER_GEAR_RATIO * ENCODER_PPR)          */
#define SPEEDCTRL_FB_DEN        (ENCODER_GEAR_RATIO * ENCODER_PPR)

/* 默认 PID 参数 (串口实测整定: 80/150/-100 RPM 全工况锁定, PWM 抖动最小; Kd=0 避免放大量化噪声)
 * 运行时可通过 KP=/KI=/KD= 在线修改 */
#define SPEEDCTRL_KP_DEFAULT    1.2f
#define SPEEDCTRL_KI_DEFAULT    0.3f
#define SPEEDCTRL_KD_DEFAULT    0.0f

static PID_t  s_pid_l;                  /* 左轮 PID 控制器 */
static PID_t  s_pid_r;                  /* 右轮 PID 控制器 */
static uint8_t s_enabled = 0;           /* 闭环使能标志 */
static volatile int32_t s_target_l = 0; /* 左轮目标转速 (RPM) */
static volatile int32_t s_target_r = 0; /* 右轮目标转速 (RPM) */
static volatile int32_t s_out_l = 0;    /* 左轮 PID 输出 (PWM) */
static volatile int32_t s_out_r = 0;    /* 右轮 PID 输出 (PWM) */

/**
 * @brief 速度闭环模块初始化
 */
void SpeedCtrl_Init(void)
{
    PID_Init(&s_pid_l, SPEEDCTRL_KP_DEFAULT, SPEEDCTRL_KI_DEFAULT,
             SPEEDCTRL_KD_DEFAULT, SPEEDCTRL_OUT_MIN, SPEEDCTRL_OUT_MAX);
    PID_Init(&s_pid_r, SPEEDCTRL_KP_DEFAULT, SPEEDCTRL_KI_DEFAULT,
             SPEEDCTRL_KD_DEFAULT, SPEEDCTRL_OUT_MIN, SPEEDCTRL_OUT_MAX);
    s_target_l = 0;
    s_target_r = 0;
    s_out_l = 0;
    s_out_r = 0;
    s_enabled = 0;
}

/**
 * @brief 速度闭环周期处理 (SysTick 1ms 调用, 内部 10ms 分频)
 */
void SpeedCtrl_Tick_Handler(void)
{
    static uint16_t ms_cnt = 0;

    if (++ms_cnt < ENCODER_SAMPLE_MS) {
        return;
    }
    ms_cnt = 0;

    if (!s_enabled) {
        return;
    }

    /* 编码器 10ms 脉冲增量 -> 输出轴 RPM (带符号) */
    int32_t fb_l = (int32_t)((int64_t)Encoder_GetL10msDelta() * 6000 / SPEEDCTRL_FB_DEN);
    int32_t fb_r = (int32_t)((int64_t)Encoder_GetR10msDelta() * 6000 / SPEEDCTRL_FB_DEN);

    /* 左右轮独立增量式 PID 运算, 输出限幅 ±PWM 满量程 */
    s_out_l = PID_IncrementalStep(&s_pid_l, (float)s_target_l, (float)fb_l);
    s_out_r = PID_IncrementalStep(&s_pid_r, (float)s_target_r, (float)fb_r);

    /* 直接驱动电机 (PWM 值符号即方向) */
    L_MOTO_SetSpeed((int)s_out_l);
    R_MOTO_SetSpeed((int)s_out_r);
}

/**
 * @brief 使能/关闭速度闭环
 */
void SpeedCtrl_Enable(uint8_t en)
{
    if (en && !s_enabled) {
        /* 使能时复位 PID 状态, 从零开始平稳建立输出 */
        PID_Reset(&s_pid_l);
        PID_Reset(&s_pid_r);
    }
    s_enabled = en ? 1 : 0;

    if (!s_enabled) {
        /* 关闭闭环: 电机停转, 同步清零开环速度设定, 避免残留旧值 */
        Motor_SetSpeed(0);
        s_out_l = 0;
        s_out_r = 0;
    }
}

/**
 * @brief 查询闭环是否使能
 */
uint8_t SpeedCtrl_IsEnabled(void)
{
    return s_enabled;
}

/**
 * @brief 设置闭环目标转速 (左右轮相同)
 */
void SpeedCtrl_SetTarget(int32_t rpm)
{
    SpeedCtrl_SetTargetLR(rpm, rpm);
}

/**
 * @brief 分别设置左右轮闭环目标转速
 */
void SpeedCtrl_SetTargetLR(int32_t l_rpm, int32_t r_rpm)
{
    if (l_rpm > SPEEDCTRL_TARGET_MAX)  l_rpm = SPEEDCTRL_TARGET_MAX;
    if (l_rpm < -SPEEDCTRL_TARGET_MAX) l_rpm = -SPEEDCTRL_TARGET_MAX;
    if (r_rpm > SPEEDCTRL_TARGET_MAX)  r_rpm = SPEEDCTRL_TARGET_MAX;
    if (r_rpm < -SPEEDCTRL_TARGET_MAX) r_rpm = -SPEEDCTRL_TARGET_MAX;

    s_target_l = l_rpm;
    s_target_r = r_rpm;
}

/**
 * @brief 在线设置 PID 参数 (同时作用于左右轮)
 */
void SpeedCtrl_SetParams(float kp, float ki, float kd)
{
    PID_SetTunings(&s_pid_l, kp, ki, kd);
    PID_SetTunings(&s_pid_r, kp, ki, kd);
}

int32_t SpeedCtrl_GetTargetL(void) { return s_target_l; }
int32_t SpeedCtrl_GetTargetR(void) { return s_target_r; }
int32_t SpeedCtrl_GetOutL(void)    { return s_out_l; }
int32_t SpeedCtrl_GetOutR(void)    { return s_out_r; }
float   SpeedCtrl_GetKp(void)      { return s_pid_l.kp; }
float   SpeedCtrl_GetKi(void)      { return s_pid_l.ki; }
float   SpeedCtrl_GetKd(void)      { return s_pid_l.kd; }
