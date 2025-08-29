/**
 * @file timer.c
 * @brief RTOS软件定时器实现 - 重构后的软件定时器系统
 * @author Assistant
 * @date 2024
 */

#include "timer.h"
#include "../core/object.h"
#include "../core/types.h"
#include <string.h>

/* 全局定时器链表头 */
static rtos_sw_timer_t *g_timer_list = NULL;

/* 内部函数声明 */
static void rtos_timer_list_insert(rtos_sw_timer_t *timer);
static void rtos_timer_list_remove(rtos_sw_timer_t *timer);
static void rtos_timer_list_update(rtos_sw_timer_t *timer);
static rtos_time_ns_t rtos_timer_get_current_time(void);

/**
 * @brief 初始化软件定时器 - 静态方式
 */
rtos_result_t rtos_sw_timer_init(rtos_sw_timer_t *timer,
                                 const rtos_sw_timer_create_params_t *params)
{
    if (!timer || !params) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!params->callback) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化对象基类 */
    rtos_object_init(&timer->parent, RTOS_OBJECT_TYPE_TIMER, params->name);
    
    /* 初始化定时器属性 */
    timer->callback = params->callback;
    timer->parameter = params->parameter;
    timer->period = params->period;
    timer->remaining_time = params->period;
    timer->auto_reload = params->auto_reload;
    timer->is_running = false;
    
    /* 初始化链表节点 */
    timer->next = NULL;
    timer->prev = NULL;
    
    /* 初始化统计信息 */
    timer->trigger_count = 0;
    timer->last_trigger_time = 0;
    timer->total_runtime = 0;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 创建软件定时器 - 动态方式
 */
rtos_sw_timer_t *rtos_sw_timer_create(const rtos_sw_timer_create_params_t *params)
{
    rtos_sw_timer_t *timer;
    
    if (!params) {
        return NULL;
    }
    
    /* 分配内存 */
    timer = (rtos_sw_timer_t *)rtos_malloc(sizeof(rtos_sw_timer_t));
    if (!timer) {
        return NULL;
    }
    
    /* 初始化定时器 */
    if (rtos_sw_timer_init(timer, params) != RTOS_SUCCESS) {
        rtos_free(timer);
        return NULL;
    }
    
    return timer;
}

/**
 * @brief 删除软件定时器
 */
rtos_result_t rtos_sw_timer_delete(rtos_sw_timer_t *timer)
{
    if (!timer) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 如果定时器正在运行，先停止它 */
    if (timer->is_running) {
        rtos_sw_timer_stop(timer);
    }
    
    /* 从链表中移除 */
    rtos_timer_list_remove(timer);
    
    /* 释放内存 */
    rtos_free(timer);
    
    return RTOS_SUCCESS;
}

/**
 * @brief 启动软件定时器
 */
rtos_result_t rtos_sw_timer_start(rtos_sw_timer_t *timer)
{
    if (!timer) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (timer->is_running) {
        return RTOS_ERROR_ALREADY_EXISTS;
    }
    
    /* 设置运行状态 */
    timer->is_running = true;
    timer->remaining_time = timer->period;
    timer->last_trigger_time = rtos_timer_get_current_time();
    
    /* 插入到定时器链表 */
    rtos_timer_list_insert(timer);
    
    return RTOS_SUCCESS;
}

/**
 * @brief 停止软件定时器
 */
rtos_result_t rtos_sw_timer_stop(rtos_sw_timer_t *timer)
{
    if (!timer) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!timer->is_running) {
        return RTOS_ERROR_NOT_FOUND;
    }
    
    /* 从链表中移除 */
    rtos_timer_list_remove(timer);
    
    /* 清除运行状态 */
    timer->is_running = false;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 重置软件定时器
 */
rtos_result_t rtos_sw_timer_reset(rtos_sw_timer_t *timer)
{
    if (!timer) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 重置剩余时间 */
    timer->remaining_time = timer->period;
    
    /* 如果正在运行，重新插入链表 */
    if (timer->is_running) {
        rtos_timer_list_update(timer);
    }
    
    return RTOS_SUCCESS;
}

/**
 * @brief 设置软件定时器周期
 */
