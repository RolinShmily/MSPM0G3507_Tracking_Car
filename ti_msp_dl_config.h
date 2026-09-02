/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     *  ======== SYSCFG_DL_init ========
     *  Perform all required MSP DL initialization
     *
     *  This function should be called once at a point before any use of
     *  MSP DL.
     */

    /* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM_LED */
#define PWM_LED_INST                                                       TIMG6
#define PWM_LED_INST_IRQHandler                                 TIMG6_IRQHandler
#define PWM_LED_INST_INT_IRQN                                   (TIMG6_INT_IRQn)
#define PWM_LED_INST_CLK_FREQ                                              40000
/* GPIO defines for channel 1 */
#define GPIO_PWM_LED_C1_PORT                                               GPIOB
#define GPIO_PWM_LED_C1_PIN                                       DL_GPIO_PIN_27
#define GPIO_PWM_LED_C1_IOMUX                                    (IOMUX_PINCM58)
#define GPIO_PWM_LED_C1_IOMUX_FUNC                   IOMUX_PINCM58_PF_TIMG6_CCP1
#define GPIO_PWM_LED_C1_IDX                                  DL_TIMER_CC_1_INDEX

/* Defines for PWM_MOTO */
#define PWM_MOTO_INST                                                      TIMG8
#define PWM_MOTO_INST_IRQHandler                                TIMG8_IRQHandler
#define PWM_MOTO_INST_INT_IRQN                                  (TIMG8_INT_IRQn)
#define PWM_MOTO_INST_CLK_FREQ                                             40000
/* GPIO defines for channel 0 */
#define GPIO_PWM_MOTO_C0_PORT                                              GPIOB
#define GPIO_PWM_MOTO_C0_PIN                                      DL_GPIO_PIN_15
#define GPIO_PWM_MOTO_C0_IOMUX                                   (IOMUX_PINCM32)
#define GPIO_PWM_MOTO_C0_IOMUX_FUNC                  IOMUX_PINCM32_PF_TIMG8_CCP0
#define GPIO_PWM_MOTO_C0_IDX                                 DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_MOTO_C1_PORT                                              GPIOB
#define GPIO_PWM_MOTO_C1_PIN                                      DL_GPIO_PIN_16
#define GPIO_PWM_MOTO_C1_IOMUX                                   (IOMUX_PINCM33)
#define GPIO_PWM_MOTO_C1_IOMUX_FUNC                  IOMUX_PINCM33_PF_TIMG8_CCP1
#define GPIO_PWM_MOTO_C1_IDX                                 DL_TIMER_CC_1_INDEX



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMG0)
#define TIMER_0_INST_IRQHandler                                 TIMG0_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMG0_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                         (39999U)




/* Port definition for Pin Group KEY1 */
#define KEY1_PORT                                                        (GPIOA)

/* Defines for PIN_28: GPIOA.28 with pinCMx 3 on package pin 35 */
#define KEY1_PIN_28_PIN                                         (DL_GPIO_PIN_28)
#define KEY1_PIN_28_IOMUX                                         (IOMUX_PINCM3)
/* Defines for PIN_18: GPIOA.18 with pinCMx 40 on package pin 11 */
// pins affected by this interrupt request:["PIN_18"]
#define KEY1_INT_IRQN                                           (GPIOA_INT_IRQn)
#define KEY1_INT_IIDX                           (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define KEY1_PIN_18_IIDX                                    (DL_GPIO_IIDX_DIO18)
#define KEY1_PIN_18_PIN                                         (DL_GPIO_PIN_18)
#define KEY1_PIN_18_IOMUX                                        (IOMUX_PINCM40)
/* Port definition for Pin Group MOTO */
#define MOTO_PORT                                                        (GPIOB)

/* Defines for PIN_22_AIN3: GPIOB.22 with pinCMx 50 on package pin 21 */
#define MOTO_PIN_22_AIN3_PIN                                    (DL_GPIO_PIN_22)
#define MOTO_PIN_22_AIN3_IOMUX                                   (IOMUX_PINCM50)
/* Defines for PIN_23_AIN4: GPIOB.23 with pinCMx 51 on package pin 22 */
#define MOTO_PIN_23_AIN4_PIN                                    (DL_GPIO_PIN_23)
#define MOTO_PIN_23_AIN4_IOMUX                                   (IOMUX_PINCM51)
/* Defines for PIN_25_BIN3: GPIOB.25 with pinCMx 56 on package pin 27 */
#define MOTO_PIN_25_BIN3_PIN                                    (DL_GPIO_PIN_25)
#define MOTO_PIN_25_BIN3_IOMUX                                   (IOMUX_PINCM56)
/* Defines for PIN_26_BIN4: GPIOB.26 with pinCMx 57 on package pin 28 */
#define MOTO_PIN_26_BIN4_PIN                                    (DL_GPIO_PIN_26)
#define MOTO_PIN_26_BIN4_IOMUX                                   (IOMUX_PINCM57)

    /* clang-format on */

    void SYSCFG_DL_init(void);
    void SYSCFG_DL_initPower(void);
    void SYSCFG_DL_GPIO_init(void);
    void SYSCFG_DL_SYSCTL_init(void);
    void SYSCFG_DL_PWM_LED_init(void);
    void SYSCFG_DL_PWM_MOTO_init(void);
    void SYSCFG_DL_TIMER_0_init(void);

    void SYSCFG_DL_SYSTICK_init(void);

    bool SYSCFG_DL_saveConfiguration(void);
    bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
