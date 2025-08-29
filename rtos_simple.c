/**
 * @file rtos_simple.c
 * @brief 简化的RTOS实现
 * @author Assistant
 * @date 2024
 */

#include "rtos_simple.h"
#include <string.h>

/* 系统全局变量 */
static bool system_initialized = false;
static bool scheduler_running = false;
static rtos_task_t *current_task = NULL;
static rtos_task_t *ready_tasks[8] = {NULL}; /* 简化的优先级队列 */
static uint32_t system_tick_ms = 0;

/* 内存管理 */
static uint8_t heap_memory[8192];
static uint32_t heap_used = 0;

/* 临界区保护计数 */
static uint32_t critical_count = 0;

/**
 * @brief 系统初始化
 */
rtos_result_t rtos_system_init(void)
{
    if (system_initialized) {
        return RTOS_OK;
    }
    
    /* 清零全局变量 */
    memset(ready_tasks, 0, sizeof(ready_tasks));
    current_task = NULL;
    system_tick_ms = 0;
    critical_count = 0;
    heap_used = 0;
    
    /* 硬件初始化 */
    rtos_hw_init();
    
    system_initialized = true;
    printf("RTOS系统初始化完成\n");
    
    return RTOS_OK;
}

/**
 * @brief 启动调度器
 */
rtos_result_t rtos_system_start(void)
{
    if (!system_initialized) {
        return RTOS_ERROR;
    }
    
    /* 找到第一个就绪任务 */
    for (int i = 0; i < 8; i++) {
        if (ready_tasks[i] != NULL) {
            current_task = ready_tasks[i];
            current_task->state = TASK_STATE_RUNNING;
            scheduler_running = true;
            printf("RTOS调度器启动，运行任务: %s\n", current_task->name);
            break;
        }
    }
    
    if (!current_task) {
        printf("没有就绪任务，无法启动调度器\n");
        return RTOS_ERROR;
    }
    
    /* 在实际系统中，这里会跳转到第一个任务 */
    printf("RTOS调度器已启动(简化实现)\n");
    return RTOS_OK;
}

/**
 * @brief 获取系统时间(毫秒)
 */
uint32_t rtos_system_get_time_ms(void)
{
    return system_tick_ms++;  /* 简化实现 */
}

/**
 * @brief 系统延时(毫秒)
 */
rtos_result_t rtos_system_delay_ms(uint32_t ms)
{
    /* 简化实现 - 在实际系统中会阻塞任务 */
    (void)ms;
    return RTOS_OK;
}

/**
 * @brief 系统延时(微秒)
 */
rtos_result_t rtos_system_delay_us(uint32_t us)
{
    /* 简化实现 */
    (void)us;
    return RTOS_OK;
}

/**
 * @brief 任务初始化
 */
rtos_result_t rtos_task_init(rtos_task_t *task, const char *name,
                            void (*entry)(void *), void *param,
                            uint32_t *stack, uint32_t stack_size,
                            uint8_t priority, uint32_t tick)
{
    if (!task || !entry || !stack || priority >= 8) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化任务控制块 */
    strncpy(task->name, name ? name : "Unknown", sizeof(task->name) - 1);
    task->name[sizeof(task->name) - 1] = '\0';
    
    task->stack_start = stack;
    task->stack_size = stack_size;
    task->priority = priority;
    task->state = TASK_STATE_READY;
    task->entry = entry;
    task->param = param;
    task->next = NULL;
    task->task_switch_count = 0;
    
    /* 初始化栈 */
    task->stack_ptr = rtos_hw_stack_init(entry, param, 
                                        (uint32_t*)((uint8_t*)stack + stack_size - 4), 
                                        NULL);
    
    (void)tick; /* 未使用 */
    
    printf("任务 %s 初始化完成，优先级 %d\n", task->name, task->priority);
    return RTOS_OK;
}

/**
 * @brief 启动任务
 */
rtos_result_t rtos_task_startup(rtos_task_t *task)
{
    if (!task) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 将任务加入就绪队列 */
    if (task->priority < 8) {
        task->next = ready_tasks[task->priority];
        ready_tasks[task->priority] = task;
        task->state = TASK_STATE_READY;
        
        printf("任务 %s 已加入就绪队列\n", task->name);
    }
    
    return RTOS_OK;
}

/**
 * @brief 任务让出CPU
 */
rtos_result_t rtos_task_yield(void)
{
    /* 简化实现 */
    if (current_task) {
        printf("任务 %s 让出CPU\n", current_task->name);
    }
    return RTOS_OK;
}

/**
 * @brief 任务延时(毫秒)
 */
rtos_result_t rtos_task_mdelay(uint32_t ms)
{
    /* 简化实现 */
    if (current_task) {
        printf("任务 %s 延时 %lu ms\n", current_task->name, (unsigned long)ms);
    }
    return RTOS_OK;
}

/**
 * @brief 信号量初始化
 */
rtos_result_t rtos_sem_init(rtos_semaphore_t *sem, const char *name,
                           uint32_t value, uint8_t flag)
{
    if (!sem) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    sem->count = value;
    sem->max_count = 65535;
    sem->wait_list = NULL;
    sem->is_valid = true;
    
    (void)name;
    (void)flag;
    
    printf("信号量初始化完成，初始值: %lu\n", (unsigned long)value);
    return RTOS_OK;
}

/**
 * @brief 获取信号量
 */
