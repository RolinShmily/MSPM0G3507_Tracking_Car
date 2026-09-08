#include "Track.h"
#include "Gray.h"
#include "SpeedCtrl.h"
#include "Tick.h"

/* 传感器权值表 (对应 bit 0 ~ bit 7)
 * bit 0: OUT1 (最右) -> +7
 * bit 1: OUT2        -> +5
 * bit 2: OUT3        -> +3
 * bit 3: OUT4 (右中) -> +1
 * bit 4: OUT5 (左中) -> -1
 * bit 5: OUT6        -> -3
 * bit 6: OUT7        -> -5
 * bit 7: OUT8 (最左) -> -7
 */
static const float s_weights[8] = {
    +7.0f,  /* bit 0: OUT1 (far right) */
    +5.0f,  /* bit 1: OUT2 */
    +3.0f,  /* bit 2: OUT3 */
    +1.0f,  /* bit 3: OUT4 (center right) */
    -1.0f,  /* bit 4: OUT5 (center left) */
    -3.0f,  /* bit 5: OUT6 */
    -5.0f,  /* bit 6: OUT7 */
    -7.0f   /* bit 7: OUT8 (far left) */
};

static uint8_t      s_enabled          = 0;
static TrackState_t s_state            = TRACK_STATE_INIT;
static int32_t      s_base_speed       = TRACK_BASE_SPEED_DEFAULT;
static float        s_normalized_pos   = 0.0f;
static float        s_last_pos_norm    = 0.0f;
static int8_t       s_last_valid_dir   = 0;    /* -1: 偏左, +1: 偏右, 0: 中间/未知 */
static uint16_t     s_lost_count       = 0;    /* 连续全白(0x00)周期计数 */
static uint8_t      s_cross_locked     = 0;    /* 十字路口直行锁定标志 */
static uint32_t     s_cross_start_tick = 0;    /* 十字路口进入时间戳 (ms) */

void Track_Init(void)
{
    s_enabled          = 0;
    s_state            = TRACK_STATE_INIT;
    s_base_speed       = TRACK_BASE_SPEED_DEFAULT;
    s_normalized_pos   = 0.0f;
    s_last_pos_norm    = 0.0f;
    s_last_valid_dir   = 0;
    s_lost_count       = 0;
    s_cross_locked     = 0;
    s_cross_start_tick = 0;
}

void Track_Enable(uint8_t en)
{
    s_enabled = en ? 1 : 0;
    if (!s_enabled) {
        s_state = TRACK_STATE_STOP;
        SpeedCtrl_SetTargetLR(0, 0);
    } else {
        s_state            = TRACK_STATE_STRAIGHT;
        s_lost_count       = 0;
        s_cross_locked     = 0;
        s_last_pos_norm    = 0.0f;
        s_last_valid_dir   = 0;
    }
}

uint8_t Track_IsEnabled(void)
{
    return s_enabled;
}

void Track_SetBaseSpeed(int32_t rpm)
{
    if (rpm < 0) {
        rpm = 0;
    } else if (rpm > 250) {
        rpm = 250;
    }
    s_base_speed = rpm;
}

int32_t Track_GetBaseSpeed(void)
{
    return s_base_speed;
}

TrackState_t Track_GetState(void)
{
    return s_state;
}

float Track_GetNormalizedPos(void)
{
    return s_normalized_pos;
}