rtos_result_t rtos_sw_timer_set_period(rtos_sw_timer_t *timer, rtos_time_ns_t period)
{
    if (!timer) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (period == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 更新周期 */
    timer->period = period;
    
    /* 如果正在运行，更新剩余时间 */
    if (timer->is_running) {
        timer->remaining_time = period;
        rtos_timer_list_update(timer);
    }
    
    return RTOS_SUCCESS;
}

/**
 * @brief 获取软件定时器周期
 */
rtos_time_ns_t rtos_sw_timer_get_period(const rtos_sw_timer_t *timer)
{
    if (!timer) {
        return 0;
    }
    
    return timer->period;
}

/**
 * @brief 设置软件定时器回调函数
 */
rtos_result_t rtos_sw_timer_set_callback(rtos_sw_timer_t *timer,
                                         rtos_timer_callback_t callback,
                                         void *parameter)
{
    if (!timer || !callback) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    timer->callback = callback;
    timer->parameter = parameter;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 获取软件定时器信息
 */
rtos_result_t rtos_sw_timer_get_info(const rtos_sw_timer_t *timer,
                                     rtos_sw_timer_info_t *info)
{
    if (!timer || !info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 复制基本信息 */
    strncpy(info->name, timer->parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    info->is_running = timer->is_running;
    info->period = timer->period;
    info->remaining_time = timer->remaining_time;
    info->auto_reload = timer->auto_reload;
    info->trigger_count = timer->trigger_count;
    info->last_trigger_time = timer->last_trigger_time;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 检查软件定时器是否运行
 */
bool rtos_sw_timer_is_running(const rtos_sw_timer_t *timer)
{
    if (!timer) {
        return false;
    }
    
    return timer->is_running;
}

/**
 * @brief 获取软件定时器剩余时间
 */
rtos_time_ns_t rtos_sw_timer_get_remaining_time(const rtos_sw_timer_t *timer)
{
    if (!timer) {
        return 0;
    }
    
    return timer->remaining_time;
}

/**
 * @brief 获取软件定时器触发次数
 */
uint32_t rtos_sw_timer_get_trigger_count(const rtos_sw_timer_t *timer)
{
    if (!timer) {
        return 0;
    }
    
    return timer->trigger_count;
}

/**
 * @brief 定时器系统滴答处理函数
 * 此函数需要由系统定时器中断调用
 */
void rtos_timer_tick(rtos_time_ns_t tick_period)
{
    rtos_sw_timer_t *timer = g_timer_list;
    rtos_sw_timer_t *next;
    
    while (timer) {
        next = timer->next;
        
        if (timer->is_running) {
            /* 更新剩余时间 */
            if (timer->remaining_time > tick_period) {
                timer->remaining_time -= tick_period;
            } else {
                /* 定时器到期 */
                timer->remaining_time = 0;
                timer->trigger_count++;
                
                /* 调用回调函数 */
                if (timer->callback) {
                    timer->callback(timer->parameter);
                }
                
                /* 处理自动重载 */
                if (timer->auto_reload) {
                    timer->remaining_time = timer->period;
                    timer->last_trigger_time = rtos_timer_get_current_time();
                } else {
                    /* 停止定时器 */
                    timer->is_running = false;
                    rtos_timer_list_remove(timer);
                }
            }
        }
        
        timer = next;
    }
}

/**
 * @brief 获取当前系统时间
 */
static rtos_time_ns_t rtos_timer_get_current_time(void)
{
    /* 这里应该调用系统时间函数 */
    /* 暂时返回0，实际实现中需要集成系统时间 */
    return 0;
}

/**
 * @brief 将定时器插入链表
 */
static void rtos_timer_list_insert(rtos_sw_timer_t *timer)
{
    rtos_sw_timer_t *current;
    rtos_sw_timer_t *prev = NULL;
    
    /* 从链表中移除（如果已存在） */
    rtos_timer_list_remove(timer);
    
    /* 按剩余时间排序插入 */
    current = g_timer_list;
    while (current && current->remaining_time <= timer->remaining_time) {
        prev = current;
        current = current->next;
    }
    
    /* 插入到prev之后 */
    if (prev) {
        timer->next = prev->next;
        timer->prev = prev;
        if (prev->next) {
            prev->next->prev = timer;
        }
        prev->next = timer;
    } else {
        /* 插入到链表头 */
        timer->next = g_timer_list;
        timer->prev = NULL;
        if (g_timer_list) {
            g_timer_list->prev = timer;
        }
        g_timer_list = timer;
    }
}

/**
 * @brief 从链表中移除定时器
 */
static void rtos_timer_list_remove(rtos_sw_timer_t *timer)
{
    if (!timer) {
        return;
    }
    
    /* 更新链表指针 */
    if (timer->prev) {
        timer->prev->next = timer->next;
    } else {
        g_timer_list = timer->next;
    }
    
    if (timer->next) {
        timer->next->prev = timer->prev;
    }
    
    /* 清除定时器链表指针 */
    timer->next = NULL;
    timer->prev = NULL;
}

/**
 * @brief 更新定时器在链表中的位置
 */
static void rtos_timer_list_update(rtos_sw_timer_t *timer)
{
    /* 重新插入以保持排序 */
    rtos_timer_list_insert(timer);
}

/**
 * @brief 获取定时器链表头
 */
rtos_sw_timer_t *rtos_timer_get_list_head(void)
{
    return g_timer_list;
}

/**
 * @brief 获取定时器数量
 */
uint32_t rtos_timer_get_count(void)
{
    uint32_t count = 0;
    rtos_sw_timer_t *timer = g_timer_list;
    
    while (timer) {
        count++;
        timer = timer->next;
    }
    
    return count;
}
