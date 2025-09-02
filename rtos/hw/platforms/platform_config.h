/**
 * @file platform_config.h
 * @brief RTOS多平台配置 - 支持多种硬件平台
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_PLATFORM_CONFIG_H__
#define __RTOS_PLATFORM_CONFIG_H__

#include "../../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 平台特定配置结构 */
typedef struct {
    const char *platform_name;
    uint32_t system_clock_freq;
    uint32_t cpu_clock_freq;
    uint32_t timer_clock_freq;
    uint32_t timer_resolution_ns;
    uint32_t flash_size_kb;
    uint32_t ram_size_kb;
    uint32_t gpio_port_count;
    uint32_t uart_count;
    uint32_t timer_count;
    uint32_t dma_controller_count;
    uint32_t dma_stream_count;
    bool has_fpu;
    bool has_dsp;
    bool has_mpu;
    bool has_cache;
    bool has_dma;
    bool has_crypto;
} rtos_platform_config_t;

/* STM32F4系列配置 */
#if defined(STM32F40_41xxx) || defined(STM32F407xx)
    #define RTOS_PLATFORM_NAME              "STM32F407VGT6"
    #define RTOS_PLATFORM_SYSTEM_CLOCK      168000000
    #define RTOS_PLATFORM_CPU_CLOCK         168000000
    #define RTOS_PLATFORM_TIMER_CLOCK       84000000
    #define RTOS_PLATFORM_TIMER_RESOLUTION  11
    #define RTOS_PLATFORM_FLASH_SIZE        1024
    #define RTOS_PLATFORM_RAM_SIZE          128
    #define RTOS_PLATFORM_CCM_SIZE          64
    #define RTOS_PLATFORM_GPIO_PORTS        9
    #define RTOS_PLATFORM_UART_COUNT        6
    #define RTOS_PLATFORM_TIMER_COUNT       14
    #define RTOS_PLATFORM_DMA_CONTROLLERS   2
    #define RTOS_PLATFORM_DMA_STREAMS       8
    #define RTOS_PLATFORM_HAS_FPU           1
    #define RTOS_PLATFORM_HAS_DSP           1
    #define RTOS_PLATFORM_HAS_MPU           1
    #define RTOS_PLATFORM_HAS_CACHE         0
    #define RTOS_PLATFORM_HAS_DMA           1
    #define RTOS_PLATFORM_HAS_CRYPTO        0

/* STM32F1系列配置 */
#elif defined(STM32F10X_HD) || defined(STM32F103xx)
    #define RTOS_PLATFORM_NAME              "STM32F103VET6"
    #define RTOS_PLATFORM_SYSTEM_CLOCK      72000000
    #define RTOS_PLATFORM_CPU_CLOCK         72000000
    #define RTOS_PLATFORM_TIMER_CLOCK       72000000
    #define RTOS_PLATFORM_TIMER_RESOLUTION  13
    #define RTOS_PLATFORM_FLASH_SIZE        512
    #define RTOS_PLATFORM_RAM_SIZE          64
    #define RTOS_PLATFORM_CCM_SIZE          0
    #define RTOS_PLATFORM_GPIO_PORTS        7
    #define RTOS_PLATFORM_UART_COUNT        5
    #define RTOS_PLATFORM_TIMER_COUNT       8
    #define RTOS_PLATFORM_DMA_CONTROLLERS   2
    #define RTOS_PLATFORM_DMA_STREAMS       7
    #define RTOS_PLATFORM_HAS_FPU           0
    #define RTOS_PLATFORM_HAS_DSP           0
    #define RTOS_PLATFORM_HAS_MPU           0
    #define RTOS_PLATFORM_HAS_CACHE         0
    #define RTOS_PLATFORM_HAS_DMA           1
    #define RTOS_PLATFORM_HAS_CRYPTO        0

/* STM32F7系列配置 */
#elif defined(STM32F767xx) || defined(STM32F7xx)
    #define RTOS_PLATFORM_NAME              "STM32F767VIT6"
    #define RTOS_PLATFORM_SYSTEM_CLOCK      216000000
    #define RTOS_PLATFORM_CPU_CLOCK         216000000
    #define RTOS_PLATFORM_TIMER_CLOCK       108000000
    #define RTOS_PLATFORM_TIMER_RESOLUTION  9
    #define RTOS_PLATFORM_FLASH_SIZE        2048
    #define RTOS_PLATFORM_RAM_SIZE          512
    #define RTOS_PLATFORM_CCM_SIZE          0
    #define RTOS_PLATFORM_GPIO_PORTS        11
    #define RTOS_PLATFORM_UART_COUNT        8
    #define RTOS_PLATFORM_TIMER_COUNT       17
    #define RTOS_PLATFORM_DMA_CONTROLLERS   2
    #define RTOS_PLATFORM_DMA_STREAMS       8
    #define RTOS_PLATFORM_HAS_FPU           1
    #define RTOS_PLATFORM_HAS_DSP           1
    #define RTOS_PLATFORM_HAS_MPU           1
    #define RTOS_PLATFORM_HAS_CACHE         1
    #define RTOS_PLATFORM_HAS_DMA           1
    #define RTOS_PLATFORM_HAS_CRYPTO        1

