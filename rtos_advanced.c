/**
 * @file rtos_advanced.c
 * @brief RTOS高级功能实现 - 软件定时器、事件组、内存池、调试、跟踪等
 */

#include "rtos_kernel.h"
#include "rtos_hw.h"
#include <string.h>

/* 外部变量声明 */
extern rtos_system_t rtos_system;
extern rtos_sw_timer_t rtos_sw_timers[RTOS_MAX_SW_TIMERS];
extern rtos_event_group_t rtos_event_groups[RTOS_MAX_EVENT_GROUPS];
extern rtos_memory_pool_t rtos_memory_pools[RTOS_MAX_MEMORY_POOLS];

/* 内部函数声明 */
static void rtos_sw_timer_process(void);
static void rtos_trace_add_record(rtos_trace_event_t event, uint32_t task_id, uint32_t data);

/* ========== 软件定时器实现 ========== */

/**
 * @brief 创建软件定时器
 */
rtos_result_t rtos_sw_timer_create(rtos_sw_timer_t *timer, const char *name,
                                  rtos_time_ns_t period_ns, bool auto_reload,
                                  void (*callback)(void *), void *param)
{
    if (!timer || !callback || period_ns == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 初始化软件定时器 */
    timer->period_ns = period_ns;
    timer->next_expire_ns = 0;
    timer->auto_reload = auto_reload;
    timer->is_active = false;
    timer->callback = callback;
    timer->param = param;
    timer->next = NULL;
    
    /* 复制名称 */
    if (name) {
        strncpy(timer->name, name, sizeof(timer->name) - 1);
        timer->name[sizeof(timer->name) - 1] = '\0';
    } else {
        strcpy(timer->name, "Unknown");
    }
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 启动软件定时器
 */
rtos_result_t rtos_sw_timer_start(rtos_sw_timer_t *timer)
{
    if (!timer) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    timer->next_expire_ns = rtos_hw_get_time_ns() + timer->period_ns;
    timer->is_active = true;
    
    /* 添加到系统软件定时器链表 */
    timer->next = rtos_system.sw_timer_list;
    rtos_system.sw_timer_list = timer;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 停止软件定时器
 */
rtos_result_t rtos_sw_timer_stop(rtos_sw_timer_t *timer)
{
    if (!timer) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    timer->is_active = false;
    
    /* 从链表中移除 */
    rtos_sw_timer_t **current = &rtos_system.sw_timer_list;
    while (*current) {
        if (*current == timer) {
            *current = timer->next;
            break;
        }
        current = &((*current)->next);
    }
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 删除软件定时器
 */
rtos_result_t rtos_sw_timer_delete(rtos_sw_timer_t *timer)
{
    rtos_result_t result = rtos_sw_timer_stop(timer);
    if (result == RTOS_OK) {
        memset(timer, 0, sizeof(rtos_sw_timer_t));
    }
    return result;
}

/**
 * @brief 重置软件定时器
 */
rtos_result_t rtos_sw_timer_reset(rtos_sw_timer_t *timer)
{
    if (!timer || !timer->is_active) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    timer->next_expire_ns = rtos_hw_get_time_ns() + timer->period_ns;
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 软件定时器处理(内部函数)
 */
static void rtos_sw_timer_process(void)
{
    rtos_time_ns_t current_time = rtos_hw_get_time_ns();
    rtos_sw_timer_t *timer = rtos_system.sw_timer_list;
    
    while (timer) {
        if (timer->is_active && current_time >= timer->next_expire_ns) {
            /* 定时器到期，调用回调函数 */
            if (timer->callback) {
                timer->callback(timer->param);
            }
            
            /* 添加跟踪记录 */
            rtos_trace_add_record(RTOS_TRACE_TIMER_EXPIRE, 0, (uint32_t)timer);
            
            /* 处理自动重装载 */
            if (timer->auto_reload) {
                timer->next_expire_ns = current_time + timer->period_ns;
            } else {
                timer->is_active = false;
            }
        }
        timer = timer->next;
    }
}

/**
 * @brief 软件定时器中断处理
 */
void rtos_sw_timer_isr(void)
{
    rtos_sw_timer_process();
}

/* ========== 事件标志组实现 ========== */

/**
 * @brief 创建事件组
 */
rtos_result_t rtos_event_group_create(rtos_event_group_t *group)
{
    if (!group) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    group->flags = 0;
    group->wait_list = NULL;
    group->is_valid = true;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 设置事件标志位
 */
rtos_result_t rtos_event_group_set_bits(rtos_event_group_t *group, uint32_t bits)
{
    if (!group || !group->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    group->flags |= bits;
    
    /* 检查并唤醒等待的任务 */
    rtos_task_t **current = &group->wait_list;
    while (*current) {
        rtos_task_t *task = *current;
        
        /* 这里简化处理，实际应该检查等待条件 */
        if (group->flags & bits) {
            /* 移除任务从等待链表 */
            *current = task->next;
            
            /* 唤醒任务 */
            task->state = TASK_STATE_READY;
            extern void rtos_add_task_to_ready_list(rtos_task_t *task);
            rtos_add_task_to_ready_list(task);
        } else {
            current = &task->next;
        }
    }
    
    rtos_exit_critical();
    
    /* 如果有任务被唤醒，触发调度 */
    if (group->flags & bits) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 清除事件标志位
 */
rtos_result_t rtos_event_group_clear_bits(rtos_event_group_t *group, uint32_t bits)
{
    if (!group || !group->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    group->flags &= ~bits;
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 等待事件标志位
 */
rtos_result_t rtos_event_group_wait_bits(rtos_event_group_t *group, 
                                        uint32_t bits_to_wait,
                                        bool clear_on_exit,
                                        bool wait_for_all,
                                        uint32_t timeout_us)
{
    if (!group || !group->is_valid || bits_to_wait == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 检查条件是否已满足 */
    bool condition_met;
    if (wait_for_all) {
        condition_met = (group->flags & bits_to_wait) == bits_to_wait;
    } else {
        condition_met = (group->flags & bits_to_wait) != 0;
    }
    
    if (condition_met) {
        if (clear_on_exit) {
            group->flags &= ~bits_to_wait;
        }
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 条件未满足，需要等待 */
    if (timeout_us == 0) {
        rtos_exit_critical();
        return RTOS_ERROR_TIMEOUT;
    }
    
    /* 将当前任务加入等待链表 */
    rtos_task_t *current_task = rtos_system.current_task;
    current_task->next = group->wait_list;
    group->wait_list = current_task;
    current_task->state = TASK_STATE_BLOCKED;
    
    /* 从就绪队列移除 */
    extern void rtos_remove_task_from_ready_list(rtos_task_t *task);
    rtos_remove_task_from_ready_list(current_task);
    
    rtos_exit_critical();
    
    /* 设置超时(如果需要) */
    if (timeout_us != 0xFFFFFFFF) {
        rtos_delay_us(timeout_us);
    }
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 删除事件组
 */
rtos_result_t rtos_event_group_delete(rtos_event_group_t *group)
{
    if (!group || !group->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 唤醒所有等待的任务 */
    while (group->wait_list) {
        rtos_task_t *task = group->wait_list;
        group->wait_list = task->next;
        
        task->state = TASK_STATE_READY;
        extern void rtos_add_task_to_ready_list(rtos_task_t *task);
        rtos_add_task_to_ready_list(task);
    }
    
    group->is_valid = false;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/* ========== 内存池管理实现 ========== */

/**
 * @brief 创建内存池
 */
rtos_result_t rtos_memory_pool_create(rtos_memory_pool_t *pool,
                                     void *buffer,
                                     uint32_t block_size,
                                     uint32_t block_count)
{
    if (!pool || !buffer || block_size == 0 || block_count == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    pool->pool_buffer = buffer;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->free_blocks = block_count;
    pool->wait_list = NULL;
    pool->is_valid = true;
    
    /* 初始化空闲链表 */
    uint8_t *block_ptr = (uint8_t *)buffer;
    pool->free_list = block_ptr;
    
    for (uint32_t i = 0; i < block_count - 1; i++) {
        *(void **)block_ptr = block_ptr + block_size;
        block_ptr += block_size;
    }
    *(void **)block_ptr = NULL; /* 最后一个块指向NULL */
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 从内存池分配内存
 */
void *rtos_memory_pool_alloc(rtos_memory_pool_t *pool, uint32_t timeout_us)
{
    if (!pool || !pool->is_valid) {
        return NULL;
    }
    
    rtos_enter_critical();
    
    /* 检查是否有空闲块 */
    if (pool->free_list) {
        void *block = pool->free_list;
        pool->free_list = *(void **)pool->free_list;
        pool->free_blocks--;
        
        rtos_exit_critical();
        return block;
    }
    
    /* 没有空闲块，需要等待 */
    if (timeout_us == 0) {
        rtos_exit_critical();
        return NULL;
    }
    
    /* 将当前任务加入等待链表 */
    rtos_task_t *current_task = rtos_system.current_task;
    current_task->next = pool->wait_list;
    pool->wait_list = current_task;
    current_task->state = TASK_STATE_BLOCKED;
    
    /* 从就绪队列移除 */
    extern void rtos_remove_task_from_ready_list(rtos_task_t *task);
    rtos_remove_task_from_ready_list(current_task);
    
    rtos_exit_critical();
    
    /* 设置超时 */
    if (timeout_us != 0xFFFFFFFF) {
        rtos_delay_us(timeout_us);
    }
    
    /* 触发调度 */
    rtos_schedule();
    
    /* 任务被唤醒后，再次尝试分配 */
    rtos_enter_critical();
    void *block = NULL;
    if (pool->free_list) {
        block = pool->free_list;
        pool->free_list = *(void **)pool->free_list;
        pool->free_blocks--;
    }
    rtos_exit_critical();
    
    return block;
}

/**
 * @brief 释放内存到内存池
 */
rtos_result_t rtos_memory_pool_free(rtos_memory_pool_t *pool, void *block)
{
    if (!pool || !pool->is_valid || !block) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 将块添加到空闲链表 */
    *(void **)block = pool->free_list;
    pool->free_list = block;
    pool->free_blocks++;
    
    /* 唤醒等待的任务 */
    if (pool->wait_list) {
        rtos_task_t *task = pool->wait_list;
        pool->wait_list = task->next;
        
        task->state = TASK_STATE_READY;
        extern void rtos_add_task_to_ready_list(rtos_task_t *task);
        rtos_add_task_to_ready_list(task);
        
        rtos_exit_critical();
        
        /* 触发调度 */
        rtos_schedule();
    } else {
        rtos_exit_critical();
    }
    
    return RTOS_OK;
}

/**
 * @brief 删除内存池
 */
rtos_result_t rtos_memory_pool_delete(rtos_memory_pool_t *pool)
{
    if (!pool || !pool->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 唤醒所有等待的任务 */
    while (pool->wait_list) {
        rtos_task_t *task = pool->wait_list;
        pool->wait_list = task->next;
        
        task->state = TASK_STATE_READY;
        extern void rtos_add_task_to_ready_list(rtos_task_t *task);
        rtos_add_task_to_ready_list(task);
    }
    
    pool->is_valid = false;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/* ========== MPU内存保护API实现 ========== */

/**
 * @brief MPU初始化
 */
rtos_result_t rtos_mpu_init(void)
{
    rtos_hw_mpu_init();
    return RTOS_OK;
}

/**
 * @brief 配置MPU区域
 */
rtos_result_t rtos_mpu_configure_region(uint8_t region_id, rtos_mpu_region_t *config)
{
    if (!config || region_id >= 8) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_hw_mpu_configure_region(region_id, config->base_addr, 
                                config->size, config->permissions);
    
    if (config->enable) {
        rtos_hw_mpu_enable();
    }
    
    return RTOS_OK;
}

/**
 * @brief 为任务启用MPU保护
 */
rtos_result_t rtos_mpu_enable_task_protection(rtos_task_t *task)
{
    if (!task) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 这里可以为任务配置专用的MPU区域 */
    /* 实际实现需要根据任务需求配置不同的内存区域 */
    
    return RTOS_OK;
}

/**
 * @brief 为任务禁用MPU保护
 */
rtos_result_t rtos_mpu_disable_task_protection(rtos_task_t *task)
{
    if (!task) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_hw_mpu_disable();
    return RTOS_OK;
}

/* ========== 功耗管理API实现 ========== */

/**
 * @brief 进入睡眠模式
 */
rtos_result_t rtos_power_enter_sleep(rtos_sleep_mode_t mode)
{
    rtos_system.sleep_mode = mode;
    rtos_hw_enter_sleep(mode);
    return RTOS_OK;
}

/**
 * @brief 配置唤醒源
 */
rtos_result_t rtos_power_configure_wakeup(uint32_t sources)
{
    rtos_system.wakeup_sources = sources;
    rtos_hw_configure_wakeup(sources);
    return RTOS_OK;
}

/**
 * @brief 设置CPU频率
 */
rtos_result_t rtos_power_set_cpu_frequency(uint32_t frequency_hz)
{
    rtos_hw_set_cpu_frequency(frequency_hz);
    return RTOS_OK;
}

/* ========== 调试和监控实现 ========== */

/**
 * @brief 获取任务调试信息
 */
rtos_result_t rtos_debug_get_task_info(rtos_task_t *task, rtos_debug_info_t *info)
{
    if (!task || !info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    info->task_switch_count = task->task_switch_count;
    info->cpu_usage_percent = 0; /* 需要实现CPU使用率计算 */
    info->free_stack_size = task->stack_size - task->stack_high_water;
    info->uptime_ns = rtos_hw_get_time_ns();
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 获取系统调试信息
 */
rtos_result_t rtos_debug_get_system_info(rtos_debug_info_t *info)
{
    if (!info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    info->task_switch_count = rtos_system.total_task_switches;
    info->interrupt_count = rtos_system.total_interrupts;
    info->cpu_usage_percent = 0; /* 需要实现 */
    info->uptime_ns = rtos_hw_get_time_ns();
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 打印系统状态(需要printf支持)
 */
rtos_result_t rtos_debug_print_system_state(void)
{
    /* 这里需要printf实现，暂时返回OK */
    return RTOS_OK;
}

/**
 * @brief 启用任务栈检查
 */
rtos_result_t rtos_debug_enable_stack_checking(rtos_task_t *task)
{
    if (!task) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    task->stack_check_enabled = true;
    /* 在栈底设置保护字 */
    *(uint32_t *)task->stack_base = task->stack_canary;
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 禁用任务栈检查
 */
rtos_result_t rtos_debug_disable_stack_checking(rtos_task_t *task)
{
    if (!task) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    task->stack_check_enabled = false;
    rtos_exit_critical();
    
    return RTOS_OK;
}

/* ========== 安全特性实现 ========== */

/**
 * @brief 注册错误处理函数
 */
rtos_result_t rtos_safety_register_error_handler(rtos_error_type_t type, 
                                                void (*handler)(void))
{
    if (!handler || type >= 4) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    rtos_system.error_handlers[type] = handler;
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 检查任务栈溢出
 */
rtos_result_t rtos_safety_check_stack_overflow(rtos_task_t *task)
{
    if (!task || !task->stack_check_enabled) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查栈底保护字 */
    if (*(uint32_t *)task->stack_base != task->stack_canary) {
        /* 栈溢出！调用错误处理函数 */
        if (rtos_system.error_handlers[RTOS_ERROR_TYPE_STACK_OVERFLOW]) {
            rtos_system.error_handlers[RTOS_ERROR_TYPE_STACK_OVERFLOW]();
        }
        return RTOS_ERROR_STACK_OVERFLOW;
    }
    
    return RTOS_OK;
}

/**
 * @brief 启用看门狗
 */
rtos_result_t rtos_safety_enable_watchdog(uint32_t timeout_ms)
{
    /* 这里需要实现看门狗配置 */
    /* STM32F407 独立看门狗配置 */
    return RTOS_OK;
}

/* ========== 系统跟踪实现 ========== */

/**
 * @brief 启动系统跟踪
 */
rtos_result_t rtos_trace_start(rtos_trace_record_t *buffer, uint32_t buffer_size)
{
    if (!buffer || buffer_size == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    rtos_system.trace_buffer = buffer;
    rtos_system.trace_buffer_size = buffer_size;
    rtos_system.trace_index = 0;
    rtos_system.trace_enabled = true;
    
    /* 清空缓冲区 */
    memset(buffer, 0, buffer_size * sizeof(rtos_trace_record_t));
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 停止系统跟踪
 */
rtos_result_t rtos_trace_stop(void)
{
    rtos_enter_critical();
    rtos_system.trace_enabled = false;
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 获取跟踪数据
 */
rtos_result_t rtos_trace_get_data(rtos_trace_record_t *buffer, uint32_t *count)
{
    if (!buffer || !count) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    *count = rtos_system.trace_index;
    if (rtos_system.trace_buffer && *count > 0) {
        memcpy(buffer, rtos_system.trace_buffer, 
               *count * sizeof(rtos_trace_record_t));
    }
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 添加跟踪事件
 */
rtos_result_t rtos_trace_add_event(rtos_trace_event_t event, uint32_t data)
{
    uint32_t task_id = rtos_system.current_task ? 
                       (uint32_t)rtos_system.current_task : 0;
    rtos_trace_add_record(event, task_id, data);
    return RTOS_OK;
}

/**
 * @brief 添加跟踪记录(内部函数)
 */
static void rtos_trace_add_record(rtos_trace_event_t event, uint32_t task_id, uint32_t data)
{
    if (!rtos_system.trace_enabled || !rtos_system.trace_buffer) {
        return;
    }
    
    if (rtos_system.trace_index < rtos_system.trace_buffer_size) {
        rtos_trace_record_t *record = &rtos_system.trace_buffer[rtos_system.trace_index];
        record->event_type = event;
        record->timestamp_ns = rtos_hw_get_time_ns();
        record->task_id = task_id;
        record->data = data;
        
        rtos_system.trace_index++;
    }
}

/* ========== 高级同步实现 ========== */

/**
 * @brief 等待多个对象
 */
rtos_result_t rtos_wait_multiple(void **objects, uint32_t count, 
                               bool wait_all, uint32_t timeout_us)
{
    if (!objects || count == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 这是一个复杂的功能，需要识别对象类型并处理等待逻辑 */
    /* 暂时返回不支持 */
    return RTOS_ERROR;
}