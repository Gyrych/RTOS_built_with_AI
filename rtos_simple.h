/**
 * @file rtos_simple.h
 * @brief 简化的RTOS头文件 - 统一所有类型定义
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_SIMPLE_H__
#define __RTOS_SIMPLE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

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

/* 任务状态定义 */
typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SUSPENDED,
    TASK_STATE_DELETED
} rtos_task_state_t;

/* 任务控制块 */
typedef struct rtos_task {
    char name[16];                      /* 任务名称 */
    uint32_t *stack_ptr;                /* 栈指针 */
    uint32_t *stack_start;              /* 栈起始地址 */
    uint32_t stack_size;                /* 栈大小 */
    uint8_t priority;                   /* 任务优先级 */
    rtos_task_state_t state;            /* 任务状态 */
    uint32_t task_switch_count;         /* 任务切换计数 */
    void (*entry)(void *);              /* 任务入口函数 */
    void *param;                        /* 任务参数 */
    struct rtos_task *next;             /* 链表指针 */
} rtos_task_t;

/* 信号量控制块 */
typedef struct rtos_semaphore {
    uint32_t count;                     /* 信号量计数 */
    uint32_t max_count;                 /* 最大计数 */
    rtos_task_t *wait_list;             /* 等待队列 */
    bool is_valid;                      /* 有效性标志 */
} rtos_semaphore_t;

/* 互斥量控制块 */
typedef struct rtos_mutex {
    rtos_task_t *owner;                 /* 拥有者任务 */
    uint8_t lock_count;                 /* 锁定计数(递归锁) */
    rtos_task_t *wait_list;             /* 等待队列 */
    bool is_valid;                      /* 有效性标志 */
} rtos_mutex_t;

/* 消息队列控制块 */
typedef struct rtos_messagequeue {
    uint8_t *buffer;                    /* 消息缓冲区 */
    uint32_t msg_size;                  /* 消息大小 */
    uint32_t max_msgs;                  /* 最大消息数 */
    uint32_t entry;                     /* 入队索引 */
    uint32_t exit;                      /* 出队索引 */
    uint32_t current_msgs;              /* 当前消息数 */
    rtos_task_t *send_wait_list;        /* 发送等待队列 */
    rtos_task_t *recv_wait_list;        /* 接收等待队列 */
    bool is_valid;                      /* 有效性标志 */
} rtos_messagequeue_t;

/* 系统初始化和控制 */
rtos_result_t rtos_system_init(void);
rtos_result_t rtos_system_start(void);
uint32_t rtos_system_get_time_ms(void);
rtos_result_t rtos_system_delay_ms(uint32_t ms);
rtos_result_t rtos_system_delay_us(uint32_t us);

/* 任务管理API */
rtos_result_t rtos_task_init(rtos_task_t *task, const char *name,
                            void (*entry)(void *), void *param,
                            uint32_t *stack, uint32_t stack_size,
                            uint8_t priority, uint32_t tick);
rtos_result_t rtos_task_startup(rtos_task_t *task);
rtos_result_t rtos_task_yield(void);
rtos_result_t rtos_task_mdelay(uint32_t ms);

/* 信号量API */
rtos_result_t rtos_sem_init(rtos_semaphore_t *sem, const char *name,
                           uint32_t value, uint8_t flag);
rtos_result_t rtos_sem_take(rtos_semaphore_t *sem, rtos_time_ns_t time);
rtos_result_t rtos_sem_release(rtos_semaphore_t *sem);

/* 互斥量API */
rtos_result_t rtos_mutex_init(rtos_mutex_t *mutex, const char *name, uint8_t flag);
rtos_result_t rtos_mutex_take(rtos_mutex_t *mutex, rtos_time_ns_t time);
rtos_result_t rtos_mutex_release(rtos_mutex_t *mutex);

/* 消息队列API */
rtos_result_t rtos_mq_init(rtos_messagequeue_t *mq, const char *name,
                          void *msgpool, uint32_t msg_size,
                          uint32_t pool_size, uint8_t flag);
rtos_result_t rtos_mq_send(rtos_messagequeue_t *mq, const void *buffer,
                          uint32_t size, rtos_time_ns_t timeout);
int rtos_mq_recv(rtos_messagequeue_t *mq, void *buffer,
                uint32_t size, rtos_time_ns_t timeout);

/* 临界区管理 */
void rtos_enter_critical(void);
void rtos_exit_critical(void);

/* 内存管理 */
void *rtos_malloc(uint32_t size);
void rtos_free(void *ptr);

/* 硬件抽象层 */
void rtos_hw_init(void);
uint32_t *rtos_hw_stack_init(void (*entry)(void *), void *param,
                            uint32_t *stack_top, void (*exit)(void));
void rtos_context_switch(uint32_t **from_sp, uint32_t **to_sp);

#endif /* __RTOS_SIMPLE_H__ */