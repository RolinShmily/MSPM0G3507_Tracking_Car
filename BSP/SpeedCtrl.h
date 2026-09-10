#ifndef __SPEEDCTRL_H
#define __SPEEDCTRL_H

#include <stdint.h>

/**
 * 速度闭环控制模块 (参考《电子系统综合实践》报告 5.4 节)
 *
 * 结构: 目标RPM -> 增量式PID(每10ms) -> PWM限幅输出 -> L/R_MOTO_SetSpeed
 * 反馈: 编码器 10ms 脉冲增量 3 拍滑动窗口 (等效 30ms) -> 输出轴 RPM
 *
 * 模式: 闭环模式 (PID 每 10ms 自动调节左右轮 PWM)
 */

/**
 * @brief 速度闭环模块初始化 (默认关闭)
 */
void SpeedCtrl_Init(void);

/**
 * @brief 速度闭环周期处理, 需在 SysTick_Handler 中每 1ms 调用一次
 *        内部使用 ENCODER_SAMPLE_MS (10ms) 分频执行一次 PID 运算与电机输出更新
 */
void SpeedCtrl_Tick_Handler(void);

/**
 * @brief 使能/关闭速度闭环
 * @param en 1: 使能 (复位 PID 后开始闭环); 0: 关闭 (电机立即停转, 清空目标与输出)
 */
void SpeedCtrl_Enable(uint8_t en);

/**
 * @brief 查询闭环是否使能
 * @return uint8_t 1: 使能, 0: 关闭
 */
uint8_t SpeedCtrl_IsEnabled(void);

/**
 * @brief 设置闭环目标转速 (左右轮相同)
 * @param rpm 目标转速 (输出轴 RPM, 带符号)
 */
void SpeedCtrl_SetTarget(int32_t rpm);

/**
 * @brief 分别设置左右轮闭环目标转速
 * @param l_rpm 左轮目标转速 (RPM)
 * @param r_rpm 右轮目标转速 (RPM)
 */
void SpeedCtrl_SetTargetLR(int32_t l_rpm, int32_t r_rpm);

/**
 * @brief 在线设置 PID 参数 (同时作用于左右轮控制器)
 */
void SpeedCtrl_SetParams(float kp, float ki, float kd);

int32_t SpeedCtrl_GetTargetL(void);
int32_t SpeedCtrl_GetTargetR(void);
int32_t SpeedCtrl_GetOutL(void);
int32_t SpeedCtrl_GetOutR(void);

/**
 * @brief 查询 30ms 窗口的反馈转速 (RPM), 用于观察内环跟随情况
 */
int32_t SpeedCtrl_GetFbL(void);
int32_t SpeedCtrl_GetFbR(void);
float   SpeedCtrl_GetKp(void);
float   SpeedCtrl_GetKi(void);
float   SpeedCtrl_GetKd(void);

#endif /* __SPEEDCTRL_H */
