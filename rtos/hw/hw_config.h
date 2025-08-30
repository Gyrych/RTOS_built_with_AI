/**
 * @file hw_config.h
 * @brief RTOS硬件配置 - STM32F4特定配置
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_HW_CONFIG_H__
#define __RTOS_HW_CONFIG_H__

#include "../core/types.h"

/* STM32F4硬件配置 */

/* 时钟配置 */
#define RTOS_HW_SYSTEM_CLOCK_FREQ        168000000   /* 168 MHz */
#define RTOS_HW_APB1_CLOCK_FREQ         84000000    /* 84 MHz (APB1) */
#define RTOS_HW_APB2_CLOCK_FREQ         168000000   /* 168 MHz (APB2) */

/* 定时器配置 */
#define RTOS_HW_TIMER_USED               TIM2        /* 使用TIM2作为系统定时器 */
#define RTOS_HW_TIMER_CLOCK_FREQ        84000000    /* TIM2时钟频率 (APB1) */
#define RTOS_HW_TIMER_PRESCALER         0           /* 不分频 */
#define RTOS_HW_TIMER_MAX_PERIOD        0xFFFFFFFF  /* 最大周期值 */

/* 中断优先级配置 */
#define RTOS_HW_TIMER_IRQ_PRIORITY      0           /* TIM2中断优先级 (最高) */
#define RTOS_HW_PENDSV_IRQ_PRIORITY     15          /* PendSV中断优先级 (最低) */
#define RTOS_HW_SVC_IRQ_PRIORITY        1           /* SVC中断优先级 (高) */

/* 时间精度配置 */
#define RTOS_HW_TIMER_RESOLUTION_NS     11          /* 定时器分辨率约11.9ns (84MHz) */
#define RTOS_HW_MIN_TIMER_PERIOD_NS     12          /* 最小定时器周期12ns */
#define RTOS_HW_MAX_TIMER_PERIOD_NS     51000000000ULL /* 最大定时器周期约51秒 */

/* 硬件特性配置 */
#define RTOS_HW_SUPPORT_DWT            1           /* 支持DWT CYCCNT */
#define RTOS_HW_SUPPORT_FPU            1           /* 支持FPU */
#define RTOS_HW_SUPPORT_DSP            1           /* 支持DSP指令 */

/* 内存配置 */
#define RTOS_HW_FLASH_SIZE             1024        /* Flash大小 (KB) */
#define RTOS_HW_SRAM_SIZE              128         /* SRAM大小 (KB) */
#define RTOS_HW_CCM_SIZE               64          /* CCM RAM大小 (KB) */

/* 外设配置 */
#define RTOS_HW_GPIO_COUNT             16          /* GPIO端口数量 */
#define RTOS_HW_ADC_COUNT              3           /* ADC数量 */
#define RTOS_HW_DAC_COUNT              2           /* DAC数量 */
#define RTOS_HW_UART_COUNT             6           /* UART数量 */
#define RTOS_HW_SPI_COUNT              3           /* SPI数量 */
#define RTOS_HW_I2C_COUNT              3           /* I2C数量 */
#define RTOS_HW_CAN_COUNT              2           /* CAN数量 */
#define RTOS_HW_TIMER_COUNT            14          /* 定时器数量 */

/* 低功耗配置 */
#define RTOS_HW_SUPPORT_SLEEP          1           /* 支持睡眠模式 */
#define RTOS_HW_SUPPORT_STOP           1           /* 支持停止模式 */
#define RTOS_HW_SUPPORT_STANDBY        1           /* 支持待机模式 */

/* 调试配置 */
#define RTOS_HW_SUPPORT_SWD            1           /* 支持SWD调试 */
#define RTOS_HW_SUPPORT_JTAG           1           /* 支持JTAG调试 */
#define RTOS_HW_SUPPORT_TRACE          1           /* 支持跟踪功能 */

/* 错误处理配置 */
#define RTOS_HW_ERROR_CHECK_ENABLED    1           /* 启用硬件错误检查 */
#define RTOS_HW_ASSERT_ENABLED         1           /* 启用硬件断言 */
#define RTOS_HW_DEBUG_ENABLED          1           /* 启用硬件调试 */

/* 兼容性配置 */
#define RTOS_HW_STM32F4XX              1           /* STM32F4系列 */
#define RTOS_HW_ARM_CORTEX_M4          1           /* ARM Cortex-M4内核 */
#define RTOS_HW_THUMB2                 1           /* Thumb-2指令集 */

#endif /* __RTOS_HW_CONFIG_H__ */
