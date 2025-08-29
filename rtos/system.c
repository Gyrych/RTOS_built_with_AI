/**
 * @file system.c
 * @brief RTOS系统核心实现 - 重构后的系统整合模块
 * @author Assistant
 * @date 2024
 */

#include "rtos.h"
#include <string.h>
#include <stddef.h>

/* 系统状态 */
static rtos_system_state_t g_system_state = RTOS_SYSTEM_STATE_INIT;
static rtos_system_stats_t g_system_stats = {0};
static rtos_memory_stats_t g_memory_stats = {0};

/* 系统钩子函数 */
static void (*g_system_startup_hook)(void) = NULL;
static void (*g_system_shutdown_hook)(void) = NULL;
static void (*g_system_idle_hook)(void) = NULL;

/* 内部函数声明 */
static void rtos_system_update_stats(void);
static void rtos_system_cleanup_objects(void);

/**
 * @brief 初始化RTOS系统
 */
rtos_result_t rtos_system_init(void)
{
    /* 初始化硬件抽象层 */
    rtos_hw_abstraction_init();
    
    /* 初始化对象系统 */
    rtos_object_system_init();
    
    /* 初始化任务系统 */
    rtos_task_system_init();
    
    /* 初始化调度器 */
    rtos_scheduler_init();
    
    /* 初始化系统统计信息 */
    memset(&g_system_stats, 0, sizeof(g_system_stats));
    memset(&g_memory_stats, 0, sizeof(g_memory_stats));
    
    /* 设置系统状态 */
    g_system_state = RTOS_SYSTEM_STATE_READY;
    
    return RTOS_OK;
}

/**
 * @brief 启动RTOS系统
 */
rtos_result_t rtos_system_start(void)
{
    if (g_system_state != RTOS_SYSTEM_STATE_READY) {
        return RTOS_ERROR;
    }
    
    /* 执行系统启动钩子 */
    if (g_system_startup_hook) {
        g_system_startup_hook();
    }
    
    /* 设置系统状态 */
    g_system_state = RTOS_SYSTEM_STATE_RUNNING;
    
    /* 启动调度器 */
    rtos_scheduler_start();
    
    return RTOS_OK;
}

/**
 * @brief 停止RTOS系统
 */
rtos_result_t rtos_system_stop(void)
{
    if (g_system_state != RTOS_SYSTEM_STATE_RUNNING) {
        return RTOS_OK;
    }
    
    /* 停止调度器 */
    rtos_scheduler_stop();
    
    /* 设置系统状态 */
    g_system_state = RTOS_SYSTEM_STATE_SUSPENDED;
    
    return RTOS_OK;
}

/**
 * @brief 关闭RTOS系统
 */
rtos_result_t rtos_system_shutdown(void)
{
    /* 执行系统关闭钩子 */
    if (g_system_shutdown_hook) {
        g_system_shutdown_hook();
    }
    
    /* 清理所有对象 */
    rtos_system_cleanup_objects();
    
    /* 设置系统状态 */
    g_system_state = RTOS_SYSTEM_STATE_SHUTDOWN;
    
    return RTOS_OK;
}

/**
 * @brief 获取系统状态
 */
rtos_system_state_t rtos_system_get_state(void)
{
    return g_system_state;
}

/**
 * @brief 检查系统是否运行
 */
bool rtos_system_is_running(void)
{
    return (g_system_state == RTOS_SYSTEM_STATE_RUNNING);
}

/**
 * @brief 获取系统统计信息
 */
