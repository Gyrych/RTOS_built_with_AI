/**
 * @file rtos.h
 * @brief RTOS主头文件 - 重构后的统一接口
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_H__
#define __RTOS_H__

/* 包含核心类型定义 */
#include "rtos/core/types.h"

/* 包含核心对象系统 */
#include "rtos/core/object.h"

/* 包含任务管理模块 */
#include "rtos/task/task.h"

/* 包含同步机制模块 */
#include "rtos/sync/semaphore.h"
#include "rtos/sync/mutex.h"
#include "rtos/sync/queue.h"

/* 包含事件组模块(框架) */
#include "rtos/sync/event.h"

/* 包含定时器模块(框架) */
#include "rtos/time/timer.h"

/* 包含Tickless时间管理模块 */
#include "rtos/time/tickless.h"

/* 包含动态延时管理模块 */
#include "rtos/time/dynamic_delay.h"

/* 包含内存池模块(框架) */
#include "rtos/memory/mempool.h"

/* 包含硬件抽象层 */
#include "rtos/hw/hw_abstraction.h"
#include "rtos/hw/interrupt_handler.h"

/* 包含硬件定时器测试模块 */
#include "rtos/hw/hw_timer_test.h"

/* 系统核心函数声明 */

/**
 * @brief 初始化RTOS系统
 * @return 操作结果
 */
rtos_result_t rtos_system_init(void);

/**
 * @brief 启动RTOS系统
 * @return 操作结果
 */
rtos_result_t rtos_system_start(void);

/**
 * @brief 停止RTOS系统
 * @return 操作结果
 */
rtos_result_t rtos_system_stop(void);

/**
 * @brief 关闭RTOS系统
 * @return 操作结果
 */
rtos_result_t rtos_system_shutdown(void);

/**
 * @brief 获取系统状态
 * @return 系统状态
 */
rtos_system_state_t rtos_system_get_state(void);

/**
 * @brief 检查系统是否运行
 * @return 是否运行
 */
bool rtos_system_is_running(void);

/**
 * @brief 获取系统统计信息
 * @param stats 统计信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_system_get_stats(rtos_system_stats_t *stats);

/**
 * @brief 获取内存统计信息
 * @param stats 内存统计信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_system_get_memory_stats(rtos_memory_stats_t *stats);

/**
 * @brief 重置系统统计信息
 * @return 操作结果
 */
rtos_result_t rtos_system_reset_stats(void);

/**
 * @brief 设置系统启动钩子函数
 * @param hook 钩子函数
 */
void rtos_system_set_startup_hook(void (*hook)(void));

/**
 * @brief 设置系统关闭钩子函数
 * @param hook 钩子函数
 */
void rtos_system_set_shutdown_hook(void (*hook)(void));

/**
 * @brief 设置系统空闲钩子函数
 * @param hook 钩子函数
 */
void rtos_system_set_idle_hook(void (*hook)(void));

/**
 * @brief 删除系统启动钩子函数
 * @param hook 钩子函数
 */
void rtos_system_delete_startup_hook(void (*hook)(void));

/**
 * @brief 删除系统关闭钩子函数
 * @param hook 钩子函数
 */
void rtos_system_delete_shutdown_hook(void (*hook)(void));

/**
 * @brief 删除系统空闲钩子函数
 * @param hook 钩子函数
 */
void rtos_system_delete_idle_hook(void (*hook)(void));

/**
 * @brief 获取系统版本信息
 * @return 版本字符串
 */
const char *rtos_system_get_version(void);

/**
 * @brief 获取系统版本号
 * @param major 主版本号指针
 * @param minor 次版本号指针
 * @param patch 修订版本号指针
 */
void rtos_system_get_version_numbers(uint8_t *major, uint8_t *minor, uint8_t *patch);

/**
 * @brief 系统延时(毫秒)
 * @param ms 毫秒数
 * @return 操作结果
 */
rtos_result_t rtos_system_delay_ms(uint32_t ms);

/**
 * @brief 系统延时(微秒)
 * @param us 微秒数
 * @return 操作结果
 */
rtos_result_t rtos_system_delay_us(uint32_t us);

/**
 * @brief 系统延时(纳秒)
 * @param ns 纳秒数
 * @return 操作结果
 */
rtos_result_t rtos_system_delay_ns(rtos_time_ns_t ns);

/**
 * @brief 进入临界区
 * @return 临界区状态
 */
rtos_critical_t rtos_system_enter_critical(void);

/**
 * @brief 退出临界区
 * @param critical 临界区状态
 */
void rtos_system_exit_critical(rtos_critical_t critical);

/**
 * @brief 系统空闲处理
 */
void rtos_system_idle(void);

/* 对象清理回调函数声明 */

/**
 * @brief 清理任务对象回调
 * @param object 对象指针
 * @param arg 参数
 */
void rtos_system_cleanup_task_callback(rtos_object_t *object, void *arg);

/**
 * @brief 清理信号量对象回调
 * @param object 对象指针
 * @param arg 参数
 */
void rtos_system_cleanup_semaphore_callback(rtos_object_t *object, void *arg);

/**
 * @brief 清理互斥量对象回调
 * @param object 对象指针
 * @param arg 参数
 */
void rtos_system_cleanup_mutex_callback(rtos_object_t *object, void *arg);

/**
 * @brief 清理队列对象回调
 * @param object 对象指针
 * @param arg 参数
 */
void rtos_system_cleanup_queue_callback(rtos_object_t *object, void *arg);

/* 兼容性宏定义 - 保持与现有代码的兼容性 */

/* 任务管理兼容性 */
#define rtos_task_init rtos_task_create_static
#define rtos_task_startup rtos_task_start
#define rtos_task_mdelay rtos_task_delay_ms
#define rtos_task_udelay rtos_task_delay_us
#define rtos_task_delay rtos_task_delay_ns

/* 信号量兼容性 */
#define rtos_sem_init rtos_semaphore_init
#define rtos_sem_take rtos_semaphore_take
#define rtos_sem_release rtos_semaphore_give

/* 互斥量兼容性 */
#define rtos_mutex_lock rtos_mutex_take
#define rtos_mutex_unlock rtos_mutex_release

/* 消息队列兼容性 */
#define rtos_messagequeue_t rtos_queue_t
#define rtos_mq_init rtos_queue_init
#define rtos_mq_send rtos_queue_send
#define rtos_mq_recv rtos_queue_receive

/* 系统兼容性 */
#define rtos_delay_ms rtos_system_delay_ms
#define rtos_delay_us rtos_system_delay_us
#define rtos_delay_ns rtos_system_delay_ns

/* 对象类型兼容性 */
#define rtos_object_class_type_t rtos_object_type_t
#define RTOS_OBJECT_CLASS_NULL RTOS_OBJECT_TYPE_NULL
#define RTOS_OBJECT_CLASS_THREAD RTOS_OBJECT_TYPE_TASK
#define RTOS_OBJECT_CLASS_SEMAPHORE RTOS_OBJECT_TYPE_SEMAPHORE
#define RTOS_OBJECT_CLASS_MUTEX RTOS_OBJECT_TYPE_MUTEX
#define RTOS_OBJECT_CLASS_MESSAGEQUEUE RTOS_OBJECT_TYPE_QUEUE
#define RTOS_OBJECT_CLASS_MEMPOOL RTOS_OBJECT_TYPE_MEMORY_POOL
#define RTOS_OBJECT_CLASS_TIMER RTOS_OBJECT_TYPE_SW_TIMER

/* 对象信息兼容性 */
#define rtos_object_information rtos_object_information_t

#endif /* __RTOS_H__ */