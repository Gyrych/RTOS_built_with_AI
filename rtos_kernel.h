/**
 * @file rtos_kernel.h
 * @brief 基于优先级抢占调度的RTOS内核头文件
 * @author Assistant
 * @date 2024
 * 
 * 特性：
 * 1. 完全基于优先级抢占调度
 * 2. 无系统滴答时钟，基于动态定时器
 * 3. 精准微秒级定时功能
 * 4. 针对STM32F407优化
 */

#ifndef __RTOS_KERNEL_H__
#define __RTOS_KERNEL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 系统配置参数 */
#define RTOS_MAX_TASKS          16      /* 最大任务数量 */
#define RTOS_MAX_SEMAPHORES     8       /* 最大信号量数量 */
#define RTOS_MAX_MUTEXES        8       /* 最大互斥量数量 */
#define RTOS_MAX_QUEUES         8       /* 最大消息队列数量 */
#define RTOS_STACK_SIZE_MIN     256     /* 最小任务栈大小(字节) */
#define RTOS_PRIORITY_LEVELS    32      /* 优先级级别数 (0-31, 0最高) */
#define RTOS_QUEUE_ITEM_SIZE    32      /* 队列项目大小(字节) */

/* 任务状态定义 */
typedef enum {
    TASK_STATE_READY = 0,       /* 就绪状态 */
    TASK_STATE_RUNNING,         /* 运行状态 */
    TASK_STATE_BLOCKED,         /* 阻塞状态 */
    TASK_STATE_SUSPENDED,       /* 挂起状态 */
    TASK_STATE_DELETED          /* 删除状态 */
} rtos_task_state_t;

/* 错误代码定义 */
typedef enum {
    RTOS_OK = 0,
    RTOS_ERROR,
    RTOS_ERROR_TIMEOUT,
    RTOS_ERROR_NO_MEMORY,
    RTOS_ERROR_INVALID_PARAM,
    RTOS_ERROR_RESOURCE_BUSY
} rtos_result_t;

/* 任务控制块 */
typedef struct rtos_task {
    uint32_t *stack_ptr;           /* 栈指针 */
    uint32_t *stack_base;          /* 栈基址 */
    uint32_t stack_size;           /* 栈大小 */
    uint8_t priority;              /* 任务优先级 */
    rtos_task_state_t state;       /* 任务状态 */
    uint32_t delay_ticks;          /* 延时时间(微秒) */
    struct rtos_task *next;        /* 链表指针 */
    char name[16];                 /* 任务名称 */
    void (*task_func)(void *);     /* 任务函数 */
    void *param;                   /* 任务参数 */
} rtos_task_t;

/* 信号量控制块 */
typedef struct {
    uint32_t count;                /* 信号量计数值 */
    uint32_t max_count;            /* 最大计数值 */
    rtos_task_t *wait_list;        /* 等待任务链表 */
    bool is_valid;                 /* 是否有效 */
} rtos_semaphore_t;

/* 互斥量控制块 */
typedef struct {
    rtos_task_t *owner;            /* 持有者任务 */
    uint32_t nest_count;           /* 嵌套计数 */
    uint8_t original_priority;     /* 原始优先级(优先级继承) */
    rtos_task_t *wait_list;        /* 等待任务链表 */
    bool is_valid;                 /* 是否有效 */
} rtos_mutex_t;

/* 消息队列控制块 */
typedef struct {
    uint8_t *buffer;               /* 缓冲区 */
    uint32_t item_size;            /* 单个消息大小 */
    uint32_t max_items;            /* 最大消息数 */
    uint32_t head;                 /* 队列头 */
    uint32_t tail;                 /* 队列尾 */
    uint32_t count;                /* 当前消息数 */
    rtos_task_t *send_wait_list;   /* 发送等待链表 */
    rtos_task_t *recv_wait_list;   /* 接收等待链表 */
    bool is_valid;                 /* 是否有效 */
} rtos_queue_t;

