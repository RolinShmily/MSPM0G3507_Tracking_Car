#ifndef __ENCODER_H
#define __ENCODER_H

#include "ti_msp_dl_config.h"

/* 编码器配置参数 */
#define ENCODER_GEAR_RATIO      20      /* 减速比 1:20 (电机轴:输出轴) */
#define ENCODER_PPR             11      /* 编码器每转脉冲数 (电机轴), 实测标定: 手转输出轴5圈计数1143, 1143/100≈11线 */
#define ENCODER_SAMPLE_MS       10      /* 速度采样周期 (ms), 由 SysTick 1ms 中断驱动 */
#define ENCODER_SAMPLES_PER_SEC (1000 / ENCODER_SAMPLE_MS)

/**
 * @brief 编码器模块初始化: 清零脉冲计数, 使能 GROUP1 GPIO 中断 (GPIOA/GPIOB)
 */
void Encoder_Init(void);

/**
 * @brief 编码器周期处理函数, 需在 SysTick_Handler 中每 1ms 调用一次
 *        每 10ms 统计一次脉冲增量, 每 1s 换算一次输出轴转速 (RPM)
 */
void Encoder_Tick_Handler(void);

/**
 * @brief 获取左轮 1s 窗口转速 (RPM, 带符号: >0 正转, <0 反转)
 * @return int 左轮转速 (RPM)
 */
int Encoder_GetLRPM(void);

/**
 * @brief 获取右轮 1s 窗口转速 (RPM, 带符号: >0 正转, <0 反转)
 * @return int 右轮转速 (RPM)
 */
int Encoder_GetRRPM(void);

/**
 * @brief 获取左轮最近一个 10ms 采样窗的脉冲增量 (带符号)
 * @note  PID 闭环的快速速度反馈源: 换算 RPM = 增量*6000/(减速比*PPR)
 * @return int32_t 左轮 10ms 脉冲增量
 */
int32_t Encoder_GetL10msDelta(void);

/**
 * @brief 获取右轮最近一个 10ms 采样窗的脉冲增量 (带符号)
 * @return int32_t 右轮 10ms 脉冲增量
 */
int32_t Encoder_GetR10msDelta(void);

/**
 * @brief 读取左右轮累计脉冲计数并清零 (PPR 实测用)
 * @note  读数 = 自上次调用以来的原始脉冲数 (带符号, 由转向决定正负)。
 *        手动转动输出轴一整圈, 读数绝对值 / 减速比(20) = PPR
 * @param rl 输出: 左轮累计脉冲数
 * @param rr 输出: 右轮累计脉冲数
 */
void Encoder_GetAndClearCNT(int32_t *rl, int32_t *rr);

#endif /* __ENCODER_H */
