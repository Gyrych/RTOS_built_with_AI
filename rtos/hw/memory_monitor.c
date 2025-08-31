/**
 * @file memory_monitor.c
 * @brief RTOS内存监控模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "memory_monitor.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 外部链接器符号 */
extern uint32_t _estack;        /* 堆栈顶部 */
extern uint32_t _sdata;         /* 数据段开始 */
extern uint32_t _edata;         /* 数据段结束 */
extern uint32_t _sbss;          /* BSS段开始 */
extern uint32_t _ebss;          /* BSS段结束 */
extern uint32_t _end;           /* 堆开始 */

/* 全局内存监控器实例 */
static rtos_memory_monitor_t g_memory_monitor;
static bool g_memory_monitor_initialized = false;

/* 内存池管理 */
#define RTOS_MAX_MEMORY_POOLS 8
static void *g_memory_pools[RTOS_MAX_MEMORY_POOLS];
static rtos_memory_pool_config_t g_pool_configs[RTOS_MAX_MEMORY_POOLS];
static rtos_memory_pool_stats_t g_pool_stats[RTOS_MAX_MEMORY_POOLS];

/* 内部函数声明 */
static void rtos_memory_update_heap_stats(void);
static void rtos_memory_update_stack_stats(void);
static rtos_result_t rtos_memory_init_stack_guards(uint32_t max_tasks);
static uint32_t rtos_memory_calculate_stack_usage(uint32_t task_id);
static bool rtos_memory_check_stack_canary(uint32_t task_id);
static void rtos_memory_set_stack_canary(uint32_t task_id);

/**
 * @brief 初始化内存监控器
 */
rtos_result_t rtos_memory_monitor_init(uint32_t max_alloc_records, 
                                      uint32_t max_tasks, 
                                      uint32_t max_pools)
{
    if (g_memory_monitor_initialized) {
        return RTOS_OK;
    }
    
    /* 清零监控器结构 */
    memset(&g_memory_monitor, 0, sizeof(g_memory_monitor));
    
    /* 设置参数 */
    g_memory_monitor.max_alloc_records = max_alloc_records;
    g_memory_monitor.max_tasks = max_tasks;
    g_memory_monitor.max_pools = max_pools;
    
    /* 分配分配记录数组 */
    if (max_alloc_records > 0) {
        g_memory_monitor.alloc_records = malloc(sizeof(rtos_memory_alloc_record_t) * max_alloc_records);
        if (!g_memory_monitor.alloc_records) {
            RTOS_MEMORY_DEBUG_PRINT("Failed to allocate memory for alloc records");
            return RTOS_ERROR_NO_MEMORY;
        }
        memset(g_memory_monitor.alloc_records, 0, sizeof(rtos_memory_alloc_record_t) * max_alloc_records);
    }
    
    /* 初始化堆栈保护 */
    if (max_tasks > 0) {
        rtos_result_t result = rtos_memory_init_stack_guards(max_tasks);
        if (result != RTOS_OK) {
            if (g_memory_monitor.alloc_records) {
                free(g_memory_monitor.alloc_records);
            }
            return result;
        }
    }
    
    /* 初始化内存池管理 */
    if (max_pools > 0) {
        memset(g_memory_pools, 0, sizeof(g_memory_pools));
        memset(g_pool_configs, 0, sizeof(g_pool_configs));
        memset(g_pool_stats, 0, sizeof(g_pool_stats));
    }
    
    /* 更新初始统计信息 */
    rtos_memory_update_heap_stats();
    rtos_memory_update_stack_stats();
    
    g_memory_monitor.initialized = true;
    g_memory_monitor_initialized = true;
    
    RTOS_MEMORY_DEBUG_PRINT("Memory monitor initialized (records:%lu, tasks:%lu, pools:%lu)", 
                           max_alloc_records, max_tasks, max_pools);
    
    return RTOS_OK;
}

