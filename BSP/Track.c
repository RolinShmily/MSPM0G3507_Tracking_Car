#include "Track.h"
#include "Gray.h"
#include "SpeedCtrl.h"

/**
 * 循迹控制逻辑实现: 连续加权质心偏差 + 分级差速转向 + 定轴自旋找回
 */

/* 探头位置权重 (单位 0.5cm): OUT1(最右) .. OUT8(最左) */
static const int8_t s_weight2[8] = { +7, +5, +3, +1, -1, -3, -5, -7 };

/* ---------------- 模块内部运行状态 ---------------- */
static uint8_t  s_enabled    = 0U;                          /* 循迹使能标志 */
static int32_t  s_base_speed = TRACK_SPEED_BASE;            /* 直道基准巡航转速 */
static Track_State_e s_state = TRACK_STATE_IDLE;            /* 当前运行状态/档位 */

static uint8_t  s_raw_prev   = 0xFFU;   /* 上一拍原始采样图案 (初值 0xFF 确保首拍判定变化) */
static uint8_t  s_same_cnt   = 0U;      /* 连续相同图案拍数统计 (软件消抖) */
static uint8_t  s_sensor     = 0x00U;   /* 经消抖确认的当前传感器图案 */

static int16_t  s_pos2       = 0;       /* 当前加权偏差 (0.5cm 单位) */
static int16_t  s_pos2_last  = 0;       /* 最近一次有效非零偏差 (脱线搜索保持用) */
static int8_t   s_dir_last   = 1;       /* 最近一次线所在半侧: +1 右侧, -1 左侧 */
static uint8_t  s_pivot_hold = 0U;      /* 直角定轴自旋迟滞锁存标志 */

static uint16_t s_lost_ticks    = 0U;   /* 连续全白脱线拍数 */
static uint16_t s_search_ticks  = 0U;   /* 原地自旋寻线已耗拍数 */
static uint16_t s_recover_ticks = 0U;   /* 捕获线后低速过渡剩余拍数 */

/**
 * @brief 由 8 路探头图案计算加权质心偏差 pos2 (单位 0.5cm, 范围 ±7)
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
        return 0;                   /* 全白脱线交由上层状态机处理 */
    }
    return (int16_t)(sum_w / cnt);
}

/**
 * @brief 施加转向控制律 (直道比例差速、弯道差速与直角弯定轴自旋)
 * @param pos2 加权偏差 (0.5cm 为单位, 正值代表黑线偏右)
 * @param base 当前巡航基准转速 (RPM)
 */
static void Track_ApplyLaw(int16_t pos2, int32_t base)
{
    int32_t mag = (pos2 < 0) ? -(int32_t)pos2 : (int32_t)pos2;
    int32_t l;
    int32_t r;

    /* 直角弯档位迟滞逻辑: 进入需 |pos2| >= 4 (2.0cm), 退出需 |pos2| <= 2 (1.0cm) */
    if (s_pivot_hold) {
        if (mag <= TRACK_PIVOT_EXIT2) {
            s_pivot_hold = 0U;
        }
    } else if (mag >= TRACK_PIVOT_ENTER2) {
        s_pivot_hold = 1U;
    }

    if (s_pivot_hold) {
        /* 直角弯: 定轴原地自旋 (平均前进速度为 0) */
        int32_t a = TRACK_SPEED_PIVOT;
        l = (pos2 > 0) ? a : -a;    /* 线偏右 -> 顺时针右转: 左正右反 */
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

        /* 差速 = 0.35 * 基速 * |偏差(cm)|, 其中 |偏差(cm)| = mag / 2 */
        turn = (base * TRACK_TURN_GAIN_PCT * mag) / (100 * TRACK_POS2_PER_CM);
        if (turn > TRACK_TURN_MAX) {
            turn = TRACK_TURN_MAX;
        }

        if (pos2 > 0) {             /* 线偏右 -> 右转: 左轮加、右轮减 */
            l = base + turn;
            r = base - turn;
        } else {                    /* 线偏左 -> 左转: 左轮减、右轮加 */
            l = base - turn;
            r = base + turn;
        }
    }

    SpeedCtrl_SetTargetLR(l, r);
}

/**
 * @brief 执行脱线寻线状态: 朝最后见线侧原地定轴自旋搜线
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
        s_state = TRACK_STATE_ALARM;        /* 寻线超时锁定并停止电机 */
        SpeedCtrl_SetTargetLR(0, 0);
        return;
    }

    if (s_dir_last > 0) {                   /* 线最后出现在右侧 -> 顺时针自旋搜线 */
        SpeedCtrl_SetTargetLR(a, -a);
    } else {                                /* 线最后出现在左侧 -> 逆时针自旋搜线 */
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
            /* 使能时重置所有状态并清除报警锁存 */
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
    if (speed < 30) {                       /* 设定转速下限保护 */
        speed = 30;
    }
    if (speed > 250) {                      /* 设定转速上限保护 */
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
        s_state = TRACK_STATE_IDLE;
        return;
    }

    if (s_state == TRACK_STATE_ALARM) {
        SpeedCtrl_SetTargetLR(0, 0);        /* 报警锁定状态 */
        return;
    }

    /* 1. 采样与消抖: 连续 TRACK_DEBOUNCE_TICKS 拍图案一致方为有效采样 */
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

    /* 2. 全白脱线处理 */
    if (s_sensor == 0x00U) {
        if (s_lost_ticks < 0xFFFFU) {
            s_lost_ticks++;
        }

        if (s_lost_ticks >= TRACK_LOST_TICKS) {
            Track_EnterSearch();            /* 全白超 50ms: 进入定轴自旋搜线 */
        } else if (s_state != TRACK_STATE_SEARCH) {
            /* 50ms 缓冲期内保持上一拍历史偏差行驶, 渡过图案切换间隙 */
            if (s_recover_ticks > 0U) {
                s_recover_ticks--;
                Track_ApplyLaw(s_pos2_last, TRACK_SPEED_RECOVER);
            } else {
                Track_ApplyLaw(s_pos2_last, s_base_speed);
            }
        }
        return;
    }

    /* 3. 正常捕获黑线 */
    s_lost_ticks = 0U;

    /* 若从搜线状态重新找回黑线，先低速过渡运行一段周期 */
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

    /* 低速交接期档位上报为 RECOVER */
    if (s_recover_ticks > 0U && s_state != TRACK_STATE_PIVOT) {
        s_state = TRACK_STATE_RECOVER;
    }
}
