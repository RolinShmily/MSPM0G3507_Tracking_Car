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




/* Defines for OLED */
#define OLED_INST                                                           I2C0
#define OLED_INST_IRQHandler                                     I2C0_IRQHandler
#define OLED_INST_INT_IRQN                                         I2C0_INT_IRQn
#define OLED_BUS_SPEED_HZ                                                 400000
#define GPIO_OLED_SDA_PORT                                                 GPIOA
#define GPIO_OLED_SDA_PIN                                          DL_GPIO_PIN_0
#define GPIO_OLED_IOMUX_SDA                                       (IOMUX_PINCM1)
#define GPIO_OLED_IOMUX_SDA_FUNC                        IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_OLED_SCL_PORT                                                 GPIOA
#define GPIO_OLED_SCL_PIN                                          DL_GPIO_PIN_1
#define GPIO_OLED_IOMUX_SCL                                       (IOMUX_PINCM2)
#define GPIO_OLED_IOMUX_SCL_FUNC                        IOMUX_PINCM2_PF_I2C0_SCL


/* Defines for UART_Debug */
#define UART_Debug_INST                                                    UART0
#define UART_Debug_INST_FREQUENCY                                        4000000
#define UART_Debug_INST_IRQHandler                              UART0_IRQHandler
#define UART_Debug_INST_INT_IRQN                                  UART0_INT_IRQn
#define GPIO_UART_Debug_RX_PORT                                            GPIOA
#define GPIO_UART_Debug_TX_PORT                                            GPIOA
#define GPIO_UART_Debug_RX_PIN                                    DL_GPIO_PIN_11
#define GPIO_UART_Debug_TX_PIN                                    DL_GPIO_PIN_10
#define GPIO_UART_Debug_IOMUX_RX                                 (IOMUX_PINCM22)
#define GPIO_UART_Debug_IOMUX_TX                                 (IOMUX_PINCM21)
#define GPIO_UART_Debug_IOMUX_RX_FUNC                  IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_Debug_IOMUX_TX_FUNC                  IOMUX_PINCM21_PF_UART0_TX
#define UART_Debug_BAUD_RATE                                              (9600)
#define UART_Debug_IBRD_4_MHZ_9600_BAUD                                     (26)
#define UART_Debug_FBRD_4_MHZ_9600_BAUD                                      (3)





/* Port definition for Pin Group KEY1 */
#define KEY1_PORT                                                        (GPIOA)

/* Defines for PIN_28: GPIOA.28 with pinCMx 3 on package pin 35 */
#define KEY1_PIN_28_PIN                                         (DL_GPIO_PIN_28)
#define KEY1_PIN_28_IOMUX                                         (IOMUX_PINCM3)
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
/* Port definition for Pin Group GRAYA */
#define GRAYA_PORT                                                       (GPIOA)

/* Defines for PIN_07_OUT1: GPIOA.7 with pinCMx 14 on package pin 49 */
#define GRAYA_PIN_07_OUT1_PIN                                    (DL_GPIO_PIN_7)
#define GRAYA_PIN_07_OUT1_IOMUX                                  (IOMUX_PINCM14)
/* Defines for PIN_08_OUT2: GPIOA.8 with pinCMx 19 on package pin 54 */
#define GRAYA_PIN_08_OUT2_PIN                                    (DL_GPIO_PIN_8)
#define GRAYA_PIN_08_OUT2_IOMUX                                  (IOMUX_PINCM19)
/* Defines for PIN_18_OUT4: GPIOA.18 with pinCMx 40 on package pin 11 */
#define GRAYA_PIN_18_OUT4_PIN                                   (DL_GPIO_PIN_18)
#define GRAYA_PIN_18_OUT4_IOMUX                                  (IOMUX_PINCM40)
/* Defines for PIN_13_OUT6: GPIOA.13 with pinCMx 35 on package pin 6 */
#define GRAYA_PIN_13_OUT6_PIN                                   (DL_GPIO_PIN_13)
#define GRAYA_PIN_13_OUT6_IOMUX                                  (IOMUX_PINCM35)
/* Defines for PIN_12_OUT7: GPIOA.12 with pinCMx 34 on package pin 5 */
#define GRAYA_PIN_12_OUT7_PIN                                   (DL_GPIO_PIN_12)
#define GRAYA_PIN_12_OUT7_IOMUX                                  (IOMUX_PINCM34)
/* Defines for PIN_22_OUT8: GPIOA.22 with pinCMx 47 on package pin 18 */
#define GRAYA_PIN_22_OUT8_PIN                                   (DL_GPIO_PIN_22)
#define GRAYA_PIN_22_OUT8_IOMUX                                  (IOMUX_PINCM47)
/* Port definition for Pin Group GRAYB */
#define GRAYB_PORT                                                       (GPIOB)

