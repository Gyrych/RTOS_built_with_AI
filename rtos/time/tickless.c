/**
 * @file tickless.c
 * @brief RTOS无滴答时钟时间管理系统实现
 * @author Assistant
 * @date 2024
 */

#include "tickless.h"
#include "../hw/hw_abstraction.h"
#include "../task/task.h"
#include <string.h>
#include <stddef.h>

/* 全局变量 */
static rtos_time_event_t *g_time_event_list = NULL;
static bool g_tickless_running = false;
static rtos_time_ns_t g_last_update_time = 0;

/* 内部函数声明 */
static void rtos_tickless_insert_event(rtos_time_event_t *event);
static void rtos_tickless_remove_event_from_list(rtos_time_event_t *event);
static rtos_time_event_t *rtos_tickless_get_next_event(void);
static void rtos_tickless_process_expired_events(rtos_time_ns_t current_time);

/**
 * @brief 初始化Tickless时间管理器
 */
rtos_result_t rtos_tickless_init(void)
{
    /* 初始化事件链表 */
    g_time_event_list = NULL;
    g_tickless_running = false;
    g_last_update_time = 0;
    
    return RTOS_OK;
}

/**
 * @brief 启动Tickless时间管理器
 */
rtos_result_t rtos_tickless_start(void)
{
    if (g_tickless_running) {
        return RTOS_ERROR;
    }
    
    g_tickless_running = true;
    g_last_update_time = rtos_hw_get_timestamp_ns();
    
    /* 设置初始硬件定时器 */
    rtos_tickless_update_timer();
    
    return RTOS_OK;
}

/**
 * @brief 停止Tickless时间管理器
 */
rtos_result_t rtos_tickless_stop(void)
{
    if (!g_tickless_running) {
        return RTOS_OK;
    }
    
    g_tickless_running = false;
    
    /* 停止硬件定时器 */
    rtos_hw_stop_timer();
    
    return RTOS_OK;
}

/**
 * @brief 添加时间事件
 */