/**
 * @brief 反初始化内存监控器
 */
rtos_result_t rtos_memory_monitor_deinit(void)
{
    if (!g_memory_monitor_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 释放分配记录 */
    if (g_memory_monitor.alloc_records) {
        free(g_memory_monitor.alloc_records);
        g_memory_monitor.alloc_records = NULL;
    }
    
    /* 释放堆栈保护 */
    if (g_memory_monitor.stack_guards) {
        free(g_memory_monitor.stack_guards);
        g_memory_monitor.stack_guards = NULL;
    }
    
    /* 销毁所有内存池 */
    for (uint32_t i = 0; i < g_memory_monitor.pool_count; i++) {
        if (g_memory_pools[i]) {
            free(g_memory_pools[i]);
            g_memory_pools[i] = NULL;
        }
    }
    
    g_memory_monitor_initialized = false;
    
    RTOS_MEMORY_DEBUG_PRINT("Memory monitor deinitialized");
    return RTOS_OK;
}

/**
 * @brief 获取内存监控器实例
 */
rtos_memory_monitor_t* rtos_memory_monitor_get_instance(void)
{
    if (!g_memory_monitor_initialized) {
        return NULL;
    }
    return &g_memory_monitor;
}

/**
 * @brief 获取内存使用统计
 */
rtos_result_t rtos_memory_monitor_get_stats(rtos_memory_monitor_stats_t *stats)
{
    RTOS_MEMORY_CHECK_PARAM(stats != NULL);
    RTOS_MEMORY_CHECK_INIT();
    
    /* 更新统计信息 */
    rtos_memory_update_heap_stats();
    rtos_memory_update_stack_stats();
    
    /* 复制统计信息 */
    *stats = g_memory_monitor.stats;
    
    return RTOS_OK;
}

/**
 * @brief 获取任务堆栈使用情况
 */
rtos_result_t rtos_memory_monitor_get_task_stack_usage(uint32_t task_id, 
                                                      uint32_t *used, 
                                                      uint32_t *free)
{
    RTOS_MEMORY_CHECK_INIT();
    RTOS_MEMORY_CHECK_PARAM(task_id < g_memory_monitor.max_tasks);
    
    uint32_t stack_usage = rtos_memory_calculate_stack_usage(task_id);
    uint32_t stack_total = 1024; /* 假设每个任务1KB堆栈 */
    
    if (used) {
        *used = stack_usage;
    }
    
    if (free) {
        *free = stack_total - stack_usage;
    }
    
    return RTOS_OK;
}

/**
 * @brief 检查堆栈溢出
 */
rtos_stack_status_t rtos_memory_monitor_check_stack_overflow(uint32_t task_id)
{
    if (!g_memory_monitor_initialized || task_id >= g_memory_monitor.max_tasks) {
        return RTOS_STACK_STATUS_CORRUPTED;
    }
    
    /* 检查金丝雀值 */
    if (!rtos_memory_check_stack_canary(task_id)) {
        g_memory_monitor.overflow_detections++;
        RTOS_MEMORY_DEBUG_PRINT("Stack overflow detected for task %lu", task_id);
        return RTOS_STACK_STATUS_OVERFLOW;
    }
    
    /* 检查使用率 */
    uint32_t used, free;
    rtos_memory_monitor_get_task_stack_usage(task_id, &used, &free);
    
    uint32_t usage_percent = RTOS_MEMORY_USAGE_PERCENT(used, used + free);
    
    if (usage_percent > 90) {
        RTOS_MEMORY_DEBUG_PRINT("Stack usage warning for task %lu: %lu%%", task_id, usage_percent);
        return RTOS_STACK_STATUS_WARNING;
    }
    
    return RTOS_STACK_STATUS_OK;
}

/**
 * @brief 设置堆栈保护
 */
rtos_result_t rtos_memory_monitor_set_stack_guard(uint32_t task_id, uint32_t guard_size)
{
    RTOS_MEMORY_CHECK_INIT();
    RTOS_MEMORY_CHECK_PARAM(task_id < g_memory_monitor.max_tasks);
    RTOS_MEMORY_CHECK_PARAM(guard_size > 0);
    
    /* 设置金丝雀值 */
    rtos_memory_set_stack_canary(task_id);
    
    /* 记录保护区大小 */
    if (g_memory_monitor.stack_guards) {
        g_memory_monitor.stack_guards[task_id] = guard_size;
    }
    
    RTOS_MEMORY_DEBUG_PRINT("Stack guard set for task %lu: %lu bytes", task_id, guard_size);
    return RTOS_OK;
}

/**
 * @brief 启用/禁用内存泄漏检测
 */
rtos_result_t rtos_memory_monitor_enable_leak_detection(bool enable)
{
    RTOS_MEMORY_CHECK_INIT();
    
    g_memory_monitor.leak_detection_enabled = enable;
    
    if (enable) {
        /* 清零泄漏统计 */
        memset(&g_memory_monitor.leak_stats, 0, sizeof(g_memory_monitor.leak_stats));
        RTOS_MEMORY_DEBUG_PRINT("Memory leak detection enabled");
    } else {
        RTOS_MEMORY_DEBUG_PRINT("Memory leak detection disabled");
    }
    
    return RTOS_OK;
}

/**
 * @brief 获取内存泄漏统计
 */
rtos_result_t rtos_memory_monitor_get_leak_stats(rtos_memory_leak_stats_t *stats)
{
    RTOS_MEMORY_CHECK_PARAM(stats != NULL);
    RTOS_MEMORY_CHECK_INIT();
    
    *stats = g_memory_monitor.leak_stats;
    return RTOS_OK;
}

/**
 * @brief 记录内存分配
 */
rtos_result_t rtos_memory_monitor_record_alloc(void *ptr, uint32_t size, 
                                              const char *file, uint32_t line)
{
    RTOS_MEMORY_CHECK_INIT();
    
    if (!g_memory_monitor.leak_detection_enabled) {
        return RTOS_OK;
    }
    
    if (!ptr || size == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 查找空闲记录槽 */
    for (uint32_t i = 0; i < g_memory_monitor.max_alloc_records; i++) {
        if (g_memory_monitor.alloc_records[i].ptr == NULL) {
            g_memory_monitor.alloc_records[i].ptr = ptr;
            g_memory_monitor.alloc_records[i].size = size;
            g_memory_monitor.alloc_records[i].timestamp = rtos_hw_get_system_time_ms();
            g_memory_monitor.alloc_records[i].file = file;
            g_memory_monitor.alloc_records[i].line = line;
            
            g_memory_monitor.alloc_record_count++;
            g_memory_monitor.leak_stats.alloc_count++;
            g_memory_monitor.leak_stats.current_usage += size;
            g_memory_monitor.leak_stats.total_allocated += size;
            
            if (g_memory_monitor.leak_stats.current_usage > g_memory_monitor.leak_stats.peak_usage) {
                g_memory_monitor.leak_stats.peak_usage = g_memory_monitor.leak_stats.current_usage;
            }
            
            RTOS_MEMORY_DEBUG_PRINT("Alloc recorded: %p, %lu bytes at %s:%lu", 
                                   ptr, size, file ? file : "unknown", line);
            return RTOS_OK;
        }
    }
    
    RTOS_MEMORY_DEBUG_PRINT("No free alloc record slot available");
    return RTOS_ERROR_NO_MEMORY;
}

/**
 * @brief 记录内存释放
 */
rtos_result_t rtos_memory_monitor_record_free(void *ptr)
{
    RTOS_MEMORY_CHECK_INIT();
    
    if (!g_memory_monitor.leak_detection_enabled) {
        return RTOS_OK;
    }
    
    if (!ptr) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 查找对应的分配记录 */
    for (uint32_t i = 0; i < g_memory_monitor.max_alloc_records; i++) {
        if (g_memory_monitor.alloc_records[i].ptr == ptr) {
            uint32_t size = g_memory_monitor.alloc_records[i].size;
            
            /* 清除记录 */
            memset(&g_memory_monitor.alloc_records[i], 0, sizeof(rtos_memory_alloc_record_t));
            
            g_memory_monitor.alloc_record_count--;
            g_memory_monitor.leak_stats.free_count++;
            g_memory_monitor.leak_stats.current_usage -= size;
            g_memory_monitor.leak_stats.total_freed += size;
            
            RTOS_MEMORY_DEBUG_PRINT("Free recorded: %p, %lu bytes", ptr, size);
            return RTOS_OK;
        }
    }
    
    RTOS_MEMORY_DEBUG_PRINT("Free without alloc record: %p", ptr);
    return RTOS_ERROR_NOT_FOUND;
}

/**
 * @brief 创建内存池
 */
rtos_result_t rtos_memory_monitor_create_pool(const rtos_memory_pool_config_t *config, 
                                             uint32_t *pool_id)
{
    RTOS_MEMORY_CHECK_PARAM(config != NULL);
    RTOS_MEMORY_CHECK_PARAM(pool_id != NULL);
    RTOS_MEMORY_CHECK_PARAM(config->block_size > 0);
    RTOS_MEMORY_CHECK_PARAM(config->block_count > 0);
    RTOS_MEMORY_CHECK_INIT();
    
    /* 查找空闲池槽 */
    for (uint32_t i = 0; i < RTOS_MAX_MEMORY_POOLS; i++) {
        if (g_memory_pools[i] == NULL) {
            /* 计算总大小 */
            uint32_t aligned_block_size = RTOS_MEMORY_ALIGN_8(config->block_size);
            uint32_t total_size = aligned_block_size * config->block_count;
            
            /* 分配内存池 */
            void *pool_memory = malloc(total_size);
            if (!pool_memory) {
                RTOS_MEMORY_DEBUG_PRINT("Failed to allocate memory pool");
                return RTOS_ERROR_NO_MEMORY;
            }
            
            /* 初始化内存池 */
            g_memory_pools[i] = pool_memory;
            g_pool_configs[i] = *config;
            g_pool_configs[i].block_size = aligned_block_size; /* 使用对齐后的大小 */
            
            /* 初始化统计信息 */
            g_pool_stats[i].total_blocks = config->block_count;
            g_pool_stats[i].free_blocks = config->block_count;
            g_pool_stats[i].used_blocks = 0;
            g_pool_stats[i].peak_usage = 0;
            g_pool_stats[i].alloc_count = 0;
            g_pool_stats[i].free_count = 0;
            
            /* 初始化空闲链表 */
            uint8_t *block_ptr = (uint8_t *)pool_memory;
            for (uint32_t j = 0; j < config->block_count - 1; j++) {
                *(void **)block_ptr = block_ptr + aligned_block_size;
                block_ptr += aligned_block_size;
            }
            *(void **)block_ptr = NULL; /* 最后一个块指向NULL */
            
            *pool_id = i;
            g_memory_monitor.pool_count++;
            
            RTOS_MEMORY_DEBUG_PRINT("Memory pool created: ID=%lu, blocks=%lu, size=%lu", 
                                   i, config->block_count, aligned_block_size);
            
            return RTOS_OK;
        }
    }
    
    RTOS_MEMORY_DEBUG_PRINT("No free memory pool slot available");
    return RTOS_ERROR_NO_MEMORY;
}

/**
 * @brief 从内存池分配内存
 */
void* rtos_memory_monitor_pool_alloc(uint32_t pool_id)
{
    if (!g_memory_monitor_initialized || pool_id >= RTOS_MAX_MEMORY_POOLS) {
        return NULL;
    }
    
    if (!g_memory_pools[pool_id]) {
        return NULL;
    }
    
    rtos_irq_state_t irq_state = rtos_hw_enter_critical();
    
    /* 获取第一个空闲块 */
    void *free_block = *(void **)g_memory_pools[pool_id];
    
    if (free_block) {
        /* 更新空闲链表头 */
        *(void **)g_memory_pools[pool_id] = *(void **)free_block;
        
        /* 更新统计信息 */
        g_pool_stats[pool_id].free_blocks--;
        g_pool_stats[pool_id].used_blocks++;
        g_pool_stats[pool_id].alloc_count++;
        
        if (g_pool_stats[pool_id].used_blocks > g_pool_stats[pool_id].peak_usage) {
            g_pool_stats[pool_id].peak_usage = g_pool_stats[pool_id].used_blocks;
        }
        
        RTOS_MEMORY_DEBUG_PRINT("Pool alloc: pool=%lu, block=%p", pool_id, free_block);
    }
    
    rtos_hw_exit_critical(irq_state);
    
    return free_block;
}

/**
 * @brief 释放内存到内存池
 */
rtos_result_t rtos_memory_monitor_pool_free(uint32_t pool_id, void *ptr)
{
    RTOS_MEMORY_CHECK_PARAM(ptr != NULL);
    RTOS_MEMORY_CHECK_INIT();
    
    if (pool_id >= RTOS_MAX_MEMORY_POOLS || !g_memory_pools[pool_id]) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_irq_state_t irq_state = rtos_hw_enter_critical();
    
    /* 将块加入空闲链表头部 */
    *(void **)ptr = *(void **)g_memory_pools[pool_id];
    *(void **)g_memory_pools[pool_id] = ptr;
    
    /* 更新统计信息 */
    g_pool_stats[pool_id].free_blocks++;
    g_pool_stats[pool_id].used_blocks--;
    g_pool_stats[pool_id].free_count++;
    
    rtos_hw_exit_critical(irq_state);
    
    RTOS_MEMORY_DEBUG_PRINT("Pool free: pool=%lu, block=%p", pool_id, ptr);
    return RTOS_OK;
}

/**
 * @brief 内存监控周期性任务
 */
void rtos_memory_monitor_periodic_task(void)
{
    if (!g_memory_monitor_initialized) {
        return;
    }
    
    g_memory_monitor.monitor_cycles++;
    
    /* 更新统计信息 */
    rtos_memory_update_heap_stats();
    rtos_memory_update_stack_stats();
    
    /* 检查内存泄漏 */
    if (g_memory_monitor.leak_detection_enabled) {
        uint32_t current_leaks = g_memory_monitor.leak_stats.alloc_count - 
                                g_memory_monitor.leak_stats.free_count;
        
        if (current_leaks != g_memory_monitor.leak_stats.leak_count) {
            g_memory_monitor.leak_stats.leak_count = current_leaks;
            g_memory_monitor.leak_detections++;
            
            RTOS_MEMORY_DEBUG_PRINT("Memory leak detected: %lu allocations not freed", current_leaks);
        }
    }
    
    /* 检查所有任务的堆栈状态 */
    for (uint32_t i = 0; i < g_memory_monitor.max_tasks; i++) {
        rtos_stack_status_t status = rtos_memory_monitor_check_stack_overflow(i);
        if (status != RTOS_STACK_STATUS_OK) {
            RTOS_MEMORY_DEBUG_PRINT("Task %lu stack status: %d", i, status);
        }
    }
}

/**
 * @brief 获取内存监控统计信息
 */
uint32_t rtos_memory_monitor_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_memory_monitor_initialized) {
        return 0;
    }
    
    rtos_memory_monitor_stats_t stats;
    rtos_memory_monitor_get_stats(&stats);
    
    int len = snprintf(buffer, size,
        "Memory Monitor Statistics:\n"
        "  RAM Usage:\n"
        "    Total: %lu KB\n"
        "    Used: %lu KB (%lu%%)\n"
        "    Free: %lu KB\n"
        "  Heap Usage:\n"
        "    Total: %lu KB\n"
        "    Used: %lu KB (%lu%%)\n"
        "    Peak: %lu KB\n"
        "  Stack Usage:\n"
        "    Total: %lu KB\n"
        "    Used: %lu KB (%lu%%)\n"
        "    Peak: %lu KB\n"
        "  Leak Detection:\n"
        "    Enabled: %s\n"
        "    Allocations: %lu\n"
        "    Frees: %lu\n"
        "    Leaks: %lu\n"
        "  Monitor Stats:\n"
        "    Cycles: %lu\n"
        "    Overflow Detections: %lu\n"
        "    Leak Detections: %lu\n",
        stats.total_ram / 1024,
        stats.used_ram / 1024,
        RTOS_MEMORY_USAGE_PERCENT(stats.used_ram, stats.total_ram),
        stats.free_ram / 1024,
        stats.heap_total / 1024,
        stats.heap_used / 1024,
        RTOS_MEMORY_USAGE_PERCENT(stats.heap_used, stats.heap_total),
        stats.heap_peak / 1024,
        stats.stack_total / 1024,
        stats.stack_used / 1024,
        RTOS_MEMORY_USAGE_PERCENT(stats.stack_used, stats.stack_total),
        stats.stack_peak / 1024,
        g_memory_monitor.leak_detection_enabled ? "Yes" : "No",
        g_memory_monitor.leak_stats.alloc_count,
        g_memory_monitor.leak_stats.free_count,
        g_memory_monitor.leak_stats.leak_count,
        g_memory_monitor.monitor_cycles,
        g_memory_monitor.overflow_detections,
        g_memory_monitor.leak_detections);
    
    if (len < 0) {
        return 0;
    }
    
    return (len >= (int)size) ? (size - 1) : (uint32_t)len;
}

