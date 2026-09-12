#include "SpeedCtrl.h"
#include "PID.h"
#include "Encoder.h"
#include "Motor.h"

/* 目标转速限幅: 防止输入异常导致电机飞车 */
#define SPEEDCTRL_TARGET_MAX    250     /* 输出轴 RPM 上限 (减速比 1:20) */

/* PID 输出限幅: 与 PWM 满量程一致 */
#define SPEEDCTRL_OUT_MIN       (-(float)MOTOR_PWM_PERIOD_MAX)
#define SPEEDCTRL_OUT_MAX       ((float)MOTOR_PWM_PERIOD_MAX)

/* PID 反馈换算: 10ms 脉冲增量 -> 输出轴 RPM
 * RPM = 增量(脉冲/10ms) * 1000/10 * 60 / (减速比 * PPR)
 *     = 增量 * 6000 / (ENCODER_GEAR_RATIO * ENCODER_PPR) */
#define SPEEDCTRL_FB_DEN        (ENCODER_GEAR_RATIO * ENCODER_PPR)

/* 默认速度环 PID 参数 (整定于 100Hz 闭环控制与 30ms 滤波反馈)
 * Kp=1.20, Ki=0.40, Kd=0.0 (置零 Kd 消除离散量化阶跃噪声) */
#define SPEEDCTRL_KP_DEFAULT    1.20f
#define SPEEDCTRL_KI_DEFAULT    0.40f
#define SPEEDCTRL_KD_DEFAULT    0.0f

static PID_t  s_pid_l;                  /* 左轮 PID 控制器 */
static PID_t  s_pid_r;                  /* 右轮 PID 控制器 */
static uint8_t s_enabled = 0;           /* 闭环使能标志 */
static volatile int32_t s_target_l = 0; /* 左轮目标转速 (RPM) */
static volatile int32_t s_target_r = 0; /* 右轮目标转速 (RPM) */
static volatile int32_t s_out_l = 0;    /* 左轮 PID 输出 (PWM) */
static volatile int32_t s_out_r = 0;    /* 右轮 PID 输出 (PWM) */

/* 反馈滤波: 3 拍 10ms 增量滑动求和 (等效 30ms 滤波窗口, 平滑 9.09 RPM 量化步阶) */
#define SPEEDCTRL_WIN_N         3
static int32_t s_dq_l[SPEEDCTRL_WIN_N] = {0};   /* 左轮 10ms 增量环形缓冲 */
static int32_t s_dq_r[SPEEDCTRL_WIN_N] = {0};   /* 右轮 10ms 增量环形缓冲 */
static uint8_t s_dq_idx = 0U;                   /* 环形缓冲写指针 */
static volatile int32_t s_fb_l = 0;             /* 左轮 30ms 窗口反馈 (RPM) */
static volatile int32_t s_fb_r = 0;             /* 右轮 30ms 窗口反馈 (RPM) */

/**
 * @brief 复位滑动窗口 (使能时调用, 避免带入上一次运行的残留)
 */
static void SpeedCtrl_ResetWindow(void)
{
    uint8_t i;
    for (i = 0U; i < SPEEDCTRL_WIN_N; i++) {
        s_dq_l[i] = 0;
        s_dq_r[i] = 0;
    }
    s_dq_idx = 0U;
    s_fb_l   = 0;
    s_fb_r   = 0;
}

/**
 * @brief 初始化速度闭环控制模块
 */
void SpeedCtrl_Init(void)
{
    PID_Init(&s_pid_l, SPEEDCTRL_KP_DEFAULT, SPEEDCTRL_KI_DEFAULT, SPEEDCTRL_KD_DEFAULT,
             SPEEDCTRL_OUT_MIN, SPEEDCTRL_OUT_MAX);
    PID_Init(&s_pid_r, SPEEDCTRL_KP_DEFAULT, SPEEDCTRL_KI_DEFAULT, SPEEDCTRL_KD_DEFAULT,
             SPEEDCTRL_OUT_MIN, SPEEDCTRL_OUT_MAX);

    s_enabled  = 0;
    s_target_l = 0;
    s_target_r = 0;
    s_out_l    = 0;
    s_out_r    = 0;
    SpeedCtrl_ResetWindow();
}

/**
 * @brief 速度闭环周期处理 (SysTick 1ms 调用, 内部 ENCODER_SAMPLE_MS 分频)
 */
