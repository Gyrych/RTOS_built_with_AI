/**
 * @file rtos_system.c
 * @brief RTOS系统核心实现 - 整合所有模块
 */

#include "rtos.h"
#include "rtos_hw.h"
#include <string.h>

/* 系统状态 */
static bool system_initialized = false;

/* 内存管理简化实现 */
static uint8_t heap_memory[RTOS_HEAP_SIZE];
static uint32_t heap_used = 0;

/**
 * @brief 初始化RTOS系统
 */
rtos_result_t rtos_system_init(void)
{
    if (system_initialized) {
        return RTOS_OK;
    }
    
    /* 初始化硬件层 */
    rtos_hw_init();
    
    /* 初始化对象系统 */
    rtos_object_system_init();
    
    /* 初始化任务系统 */
    rtos_task_system_init();
    
    /* 初始化调度器 */
    rtos_scheduler_init();
    
    system_initialized = true;
    
    return RTOS_OK;
}

/**
 * @brief 启动RTOS调度器
 */
rtos_result_t rtos_system_start(void)
{
    if (!system_initialized) {
        return RTOS_ERROR;
    }
    
    /* 启动调度器 */
    rtos_scheduler_start();
    
    return RTOS_OK;
}

/**
 * @brief 获取系统运行时间(纳秒)
 */
rtos_time_ns_t rtos_system_get_time_ns(void)
{
    return rtos_hw_get_time_ns();
}

/**
 * @brief 获取系统运行时间(微秒)
 */
uint32_t rtos_system_get_time_us(void)
{
    return rtos_hw_get_time_us();
}

/**
 * @brief 获取系统运行时间(毫秒)
 */
uint32_t rtos_system_get_time_ms(void)
{
    return rtos_hw_get_time_us() / 1000;
}

/**
 * @brief 系统延时(纳秒)
 */
rtos_result_t rtos_system_delay_ns(rtos_time_ns_t ns)
{
    return rtos_task_delay(ns);
}

/**
 * @brief 系统延时(微秒)
 */
rtos_result_t rtos_system_delay_us(uint32_t us)
{
    return rtos_task_udelay(us);
}

/**
 * @brief 系统延时(毫秒)
 */
rtos_result_t rtos_system_delay_ms(uint32_t ms)
{
    return rtos_task_mdelay(ms);
}

/**
 * @brief 检查调度器是否运行
 */
bool rtos_system_scheduler_is_running(void)
{
    return rtos_scheduler_is_running();
}

/**
 * @brief 获取版本信息
 */
const char *rtos_system_get_version(void)
{
    return RTOS_VERSION_STRING;
}

/**
 * @brief 简化的内存分配
 */
void *rtos_malloc(uint32_t size)
{
    void *ptr = NULL;
    
    rtos_enter_critical();
    
    /* 简单的线性分配器 */
    if (heap_used + size <= RTOS_HEAP_SIZE) {
        ptr = &heap_memory[heap_used];
        heap_used += RTOS_ALIGN(size, RTOS_ALIGN_SIZE);
    }
    
    rtos_exit_critical();
    
    return ptr;
}

/**
 * @brief 简化的内存释放(此实现中不支持释放)
 */
void rtos_free(void *ptr)
{
    /* 简化实现：不支持释放 */
    (void)ptr;
}

#ifdef RTOS_DEBUG
/**
 * @brief 断言失败处理
 */
void rtos_system_assert_failed(const char *file, int line, const char *expr)
{
    printf("ASSERTION FAILED: %s at %s:%d\n", expr, file, line);
    while (1) {
        /* 停机 */
    }
}
#endif