#ifndef __TRACK_H
#define __TRACK_H

#include <stdint.h>

/**
 * 8 路灰度循迹模块 (参考电子系统综合实践)
 *
 * 传感器排布 (从车身左侧到右侧):
 * OUT8 (最左, bit 7, 权值 -7)
 * OUT7 (左三, bit 6, 权值 -5)
 * OUT6 (左二, bit 5, 权值 -3)
 * OUT5 (左中, bit 4, 权值 -1)
 * OUT4 (右中, bit 3, 权值 +1)
 * OUT3 (右二, bit 2, 权值 +3)
 * OUT2 (右三, bit 1, 权值 +5)
 * OUT1 (最右, bit 0, 权值 +7)
 *
 * 归一化偏差 pos_norm 范围 [-1.0, +1.0]:
 * pos_norm < 0: 黑线偏左, 车身需向左修正 (Target_L 减小, Target_R 增大)
 * pos_norm > 0: 黑线偏右, 车身需向右修正 (Target_L 增大, Target_R 减小)
 * pos_norm == 0: 黑线位于车身正中 (OUT4 与 OUT5 之间)
 */

#define TRACK_BASE_SPEED_DEFAULT    100     /* 默认循迹基础转速 (RPM) */
#define TRACK_CROSS_LOCK_MS         300     /* 十字路口锁定直行时间 (ms) */

/**
 * @brief 循迹状态机枚举定义
 */
typedef enum {
    TRACK_STATE_INIT = 0,       /* 初始化阶段 */
    TRACK_STATE_STOP,           /* 停止状态 */
    TRACK_STATE_STRAIGHT,       /* 直道巡航 (|pos_norm| <= 0.25) */
    TRACK_STATE_CURVE,          /* 弯道缓转 (0.25 < |pos_norm| <= 0.65) */
    TRACK_STATE_SHARP_TURN,     /* 急弯大转向 (|pos_norm| > 0.65) */
    TRACK_STATE_CROSS,          /* 十字路口保护直行 */
    TRACK_STATE_LOST            /* 丢线原位寻迹 */
} TrackState_t;

/**
 * @brief 循迹控制模块初始化
 */
void Track_Init(void);

/**
 * @brief 循迹周期处理函数 (由主循环或定时任务周期性调用, 周期约 10ms)
 */
void Track_Process(void);

/**
 * @brief 使能或禁用循迹功能
 * @param en 1: 使能; 0: 禁用 (停止电机并切换为 TRACK_STATE_STOP)
 */
void Track_Enable(uint8_t en);

/**
 * @brief 查询循迹是否使能
 * @return 1: 使能; 0: 禁用
 */
uint8_t Track_IsEnabled(void);

/**
 * @brief 设置循迹基础运行转速
 * @param rpm 基础转速 (RPM, >= 0)
 */
void Track_SetBaseSpeed(int32_t rpm);

/**
 * @brief 获取当前循迹基础运行转速
 * @return 基础转速 (RPM)
 */
int32_t Track_GetBaseSpeed(void);

/**
 * @brief 获取当前循迹状态机状态
 * @return TrackState_t
 */
TrackState_t Track_GetState(void);

/**
 * @brief 获取最新计算得到的归一化偏差位置 [-1.0, +1.0]
 * @return 归一化偏差
 */
float Track_GetNormalizedPos(void);

#endif /* __TRACK_H */
