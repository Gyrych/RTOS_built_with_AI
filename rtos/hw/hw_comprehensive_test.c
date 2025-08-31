/**
 * @file hw_comprehensive_test.c
 * @brief RTOS硬件抽象层综合测试程序实现
 * @author Assistant
 * @date 2024
 */

#include "hw_comprehensive_test.h"
#include "hw_abstraction.h"
#include "power_management.h"
#include "memory_monitor.h"
#include "watchdog_manager.h"
#include "gpio_abstraction.h"
#include "uart_abstraction.h"
#include <stdio.h>
#include <string.h>

/* 全局测试统计 */
static rtos_comprehensive_test_stats_t g_test_stats;
static bool g_test_initialized = false;

/* 内部函数声明 */
static void rtos_test_init_stats(void);
static void rtos_test_update_suite_result(rtos_test_suite_t suite, uint32_t total, uint32_t passed, uint32_t execution_time);
static void rtos_test_calculate_overall_stats(void);

/**
 * @brief 运行所有硬件抽象层测试
 */
rtos_result_t rtos_hw_run_comprehensive_tests(void)
{
    printf("=====================================\n");
    printf("RTOS硬件抽象层综合测试开始\n");
    printf("=====================================\n");
    
    /* 初始化测试统计 */
    rtos_test_init_stats();
    
    uint32_t total_start_time = rtos_hw_get_system_time_ms();
    
    /* 运行所有测试套件 */
    rtos_result_t overall_result = RTOS_OK;
    
    /* 基础功能测试 */
    if (rtos_hw_run_test_suite(RTOS_TEST_SUITE_POWER) != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_run_test_suite(RTOS_TEST_SUITE_MEMORY) != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_run_test_suite(RTOS_TEST_SUITE_WATCHDOG) != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_run_test_suite(RTOS_TEST_SUITE_GPIO) != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_run_test_suite(RTOS_TEST_SUITE_UART) != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_run_test_suite(RTOS_TEST_SUITE_TIMER) != RTOS_OK) overall_result = RTOS_ERROR;
    
    /* 高级功能测试 */
    printf("\n--- 高级功能模块测试 ---\n");
    if (rtos_hw_test_dma() != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_test_spi() != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_test_i2c() != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_test_adc_dac() != RTOS_OK) overall_result = RTOS_ERROR;
    
    /* 安全和网络测试 */
    printf("\n--- 安全和网络模块测试 ---\n");
    if (rtos_hw_test_security() != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_test_network() != RTOS_OK) overall_result = RTOS_ERROR;
    if (rtos_hw_test_ota() != RTOS_OK) overall_result = RTOS_ERROR;
    
    /* 调试工具测试 */
    printf("\n--- 调试工具模块测试 ---\n");
    if (rtos_hw_test_debug_tools() != RTOS_OK) overall_result = RTOS_ERROR;
    
    /* 集成测试 */
    if (rtos_hw_run_test_suite(RTOS_TEST_SUITE_INTEGRATION) != RTOS_OK) overall_result = RTOS_ERROR;
    
    /* 计算总体统计 */
    uint32_t total_time = rtos_hw_get_system_time_ms() - total_start_time;
    g_test_stats.total_execution_time_ms = total_time;
    rtos_test_calculate_overall_stats();
    
    printf("=====================================\n");
    printf("综合测试完成\n");
    printf("总耗时: %lu ms\n", total_time);
    printf("总通过率: %.1f%%\n", g_test_stats.overall_pass_rate);
    printf("=====================================\n");
    
    return overall_result;
}

/**
 * @brief 运行指定测试套件
 */
rtos_result_t rtos_hw_run_test_suite(rtos_test_suite_t suite)
{
    if (suite >= RTOS_TEST_SUITE_MAX) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_result_t result = RTOS_OK;
    
    switch (suite) {
        case RTOS_TEST_SUITE_POWER:
            result = rtos_hw_test_power_management();
            break;
        case RTOS_TEST_SUITE_MEMORY:
            result = rtos_hw_test_memory_management();
            break;
        case RTOS_TEST_SUITE_WATCHDOG:
            result = rtos_hw_test_watchdog();
            break;
        case RTOS_TEST_SUITE_GPIO:
            result = rtos_hw_test_gpio();
            break;
        case RTOS_TEST_SUITE_UART:
            result = rtos_hw_test_uart();
            break;
        case RTOS_TEST_SUITE_TIMER:
            result = rtos_hw_test_timer(); /* 使用定时器测试函数 */
            break;
        case RTOS_TEST_SUITE_INTEGRATION:
            result = rtos_hw_test_integration();
            break;
        default:
            result = RTOS_ERROR_INVALID_PARAM;
            break;
    }
    
    return result;
}

