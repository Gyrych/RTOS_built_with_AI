/**
 * @file rtos.h
 * @brief RTOS主头文件 - 模块化内核对象系统
 * @author Assistant
 * @date 2024
 * 
 * 基于RT-Thread内核对象概念设计的模块化RTOS系统
 * 特点：
 * 1. 统一的内核对象基类和继承体系
 * 2. 模块化设计，高内聚低耦合
 * 3. 支持静态和动态对象管理
 * 4. 完善的对象容器和链表管理
 * 5. 支持对象查找、遍历和信息获取
 */

#ifndef __RTOS_H__
#define __RTOS_H__

/* 系统配置 */
#include "rtos_config.h"

/* 基础类型和错误码 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 错误代码定义 */
typedef enum {
    RTOS_OK = 0,
    RTOS_ERROR,
    RTOS_ERROR_TIMEOUT,
    RTOS_ERROR_NO_MEMORY,
    RTOS_ERROR_INVALID_PARAM,
    RTOS_ERROR_RESOURCE_BUSY,
    RTOS_ERROR_DEADLOCK,
    RTOS_ERROR_STACK_OVERFLOW,
    RTOS_ERROR_MEMORY_CORRUPTION
} rtos_result_t;

/* 时间类型定义 */
typedef uint64_t rtos_time_ns_t;

/* 内存管理函数声明 */
void *rtos_malloc(uint32_t size);
void rtos_free(void *ptr);

/* 临界区管理 */
void rtos_enter_critical(void);
void rtos_exit_critical(void);

/* 内核对象系统 */
#include "rtos_object.h"

/* 任务管理 */
#include "rtos_task.h"

/* IPC机制 */
#include "rtos_semaphore.h"
#include "rtos_mutex.h"
#include "rtos_queue.h"
#include "rtos_event.h"

/* 定时器 */
#include "rtos_timer.h"

/* 内存池 */
#include "rtos_mempool.h"

/* 硬件抽象层 */
#include "rtos_hw.h"

/* 系统初始化和控制 */

/**
 * @brief 初始化RTOS系统
 * @return 结果码
 */
rtos_result_t rtos_system_init(void);

/**
 * @brief 启动RTOS调度器
 * @return 结果码
 */
rtos_result_t rtos_system_start(void);

/**
 * @brief 获取系统运行时间(纳秒)
 * @return 系统运行时间
 */
rtos_time_ns_t rtos_system_get_time_ns(void);

/**
 * @brief 获取系统运行时间(微秒)
 * @return 系统运行时间
 */
uint32_t rtos_system_get_time_us(void);

/**
 * @brief 获取系统运行时间(毫秒)
 * @return 系统运行时间
 */
uint32_t rtos_system_get_time_ms(void);

/**
 * @brief 系统延时(纳秒)
 * @param ns 延时时间(纳秒)
 * @return 结果码
 */
rtos_result_t rtos_system_delay_ns(rtos_time_ns_t ns);

/**
 * @brief 系统延时(微秒)
 * @param us 延时时间(微秒)
 * @return 结果码
 */
rtos_result_t rtos_system_delay_us(uint32_t us);

/**
 * @brief 系统延时(毫秒)
 * @param ms 延时时间(毫秒)
 * @return 结果码
 */
rtos_result_t rtos_system_delay_ms(uint32_t ms);

/* 系统状态查询 */

/**
 * @brief 检查调度器是否运行
 * @return 是否运行
 */
bool rtos_system_scheduler_is_running(void);

/**
 * @brief 获取系统对象统计信息
 * @param type 对象类型
 * @return 对象信息指针
 */
rtos_object_information_t *rtos_system_get_object_info(rtos_object_class_type_t type);

/**
 * @brief 打印系统状态信息
 */
void rtos_system_print_status(void);

/**
 * @brief 打印所有对象信息
 */
void rtos_system_print_objects(void);

/* 系统钩子函数 */
typedef void (*rtos_idle_hook_t)(void);
typedef void (*rtos_scheduler_hook_t)(rtos_task_t *from, rtos_task_t *to);

/**
 * @brief 设置空闲钩子函数
 * @param hook 钩子函数
 */
void rtos_system_set_idle_hook(rtos_idle_hook_t hook);

/**
 * @brief 设置调度器钩子函数
 * @param hook 钩子函数
 */
void rtos_system_set_scheduler_hook(rtos_scheduler_hook_t hook);

/* 版本信息 */
#define RTOS_VERSION_MAJOR      2
#define RTOS_VERSION_MINOR      0
#define RTOS_VERSION_PATCH      0

#define RTOS_VERSION_STRING     "RTOS v2.0.0 - Object-Oriented Kernel"

/**
 * @brief 获取版本信息
 * @return 版本字符串
 */
const char *rtos_system_get_version(void);

/* 调试和诊断 */
#ifdef RTOS_DEBUG
#define RTOS_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            rtos_system_assert_failed(__FILE__, __LINE__, #expr); \
        } \
    } while (0)

/**
 * @brief 断言失败处理
 * @param file 文件名
 * @param line 行号
 * @param expr 表达式
 */
void rtos_system_assert_failed(const char *file, int line, const char *expr);
#else
#define RTOS_ASSERT(expr)
#endif

/* 内存对齐宏 */
#define RTOS_ALIGN(size, align)     (((size) + (align) - 1) & ~((align) - 1))
#define RTOS_ALIGN_SIZE             4

/* 容器操作宏 */
#define rtos_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define rtos_offsetof(type, member) \
    ((unsigned long)(&((type *)0)->member))

/* 链表操作宏 */
#define rtos_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define rtos_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#endif /* __RTOS_H__ */