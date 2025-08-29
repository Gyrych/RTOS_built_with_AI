/**
 * @file task.c
 * @brief RTOS任务管理系统实现
 * @author Assistant
 * @date 2024
 */

#include "task.h"
#include <string.h>
#include <stddef.h>

/* 全局变量 */
static rtos_task_t *g_current_task = NULL;
static rtos_task_t *g_idle_task = NULL;
static bool g_scheduler_running = false;
static uint32_t g_scheduler_lock_level = 0;

/* 任务队列 */
static rtos_task_node_t g_ready_queue_head = {0};
static rtos_task_node_t g_blocked_queue_head = {0};

/* 任务统计 */
static uint32_t g_total_tasks = 0;
static uint32_t g_task_switch_count = 0;

/* 钩子函数 */
static rtos_task_switch_hook_t g_task_switch_hook = NULL;
static void (*g_idle_hook)(void) = NULL;

/* 栈魔数定义 */
#define RTOS_STACK_MAGIC 0xDEADBEEF

/* 内部函数声明 */
static void rtos_task_add_to_ready_queue(rtos_task_t *task);
static void rtos_task_remove_from_ready_queue(rtos_task_t *task);
static void rtos_task_add_to_blocked_queue(rtos_task_t *task);
static void rtos_task_remove_from_blocked_queue(rtos_task_t *task);
static rtos_task_t *rtos_task_get_highest_priority_ready(void);
static void rtos_task_switch_to(rtos_task_t *new_task);

/**
 * @brief 初始化任务系统
 */
void rtos_task_system_init(void)
{
    /* 初始化任务队列 */
    memset(&g_ready_queue_head, 0, sizeof(g_ready_queue_head));
    memset(&g_blocked_queue_head, 0, sizeof(g_blocked_queue_head));
    
    g_ready_queue_head.next = &g_ready_queue_head;
    g_ready_queue_head.prev = &g_ready_queue_head;
    g_blocked_queue_head.next = &g_blocked_queue_head;
    g_blocked_queue_head.prev = &g_blocked_queue_head;
    
    /* 初始化全局变量 */
    g_current_task = NULL;
    g_idle_task = NULL;
    g_scheduler_running = false;
    g_scheduler_lock_level = 0;
    g_total_tasks = 0;
    g_task_switch_count = 0;
    
    /* 清除钩子函数 */
    g_task_switch_hook = NULL;
    g_idle_hook = NULL;
}

/**
 * @brief 创建任务 - 静态方式
 */