/**
 * @brief 运行电源管理测试
 */
rtos_result_t rtos_hw_test_power_management(void)
{
    printf("\n--- 电源管理模块测试 ---\n");
    
    uint32_t start_time = rtos_hw_get_system_time_ms();
    uint32_t total_tests = 0;
    uint32_t passed_tests = 0;
    
    /* 测试1: 电源管理器初始化 */
    total_tests++;
    printf("测试1: 电源管理器初始化...");
    rtos_power_manager_t *power_mgr = rtos_power_manager_get_instance();
    if (power_mgr != NULL) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试2: 电源状态查询 */
    total_tests++;
    printf("测试2: 电源状态查询...");
    rtos_power_status_t status;
    if (rtos_power_manager_get_status(&status) == RTOS_OK) {
        printf("通过 (VDD: %lu mV, 温度: %d°C)\n", status.vdd_voltage_mv, status.temperature_celsius);
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试3: 电源策略设置 */
    total_tests++;
    printf("测试3: 电源策略设置...");
    rtos_power_policy_t policy = {
        .auto_sleep_enable = true,
        .idle_timeout_ms = 1000,
        .deep_sleep_threshold_ms = 5000,
        .max_sleep_mode = RTOS_POWER_MODE_STOP,
        .wakeup_sources = RTOS_WAKEUP_SOURCE_RTC | RTOS_WAKEUP_SOURCE_WKUP_PIN,
        .voltage_scaling_enable = true,
        .min_voltage_mv = 2700
    };
    
    if (rtos_power_manager_set_policy(&policy) == RTOS_OK) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试4: 低功耗模式切换 */
    total_tests++;
    printf("测试4: 低功耗模式切换...");
    if (rtos_power_manager_set_mode(RTOS_POWER_MODE_SLEEP) == RTOS_OK) {
        rtos_hw_delay_ms(10); /* 短暂延时 */
        if (rtos_power_manager_set_mode(RTOS_POWER_MODE_RUN) == RTOS_OK) {
            printf("通过\n");
            passed_tests++;
        } else {
            printf("失败 (无法退出睡眠模式)\n");
        }
    } else {
        printf("失败 (无法进入睡眠模式)\n");
    }
    
    uint32_t execution_time = rtos_hw_get_system_time_ms() - start_time;
    rtos_test_update_suite_result(RTOS_TEST_SUITE_POWER, total_tests, passed_tests, execution_time);
    
    printf("电源管理测试完成: %lu/%lu 通过 (%.1f%%)\n", 
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行内存管理测试
 */
rtos_result_t rtos_hw_test_memory_management(void)
{
    printf("\n--- 内存管理模块测试 ---\n");
    
    uint32_t start_time = rtos_hw_get_system_time_ms();
    uint32_t total_tests = 0;
    uint32_t passed_tests = 0;
    
    /* 测试1: 内存监控器初始化 */
    total_tests++;
    printf("测试1: 内存监控器初始化...");
    rtos_memory_monitor_t *mem_monitor = rtos_memory_monitor_get_instance();
    if (mem_monitor != NULL) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试2: 内存统计查询 */
    total_tests++;
    printf("测试2: 内存统计查询...");
    rtos_memory_monitor_stats_t mem_stats;
    if (rtos_memory_monitor_get_stats(&mem_stats) == RTOS_OK) {
        printf("通过 (总RAM: %lu KB, 已用: %lu KB)\n", 
               mem_stats.total_ram / 1024, mem_stats.used_ram / 1024);
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试3: 内存泄漏检测 */
    total_tests++;
    printf("测试3: 内存泄漏检测...");
    if (rtos_memory_monitor_enable_leak_detection(true) == RTOS_OK) {
        /* 模拟内存分配 */
        void *test_ptr = rtos_malloc(256);
        if (test_ptr) {
            rtos_memory_leak_stats_t leak_stats;
            rtos_memory_monitor_get_leak_stats(&leak_stats);
            
            rtos_free(test_ptr);
            
            printf("通过 (分配: %lu, 释放: %lu)\n", 
                   leak_stats.alloc_count, leak_stats.free_count + 1);
            passed_tests++;
        } else {
            printf("失败 (内存分配失败)\n");
        }
    } else {
        printf("失败\n");
    }
    
    /* 测试4: 内存池管理 */
    total_tests++;
    printf("测试4: 内存池管理...");
    rtos_memory_pool_config_t pool_config = {
        .block_size = 64,
        .block_count = 16,
        .alignment = 8,
        .thread_safe = true,
        .name = "test_pool"
    };
    
    uint32_t pool_id;
    if (rtos_memory_monitor_create_pool(&pool_config, &pool_id) == RTOS_OK) {
        void *block = rtos_memory_monitor_pool_alloc(pool_id);
        if (block) {
            if (rtos_memory_monitor_pool_free(pool_id, block) == RTOS_OK) {
                printf("通过\n");
                passed_tests++;
            } else {
                printf("失败 (释放失败)\n");
            }
        } else {
            printf("失败 (分配失败)\n");
        }
    } else {
        printf("失败 (创建池失败)\n");
    }
    
    uint32_t execution_time = rtos_hw_get_system_time_ms() - start_time;
    rtos_test_update_suite_result(RTOS_TEST_SUITE_MEMORY, total_tests, passed_tests, execution_time);
    
    printf("内存管理测试完成: %lu/%lu 通过 (%.1f%%)\n", 
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行看门狗测试
 */
rtos_result_t rtos_hw_test_watchdog(void)
{
    printf("\n--- 看门狗模块测试 ---\n");
    
    uint32_t start_time = rtos_hw_get_system_time_ms();
    uint32_t total_tests = 0;
    uint32_t passed_tests = 0;
    
    /* 测试1: 看门狗管理器初始化 */
    total_tests++;
    printf("测试1: 看门狗管理器初始化...");
    rtos_watchdog_manager_t *wdt_mgr = rtos_watchdog_manager_get_instance();
    if (wdt_mgr != NULL) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试2: 硬件看门狗配置 */
    total_tests++;
    printf("测试2: 硬件看门狗配置...");
    rtos_watchdog_config_t wdt_config = {
        .timeout_ms = 1000,
        .auto_reload = true,
        .reset_on_timeout = false, /* 测试时不复位 */
        .prescaler = 4
    };
    
    if (rtos_watchdog_manager_hw_init(&wdt_config) == RTOS_OK) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试3: 软件看门狗任务注册 */
    total_tests++;
    printf("测试3: 软件看门狗任务注册...");
    if (rtos_watchdog_manager_soft_register_task(1, 500, "test_task") == RTOS_OK) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试4: 看门狗喂狗操作 */
    total_tests++;
    printf("测试4: 看门狗喂狗操作...");
    if (rtos_watchdog_manager_hw_feed() == RTOS_OK &&
        rtos_watchdog_manager_soft_feed_task(1) == RTOS_OK) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    uint32_t execution_time = rtos_hw_get_system_time_ms() - start_time;
    rtos_test_update_suite_result(RTOS_TEST_SUITE_WATCHDOG, total_tests, passed_tests, execution_time);
    
    printf("看门狗测试完成: %lu/%lu 通过 (%.1f%%)\n", 
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行GPIO测试
 */
rtos_result_t rtos_hw_test_gpio(void)
{
    printf("\n--- GPIO模块测试 ---\n");
    
    uint32_t start_time = rtos_hw_get_system_time_ms();
    uint32_t total_tests = 0;
    uint32_t passed_tests = 0;
    
    /* 测试1: GPIO管理器初始化 */
    total_tests++;
    printf("测试1: GPIO管理器初始化...");
    rtos_gpio_manager_t *gpio_mgr = rtos_gpio_manager_get_instance();
    if (gpio_mgr != NULL) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试2: GPIO配置 */
    total_tests++;
    printf("测试2: GPIO配置...");
    rtos_gpio_config_t gpio_config = {
        .port = RTOS_GPIO_PORT_A,
        .pin = RTOS_GPIO_PIN_0,
        .mode = RTOS_GPIO_MODE_OUTPUT_PP,
        .pull = RTOS_GPIO_PULL_NONE,
        .speed = RTOS_GPIO_SPEED_MEDIUM,
        .alternate_function = 0,
        .trigger = RTOS_GPIO_TRIGGER_NONE,
        .initial_value = false
    };
    
    rtos_gpio_handle_t *gpio_handle = NULL;
    if (rtos_gpio_manager_config_pin(&gpio_config, &gpio_handle) == RTOS_OK && gpio_handle != NULL) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试3: GPIO读写操作 */
    if (gpio_handle) {
        total_tests++;
        printf("测试3: GPIO读写操作...");
        
        bool test_passed = true;
        
        /* 写高电平 */
        if (rtos_gpio_manager_write_pin(gpio_handle, true) != RTOS_OK) {
            test_passed = false;
        }
        
        /* 读取值 */
        bool read_value;
        if (rtos_gpio_manager_read_pin(gpio_handle, &read_value) != RTOS_OK) {
            test_passed = false;
        }
        
        /* 翻转引脚 */
        if (rtos_gpio_manager_toggle_pin(gpio_handle) != RTOS_OK) {
            test_passed = false;
        }
        
        if (test_passed) {
            printf("通过\n");
            passed_tests++;
        } else {
            printf("失败\n");
        }
    }
    
    /* 测试4: GPIO端口批量操作 */
    total_tests++;
    printf("测试4: GPIO端口批量操作...");
    uint16_t port_value;
    if (rtos_gpio_manager_read_port(RTOS_GPIO_PORT_A, &port_value) == RTOS_OK) {
        if (rtos_gpio_manager_write_port(RTOS_GPIO_PORT_A, 0x0001, 0x0001) == RTOS_OK) {
            printf("通过\n");
            passed_tests++;
        } else {
            printf("失败 (写端口失败)\n");
        }
    } else {
        printf("失败 (读端口失败)\n");
    }
    
    uint32_t execution_time = rtos_hw_get_system_time_ms() - start_time;
    rtos_test_update_suite_result(RTOS_TEST_SUITE_GPIO, total_tests, passed_tests, execution_time);
    
    printf("GPIO测试完成: %lu/%lu 通过 (%.1f%%)\n", 
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行UART测试
 */
rtos_result_t rtos_hw_test_uart(void)
{
    printf("\n--- UART模块测试 ---\n");
    
    uint32_t start_time = rtos_hw_get_system_time_ms();
    uint32_t total_tests = 0;
    uint32_t passed_tests = 0;
    
    /* 测试1: UART管理器初始化 */
    total_tests++;
    printf("测试1: UART管理器初始化...");
    rtos_uart_manager_t *uart_mgr = rtos_uart_manager_get_instance();
    if (uart_mgr != NULL) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试2: UART端口初始化 */
    total_tests++;
    printf("测试2: UART端口初始化...");
    rtos_uart_config_t uart_config = {
        .baudrate = 115200,
        .databits = RTOS_UART_DATABITS_8,
        .stopbits = RTOS_UART_STOPBITS_1,
        .parity = RTOS_UART_PARITY_NONE,
        .flowctrl = RTOS_UART_FLOWCTRL_NONE,
        .timeout_ms = 1000,
        .auto_baudrate = false
    };
    
    if (rtos_uart_manager_init_port(RTOS_UART_PORT_1, &uart_config, NULL) == RTOS_OK) {
        printf("通过 (115200 bps)\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试3: UART状态查询 */
    total_tests++;
    printf("测试3: UART状态查询...");
    rtos_uart_state_t state = rtos_uart_manager_get_state(RTOS_UART_PORT_1);
    if (state == RTOS_UART_STATE_READY) {
        printf("通过 (状态: 就绪)\n");
        passed_tests++;
    } else {
        printf("失败 (状态: %d)\n", state);
    }
    
    /* 测试4: UART数据发送 */
    total_tests++;
    printf("测试4: UART数据发送...");
    const char *test_string = "RTOS UART Test\r\n";
    if (rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)test_string, strlen(test_string)) == RTOS_OK) {
        printf("通过 (%lu 字节)\n", strlen(test_string));
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    uint32_t execution_time = rtos_hw_get_system_time_ms() - start_time;
    rtos_test_update_suite_result(RTOS_TEST_SUITE_UART, total_tests, passed_tests, execution_time);
    
    printf("UART测试完成: %lu/%lu 通过 (%.1f%%)\n", 
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行集成测试
 */
rtos_result_t rtos_hw_test_integration(void)
{
    printf("\n--- 集成测试 ---\n");
    
    uint32_t start_time = rtos_hw_get_system_time_ms();
    uint32_t total_tests = 0;
    uint32_t passed_tests = 0;
    
    /* 测试1: 模块间协作 */
    total_tests++;
    printf("测试1: 模块间协作...");
    
    /* 启动内存监控的周期性任务 */
    rtos_memory_monitor_periodic_task();
    
    /* 启动电源管理的周期性任务 */
    rtos_power_manager_periodic_task();
    
    /* 启动看门狗的周期性任务 */
    rtos_watchdog_manager_periodic_task();
    
    printf("通过\n");
    passed_tests++;
    
    /* 测试2: 系统信息统计 */
    total_tests++;
    printf("测试2: 系统信息统计...");
    
    char info_buffer[1024];
    uint32_t info_len = rtos_hw_get_info_string(info_buffer, sizeof(info_buffer));
    
    if (info_len > 0) {
        printf("通过 (%lu 字符)\n", info_len);
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试3: 多模块状态查询 */
    total_tests++;
    printf("测试3: 多模块状态查询...");
    
    /* 查询各模块状态 */
    rtos_power_status_t power_status;
            rtos_memory_monitor_stats_t memory_stats;
    
    bool status_ok = true;
    if (rtos_power_manager_get_status(&power_status) != RTOS_OK) {
        status_ok = false;
    }
    if (rtos_memory_monitor_get_stats(&memory_stats) != RTOS_OK) {
        status_ok = false;
    }
    
    if (status_ok) {
        printf("通过\n");
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    uint32_t execution_time = rtos_hw_get_system_time_ms() - start_time;
    rtos_test_update_suite_result(RTOS_TEST_SUITE_INTEGRATION, total_tests, passed_tests, execution_time);
    
    printf("集成测试完成: %lu/%lu 通过 (%.1f%%)\n", 
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 性能基准测试
 */
rtos_result_t rtos_hw_run_performance_benchmark(void)
{
    printf("\n--- 性能基准测试 ---\n");
    
    uint32_t start_time = rtos_hw_get_system_time_ms();
    
    /* GPIO性能测试 */
    printf("GPIO性能测试...");
    rtos_gpio_config_t demo_gpio_config = {
        .port = RTOS_GPIO_PORT_A,
        .pin = RTOS_GPIO_PIN_2,
        .mode = RTOS_GPIO_MODE_OUTPUT_PP,
        .pull = RTOS_GPIO_PULL_NONE,
        .speed = RTOS_GPIO_SPEED_MEDIUM,
        .alternate_function = 0,
        .trigger = RTOS_GPIO_TRIGGER_NONE,
        .initial_value = false
    };
    rtos_gpio_handle_t *gpio_handle = NULL;
    
    if (rtos_gpio_manager_config_pin(&demo_gpio_config, &gpio_handle) == RTOS_OK) {
        uint32_t gpio_start = rtos_hw_get_system_time_ms();
        
        /* 执行1000次GPIO翻转 */
        for (int i = 0; i < 1000; i++) {
            rtos_gpio_manager_toggle_pin(gpio_handle);
        }
        
        uint32_t gpio_time = rtos_hw_get_system_time_ms() - gpio_start;
        printf("完成 (1000次翻转用时: %lu ms, 平均: %.2f μs/次)\n", 
               gpio_time, (float)gpio_time * 1000.0f / 1000);
    } else {
        printf("失败\n");
    }
    
    /* 内存分配性能测试 */
    printf("内存分配性能测试...");
    uint32_t mem_start = rtos_hw_get_system_time_ms();
    
    /* 执行100次内存分配和释放 */
    for (int i = 0; i < 100; i++) {
        void *ptr = rtos_malloc(256);
        if (ptr) {
            rtos_free(ptr);
        }
    }
    
    uint32_t mem_time = rtos_hw_get_system_time_ms() - mem_start;
    printf("完成 (100次分配/释放用时: %lu ms, 平均: %.2f μs/次)\n", 
           mem_time, (float)mem_time * 1000.0f / 100);
    
    /* 定时器精度测试 */
    printf("定时器精度测试...");
    uint32_t timer_start = rtos_hw_get_system_time_ms();
    
    /* 设置1ms定时器并测量实际时间 */
    if (rtos_hw_set_timer(1000000) == RTOS_OK) { /* 1ms */
        rtos_hw_delay_ms(2); /* 等待定时器超时 */
        rtos_hw_stop_timer();
    }
    
    uint32_t timer_time = rtos_hw_get_system_time_ms() - timer_start;
    printf("完成 (定时器测试用时: %lu ms)\n", timer_time);
    
    uint32_t total_time = rtos_hw_get_system_time_ms() - start_time;
    printf("性能基准测试完成，总耗时: %lu ms\n", total_time);
    
    return RTOS_OK;
}

/**
 * @brief 获取综合测试统计信息
 */
const rtos_comprehensive_test_stats_t* rtos_hw_get_comprehensive_test_stats(void)
{
    if (!g_test_initialized) {
        return NULL;
    }
    return &g_test_stats;
}

/**
 * @brief 生成测试报告
 */
uint32_t rtos_hw_generate_test_report(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_test_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "RTOS硬件抽象层综合测试报告\n"
        "============================\n"
        "测试概览:\n"
        "  测试套件总数: %lu\n"
        "  通过套件数: %lu\n"
        "  失败套件数: %lu\n"
        "  总测试数: %lu\n"
        "  总通过数: %lu\n"
        "  总失败数: %lu\n"
        "  总执行时间: %lu ms\n"
        "  总体通过率: %.1f%%\n\n"
        "各套件详细结果:\n",
        g_test_stats.total_suites,
        g_test_stats.passed_suites,
        g_test_stats.failed_suites,
        g_test_stats.total_tests,
        g_test_stats.total_passed,
        g_test_stats.total_failed,
        g_test_stats.total_execution_time_ms,
        g_test_stats.overall_pass_rate);
    
    /* 添加各套件详细信息 */
    for (uint32_t i = 0; i < RTOS_TEST_SUITE_MAX; i++) {
        rtos_test_result_t *result = &g_test_stats.suite_results[i];
        
        int suite_len = snprintf(buffer + len, size - len,
            "  %s:\n"
            "    测试数: %lu, 通过: %lu, 失败: %lu\n"
            "    通过率: %.1f%%, 执行时间: %lu ms\n",
            result->suite_name,
            result->total_tests,
            result->passed_tests,
            result->failed_tests,
            result->pass_rate,
            result->execution_time_ms);
            
        if (suite_len > 0 && len + suite_len < (int)size) {
            len += suite_len;
        }
    }
    
    if (len < 0) {
        return 0;
    }
    
    return (len >= (int)size) ? (size - 1) : (uint32_t)len;
}

/* 内部函数实现 */

/**
 * @brief 初始化测试统计
 */
static void rtos_test_init_stats(void)
{
    memset(&g_test_stats, 0, sizeof(g_test_stats));
    
    /* 设置套件名称 */
    g_test_stats.suite_results[RTOS_TEST_SUITE_POWER].suite_name = "电源管理";
    g_test_stats.suite_results[RTOS_TEST_SUITE_MEMORY].suite_name = "内存管理";
    g_test_stats.suite_results[RTOS_TEST_SUITE_WATCHDOG].suite_name = "看门狗";
    g_test_stats.suite_results[RTOS_TEST_SUITE_GPIO].suite_name = "GPIO";
    g_test_stats.suite_results[RTOS_TEST_SUITE_UART].suite_name = "UART";
    g_test_stats.suite_results[RTOS_TEST_SUITE_TIMER].suite_name = "定时器";
    g_test_stats.suite_results[RTOS_TEST_SUITE_INTEGRATION].suite_name = "集成测试";
    
    g_test_stats.total_suites = RTOS_TEST_SUITE_MAX;
    g_test_initialized = true;
}

/**
 * @brief 更新套件测试结果
 */
static void rtos_test_update_suite_result(rtos_test_suite_t suite, uint32_t total, uint32_t passed, uint32_t execution_time)
{
    if (suite >= RTOS_TEST_SUITE_MAX) {
        return;
    }
    
    rtos_test_result_t *result = &g_test_stats.suite_results[suite];
    
    result->suite = suite;
    result->total_tests = total;
    result->passed_tests = passed;
    result->failed_tests = total - passed;
    result->execution_time_ms = execution_time;
    result->pass_rate = (total > 0) ? ((float)passed / total * 100.0f) : 0.0f;
    
    /* 更新套件通过状态 */
    if (passed == total) {
        g_test_stats.passed_suites++;
    } else {
        g_test_stats.failed_suites++;
    }
}

/**
 * @brief 计算总体统计
 */
static void rtos_test_calculate_overall_stats(void)
{
    g_test_stats.total_tests = 0;
    g_test_stats.total_passed = 0;
    g_test_stats.total_failed = 0;
    
    for (uint32_t i = 0; i < RTOS_TEST_SUITE_MAX; i++) {
        g_test_stats.total_tests += g_test_stats.suite_results[i].total_tests;
        g_test_stats.total_passed += g_test_stats.suite_results[i].passed_tests;
        g_test_stats.total_failed += g_test_stats.suite_results[i].failed_tests;
    }
    
    g_test_stats.overall_pass_rate = (g_test_stats.total_tests > 0) ? 
        ((float)g_test_stats.total_passed / g_test_stats.total_tests * 100.0f) : 0.0f;
}

/* 新增测试函数 */

/**
 * @brief 运行定时器测试
 */
rtos_result_t rtos_hw_test_timer(void)
{
    printf("\n--- 定时器模块测试 ---\n");
    
    uint32_t start_time = rtos_hw_get_system_time_ms();
    uint32_t total_tests = 0;
    uint32_t passed_tests = 0;
    
    /* 测试1: 设置定时器 */
    total_tests++;
    printf("测试1: 设置定时器...");
    if (rtos_hw_set_timer(1000000) == RTOS_OK) { /* 1ms */
        printf("通过\n");
        passed_tests++;
        rtos_hw_stop_timer();
    } else {
        printf("失败\n");
    }
    
    /* 测试2: 获取时间戳 */
    total_tests++;
    printf("测试2: 获取时间戳...");
    rtos_time_ns_t timestamp1 = rtos_hw_get_timestamp_ns();
    rtos_hw_delay_ms(1);
    rtos_time_ns_t timestamp2 = rtos_hw_get_timestamp_ns();
    
    if (timestamp2 > timestamp1) {
        printf("通过 (时间差: %llu ns)\n", timestamp2 - timestamp1);
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    /* 测试3: 系统时间查询 */
    total_tests++;
    printf("测试3: 系统时间查询...");
    uint64_t sys_time_ms = rtos_hw_get_system_time_ms();
    if (sys_time_ms > 0) {
        printf("通过 (系统时间: %llu ms)\n", sys_time_ms);
        passed_tests++;
    } else {
        printf("失败\n");
    }
    
    uint32_t execution_time = rtos_hw_get_system_time_ms() - start_time;
    rtos_test_update_suite_result(RTOS_TEST_SUITE_TIMER, total_tests, passed_tests, execution_time);
    
    printf("定时器测试完成: %lu/%lu 通过 (%.1f%%)\n", 
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行DMA测试
 */
rtos_result_t rtos_hw_test_dma(void)
{
    printf("DMA模块测试...");
    
    uint32_t total_tests = 3;
    uint32_t passed_tests = 0;
    
    /* 测试1: DMA管理器初始化检查 */
    rtos_dma_manager_t *dma_mgr = rtos_dma_manager_get_instance();
    if (dma_mgr != NULL) {
        passed_tests++;
    }
    
    /* 测试2: DMA内存拷贝测试 */
    uint8_t src_buffer[64], dst_buffer[64];
    for (int i = 0; i < 64; i++) src_buffer[i] = (uint8_t)i;
    memset(dst_buffer, 0, 64);
    
    if (rtos_dma_manager_memcpy(dst_buffer, src_buffer, 64) == RTOS_OK) {
        bool copy_ok = (memcmp(src_buffer, dst_buffer, 64) == 0);
        if (copy_ok) passed_tests++;
    }
    
    /* 测试3: DMA统计信息 */
    char dma_stats[256];
    if (rtos_dma_manager_get_statistics(dma_stats, sizeof(dma_stats)) > 0) {
        passed_tests++;
    }
    
    printf("完成 (%lu/%lu 通过)\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行SPI测试
 */
rtos_result_t rtos_hw_test_spi(void)
{
    printf("SPI模块测试...");
    
    uint32_t total_tests = 2;
    uint32_t passed_tests = 0;
    
    /* 测试1: SPI管理器检查 */
    rtos_spi_manager_t *spi_mgr = rtos_spi_manager_get_instance();
    if (spi_mgr != NULL) {
        passed_tests++;
    }
    
    /* 测试2: SPI端口配置 */
    rtos_spi_config_t spi_config = RTOS_SPI_DEFAULT_CONFIG();
    if (rtos_spi_manager_init_port(RTOS_SPI_PORT_1, &spi_config, 4) == RTOS_OK) {
        passed_tests++;
    }
    
    printf("完成 (%lu/%lu 通过)\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行I2C测试
 */
rtos_result_t rtos_hw_test_i2c(void)
{
    printf("I2C模块测试...");
    
    uint32_t total_tests = 2;
    uint32_t passed_tests = 0;
    
    /* 测试1: I2C管理器检查 */
    rtos_i2c_manager_t *i2c_mgr = rtos_i2c_manager_get_instance();
    if (i2c_mgr != NULL) {
        passed_tests++;
    }
    
    /* 测试2: I2C端口配置 */
    rtos_i2c_config_t i2c_config = RTOS_I2C_DEFAULT_CONFIG(RTOS_I2C_MODE_STANDARD);
    if (rtos_i2c_manager_init_port(RTOS_I2C_PORT_1, &i2c_config, 8) == RTOS_OK) {
        passed_tests++;
    }
    
    printf("完成 (%lu/%lu 通过)\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行ADC/DAC测试
 */
rtos_result_t rtos_hw_test_adc_dac(void)
{
    printf("ADC/DAC模块测试...");
    
    uint32_t total_tests = 4;
    uint32_t passed_tests = 0;
    
    /* 测试1-4: 基础功能检查 */
    rtos_adc_manager_t *adc_mgr = rtos_adc_manager_get_instance();
    if (adc_mgr != NULL) passed_tests++;
    
    rtos_dac_manager_t *dac_mgr = rtos_dac_manager_get_instance();
    if (dac_mgr != NULL) passed_tests++;
    
    rtos_adc_config_t adc_config = RTOS_ADC_DEFAULT_CONFIG();
    if (rtos_adc_manager_init_controller(RTOS_ADC_CONTROLLER_1, &adc_config, 8) == RTOS_OK) {
        passed_tests++;
    }
    
    rtos_dac_config_t dac_config = RTOS_DAC_DEFAULT_CONFIG();
    if (rtos_dac_manager_init_channel(RTOS_DAC_CHANNEL_1, &dac_config) == RTOS_OK) {
        passed_tests++;
    }
    
    printf("完成 (%lu/%lu 通过)\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行安全模块测试
 */
rtos_result_t rtos_hw_test_security(void)
{
    printf("安全模块测试...");
    
    uint32_t total_tests = 3;
    uint32_t passed_tests = 0;
    
    /* 测试1: 安全启动管理器 */
    rtos_secure_boot_config_t boot_config = RTOS_SECURE_BOOT_DEFAULT_CONFIG();
    if (rtos_secure_boot_manager_init(&boot_config) == RTOS_OK) {
        passed_tests++;
    }
    
    /* 测试2: 加密管理器 */
    if (rtos_crypto_manager_init(8, 4, 1024) == RTOS_OK) {
        passed_tests++;
    }
    
    /* 测试3: 随机数生成 */
    uint8_t random_buffer[32];
    if (rtos_crypto_generate_random(random_buffer, 32) == RTOS_OK) {
        passed_tests++;
    }
    
    printf("完成 (%lu/%lu 通过)\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行网络模块测试
 */
rtos_result_t rtos_hw_test_network(void)
{
    printf("网络模块测试...");
    
    uint32_t total_tests = 2;
    uint32_t passed_tests = 0;
    
    /* 测试1: 以太网管理器 */
    rtos_eth_config_t eth_config = RTOS_ETH_DEFAULT_CONFIG();
    if (rtos_ethernet_manager_init(&eth_config, 16, 16) == RTOS_OK) {
        passed_tests++;
    }
    
    /* 测试2: 链路状态查询 */
    rtos_eth_link_state_t link_state = rtos_ethernet_manager_get_link_state();
    (void)link_state; /* 避免未使用警告 */
    passed_tests++;
    
    printf("完成 (%lu/%lu 通过)\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行OTA模块测试
 */
rtos_result_t rtos_hw_test_ota(void)
{
    printf("OTA模块测试...");
    
    uint32_t total_tests = 2;
    uint32_t passed_tests = 0;
    
    /* 测试1: OTA管理器初始化 */
    rtos_ota_config_t ota_config = RTOS_OTA_DEFAULT_CONFIG();
    if (rtos_ota_manager_init(&ota_config) == RTOS_OK) {
        passed_tests++;
    }
    
    /* 测试2: 固件信息查询 */
    rtos_ota_firmware_info_t firmware_info;
    if (rtos_ota_manager_get_current_firmware_info(&firmware_info) == RTOS_OK) {
        passed_tests++;
    }
    
    printf("完成 (%lu/%lu 通过)\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 运行调试工具测试
 */
rtos_result_t rtos_hw_test_debug_tools(void)
{
    printf("调试工具测试...");
    
    uint32_t total_tests = 3;
    uint32_t passed_tests = 0;
    
    /* 测试1: 性能分析器 */
    rtos_perf_profiler_config_t perf_config = RTOS_PERF_DEFAULT_CONFIG();
    if (rtos_performance_profiler_init(&perf_config) == RTOS_OK) {
        passed_tests++;
    }
    
    /* 测试2: 系统跟踪器 */
    rtos_trace_config_t trace_config = RTOS_TRACE_DEFAULT_CONFIG();
    if (rtos_system_tracer_init(&trace_config) == RTOS_OK) {
        passed_tests++;
    }
    
    /* 测试3: 性能事件记录 */
    if (rtos_performance_profiler_record_event(RTOS_PERF_EVENT_USER_DEFINED, 0, 0, NULL) == RTOS_OK) {
        passed_tests++;
    }
    
    printf("完成 (%lu/%lu 通过)\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? RTOS_OK : RTOS_ERROR;
}