rtos_result_t rtos_system_get_stats(rtos_system_stats_t *stats)
{
    if (stats == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 更新统计信息 */
    rtos_system_update_stats();
    
    /* 复制统计信息 */
    memcpy(stats, &g_system_stats, sizeof(rtos_system_stats_t));
    
    return RTOS_OK;
}

/**
 * @brief 获取内存统计信息
 */
rtos_result_t rtos_system_get_memory_stats(rtos_memory_stats_t *stats)
{
    if (stats == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 复制内存统计信息 */
    memcpy(stats, &g_memory_stats, sizeof(rtos_memory_stats_t));
    
    return RTOS_OK;
}

/**
 * @brief 重置系统统计信息
 */
rtos_result_t rtos_system_reset_stats(void)
{
    memset(&g_system_stats, 0, sizeof(g_system_stats));
    memset(&g_memory_stats, 0, sizeof(g_memory_stats));
    
    return RTOS_OK;
}

/**
 * @brief 设置系统启动钩子函数
 */
void rtos_system_set_startup_hook(void (*hook)(void))
{
    g_system_startup_hook = hook;
}

/**
 * @brief 设置系统关闭钩子函数
 */
void rtos_system_set_shutdown_hook(void (*hook)(void))
{
    g_system_shutdown_hook = hook;
}

/**
 * @brief 设置系统空闲钩子函数
 */
void rtos_system_set_idle_hook(void (*hook)(void))
{
    g_system_idle_hook = hook;
    
    /* 同时设置任务空闲钩子 */
    rtos_task_set_idle_hook(hook);
}

/**
 * @brief 删除系统启动钩子函数
 */
void rtos_system_delete_startup_hook(void (*hook)(void))
{
    if (g_system_startup_hook == hook) {
        g_system_startup_hook = NULL;
    }
}

/**
 * @brief 删除系统关闭钩子函数
 */
void rtos_system_delete_shutdown_hook(void (*hook)(void))
{
    if (g_system_shutdown_hook == hook) {
        g_system_shutdown_hook = NULL;
    }
}

/**
 * @brief 删除系统空闲钩子函数
 */
void rtos_system_delete_idle_hook(void (*hook)(void))
{
    if (g_system_idle_hook == hook) {
        g_system_idle_hook = NULL;
        rtos_task_delete_idle_hook(hook);
    }
}

/**
 * @brief 获取系统版本信息
 */
const char *rtos_system_get_version(void)
{
    return RTOS_VERSION_STRING;
}

/**
 * @brief 获取系统版本号
 */
void rtos_system_get_version_numbers(uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    if (major) *major = RTOS_VERSION_MAJOR;
    if (minor) *minor = RTOS_VERSION_MINOR;
    if (patch) *patch = RTOS_VERSION_PATCH;
}

/**
 * @brief 系统延时(毫秒)
 */
rtos_result_t rtos_system_delay_ms(uint32_t ms)
{
    return rtos_task_delay_ms(ms);
}

/**
 * @brief 系统延时(微秒)
 */
rtos_result_t rtos_system_delay_us(uint32_t us)
{
    return rtos_task_delay_us(us);
}

/**
 * @brief 系统延时(纳秒)
 */
rtos_result_t rtos_system_delay_ns(rtos_time_ns_t ns)
{
    return rtos_task_delay_ns(ns);
}

/**
 * @brief 进入临界区
 */
rtos_critical_t rtos_system_enter_critical(void)
{
    rtos_critical_t critical;
    
    /* 禁用中断 */
    critical.state = 0; /* 需要硬件相关实现 */
    critical.nesting_count = 0;
    
    return critical;
}

/**
 * @brief 退出临界区
 */
void rtos_system_exit_critical(rtos_critical_t critical)
{
    /* 恢复中断状态 */
    /* 需要硬件相关实现 */
    (void)critical;
}

/**
 * @brief 系统空闲处理
 */
void rtos_system_idle(void)
{
    /* 执行空闲钩子 */
    if (g_system_idle_hook) {
        g_system_idle_hook();
    }
    
    /* 更新系统统计信息 */
    rtos_system_update_stats();
    
    /* 进入低功耗模式 */
    /* 这里需要根据具体硬件实现 */
}

/* 内部函数实现 */

/**
 * @brief 更新系统统计信息
 */
static void rtos_system_update_stats(void)
{
    /* 更新系统运行时间 */
    g_system_stats.system_uptime += 1; /* 需要从系统时钟获取 */
    
    /* 更新空闲时间 */
    g_system_stats.total_idle_time += 1; /* 需要从系统时钟获取 */
}

/**
 * @brief 清理系统对象
 */
static void rtos_system_cleanup_objects(void)
{
    /* 清理任务对象 */
    rtos_object_information_t *task_container = rtos_object_get_container(RTOS_OBJECT_TYPE_TASK);
    if (task_container) {
        rtos_object_container_traverse(task_container, 
                                      rtos_system_cleanup_task_callback, NULL);
    }
    
    /* 清理信号量对象 */
    rtos_object_information_t *sem_container = rtos_object_get_container(RTOS_OBJECT_TYPE_SEMAPHORE);
    if (sem_container) {
        rtos_object_container_traverse(sem_container, 
                                      rtos_system_cleanup_semaphore_callback, NULL);
    }
    
    /* 清理互斥量对象 */
    rtos_object_information_t *mutex_container = rtos_object_get_container(RTOS_OBJECT_TYPE_MUTEX);
    if (mutex_container) {
        rtos_object_container_traverse(mutex_container, 
                                      rtos_system_cleanup_mutex_callback, NULL);
    }
    
    /* 清理队列对象 */
    rtos_object_information_t *queue_container = rtos_object_get_container(RTOS_OBJECT_TYPE_QUEUE);
    if (queue_container) {
        rtos_object_container_traverse(queue_container, 
                                      rtos_system_cleanup_queue_callback, NULL);
    }
}

/**
 * @brief 清理任务对象回调
 */
void rtos_system_cleanup_task_callback(rtos_object_t *object, void *arg)
{
    (void)arg;
    
    if (object && rtos_object_is_dynamic(object)) {
        rtos_task_t *task = (rtos_task_t *)object;
        rtos_task_delete(task);
    }
}

/**
 * @brief 清理信号量对象回调
 */
void rtos_system_cleanup_semaphore_callback(rtos_object_t *object, void *arg)
{
    (void)arg;
    
    if (object && rtos_object_is_dynamic(object)) {
        rtos_semaphore_t *sem = (rtos_semaphore_t *)object;
        rtos_semaphore_delete(sem);
    }
}

/**
 * @brief 清理互斥量对象回调
 */
void rtos_system_cleanup_mutex_callback(rtos_object_t *object, void *arg)
{
    (void)arg;
    
    if (object && rtos_object_is_dynamic(object)) {
        rtos_mutex_t *mutex = (rtos_mutex_t *)object;
        rtos_mutex_delete(mutex);
    }
}

/**
 * @brief 清理队列对象回调
 */
void rtos_system_cleanup_queue_callback(rtos_object_t *object, void *arg)
{
    (void)arg;
    
    if (object && rtos_object_is_dynamic(object)) {
        rtos_queue_t *queue = (rtos_queue_t *)object;
        rtos_queue_delete(queue);
    }
}