/* 定时器控制块 */
typedef struct rtos_timer {
    uint32_t expire_time;          /* 到期时间(微秒) */
    rtos_task_t *task;             /* 关联任务 */
    struct rtos_timer *next;       /* 链表指针 */
    bool is_active;                /* 是否激活 */
} rtos_timer_t;

/* 系统控制块 */
typedef struct {
    rtos_task_t *current_task;     /* 当前运行任务 */
    rtos_task_t *ready_list[RTOS_PRIORITY_LEVELS];  /* 就绪队列 */
    uint32_t ready_bitmap;         /* 就绪位图 */
    rtos_timer_t *timer_list;      /* 定时器链表 */
    uint32_t system_time;          /* 系统时间(微秒) */
    bool scheduler_running;        /* 调度器是否运行 */
    bool in_critical;              /* 是否在临界区 */
    uint32_t critical_nesting;     /* 临界区嵌套计数 */
} rtos_system_t;

/* 系统API声明 */

/* 系统初始化和启动 */
rtos_result_t rtos_init(void);
rtos_result_t rtos_start(void);

/* 任务管理 */
rtos_result_t rtos_task_create(rtos_task_t *task, 
                              const char *name,
                              void (*task_func)(void *),
                              void *param,
                              uint8_t priority,
                              uint32_t *stack,
                              uint32_t stack_size);
rtos_result_t rtos_task_delete(rtos_task_t *task);
rtos_result_t rtos_task_suspend(rtos_task_t *task);
rtos_result_t rtos_task_resume(rtos_task_t *task);
void rtos_task_yield(void);
rtos_task_t *rtos_get_current_task(void);

/* 延时功能 */
rtos_result_t rtos_delay_us(uint32_t microseconds);
rtos_result_t rtos_delay_ms(uint32_t milliseconds);

/* 信号量 */
rtos_result_t rtos_semaphore_create(rtos_semaphore_t *sem, uint32_t initial_count, uint32_t max_count);
rtos_result_t rtos_semaphore_take(rtos_semaphore_t *sem, uint32_t timeout_us);
rtos_result_t rtos_semaphore_give(rtos_semaphore_t *sem);
rtos_result_t rtos_semaphore_delete(rtos_semaphore_t *sem);

/* 互斥量 */
rtos_result_t rtos_mutex_create(rtos_mutex_t *mutex);
rtos_result_t rtos_mutex_lock(rtos_mutex_t *mutex, uint32_t timeout_us);
rtos_result_t rtos_mutex_unlock(rtos_mutex_t *mutex);
rtos_result_t rtos_mutex_delete(rtos_mutex_t *mutex);

/* 消息队列 */
rtos_result_t rtos_queue_create(rtos_queue_t *queue, void *buffer, uint32_t item_size, uint32_t max_items);
rtos_result_t rtos_queue_send(rtos_queue_t *queue, const void *item, uint32_t timeout_us);
rtos_result_t rtos_queue_receive(rtos_queue_t *queue, void *item, uint32_t timeout_us);
rtos_result_t rtos_queue_delete(rtos_queue_t *queue);

/* 临界区管理 */
void rtos_enter_critical(void);
void rtos_exit_critical(void);

/* 系统时间 */
uint32_t rtos_get_time_us(void);
uint32_t rtos_get_time_ms(void);

/* 调度器控制 */
void rtos_scheduler_suspend(void);
void rtos_scheduler_resume(void);

/* 内部函数声明(供汇编和硬件层调用) */
void rtos_schedule(void);
void rtos_context_switch(void);
uint32_t *rtos_task_stack_init(uint32_t *stack_top, void (*task_func)(void *), void *param);

/* STM32F407硬件相关 */
void rtos_hw_init(void);
void rtos_hw_timer_init(void);
void rtos_hw_timer_set(uint32_t microseconds);
void rtos_hw_timer_stop(void);
uint32_t rtos_hw_get_time_us(void);
void rtos_hw_disable_interrupts(void);
void rtos_hw_enable_interrupts(void);

/* 中断服务程序 */
void rtos_systick_handler(void);
void rtos_pendsv_handler(void);
void rtos_timer_isr(void);

#endif /* __RTOS_KERNEL_H__ */