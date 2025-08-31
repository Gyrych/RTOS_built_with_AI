/**
 * @file watchdog_manager.c
 * @brief RTOS看门狗管理模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "watchdog_manager.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_iwdg.h"
#include "fwlib/inc/stm32f4xx_wwdg.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局看门狗管理器实例 */
static rtos_watchdog_manager_t g_watchdog_manager;
static bool g_watchdog_manager_initialized = false;

/* 内部函数声明 */
static rtos_result_t rtos_watchdog_platform_hw_init(const rtos_watchdog_config_t *config);
static rtos_result_t rtos_watchdog_platform_hw_start(void);
static rtos_result_t rtos_watchdog_platform_hw_stop(void);
static void rtos_watchdog_platform_hw_feed(void);
static uint32_t rtos_watchdog_platform_hw_get_remaining(void);
static void rtos_watchdog_trigger_event(rtos_watchdog_event_t event, uint32_t watchdog_id);
static rtos_result_t rtos_watchdog_soft_find_task(uint32_t task_id, uint32_t *index);

/**
 * @brief 初始化看门狗管理器
 */
rtos_result_t rtos_watchdog_manager_init(uint32_t max_soft_tasks)
{
    if (g_watchdog_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_watchdog_manager, 0, sizeof(g_watchdog_manager));
    
    /* 设置默认配置 */
    g_watchdog_manager.hw_config.timeout_ms = 1000;
    g_watchdog_manager.hw_config.auto_reload = true;
    g_watchdog_manager.hw_config.reset_on_timeout = true;
    g_watchdog_manager.hw_config.prescaler = 4;
    g_watchdog_manager.hw_state = RTOS_WATCHDOG_STATE_STOPPED;
    
    /* 初始化软件看门狗 */
    if (max_soft_tasks > 0) {
        g_watchdog_manager.soft_tasks = malloc(sizeof(rtos_soft_watchdog_task_t) * max_soft_tasks);
        if (!g_watchdog_manager.soft_tasks) {
            RTOS_WATCHDOG_DEBUG_PRINT("Failed to allocate soft watchdog tasks");
            return RTOS_ERROR_NO_MEMORY;
        }
        
        memset(g_watchdog_manager.soft_tasks, 0, sizeof(rtos_soft_watchdog_task_t) * max_soft_tasks);
        g_watchdog_manager.max_soft_tasks = max_soft_tasks;
        g_watchdog_manager.soft_watchdog_enabled = true;
    }
    
    g_watchdog_manager.initialized = true;
    g_watchdog_manager_initialized = true;
    
    RTOS_WATCHDOG_DEBUG_PRINT("Watchdog manager initialized (soft tasks: %lu)", max_soft_tasks);
    return RTOS_OK;
}

/**
 * @brief 反初始化看门狗管理器
 */
