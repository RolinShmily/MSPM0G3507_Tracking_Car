

#include "ti_msp_dl_config.h"
#include "BSP/Tick.h"
#include "BSP/USART.h"
#include "BSP/Motor.h"
#include "BSP/Encoder.h"
#include "BSP/SpeedCtrl.h"
#include "BSP/OLED.h"
#include "BSP/Gray.h"
#include "BSP/Track.h"
#include "BSP/Key.h"
#include "BSP/Watchdog.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* 1. 初始化系统时钟/GPIO/TIMG8/UART0/I2C0 等外设 (SysConfig 自动生成) */
    SYSCFG_DL_init();

    /* 2. 初始化各模块: 电机驱动、串口通信、OLED 显示、灰度传感器、编码器等 */
    Motor_Init();
    USART_Init();

    /* 3. 初始化硬件独立看门狗 WWDT0 并通过串口上报本次复位来源 */
    Watchdog_Init();
    char rst_info[64];
    snprintf(rst_info, sizeof(rst_info), "[SYS] Boot reset cause: 0x%02X (%s)\r\n",
             (unsigned int)Watchdog_GetResetCause(), Watchdog_GetResetCauseStr());
    USART_Send_Str(rst_info);

    OLED_Init();
    Gray_Init();
    Encoder_Init();
    SpeedCtrl_Init();
    Track_Init();
    Key_Init();

    /* 4. OLED 初始化静态界面显示 */
    OLED_Clear();
    OLED_ShowString(0, 0,  "Gray Sensor 8CH", OLED_8X16);
    OLED_ShowString(0, 18, "8 7 6 5 4 3 2 1", OLED_8X16);
    OLED_Update();

    char gray_str[16];
    char display_str[32];
    uint32_t last_oled_tick = 0;

    while (1)
    {
        /* 硬件看门狗喂狗: 周期性刷新 WWDT0 计数器，若主循环阻塞超时 1.0s 则自动硬件复位 */
        Watchdog_Feed();

        uint32_t now = get_ticks();

        /* 5. 周期刷新 OLED 屏幕: 每 200ms 刷新一次，彻底避开 I2C 阻塞影响串口响应 */
        if (now - last_oled_tick >= 200) {
            last_oled_tick = now;

            /* 实时获取 8 路灰度传感器状态 (例如 "01010101") */
            Gray_GetStatusString(gray_str);

            /* 在 OLED 屏幕实时刷新传感器状态 (探头 8~1 字符间隔显示) */
            sprintf(display_str, "%c %c %c %c %c %c %c %c",
                    gray_str[0], gray_str[1], gray_str[2], gray_str[3],
                    gray_str[4], gray_str[5], gray_str[6], gray_str[7]);
            OLED_ShowString(0, 36, display_str, OLED_8X16);

            /* 显示循迹观测量: 偏差(mm, 正=线在右) / 档位 / 左右轮目标 RPM */
            sprintf(display_str, "P%+4d B%d L%+4d R%+4d",
                    Track_GetPosMM(), (int)Track_GetState(),
                    SpeedCtrl_GetTargetL(), SpeedCtrl_GetTargetR());
            OLED_ShowString(0, 54, display_str, OLED_6X8);

            OLED_Update();
        }

        /* 6. 按键切换: 切换循迹闭环启停 (Scheme B) */
        KeyEvent_t key_evt = Key_GetEvent();
        if (key_evt != KEY_EVENT_NONE) {
            if (Track_IsEnabled()) {
                Track_Enable(0);
                SpeedCtrl_Enable(0);
                USART_Send_Str("[KEY] Track Stopped\r\n");
            } else {
                SpeedCtrl_Enable(1);
                Track_Enable(1);
                USART_Send_Str("[KEY] Track Started\r\n");
            }
        }

        /* 7. 帧处理: 解析+执行串口指令并控制电机 (主循环非阻塞响应，避开中断) */
        USART_ProcessFrame();

        /* 8. 周期性查询并通过串口上报电机当前状态 (200ms 一次) */
        USART_Poll_MotorStatus(200);

        /* 9. 主循环单次轮询延时 1ms, 保证串口指令即时响应 */
        delay_ms(1);
    }
}