/* RISC-V配置 */
#elif defined(__riscv)
    #define RTOS_PLATFORM_NAME              "RISC-V Generic"
    #define RTOS_PLATFORM_SYSTEM_CLOCK      100000000
    #define RTOS_PLATFORM_CPU_CLOCK         100000000
    #define RTOS_PLATFORM_TIMER_CLOCK       100000000
    #define RTOS_PLATFORM_TIMER_RESOLUTION  10
    #define RTOS_PLATFORM_FLASH_SIZE        1024
    #define RTOS_PLATFORM_RAM_SIZE          256
    #define RTOS_PLATFORM_CCM_SIZE          0
    #define RTOS_PLATFORM_GPIO_PORTS        8
    #define RTOS_PLATFORM_UART_COUNT        4
    #define RTOS_PLATFORM_TIMER_COUNT       8
    #define RTOS_PLATFORM_DMA_CONTROLLERS   1
    #define RTOS_PLATFORM_DMA_STREAMS       8
    #define RTOS_PLATFORM_HAS_FPU           0
    #define RTOS_PLATFORM_HAS_DSP           0
    #define RTOS_PLATFORM_HAS_MPU           1
    #define RTOS_PLATFORM_HAS_CACHE         0
    #define RTOS_PLATFORM_HAS_DMA           1
    #define RTOS_PLATFORM_HAS_CRYPTO        0

/* x86-64仿真配置 */
#elif defined(__x86_64__)
    #define RTOS_PLATFORM_NAME              "x86-64 Simulation"
    #define RTOS_PLATFORM_SYSTEM_CLOCK      2400000000
    #define RTOS_PLATFORM_CPU_CLOCK         2400000000
    #define RTOS_PLATFORM_TIMER_CLOCK       1000000
    #define RTOS_PLATFORM_TIMER_RESOLUTION  1
    #define RTOS_PLATFORM_FLASH_SIZE        0
    #define RTOS_PLATFORM_RAM_SIZE          1048576
    #define RTOS_PLATFORM_CCM_SIZE          0
    #define RTOS_PLATFORM_GPIO_PORTS        0
    #define RTOS_PLATFORM_UART_COUNT        4
    #define RTOS_PLATFORM_TIMER_COUNT       16
    #define RTOS_PLATFORM_DMA_CONTROLLERS   0
    #define RTOS_PLATFORM_DMA_STREAMS       0
    #define RTOS_PLATFORM_HAS_FPU           1
    #define RTOS_PLATFORM_HAS_DSP           0
    #define RTOS_PLATFORM_HAS_MPU           0
    #define RTOS_PLATFORM_HAS_CACHE         1
    #define RTOS_PLATFORM_HAS_DMA           0
    #define RTOS_PLATFORM_HAS_CRYPTO        1

/* 默认配置 */
#else
    #define RTOS_PLATFORM_NAME              "Generic Platform"
    #define RTOS_PLATFORM_SYSTEM_CLOCK      100000000
    #define RTOS_PLATFORM_CPU_CLOCK         100000000
    #define RTOS_PLATFORM_TIMER_CLOCK       100000000
    #define RTOS_PLATFORM_TIMER_RESOLUTION  10
    #define RTOS_PLATFORM_FLASH_SIZE        512
    #define RTOS_PLATFORM_RAM_SIZE          64
    #define RTOS_PLATFORM_CCM_SIZE          0
    #define RTOS_PLATFORM_GPIO_PORTS        4
    #define RTOS_PLATFORM_UART_COUNT        2
    #define RTOS_PLATFORM_TIMER_COUNT       4
    #define RTOS_PLATFORM_DMA_CONTROLLERS   1
    #define RTOS_PLATFORM_DMA_STREAMS       4
    #define RTOS_PLATFORM_HAS_FPU           0
    #define RTOS_PLATFORM_HAS_DSP           0
    #define RTOS_PLATFORM_HAS_MPU           0
    #define RTOS_PLATFORM_HAS_CACHE         0
    #define RTOS_PLATFORM_HAS_DMA           0
    #define RTOS_PLATFORM_HAS_CRYPTO        0
#endif

/* 平台特性检查宏 */
#define RTOS_PLATFORM_SUPPORTS_FPU()        (RTOS_PLATFORM_HAS_FPU)
#define RTOS_PLATFORM_SUPPORTS_DSP()        (RTOS_PLATFORM_HAS_DSP)
#define RTOS_PLATFORM_SUPPORTS_MPU()        (RTOS_PLATFORM_HAS_MPU)
#define RTOS_PLATFORM_SUPPORTS_CACHE()      (RTOS_PLATFORM_HAS_CACHE)
#define RTOS_PLATFORM_SUPPORTS_DMA()        (RTOS_PLATFORM_HAS_DMA)
#define RTOS_PLATFORM_SUPPORTS_CRYPTO()     (RTOS_PLATFORM_HAS_CRYPTO)

/* 平台资源限制宏 */
#define RTOS_PLATFORM_MAX_GPIO_PORTS        RTOS_PLATFORM_GPIO_PORTS
#define RTOS_PLATFORM_MAX_UART_COUNT        RTOS_PLATFORM_UART_COUNT
#define RTOS_PLATFORM_MAX_TIMER_COUNT       RTOS_PLATFORM_TIMER_COUNT
#define RTOS_PLATFORM_MAX_DMA_CONTROLLERS   RTOS_PLATFORM_DMA_CONTROLLERS
#define RTOS_PLATFORM_MAX_DMA_STREAMS       RTOS_PLATFORM_DMA_STREAMS

/**
 * @brief 获取当前平台配置
 * @return 平台配置结构指针
 */
const rtos_platform_config_t* rtos_platform_get_config(void);

/**
 * @brief 检查平台特性支持
 * @param feature 特性标识
 * @return 是否支持
 */
bool rtos_platform_check_feature(const char *feature);

/**
 * @brief 获取平台信息字符串
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_platform_get_info_string(char *buffer, uint32_t size);

/**
 * @brief 平台相关初始化
 * @return 操作结果
 */
rtos_result_t rtos_platform_init(void);

/**
 * @brief 平台相关反初始化
 * @return 操作结果
 */
rtos_result_t rtos_platform_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_PLATFORM_CONFIG_H__ */