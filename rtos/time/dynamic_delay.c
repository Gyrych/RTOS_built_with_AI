/**
 * @file dynamic_delay.c
 * @brief RTOS动态延时管理器实现 - 使用硬件定时器动态设置系统延时
 * @author Assistant
 * @date 2024
 */

#include "dynamic_delay.h"
#include "tickless.h"
#include "../hw/hw_abstraction.h"
#include <string.h>
#include <stdio.h>

/* 全局变量 */
static rtos_delay_manager_state_t g_delay_state = RTOS_DELAY_MANAGER_IDLE;
static rtos_time_ns_t g_current_delay = 0;
static rtos_time_ns_t g_delay_start_time = 0;
static rtos_delay_stats_t g_delay_stats = {0};

/* 内部函数声明 */
static void rtos_dynamic_delay_update_stats(rtos_time_ns_t delay_time);
static rtos_time_ns_t rtos_dynamic_delay_calculate_optimal_period(void);

/**
 * @brief 初始化动态延时管理器
 */
rtos_result_t rtos_dynamic_delay_init(void)
{
    /* 初始化状态 */
    g_delay_state = RTOS_DELAY_MANAGER_IDLE;
    g_current_delay = 0;
    g_delay_start_time = 0;
    
    /* 初始化统计信息 */
    memset(&g_delay_stats, 0, sizeof(g_delay_stats));
    g_delay_stats.min_delay = UINT64_MAX;
    
    return RTOS_OK;
}

/**
 * @brief 启动动态延时管理器
 */
rtos_result_t rtos_dynamic_delay_start(void)
{
    if (g_delay_state != RTOS_DELAY_MANAGER_IDLE) {
        return RTOS_ERROR;
    }
    
    g_delay_state = RTOS_DELAY_MANAGER_ACTIVE;
    return RTOS_OK;
}

/**
 * @brief 停止动态延时管理器
 */
rtos_result_t rtos_dynamic_delay_stop(void)
{
    if (g_delay_state == RTOS_DELAY_MANAGER_IDLE) {
        return RTOS_OK;
    }
    
    /* 取消当前延时 */
    rtos_dynamic_delay_cancel();
    
    g_delay_state = RTOS_DELAY_MANAGER_IDLE;
    return RTOS_OK;
}

/**
 * @brief 请求系统延时
 */