rtos_result_t rtos_sem_take(rtos_semaphore_t *sem, rtos_time_ns_t time)
{
    if (!sem || !sem->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    if (sem->count > 0) {
        sem->count--;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    rtos_exit_critical();
    
    /* 简化实现 - 不阻塞 */
    (void)time;
    return RTOS_ERROR_TIMEOUT;
}

/**
 * @brief 释放信号量
 */
rtos_result_t rtos_sem_release(rtos_semaphore_t *sem)
{
    if (!sem || !sem->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    if (sem->count < sem->max_count) {
        sem->count++;
    }
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 互斥量初始化
 */
rtos_result_t rtos_mutex_init(rtos_mutex_t *mutex, const char *name, uint8_t flag)
{
    if (!mutex) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    mutex->owner = NULL;
    mutex->lock_count = 0;
    mutex->wait_list = NULL;
    mutex->is_valid = true;
    
    (void)name;
    (void)flag;
    
    return RTOS_OK;
}

/**
 * @brief 获取互斥量
 */
rtos_result_t rtos_mutex_take(rtos_mutex_t *mutex, rtos_time_ns_t time)
{
    if (!mutex || !mutex->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    if (mutex->owner == NULL) {
        mutex->owner = current_task;
        mutex->lock_count = 1;
        rtos_exit_critical();
        return RTOS_OK;
    } else if (mutex->owner == current_task) {
        mutex->lock_count++;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    rtos_exit_critical();
    
    /* 简化实现 - 不阻塞 */
    (void)time;
    return RTOS_ERROR_TIMEOUT;
}

/**
 * @brief 释放互斥量
 */
rtos_result_t rtos_mutex_release(rtos_mutex_t *mutex)
{
    if (!mutex || !mutex->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    if (mutex->owner == current_task && mutex->lock_count > 0) {
        mutex->lock_count--;
        if (mutex->lock_count == 0) {
            mutex->owner = NULL;
        }
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    rtos_exit_critical();
    
    return RTOS_ERROR;
}

/**
 * @brief 消息队列初始化
 */
rtos_result_t rtos_mq_init(rtos_messagequeue_t *mq, const char *name,
                          void *msgpool, uint32_t msg_size,
                          uint32_t pool_size, uint8_t flag)
{
    if (!mq || !msgpool || msg_size == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    mq->buffer = (uint8_t*)msgpool;
    mq->msg_size = msg_size;
    mq->max_msgs = pool_size / msg_size;
    mq->entry = 0;
    mq->exit = 0;
    mq->current_msgs = 0;
    mq->send_wait_list = NULL;
    mq->recv_wait_list = NULL;
    mq->is_valid = true;
    
    (void)name;
    (void)flag;
    
    return RTOS_OK;
}

/**
 * @brief 发送消息
 */
rtos_result_t rtos_mq_send(rtos_messagequeue_t *mq, const void *buffer,
                          uint32_t size, rtos_time_ns_t timeout)
{
    if (!mq || !mq->is_valid || !buffer || size != mq->msg_size) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    if (mq->current_msgs < mq->max_msgs) {
        memcpy(&mq->buffer[mq->entry * mq->msg_size], buffer, size);
        mq->entry = (mq->entry + 1) % mq->max_msgs;
        mq->current_msgs++;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    rtos_exit_critical();
    
    (void)timeout;
    return RTOS_ERROR_TIMEOUT;
}

/**
 * @brief 接收消息
 */
int rtos_mq_recv(rtos_messagequeue_t *mq, void *buffer,
                uint32_t size, rtos_time_ns_t timeout)
{
    if (!mq || !mq->is_valid || !buffer || size < mq->msg_size) {
        return -1;
    }
    
    rtos_enter_critical();
    
    if (mq->current_msgs > 0) {
        memcpy(buffer, &mq->buffer[mq->exit * mq->msg_size], mq->msg_size);
        mq->exit = (mq->exit + 1) % mq->max_msgs;
        mq->current_msgs--;
        rtos_exit_critical();
        return (int)mq->msg_size;
    }
    
    rtos_exit_critical();
    
    (void)timeout;
    return -1;
}

/**
 * @brief 进入临界区
 */
void rtos_enter_critical(void)
{
    /* 简化实现 - 禁用中断 */
    critical_count++;
}

/**
 * @brief 退出临界区
 */
void rtos_exit_critical(void)
{
    if (critical_count > 0) {
        critical_count--;
    }
}

/**
 * @brief 内存分配
 */
void *rtos_malloc(uint32_t size)
{
    if (heap_used + size > sizeof(heap_memory)) {
        return NULL;
    }
    
    void *ptr = &heap_memory[heap_used];
    heap_used += (size + 3) & ~3; /* 4字节对齐 */
    
    return ptr;
}

/**
 * @brief 内存释放
 */
void rtos_free(void *ptr)
{
    /* 简化实现 - 不实际释放 */
    (void)ptr;
}

/**
 * @brief 硬件初始化
 */
void rtos_hw_init(void)
{
    /* 简化实现 */
    printf("硬件抽象层初始化完成\n");
}

/**
 * @brief 栈初始化
 */
uint32_t *rtos_hw_stack_init(void (*entry)(void *), void *param,
                            uint32_t *stack_top, void (*exit)(void))
{
    /* 简化实现 - 返回栈顶减少一些空间 */
    (void)entry;
    (void)param;
    (void)exit;
    
    return stack_top - 16; /* 为寄存器上下文预留空间 */
}

/**
 * @brief 上下文切换
 */
void rtos_context_switch(uint32_t **from_sp, uint32_t **to_sp)
{
    /* 简化实现 - 仅用于编译 */
    (void)from_sp;
    (void)to_sp;
}