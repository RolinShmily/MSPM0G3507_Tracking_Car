#ifndef __KEY_H
#define __KEY_H

#include "ti_msp_dl_config.h"

#define KEY_NONE    0
#define KEY_1       1   /* PA28 */
#define KEY_2       2   /* PA18 */

/* 按键事件枚举 */
typedef enum {
    KEY_EVENT_NONE = 0,        /* 无按键事件 */
    KEY_EVENT_SHORT_PRESS,     /* 短按事件 (按下时间 < 500ms) */
    KEY_EVENT_LONG_PRESS       /* 长按事件 (按下时间 >= 1000ms) */
} KeyEvent_t;

/**
 * @brief 实时获取按键引脚电平（即时读取，不阻塞）
 * @return KEY_NONE(0), KEY_1(PA28按下), KEY_2(PA18按下)
 */
uint8_t Key_GetData_RealTime(void);

/**
 * @brief 带软件消抖与松手检测的按键读取函数
 * @return KEY_NONE(0), KEY_1(PA28有效按下), KEY_2(PA18有效按下)
 */
uint8_t Key_GetData_Debounce(void);

/**
 * @brief 按键状态机处理函数（需在 1ms 滴答定时器中断服务函数中周期调用）
 */
void Key_Tick_Handler(void);

/**
 * @brief 获取按键触发事件（读取后自动清空）
 * @return KeyEvent_t 当前触发的按键事件
 */
KeyEvent_t Key_GetEvent(void);

#endif /* __KEY_H */