rtos_result_t rtos_tickless_add_event(rtos_time_event_t *event, rtos_time_ns_t delay_ns)
{
    if (!event || delay_ns == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 计算到期时间 */
    rtos_time_ns_t current_time = rtos_hw_get_timestamp_ns();
    event->expire_time = current_time + delay_ns;
    
    /* 插入到事件链表 */
    rtos_tickless_insert_event(event);
    
    /* 更新硬件定时器 */
    if (g_tickless_running) {
        rtos_tickless_update_timer();
    }
    
    return RTOS_OK;
}

/**
 * @brief 移除时间事件
 */
rtos_result_t rtos_tickless_remove_event(rtos_time_event_t *event)
{
    if (!event) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 从事件链表移除 */
    rtos_tickless_remove_event_from_list(event);
    
    /* 更新硬件定时器 */
    if (g_tickless_running) {
        rtos_tickless_update_timer();
    }
    
    return RTOS_OK;
}

/**
 * @brief 获取下一个到期事件的时间
 */
rtos_time_ns_t rtos_tickless_get_next_expire_time(void)
{
    rtos_time_event_t *next_event = rtos_tickless_get_next_event();
    if (!next_event) {
        return 0;
    }
    
    return next_event->expire_time;
}

/**
 * @brief 处理到期的时间事件
 */
void rtos_tickless_handle_expired_events(void)
{
    if (!g_tickless_running) {
        return;
    }
    
    rtos_time_ns_t current_time = rtos_hw_get_timestamp_ns();
    rtos_tickless_process_expired_events(current_time);
    
    /* 更新硬件定时器设置下一个事件 */
    rtos_tickless_update_timer();
}

/**
 * @brief 更新硬件定时器设置
 */
void rtos_tickless_update_timer(void)
{
    if (!g_tickless_running) {
        return;
    }
    
    rtos_time_event_t *next_event = rtos_tickless_get_next_event();
    if (!next_event) {
        /* 没有待处理事件，停止定时器 */
        rtos_hw_stop_timer();
        return;
    }
    
    rtos_time_ns_t current_time = rtos_hw_get_timestamp_ns();
    
    /* 检查事件是否已经到期 */
    if (next_event->expire_time <= current_time) {
        /* 事件已到期，立即处理 */
        rtos_tickless_handle_expired_events();
        return;
    }
    
    /* 计算到下一个事件的时间 */
    rtos_time_ns_t timeout = next_event->expire_time - current_time;
    
    /* 设置硬件定时器 */
    rtos_hw_set_timer(timeout);
}

/**
 * @brief 获取当前系统时间
 */
rtos_time_ns_t rtos_tickless_get_current_time(void)
{
    return rtos_hw_get_timestamp_ns();
}

/**
 * @brief 计算时间差
 */
rtos_time_ns_t rtos_tickless_time_diff(rtos_time_ns_t time1, rtos_time_ns_t time2)
{
    if (time1 >= time2) {
        return time1 - time2;
    } else {
        return time2 - time1;
    }
}

/**
 * @brief 检查时间是否到期
 */
bool rtos_tickless_is_time_expired(rtos_time_ns_t target_time, rtos_time_ns_t current_time)
{
    return current_time >= target_time;
}

/* 内部函数实现 */

/**
 * @brief 将事件插入到链表中（按到期时间排序）
 */
static void rtos_tickless_insert_event(rtos_time_event_t *event)
{
    if (!event) {
        return;
    }
    
    /* 从链表中移除（如果已存在） */
    rtos_tickless_remove_event_from_list(event);
    
    /* 按到期时间排序插入 */
    rtos_time_event_t *current = g_time_event_list;
    rtos_time_event_t *prev = NULL;
    
    while (current && current->expire_time <= event->expire_time) {
        prev = current;
        current = current->next;
    }
    
    /* 插入到prev之后 */
    if (prev) {
        event->next = prev->next;
        event->prev = prev;
        if (prev->next) {
            prev->next->prev = event;
        }
        prev->next = event;
    } else {
        /* 插入到链表头 */
        event->next = g_time_event_list;
        event->prev = NULL;
        if (g_time_event_list) {
            g_time_event_list->prev = event;
        }
        g_time_event_list = event;
    }
}

/**
 * @brief 从链表中移除事件
 */
static void rtos_tickless_remove_event_from_list(rtos_time_event_t *event)
{
    if (!event) {
        return;
    }
    
    /* 更新链表指针 */
    if (event->prev) {
        event->prev->next = event->next;
    } else {
        g_time_event_list = event->next;
    }
    
    if (event->next) {
        event->next->prev = event->prev;
    }
    
    /* 清除事件链表指针 */
    event->next = NULL;
    event->prev = NULL;
}

/**
 * @brief 获取下一个事件
 */
static rtos_time_event_t *rtos_tickless_get_next_event(void)
{
    return g_time_event_list;
}

/**
 * @brief 处理已到期的事件
 */
static void rtos_tickless_process_expired_events(rtos_time_ns_t current_time)
{
    rtos_time_event_t *event = g_time_event_list;
    rtos_time_event_t *next;
    
    while (event && event->expire_time <= current_time) {
        next = event->next;
        
        /* 从链表中移除 */
        rtos_tickless_remove_event_from_list(event);
        
        /* 处理事件 */
        switch (event->type) {
            case RTOS_TIME_EVENT_TASK_DELAY:
                /* 任务延时到期，将任务从阻塞队列移到就绪队列 */
                if (event->object) {
                    rtos_task_t *task = (rtos_task_t *)event->object;
                    if (task->state == RTOS_TASK_STATE_BLOCKED) {
                        /* 这里需要调用任务管理模块的函数 */
                        extern void rtos_task_wakeup_from_delay(rtos_task_t *task);
                        rtos_task_wakeup_from_delay(task);
                    }
                }
                break;
                
            case RTOS_TIME_EVENT_TIMER_EXPIRE:
                /* 软件定时器到期 */
                if (event->callback) {
                    event->callback(event->object);
                }
                break;
                
            case RTOS_TIME_EVENT_TIMEOUT:
                /* 超时事件 */
                if (event->callback) {
                    event->callback(event->object);
                }
                break;
        }
        
        event = next;
    }
}

/**
 * @brief 获取最早到期事件的延时时间
 * @return 延时时间(纳秒)，0表示无事件
 */
rtos_time_ns_t rtos_tickless_get_next_delay(void)
{
    rtos_time_event_t *next_event = rtos_tickless_get_next_event();
    if (!next_event) {
        return 0;
    }
    
    rtos_time_ns_t current_time = rtos_hw_get_timestamp_ns();
    if (next_event->expire_time <= current_time) {
        return 1; /* 立即到期 */
    }
    
    return next_event->expire_time - current_time;
}

/**
 * @brief 获取事件数量
 * @return 当前事件数量
 */
uint32_t rtos_tickless_get_event_count(void)
{
    uint32_t count = 0;
    rtos_time_event_t *event = g_time_event_list;
    
    while (event) {
        count++;
        event = event->next;
    }
    
    return count;
}