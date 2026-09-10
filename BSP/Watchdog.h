#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_wwdt.h>
#include <ti/driverlib/m0p/dl_sysctl.h>

/**
 * @brief 初始化硬件独立窗口看门狗 (WWDT0)
 * @note 运行在 Watchdog 模式，时钟源为内部 32kHz LFOSC，
 *       超时周期设定为约 1.0 秒 (2^15 = 32768 个周期)，
 *       开窗比例为 100% (任何时候均可喂狗)，
 *       调试器暂停时看门狗自动挂起。
 */
void Watchdog_Init(void);

/**
 * @brief 喂看门狗 (刷新计数器)
 */
void Watchdog_Feed(void);

/**
 * @brief 获取复位来源状态码
 * @return DL_SYSCTL_RESET_CAUSE
 */
DL_SYSCTL_RESET_CAUSE Watchdog_GetResetCause(void);

/**
 * @brief 获取复位原因的可读字符串描述
 */
const char* Watchdog_GetResetCauseStr(void);

#endif /* __WATCHDOG_H */
