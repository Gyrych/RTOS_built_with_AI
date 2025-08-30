/**
 * @file hw_timer_test.c
 * @brief 硬件定时器测试程序 - 验证TIM2功能
 * @author Assistant
 * @date 2024
 */

#include "hw_abstraction.h"
#include "hw_config.h"
#include "../core/types.h"
#include <stdio.h>

/* 声明外部变量 */
extern volatile bool g_hardware_timer_running;

/* 测试结果统计 */
static struct {
    uint32_t test_count;
    uint32_t success_count;
    uint32_t error_count;
    rtos_time_ns_t total_error_ns;
    rtos_time_ns_t max_error_ns;
    rtos_time_ns_t min_error_ns;
} g_test_stats = {0};

/**
 * @brief 测试定时器基本功能
 */
static rtos_result_t test_timer_basic(void)
{
    printf("测试定时器基本功能...\n");
    
    /* 测试1: 设置1ms定时器 */
    rtos_result_t result = rtos_hw_set_timer(1000000); // 1ms
    if (result != RTOS_OK) {
        printf("错误: 设置1ms定时器失败\n");
        return RTOS_ERROR;
    }
    
    /* 检查定时器是否运行 */
    if (!g_hardware_timer_running) {
        printf("错误: 定时器未启动\n");
        return RTOS_ERROR;
    }
    
    /* 获取剩余时间 */
    rtos_time_ns_t remaining = rtos_hw_get_timer_remaining();
    if (remaining == 0) {
        printf("错误: 定时器剩余时间为0\n");
        return RTOS_ERROR;
    }
    
    printf("定时器设置成功: 1ms, 剩余时间: %llu ns\n", remaining);
    
    /* 停止定时器 */
    result = rtos_hw_stop_timer();
    if (result != RTOS_OK) {
        printf("错误: 停止定时器失败\n");
        return RTOS_ERROR;
    }
    
    printf("定时器基本功能测试通过\n");
    return RTOS_OK;
}

/**
 * @brief 测试定时器精度
 */
static rtos_result_t test_timer_accuracy(void)
{
    printf("测试定时器精度...\n");
    
    const rtos_time_ns_t test_periods[] = {
        1000,      // 1μs
        10000,     // 10μs
        100000,    // 100μs
        1000000,   // 1ms
        10000000,  // 10ms
        100000000  // 100ms
    };
    
    for (int i = 0; i < sizeof(test_periods) / sizeof(test_periods[0]); i++) {
        rtos_time_ns_t period = test_periods[i];
        
        /* 设置定时器 */
        rtos_result_t result = rtos_hw_set_timer(period);
        if (result != RTOS_OK) {
            printf("错误: 设置%llu ns定时器失败\n", period);
            return RTOS_ERROR;
        }
        
        /* 获取剩余时间 */
        rtos_time_ns_t remaining = rtos_hw_get_timer_remaining();
        
        /* 计算误差 */
        rtos_time_ns_t error = (remaining > period) ? (remaining - period) : (period - remaining);
        
        /* 更新统计信息 */
        g_test_stats.total_error_ns += error;
        if (error > g_test_stats.max_error_ns) {
            g_test_stats.max_error_ns = error;
        }
        if (error < g_test_stats.min_error_ns || g_test_stats.min_error_ns == 0) {
            g_test_stats.min_error_ns = error;
        }
        
        printf("周期: %llu ns, 剩余: %llu ns, 误差: %llu ns\n", 
               period, remaining, error);
        
        /* 停止定时器 */
        rtos_hw_stop_timer();
    }
    
    printf("定时器精度测试完成\n");
    return RTOS_OK;
}

/**
 * @brief 测试定时器边界条件
 */