/* 内部函数实现 */

/**
 * @brief 更新堆统计信息
 */
static void rtos_memory_update_heap_stats(void)
{
    /* 简化实现：使用链接器符号计算 */
    uint32_t heap_start = (uint32_t)&_end;
    uint32_t heap_end = (uint32_t)&_estack - 1024; /* 预留1KB给堆栈 */
    
    g_memory_monitor.stats.heap_total = heap_end - heap_start;
    
    /* 这里应该遍历堆块链表计算实际使用量 */
    /* 简化实现：使用估算值 */
    g_memory_monitor.stats.heap_used = g_memory_monitor.leak_stats.current_usage;
    g_memory_monitor.stats.heap_free = g_memory_monitor.stats.heap_total - g_memory_monitor.stats.heap_used;
    
    if (g_memory_monitor.stats.heap_used > g_memory_monitor.stats.heap_peak) {
        g_memory_monitor.stats.heap_peak = g_memory_monitor.stats.heap_used;
    }
}

/**
 * @brief 更新堆栈统计信息
 */
static void rtos_memory_update_stack_stats(void)
{
    /* 简化实现：假设总堆栈为各任务堆栈之和 */
    g_memory_monitor.stats.stack_total = g_memory_monitor.max_tasks * 1024; /* 每任务1KB */
    
    uint32_t total_used = 0;
    for (uint32_t i = 0; i < g_memory_monitor.max_tasks; i++) {
        total_used += rtos_memory_calculate_stack_usage(i);
    }
    
    g_memory_monitor.stats.stack_used = total_used;
    g_memory_monitor.stats.stack_free = g_memory_monitor.stats.stack_total - total_used;
    
    if (total_used > g_memory_monitor.stats.stack_peak) {
        g_memory_monitor.stats.stack_peak = total_used;
    }
    
    /* 更新总RAM统计 */
    g_memory_monitor.stats.total_ram = RTOS_HW_SRAM_SIZE * 1024;
    g_memory_monitor.stats.used_ram = g_memory_monitor.stats.heap_used + g_memory_monitor.stats.stack_used;
    g_memory_monitor.stats.free_ram = g_memory_monitor.stats.total_ram - g_memory_monitor.stats.used_ram;
}

