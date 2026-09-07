#ifndef __ENCODER_H
#define __ENCODER_H

#include "ti_msp_dl_config.h"

/* 编码器配置参数 */
#define ENCODER_GEAR_RATIO      20      /* 减速比 1:20 (电机轴:输出轴) */
#define ENCODER_PPR             1       /* 编码器每转脉冲数 (电机轴), 请按实际编码器修改 */
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

#endif /* __ENCODER_H */