void SpeedCtrl_Tick_Handler(void)
{
    static uint32_t s_ms_cnt = 0;
    s_ms_cnt++;
    if (s_ms_cnt < ENCODER_SAMPLE_MS) {
        return;
    }
    s_ms_cnt = 0;

    if (!s_enabled) {
        return;
    }

    /* 1. 将最新的 10ms 脉冲增量写入滑动窗口 */
    s_dq_l[s_dq_idx] = Encoder_GetL10msDelta();
    s_dq_r[s_dq_idx] = Encoder_GetR10msDelta();
    s_dq_idx = (uint8_t)((s_dq_idx + 1U) % SPEEDCTRL_WIN_N);

    /* 2. 统计 3 拍累计脉冲求和 */
    int32_t sum_l = 0;
    int32_t sum_r = 0;
    uint8_t i;
    for (i = 0U; i < SPEEDCTRL_WIN_N; i++) {
        sum_l += s_dq_l[i];
        sum_r += s_dq_r[i];
    }

    /* 3. 换算为 RPM 反馈值 (30ms 窗口):
     * RPM = sum * (1000/30) * 60 / (减速比 * PPR) = sum * 2000 / (减速比 * PPR) */
    s_fb_l = sum_l * 2000 / SPEEDCTRL_FB_DEN;
    s_fb_r = sum_r * 2000 / SPEEDCTRL_FB_DEN;

    /* 4. 执行增量式 PI 计算 */
    s_out_l = PID_IncrementalStep(&s_pid_l, (float)s_target_l, (float)s_fb_l);
    s_out_r = PID_IncrementalStep(&s_pid_r, (float)s_target_r, (float)s_fb_r);

    /* 5. 驱动电机底层输出 */
    L_MOTO_SetSpeed((int)s_out_l);
    R_MOTO_SetSpeed((int)s_out_r);
}

/**
 * @brief 使能/关闭速度闭环
 */
void SpeedCtrl_Enable(uint8_t en)
{
    if (en) {
        if (!s_enabled) {
            PID_Reset(&s_pid_l);
            PID_Reset(&s_pid_r);
            SpeedCtrl_ResetWindow();
            s_out_l   = 0;
            s_out_r   = 0;
            s_enabled = 1;
        }
    } else {
        s_enabled  = 0;
        s_target_l = 0;
        s_target_r = 0;
        s_out_l    = 0;
        s_out_r    = 0;
        L_MOTO_SetSpeed(0);
        R_MOTO_SetSpeed(0);
        PID_Reset(&s_pid_l);
        PID_Reset(&s_pid_r);
        SpeedCtrl_ResetWindow();
    }
}

uint8_t SpeedCtrl_IsEnabled(void)
{
    return s_enabled;
}

void SpeedCtrl_SetTarget(int32_t rpm)
{
    SpeedCtrl_SetTargetLR(rpm, rpm);
}

void SpeedCtrl_SetTargetLR(int32_t l_rpm, int32_t r_rpm)
{
    if (l_rpm >  SPEEDCTRL_TARGET_MAX) l_rpm =  SPEEDCTRL_TARGET_MAX;
    if (l_rpm < -SPEEDCTRL_TARGET_MAX) l_rpm = -SPEEDCTRL_TARGET_MAX;
    if (r_rpm >  SPEEDCTRL_TARGET_MAX) r_rpm =  SPEEDCTRL_TARGET_MAX;
    if (r_rpm < -SPEEDCTRL_TARGET_MAX) r_rpm = -SPEEDCTRL_TARGET_MAX;

    s_target_l = l_rpm;
    s_target_r = r_rpm;
}

int32_t SpeedCtrl_GetTargetL(void)
{
    return s_target_l;
}

int32_t SpeedCtrl_GetTargetR(void)
{
    return s_target_r;
}

void SpeedCtrl_SetParams(float kp, float ki, float kd)
{
    PID_SetTunings(&s_pid_l, kp, ki, kd);
    PID_SetTunings(&s_pid_r, kp, ki, kd);
}

float SpeedCtrl_GetKp(void)
{
    return s_pid_l.kp;
}

float SpeedCtrl_GetKi(void)
{
    return s_pid_l.ki;
}

float SpeedCtrl_GetKd(void)
{
    return s_pid_l.kd;
}

int32_t SpeedCtrl_GetFbL(void)
{
    return s_fb_l;
}

int32_t SpeedCtrl_GetFbR(void)
{
    return s_fb_r;
}