rtos_result_t rtos_watchdog_manager_deinit(void)
{
    if (!g_watchdog_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 停止硬件看门狗 */
    if (g_watchdog_manager.hw_watchdog_enabled) {
        rtos_watchdog_manager_hw_stop();
    }
    
    /* 释放软件看门狗任务 */
    if (g_watchdog_manager.soft_tasks) {
        free(g_watchdog_manager.soft_tasks);
        g_watchdog_manager.soft_tasks = NULL;
    }
    
    /* 清空事件回调 */
    for (int i = 0; i < RTOS_WATCHDOG_EVENT_MAX; i++) {
        g_watchdog_manager.event_callbacks[i] = NULL;
        g_watchdog_manager.event_contexts[i] = NULL;
    }
    
    g_watchdog_manager_initialized = false;
    
    RTOS_WATCHDOG_DEBUG_PRINT("Watchdog manager deinitialized");
    return RTOS_OK;
}

/**
 * @brief 获取看门狗管理器实例
 */
rtos_watchdog_manager_t* rtos_watchdog_manager_get_instance(void)
{
    if (!g_watchdog_manager_initialized) {
        return NULL;
    }
    return &g_watchdog_manager;
}

/* 硬件看门狗接口实现 */

/**
 * @brief 初始化硬件看门狗
 */
rtos_result_t rtos_watchdog_manager_hw_init(const rtos_watchdog_config_t *config)
{
    RTOS_WATCHDOG_CHECK_PARAM(config != NULL);
    RTOS_WATCHDOG_CHECK_PARAM(config->timeout_ms > 0);
    RTOS_WATCHDOG_CHECK_INIT();
    
    /* 保存配置 */
    g_watchdog_manager.hw_config = *config;
    
    /* 调用平台相关初始化 */
    rtos_result_t result = rtos_watchdog_platform_hw_init(config);
    if (result != RTOS_OK) {
        RTOS_WATCHDOG_DEBUG_PRINT("Hardware watchdog init failed: %d", result);
        return result;
    }
    
    g_watchdog_manager.hw_watchdog_enabled = true;
    g_watchdog_manager.hw_state = RTOS_WATCHDOG_STATE_STOPPED;
    
    RTOS_WATCHDOG_DEBUG_PRINT("Hardware watchdog initialized (timeout: %lu ms)", config->timeout_ms);
    return RTOS_OK;
}

/**
 * @brief 启动硬件看门狗
 */
rtos_result_t rtos_watchdog_manager_hw_start(void)
{
    RTOS_WATCHDOG_CHECK_INIT();
    
    if (!g_watchdog_manager.hw_watchdog_enabled) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (g_watchdog_manager.hw_state == RTOS_WATCHDOG_STATE_RUNNING) {
        return RTOS_OK; /* 已经在运行 */
    }
    
    rtos_result_t result = rtos_watchdog_platform_hw_start();
    if (result == RTOS_OK) {
        g_watchdog_manager.hw_state = RTOS_WATCHDOG_STATE_RUNNING;
        RTOS_WATCHDOG_DEBUG_PRINT("Hardware watchdog started");
    }
    
    return result;
}

/**
 * @brief 停止硬件看门狗
 */
rtos_result_t rtos_watchdog_manager_hw_stop(void)
{
    RTOS_WATCHDOG_CHECK_INIT();
    
    if (!g_watchdog_manager.hw_watchdog_enabled) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    rtos_result_t result = rtos_watchdog_platform_hw_stop();
    if (result == RTOS_OK) {
        g_watchdog_manager.hw_state = RTOS_WATCHDOG_STATE_STOPPED;
        RTOS_WATCHDOG_DEBUG_PRINT("Hardware watchdog stopped");
    }
    
    return result;
}

/**
 * @brief 喂硬件看门狗
 */
rtos_result_t rtos_watchdog_manager_hw_feed(void)
{
    RTOS_WATCHDOG_CHECK_INIT();
    
    if (!g_watchdog_manager.hw_watchdog_enabled || 
        g_watchdog_manager.hw_state != RTOS_WATCHDOG_STATE_RUNNING) {
        return RTOS_ERROR_INVALID_STATE;
    }
    
    /* 更新统计信息 */
    uint32_t current_time = rtos_hw_get_system_time_ms();
    uint32_t interval = current_time - g_watchdog_manager.hw_stats.last_feed_time;
    
    g_watchdog_manager.hw_stats.feed_count++;
    g_watchdog_manager.hw_stats.last_feed_time = current_time;
    g_watchdog_manager.global_feed_count++;
    
    if (interval > g_watchdog_manager.hw_stats.max_feed_interval) {
        g_watchdog_manager.hw_stats.max_feed_interval = interval;
    }
    
    if (g_watchdog_manager.hw_stats.min_feed_interval == 0 || 
        interval < g_watchdog_manager.hw_stats.min_feed_interval) {
        g_watchdog_manager.hw_stats.min_feed_interval = interval;
    }
    
    /* 计算平均间隔 */
    if (g_watchdog_manager.hw_stats.feed_count > 1) {
        g_watchdog_manager.hw_stats.avg_feed_interval = 
            (g_watchdog_manager.hw_stats.avg_feed_interval * (g_watchdog_manager.hw_stats.feed_count - 1) + interval) /
            g_watchdog_manager.hw_stats.feed_count;
    }
    
    /* 调用平台相关喂狗 */
    rtos_watchdog_platform_hw_feed();
    
    /* 触发喂狗事件 */
    rtos_watchdog_trigger_event(RTOS_WATCHDOG_EVENT_FEED, 0);
    
    RTOS_WATCHDOG_DEBUG_PRINT("Hardware watchdog fed (interval: %lu ms)", interval);
    return RTOS_OK;
}

/**
 * @brief 获取硬件看门狗剩余时间
 */
uint32_t rtos_watchdog_manager_hw_get_remaining_time(void)
{
    if (!g_watchdog_manager_initialized || !g_watchdog_manager.hw_watchdog_enabled) {
        return 0;
    }
    
    return rtos_watchdog_platform_hw_get_remaining();
}

/**
 * @brief 获取硬件看门狗状态
 */
rtos_watchdog_state_t rtos_watchdog_manager_hw_get_state(void)
{
    if (!g_watchdog_manager_initialized) {
        return RTOS_WATCHDOG_STATE_ERROR;
    }
    
    return g_watchdog_manager.hw_state;
}

/* 软件看门狗接口实现 */

/**
 * @brief 启用软件看门狗
 */
rtos_result_t rtos_watchdog_manager_soft_enable(bool enable)
{
    RTOS_WATCHDOG_CHECK_INIT();
    
    g_watchdog_manager.soft_watchdog_enabled = enable;
    
    RTOS_WATCHDOG_DEBUG_PRINT("Software watchdog %s", enable ? "enabled" : "disabled");
    return RTOS_OK;
}

/**
 * @brief 注册软件看门狗任务
 */
rtos_result_t rtos_watchdog_manager_soft_register_task(uint32_t task_id, 
                                                      uint32_t timeout_ms, 
                                                      const char *task_name)
{
    RTOS_WATCHDOG_CHECK_PARAM(timeout_ms > 0);
    RTOS_WATCHDOG_CHECK_INIT();
    
    /* 检查是否已存在 */
    uint32_t index;
    if (rtos_watchdog_soft_find_task(task_id, &index) == RTOS_OK) {
        /* 更新现有任务 */
        g_watchdog_manager.soft_tasks[index].timeout_ms = timeout_ms;
        g_watchdog_manager.soft_tasks[index].task_name = task_name;
        g_watchdog_manager.soft_tasks[index].last_feed_time = rtos_hw_get_system_time_ms();
        g_watchdog_manager.soft_tasks[index].enabled = true;
        
        RTOS_WATCHDOG_DEBUG_PRINT("Software watchdog task updated: %lu (%s)", 
                                 task_id, task_name ? task_name : "unnamed");
        return RTOS_OK;
    }
    
    /* 查找空闲槽 */
    for (uint32_t i = 0; i < g_watchdog_manager.max_soft_tasks; i++) {
        if (!g_watchdog_manager.soft_tasks[i].enabled) {
            g_watchdog_manager.soft_tasks[i].task_id = task_id;
            g_watchdog_manager.soft_tasks[i].timeout_ms = timeout_ms;
            g_watchdog_manager.soft_tasks[i].last_feed_time = rtos_hw_get_system_time_ms();
            g_watchdog_manager.soft_tasks[i].enabled = true;
            g_watchdog_manager.soft_tasks[i].timeout_count = 0;
            g_watchdog_manager.soft_tasks[i].task_name = task_name;
            
            g_watchdog_manager.soft_task_count++;
            
            RTOS_WATCHDOG_DEBUG_PRINT("Software watchdog task registered: %lu (%s, timeout: %lu ms)", 
                                     task_id, task_name ? task_name : "unnamed", timeout_ms);
            return RTOS_OK;
        }
    }
    
    RTOS_WATCHDOG_DEBUG_PRINT("No free soft watchdog slot available");
    return RTOS_ERROR_NO_MEMORY;
}

/**
 * @brief 注销软件看门狗任务
 */
rtos_result_t rtos_watchdog_manager_soft_unregister_task(uint32_t task_id)
{
    RTOS_WATCHDOG_CHECK_INIT();
    
    uint32_t index;
    rtos_result_t result = rtos_watchdog_soft_find_task(task_id, &index);
    if (result != RTOS_OK) {
        return result;
    }
    
    /* 清除任务记录 */
    memset(&g_watchdog_manager.soft_tasks[index], 0, sizeof(rtos_soft_watchdog_task_t));
    g_watchdog_manager.soft_task_count--;
    
    RTOS_WATCHDOG_DEBUG_PRINT("Software watchdog task unregistered: %lu", task_id);
    return RTOS_OK;
}

/**
 * @brief 喂软件看门狗
 */
rtos_result_t rtos_watchdog_manager_soft_feed_task(uint32_t task_id)
{
    RTOS_WATCHDOG_CHECK_INIT();
    
    if (!g_watchdog_manager.soft_watchdog_enabled) {
        return RTOS_ERROR_INVALID_STATE;
    }
    
    uint32_t index;
    rtos_result_t result = rtos_watchdog_soft_find_task(task_id, &index);
    if (result != RTOS_OK) {
        return result;
    }
    
    /* 更新喂狗时间 */
    g_watchdog_manager.soft_tasks[index].last_feed_time = rtos_hw_get_system_time_ms();
    
    /* 触发喂狗事件 */
    rtos_watchdog_trigger_event(RTOS_WATCHDOG_EVENT_FEED, task_id);
    
    RTOS_WATCHDOG_DEBUG_PRINT("Software watchdog fed: task %lu", task_id);
    return RTOS_OK;
}

/**
 * @brief 检查所有软件看门狗任务
 */
rtos_result_t rtos_watchdog_manager_soft_check_all_tasks(void)
{
    RTOS_WATCHDOG_CHECK_INIT();
    
    if (!g_watchdog_manager.soft_watchdog_enabled) {
        return RTOS_OK;
    }
    
    uint32_t current_time = rtos_hw_get_system_time_ms();
    uint32_t timeout_tasks = 0;
    
    for (uint32_t i = 0; i < g_watchdog_manager.max_soft_tasks; i++) {
        if (!g_watchdog_manager.soft_tasks[i].enabled) {
            continue;
        }
        
        uint32_t elapsed = current_time - g_watchdog_manager.soft_tasks[i].last_feed_time;
        
        if (elapsed >= g_watchdog_manager.soft_tasks[i].timeout_ms) {
            /* 任务超时 */
            g_watchdog_manager.soft_tasks[i].timeout_count++;
            g_watchdog_manager.global_timeout_count++;
            timeout_tasks++;
            
            RTOS_WATCHDOG_DEBUG_PRINT("Software watchdog timeout: task %lu (%s), elapsed %lu ms", 
                                     g_watchdog_manager.soft_tasks[i].task_id,
                                     g_watchdog_manager.soft_tasks[i].task_name ? 
                                     g_watchdog_manager.soft_tasks[i].task_name : "unnamed",
                                     elapsed);
            
            /* 触发超时事件 */
            rtos_watchdog_trigger_event(RTOS_WATCHDOG_EVENT_TIMEOUT, g_watchdog_manager.soft_tasks[i].task_id);
            
            /* 重置喂狗时间以避免重复超时 */
            g_watchdog_manager.soft_tasks[i].last_feed_time = current_time;
        }
    }
    
    return (timeout_tasks > 0) ? RTOS_ERROR : RTOS_OK;
}

/**
 * @brief 看门狗管理器周期性任务
 */
void rtos_watchdog_manager_periodic_task(void)
{
    if (!g_watchdog_manager_initialized) {
        return;
    }
    
    g_watchdog_manager.monitor_cycles++;
    
    /* 检查软件看门狗 */
    if (g_watchdog_manager.soft_watchdog_enabled) {
        rtos_watchdog_manager_soft_check_all_tasks();
    }
    
    /* 自动喂硬件看门狗 */
    if (g_watchdog_manager.hw_watchdog_enabled && 
        g_watchdog_manager.hw_state == RTOS_WATCHDOG_STATE_RUNNING &&
        g_watchdog_manager.hw_config.auto_reload) {
        
        uint32_t remaining = rtos_watchdog_manager_hw_get_remaining_time();
        uint32_t threshold = g_watchdog_manager.hw_config.timeout_ms / 4; /* 25%阈值 */
        
        if (remaining < threshold) {
            rtos_watchdog_manager_hw_feed();
            RTOS_WATCHDOG_DEBUG_PRINT("Auto-fed hardware watchdog (remaining: %lu ms)", remaining);
        }
    }
}

/**
 * @brief 看门狗中断处理函数
 */
void rtos_watchdog_manager_interrupt_handler(void)
{
    if (!g_watchdog_manager_initialized) {
        return;
    }
    
    /* 硬件看门狗超时 */
    g_watchdog_manager.hw_stats.timeout_count++;
    g_watchdog_manager.global_timeout_count++;
    g_watchdog_manager.hw_state = RTOS_WATCHDOG_STATE_EXPIRED;
    
    RTOS_WATCHDOG_DEBUG_PRINT("Hardware watchdog timeout interrupt");
    
    /* 触发超时事件 */
    rtos_watchdog_trigger_event(RTOS_WATCHDOG_EVENT_TIMEOUT, 0);
    
    /* 如果配置为复位，则触发系统复位 */
    if (g_watchdog_manager.hw_config.reset_on_timeout) {
        rtos_watchdog_trigger_event(RTOS_WATCHDOG_EVENT_RESET, 0);
        rtos_hw_system_reset();
    }
}

/**
 * @brief 获取看门狗管理器统计信息
 */
uint32_t rtos_watchdog_manager_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_watchdog_manager_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "Watchdog Manager Statistics:\n"
        "  Hardware Watchdog:\n"
        "    Enabled: %s\n"
        "    State: %d\n"
        "    Timeout: %lu ms\n"
        "    Feed Count: %lu\n"
        "    Timeout Count: %lu\n"
        "    Remaining Time: %lu ms\n"
        "  Software Watchdog:\n"
        "    Enabled: %s\n"
        "    Registered Tasks: %lu/%lu\n"
        "    Total Timeouts: %lu\n"
        "  Global Stats:\n"
        "    Total Feeds: %lu\n"
        "    Total Timeouts: %lu\n"
        "    Monitor Cycles: %lu\n",
        g_watchdog_manager.hw_watchdog_enabled ? "Yes" : "No",
        g_watchdog_manager.hw_state,
        g_watchdog_manager.hw_config.timeout_ms,
        g_watchdog_manager.hw_stats.feed_count,
        g_watchdog_manager.hw_stats.timeout_count,
        rtos_watchdog_manager_hw_get_remaining_time(),
        g_watchdog_manager.soft_watchdog_enabled ? "Yes" : "No",
        g_watchdog_manager.soft_task_count,
        g_watchdog_manager.max_soft_tasks,
        g_watchdog_manager.global_timeout_count,
        g_watchdog_manager.global_feed_count,
        g_watchdog_manager.global_timeout_count,
        g_watchdog_manager.monitor_cycles);
    
    if (len < 0) {
        return 0;
    }
    
    return (len >= (int)size) ? (size - 1) : (uint32_t)len;
}