rtos_result_t rtos_task_create_static(rtos_task_t *task,
                                      const rtos_task_create_params_t *params)
{
    if (task == NULL || params == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (params->entry == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (params->stack_size < RTOS_STACK_SIZE_MIN || 
        params->stack_size > RTOS_STACK_SIZE_MAX) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (params->priority >= RTOS_PRIORITY_LEVELS) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化对象基类 */
    rtos_object_init(&task->parent, RTOS_OBJECT_TYPE_TASK, 
                     params->name, RTOS_OBJECT_FLAG_STATIC);
    
    /* 设置任务基本信息 */
    task->entry = params->entry;
    task->parameter = params->parameter;
    task->stack_size = params->stack_size;
    task->priority = params->priority;
    task->base_priority = params->priority;
    task->timeslice = params->timeslice ? params->timeslice : RTOS_TIMESLICE_DEFAULT;
    task->remaining_timeslice = task->timeslice;
    
    /* 设置任务状态 */
    task->state = RTOS_TASK_STATE_INIT;
    task->flags = params->flags;
    task->suspend_count = 0;
    
    /* 初始化链表节点 */
    memset(&task->ready_node, 0, sizeof(task->ready_node));
    memset(&task->block_node, 0, sizeof(task->block_node));
    task->ready_node.task = task;
    task->block_node.task = task;
    
    /* 初始化同步相关 */
    task->wait_object = NULL;
    task->timeout = 0;
    
    /* 初始化统计信息 */
    task->total_runtime = 0;
    task->switch_count = 0;
    task->max_stack_usage = 0;
    task->current_stack_usage = 0;
    task->last_run_time = 0;
    
    /* 初始化栈溢出检测 */
    task->stack_magic_start = RTOS_STACK_MAGIC;
    task->stack_magic_end = RTOS_STACK_MAGIC;
    task->stack_overflow_flag = false;
    
    /* 清除清理函数 */
    task->cleanup = NULL;
    
    /* 添加到任务容器 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_TASK);
    if (container) {
        rtos_object_container_add(container, &task->parent);
    }
    
    g_total_tasks++;
    return RTOS_OK;
}

/**
 * @brief 创建任务 - 动态方式
 */
rtos_task_t *rtos_task_create_dynamic(const rtos_task_create_params_t *params)
{
    if (params == NULL) {
        return NULL;
    }
    
    /* 分配任务控制块 */
    rtos_task_t *task = (rtos_task_t *)malloc(sizeof(rtos_task_t));
    if (task == NULL) {
        return NULL;
    }
    
    /* 分配栈空间 */
    uint32_t *stack = (uint32_t *)malloc(params->stack_size);
    if (stack == NULL) {
        free(task);
        return NULL;
    }
    
    /* 设置栈地址 */
    task->stack_addr = stack;
    task->stack_top = stack + (params->stack_size / sizeof(uint32_t));
    
    /* 创建任务 */
    rtos_result_t result = rtos_task_create_static(task, params);
    if (result != RTOS_OK) {
        free(stack);
        free(task);
        return NULL;
    }
    
    /* 设置动态分配标志 */
    rtos_object_set_flags(&task->parent, RTOS_OBJECT_FLAG_DYNAMIC);
    
    return task;
}

/**
 * @brief 删除任务
 */
rtos_result_t rtos_task_delete(rtos_task_t *task)
{
    if (task == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (task == g_current_task) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 从队列中移除 */
    if (task->state == RTOS_TASK_STATE_READY) {
        rtos_task_remove_from_ready_queue(task);
    } else if (task->state == RTOS_TASK_STATE_BLOCKED) {
        rtos_task_remove_from_blocked_queue(task);
    }
    
    /* 从容器中移除 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_TASK);
    if (container) {
        rtos_object_container_remove(container, &task->parent);
    }
    
    /* 设置状态为已删除 */
    task->state = RTOS_TASK_STATE_DELETED;
    
    /* 如果是动态分配的任务，释放内存 */
    if (rtos_object_is_dynamic(&task->parent)) {
        if (task->stack_addr) {
            free(task->stack_addr);
        }
        free(task);
    }
    
    g_total_tasks--;
    return RTOS_OK;
}

/**
 * @brief 启动任务
 */
rtos_result_t rtos_task_start(rtos_task_t *task)
{
    if (task == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (task->state != RTOS_TASK_STATE_INIT) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 初始化任务栈 */
    task->sp = rtos_task_stack_init(task->entry, task->parameter, 
                                   task->stack_addr, task->stack_size);
    if (task->sp == NULL) {
        return RTOS_ERROR;
    }
    
    /* 设置状态为就绪 */
    task->state = RTOS_TASK_STATE_READY;
    
    /* 添加到就绪队列 */
    rtos_task_add_to_ready_queue(task);
    
    return RTOS_OK;
}

/**
 * @brief 挂起任务
 */
rtos_result_t rtos_task_suspend(rtos_task_t *task)
{
    if (task == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (task->state == RTOS_TASK_STATE_SUSPENDED) {
        return RTOS_OK;
    }
    
    /* 增加挂起计数 */
    task->suspend_count++;
    
    if (task->suspend_count == 1) {
        /* 第一次挂起 */
        if (task->state == RTOS_TASK_STATE_READY) {
            rtos_task_remove_from_ready_queue(task);
        } else if (task->state == RTOS_TASK_STATE_BLOCKED) {
            rtos_task_remove_from_blocked_queue(task);
        }
        
        task->state = RTOS_TASK_STATE_SUSPENDED;
    }
    
    return RTOS_OK;
}

/**
 * @brief 恢复任务
 */
rtos_result_t rtos_task_resume(rtos_task_t *task)
{
    if (task == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (task->state != RTOS_TASK_STATE_SUSPENDED) {
        return RTOS_OK;
    }
    
    /* 减少挂起计数 */
    if (task->suspend_count > 0) {
        task->suspend_count--;
    }
    
    if (task->suspend_count == 0) {
        /* 恢复任务 */
        task->state = RTOS_TASK_STATE_READY;
        rtos_task_add_to_ready_queue(task);
    }
    
    return RTOS_OK;
}

/**
 * @brief 让出CPU
 */
rtos_result_t rtos_task_yield(void)
{
    if (g_current_task == NULL || !g_scheduler_running) {
        return RTOS_ERROR;
    }
    
    /* 触发调度 */
    rtos_scheduler_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 延时指定时间(毫秒)
 */
rtos_result_t rtos_task_delay_ms(uint32_t ms)
{
    return rtos_task_delay_ns((rtos_time_ns_t)ms * 1000000ULL);
}

/**
 * @brief 延时指定时间(微秒)
 */
rtos_result_t rtos_task_delay_us(uint32_t us)
{
    return rtos_task_delay_ns((rtos_time_ns_t)us * 1000ULL);
}

/**
 * @brief 延时指定时间(纳秒)
 */
rtos_result_t rtos_task_delay_ns(rtos_time_ns_t ns)
{
    if (g_current_task == NULL) {
        return RTOS_ERROR;
    }
    
    if (ns == 0) {
        return RTOS_OK;
    }
    
    /* 设置超时时间 */
    g_current_task->timeout = ns;
    g_current_task->state = RTOS_TASK_STATE_BLOCKED;
    
    /* 从就绪队列移除，添加到阻塞队列 */
    rtos_task_remove_from_ready_queue(g_current_task);
    rtos_task_add_to_blocked_queue(g_current_task);
    
    /* 触发调度 */
    rtos_scheduler_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 延时到指定时间点
 */
rtos_result_t rtos_task_delay_until(rtos_time_ns_t absolute_time)
{
    if (g_current_task == NULL) {
        return RTOS_ERROR;
    }
    
    /* 计算相对延时时间 */
    rtos_time_ns_t current_time = 0; /* 需要从系统时钟获取 */
    rtos_time_ns_t delay_time = absolute_time - current_time;
    
    return rtos_task_delay_ns(delay_time);
}

/**
 * @brief 设置任务优先级
 */
rtos_priority_t rtos_task_set_priority(rtos_task_t *task, rtos_priority_t priority)
{
    if (task == NULL || priority >= RTOS_PRIORITY_LEVELS) {
        return RTOS_PRIORITY_NORMAL;
    }
    
    rtos_priority_t old_priority = task->priority;
    task->priority = priority;
    
    /* 如果任务在就绪队列中，需要重新排序 */
    if (task->state == RTOS_TASK_STATE_READY) {
        rtos_task_remove_from_ready_queue(task);
        rtos_task_add_to_ready_queue(task);
    }
    
    return old_priority;
}

/**
 * @brief 获取任务优先级
 */
rtos_priority_t rtos_task_get_priority(const rtos_task_t *task)
{
    if (task == NULL) {
        return RTOS_PRIORITY_NORMAL;
    }
    
    return task->priority;
}

/**
 * @brief 获取当前任务
 */
rtos_task_t *rtos_task_get_current(void)
{
    return g_current_task;
}

/**
 * @brief 查找任务
 */
rtos_task_t *rtos_task_find(const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_TASK);
    if (container == NULL) {
        return NULL;
    }
    
    rtos_object_t *obj = rtos_object_container_find(container, name);
    if (obj == NULL) {
        return NULL;
    }
    
    return (rtos_task_t *)obj;
}

/**
 * @brief 获取任务信息
 */
rtos_result_t rtos_task_get_info(const rtos_task_t *task, rtos_task_info_t *info)
{
    if (task == NULL || info == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    strncpy(info->name, task->parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    info->state = task->state;
    info->priority = task->priority;
    info->stack_size = task->stack_size;
    info->free_stack = rtos_task_get_stack_free(task);
    info->switch_count = task->switch_count;
    info->total_runtime = task->total_runtime;
    info->stack_overflow = task->stack_overflow_flag;
    
    return RTOS_OK;
}

/**
 * @brief 设置任务清理函数
 */
void rtos_task_set_cleanup(rtos_task_t *task, void (*cleanup)(rtos_task_t *))
{
    if (task == NULL) {
        return;
    }
    
    task->cleanup = cleanup;
}

/* 栈管理相关函数 */

/**
 * @brief 检查任务栈溢出
 */
bool rtos_task_check_stack_overflow(const rtos_task_t *task)
{
    if (task == NULL) {
        return false;
    }
    
    return task->stack_overflow_flag;
}

/**
 * @brief 获取任务栈使用量
 */
uint32_t rtos_task_get_stack_usage(const rtos_task_t *task)
{
    if (task == NULL || task->stack_addr == NULL) {
        return 0;
    }
    
    /* 计算栈使用量 */
    uint32_t *sp = task->sp;
    uint32_t *base = task->stack_addr;
    
    if (sp >= base && sp < task->stack_top) {
        return (uint32_t)(task->stack_top - sp) * sizeof(uint32_t);
    }
    
    return 0;
}

/**
 * @brief 获取任务栈剩余量
 */
uint32_t rtos_task_get_stack_free(const rtos_task_t *task)
{
    if (task == NULL) {
        return 0;
    }
    
    uint32_t usage = rtos_task_get_stack_usage(task);
    if (usage > task->stack_size) {
        return 0;
    }
    
    return task->stack_size - usage;
}

/* 任务调度器相关函数 */

/**
 * @brief 初始化调度器
 */
void rtos_scheduler_init(void)
{
    g_scheduler_running = false;
    g_scheduler_lock_level = 0;
    g_task_switch_count = 0;
}

/**
 * @brief 启动调度器
 */
void rtos_scheduler_start(void)
{
    if (g_scheduler_running) {
        return;
    }
    
    g_scheduler_running = true;
    
    /* 选择第一个就绪任务 */
    rtos_task_t *first_task = rtos_task_get_highest_priority_ready();
    if (first_task) {
        rtos_task_switch_to(first_task);
    }
}

/**
 * @brief 停止调度器
 */
void rtos_scheduler_stop(void)
{
    g_scheduler_running = false;
}

/**
 * @brief 检查调度器是否运行
 */
bool rtos_scheduler_is_running(void)
{
    return g_scheduler_running;
}

/**
 * @brief 执行调度
 */
void rtos_scheduler_schedule(void)
{
    if (!g_scheduler_running || g_scheduler_lock_level > 0) {
        return;
    }
    
    /* 获取最高优先级的就绪任务 */
    rtos_task_t *next_task = rtos_task_get_highest_priority_ready();
    if (next_task == NULL || next_task == g_current_task) {
        return;
    }
    
    /* 执行任务切换 */
    rtos_task_switch_to(next_task);
}

/**
 * @brief 锁定调度器
 */
uint32_t rtos_scheduler_lock(void)
{
    g_scheduler_lock_level++;
    return g_scheduler_lock_level;
}

/**
 * @brief 解锁调度器
 */
void rtos_scheduler_unlock(uint32_t level)
{
    if (g_scheduler_lock_level > 0 && g_scheduler_lock_level >= level) {
        g_scheduler_lock_level = level - 1;
    }
}

/* 空闲任务相关函数 */

/**
 * @brief 空闲任务入口函数
 */
void rtos_task_idle_entry(void *parameter)
{
    (void)parameter;
    
    while (1) {
        /* 执行空闲钩子函数 */
        if (g_idle_hook) {
            g_idle_hook();
        }
        
        /* 进入低功耗模式 */
        /* 这里需要根据具体硬件实现 */
    }
}

/**
 * @brief 设置空闲任务钩子函数
 */
void rtos_task_set_idle_hook(void (*hook)(void))
{
    g_idle_hook = hook;
}

/**
 * @brief 删除空闲任务钩子函数
 */
void rtos_task_delete_idle_hook(void (*hook)(void))
{
    if (g_idle_hook == hook) {
        g_idle_hook = NULL;
    }
}

/* 内部函数实现 */

/**
 * @brief 添加任务到就绪队列
 */
static void rtos_task_add_to_ready_queue(rtos_task_t *task)
{
    if (task == NULL) {
        return;
    }
    
    /* 按优先级插入队列 */
    rtos_task_node_t *current = g_ready_queue_head.next;
    
    while (current != &g_ready_queue_head) {
        rtos_task_t *current_task = current->task;
        if (task->priority < current_task->priority) {
            break;
        }
        current = current->next;
    }
    
    /* 插入到current之前 */
    task->ready_node.next = current;
    task->ready_node.prev = current->prev;
    current->prev->next = &task->ready_node;
    current->prev = &task->ready_node;
}

/**
 * @brief 从就绪队列移除任务
 */
static void rtos_task_remove_from_ready_queue(rtos_task_t *task)
{
    if (task == NULL) {
        return;
    }
    
    task->ready_node.prev->next = task->ready_node.next;
    task->ready_node.next->prev = task->ready_node.prev;
    task->ready_node.next = NULL;
    task->ready_node.prev = NULL;
}

/**
 * @brief 添加任务到阻塞队列
 */
static void rtos_task_add_to_blocked_queue(rtos_task_t *task)
{
    if (task == NULL) {
        return;
    }
    
    /* 添加到阻塞队列末尾 */
    task->block_node.next = &g_blocked_queue_head;
    task->block_node.prev = g_blocked_queue_head.prev;
    g_blocked_queue_head.prev->next = &task->block_node;
    g_blocked_queue_head.prev = &task->block_node;
}

/**
 * @brief 从阻塞队列移除任务
 */
static void rtos_task_remove_from_blocked_queue(rtos_task_t *task)
{
    if (task == NULL) {
        return;
    }
    
    task->block_node.prev->next = task->block_node.next;
    task->block_node.next->prev = task->block_node.prev;
    task->block_node.next = NULL;
    task->block_node.prev = NULL;
}

/**
 * @brief 获取最高优先级的就绪任务
 */
static rtos_task_t *rtos_task_get_highest_priority_ready(void)
{
    if (g_ready_queue_head.next == &g_ready_queue_head) {
        return NULL;
    }
    
    return g_ready_queue_head.next->task;
}

/**
 * @brief 切换到指定任务
 */
static void rtos_task_switch_to(rtos_task_t *new_task)
{
    if (new_task == NULL || new_task == g_current_task) {
        return;
    }
    
    rtos_task_t *old_task = g_current_task;
    
    /* 执行任务切换钩子 */
    if (g_task_switch_hook) {
        g_task_switch_hook(old_task, new_task);
    }
    
    /* 更新当前任务 */
    g_current_task = new_task;
    g_current_task->state = RTOS_TASK_STATE_RUNNING;
    g_current_task->switch_count++;
    
    /* 更新统计信息 */
    g_task_switch_count++;
    
    /* 这里需要实现具体的上下文切换 */
    /* 调用硬件相关的上下文切换函数 */
}

/**
 * @brief 任务栈初始化
 */
uint32_t *rtos_task_stack_init(void (*task_entry)(void *),
                               void *parameter,
                               uint32_t *stack_addr,
                               rtos_stack_size_t stack_size)
{
    if (task_entry == NULL || stack_addr == NULL) {
        return NULL;
    }
    
    /* 设置栈魔数 */
    uint32_t *stack_ptr = stack_addr + (stack_size / sizeof(uint32_t));
    
    /* 在栈顶设置魔数 */
    *(--stack_ptr) = RTOS_STACK_MAGIC;
    
    /* 模拟ARM Cortex-M的栈帧 */
    *(--stack_ptr) = 0x01000000;  /* xPSR */
    *(--stack_ptr) = (uint32_t)task_entry;  /* PC */
    *(--stack_ptr) = 0xFFFFFFFD;  /* LR */
    *(--stack_ptr) = 0x12121212;  /* R12 */
    *(--stack_ptr) = 0x03030303;  /* R3 */
    *(--stack_ptr) = 0x02020202;  /* R2 */
    *(--stack_ptr) = 0x01010101;  /* R1 */
    *(--stack_ptr) = (uint32_t)parameter;  /* R0 */
    
    return stack_ptr;
}

/**
 * @brief 设置任务切换钩子
 */
void rtos_task_set_switch_hook(rtos_task_switch_hook_t hook)
{
    g_task_switch_hook = hook;
}
