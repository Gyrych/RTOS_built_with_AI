/**
 * @file types.h
 * @brief RTOS统一类型定义 - 重构后的核心类型系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_TYPES_H__
#define __RTOS_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* 系统版本信息 */
#define RTOS_VERSION_MAJOR      2
#define RTOS_VERSION_MINOR      0
#define RTOS_VERSION_PATCH      0
#define RTOS_VERSION_STRING     "RTOS v2.0.0 - Refactored Architecture"

/* 系统配置常量 */
#define RTOS_MAX_TASKS          16
#define RTOS_MAX_SEMAPHORES     8
#define RTOS_MAX_MUTEXES        8
#define RTOS_MAX_QUEUES         8
#define RTOS_MAX_SW_TIMERS      16
#define RTOS_MAX_EVENT_GROUPS   8
#define RTOS_MAX_MEMORY_POOLS   8
#define RTOS_PRIORITY_LEVELS    32
#define RTOS_STACK_SIZE_MIN     256

/* 时间单位定义 */
#define RTOS_TIME_UNIT_NS       1
#define RTOS_TIME_UNIT_US       1000
#define RTOS_TIME_UNIT_MS       1000000

/* 统一错误码定义 */
typedef enum {
    RTOS_OK = 0,
    RTOS_ERROR,
    RTOS_ERROR_TIMEOUT,
    RTOS_ERROR_NO_MEMORY,
    RTOS_ERROR_INVALID_PARAM,
    RTOS_ERROR_RESOURCE_BUSY,
    RTOS_ERROR_DEADLOCK,
    RTOS_ERROR_STACK_OVERFLOW,
    RTOS_ERROR_MEMORY_CORRUPTION,
    RTOS_ERROR_NOT_IMPLEMENTED,
    RTOS_ERROR_DELETED,
    RTOS_ERROR_NOT_FOUND,
    RTOS_ERROR_ALREADY_EXISTS,
    RTOS_ERROR_CORRUPTED
} rtos_result_t;

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define RTOS_SUCCESS RTOS_OK

/* 时间类型定义 */
typedef uint64_t rtos_time_ns_t;
typedef uint32_t rtos_time_us_t;
typedef uint32_t rtos_time_ms_t;

/* 任务状态定义 */
typedef enum {
    RTOS_TASK_STATE_INIT = 0,
    RTOS_TASK_STATE_READY,
    RTOS_TASK_STATE_RUNNING,
    RTOS_TASK_STATE_BLOCKED,
    RTOS_TASK_STATE_SUSPENDED,
    RTOS_TASK_STATE_DELETED
} rtos_task_state_t;

/* 任务优先级定义 */
typedef uint8_t rtos_priority_t;
#define RTOS_PRIORITY_IDLE      (RTOS_PRIORITY_LEVELS - 1)
#define RTOS_PRIORITY_LOW       (RTOS_PRIORITY_LEVELS - 8)
#define RTOS_PRIORITY_NORMAL    (RTOS_PRIORITY_LEVELS / 2)
#define RTOS_PRIORITY_HIGH      8
#define RTOS_PRIORITY_CRITICAL  0

/* 内核对象类型定义 */
typedef enum {
    RTOS_OBJECT_TYPE_NULL = 0,
    RTOS_OBJECT_TYPE_TASK,
    RTOS_OBJECT_TYPE_SEMAPHORE,
    RTOS_OBJECT_TYPE_MUTEX,
    RTOS_OBJECT_TYPE_QUEUE,
    RTOS_OBJECT_TYPE_EVENT_GROUP,
    RTOS_OBJECT_TYPE_SW_TIMER,
    RTOS_OBJECT_TYPE_MEMORY_POOL,
    RTOS_OBJECT_TYPE_DEVICE
} rtos_object_type_t;

/* 对象标志定义 */
typedef enum {
    RTOS_OBJECT_FLAG_NONE = 0x00,
    RTOS_OBJECT_FLAG_STATIC = 0x01,
    RTOS_OBJECT_FLAG_DYNAMIC = 0x02,
    RTOS_OBJECT_FLAG_SYSTEM = 0x04
} rtos_object_flag_t;

/* 任务控制标志 */
typedef enum {
    RTOS_TASK_FLAG_NONE = 0x00,
    RTOS_TASK_FLAG_STACK_CHECK = 0x01,
    RTOS_TASK_FLAG_PRIORITY_INHERIT = 0x02,
    RTOS_TASK_FLAG_TIMESLICE = 0x04
} rtos_task_flag_t;

