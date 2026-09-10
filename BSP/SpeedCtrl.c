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
 *     = 增量 * 6000 / (ENCODER_GEAR_RATIO * ENCODER_PPR)          */
#define SPEEDCTRL_FB_DEN        (ENCODER_GEAR_RATIO * ENCODER_PPR)

/* 默认 PID 参数 -- 2026-09-10 实车台架辨识 + 阶跃多组实测对比整定结果 (旧值 Kp=0.35, Ki=0.15)
 *
 * 电机对象模型 (悬空实测辨识):
 *   K   = 0.285 RPM/占空比  (60~250 RPM 线性区, 静态标定与饱和段外推完全一致)
 *   tau = 45ms              (跟随亏差法与阶跃响应拟合值)
 *   启动静摩擦死区约 120 占空比 (仅在零速起步与换向穿越零速瞬间生效)
 *   反馈: 30ms 滑动窗口, 220 脉冲/圈, 量化分辨率 1 脉冲 = 9.09 RPM
 *
 * 实测三组参数阶跃对比 (0->100 RPM 起步与 100->150 RPM 阶跃):
 *   1. 原始组 (Kp=0.35, Ki=0.15):
 *      达到 90 RPM 用时左 356ms / 右 590ms (双轮极其不对称，右轮迟缓滞后 234ms);
 *      PWM 稳态 SD 较小 (±4~6)，但响应过慢且有稳态转速差。
 *   2. 稳健推荐组 (Kp=1.20, Ki=0.40):
 *      达到 90 RPM 用时左 186ms / 右 186ms (双轮高度同步，用时缩短到原 1/3);
 *      超调仅 1 个量化步阶 (9 RPM, 9%);
 *      PWM 稳态波动 SD 极低 (左 ±8.4, 右 ±5.9, 范围仅 334~373)，运行平稳安静。
 *   3. 激进组 (Kp=2.00, Ki=0.50):
 *      用时 202ms (受电机物理升速限制，未进一步提升);
 *      超调增大至 36% (左轮冲到 136 RPM);
 *      PWM 稳态波动 SD 激增至 ±26.7 (范围 287~381，抖动高达 75 单位，电机高频蜂鸣)。
 *
 * 结论: Kp=1.20, Ki=0.40 为工程最优解。
 * 注意: Kd 必须维持 0.0f (编码器 9.09 RPM 离散台阶会被微分项急剧放大造成剧烈抖振)。 */
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

/* ---- 反馈窗口: 3 拍 10ms 增量的滑动和 (等效 30ms 平均) ----
 * 编码器分辨率只有 ENCODER_GEAR_RATIO*ENCODER_PPR = 220 脉冲/圈, 基速 100RPM 时单拍
 * 10ms 增量只有约 3.7 个脉冲, 而 1 个脉冲就等于 27 RPM。把单拍增量直接送 PID, 等于给
 * 反馈叠了一层 ±13.5RPM 的量化噪声; 而循迹环交给速度环的差速指令本身只有几十 RPM,
 * 会被这层噪声淹没(表现为电机抖、直道画龙)。改成 3 拍滑动平均后 1 脉冲 = 9 RPM。
 * PID 仍每 10ms 运算一次, 只是反馈量换成 30ms 窗口, 代价是 20ms 额外滞后。 */
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
    uint8_t k;

    for (k = 0U; k < SPEEDCTRL_WIN_N; k++) {
        s_dq_l[k] = 0;
        s_dq_r[k] = 0;
    }
    s_dq_idx = 0U;
    s_fb_l = 0;
    s_fb_r = 0;
}

/**
 * @brief 初始化速度闭环控制模块
 */
