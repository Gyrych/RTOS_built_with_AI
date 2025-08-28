/**
 * @file rtos_kernel.c
 * @brief RTOS内核实现
 */

#include "rtos_kernel.h"
#include "rtos_hw.h"
#include <string.h>

/* 系统全局变量 */
static rtos_system_t rtos_system;
static rtos_task_t rtos_tasks[RTOS_MAX_TASKS];
static rtos_semaphore_t rtos_semaphores[RTOS_MAX_SEMAPHORES];
static rtos_mutex_t rtos_mutexes[RTOS_MAX_MUTEXES];
static rtos_queue_t rtos_queues[RTOS_MAX_QUEUES];
static rtos_timer_t rtos_timers[RTOS_MAX_TASKS]; /* 每个任务一个定时器 */
static rtos_sw_timer_t rtos_sw_timers[RTOS_MAX_SW_TIMERS]; /* 软件定时器 */
static rtos_event_group_t rtos_event_groups[RTOS_MAX_EVENT_GROUPS]; /* 事件组 */
static rtos_memory_pool_t rtos_memory_pools[RTOS_MAX_MEMORY_POOLS]; /* 内存池 */

/* 跟踪相关变量 */
static rtos_trace_record_t *trace_buffer = NULL;
static uint32_t trace_buffer_size = 0;
static volatile uint32_t trace_index = 0;

/* 内部函数声明 */
static void rtos_add_task_to_ready_list(rtos_task_t *task);
static void rtos_remove_task_from_ready_list(rtos_task_t *task);
static rtos_task_t *rtos_get_highest_priority_task(void);
static void rtos_timer_add(rtos_timer_t *timer);
static void rtos_timer_remove(rtos_timer_t *timer);
static void rtos_timer_update(void);
static void rtos_block_task(rtos_task_t *task, rtos_task_t **wait_list);
static void rtos_unblock_task(rtos_task_t *task, rtos_task_t **wait_list);
static rtos_task_t *rtos_get_next_task_from_wait_list(rtos_task_t **wait_list);

/**
 * @brief 系统初始化
 */
rtos_result_t rtos_init(void)
{
    /* 清零系统控制块 */
    memset(&rtos_system, 0, sizeof(rtos_system_t));
    memset(rtos_tasks, 0, sizeof(rtos_tasks));
    memset(rtos_semaphores, 0, sizeof(rtos_semaphores));
    memset(rtos_mutexes, 0, sizeof(rtos_mutexes));
    memset(rtos_queues, 0, sizeof(rtos_queues));
    memset(rtos_timers, 0, sizeof(rtos_timers));
    memset(rtos_sw_timers, 0, sizeof(rtos_sw_timers));
    memset(rtos_event_groups, 0, sizeof(rtos_event_groups));
    memset(rtos_memory_pools, 0, sizeof(rtos_memory_pools));
    
    /* 初始化系统控制块 */
    rtos_system.sleep_mode = RTOS_SLEEP_NONE;
    rtos_system.trace_enabled = false;
    
    /* 初始化硬件 */
    rtos_hw_init();
    rtos_hw_timer_init();
    rtos_hw_mpu_init();
    
    return RTOS_OK;
}

/**
 * @brief 启动调度器
 */
rtos_result_t rtos_start(void)
{
    if (rtos_system.ready_bitmap == 0) {
        return RTOS_ERROR; /* 没有就绪任务 */
    }
    
    rtos_system.scheduler_running = true;
    
    /* 获取最高优先级任务 */
    rtos_system.current_task = rtos_get_highest_priority_task();
    rtos_system.current_task->state = TASK_STATE_RUNNING;
    
    /* 启动第一个任务 */
    rtos_hw_start_first_task();
    
    return RTOS_OK;
}

/**
 * @brief 创建任务
 */
