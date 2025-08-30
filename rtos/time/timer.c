/**
 * @file timer.c
 * @brief RTOS软件定时器实现 - 重构后的软件定时器系统
 * @author Assistant
 * @date 2024
 */

#include "timer.h"
#include "tickless.h"
#include "../core/object.h"
#include "../core/types.h"
#include <string.h>

/* 全局定时器链表头 */
static rtos_sw_timer_t *g_timer_list = NULL;

/* 定时器事件数据结构 */
typedef struct rtos_timer_event_data {
    rtos_sw_timer_t       *timer;              /* 关联的定时器 */
    rtos_time_event_t     time_event;          /* 时间事件 */
} rtos_timer_event_data_t;

/* 内部函数声明 */
static void rtos_timer_list_insert(rtos_sw_timer_t *timer);
static void rtos_timer_list_remove(rtos_sw_timer_t *timer);
static void rtos_timer_list_update(rtos_sw_timer_t *timer);
static void rtos_timer_expired_callback(void *parameter);

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
    rtos_object_init(&timer->parent, RTOS_OBJECT_TYPE_SW_TIMER, params->name, RTOS_OBJECT_FLAG_STATIC);
    
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
    
    return RTOS_OK;
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
    if (rtos_sw_timer_init(timer, params) != RTOS_OK) {
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
    
    return RTOS_OK;
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
        return RTOS_ERROR;
    }
    
    /* 设置运行状态 */
    timer->is_running = true;
    timer->remaining_time = timer->period;
    timer->last_trigger_time = rtos_tickless_get_current_time();
    
    /* 创建定时器事件数据 */
    rtos_timer_event_data_t *event_data = (rtos_timer_event_data_t *)rtos_malloc(sizeof(rtos_timer_event_data_t));
    if (!event_data) {
        timer->is_running = false;
        return RTOS_ERROR_NO_MEMORY;
    }
    
    event_data->timer = timer;
    event_data->time_event.type = RTOS_TIME_EVENT_TIMER_EXPIRE;
    event_data->time_event.object = event_data;
    event_data->time_event.callback = rtos_timer_expired_callback;
    
    /* 添加到tickless时间管理器 */
    rtos_result_t result = rtos_tickless_add_event(&event_data->time_event, timer->period);
    if (result != RTOS_OK) {
        rtos_free(event_data);
        timer->is_running = false;
        return result;
    }
    
    /* 插入到定时器链表 */
    rtos_timer_list_insert(timer);
    
    return RTOS_OK;
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
        return RTOS_ERROR;
    }
    
    /* 从链表中移除 */
    rtos_timer_list_remove(timer);
    
    /* 清除运行状态 */
    timer->is_running = false;
    
    return RTOS_OK;
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
    
    return RTOS_OK;
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
    
    return RTOS_OK;
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
    
    return RTOS_OK;
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
    
    return RTOS_OK;
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
 * @brief 初始化定时器系统
 * 使用tickless时间管理器
 */
rtos_result_t rtos_timer_system_init(void)
{
    /* 初始化tickless时间管理器 */
    return rtos_tickless_init();
}

/**
 * @brief 启动定时器系统
 */
rtos_result_t rtos_timer_system_start(void)
{
    /* 启动tickless时间管理器 */
    return rtos_tickless_start();
}

/**
 * @brief 定时器到期回调函数
 */
static void rtos_timer_expired_callback(void *parameter)
{
    rtos_timer_event_data_t *event_data = (rtos_timer_event_data_t *)parameter;
    if (!event_data || !event_data->timer) {
        return;
    }
    
    rtos_sw_timer_t *timer = event_data->timer;
    
    /* 更新统计信息 */
    timer->trigger_count++;
    timer->last_trigger_time = rtos_tickless_get_current_time();
    
    /* 调用用户回调函数 */
    if (timer->callback) {
        timer->callback(timer->parameter);
    }
    
    /* 处理自动重载 */
    if (timer->auto_reload && timer->is_running) {
        /* 重新添加定时器事件 */
        rtos_result_t result = rtos_tickless_add_event(&event_data->time_event, timer->period);
        if (result != RTOS_OK) {
            /* 添加失败，停止定时器 */
            timer->is_running = false;
            rtos_timer_list_remove(timer);
            rtos_free(event_data);
        }
    } else {
        /* 停止定时器 */
        timer->is_running = false;
        rtos_timer_list_remove(timer);
        rtos_free(event_data);
    }
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
