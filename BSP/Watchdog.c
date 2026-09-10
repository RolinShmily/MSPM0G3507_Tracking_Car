#include "Watchdog.h"

static DL_SYSCTL_RESET_CAUSE s_boot_reset_cause;

void Watchdog_Init(void)
{
    /* 1. 记录开机/复位原因 (开机后首次读取有效) */
    s_boot_reset_cause = DL_SYSCTL_getResetCause();

    /* 2. 重置并使能 WWDT0 电源 */
    DL_WWDT_reset(WWDT0);
    DL_WWDT_enablePower(WWDT0);
    delay_cycles(POWER_STARTUP_DELAY);

    /* 3. 调试器暂停时挂起看门狗，避免断点调试时误触发复位 */
    DL_WWDT_setCoreHaltBehavior(WWDT0, DL_WWDT_CORE_HALT_STOP);

    /* 4. 初始化为看门狗模式: 分频 1, 周期 2^15 (1.0s), 睡眠停止, 窗口0关闭期 0% (全开窗) */
    DL_WWDT_initWatchdogMode(WWDT0, DL_WWDT_CLOCK_DIVIDE_1,
        DL_WWDT_TIMER_PERIOD_15_BITS, DL_WWDT_STOP_IN_SLEEP,
        DL_WWDT_WINDOW_PERIOD_0, DL_WWDT_WINDOW_PERIOD_0);
}

void Watchdog_Feed(void)
{
    DL_WWDT_restart(WWDT0);
}

DL_SYSCTL_RESET_CAUSE Watchdog_GetResetCause(void)
{
    return s_boot_reset_cause;
}

const char* Watchdog_GetResetCauseStr(void)
{
    switch (s_boot_reset_cause) {
        case DL_SYSCTL_RESET_CAUSE_POR_HW_FAILURE:
            return "POR_HW_FAIL";
        case DL_SYSCTL_RESET_CAUSE_POR_EXTERNAL_NRST:
            return "POR_EXT_NRST";
        case DL_SYSCTL_RESET_CAUSE_POR_SW_TRIGGERED:
            return "POR_SW";
        case DL_SYSCTL_RESET_CAUSE_BOR_SUPPLY_FAILURE:
            return "BOR_SUPPLY_FAIL";
        case DL_SYSCTL_RESET_CAUSE_BOOTRST_EXTERNAL_NRST:
            return "RESET_BUTTON";
        case DL_SYSCTL_RESET_CAUSE_BOOTRST_SW_TRIGGERED:
            return "BOOT_SW";
        case DL_SYSCTL_RESET_CAUSE_SYSRST_WWDT0_VIOLATION:
            return "WATCHDOG_WWDT0_RESET";
        case DL_SYSCTL_RESET_CAUSE_SYSRST_CPU_LOCKUP_VIOLATION:
            return "CPU_LOCKUP";
        case DL_SYSCTL_RESET_CAUSE_SYSRST_SW_TRIGGERED:
            return "SYS_SW_RESET";
        default:
            return "NORMAL_OR_OTHER";
    }
}
