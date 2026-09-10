#ifndef __TRACK_H
#define __TRACK_H

#include <stdint.h>

/**
 * 循迹模块: 8 路灰度加权偏差 + 分档比例差速 + 定轴自旋找回
 * ==================================================================
 * 探头映射 (车头正视图, 左 -> 右):
 *     OUT8   OUT7   OUT6   OUT5   OUT4   OUT3   OUT2   OUT1
 *     bit7   bit6   bit5   bit4   bit3   bit2   bit1   bit0
 *     0x80   0x40   0x20   0x10   0x08   0x04   0x02   0x01     (黑=1, 白=0)
 * 即 OUT1 是整车最右侧探头, OUT8 是最左侧 (见 Gray_ReadByte 的 bit0/bit7)。
 * Gray_GetStatusString() 返回的字符串顺序是 OUT8..OUT1, 与 OLED 表头 "8 7 6 5 4 3 2 1" 对应。
 *
 * 【为什么重写: 弃用"事件枚举 + 逐事件速度表"】
 * 老实现把图案硬分成 居中/微偏/中偏/直角族 若干类, 每类配一组固定轮速。它丢掉的是
 * 偏差的**大小**这个连续信息, 于是每冒出一条新轨迹就要加一条分支和一组标定值; 更麻烦
 * 的是直角必须靠"前冲 Xcm 让轴心正好停在拐点"来对齐, 而探头离轴距离、打滑系数只能猜
 * -> 每标错一个常量就多一种歪法(扔头过头 / 转得太早 / 压不上线)。
 *
 * 新实现只做一件事: 把 8 路图案换算成一个连续的横向偏差 pos, 再用**同一条律**驱动左右
 * 轮。整条控制律里没有任何几何标定常量(轮距、探头离轴距离都不出现), 因为直角不是"算
 * 出来的", 而是"转过去 + 丢线找回"闭环出来的。
 *
 * 【找回为什么几乎不会失手】探头在轴心前方约 9cm, 车体绕轴心自旋时 8 个探头扫过的是
 * 一个半径约 9cm 的圆盘。直角的两条线都穿过轴心附近 -> 只要轴心距那条线小于 9cm, 圆盘
 * 必与它相交, 转一圈一定能扫到。所以老实现要求"轴心停在拐点 ±3mm 内"(否则偏差 10° 以上
 * 必然甩出) 的苛刻条件, 在这里被换成了"轴心别离拐点太远"这个宽松条件。
 *
 * 【物理量约定】L/R = 左右轮目标 RPM (正 = 前进), 轮周 12cm => 1 RPM = 0.2 cm/s:
 *     轴心前进速度   v = 0.2*(L+R)/2          [cm/s]
 *     车体自转角速度 w = 0.2*(L-R)/W          [rad/s], 正值 = 右转(顺时针), W = 轮距
 * 即 L > R 车身向右转; 定轴自旋 (L = -R) 时 v = 0, 原地转不产生位移。
 */

/* ==================== 1. 偏差换算 (加权质心) ==================== */

/* 各探头位置权重, 单位 0.5cm (乘 2 取整以避开浮点):
 *     OUT1 +3.5  OUT2 +2.5  OUT3 +1.5  OUT4 +0.5
 *     OUT5 -0.5  OUT6 -1.5  OUT7 -2.5  OUT8 -3.5      [cm]
 * 权重 x2 后: +7 +5 +3 +1 -1 -3 -5 -7
 *
 *     pos2 = Σ(w2_i * b_i) / Σb_i        (单位 0.5cm, 值域 ±7 即 ±3.5cm)
 *
 * 说明: 只要黑的是"相邻若干个"探头, Σw2 一定能被探头数整除 (连续奇数之和的平均数仍是
 * 整数), 所以整数除法不丢精度, 分辨率天然是 0.5cm —— 比单个探头间距 1cm 细一倍。
 * 已知图案: 0x18(中间两个) = 0 | 0x08/0x10(中间单个) = -1/+1 | 0x06(右侧三个) = 5 (2.5cm)
 *           0x1B = 3 (1.5cm) | 0x13 = 3 | 0x0B = 4 (2.0cm)     <- 直角族的典型值
 */
#define TRACK_POS2_PER_CM       2       /* pos2 单位 = 0.5cm */
#define TRACK_POS2_TO_MM        5       /* pos2 * 5 = 毫米 (串口/OLED 上报用) */

/* ==================== 2. 控制参数 ==================== */

#define TRACK_SPEED_BASE        100     /* 直道基速 RPM (20 cm/s), 串口 TSPD= 在线可改 */
#define TRACK_SPEED_PIVOT       40      /* 直角定轴自旋转速幅值 (L=-40,R=+40 => 零前进) */
#define TRACK_SPEED_RECOVER     35      /* 找回成功后的低速交接基速 RPM */

#define TRACK_TURN_GAIN_PCT     35      /* 比例律: 差速 = 35% x 基速 x |偏差|(cm) */
#define TRACK_TURN_MAX          40      /* 差速限幅 RPM (与自旋幅值相同 => 档间角速度连续) */
#define TRACK_CURVE_BASE_PCT    70      /* 弯道档基速 = 基速 x 70% */

