/**
 * @file memory_monitor.h
 * @brief RTOS内存监控模块 - 面向对象的内存管理抽象
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_MEMORY_MONITOR_H__
#define __RTOS_MEMORY_MONITOR_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 内存区域类型定义 */
typedef enum {
    RTOS_MEMORY_REGION_FLASH = 0,
    RTOS_MEMORY_REGION_SRAM,
    RTOS_MEMORY_REGION_CCM,
    RTOS_MEMORY_REGION_HEAP,
    RTOS_MEMORY_REGION_STACK,
    RTOS_MEMORY_REGION_MAX
} rtos_memory_region_t;

/* 堆栈状态定义 */
typedef enum {
    RTOS_STACK_STATUS_OK = 0,       /**< 堆栈正常 */
    RTOS_STACK_STATUS_WARNING,      /**< 堆栈使用量警告 */
    RTOS_STACK_STATUS_OVERFLOW,     /**< 堆栈溢出 */
    RTOS_STACK_STATUS_CORRUPTED     /**< 堆栈数据损坏 */
} rtos_stack_status_t;

/* 内存使用统计结构 */
typedef struct {
    uint32_t total_ram;             /**< 总RAM大小 */
    uint32_t free_ram;              /**< 空闲RAM大小 */
    uint32_t used_ram;              /**< 已使用RAM大小 */
    uint32_t heap_total;            /**< 堆总大小 */
    uint32_t heap_used;             /**< 堆已使用大小 */
    uint32_t heap_free;             /**< 堆空闲大小 */
    uint32_t heap_peak;             /**< 堆使用峰值 */
    uint32_t stack_total;           /**< 总堆栈大小 */
    uint32_t stack_used;            /**< 已使用堆栈大小 */
    uint32_t stack_free;            /**< 空闲堆栈大小 */
    uint32_t stack_peak;            /**< 堆栈使用峰值 */
    uint32_t fragmentation_percent; /**< 内存碎片率 */
} rtos_memory_stats_t;

/* 内存泄漏统计结构 */
typedef struct {
    uint32_t alloc_count;           /**< 分配次数 */
    uint32_t free_count;            /**< 释放次数 */
    uint32_t leak_count;            /**< 泄漏次数 */
    uint32_t peak_usage;            /**< 峰值使用量 */
    uint32_t current_usage;         /**< 当前使用量 */
    uint32_t total_allocated;       /**< 总分配量 */
    uint32_t total_freed;           /**< 总释放量 */
} rtos_memory_leak_stats_t;

/* 内存分配记录 */
typedef struct rtos_memory_alloc_record {
    void *ptr;                      /**< 分配的指针 */
    uint32_t size;                  /**< 分配大小 */
    uint32_t timestamp;             /**< 分配时间戳 */
    const char *file;               /**< 分配文件名 */
    uint32_t line;                  /**< 分配行号 */
    struct rtos_memory_alloc_record *next; /**< 下一个记录 */
} rtos_memory_alloc_record_t;

/* 内存池配置 */
typedef struct {
    uint32_t block_size;            /**< 块大小 */
    uint32_t block_count;           /**< 块数量 */
    uint32_t alignment;             /**< 对齐要求 */
    bool thread_safe;               /**< 线程安全 */
    const char *name;               /**< 池名称 */
} rtos_memory_pool_config_t;

/* 内存池统计 */
typedef struct {
    uint32_t total_blocks;          /**< 总块数 */
    uint32_t free_blocks;           /**< 空闲块数 */
    uint32_t used_blocks;           /**< 已使用块数 */
    uint32_t peak_usage;            /**< 峰值使用量 */
    uint32_t alloc_count;           /**< 分配次数 */
    uint32_t free_count;            /**< 释放次数 */
} rtos_memory_pool_stats_t;

/* 内存监控器类结构 */
typedef struct {
    /* 私有成员 */
    bool initialized;
    bool leak_detection_enabled;
    rtos_memory_stats_t stats;
    rtos_memory_leak_stats_t leak_stats;
    
    /* 分配记录链表 */
    rtos_memory_alloc_record_t *alloc_records;
    uint32_t alloc_record_count;
    uint32_t max_alloc_records;
    
    /* 堆栈监控 */
    uint32_t *stack_guards;
    uint32_t max_tasks;
    
    /* 内存池管理 */
    void **memory_pools;
    rtos_memory_pool_config_t *pool_configs;
    rtos_memory_pool_stats_t *pool_stats;
    uint32_t max_pools;
    uint32_t pool_count;
    
    /* 统计信息 */
    uint32_t monitor_cycles;
    uint32_t overflow_detections;
    uint32_t leak_detections;
    
} rtos_memory_monitor_t;

/**
 * @brief 初始化内存监控器
 * @param max_alloc_records 最大分配记录数
 * @param max_tasks 最大任务数
 * @param max_pools 最大内存池数
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_init(uint32_t max_alloc_records, 
                                      uint32_t max_tasks, 
                                      uint32_t max_pools);

/**
 * @brief 反初始化内存监控器
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_deinit(void);

/**
 * @brief 获取内存监控器实例
 * @return 内存监控器指针
 */
rtos_memory_monitor_t* rtos_memory_monitor_get_instance(void);

/**
 * @brief 获取内存使用统计
 * @param stats 统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_get_stats(rtos_memory_stats_t *stats);

/**
 * @brief 获取任务堆栈使用情况
 * @param task_id 任务ID
 * @param used 已使用堆栈大小指针
 * @param free 空闲堆栈大小指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_get_task_stack_usage(uint32_t task_id, 
                                                      uint32_t *used, 
                                                      uint32_t *free);

/**
 * @brief 检查堆栈溢出
 * @param task_id 任务ID
 * @return 堆栈状态
 */
