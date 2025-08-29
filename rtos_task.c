/**
 * @file rtos_task.c
 * @brief RTOS任务模块实现 - 基于内核对象
 */

#include "rtos_task.h"
#include "rtos_object.h"
#include "rtos_kernel.h"
#include "rtos_hw.h"
#include <string.h>

/* 调度器状态 */
static bool scheduler_running = false;
static uint32_t scheduler_lock_nest = 0;

/* 当前任务和最高优先级任务 */
static rtos_task_t *rtos_current_task = NULL;
static rtos_task_t *rtos_to_task = NULL;

/* 就绪队列位图和队列 */
static uint32_t rtos_task_ready_priority_group = 0;
static uint8_t rtos_task_ready_table[32];
static rtos_task_t *rtos_task_ready_queue[RTOS_TASK_PRIORITY_MAX];

/* 空闲任务相关 */
static rtos_task_t rtos_task_idle;
static uint32_t rtos_idle_task_stack[512];
static void (*rtos_task_idle_hook)(void) = NULL;

/* 任务钩子函数 */
static rtos_task_switch_hook_t rtos_task_switch_hook = NULL;
static rtos_task_inout_hook_t rtos_task_enter_hook = NULL;
static rtos_task_inout_hook_t rtos_task_exit_hook = NULL;

/* 统计信息 */
static uint32_t rtos_task_switch_interrupt_flag = 0;

/* 内部函数声明 */
static void rtos_schedule_insert_task_to_queue(rtos_task_t *task);
static void rtos_schedule_remove_task_from_queue(rtos_task_t *task);
static rtos_task_t *rtos_schedule_get_highest_ready_task(void);
static void rtos_task_exit(void);
static uint8_t rtos_lowest_bitmap[256];

/**
 * @brief 初始化任务系统
 */
void rtos_task_system_init(void)
{
    int i;
    
    /* 初始化就绪队列 */
    rtos_task_ready_priority_group = 0;
    memset(rtos_task_ready_table, 0, sizeof(rtos_task_ready_table));
    memset(rtos_task_ready_queue, 0, sizeof(rtos_task_ready_queue));
    
    /* 初始化位图查找表 */
    rtos_lowest_bitmap[0] = 0;
    for (i = 1; i < 256; i++) {
        rtos_lowest_bitmap[i] = (uint8_t)(__builtin_ctz(i));
    }
    
    /* 创建空闲任务 */
    rtos_task_init(&rtos_task_idle,
                   "idle",
                   rtos_task_idle_entry,
                   NULL,
                   rtos_idle_task_stack,
                   sizeof(rtos_idle_task_stack),
                   RTOS_IDLE_THREAD_PRIORITY,
                   RTOS_TASK_TIMESLICE_DEFAULT);
}

/**
 * @brief 初始化任务 - 静态方式
 */