/* 内部函数实现 */

/**
 * @brief 平台相关硬件看门狗初始化
 */
static rtos_result_t rtos_watchdog_platform_hw_init(const rtos_watchdog_config_t *config)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 使能IWDG时钟 */
    /* IWDG时钟来自LSI，无需使能 */
    
    /* 解锁IWDG寄存器 */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    
    /* 设置预分频器 */
    IWDG_SetPrescaler(IWDG_Prescaler_32); /* LSI/32 = 32kHz/32 = 1kHz */
    
    /* 计算重载值 */
    /* timeout_ms = (reload_value + 1) * prescaler / LSI_freq * 1000 */
    /* reload_value = (timeout_ms * LSI_freq / prescaler / 1000) - 1 */
    uint32_t reload_value = (config->timeout_ms * 32000 / 32 / 1000) - 1;
    
    if (reload_value > 0xFFF) {
        reload_value = 0xFFF; /* 最大值限制 */
    }
    
    IWDG_SetReload(reload_value);
    
    /* 重载计数器 */
    IWDG_ReloadCounter();
    
    RTOS_WATCHDOG_DEBUG_PRINT("IWDG initialized: reload=%lu, timeout=%lu ms", 
                             reload_value, config->timeout_ms);
    
    return RTOS_OK;
#else
    (void)config;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关硬件看门狗启动
 */