void Track_Process(void)
{
    uint32_t now = get_ticks();

    /* 1. 检查循迹使能状态 */
    if (!s_enabled) {
        s_state = TRACK_STATE_STOP;
        return;
    }

    /* 2. 十字路口锁定保护 (300ms 盲跑直行, 保持 60% 基础转速, 避免横线干扰) */
    if (s_cross_locked) {
        if ((now - s_cross_start_tick) < TRACK_CROSS_LOCK_MS) {
            s_state = TRACK_STATE_CROSS;
            float target = (float)s_base_speed * 0.60f;
            SpeedCtrl_SetTargetLR((int32_t)target, (int32_t)target);
            return;
        } else {
            s_cross_locked = 0;
        }
    }

    /* 3. 读取 8 路灰度传感器电平状态 */
    uint8_t sensor = Gray_ReadByte();

    /* 4. 全白/无信号处理 (sensor == 0x00) */
    if (sensor == 0x00) {
        s_lost_count++;
        if (s_lost_count > 20) {
            /* 连续超过 200ms 无信号, 判定丢线, 执行原地旋转搜线 (±50 RPM) */
            s_state = TRACK_STATE_LOST;
            s_lost_count = 21; /* 防止计数溢出 */

            if (s_last_valid_dir < 0) {
                /* 之前黑线偏左 -> 逆时针原地旋转 CCW (左轮反转, 右轮正转) */
                SpeedCtrl_SetTargetLR(-50, 50);
            } else {
                /* 之前黑线偏右或居中 -> 顺时针原地旋转 CW (左轮正转, 右轮反转) */
                SpeedCtrl_SetTargetLR(50, -50);
            }
        }
        return;
    }

    /* 5. 传感器检测到黑线 (sensor != 0x00) */
    s_lost_count = 0;

    /* 加权质心计算 */
    float sum_w = 0.0f;
    uint8_t active_count = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (sensor & (1 << i)) {
            sum_w += s_weights[i];
            active_count++;
        }
    }

    float pos = sum_w / (float)active_count;
    float pos_norm = pos / 7.0f;
    if (pos_norm > 1.0f) {
        pos_norm = 1.0f;
    } else if (pos_norm < -1.0f) {
        pos_norm = -1.0f;
    }
    s_normalized_pos = pos_norm;

    /* 更新最后有效方向 */
    if (pos_norm < 0.0f) {
        s_last_valid_dir = -1;
    } else if (pos_norm > 0.0f) {
        s_last_valid_dir = 1;
    }

    /* 6. 状态机迁移判定 */
    if (active_count >= 5 && (sensor & 0x18)) {
        /* 十字路口: 触发探头 >= 5 且包含中间探头 OUT4 或 OUT5 */
        s_state = TRACK_STATE_CROSS;
        s_cross_locked = 1;
        s_cross_start_tick = now;
    } else {
        float abs_pos = (pos_norm >= 0.0f) ? pos_norm : -pos_norm;
        if (abs_pos > 0.65f) {
            s_state = TRACK_STATE_SHARP_TURN;
        } else if (abs_pos > 0.25f) {
            s_state = TRACK_STATE_CURVE;
        } else {
            s_state = TRACK_STATE_STRAIGHT;
        }
    }

    /* 7. 目标速度与差速计算 */
    float base = 0.0f;
    float turn = 0.0f;

    switch (s_state) {
        case TRACK_STATE_CROSS: {
            base = (float)s_base_speed * 0.60f;
            turn = 0.0f;
            break;
        }
        case TRACK_STATE_SHARP_TURN: {
            base = (float)s_base_speed * 0.45f;
            float sgn = (pos_norm > 0.0f) ? 1.0f : ((pos_norm < 0.0f) ? -1.0f : 0.0f);
            turn = 80.0f * sgn;
            break;
        }
        case TRACK_STATE_CURVE: {
            base = (float)s_base_speed * 0.70f;
            float kp = 90.0f;
            float kd = 30.0f;
            turn = kp * pos_norm + kd * (pos_norm - s_last_pos_norm);
            break;
        }
        case TRACK_STATE_STRAIGHT: {
            base = (float)s_base_speed * 1.00f;
            float kp = 60.0f;
            float kd = 20.0f;
            turn = kp * pos_norm + kd * (pos_norm - s_last_pos_norm);
            break;
        }
        default:
            base = 0.0f;
            turn = 0.0f;
            break;
    }

    s_last_pos_norm = pos_norm;

    /* 左右轮速度分配:
     * 当黑线在左 (pos_norm < 0, turn < 0): Target_L 减小, Target_R 增大, 车辆左偏修正
     * 当黑线在右 (pos_norm > 0, turn > 0): Target_L 增大, Target_R 减小, 车辆右偏修正
     */
    float target_l = base + turn;
    float target_r = base - turn;

    /* 前向循迹状态目标转速限幅: 保证内侧轮最小速度为 0 (停转), 防止转向时电机反向倒转 */
    if (s_state == TRACK_STATE_STRAIGHT || s_state == TRACK_STATE_CURVE ||
        s_state == TRACK_STATE_SHARP_TURN || s_state == TRACK_STATE_CROSS) {
        if (target_l < 0.0f) target_l = 0.0f;
        if (target_r < 0.0f) target_r = 0.0f;
    }

    SpeedCtrl_SetTargetLR((int32_t)target_l, (int32_t)target_r);
}