rtos_result_t rtos_task_create(rtos_task_t *task, 
                              const char *name,
                              void (*task_func)(void *),
                              void *param,
                              uint8_t priority,
                              uint32_t *stack,
                              uint32_t stack_size)
{
    if (!task || !task_func || !stack || priority >= RTOS_PRIORITY_LEVELS) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (stack_size < RTOS_STACK_SIZE_MIN) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 初始化任务控制块 */
    task->stack_base = stack;
    task->stack_size = stack_size;
    task->priority = priority;
    task->original_priority = priority;
    task->state = TASK_STATE_READY;
    task->delay_time_ns = 0;
    task->next = NULL;
    task->task_func = task_func;
    task->param = param;
    
    /* 初始化调试和安全信息 */
    task->task_switch_count = 0;
    task->stack_high_water = 0;
    task->runtime_us = 0;
    task->stack_check_enabled = false;
    task->stack_canary = 0xDEADBEEF;
    task->mpu_region_count = 0;
    memset(task->mpu_regions, 0, sizeof(task->mpu_regions));
    
    /* 复制任务名称 */
    if (name) {
        strncpy(task->name, name, sizeof(task->name) - 1);
        task->name[sizeof(task->name) - 1] = '\0';
    } else {
        strcpy(task->name, "Unknown");
    }
    
    /* 初始化任务栈 */
    task->stack_ptr = rtos_task_stack_init(stack + stack_size/4 - 1, task_func, param);
    
    /* 添加到就绪队列 */
    rtos_add_task_to_ready_list(task);
    
    rtos_exit_critical();
    
    /* 如果调度器已运行且新任务优先级更高，触发调度 */
    if (rtos_system.scheduler_running && 
        priority < rtos_system.current_task->priority) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 删除任务
 */
rtos_result_t rtos_task_delete(rtos_task_t *task)
{
    if (!task) {
        task = rtos_system.current_task;
    }
    
    rtos_enter_critical();
    
    /* 从就绪队列移除 */
    rtos_remove_task_from_ready_list(task);
    
    /* 标记为删除状态 */
    task->state = TASK_STATE_DELETED;
    
    /* 如果删除的是当前任务，触发调度 */
    if (task == rtos_system.current_task) {
        rtos_exit_critical();
        rtos_schedule();
    } else {
        rtos_exit_critical();
    }
    
    return RTOS_OK;
}

/**
 * @brief 任务主动让出CPU
 */
void rtos_task_yield(void)
{
    rtos_schedule();
}

/**
 * @brief 获取当前任务
 */
rtos_task_t *rtos_get_current_task(void)
{
    return rtos_system.current_task;
}

/**
 * @brief 微秒级延时
 */
rtos_result_t rtos_delay_us(uint32_t microseconds)
{
    if (microseconds == 0) {
        return RTOS_OK;
    }
    
    if (!rtos_system.scheduler_running) {
        return RTOS_ERROR;
    }
    
    rtos_enter_critical();
    
    rtos_task_t *current_task = rtos_system.current_task;
    
    /* 设置延时时间 */
    current_task->delay_time_ns = (rtos_time_ns_t)microseconds * 1000;
    current_task->state = TASK_STATE_BLOCKED;
    
    /* 从就绪队列移除 */
    rtos_remove_task_from_ready_list(current_task);
    
    /* 创建定时器 */
    for (int i = 0; i < RTOS_MAX_TASKS; i++) {
        if (!rtos_timers[i].is_active) {
            rtos_timers[i].expire_time_ns = rtos_hw_get_time_ns() + ((rtos_time_ns_t)microseconds * 1000);
            rtos_timers[i].task = current_task;
            rtos_timers[i].is_active = true;
            rtos_timer_add(&rtos_timers[i]);
            break;
        }
    }
    
    /* 设置硬件定时器 */
    if (rtos_system.timer_list) {
        rtos_time_ns_t current_time = rtos_hw_get_time_ns();
        rtos_time_ns_t next_expire = rtos_system.timer_list->expire_time_ns;
        if (next_expire > current_time) {
            rtos_hw_timer_set_ns(next_expire - current_time);
        } else {
            rtos_hw_timer_set_ns(1000); /* 立即触发(1微秒) */
        }
    }
    
    rtos_exit_critical();
    
    /* 触发任务调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 毫秒级延时
 */
rtos_result_t rtos_delay_ms(uint32_t milliseconds)
{
    return rtos_delay_us(milliseconds * 1000);
}

/**
 * @brief 纳秒级延时
 */
rtos_result_t rtos_delay_ns(rtos_time_ns_t nanoseconds)
{
    if (nanoseconds == 0) {
        return RTOS_OK;
    }
    
    if (!rtos_system.scheduler_running) {
        return RTOS_ERROR;
    }
    
    rtos_enter_critical();
    
    rtos_task_t *current_task = rtos_system.current_task;
    
    /* 设置延时时间 */
    current_task->delay_time_ns = nanoseconds;
    current_task->state = TASK_STATE_BLOCKED;
    
    /* 从就绪队列移除 */
    rtos_remove_task_from_ready_list(current_task);
    
    /* 创建定时器 */
    for (int i = 0; i < RTOS_MAX_TASKS; i++) {
        if (!rtos_timers[i].is_active) {
            rtos_timers[i].expire_time_ns = rtos_hw_get_time_ns() + nanoseconds;
            rtos_timers[i].task = current_task;
            rtos_timers[i].is_active = true;
            rtos_timer_add(&rtos_timers[i]);
            break;
        }
    }
    
    /* 设置硬件定时器 */
    if (rtos_system.timer_list) {
        rtos_time_ns_t current_time = rtos_hw_get_time_ns();
        rtos_time_ns_t next_expire = rtos_system.timer_list->expire_time_ns;
        if (next_expire > current_time) {
            rtos_hw_timer_set_ns(next_expire - current_time);
        } else {
            rtos_hw_timer_set_ns(1000); /* 立即触发(1微秒) */
        }
    }
    
    rtos_exit_critical();
    
    /* 触发任务调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 灵活延时(支持不同时间单位)
 */
rtos_result_t rtos_delay_ticks(uint64_t ticks, rtos_time_unit_t unit)
{
    rtos_time_ns_t nanoseconds = ticks * unit;
    return rtos_delay_ns(nanoseconds);
}

/**
 * @brief 延时到绝对时间
 */
rtos_result_t rtos_delay_until(rtos_time_ns_t absolute_time_ns)
{
    rtos_time_ns_t current_time = rtos_hw_get_time_ns();
    if (absolute_time_ns <= current_time) {
        return RTOS_OK; /* 时间已过 */
    }
    
    return rtos_delay_ns(absolute_time_ns - current_time);
}

/**
 * @brief 进入临界区
 */
void rtos_enter_critical(void)
{
    rtos_hw_disable_interrupts();
    rtos_system.critical_nesting++;
    rtos_system.in_critical = true;
}

/**
 * @brief 退出临界区
 */
void rtos_exit_critical(void)
{
    rtos_system.critical_nesting--;
    if (rtos_system.critical_nesting == 0) {
        rtos_system.in_critical = false;
        rtos_hw_enable_interrupts();
    }
}

/**
 * @brief 获取系统时间(微秒)
 */
uint32_t rtos_get_time_us(void)
{
    return rtos_hw_get_time_us();
}

/**
 * @brief 获取系统时间(毫秒)
 */
uint32_t rtos_get_time_ms(void)
{
    return rtos_hw_get_time_us() / 1000;
}

/**
 * @brief 获取系统时间(纳秒)
 */
rtos_time_ns_t rtos_get_time_ns(void)
{
    return rtos_hw_get_time_ns();
}

/**
 * @brief 任务调度
 */
void rtos_schedule(void)
{
    if (!rtos_system.scheduler_running || rtos_system.in_critical) {
        return;
    }
    
    rtos_enter_critical();
    
    rtos_task_t *next_task = rtos_get_highest_priority_task();
    
    if (next_task && next_task != rtos_system.current_task) {
        /* 当前任务状态切换 */
        if (rtos_system.current_task && 
            rtos_system.current_task->state == TASK_STATE_RUNNING) {
            rtos_system.current_task->state = TASK_STATE_READY;
        }
        
        /* 新任务状态切换 */
        next_task->state = TASK_STATE_RUNNING;
        rtos_system.current_task = next_task;
        
        rtos_exit_critical();
        
        /* 触发上下文切换 */
        rtos_context_switch();
    } else {
        rtos_exit_critical();
    }
}

/**
 * @brief 定时器中断服务程序
 */
void rtos_timer_isr(void)
{
    rtos_timer_update();
    
    /* 设置下一个定时器 */
    if (rtos_system.timer_list) {
        rtos_time_ns_t current_time = rtos_hw_get_time_ns();
        rtos_time_ns_t next_expire = rtos_system.timer_list->expire_time_ns;
        if (next_expire > current_time) {
            rtos_hw_timer_set_ns(next_expire - current_time);
        } else {
            rtos_hw_timer_set_ns(1000); /* 1微秒 */
        }
    } else {
        rtos_hw_timer_stop();
    }
    
    /* 触发调度 */
    rtos_schedule();
}

/* ========== 内部函数实现 ========== */

/**
 * @brief 将任务添加到就绪队列
 */
static void rtos_add_task_to_ready_list(rtos_task_t *task)
{
    uint8_t priority = task->priority;
    
    /* 添加到对应优先级队列末尾 */
    if (rtos_system.ready_list[priority] == NULL) {
        rtos_system.ready_list[priority] = task;
        task->next = NULL;
    } else {
        rtos_task_t *current = rtos_system.ready_list[priority];
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = task;
        task->next = NULL;
    }
    
    /* 设置就绪位图 */
    rtos_system.ready_bitmap |= (1U << priority);
}

/**
 * @brief 从就绪队列移除任务
 */
static void rtos_remove_task_from_ready_list(rtos_task_t *task)
{
    uint8_t priority = task->priority;
    rtos_task_t **current = &rtos_system.ready_list[priority];
    
    while (*current != NULL) {
        if (*current == task) {
            *current = task->next;
            task->next = NULL;
            break;
        }
        current = &((*current)->next);
    }
    
    /* 如果队列为空，清除就绪位图 */
    if (rtos_system.ready_list[priority] == NULL) {
        rtos_system.ready_bitmap &= ~(1U << priority);
    }
}

/**
 * @brief 获取最高优先级任务
 */
static rtos_task_t *rtos_get_highest_priority_task(void)
{
    if (rtos_system.ready_bitmap == 0) {
        return NULL;
    }
    
    /* 查找最高优先级(最低数值) */
    uint8_t highest_priority = __builtin_ctz(rtos_system.ready_bitmap);
    
    return rtos_system.ready_list[highest_priority];
}

/**
 * @brief 添加定时器到链表
 */
static void rtos_timer_add(rtos_timer_t *timer)
{
    if (rtos_system.timer_list == NULL) {
        rtos_system.timer_list = timer;
        timer->next = NULL;
    } else {
        /* 按到期时间排序插入 */
        if (timer->expire_time_ns < rtos_system.timer_list->expire_time_ns) {
            timer->next = rtos_system.timer_list;
            rtos_system.timer_list = timer;
        } else {
            rtos_timer_t *current = rtos_system.timer_list;
            while (current->next != NULL && 
                   current->next->expire_time_ns <= timer->expire_time_ns) {
                current = current->next;
            }
            timer->next = current->next;
            current->next = timer;
        }
    }
}

/**
 * @brief 从定时器链表移除
 */
static void rtos_timer_remove(rtos_timer_t *timer)
{
    rtos_timer_t **current = &rtos_system.timer_list;
    
    while (*current != NULL) {
        if (*current == timer) {
            *current = timer->next;
            timer->next = NULL;
            timer->is_active = false;
            break;
        }
        current = &((*current)->next);
    }
}

/**
 * @brief 更新定时器
 */
static void rtos_timer_update(void)
{
    rtos_time_ns_t current_time = rtos_hw_get_time_ns();
    rtos_timer_t *timer = rtos_system.timer_list;
    
    while (timer != NULL && timer->expire_time_ns <= current_time) {
        rtos_timer_t *expired_timer = timer;
        timer = timer->next;
        
        /* 移除过期定时器 */
        rtos_timer_remove(expired_timer);
        
        /* 恢复关联任务 */
        if (expired_timer->task) {
            expired_timer->task->state = TASK_STATE_READY;
            expired_timer->task->delay_time_ns = 0;
            rtos_add_task_to_ready_list(expired_timer->task);
        }
    }
}