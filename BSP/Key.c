#include "Key.h"
#include "Tick.h"

/* 按键状态机内部状态枚举 */
typedef enum {
    KEY_STATE_IDLE = 0,        /* 空闲状态（等待按键按下） */
    KEY_STATE_DEBOUNCE,        /* 消抖状态 */
    KEY_STATE_PRESSED,         /* 确认按下状态（开始计时） */
    KEY_STATE_LONG_HELD        /* 长按保持状态（等待释放） */
} KeyState_t;

/* 全局事件与状态机内部变量 */
static volatile KeyEvent_t g_key_event  = KEY_EVENT_NONE;
static volatile KeyState_t g_key_state  = KEY_STATE_IDLE;
static volatile uint32_t   g_press_ticks = 0;
static volatile uint8_t    g_last_key   = KEY_NONE;
static volatile uint8_t    g_current_pressed_key = KEY_NONE;

/**
 * @brief 实时读取当前按键引脚电平
 * @return KEY_NONE(0), KEY_1(PA28按下), KEY_2(PA18按下)
 */
uint8_t Key_GetData_RealTime(void)
{
    /* 检查 PA28 (KEY1) */
    if (DL_GPIO_readPins(KEY1_PORT, KEY1_PIN_28_PIN) == 0) {
        return KEY_1;
    }
    /* 检查 PA18 (KEY2) */
    if (DL_GPIO_readPins(KEY1_PORT, KEY1_PIN_18_PIN) == 0) {
        return KEY_2;
    }
    return KEY_NONE;
}

/**
 * @brief 软件延时消抖模式读取按键
 * @return KEY_NONE(0), KEY_1(PA28有效按下), KEY_2(PA18有效按下)
 */
uint8_t Key_GetData_Debounce(void)
{
    /* 1. 检查 PA28 */
    if (DL_GPIO_readPins(KEY1_PORT, KEY1_PIN_28_PIN) == 0) {
        delay_ms(20); /* 硬件消抖 20ms */
        if (DL_GPIO_readPins(KEY1_PORT, KEY1_PIN_28_PIN) == 0) {
            while (DL_GPIO_readPins(KEY1_PORT, KEY1_PIN_28_PIN) == 0); /* 等待释放 */
            delay_ms(10);
            return KEY_1;
        }
    }

    /* 2. 检查 PA18 */
    if (DL_GPIO_readPins(KEY1_PORT, KEY1_PIN_18_PIN) == 0) {
        delay_ms(20); /* 硬件消抖 20ms */
        if (DL_GPIO_readPins(KEY1_PORT, KEY1_PIN_18_PIN) == 0) {
            while (DL_GPIO_readPins(KEY1_PORT, KEY1_PIN_18_PIN) == 0); /* 等待释放 */
            delay_ms(10);
            return KEY_2;
        }
    }

    return KEY_NONE;
}

/**
 * @brief 1ms 中断驱动的按键状态机
 */
void Key_Tick_Handler(void)
{
    uint8_t current_key = Key_GetData_RealTime();

    switch (g_key_state) {
        case KEY_STATE_IDLE:
            if (current_key != KEY_NONE) {
                g_current_pressed_key = current_key;
                g_key_state = KEY_STATE_DEBOUNCE;
                g_press_ticks = 0;
            }
            break;

        case KEY_STATE_DEBOUNCE:
            if (current_key != KEY_NONE && current_key == g_current_pressed_key) {
                g_press_ticks++;
                if (g_press_ticks >= 20) { /* 20ms 消抖通过 */
                    g_key_state = KEY_STATE_PRESSED;
                }
            } else {
                g_key_state = KEY_STATE_IDLE;
                g_press_ticks = 0;
                g_current_pressed_key = KEY_NONE;
            }
            break;

        case KEY_STATE_PRESSED:
            if (current_key != KEY_NONE && current_key == g_current_pressed_key) {
                g_press_ticks++;
                if (g_press_ticks >= 1000) { /* 达到 1000ms 判定为长按 */
                    g_last_key  = g_current_pressed_key;
                    g_key_event = KEY_EVENT_LONG_PRESS;
                    g_key_state = KEY_STATE_LONG_HELD;
                }
            } else {
                /* 1000ms 前释放 */
                if (g_press_ticks < 500) {
                    g_last_key  = g_current_pressed_key;
                    g_key_event = KEY_EVENT_SHORT_PRESS;
                } else {
                    g_key_event = KEY_EVENT_NONE;
                }
                g_key_state = KEY_STATE_IDLE;
                g_press_ticks = 0;
                g_current_pressed_key = KEY_NONE;
            }
            break;

        case KEY_STATE_LONG_HELD:
            if (current_key == KEY_NONE) {
                g_key_state = KEY_STATE_IDLE;
                g_press_ticks = 0;
                g_current_pressed_key = KEY_NONE;
            }
            break;

        default:
            g_key_state = KEY_STATE_IDLE;
            g_press_ticks = 0;
            g_current_pressed_key = KEY_NONE;
            break;
    }
}

/**
 * @brief 获取按键事件（读后自动清空）
 */
KeyEvent_t Key_GetEvent(void)
{
    KeyEvent_t evt = g_key_event;
    g_key_event = KEY_EVENT_NONE;
    return evt;
}

/**
 * @brief 获取最后一次触发事件的按键编号
 */
uint8_t Key_GetLastKey(void)
{
    return g_last_key;
}
