/**
 * @file tickless.h
 * @brief RTOS无滴答时钟时间管理系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_TICKLESS_H__
#define __RTOS_TICKLESS_H__

#include "../core/types.h"

/* 时间事件类型 */
typedef enum {
    RTOS_TIME_EVENT_TASK_DELAY = 0,
    RTOS_TIME_EVENT_TIMER_EXPIRE,
    RTOS_TIME_EVENT_TIMEOUT
} rtos_time_event_type_t;

/* 时间事件结构体 */
typedef struct rtos_time_event {
    rtos_time_event_type_t    type;           /* 事件类型 */
    rtos_time_ns_t           expire_time;     /* 到期时间 */
    void                     *object;         /* 关联对象 */
    void                     (*callback)(void *); /* 回调函数 */
    struct rtos_time_event   *next;           /* 下一个事件 */
    struct rtos_time_event   *prev;           /* 上一个事件 */
} rtos_time_event_t;

/* Tickless时间管理器API */

/**
 * @brief 初始化Tickless时间管理器
 * @return 操作结果
 */
rtos_result_t rtos_tickless_init(void);

/**
 * @brief 启动Tickless时间管理器
 * @return 操作结果
 */
rtos_result_t rtos_tickless_start(void);

/**
 * @brief 停止Tickless时间管理器
 * @return 操作结果
 */
rtos_result_t rtos_tickless_stop(void);

/**
 * @brief 添加时间事件
 * @param event 时间事件指针
 * @param delay_ns 延时时间(纳秒)
 * @return 操作结果
 */
rtos_result_t rtos_tickless_add_event(rtos_time_event_t *event, rtos_time_ns_t delay_ns);

/**
 * @brief 移除时间事件
 * @param event 时间事件指针
 * @return 操作结果
 */
rtos_result_t rtos_tickless_remove_event(rtos_time_event_t *event);

/**
 * @brief 获取下一个到期事件的时间
 * @return 下一个到期时间(纳秒)，0表示无事件
 */
rtos_time_ns_t rtos_tickless_get_next_expire_time(void);

/**
 * @brief 处理到期的时间事件
 * 此函数由硬件定时器中断调用
 */
void rtos_tickless_handle_expired_events(void);

/**
 * @brief 更新硬件定时器设置
 * 根据最近的时间事件动态设置硬件定时器
 */
void rtos_tickless_update_timer(void);

/**
 * @brief 获取当前系统时间
 * @return 当前系统时间(纳秒)
 */
rtos_time_ns_t rtos_tickless_get_current_time(void);

/**
 * @brief 计算时间差
 * @param time1 时间1
 * @param time2 时间2
 * @return 时间差(纳秒)
 */
rtos_time_ns_t rtos_tickless_time_diff(rtos_time_ns_t time1, rtos_time_ns_t time2);

/**
 * @brief 检查时间是否到期
 * @param target_time 目标时间
 * @param current_time 当前时间
 * @return 是否到期
 */
bool rtos_tickless_is_time_expired(rtos_time_ns_t target_time, rtos_time_ns_t current_time);

/**
 * @brief 获取最早到期事件的延时时间
 * @return 延时时间(纳秒)，0表示无事件
 */
rtos_time_ns_t rtos_tickless_get_next_delay(void);

/**
 * @brief 获取事件数量
 * @return 当前事件数量
 */
uint32_t rtos_tickless_get_event_count(void);

#endif /* __RTOS_TICKLESS_H__ */