rtos_stack_status_t rtos_memory_monitor_check_stack_overflow(uint32_t task_id);

/**
 * @brief 设置堆栈保护
 * @param task_id 任务ID
 * @param guard_size 保护区大小
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_set_stack_guard(uint32_t task_id, uint32_t guard_size);

/**
 * @brief 启用/禁用内存泄漏检测
 * @param enable 是否启用
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_enable_leak_detection(bool enable);

/**
 * @brief 获取内存泄漏统计
 * @param stats 泄漏统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_get_leak_stats(rtos_memory_leak_stats_t *stats);

/**
 * @brief 记录内存分配
 * @param ptr 分配的指针
 * @param size 分配大小
 * @param file 文件名
 * @param line 行号
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_record_alloc(void *ptr, uint32_t size, 
                                              const char *file, uint32_t line);

/**
 * @brief 记录内存释放
 * @param ptr 释放的指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_record_free(void *ptr);

/**
 * @brief 创建内存池
 * @param config 内存池配置
 * @param pool_id 返回的池ID指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_create_pool(const rtos_memory_pool_config_t *config, 
                                             uint32_t *pool_id);

/**
 * @brief 销毁内存池
 * @param pool_id 池ID
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_destroy_pool(uint32_t pool_id);

/**
 * @brief 从内存池分配内存
 * @param pool_id 池ID
 * @return 分配的内存指针，失败返回NULL
 */
void* rtos_memory_monitor_pool_alloc(uint32_t pool_id);

/**
 * @brief 释放内存到内存池
 * @param pool_id 池ID
 * @param ptr 内存指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_pool_free(uint32_t pool_id, void *ptr);

/**
 * @brief 获取内存池统计
 * @param pool_id 池ID
 * @param stats 统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_monitor_get_pool_stats(uint32_t pool_id, 
                                                rtos_memory_pool_stats_t *stats);

/**
 * @brief 内存监控周期性任务
 * 应该被定时调用以更新内存统计信息
 */
void rtos_memory_monitor_periodic_task(void);

/**
 * @brief 内存错误处理函数
 * @param error_type 错误类型
 * @param address 错误地址
 */
void rtos_memory_monitor_error_handler(uint32_t error_type, void *address);

/**
 * @brief 获取内存监控统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_memory_monitor_get_statistics(char *buffer, uint32_t size);

/**
 * @brief 生成内存使用报告
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_memory_monitor_generate_report(char *buffer, uint32_t size);

/* 内存分配跟踪宏 */
#ifdef RTOS_MEMORY_LEAK_DETECTION
#define rtos_malloc(size) rtos_memory_monitor_malloc_tracked((size), __FILE__, __LINE__)
#define rtos_free(ptr) rtos_memory_monitor_free_tracked((ptr))
#define rtos_calloc(count, size) rtos_memory_monitor_calloc_tracked((count), (size), __FILE__, __LINE__)
#define rtos_realloc(ptr, size) rtos_memory_monitor_realloc_tracked((ptr), (size), __FILE__, __LINE__)
#else
#define rtos_malloc(size) malloc(size)
#define rtos_free(ptr) free(ptr)
#define rtos_calloc(count, size) calloc((count), (size))
#define rtos_realloc(ptr, size) realloc((ptr), (size))
#endif

/* 跟踪版本的内存分配函数 */
void* rtos_memory_monitor_malloc_tracked(uint32_t size, const char *file, uint32_t line);
void rtos_memory_monitor_free_tracked(void *ptr);
void* rtos_memory_monitor_calloc_tracked(uint32_t count, uint32_t size, const char *file, uint32_t line);
void* rtos_memory_monitor_realloc_tracked(void *ptr, uint32_t size, const char *file, uint32_t line);

/* 堆栈保护宏 */
#define RTOS_STACK_GUARD_PATTERN    0xDEADBEEF
#define RTOS_STACK_CANARY_SIZE      16

/* 内存对齐宏 */
#define RTOS_MEMORY_ALIGN_4(x)      RTOS_HW_ALIGN_UP((x), 4)
#define RTOS_MEMORY_ALIGN_8(x)      RTOS_HW_ALIGN_UP((x), 8)
#define RTOS_MEMORY_ALIGN_16(x)     RTOS_HW_ALIGN_UP((x), 16)

/* 内存使用率计算宏 */
#define RTOS_MEMORY_USAGE_PERCENT(used, total) \
    (((used) * 100) / ((total) > 0 ? (total) : 1))

/* 调试宏定义 */
#ifdef RTOS_MEMORY_DEBUG
#define RTOS_MEMORY_DEBUG_PRINT(fmt, ...) \
    printf("[MEMORY] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_MEMORY_DEBUG_PRINT(fmt, ...)
#endif

/* 错误检查宏定义 */
#ifdef RTOS_MEMORY_ERROR_CHECK
#define RTOS_MEMORY_CHECK_PARAM(param) \
    do { \
        if (!(param)) { \
            RTOS_MEMORY_DEBUG_PRINT("Parameter check failed: %s", #param); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
    
#define RTOS_MEMORY_CHECK_INIT() \
    do { \
        if (!rtos_memory_monitor_get_instance()) { \
            RTOS_MEMORY_DEBUG_PRINT("Memory monitor not initialized"); \
            return RTOS_ERROR_NOT_INITIALIZED; \
        } \
    } while(0)
#else
#define RTOS_MEMORY_CHECK_PARAM(param)
#define RTOS_MEMORY_CHECK_INIT()
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_MEMORY_MONITOR_H__ */