/* Defines for PIN_19_OUT3: GPIOB.19 with pinCMx 45 on package pin 16 */
#define GRAYB_PIN_19_OUT3_PIN                                   (DL_GPIO_PIN_19)
#define GRAYB_PIN_19_OUT3_IOMUX                                  (IOMUX_PINCM45)
/* Defines for PIN_05_OUT5: GPIOB.5 with pinCMx 18 on package pin 53 */
#define GRAYB_PIN_05_OUT5_PIN                                    (DL_GPIO_PIN_5)
#define GRAYB_PIN_05_OUT5_IOMUX                                  (IOMUX_PINCM18)
/* Defines for OA3: GPIOA.31 with pinCMx 6 on package pin 39 */
#define Encoder_OA3_PORT                                                 (GPIOA)
// pins affected by this interrupt request:["OA3","OA4"]
#define Encoder_GPIOA_INT_IRQN                                  (GPIOA_INT_IRQn)
#define Encoder_GPIOA_INT_IIDX                  (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define Encoder_OA3_IIDX                                    (DL_GPIO_IIDX_DIO31)
#define Encoder_OA3_PIN                                         (DL_GPIO_PIN_31)
#define Encoder_OA3_IOMUX                                         (IOMUX_PINCM6)
/* Defines for OA4: GPIOA.29 with pinCMx 4 on package pin 36 */
#define Encoder_OA4_PORT                                                 (GPIOA)
#define Encoder_OA4_IIDX                                    (DL_GPIO_IIDX_DIO29)
#define Encoder_OA4_PIN                                         (DL_GPIO_PIN_29)
#define Encoder_OA4_IOMUX                                         (IOMUX_PINCM4)
/* Defines for OB3: GPIOB.13 with pinCMx 30 on package pin 1 */
#define Encoder_OB3_PORT                                                 (GPIOB)
// pins affected by this interrupt request:["OB3","OB4"]
#define Encoder_GPIOB_INT_IRQN                                  (GPIOB_INT_IRQn)
#define Encoder_GPIOB_INT_IIDX                  (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define Encoder_OB3_IIDX                                    (DL_GPIO_IIDX_DIO13)
#define Encoder_OB3_PIN                                         (DL_GPIO_PIN_13)
#define Encoder_OB3_IOMUX                                        (IOMUX_PINCM30)
/* Defines for OB4: GPIOB.3 with pinCMx 16 on package pin 51 */
#define Encoder_OB4_PORT                                                 (GPIOB)
#define Encoder_OB4_IIDX                                     (DL_GPIO_IIDX_DIO3)
#define Encoder_OB4_PIN                                          (DL_GPIO_PIN_3)
#define Encoder_OB4_IOMUX                                        (IOMUX_PINCM16)

    /* clang-format on */

    void SYSCFG_DL_init(void);
    void SYSCFG_DL_initPower(void);
    void SYSCFG_DL_GPIO_init(void);
    void SYSCFG_DL_SYSCTL_init(void);
    void SYSCFG_DL_PWM_LED_init(void);
    void SYSCFG_DL_PWM_MOTO_init(void);
    void SYSCFG_DL_TIMER_0_init(void);
    void SYSCFG_DL_OLED_init(void);
    void SYSCFG_DL_UART_Debug_init(void);

    void SYSCFG_DL_SYSTICK_init(void);

    bool SYSCFG_DL_saveConfiguration(void);
    bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