/* 同步对象标志 */
typedef enum {
    RTOS_SYNC_FLAG_NONE = 0x00,
    RTOS_SYNC_FLAG_PRIORITY_INHERIT = 0x01,
    RTOS_SYNC_FLAG_ROBUST = 0x02
} rtos_sync_flag_t;

/* 时间片定义 */
typedef uint32_t rtos_timeslice_t;
#define RTOS_TIMESLICE_DEFAULT  10000   /* 10ms */
#define RTOS_TIMESLICE_INF      0       /* 无限时间片 */

/* 栈大小定义 */
typedef uint32_t rtos_stack_size_t;
#define RTOS_STACK_SIZE_DEFAULT 1024
#define RTOS_STACK_SIZE_MAX     65536

/* 超时时间定义 */
typedef uint64_t rtos_timeout_t;
#define RTOS_TIMEOUT_INF        0xFFFFFFFFFFFFFFFFULL
#define RTOS_TIMEOUT_IMMEDIATE  0
#define RTOS_WAIT_FOREVER       RTOS_TIMEOUT_INF

/* 系统状态定义 */
typedef enum {
    RTOS_SYSTEM_STATE_INIT = 0,
    RTOS_SYSTEM_STATE_READY,
    RTOS_SYSTEM_STATE_RUNNING,
    RTOS_SYSTEM_STATE_SUSPENDED,
    RTOS_SYSTEM_STATE_SHUTDOWN
} rtos_system_state_t;

/* 中断状态定义 */
typedef uint32_t rtos_interrupt_state_t;

/* 临界区管理 */
typedef struct {
    rtos_interrupt_state_t state;
    uint32_t nesting_count;
} rtos_critical_t;

/* 系统统计信息 */
typedef struct {
    uint32_t total_task_switches;
    uint32_t total_interrupts;
    rtos_time_ns_t total_idle_time;
    rtos_time_ns_t system_uptime;
} rtos_system_stats_t;

/* 任务统计信息 - 修复命名错误 */
typedef struct {
    rtos_time_ns_t total_runtime;
    uint32_t switch_count;
    uint32_t max_stack_usage;
    uint32_t current_stack_usage;
} rtos_task_stats_t;

/* 内存统计信息 */
typedef struct {
    uint32_t total_allocated;
    uint32_t total_freed;
    uint32_t peak_usage;
    uint32_t current_usage;
} rtos_memory_stats_t;

/* 堆统计信息 - 新增 */
typedef struct {
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t used_blocks;
    uint32_t fragmentation;
    uint32_t largest_free_block;
} rtos_heap_stats_t;

/* 定时器统计信息 - 新增 */
typedef struct {
    uint32_t total_timers;
    uint32_t active_timers;
    uint32_t total_triggers;
    rtos_time_ns_t average_period;
} rtos_timer_stats_t;

/* 等待标志定义 */
typedef enum {
    RTOS_WAIT_FLAG_NONE = 0x00,
    RTOS_WAIT_FLAG_ALL = 0x01,
    RTOS_WAIT_FLAG_CLEAR_ON_EXIT = 0x02
} rtos_wait_flag_t;

/* 等待队列节点 - 新增统一等待队列设计 */
typedef struct rtos_wait_node {
    struct rtos_task      *task;               /* 等待的任务 */
    rtos_timeout_t        timeout;             /* 超时时间 */
    void                  *data;               /* 等待数据 */
    rtos_wait_flag_t      flags;               /* 等待标志 */
    struct rtos_wait_node *next;               /* 下一个节点 */
    struct rtos_wait_node *prev;               /* 上一个节点 */
} rtos_wait_node_t;

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_object_class_type_t rtos_object_type_t
#define RTOS_OBJECT_CLASS_NULL RTOS_OBJECT_TYPE_NULL
#define RTOS_OBJECT_CLASS_THREAD RTOS_OBJECT_TYPE_TASK
#define RTOS_OBJECT_CLASS_SEMAPHORE RTOS_OBJECT_TYPE_SEMAPHORE
#define RTOS_OBJECT_CLASS_MUTEX RTOS_OBJECT_TYPE_MUTEX
#define RTOS_OBJECT_CLASS_MESSAGEQUEUE RTOS_OBJECT_TYPE_QUEUE
#define RTOS_OBJECT_CLASS_MEMPOOL RTOS_OBJECT_TYPE_MEMORY_POOL
#define RTOS_OBJECT_CLASS_TIMER RTOS_OBJECT_TYPE_SW_TIMER

/* 内存分配函数定义 */
#define rtos_malloc(size) malloc(size)
#define rtos_free(ptr) free(ptr)

#endif /* __RTOS_TYPES_H__ */
