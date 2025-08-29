/**
 * @file mutex.c
 * @brief RTOS互斥量系统实现
 * @author Assistant
 * @date 2024
 */

#include "mutex.h"
#include "../task/task.h"
#include <string.h>
#include <stdlib.h>

/* 内部函数声明 */
static void rtos_mutex_wakeup_waiting_tasks(rtos_mutex_t *mutex);
static rtos_result_t rtos_mutex_priority_inheritance(rtos_mutex_t *mutex, rtos_task_t *task);

/**
 * @brief 初始化互斥量 - 静态方式
 */
rtos_result_t rtos_mutex_init(rtos_mutex_t *mutex,
                              const rtos_mutex_create_params_t *params)
{
    if (mutex == NULL || params == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化对象基类 */
    rtos_object_init(&mutex->parent, RTOS_OBJECT_TYPE_MUTEX, 
                     params->name, RTOS_OBJECT_FLAG_STATIC);
    
    /* 设置互斥量属性 */
    mutex->owner_priority = RTOS_PRIORITY_NORMAL;
    mutex->original_priority = RTOS_PRIORITY_NORMAL;
    mutex->owner = NULL;
    mutex->lock_count = 0;
    mutex->is_recursive = params->recursive;
    
    /* 初始化等待队列 */
    rtos_wait_queue_init(&mutex->wait_queue);
    mutex->wait_count = 0;
    
    /* 初始化统计信息 */
    mutex->lock_count_total = 0;
    mutex->unlock_count_total = 0;
    mutex->max_hold_time = 0;
    
    /* 添加到互斥量容器 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_MUTEX);
    if (container) {
        rtos_object_container_add(container, &mutex->parent);
    }
    
    return RTOS_OK;
}

/**
 * @brief 创建互斥量 - 动态方式
 */
rtos_mutex_t *rtos_mutex_create(const rtos_mutex_create_params_t *params)
{
    if (params == NULL) {
        return NULL;
    }
    
    /* 分配互斥量控制块 */
    rtos_mutex_t *mutex = (rtos_mutex_t *)malloc(sizeof(rtos_mutex_t));
    if (mutex == NULL) {
        return NULL;
    }
    
    /* 初始化互斥量 */
    rtos_result_t result = rtos_mutex_init(mutex, params);
    if (result != RTOS_OK) {
        free(mutex);
        return NULL;
    }
    
    /* 设置动态分配标志 */
    rtos_object_set_flags(&mutex->parent, RTOS_OBJECT_FLAG_DYNAMIC);
    
    return mutex;
}

/**
 * @brief 删除互斥量
 */
rtos_result_t rtos_mutex_delete(rtos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否被锁定 */
    if (mutex->owner != NULL) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 检查是否有任务在等待 */
    if (mutex->wait_count > 0) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 从容器中移除 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_MUTEX);
    if (container) {
        rtos_object_container_remove(container, &mutex->parent);
    }
    
    /* 如果是动态分配的，释放内存 */
    if (rtos_object_is_dynamic(&mutex->parent)) {
        free(mutex);
    }
    
    return RTOS_OK;
}

/**
 * @brief 锁定互斥量
 */
rtos_result_t rtos_mutex_take(rtos_mutex_t *mutex, rtos_timeout_t timeout)
{
    if (mutex == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_task_t *current_task = rtos_task_get_current();
    if (current_task == NULL) {
        return RTOS_ERROR;
    }
    
    /* 检查是否已经被当前任务持有 */
    if (mutex->owner == current_task) {
        if (mutex->is_recursive) {
            mutex->lock_count++;
            mutex->lock_count_total++;
            return RTOS_OK;
        } else {
            return RTOS_ERROR_DEADLOCK;
        }
    }
    
    /* 检查是否可用 */
    if (mutex->owner == NULL) {
        mutex->owner = current_task;
        mutex->owner_priority = current_task->priority;
        mutex->original_priority = current_task->priority;
        mutex->lock_count = 1;
        mutex->lock_count_total++;
        return RTOS_OK;
    }
    
    /* 如果超时时间为0，立即返回 */
    if (timeout == RTOS_TIMEOUT_IMMEDIATE) {
        return RTOS_ERROR_TIMEOUT;
    }
    
    /* 添加到等待队列 */
    rtos_result_t result = rtos_wait_queue_add(&mutex->wait_queue, current_task, timeout);
    if (result != RTOS_OK) {
        return result;
    }
    
    mutex->wait_count++;
    
    /* 优先级继承处理 */
    rtos_mutex_priority_inheritance(mutex, current_task);
    
    /* 阻塞当前任务 */
    current_task->state = RTOS_TASK_STATE_BLOCKED;
    current_task->wait_object = &mutex->parent;
    current_task->timeout = timeout;
    
    /* 触发调度 */
    rtos_task_yield();
    
    return RTOS_OK;
}

/**
 * @brief 解锁互斥量
 */
rtos_result_t rtos_mutex_release(rtos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_task_t *current_task = rtos_task_get_current();
    if (current_task == NULL) {
        return RTOS_ERROR;
    }
    
    /* 检查是否是所有者 */
    if (mutex->owner != current_task) {
        return RTOS_ERROR;
    }
    
    /* 减少锁定计数 */
    if (mutex->lock_count > 1) {
        mutex->lock_count--;
        mutex->unlock_count_total++;
        return RTOS_OK;
    }
    
    /* 完全解锁 */
    mutex->lock_count = 0;
    mutex->unlock_count_total++;
    
    /* 恢复原始优先级 */
    if (current_task->priority != mutex->original_priority) {
        rtos_task_set_priority(current_task, mutex->original_priority);
    }
    
    /* 清除所有者信息 */
    mutex->owner = NULL;
    mutex->owner_priority = RTOS_PRIORITY_NORMAL;
    mutex->original_priority = RTOS_PRIORITY_NORMAL;
    
    /* 唤醒等待的任务 */
    rtos_mutex_wakeup_waiting_tasks(mutex);
    
    return RTOS_OK;
}

/**
 * @brief 尝试锁定互斥量(非阻塞)
 */
rtos_result_t rtos_mutex_try_take(rtos_mutex_t *mutex)
{
    return rtos_mutex_take(mutex, RTOS_TIMEOUT_IMMEDIATE);
}

/**
 * @brief 获取互斥量信息
 */
rtos_result_t rtos_mutex_get_info(const rtos_mutex_t *mutex,
                                  rtos_mutex_info_t *info)
{
    if (mutex == NULL || info == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    strncpy(info->name, mutex->parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    info->is_locked = (mutex->owner != NULL);
    info->owner_priority = mutex->owner_priority;
    info->lock_count = mutex->lock_count;
    info->is_recursive = mutex->is_recursive;
    info->wait_count = mutex->wait_count;
    info->lock_count_total = mutex->lock_count_total;
    info->unlock_count_total = mutex->unlock_count_total;
    
    return RTOS_OK;
}

/**
 * @brief 设置互斥量天花板优先级
 */
rtos_result_t rtos_mutex_set_ceiling_priority(rtos_mutex_t *mutex,
                                              rtos_priority_t ceiling_priority)
{
    if (mutex == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (ceiling_priority >= RTOS_PRIORITY_LEVELS) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    mutex->original_priority = ceiling_priority;
    
    return RTOS_OK;
}

/**
 * @brief 获取互斥量天花板优先级
 */
rtos_priority_t rtos_mutex_get_ceiling_priority(const rtos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return RTOS_PRIORITY_NORMAL;
    }
    
    return mutex->original_priority;
}

/**
 * @brief 检查互斥量是否被当前任务持有
 */
bool rtos_mutex_is_owner(const rtos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return false;
    }
    
    rtos_task_t *current_task = rtos_task_get_current();
    if (current_task == NULL) {
        return false;
    }
    
    return (mutex->owner == current_task);
}

/* 内部函数实现 */

/**
 * @brief 唤醒等待的任务
 */
static void rtos_mutex_wakeup_waiting_tasks(rtos_mutex_t *mutex)
{
    if (mutex->wait_count == 0) {
        return;
    }
    
    /* 获取第一个等待的任务 */
    rtos_task_t *waiting_task = rtos_wait_queue_get_first(&mutex->wait_queue);
    if (waiting_task == NULL) {
        return;
    }
    
    /* 从等待队列移除 */
    rtos_wait_queue_remove(&mutex->wait_queue, waiting_task);
    mutex->wait_count--;
    
    /* 恢复任务状态 */
    waiting_task->state = RTOS_TASK_STATE_READY;
    waiting_task->wait_object = NULL;
    waiting_task->timeout = 0;
    
    /* 添加到就绪队列 */
    /* 这里需要调用任务管理模块的函数 */
    /* 暂时使用简单的状态设置 */
}

/**
 * @brief 优先级继承处理
 */
static rtos_result_t rtos_mutex_priority_inheritance(rtos_mutex_t *mutex, rtos_task_t *task)
{
    if (mutex->owner == NULL || task == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 如果等待任务的优先级更高，提升所有者优先级 */
    if (task->priority < mutex->owner->priority) {
        rtos_task_set_priority(mutex->owner, task->priority);
        mutex->owner_priority = task->priority;
    }
    
    return RTOS_OK;
}