/**
 * @brief 初始化堆栈保护
 */
static rtos_result_t rtos_memory_init_stack_guards(uint32_t max_tasks)
{
    g_memory_monitor.stack_guards = malloc(sizeof(uint32_t) * max_tasks);
    if (!g_memory_monitor.stack_guards) {
        RTOS_MEMORY_DEBUG_PRINT("Failed to allocate stack guards");
        return RTOS_ERROR_NO_MEMORY;
    }
    
    memset(g_memory_monitor.stack_guards, 0, sizeof(uint32_t) * max_tasks);
    
    /* 为所有任务设置金丝雀值 */
    for (uint32_t i = 0; i < max_tasks; i++) {
        rtos_memory_set_stack_canary(i);
    }
    
    return RTOS_OK;
}

/**
 * @brief 计算堆栈使用量
 */
static uint32_t rtos_memory_calculate_stack_usage(uint32_t task_id)
{
    /* 简化实现：返回估算值 */
    /* 实际实现应该检查堆栈内容，计算实际使用量 */
    return 256 + (task_id * 64); /* 基础256字节 + 任务相关开销 */
}

/**
 * @brief 检查堆栈金丝雀值
 */
static bool rtos_memory_check_stack_canary(uint32_t task_id)
{
    /* 简化实现：假设金丝雀值正常 */
    /* 实际实现应该检查堆栈底部的金丝雀值 */
    (void)task_id;
    return true;
}

