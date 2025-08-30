/**
 * @file dynamic_delay.h
 * @brief RTOS动态延时管理器 - 使用硬件定时器动态设置系统延时
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_DYNAMIC_DELAY_H__
#define __RTOS_DYNAMIC_DELAY_H__

#include "../core/types.h"

/* 延时管理器状态 */
typedef enum {
    RTOS_DELAY_MANAGER_IDLE = 0,
    RTOS_DELAY_MANAGER_ACTIVE,
    RTOS_DELAY_MANAGER_SUSPENDED
} rtos_delay_manager_state_t;

/* 延时统计信息 */
typedef struct {
    uint32_t total_delays;           /* 总延时次数 */
    uint32_t active_delays;          /* 当前活跃延时 */
    rtos_time_ns_t min_delay;        /* 最小延时时间 */
    rtos_time_ns_t max_delay;        /* 最大延时时间 */
    rtos_time_ns_t average_delay;    /* 平均延时时间 */
    uint32_t timer_reconfigs;        /* 定时器重配置次数 */
} rtos_delay_stats_t;

/* 动态延时管理器API */

/**
 * @brief 初始化动态延时管理器
 * @return 操作结果
 */
rtos_result_t rtos_dynamic_delay_init(void);

/**
 * @brief 启动动态延时管理器
 * @return 操作结果
 */
rtos_result_t rtos_dynamic_delay_start(void);

/**
 * @brief 停止动态延时管理器
 * @return 操作结果
 */
rtos_result_t rtos_dynamic_delay_stop(void);

/**
 * @brief 请求系统延时
 * @param delay_ns 延时时间(纳秒)
 * @return 操作结果
 */
rtos_result_t rtos_dynamic_delay_request(rtos_time_ns_t delay_ns);

/**
 * @brief 取消系统延时
 * @return 操作结果
 */
rtos_result_t rtos_dynamic_delay_cancel(void);

/**
 * @brief 获取当前延时状态
 * @return 延时管理器状态
 */
rtos_delay_manager_state_t rtos_dynamic_delay_get_state(void);

/**
 * @brief 获取剩余延时时间
 * @return 剩余延时时间(纳秒)
 */
rtos_time_ns_t rtos_dynamic_delay_get_remaining(void);

/**
 * @brief 获取延时统计信息
 * @param stats 统计信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_dynamic_delay_get_stats(rtos_delay_stats_t *stats);

/**
 * @brief 重置延时统计信息
 * @return 操作结果
 */
rtos_result_t rtos_dynamic_delay_reset_stats(void);

/**
 * @brief 动态调整硬件定时器
 * 根据系统状态动态调整定时器配置以优化功耗和性能
 */
void rtos_dynamic_delay_optimize_timer(void);

/**
 * @brief 延时管理器中断处理函数
 * 由硬件定时器中断调用
 */
void rtos_dynamic_delay_interrupt_handler(void);

/* 便捷宏定义 */
#define RTOS_DELAY_MS(ms) rtos_dynamic_delay_request(RTOS_TIMER_MS_TO_NS(ms))
#define RTOS_DELAY_US(us) rtos_dynamic_delay_request(RTOS_TIMER_US_TO_NS(us))
#define RTOS_DELAY_NS(ns) rtos_dynamic_delay_request(ns)

#endif /* __RTOS_DYNAMIC_DELAY_H__ */