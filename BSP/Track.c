#include "Track.h"
#include "Gray.h"
#include "SpeedCtrl.h"

/**
 * 循迹实现: 加权偏差 + 分档比例差速 + 定轴自旋找回 (详见 Track.h 顶部说明)
 */

/* 探头位置权重 (单位 0.5cm): OUT1(最右) .. OUT8(最左) */
static const int8_t s_weight2[8] = { +7, +5, +3, +1, -1, -3, -5, -7 };

/* ---------------- 模块状态 ---------------- */
static uint8_t  s_enabled    = 0U;                          /* 循迹使能 */
static int32_t  s_base_speed = TRACK_SPEED_BASE;            /* 直道基速 (可在线改) */
static Track_State_e s_state = TRACK_STATE_IDLE;            /* 当前档位/状态 */

static uint8_t  s_raw_prev   = 0xFFU;   /* 上一拍原始图案 (初值取不可能值, 保证首拍判为变化) */
static uint8_t  s_same_cnt   = 0U;      /* 连续同图案拍数 (消抖) */
static uint8_t  s_sensor     = 0x00U;   /* 消抖后的图案 */

static int16_t  s_pos2       = 0;       /* 最近一次偏差 (0.5cm 单位) */
static int16_t  s_pos2_last  = 0;       /* 最近一次**非零**偏差 (短暂全白时保持用) */
static int8_t   s_dir_last   = 1;       /* 最近一次线所在半侧: +1 右, -1 左 */
static uint8_t  s_pivot_hold = 0U;      /* 直角自旋滞回锁存 */

static uint16_t s_lost_ticks    = 0U;   /* 全白连续拍数 */
static uint16_t s_search_ticks  = 0U;   /* 找回自旋已转拍数 */
static uint16_t s_recover_ticks = 0U;   /* 找回后低速交接剩余拍数 */

/**
 * @brief 由 8 路图案算加权偏差 pos2 (单位 0.5cm, 值域 ±7)
 */
static int16_t Track_CalcPos2(uint8_t sensor)
{
    int32_t sum_w = 0;
    int32_t cnt = 0;
    uint8_t i;

    for (i = 0; i < 8U; i++) {
        if (sensor & (uint8_t)(1U << i)) {
            sum_w += s_weight2[i];
            cnt++;
        }
    }
    if (cnt == 0) {
        return 0;                   /* 全白交给上层(脱线逻辑)处理 */
    }
    return (int16_t)(sum_w / cnt);
}

/**
 * @brief 施加控制律 —— 全模块唯一的一条律, 直道/弯道/直角三档都走这里
 * @param pos2 偏差 (0.5cm 单位, 正 = 线在右)
 * @param base 当前基速 RPM (低速交接期间传入更小的值)
 */
static void Track_ApplyLaw(int16_t pos2, int32_t base)
{
    int32_t mag = (pos2 < 0) ? -(int32_t)pos2 : (int32_t)pos2;
    int32_t l;
    int32_t r;

    /* 直角档滞回: 进入需 |pos2| >= 4 (2.0cm), 退出需 |pos2| <= 2 (1.0cm) */
    if (s_pivot_hold) {
        if (mag <= TRACK_PIVOT_EXIT2) {
            s_pivot_hold = 0U;
        }
    } else if (mag >= TRACK_PIVOT_ENTER2) {
        s_pivot_hold = 1U;
    }

    if (s_pivot_hold) {
        /* 直角档: 定轴自旋, 平均速度为 0 -> 原地转, 不吃掉直角后的直线段 */
        int32_t a = TRACK_SPEED_PIVOT;
        l = (pos2 > 0) ? a : -a;    /* 线在右 -> 右转(顺时针): 左轮正转, 右轮反转 */
        r = (pos2 > 0) ? -a : a;
        s_state = TRACK_STATE_PIVOT;
    } else {
        int32_t turn;

        if (mag >= TRACK_CURVE_ENTER2) {
            base = (base * TRACK_CURVE_BASE_PCT) / 100;
            s_state = TRACK_STATE_CURVE;
        } else {
            s_state = TRACK_STATE_LINE;
        }

        /* 差速 = 0.35 x 基速 x |偏差(cm)|, 其中 |偏差(cm)| = mag / 2 */
        turn = (base * TRACK_TURN_GAIN_PCT * mag) / (100 * TRACK_POS2_PER_CM);
        if (turn > TRACK_TURN_MAX) {
            turn = TRACK_TURN_MAX;
        }

        if (pos2 > 0) {             /* 线偏右 -> 右转: 左轮快, 右轮慢 */
            l = base + turn;
            r = base - turn;
        } else {                    /* 线偏左 -> 左转: 右轮快, 左轮慢 */
            l = base - turn;
            r = base + turn;
        }
    }

    SpeedCtrl_SetTargetLR(l, r);
}

/**
 * @brief 进入/维持脱线找回: 朝"最后看到线的那一侧"定轴自旋
 *        自旋满 TRACK_SEARCH_MAX_TICKS 仍未扫到线 -> 停车报警 (靠按键重启)
 */
static void Track_EnterSearch(void)
{
    int32_t a = TRACK_SPEED_PIVOT;

    if (s_state != TRACK_STATE_SEARCH) {
        s_search_ticks = 0U;
        s_pivot_hold = 0U;
        s_recover_ticks = 0U;
        s_state = TRACK_STATE_SEARCH;
    }

    if (s_search_ticks < 0xFFFFU) {
        s_search_ticks++;
    }
    if (s_search_ticks >= TRACK_SEARCH_MAX_TICKS) {
        s_state = TRACK_STATE_ALARM;        /* 转了一圈还是没线: 锁存停车 */
        SpeedCtrl_SetTargetLR(0, 0);
        return;
    }

    if (s_dir_last > 0) {                   /* 线最后在右侧 -> 顺时针转回去找 */
        SpeedCtrl_SetTargetLR(a, -a);
    } else {                                /* 线最后在左侧 -> 逆时针转回去找 */
        SpeedCtrl_SetTargetLR(-a, a);
    }
}

