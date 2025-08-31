/**
 * @file hw_comprehensive_test.h
 * @brief RTOS硬件抽象层综合测试程序
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_HW_COMPREHENSIVE_TEST_H__
#define __RTOS_HW_COMPREHENSIVE_TEST_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 测试套件定义 */
typedef enum {
    RTOS_TEST_SUITE_POWER = 0,      /**< 电源管理测试 */
    RTOS_TEST_SUITE_MEMORY,         /**< 内存管理测试 */
    RTOS_TEST_SUITE_WATCHDOG,       /**< 看门狗测试 */
    RTOS_TEST_SUITE_GPIO,           /**< GPIO测试 */
    RTOS_TEST_SUITE_UART,           /**< UART测试 */
    RTOS_TEST_SUITE_TIMER,          /**< 定时器测试 */
    RTOS_TEST_SUITE_INTEGRATION,    /**< 集成测试 */
    RTOS_TEST_SUITE_MAX
} rtos_test_suite_t;

/* 测试结果结构 */
typedef struct {
    rtos_test_suite_t suite;
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    uint32_t execution_time_ms;
    float pass_rate;
    const char *suite_name;
} rtos_test_result_t;

/* 综合测试统计 */
typedef struct {
    rtos_test_result_t suite_results[RTOS_TEST_SUITE_MAX];
    uint32_t total_suites;
    uint32_t passed_suites;
    uint32_t failed_suites;
    uint32_t total_tests;
    uint32_t total_passed;
    uint32_t total_failed;
    uint32_t total_execution_time_ms;
    float overall_pass_rate;
} rtos_comprehensive_test_stats_t;

/**
 * @brief 运行所有硬件抽象层测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_run_comprehensive_tests(void);

/**
 * @brief 运行指定测试套件
 * @param suite 测试套件
 * @return 测试结果
 */
rtos_result_t rtos_hw_run_test_suite(rtos_test_suite_t suite);

/**
 * @brief 运行电源管理测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_power_management(void);

/**
 * @brief 运行内存管理测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_memory_management(void);

/**
 * @brief 运行看门狗测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_watchdog(void);

/**
 * @brief 运行GPIO测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_gpio(void);

/**
 * @brief 运行UART测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_uart(void);

/**
 * @brief 运行集成测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_integration(void);

/**
 * @brief 运行DMA测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_dma(void);

/**
 * @brief 运行SPI测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_spi(void);

/**
 * @brief 运行I2C测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_i2c(void);

/**
 * @brief 运行ADC/DAC测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_adc_dac(void);

/**
 * @brief 运行安全模块测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_security(void);

/**
 * @brief 运行网络模块测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_network(void);

/**
 * @brief 运行OTA模块测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_ota(void);

/**
 * @brief 运行调试工具测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_test_debug_tools(void);

/**
 * @brief 获取综合测试统计信息
 * @return 测试统计结构指针
 */
const rtos_comprehensive_test_stats_t* rtos_hw_get_comprehensive_test_stats(void);

/**
 * @brief 生成测试报告
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_hw_generate_test_report(char *buffer, uint32_t size);

/**
 * @brief 性能基准测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_run_performance_benchmark(void);

/**
 * @brief 压力测试
 * @param duration_ms 测试持续时间
 * @return 测试结果
 */
rtos_result_t rtos_hw_run_stress_test(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_HW_COMPREHENSIVE_TEST_H__ */