rtos_result_t rtos_dynamic_delay_request(rtos_time_ns_t delay_ns)
{
    if (g_delay_state != RTOS_DELAY_MANAGER_ACTIVE) {
        return RTOS_ERROR;
    }
    
    if (delay_ns == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 如果当前有延时，先取消 */
    if (g_current_delay > 0) {
        rtos_dynamic_delay_cancel();
    }
    
    /* 设置新的延时 */
    g_current_delay = delay_ns;
    g_delay_start_time = rtos_tickless_get_current_time();
    
    /* 计算最优的硬件定时器周期 */
    rtos_time_ns_t optimal_period = rtos_dynamic_delay_calculate_optimal_period();
    
    /* 使用最优周期或请求的延时时间（取较小值） */
    rtos_time_ns_t timer_period = (optimal_period < delay_ns) ? optimal_period : delay_ns;
    
    /* 设置硬件定时器 */
    rtos_result_t result = rtos_hw_set_timer(timer_period);
    if (result != RTOS_OK) {
        g_current_delay = 0;
        g_delay_start_time = 0;
        return result;
    }
    
    /* 更新统计信息 */
    rtos_dynamic_delay_update_stats(delay_ns);
    
    return RTOS_OK;
}

/**
 * @brief 取消系统延时
 */
rtos_result_t rtos_dynamic_delay_cancel(void)
{
    if (g_current_delay == 0) {
        return RTOS_OK;
    }
    
    /* 停止硬件定时器 */
    rtos_hw_stop_timer();
    
    /* 清除延时状态 */
    g_current_delay = 0;
    g_delay_start_time = 0;
    
    return RTOS_OK;
}

/**
 * @brief 获取当前延时状态
 */
rtos_delay_manager_state_t rtos_dynamic_delay_get_state(void)
{
    return g_delay_state;
}

/**
 * @brief 获取剩余延时时间
 */
rtos_time_ns_t rtos_dynamic_delay_get_remaining(void)
{
    if (g_current_delay == 0) {
        return 0;
    }
    
    rtos_time_ns_t current_time = rtos_tickless_get_current_time();
    rtos_time_ns_t elapsed = current_time - g_delay_start_time;
    
    if (elapsed >= g_current_delay) {
        return 0;
    }
    
    return g_current_delay - elapsed;
}

/**
 * @brief 获取延时统计信息
 */
rtos_result_t rtos_dynamic_delay_get_stats(rtos_delay_stats_t *stats)
{
    if (!stats) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    memcpy(stats, &g_delay_stats, sizeof(rtos_delay_stats_t));
    return RTOS_OK;
}

/**
 * @brief 重置延时统计信息
 */
rtos_result_t rtos_dynamic_delay_reset_stats(void)
{
    memset(&g_delay_stats, 0, sizeof(g_delay_stats));
    g_delay_stats.min_delay = UINT64_MAX;
    return RTOS_OK;
}

/**
 * @brief 动态调整硬件定时器
 */
void rtos_dynamic_delay_optimize_timer(void)
{
    if (g_delay_state != RTOS_DELAY_MANAGER_ACTIVE) {
        return;
    }
    
    /* 获取下一个tickless事件时间 */
    rtos_time_ns_t next_event_delay = rtos_tickless_get_next_delay();
    
    if (next_event_delay > 0) {
        /* 计算最优定时器周期 */
        rtos_time_ns_t optimal_period = rtos_dynamic_delay_calculate_optimal_period();
        rtos_time_ns_t timer_period = (optimal_period < next_event_delay) ? 
                                     optimal_period : next_event_delay;
        
        /* 重新设置硬件定时器 */
        rtos_hw_set_timer(timer_period);
        g_delay_stats.timer_reconfigs++;
    } else {
        /* 没有待处理事件，停止定时器以节能 */
        rtos_hw_stop_timer();
    }
}

/**
 * @brief 延时管理器中断处理函数
 */
void rtos_dynamic_delay_interrupt_handler(void)
{
    if (g_delay_state != RTOS_DELAY_MANAGER_ACTIVE) {
        return;
    }
    
    /* 处理tickless事件 */
    rtos_tickless_handle_expired_events();
    
    /* 动态优化定时器设置 */
    rtos_dynamic_delay_optimize_timer();
}

/* 内部函数实现 */

/**
 * @brief 更新延时统计信息
 */
static void rtos_dynamic_delay_update_stats(rtos_time_ns_t delay_time)
{
    g_delay_stats.total_delays++;
    
    /* 更新最小延时 */
    if (delay_time < g_delay_stats.min_delay) {
        g_delay_stats.min_delay = delay_time;
    }
    
    /* 更新最大延时 */
    if (delay_time > g_delay_stats.max_delay) {
        g_delay_stats.max_delay = delay_time;
    }
    
    /* 更新平均延时 */
    if (g_delay_stats.total_delays > 0) {
        g_delay_stats.average_delay = 
            (g_delay_stats.average_delay * (g_delay_stats.total_delays - 1) + delay_time) / 
            g_delay_stats.total_delays;
    }
}

/**
 * @brief 计算最优定时器周期
 */
static rtos_time_ns_t rtos_dynamic_delay_calculate_optimal_period(void)
{
    /* 基于系统负载和历史数据计算最优周期 */
    
    /* 基础周期：1ms */
    rtos_time_ns_t base_period = 1000000; /* 1ms */
    
    /* 根据活跃延时数量调整 */
    uint32_t event_count = rtos_tickless_get_event_count();
    if (event_count > 10) {
        /* 高负载：使用较短周期 */
        base_period = 100000; /* 100μs */
    } else if (event_count < 3) {
        /* 低负载：使用较长周期以节能 */
        base_period = 10000000; /* 10ms */
    }
    
    /* 考虑平均延时时间 */
    if (g_delay_stats.average_delay > 0) {
        rtos_time_ns_t adaptive_period = g_delay_stats.average_delay / 10;
        if (adaptive_period > base_period && adaptive_period < 50000000) { /* 最大50ms */
            base_period = adaptive_period;
        }
    }
    
    return base_period;
}

/**
 * @brief 获取动态延时管理器信息字符串
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 * @return 字符串长度
 */
uint32_t rtos_dynamic_delay_get_info_string(char *buffer, uint32_t size)
{
    if (!buffer || size == 0) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
                       "Dynamic Delay Manager Status:\n"
                       "  State: %s\n"
                       "  Current Delay: %lu ns\n"
                       "  Remaining: %lu ns\n"
                                               "  Total Delays: %lu\n"
                                               "  Active Events: %lu\n"
                                               "  Timer Reconfigs: %lu\n"
                       "  Min Delay: %lu ns\n"
                       "  Max Delay: %lu ns\n"
                       "  Avg Delay: %lu ns",
                       (g_delay_state == RTOS_DELAY_MANAGER_IDLE) ? "Idle" :
                       (g_delay_state == RTOS_DELAY_MANAGER_ACTIVE) ? "Active" : "Suspended",
                       (unsigned long)g_current_delay,
                       (unsigned long)rtos_dynamic_delay_get_remaining(),
                       g_delay_stats.total_delays,
                       rtos_tickless_get_event_count(),
                       g_delay_stats.timer_reconfigs,
                       (unsigned long)(g_delay_stats.min_delay == UINT64_MAX ? 0 : g_delay_stats.min_delay),
                       (unsigned long)g_delay_stats.max_delay,
                       (unsigned long)g_delay_stats.average_delay);
    
    if (len < 0) {
        len = 0;
    } else if ((uint32_t)len >= size) {
        len = size - 1;
    }
    
    return (uint32_t)len;
}