static rtos_result_t test_timer_boundaries(void)
{
    printf("测试定时器边界条件...\n");
    
    /* 测试最小周期 */
    rtos_result_t result = rtos_hw_set_timer(RTOS_HW_MIN_TIMER_PERIOD_NS);
    if (result != RTOS_OK) {
        printf("错误: 设置最小周期定时器失败\n");
        return RTOS_ERROR;
    }
    rtos_hw_stop_timer();
    printf("最小周期测试通过: %llu ns\n", RTOS_HW_MIN_TIMER_PERIOD_NS);
    
    /* 测试最大周期 */
    result = rtos_hw_set_timer(RTOS_HW_MAX_TIMER_PERIOD_NS);
    if (result != RTOS_OK) {
        printf("错误: 设置最大周期定时器失败\n");
        return RTOS_ERROR;
    }
    rtos_hw_stop_timer();
    printf("最大周期测试通过: %llu ns\n", RTOS_HW_MAX_TIMER_PERIOD_NS);
    
    /* 测试零周期 */
    result = rtos_hw_set_timer(0);
    if (result == RTOS_OK) {
        printf("错误: 零周期定时器应该失败\n");
        return RTOS_ERROR;
    }
    printf("零周期测试通过: 正确拒绝\n");
    
    /* 测试超出范围的周期 */
    result = rtos_hw_set_timer(RTOS_HW_MAX_TIMER_PERIOD_NS + 1000000);
    if (result != RTOS_OK) {
        printf("错误: 超出范围周期应该被限制\n");
        return RTOS_ERROR;
    }
    rtos_hw_stop_timer();
    printf("超出范围周期测试通过: 正确限制\n");
    
    printf("定时器边界条件测试通过\n");
    return RTOS_OK;
}

/**
 * @brief 运行所有定时器测试
 */
rtos_result_t rtos_hw_run_timer_tests(void)
{
    printf("开始硬件定时器测试...\n");
    printf("系统时钟频率: %u Hz\n", RTOS_HW_SYSTEM_CLOCK_FREQ);
    printf("定时器时钟频率: %u Hz\n", RTOS_HW_APB1_CLOCK_FREQ);
    printf("定时器分辨率: %u ns\n", RTOS_HW_TIMER_RESOLUTION_NS);
    printf("最小周期: %llu ns\n", RTOS_HW_MIN_TIMER_PERIOD_NS);
    printf("最大周期: %llu ns\n", RTOS_HW_MAX_TIMER_PERIOD_NS);
    printf("----------------------------------------\n");
    
    /* 初始化测试统计 */
    g_test_stats.test_count = 0;
    g_test_stats.success_count = 0;
    g_test_stats.error_count = 0;
    g_test_stats.total_error_ns = 0;
    g_test_stats.max_error_ns = 0;
    g_test_stats.min_error_ns = 0;
    
    /* 运行测试 */
    rtos_result_t result;
    
    /* 测试1: 基本功能 */
    g_test_stats.test_count++;
    result = test_timer_basic();
    if (result == RTOS_OK) {
        g_test_stats.success_count++;
    } else {
        g_test_stats.error_count++;
    }
    
    /* 测试2: 精度测试 */
    g_test_stats.test_count++;
    result = test_timer_accuracy();
    if (result == RTOS_OK) {
        g_test_stats.success_count++;
    } else {
        g_test_stats.error_count++;
    }
    
    /* 测试3: 边界条件 */
    g_test_stats.test_count++;
    result = test_timer_boundaries();
    if (result == RTOS_OK) {
        g_test_stats.success_count++;
    } else {
        g_test_stats.error_count++;
    }
    
    /* 输出测试结果 */
    printf("----------------------------------------\n");
    printf("测试完成!\n");
    printf("总测试数: %lu\n", g_test_stats.test_count);
    printf("成功数: %lu\n", g_test_stats.success_count);
    printf("失败数: %lu\n", g_test_stats.error_count);
    printf("成功率: %.1f%%\n", 
           (float)g_test_stats.success_count / g_test_stats.test_count * 100.0f);
    
    if (g_test_stats.success_count > 0) {
        printf("平均误差: %llu ns\n", 
               g_test_stats.total_error_ns / g_test_stats.success_count);
        printf("最大误差: %llu ns\n", g_test_stats.max_error_ns);
        printf("最小误差: %llu ns\n", g_test_stats.min_error_ns);
    }
    
    return (g_test_stats.error_count == 0) ? RTOS_OK : RTOS_ERROR;
}

/**
 * @brief 获取测试统计信息
 */
const void* rtos_hw_get_test_stats(void)
{
    return &g_test_stats;
}