/* 档位阈值 (单位 pos2 = 0.5cm) */
#define TRACK_CURVE_ENTER2      3       /* |pos2| >= 3 (1.5cm): 圆弧/连续弯档, 降基速 */
#define TRACK_PIVOT_ENTER2      4       /* |pos2| >= 4 (2.0cm): 直角档, 定轴自旋 */
#define TRACK_PIVOT_EXIT2       2       /* 自旋退出需 |pos2| <= 2 (1.0cm): 滞回, 防在阈值上抖 */

/* 时间阈值 (SysTick 10ms 一拍) */
#define TRACK_DEBOUNCE_TICKS    2       /* 连续 2 拍同一图案才采用 (约 20ms) */
#define TRACK_LOST_TICKS        5       /* 全白连续 5 拍 (50ms) 判定脱线 -> 进入找回 */
#define TRACK_SEARCH_MAX_TICKS  500     /* 找回自旋上限 5.0s 约 360° (轮距按 12cm 估) */
#define TRACK_RECOVER_TICKS     50      /* 找回成功后低速交接 0.5s 再回主律 */

/**
 * 【控制律】记 |偏差|(cm) = |pos2|/2, mag = |pos2|
 *
 *   档位          条件           基速         差速 turn                轮速
 *   直道          mag <= 2       基速         0.35 x 基速 x 偏差(cm)    L=基速+turn, R=基速-turn
 *   圆弧/连续弯   mag == 3       基速 x 70%   同上 (上限 40)            同上
 *   直角          mag >= 4       --           定轴自旋 +-40            (L,R)=(+40,-40) 或反向
 *
 *   方向: 线偏右 (pos>0) 必须右转 => 左轮快、右轮慢 (L = 基速+turn, R = 基速-turn);
 *         直角档同向 (+40,-40)。这与老代码里已在车上验证过的 TURN_R = (40,-40) 一致。
 *
 * 【为什么差速限幅取 40】轮距 W 未知(按 12cm 估), 由 w = 0.2*ΔRPM/W:
 *     直道 |偏差| = 1.0cm : turn = 35 -> Δ=70 -> w 约 67 °/s
 *     弯道 |偏差| = 1.5cm : turn = 36 -> Δ=73 -> w 约 70 °/s
 *     直角 定轴自旋       : turn = 40 -> Δ=80 -> w 约 76 °/s
 *   三档角速度单调连续 (67 -> 70 -> 76 °/s), 换档不会突然变成另一种转法。
 *   老实现的教训正在这里: 出弯一瞬间从大差速直接切到"基速 100 直行", 残余自转仍有
 *   76°/s 而直道档当时只有 28°/s 的修正能力, 于是车身继续转 -> 线滑到最外侧 -> 全白
 *   -> 急停。本律让大偏差时的差速被限在 40 (而不是把它加到 100 上), 保证"转得动但不会
 *   自己冲出去"; 修正能力随偏差连续变化, 不会出现"档位切换瞬间失控"。
 *
 *   调参入口: 若仍有余摆/甩头 -> 先降 TRACK_TURN_GAIN_PCT (35 -> 28), 再降 TRACK_SPEED_BASE;
 *             若入弯发晚/迟钝  -> 抬高 TRACK_TURN_GAIN_PCT 或降低 TRACK_PIVOT_ENTER2。
 */

/**
 * 循迹状态/档位 (串口 TRK 上报值, PC 端脚本按新编号改)
 */
typedef enum {
    TRACK_STATE_IDLE    = 0,    /* 未使能 */
    TRACK_STATE_LINE    = 1,    /* 直道: |pos2| <= 2 (1.0cm 内), 比例差速 */
    TRACK_STATE_CURVE   = 2,    /* 圆弧/连续弯: |pos2| = 3 (1.5cm), 基速 x70% */
    TRACK_STATE_PIVOT   = 3,    /* 直角: |pos2| >= 4 (2.0cm), 定轴自旋 +-40 */
    TRACK_STATE_SEARCH  = 4,    /* 脱线找回: 全白满 50ms, 朝最后见线侧定轴自旋 */
    TRACK_STATE_RECOVER = 5,    /* 找回成功: 低速 35RPM 走 0.5s 再回主律 */
    TRACK_STATE_ALARM   = 6     /* 找回转满上限仍无线: 停车锁存, 按键重启 */
} Track_State_e;

/* ==================== 3. 对外接口 ==================== */

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
 * @brief 循迹周期处理, 由 Tick.c 每 10ms 调用一次 (在 SpeedCtrl_Tick_Handler 之前)
 *        内部完成: 采样 -> 消抖 -> 算偏差 -> 施加控制律 -> SetTargetLR
 */
void Track_Process(void);

/**
 * @brief 设置直道基速 (RPM), 会自动限幅到 30~250
 */
void Track_SetBaseSpeed(int32_t speed);

/**
 * @brief 查询直道基速 (RPM)
 */
int32_t Track_GetBaseSpeed(void);

/**
 * @brief 查询当前档位/状态
 */
Track_State_e Track_GetState(void);

/**
 * @brief 查询最近一次偏差 (毫米, 正 = 线在右侧, 负 = 线在左侧)
 *        串口遥测与 OLED 观测用, 单位 mm 以避免浮点 printf
 */
int16_t Track_GetPosMM(void);

/**
 * @brief 查询消抖后的 8 路图案 (bit0=OUT1 最右 .. bit7=OUT8 最左)
 */
uint8_t Track_GetSensor(void);

#endif /* __TRACK_H */