static rtos_result_t rtos_watchdog_platform_hw_start(void)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 启动IWDG */
    IWDG_Enable();
    
    RTOS_WATCHDOG_DEBUG_PRINT("IWDG started");
    return RTOS_OK;
#else
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关硬件看门狗停止
 */
static rtos_result_t rtos_watchdog_platform_hw_stop(void)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 注意：IWDG一旦启动就无法停止，只能通过系统复位 */
    RTOS_WATCHDOG_DEBUG_PRINT("IWDG cannot be stopped once started");
    return RTOS_ERROR_NOT_SUPPORTED;
#else
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关硬件看门狗喂狗
 */
static void rtos_watchdog_platform_hw_feed(void)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 重载IWDG计数器 */
    IWDG_ReloadCounter();
#endif
}

/**
 * @brief 平台相关硬件看门狗剩余时间获取
 */
static uint32_t rtos_watchdog_platform_hw_get_remaining(void)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* IWDG没有提供读取当前计数值的寄存器 */
    /* 估算剩余时间 */
    uint32_t elapsed = rtos_hw_get_system_time_ms() - g_watchdog_manager.hw_stats.last_feed_time;
    
    if (elapsed >= g_watchdog_manager.hw_config.timeout_ms) {
        return 0;
    }
    
    return g_watchdog_manager.hw_config.timeout_ms - elapsed;
