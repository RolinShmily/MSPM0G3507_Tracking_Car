#ifndef __KEY_H
#define __KEY_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

#define KEY_NONE    0
#define KEY_1       1   /* PA28 */

/* 按键事件枚举 */
typedef enum {
    KEY_EVENT_NONE = 0,        /* 无按键事件 */
    KEY_EVENT_SHORT_PRESS,     /* 短按事件 (按下时间 < 500ms) */
    KEY_EVENT_LONG_PRESS       /* 长按事件 (按下时间 >= 1000ms) */
} KeyEvent_t;

/**
 * @brief 实时读取按键引脚电平
 * @return KEY_NONE(0), KEY_1(PA28按下)
 */
uint8_t Key_GetData_RealTime(void);

/**
 * @brief 软件延时消抖模式读取按键
 * @return KEY_NONE(0), KEY_1(PA28有效按下)
 */
uint8_t Key_GetData_Debounce(void);

/**
 * @brief 按键状态机滴答处理（由 1ms 滴答定时器中断周期调用）
 */
void Key_Tick_Handler(void);

/**
 * @brief 获取按键触发事件（读取后自动清空事件）
 * @return KeyEvent_t 当前产生的按键事件
 */
KeyEvent_t Key_GetEvent(void);

/**
 * @brief 获取最后一次触发事件的物理按键编号
 * @return KEY_NONE(0), KEY_1(PA28)
 */
uint8_t Key_GetLastKey(void);

#endif /* __KEY_H */