void Track_Init(void)
{
    s_enabled = 0U;
    s_base_speed = TRACK_SPEED_BASE;
    s_state = TRACK_STATE_IDLE;
    s_raw_prev = 0xFFU;
    s_same_cnt = 0U;
    s_sensor = 0x00U;
    s_pos2 = 0;
    s_pos2_last = 0;
    s_dir_last = 1;
    s_pivot_hold = 0U;
    s_lost_ticks = 0U;
    s_search_ticks = 0U;
    s_recover_ticks = 0U;
}

void Track_Enable(uint8_t enable)
{
    if (enable) {
        if (!s_enabled) {
            /* 每次启动都从干净状态开始 (同时解除报警锁存) */
            s_state = TRACK_STATE_LINE;
            s_raw_prev = 0xFFU;
            s_same_cnt = 0U;
            s_sensor = 0x00U;
            s_pos2 = 0;
            s_pos2_last = 0;
            s_dir_last = 1;
            s_pivot_hold = 0U;
            s_lost_ticks = 0U;
            s_search_ticks = 0U;
            s_recover_ticks = 0U;
            s_enabled = 1U;
        }
    } else {
        s_enabled = 0U;
        s_state = TRACK_STATE_IDLE;
        SpeedCtrl_SetTargetLR(0, 0);
    }
}

uint8_t Track_IsEnabled(void)
{
    return s_enabled;
}

void Track_SetBaseSpeed(int32_t speed)
{
    if (speed < 0) {
        speed = -speed;
    }
    if (speed < 30) {                       /* 下限: 低于 30RPM 基本推不动车 */
        speed = 30;
    }
    if (speed > 250) {                      /* 上限: 与 SpeedCtrl 目标上限一致 */
        speed = 250;
    }
    s_base_speed = speed;
}

int32_t Track_GetBaseSpeed(void)
{
    return s_base_speed;
}

Track_State_e Track_GetState(void)
{
    return s_state;
}

int16_t Track_GetPosMM(void)
{
    return (int16_t)(s_pos2 * TRACK_POS2_TO_MM);
}

uint8_t Track_GetSensor(void)
{
    return s_sensor;
}

void Track_Process(void)
{
    uint8_t raw;
    int16_t pos2;
    int32_t base;

    if (!s_enabled) {
        /* 未使能: 不动目标值, 保留台架测试 (SPD= / KP= 指令直接驱动电机) 的能力 */
        s_state = TRACK_STATE_IDLE;
        return;
    }

    if (s_state == TRACK_STATE_ALARM) {
        SpeedCtrl_SetTargetLR(0, 0);        /* 报警锁存: 只有 Track_Enable(1) 能复位 */
        return;
    }

    /* ---- 1. 采样 + 消抖: 连续 TRACK_DEBOUNCE_TICKS 拍同一图案才采用 ---- */
    raw = Gray_ReadByte();
    if (raw == s_raw_prev) {
        if (s_same_cnt < 0xFFU) {
            s_same_cnt++;
        }
    } else {
        s_same_cnt = 1U;
        s_raw_prev = raw;
    }
    if (s_same_cnt >= TRACK_DEBOUNCE_TICKS) {
        s_sensor = raw;
    }

    /* ---- 2. 全白处理 ---- */
    if (s_sensor == 0x00U) {
        if (s_lost_ticks < 0xFFFFU) {
            s_lost_ticks++;
        }

        if (s_lost_ticks >= TRACK_LOST_TICKS) {
            Track_EnterSearch();            /* 满 50ms: 判定脱线, 定轴自旋找回 */
        } else if (s_state != TRACK_STATE_SEARCH) {
            /* 50ms 内保持上一拍的偏差继续走。
             * 直角两条线之间、赛道小缺口、图案切换的空档都会短暂全白, 这时"保持原来的
             * 修正方向"比"改直行"更正确 (老实现就是在这里丢掉拐点的)。 */
            if (s_recover_ticks > 0U) {
                s_recover_ticks--;
                Track_ApplyLaw(s_pos2_last, TRACK_SPEED_RECOVER);
            } else {
                Track_ApplyLaw(s_pos2_last, s_base_speed);
            }
        }
        return;
    }

    /* ---- 3. 见到线 ---- */
    s_lost_ticks = 0U;

    /* 从找回状态重新捕获到线 -> 先低速走一段再回主律, 防止刚好转又冲出去 */
    if (s_state == TRACK_STATE_SEARCH) {
        s_recover_ticks = TRACK_RECOVER_TICKS;
    }

    pos2 = Track_CalcPos2(s_sensor);
    if (pos2 != 0) {
        s_pos2_last = pos2;
        s_dir_last = (pos2 > 0) ? 1 : -1;
    }
    s_pos2 = pos2;

    base = s_base_speed;
    if (s_recover_ticks > 0U) {
        s_recover_ticks--;
        base = TRACK_SPEED_RECOVER;
    }

    Track_ApplyLaw(pos2, base);

    /* 交接期间档位上报为 RECOVER (控制律本身仍按偏差走, 只是基速低) */
    if (s_recover_ticks > 0U && s_state != TRACK_STATE_PIVOT) {
        s_state = TRACK_STATE_RECOVER;
    }
}
