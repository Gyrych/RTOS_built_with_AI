/**
 * @file rtos_sync.c
 * @brief RTOS同步机制实现(信号量、互斥量、消息队列)
 */

#include "rtos_kernel.h"
#include <string.h>

/* 外部变量声明 */
extern rtos_system_t rtos_system;
extern rtos_semaphore_t rtos_semaphores[RTOS_MAX_SEMAPHORES];
extern rtos_mutex_t rtos_mutexes[RTOS_MAX_MUTEXES];
extern rtos_queue_t rtos_queues[RTOS_MAX_QUEUES];

/* 内部函数声明 */
static void rtos_block_task_with_timeout(rtos_task_t *task, rtos_task_t **wait_list, uint32_t timeout_us);
static rtos_result_t rtos_wait_with_timeout(rtos_task_t **wait_list, uint32_t timeout_us);
static void rtos_priority_inherit(rtos_task_t *task, uint8_t priority);
static void rtos_priority_restore(rtos_task_t *task);

/* ========== 信号量实现 ========== */

/**
 * @brief 创建信号量
 */
rtos_result_t rtos_semaphore_create(rtos_semaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
    if (!sem || max_count == 0 || initial_count > max_count) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    sem->count = initial_count;
    sem->max_count = max_count;
    sem->wait_list = NULL;
    sem->is_valid = true;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 获取信号量
 */
rtos_result_t rtos_semaphore_take(rtos_semaphore_t *sem, uint32_t timeout_us)
{
    if (!sem || !sem->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    if (sem->count > 0) {
        /* 信号量可用，直接获取 */
        sem->count--;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 信号量不可用，需要等待 */
    if (timeout_us == 0) {
        rtos_exit_critical();
        return RTOS_ERROR_TIMEOUT;
    }
    
    rtos_result_t result = rtos_wait_with_timeout(&sem->wait_list, timeout_us);
    
    if (result == RTOS_OK) {
        sem->count--;
    }
    
    rtos_exit_critical();
    return result;
}

/**
 * @brief 释放信号量
 */
rtos_result_t rtos_semaphore_give(rtos_semaphore_t *sem)
{
    if (!sem || !sem->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 检查是否有任务在等待 */
    if (sem->wait_list != NULL) {
        /* 唤醒等待的任务 */
        rtos_task_t *task = rtos_get_next_task_from_wait_list(&sem->wait_list);
        if (task) {
            task->state = TASK_STATE_READY;
            rtos_add_task_to_ready_list(task);
            
            rtos_exit_critical();
            
            /* 如果唤醒的任务优先级更高，触发调度 */
            if (task->priority < rtos_system.current_task->priority) {
                rtos_schedule();
            }
            
            return RTOS_OK;
        }
    }
    
    /* 没有等待任务，增加计数 */
    if (sem->count < sem->max_count) {
        sem->count++;
    }
    
    rtos_exit_critical();
    return RTOS_OK;
}

/**
 * @brief 删除信号量
 */
rtos_result_t rtos_semaphore_delete(rtos_semaphore_t *sem)
{
    if (!sem || !sem->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 唤醒所有等待的任务 */
    while (sem->wait_list != NULL) {
        rtos_task_t *task = rtos_get_next_task_from_wait_list(&sem->wait_list);
        if (task) {
            task->state = TASK_STATE_READY;
            rtos_add_task_to_ready_list(task);
        }
    }
    
    sem->is_valid = false;
    
    rtos_exit_critical();
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/* ========== 互斥量实现 ========== */

/**
 * @brief 创建互斥量
 */
rtos_result_t rtos_mutex_create(rtos_mutex_t *mutex)
{
    if (!mutex) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    mutex->owner = NULL;
    mutex->nest_count = 0;
    mutex->original_priority = 0;
    mutex->wait_list = NULL;
    mutex->is_valid = true;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 锁定互斥量
 */
rtos_result_t rtos_mutex_lock(rtos_mutex_t *mutex, uint32_t timeout_us)
{
    if (!mutex || !mutex->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_task_t *current_task = rtos_system.current_task;
    
    rtos_enter_critical();
    
    if (mutex->owner == NULL) {
        /* 互斥量可用，直接获取 */
        mutex->owner = current_task;
        mutex->nest_count = 1;
        mutex->original_priority = current_task->priority;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    if (mutex->owner == current_task) {
        /* 递归锁定 */
        mutex->nest_count++;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 互斥量被其他任务持有，实现优先级继承 */
    if (current_task->priority < mutex->owner->priority) {
        rtos_priority_inherit(mutex->owner, current_task->priority);
    }
    
    /* 需要等待 */
    if (timeout_us == 0) {
        rtos_exit_critical();
        return RTOS_ERROR_TIMEOUT;
    }
    
    rtos_result_t result = rtos_wait_with_timeout(&mutex->wait_list, timeout_us);
    
    if (result == RTOS_OK) {
        mutex->owner = current_task;
        mutex->nest_count = 1;
        mutex->original_priority = current_task->priority;
    }
    
    rtos_exit_critical();
    return result;
}

/**
 * @brief 解锁互斥量
 */
rtos_result_t rtos_mutex_unlock(rtos_mutex_t *mutex)
{
    if (!mutex || !mutex->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_task_t *current_task = rtos_system.current_task;
    
    rtos_enter_critical();
    
    if (mutex->owner != current_task) {
        rtos_exit_critical();
        return RTOS_ERROR;
    }
    
    mutex->nest_count--;
    
    if (mutex->nest_count > 0) {
        /* 仍然有嵌套锁定 */
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 完全解锁，恢复原始优先级 */
    rtos_priority_restore(current_task);
    
    /* 检查是否有任务在等待 */
    if (mutex->wait_list != NULL) {
        rtos_task_t *next_task = rtos_get_next_task_from_wait_list(&mutex->wait_list);
        if (next_task) {
            mutex->owner = next_task;
            mutex->nest_count = 1;
            mutex->original_priority = next_task->priority;
            
            next_task->state = TASK_STATE_READY;
            rtos_add_task_to_ready_list(next_task);
            
            rtos_exit_critical();
            
            /* 如果唤醒的任务优先级更高，触发调度 */
            if (next_task->priority < current_task->priority) {
                rtos_schedule();
            }
            
            return RTOS_OK;
        }
    }
    
    mutex->owner = NULL;
    
    rtos_exit_critical();
    return RTOS_OK;
}

/**
 * @brief 删除互斥量
 */
rtos_result_t rtos_mutex_delete(rtos_mutex_t *mutex)
{
    if (!mutex || !mutex->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 如果有持有者，恢复其优先级 */
    if (mutex->owner) {
        rtos_priority_restore(mutex->owner);
    }
    
    /* 唤醒所有等待的任务 */
    while (mutex->wait_list != NULL) {
        rtos_task_t *task = rtos_get_next_task_from_wait_list(&mutex->wait_list);
        if (task) {
            task->state = TASK_STATE_READY;
            rtos_add_task_to_ready_list(task);
        }
    }
    
    mutex->is_valid = false;
    
    rtos_exit_critical();
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/* ========== 消息队列实现 ========== */

/**
 * @brief 创建消息队列
 */
rtos_result_t rtos_queue_create(rtos_queue_t *queue, void *buffer, uint32_t item_size, uint32_t max_items)
{
    if (!queue || !buffer || item_size == 0 || max_items == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    queue->buffer = (uint8_t *)buffer;
    queue->item_size = item_size;
    queue->max_items = max_items;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->send_wait_list = NULL;
    queue->recv_wait_list = NULL;
    queue->is_valid = true;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/**
 * @brief 发送消息到队列
 */
rtos_result_t rtos_queue_send(rtos_queue_t *queue, const void *item, uint32_t timeout_us)
{
    if (!queue || !queue->is_valid || !item) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 检查是否有任务在等待接收 */
    if (queue->recv_wait_list != NULL && queue->count == 0) {
        rtos_task_t *recv_task = rtos_get_next_task_from_wait_list(&queue->recv_wait_list);
        if (recv_task) {
            /* 直接传递消息给等待任务 */
            memcpy(queue->buffer + queue->tail * queue->item_size, item, queue->item_size);
            queue->count = 1; /* 临时设置，等接收任务取走 */
            
            recv_task->state = TASK_STATE_READY;
            rtos_add_task_to_ready_list(recv_task);
            
            rtos_exit_critical();
            
            if (recv_task->priority < rtos_system.current_task->priority) {
                rtos_schedule();
            }
            
            return RTOS_OK;
        }
    }
    
    /* 检查队列是否已满 */
    if (queue->count >= queue->max_items) {
        if (timeout_us == 0) {
            rtos_exit_critical();
            return RTOS_ERROR_TIMEOUT;
        }
        
        /* 等待队列有空间 */
        rtos_result_t result = rtos_wait_with_timeout(&queue->send_wait_list, timeout_us);
        if (result != RTOS_OK) {
            rtos_exit_critical();
            return result;
        }
    }
    
    /* 复制消息到队列 */
    memcpy(queue->buffer + queue->tail * queue->item_size, item, queue->item_size);
    queue->tail = (queue->tail + 1) % queue->max_items;
    queue->count++;
    
    rtos_exit_critical();
    return RTOS_OK;
}

/**
 * @brief 从队列接收消息
 */
rtos_result_t rtos_queue_receive(rtos_queue_t *queue, void *item, uint32_t timeout_us)
{
    if (!queue || !queue->is_valid || !item) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    if (queue->count > 0) {
        /* 有消息可接收 */
        memcpy(item, queue->buffer + queue->head * queue->item_size, queue->item_size);
        queue->head = (queue->head + 1) % queue->max_items;
        queue->count--;
        
        /* 检查是否有任务在等待发送 */
        if (queue->send_wait_list != NULL) {
            rtos_task_t *send_task = rtos_get_next_task_from_wait_list(&queue->send_wait_list);
            if (send_task) {
                send_task->state = TASK_STATE_READY;
                rtos_add_task_to_ready_list(send_task);
            }
        }
        
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 队列为空，需要等待 */
    if (timeout_us == 0) {
        rtos_exit_critical();
        return RTOS_ERROR_TIMEOUT;
    }
    
    rtos_result_t result = rtos_wait_with_timeout(&queue->recv_wait_list, timeout_us);
    
    if (result == RTOS_OK) {
        /* 接收消息 */
        memcpy(item, queue->buffer + queue->head * queue->item_size, queue->item_size);
        queue->head = (queue->head + 1) % queue->max_items;
        queue->count--;
    }
    
    rtos_exit_critical();
    return result;
}

/**
 * @brief 删除消息队列
 */
rtos_result_t rtos_queue_delete(rtos_queue_t *queue)
{
    if (!queue || !queue->is_valid) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 唤醒所有等待的任务 */
    while (queue->send_wait_list != NULL) {
        rtos_task_t *task = rtos_get_next_task_from_wait_list(&queue->send_wait_list);
        if (task) {
            task->state = TASK_STATE_READY;
            rtos_add_task_to_ready_list(task);
        }
    }
    
    while (queue->recv_wait_list != NULL) {
        rtos_task_t *task = rtos_get_next_task_from_wait_list(&queue->recv_wait_list);
        if (task) {
            task->state = TASK_STATE_READY;
            rtos_add_task_to_ready_list(task);
        }
    }
    
    queue->is_valid = false;
    
    rtos_exit_critical();
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/* ========== 内部函数实现 ========== */

/**
 * @brief 带超时的等待
 */
static rtos_result_t rtos_wait_with_timeout(rtos_task_t **wait_list, uint32_t timeout_us)
{
    rtos_task_t *current_task = rtos_system.current_task;
    
    /* 将当前任务加入等待列表 */
    rtos_block_task(current_task, wait_list);
    
    /* 设置超时定时器 */
    if (timeout_us != UINT32_MAX) {
        /* 创建超时定时器(简化实现) */
        current_task->delay_ticks = timeout_us;
    }
    
    rtos_exit_critical();
    
    /* 触发调度 */
    rtos_schedule();
    
    rtos_enter_critical();
    
    /* 检查是否超时 */
    if (current_task->delay_ticks > 0) {
        /* 超时了，从等待列表移除 */
        rtos_unblock_task(current_task, wait_list);
        return RTOS_ERROR_TIMEOUT;
    }
    
    return RTOS_OK;
}

/**
 * @brief 阻塞任务
 */
static void rtos_block_task(rtos_task_t *task, rtos_task_t **wait_list)
{
    task->state = TASK_STATE_BLOCKED;
    rtos_remove_task_from_ready_list(task);
    
    /* 按优先级顺序插入等待列表 */
    if (*wait_list == NULL || (*wait_list)->priority > task->priority) {
        task->next = *wait_list;
        *wait_list = task;
    } else {
        rtos_task_t *current = *wait_list;
        while (current->next != NULL && current->next->priority <= task->priority) {
            current = current->next;
        }
        task->next = current->next;
        current->next = task;
    }
}

/**
 * @brief 解除任务阻塞
 */
static void rtos_unblock_task(rtos_task_t *task, rtos_task_t **wait_list)
{
    rtos_task_t **current = wait_list;
    
    while (*current != NULL) {
        if (*current == task) {
            *current = task->next;
            task->next = NULL;
            break;
        }
        current = &((*current)->next);
    }
}

/**
 * @brief 从等待列表获取下一个任务
 */
static rtos_task_t *rtos_get_next_task_from_wait_list(rtos_task_t **wait_list)
{
    if (*wait_list == NULL) {
        return NULL;
    }
    
    rtos_task_t *task = *wait_list;
    *wait_list = task->next;
    task->next = NULL;
    
    return task;
}

/**
 * @brief 优先级继承
 */
static void rtos_priority_inherit(rtos_task_t *task, uint8_t priority)
{
    if (task->priority > priority) {
        /* 移除任务从当前优先级队列 */
        rtos_remove_task_from_ready_list(task);
        
        /* 设置新优先级 */
        task->priority = priority;
        
        /* 如果任务是就绪状态，重新加入队列 */
        if (task->state == TASK_STATE_READY || task->state == TASK_STATE_RUNNING) {
            rtos_add_task_to_ready_list(task);
        }
    }
}

/**
 * @brief 恢复原始优先级
 */
static void rtos_priority_restore(rtos_task_t *task)
{
    /* 这里简化实现，实际应该维护优先级堆栈 */
    /* 暂时不实现复杂的优先级恢复逻辑 */
}