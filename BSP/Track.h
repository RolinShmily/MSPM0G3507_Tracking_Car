#ifndef __TRACK_H
#define __TRACK_H

#include <stdint.h>

/**
 * @file    Track.h
 * @brief   八路红外灰度加权质心循迹与分级转向控制模块
 *
 * 探头物理映射 (车头正视, 左 -> 右):
 *     OUT8   OUT7   OUT6   OUT5   OUT4   OUT3   OUT2   OUT1
 *     bit7   bit6   bit5   bit4   bit3   bit2   bit1   bit0
 *     0x80   0x40   0x20   0x10   0x08   0x04   0x02   0x01   (黑线=1, 浅色底=0)
 *
 * 坐标与物理量约定:
 *   - OUT1 为车身最右侧探头, OUT8 为最左侧探头
 *   - 偏差定义: 正值表示黑线在小车右侧 (需右转修正), 负值表示黑线在左侧
 *   - 轮速定义: 轮周长 12cm => 1 RPM = 0.2 cm/s
 *     前进速度   v = 0.2 * (L + R) / 2          [cm/s]
 *     自转角速度 w = 0.2 * (L - R) / W          [rad/s] (W 为轮距)
 *   - 当 L > R 时小车顺时针(右转)修正; 定轴自旋时 (L = -R) 前进速度为 0，实现零半径原地转向。
 */

/* ==================== 1. 偏差换算 (加权质心) ==================== */

/* 各探头位置权重, 单位 0.5cm:
 *     OUT1 +3.5  OUT2 +2.5  OUT3 +1.5  OUT4 +0.5
 *     OUT5 -0.5  OUT6 -1.5  OUT7 -2.5  OUT8 -3.5   [cm]
 * 权重乘 2 整数化: +7, +5, +3, +1, -1, -3, -5, -7
 *
 * 偏差计算公式:
 *     pos2 = Σ(w2_i * b_i) / Σb_i   (单位: 0.5cm, 范围: -7 ~ +7 即 -3.5cm ~ +3.5cm)
 */
#define TRACK_POS2_PER_CM       2       /* pos2 单位: 2 代表 1.0cm (即每单位 0.5cm) */
#define TRACK_POS2_TO_MM        5       /* pos2 * 5 = 物理偏差毫米数 (遥测与 OLED 显示用) */

/* ==================== 2. 控制参数 ==================== */

#define TRACK_SPEED_BASE        100     /* 直道巡航基准转速 (RPM), 串口 TSPD= 可动态修改 */
#define TRACK_SPEED_PIVOT       40      /* 直角弯原地定轴自旋转速幅值 (L=-40, R=+40) */
#define TRACK_SPEED_RECOVER     35      /* 找线成功后交接过渡基准转速 (RPM) */

#define TRACK_TURN_GAIN_PCT     35      /* 比例增益: 差速 = 35% * 基速 * |偏差|(cm) */
#define TRACK_TURN_MAX          40      /* 差速幅值上限 (RPM), 保证各档位转向角速度连续 */
#define TRACK_CURVE_BASE_PCT    70      /* 弯道档巡航基速 = 基速 * 70% */

/* 档位阈值 (单位: 0.5cm) */
#define TRACK_CURVE_ENTER2      3       /* |pos2| >= 3 (1.5cm): 进入圆弧/连续弯档 */
#define TRACK_PIVOT_ENTER2      4       /* |pos2| >= 4 (2.0cm): 进入直角弯原地定轴自旋 */
#define TRACK_PIVOT_EXIT2       2       /* |pos2| <= 2 (1.0cm): 退出自旋迟滞阈值 (防止临界抖动) */

/* 时间阈值 (以 SysTick 10ms 为 1 拍) */
#define TRACK_DEBOUNCE_TICKS    2       /* 图案连续 2 拍一致方确认为有效采样 (~20ms 消抖) */
#define TRACK_LOST_TICKS        5       /* 全白持续 5 拍 (50ms) 判定为脱线, 进入找回 */
#define TRACK_SEARCH_MAX_TICKS  500     /* 自旋搜线超时上限 5.0s (约 360°), 超时停车报警 */
#define TRACK_RECOVER_TICKS     50      /* 找线成功后低速运行 0.5s 平稳过渡 */

/**
 * 循迹控制律对照表:
 *
 *   档位          判定条件        基准转速       转向差速 turn            双轮目标速度
 *   直道          mag <= 2        基速           0.35 * 基速 * 偏差(cm)    L=基速+turn, R=基速-turn
 *   连续弯        mag == 3        基速 * 70%     同上 (限幅 40)            L=基速+turn, R=基速-turn
 *   直角弯        mag >= 4        0 (原地转向)   定轴自旋 ±40             (L, R) = (+40, -40) 或反向
 *   脱线找回      全白 > 50ms     0 (原地搜线)   朝最后见线方向自旋 ±40    (L, R) = (±40, ?40)
 */

/**
 * 循迹状态与档位定义
 */
typedef enum {
    TRACK_STATE_IDLE    = 0,    /* 未使能 */
    TRACK_STATE_LINE    = 1,    /* 直道巡航: |pos2| <= 2 (1.0cm 内), 比例差速 */
    TRACK_STATE_CURVE   = 2,    /* 连续弯道: |pos2| = 3 (1.5cm), 降速差速 */
    TRACK_STATE_PIVOT   = 3,    /* 直角弯道: |pos2| >= 4 (2.0cm), 原地定轴自旋 */
    TRACK_STATE_SEARCH  = 4,    /* 脱线搜线: 全白超 50ms, 原地自旋寻线 */
    TRACK_STATE_RECOVER = 5,    /* 找回过渡: 低速 35RPM 维持 0.5s 后回切 */
    TRACK_STATE_ALARM   = 6     /* 搜线超时: 停车锁定并声光报警 */
} Track_State_e;

/* ==================== 3. 外部调用接口 ==================== */

/**
 * @brief 循迹模块初始化 (默认关闭)
 */
void Track_Init(void);

/**
 * @brief 使能/关闭循迹
 * @param enable 1: 使能 (状态清零重新开始); 0: 关闭并清零左右轮目标
 */
void Track_Enable(uint8_t enable);

/**
 * @brief 查询循迹是否使能
 * @return uint8_t 1: 使能, 0: 关闭
 */
uint8_t Track_IsEnabled(void);

/**
 * @brief 循迹周期处理函数, 由 Tick.c 每 10ms 调用一次
 *        内部完成: 采样 -> 消抖 -> 质心偏差解算 -> 控制律执行 -> 轮速目标更新
 */
void Track_Process(void);

/**
 * @brief 设置直道基准巡航转速 (RPM), 内部限幅 [30, 250]
 * @param speed 基准转速 (RPM)
 */
void Track_SetBaseSpeed(int32_t speed);

/**
 * @brief 查询直道基准转速 (RPM)
 * @return int32_t 基准转速
 */
int32_t Track_GetBaseSpeed(void);

/**
 * @brief 查询当前循迹状态/档位
 * @return Track_State_e 当前状态
 */
Track_State_e Track_GetState(void);

/**
 * @brief 查询最近一次偏差值 (毫米, 正=线在右, 负=线在左)
 * @return int16_t 偏差毫米数
 */
int16_t Track_GetPosMM(void);

/**
 * @brief 查询当前消抖后的 8 路探头图案 (bit0=OUT1 最右 .. bit7=OUT8 最左)
 * @return uint8_t 图案掩码
 */
uint8_t Track_GetSensor(void);

#endif /* __TRACK_H */