rtos_result_t rtos_task_init(rtos_task_t *task,
                            const char  *name,
                            void        (*entry)(void *parameter),
                            void        *parameter,
                            void        *stack_start,
                            uint32_t     stack_size,
                            uint8_t      priority,
                            uint32_t     tick)
{
    if (!task || !entry || !stack_start || priority >= RTOS_TASK_PRIORITY_MAX) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化对象基类 */
    rtos_object_init(&(task->parent), RTOS_OBJECT_CLASS_THREAD, name);
    
    rtos_enter_critical();
    
    /* 初始化任务基本信息 */
    task->entry = entry;
    task->parameter = parameter;
    task->stack_addr = (uint32_t *)stack_start;
    task->stack_size = stack_size;
    task->stack_top = (uint32_t *)((uint8_t *)stack_start + stack_size - sizeof(uint32_t));
    
    /* 初始化栈指针 */
    task->sp = rtos_hw_stack_init(entry, parameter, task->stack_top, rtos_task_exit);
    
    /* 初始化优先级 */
    task->current_priority = priority;
    task->init_priority = priority;
    task->number = priority;
    task->number_mask = 1L << (priority & 0x1f);
    task->high_mask = 1L << (priority >> 3);
    
    /* 初始化状态 */
    task->stat = RTOS_TASK_INIT;
    task->task_flags = RTOS_TASK_FLAG_NONE;
    task->tlist = NULL;
    
    /* 初始化时间片 */
    task->init_tick = tick;
    task->remaining_tick = tick;
    
    /* 初始化其他字段 */
    task->error = RTOS_OK;
    task->event_set = 0;
    task->event_info = 0;
    task->switch_count = 0;
    task->total_runtime = 0;
    task->last_start_time = 0;
    task->max_used_stack = 0;
    task->user_data = 0;
    task->cleanup = NULL;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 创建任务 - 动态方式
 */
rtos_task_t *rtos_task_create(const char *name,
                             void       (*entry)(void *parameter),
                             void       *parameter,
                             uint32_t    stack_size,
                             uint8_t     priority,
                             uint32_t    tick)
{
    rtos_task_t *task;
    void *stack_start;
    
    if (!entry || priority >= RTOS_TASK_PRIORITY_MAX) {
        return NULL;
    }
    
    /* 分配任务控制块 */
    task = (rtos_task_t *)rtos_object_allocate(RTOS_OBJECT_CLASS_THREAD, name);
    if (!task) {
        return NULL;
    }
    
    /* 分配栈空间 */
    stack_start = rtos_malloc(stack_size);
    if (!stack_start) {
        rtos_object_delete(&(task->parent));
        return NULL;
    }
    
    /* 初始化任务 */
    if (rtos_task_init(task, name, entry, parameter, stack_start, stack_size, priority, tick) != RTOS_OK) {
        rtos_free(stack_start);
        rtos_object_delete(&(task->parent));
        return NULL;
    }
    
    /* 标记为动态对象 */
    task->parent.flag |= RTOS_OBJECT_FLAG_DYNAMIC;
    
    return task;
}

/**
 * @brief 分离任务
 */
rtos_result_t rtos_task_detach(rtos_task_t *task)
{
    rtos_result_t result;
    
    if (!task || !rtos_object_is_static(&(task->parent))) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 如果任务在就绪队列中，移除 */
    if ((task->stat & RTOS_TASK_STAT_MASK) == RTOS_TASK_READY) {
        rtos_schedule_remove_task_from_queue(task);
    }
    
    /* 设置任务状态为关闭 */
    task->stat = RTOS_TASK_CLOSE;
    
    /* 从对象系统分离 */
    result = rtos_object_detach(&(task->parent));
    
    rtos_exit_critical();
    
    /* 如果当前任务退出，需要调度 */
    if (task == rtos_current_task) {
        rtos_schedule();
    }
    
    return result;
}

/**
 * @brief 删除任务
 */
rtos_result_t rtos_task_delete(rtos_task_t *task)
{
    void *stack_start;
    
    if (!task || !rtos_object_is_dynamic(&(task->parent))) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 如果任务在就绪队列中，移除 */
    if ((task->stat & RTOS_TASK_STAT_MASK) == RTOS_TASK_READY) {
        rtos_schedule_remove_task_from_queue(task);
    }
    
    /* 保存栈地址用于释放 */
    stack_start = task->stack_addr;
    
    /* 设置任务状态为关闭 */
    task->stat = RTOS_TASK_CLOSE;
    
    rtos_exit_critical();
    
    /* 释放栈空间 */
    if (stack_start) {
        rtos_free(stack_start);
    }
    
    /* 从对象系统删除 */
    rtos_object_delete(&(task->parent));
    
    /* 如果当前任务退出，需要调度 */
    if (task == rtos_current_task) {
        rtos_schedule();
    }
    
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
    
    rtos_enter_critical();
    
    /* 设置任务为就绪状态 */
    task->stat = RTOS_TASK_READY;
    
    /* 插入到就绪队列 */
    rtos_schedule_insert_task_to_queue(task);
    
    rtos_exit_critical();
    
    /* 如果调度器已运行，尝试调度 */
    if (scheduler_running) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 获取当前任务
 */
rtos_task_t *rtos_task_self(void)
{
    return rtos_current_task;
}

/**
 * @brief 查找任务
 */
rtos_task_t *rtos_task_find(const char *name)
{
    rtos_object_t *object;
    
    object = rtos_object_find(RTOS_OBJECT_CLASS_THREAD, name);
    if (object) {
        return rtos_container_of(object, rtos_task_t, parent);
    }
    
    return NULL;
}

/**
 * @brief 让出CPU
 */
rtos_result_t rtos_task_yield(void)
{
    if (!scheduler_running) {
        return RTOS_ERROR;
    }
    
    rtos_enter_critical();
    
    /* 如果同优先级有其他就绪任务，进行调度 */
    if (rtos_current_task && 
        rtos_task_ready_queue[rtos_current_task->current_priority] != rtos_current_task) {
        rtos_schedule();
    }
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 睡眠指定时间(毫秒)
 */
rtos_result_t rtos_task_mdelay(uint32_t ms)
{
    return rtos_delay_ms(ms);
}

/**
 * @brief 睡眠指定时间(微秒)
 */
rtos_result_t rtos_task_udelay(uint32_t us)
{
    return rtos_delay_us(us);
}

/**
 * @brief 睡眠指定tick数
 */
rtos_result_t rtos_task_delay(rtos_time_ns_t tick)
{
    return rtos_delay_ns(tick);
}

/**
 * @brief 挂起任务
 */
rtos_result_t rtos_task_suspend(rtos_task_t *task)
{
    if (!task) {
        task = rtos_current_task;
    }
    
    if (!task) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 检查任务当前状态 */
    if ((task->stat & RTOS_TASK_STAT_MASK) != RTOS_TASK_READY &&
        (task->stat & RTOS_TASK_STAT_MASK) != RTOS_TASK_RUNNING) {
        rtos_exit_critical();
        return RTOS_ERROR;
    }
    
    /* 从就绪队列移除 */
    if ((task->stat & RTOS_TASK_STAT_MASK) == RTOS_TASK_READY) {
        rtos_schedule_remove_task_from_queue(task);
    }
    
    /* 设置挂起状态 */
    task->stat = RTOS_TASK_SUSPEND;
    
    rtos_exit_critical();
    
    /* 如果挂起当前任务，需要调度 */
    if (task == rtos_current_task) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 恢复任务
 */
rtos_result_t rtos_task_resume(rtos_task_t *task)
{
    if (!task) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 检查任务是否处于挂起状态 */
    if ((task->stat & RTOS_TASK_STAT_MASK) != RTOS_TASK_SUSPEND) {
        rtos_exit_critical();
        return RTOS_ERROR;
    }
    
    /* 设置为就绪状态 */
    task->stat = RTOS_TASK_READY;
    
    /* 插入到就绪队列 */
    rtos_schedule_insert_task_to_queue(task);
    
    rtos_exit_critical();
    
    /* 如果调度器已运行，尝试调度 */
    if (scheduler_running) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 设置任务优先级
 */
uint8_t rtos_task_set_priority(rtos_task_t *task, uint8_t priority)
{
    uint8_t old_priority;
    
    if (!task || priority >= RTOS_TASK_PRIORITY_MAX) {
        return RTOS_TASK_PRIORITY_MAX;
    }
    
    rtos_enter_critical();
    
    old_priority = task->current_priority;
    
    /* 如果优先级没有变化，直接返回 */
    if (old_priority == priority) {
        rtos_exit_critical();
        return old_priority;
    }
    
    /* 如果任务在就绪队列中，先移除 */
    if ((task->stat & RTOS_TASK_STAT_MASK) == RTOS_TASK_READY) {
        rtos_schedule_remove_task_from_queue(task);
    }
    
    /* 更新优先级相关字段 */
    task->current_priority = priority;
    task->number = priority;
    task->number_mask = 1L << (priority & 0x1f);
    task->high_mask = 1L << (priority >> 3);
    
    /* 如果任务是就绪状态，重新插入就绪队列 */
    if ((task->stat & RTOS_TASK_STAT_MASK) == RTOS_TASK_READY) {
        rtos_schedule_insert_task_to_queue(task);
    }
    
    rtos_exit_critical();
    
    /* 如果调度器已运行，尝试调度 */
    if (scheduler_running) {
        rtos_schedule();
    }
    
    return old_priority;
}

/**
 * @brief 获取任务优先级
 */
uint8_t rtos_task_get_priority(rtos_task_t *task)
{
    if (!task) {
        return RTOS_TASK_PRIORITY_MAX;
    }
    
    return task->current_priority;
}

/**
 * @brief 初始化调度器
 */
void rtos_scheduler_init(void)
{
    scheduler_running = false;
    scheduler_lock_nest = 0;
    rtos_current_task = NULL;
    rtos_to_task = NULL;
}

/**
 * @brief 启动调度器
 */
void rtos_scheduler_start(void)
{
    rtos_task_t *to_task;
    
    /* 启动空闲任务 */
    rtos_task_startup(&rtos_task_idle);
    
    /* 查找最高优先级任务 */
    to_task = rtos_schedule_get_highest_ready_task();
    if (!to_task) {
        return; /* 没有就绪任务 */
    }
    
    rtos_current_task = to_task;
    rtos_to_task = to_task;
    to_task->stat = RTOS_TASK_RUNNING;
    
    /* 标记调度器为运行状态 */
    scheduler_running = true;
    
    /* 启动第一个任务 */
    rtos_hw_context_switch_to((uint32_t)&to_task->sp);
}

/**
 * @brief 调度器是否已启动
 */
bool rtos_scheduler_is_running(void)
{
    return scheduler_running;
}

/**
 * @brief 执行调度
 */
void rtos_schedule(void)
{
    rtos_task_t *to_task;
    rtos_task_t *from_task;
    
    /* 检查调度器状态 */
    if (!scheduler_running || scheduler_lock_nest > 0) {
        rtos_task_switch_interrupt_flag = 1;
        return;
    }
    
    rtos_task_switch_interrupt_flag = 0;
    
    rtos_enter_critical();
    
    /* 获取最高优先级就绪任务 */
    to_task = rtos_schedule_get_highest_ready_task();
    if (!to_task) {
        rtos_exit_critical();
        return;
    }
    
    if (to_task != rtos_current_task) {
        /* 需要进行任务切换 */
        rtos_to_task = to_task;
        from_task = rtos_current_task;
        
        /* 更新统计信息 */
        if (from_task) {
            from_task->switch_count++;
            from_task->total_runtime += rtos_get_time_ns() - from_task->last_start_time;
        }
        
        if (to_task) {
            to_task->stat = RTOS_TASK_RUNNING;
            to_task->last_start_time = rtos_get_time_ns();
        }
        
        if (from_task && (from_task->stat & RTOS_TASK_STAT_MASK) == RTOS_TASK_RUNNING) {
            from_task->stat = RTOS_TASK_READY;
        }
        
        rtos_current_task = to_task;
        
        /* 调用钩子函数 */
        if (rtos_task_switch_hook) {
            rtos_task_switch_hook(from_task, to_task);
        }
        
        rtos_exit_critical();
        
        /* 执行上下文切换 */
        if (from_task) {
            rtos_hw_context_switch((uint32_t)&from_task->sp, (uint32_t)&to_task->sp);
        } else {
            rtos_hw_context_switch_to((uint32_t)&to_task->sp);
        }
    } else {
        rtos_exit_critical();
    }
}

/**
 * @brief 空闲任务入口函数
 */
void rtos_task_idle_entry(void *parameter)
{
    (void)parameter;
    
    while (1) {
        /* 调用空闲钩子函数 */
        if (rtos_task_idle_hook) {
            rtos_task_idle_hook();
        }
        
        /* 功耗管理 */
        rtos_hw_cpu_idle();
    }
}

/**
 * @brief 设置空闲任务钩子函数
 */
void rtos_task_set_idle_hook(void (*hook)(void))
{
    rtos_task_idle_hook = hook;
}

/**
 * @brief 任务退出函数
 */
static void rtos_task_exit(void)
{
    rtos_task_t *task = rtos_current_task;
    
    if (task) {
        /* 调用清理函数 */
        if (task->cleanup) {
            task->cleanup(task);
        }
        
        /* 调用退出钩子 */
        if (rtos_task_exit_hook) {
            rtos_task_exit_hook(task);
        }
        
        /* 删除任务 */
        if (rtos_object_is_dynamic(&(task->parent))) {
            rtos_task_delete(task);
        } else {
            rtos_task_detach(task);
        }
    }
}

/* ========== 调度器内部函数 ========== */

/**
 * @brief 插入任务到就绪队列
 */
static void rtos_schedule_insert_task_to_queue(rtos_task_t *task)
{
    uint8_t y, x;
    uint32_t temp;
    
    y = task->number >> 3;
    x = task->number & 0x07;
    
    /* 插入到优先级队列 */
    temp = 1 << x;
    rtos_task_ready_table[y] |= temp;
    rtos_task_ready_priority_group |= (1 << y);
    
    /* 插入到链表 */
    task->tlist = rtos_task_ready_queue[task->current_priority];
    rtos_task_ready_queue[task->current_priority] = task;
}

/**
 * @brief 从就绪队列移除任务
 */
static void rtos_schedule_remove_task_from_queue(rtos_task_t *task)
{
    uint8_t y, x;
    rtos_task_t **task_ptr;
    
    /* 从链表中移除 */
    task_ptr = &rtos_task_ready_queue[task->current_priority];
    while (*task_ptr) {
        if (*task_ptr == task) {
            *task_ptr = task->tlist;
            task->tlist = NULL;
            break;
        }
        task_ptr = &((*task_ptr)->tlist);
    }
    
    /* 如果该优先级没有任务了，清除位图 */
    if (!rtos_task_ready_queue[task->current_priority]) {
        y = task->number >> 3;
        x = task->number & 0x07;
        
        rtos_task_ready_table[y] &= ~(1 << x);
        if (rtos_task_ready_table[y] == 0) {
            rtos_task_ready_priority_group &= ~(1 << y);
        }
    }
}

/**
 * @brief 获取最高优先级就绪任务
 */
static rtos_task_t *rtos_schedule_get_highest_ready_task(void)
{
    uint8_t y, x;
    uint8_t highest_ready_priority;
    
    /* 查找最高优先级 */
    if (rtos_task_ready_priority_group == 0) {
        return NULL;
    }
    
    y = rtos_lowest_bitmap[rtos_task_ready_priority_group];
    x = rtos_lowest_bitmap[rtos_task_ready_table[y]];
    highest_ready_priority = (y << 3) + x;
    
    return rtos_task_ready_queue[highest_ready_priority];
}

/**
 * @brief 设置任务切换钩子
 */
void rtos_task_set_switch_hook(rtos_task_switch_hook_t hook)
{
    rtos_task_switch_hook = hook;
}

/**
 * @brief 获取任务信息
 */
rtos_result_t rtos_task_get_info(rtos_task_t *task, rtos_task_info_t *info)
{
    if (!task || !info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    info->state = task->stat;
    info->priority = task->current_priority;
    info->max_priority = task->init_priority;
    info->stack_size = task->stack_size;
    /* 简化的栈使用量计算 */
    info->free_stack = task->stack_size - task->max_used_stack;
    strncpy(info->name, task->parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    rtos_exit_critical();
    
    return RTOS_OK;
}