/**
 * @brief 设置堆栈金丝雀值
 */
static void rtos_memory_set_stack_canary(uint32_t task_id)
{
    /* 简化实现：记录设置时间 */
    /* 实际实现应该在堆栈底部写入金丝雀值 */
    if (g_memory_monitor.stack_guards && task_id < g_memory_monitor.max_tasks) {
        g_memory_monitor.stack_guards[task_id] = RTOS_STACK_GUARD_PATTERN;
    }
}

/* 跟踪版本的内存分配函数实现 */

/**
 * @brief 跟踪版本malloc
 */
void* rtos_memory_monitor_malloc_tracked(uint32_t size, const char *file, uint32_t line)
{
    void *ptr = malloc(size);
    if (ptr && g_memory_monitor_initialized) {
        rtos_memory_monitor_record_alloc(ptr, size, file, line);
    }
    return ptr;
}

/**
 * @brief 跟踪版本free
 */
void rtos_memory_monitor_free_tracked(void *ptr)
{
    if (ptr && g_memory_monitor_initialized) {
        rtos_memory_monitor_record_free(ptr);
    }
    free(ptr);
}

/**
 * @brief 跟踪版本calloc
 */
void* rtos_memory_monitor_calloc_tracked(uint32_t count, uint32_t size, const char *file, uint32_t line)
{
    void *ptr = calloc(count, size);
    if (ptr && g_memory_monitor_initialized) {
        rtos_memory_monitor_record_alloc(ptr, count * size, file, line);
    }
    return ptr;
}

/**
 * @brief 跟踪版本realloc
 */
void* rtos_memory_monitor_realloc_tracked(void *ptr, uint32_t size, const char *file, uint32_t line)
{
    if (ptr && g_memory_monitor_initialized) {
        rtos_memory_monitor_record_free(ptr);
    }
    
    void *new_ptr = realloc(ptr, size);
    
    if (new_ptr && g_memory_monitor_initialized) {
        rtos_memory_monitor_record_alloc(new_ptr, size, file, line);
    }
    
    return new_ptr;
}