#else
    return 0;
#endif
}

/**
 * @brief 触发看门狗事件
 */
static void rtos_watchdog_trigger_event(rtos_watchdog_event_t event, uint32_t watchdog_id)
{
    if (event >= RTOS_WATCHDOG_EVENT_MAX) {
        return;
    }
    
    /* 调用回调函数 */
    if (g_watchdog_manager.event_callbacks[event]) {
        g_watchdog_manager.event_callbacks[event](event, watchdog_id, g_watchdog_manager.event_contexts[event]);
    }
}

/**
 * @brief 查找软件看门狗任务
 */
static rtos_result_t rtos_watchdog_soft_find_task(uint32_t task_id, uint32_t *index)
{
    for (uint32_t i = 0; i < g_watchdog_manager.max_soft_tasks; i++) {
        if (g_watchdog_manager.soft_tasks[i].enabled && 
            g_watchdog_manager.soft_tasks[i].task_id == task_id) {
            if (index) {
                *index = i;
            }
            return RTOS_OK;
        }
    }
    
    return RTOS_ERROR_NOT_FOUND;
}

/**
 * @brief 注册看门狗事件回调
 */
rtos_result_t rtos_watchdog_manager_register_event_callback(rtos_watchdog_event_t event,
                                                           rtos_watchdog_event_callback_t callback,
                                                           void *context)
{
    RTOS_WATCHDOG_CHECK_PARAM(event < RTOS_WATCHDOG_EVENT_MAX);
    RTOS_WATCHDOG_CHECK_PARAM(callback != NULL);
    RTOS_WATCHDOG_CHECK_INIT();
    
    g_watchdog_manager.event_callbacks[event] = callback;
    g_watchdog_manager.event_contexts[event] = context;
    
    RTOS_WATCHDOG_DEBUG_PRINT("Watchdog event callback registered: %d", event);
    return RTOS_OK;
}