void SpeedCtrl_Init(void)
{
    PID_Init(&s_pid_l, SPEEDCTRL_KP_DEFAULT, SPEEDCTRL_KI_DEFAULT,
             SPEEDCTRL_KD_DEFAULT, SPEEDCTRL_OUT_MIN, SPEEDCTRL_OUT_MAX);
    PID_Init(&s_pid_r, SPEEDCTRL_KP_DEFAULT, SPEEDCTRL_KI_DEFAULT,
             SPEEDCTRL_KD_DEFAULT, SPEEDCTRL_OUT_MIN, SPEEDCTRL_OUT_MAX);
    s_target_l = 0;
    s_target_r = 0;
    s_out_l    = 0;
    s_out_r    = 0;
    s_enabled  = 0;
}

/**
 * @brief 速度闭环周期处理 (SysTick 1ms 调用, 内部 ENCODER_SAMPLE_MS 分频)
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
    /* 编码器 10ms 脉冲增量 -> 30ms 滑动窗口 -> 输出轴 RPM (带符号) */
    int32_t sum_l = 0;
    int32_t sum_r = 0;
    int32_t fb_l;
    int32_t fb_r;
    uint8_t k;

    s_dq_l[s_dq_idx] = Encoder_GetL10msDelta();
    s_dq_r[s_dq_idx] = Encoder_GetR10msDelta();
    s_dq_idx = (uint8_t)((s_dq_idx + 1U) % SPEEDCTRL_WIN_N);

    for (k = 0U; k < SPEEDCTRL_WIN_N; k++) {
        sum_l += s_dq_l[k];
        sum_r += s_dq_r[k];
    }
    /* 3 拍(30ms)增量之和 -> RPM: 增量/30ms => (1000/30*60)/(减速比 x PPR) = 2000/220 */
    fb_l = (int32_t)((int64_t)sum_l * 2000 / SPEEDCTRL_FB_DEN);
    fb_r = (int32_t)((int64_t)sum_r * 2000 / SPEEDCTRL_FB_DEN);
    s_fb_l = fb_l;
    s_fb_r = fb_r;

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
        s_out_l = 0;
        s_out_r = 0;
        SpeedCtrl_ResetWindow();
        s_enabled = 1;
    } else if (!en && s_enabled) {
        s_enabled = 0;
        s_target_l = 0;
        s_target_r = 0;
        s_out_l = 0;
        s_out_r = 0;
        PID_Reset(&s_pid_l);
        PID_Reset(&s_pid_r);
        Motor_SetSpeed(0); /* 停转电机 */
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
    if (l_rpm > SPEEDCTRL_TARGET_MAX) l_rpm = SPEEDCTRL_TARGET_MAX;
    if (l_rpm < -SPEEDCTRL_TARGET_MAX) l_rpm = -SPEEDCTRL_TARGET_MAX;
    if (r_rpm > SPEEDCTRL_TARGET_MAX) r_rpm = SPEEDCTRL_TARGET_MAX;
    if (r_rpm < -SPEEDCTRL_TARGET_MAX) r_rpm = -SPEEDCTRL_TARGET_MAX;

    s_target_l = l_rpm;
    s_target_r = r_rpm;
}

void SpeedCtrl_SetParams(float kp, float ki, float kd)
{
    PID_SetTunings(&s_pid_l, kp, ki, kd);
    PID_SetTunings(&s_pid_r, kp, ki, kd);
}

int32_t SpeedCtrl_GetTargetL(void) { return s_target_l; }
int32_t SpeedCtrl_GetTargetR(void) { return s_target_r; }
int32_t SpeedCtrl_GetOutL(void)    { return s_out_l; }
int32_t SpeedCtrl_GetOutR(void)    { return s_out_r; }
int32_t SpeedCtrl_GetFbL(void)     { return s_fb_l; }
int32_t SpeedCtrl_GetFbR(void)     { return s_fb_r; }
float   SpeedCtrl_GetKp(void)      { return s_pid_l.kp; }
float   SpeedCtrl_GetKi(void)      { return s_pid_l.ki; }
float   SpeedCtrl_GetKd(void)      { return s_pid_l.kd; }
