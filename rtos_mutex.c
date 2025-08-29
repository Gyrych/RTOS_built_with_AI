/**
 * @file rtos_mutex.c
 * @brief RTOS互斥量模块实现 - 基于IPC对象
 */

#include "rtos_mutex.h"
#include "rtos_object.h"
#include "rtos_task.h"
#include "rtos_kernel.h"
#include <string.h>

/* 内部函数声明 */
static rtos_result_t rtos_mutex_take_with_timeout(rtos_mutex_t *mutex, rtos_time_ns_t timeout);
static void rtos_mutex_priority_inherit(rtos_mutex_t *mutex, rtos_task_t *task);
static void rtos_mutex_priority_restore(rtos_mutex_t *mutex, rtos_task_t *task);

/**
 * @brief 初始化互斥量 - 静态方式
 */
rtos_result_t rtos_mutex_init(rtos_mutex_t *mutex,
                             const char   *name,
                             uint8_t       flag)
{
    if (!mutex) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化IPC对象基类 */
    rtos_ipc_object_init(&(mutex->parent), RTOS_OBJECT_CLASS_MUTEX, name);
    
    /* 初始化互斥量特有字段 */
    mutex->value = 1;                           /* 初始状态为可用 */
    mutex->recursive_count = 0;
    mutex->owner = NULL;
    mutex->original_priority = 0;
    mutex->hold_count = 0;
    mutex->flag = flag;
    mutex->reserved = 0;
    
    return RTOS_OK;
}

/**
 * @brief 创建互斥量 - 动态方式
 */
rtos_mutex_t *rtos_mutex_create(const char *name, uint8_t flag)
{
    rtos_mutex_t *mutex;
    
    /* 分配互斥量对象 */
    mutex = (rtos_mutex_t *)rtos_object_allocate(RTOS_OBJECT_CLASS_MUTEX, name);
    if (!mutex) {
        return NULL;
    }
    
    /* 初始化互斥量 */
    if (rtos_mutex_init(mutex, name, flag) != RTOS_OK) {
        rtos_object_delete(&(mutex->parent.parent));
        return NULL;
    }
    
    return mutex;
}

/**
 * @brief 分离互斥量
 */
rtos_result_t rtos_mutex_detach(rtos_mutex_t *mutex)
{
    if (!rtos_mutex_is_valid(mutex) || !rtos_object_is_static(&(mutex->parent.parent))) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 如果互斥量被持有，恢复持有者优先级 */
    if (mutex->owner) {
        rtos_mutex_priority_restore(mutex, mutex->owner);
        mutex->owner = NULL;
    }
    
    /* 恢复所有挂起的线程 */
    rtos_ipc_object_resume_all_thread(&(mutex->parent));
    
    /* 从对象系统分离 */
    rtos_object_detach(&(mutex->parent.parent));
    
    rtos_exit_critical();
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 删除互斥量
 */
rtos_result_t rtos_mutex_delete(rtos_mutex_t *mutex)
{
    if (!rtos_mutex_is_valid(mutex) || !rtos_object_is_dynamic(&(mutex->parent.parent))) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 如果互斥量被持有，恢复持有者优先级 */
    if (mutex->owner) {
        rtos_mutex_priority_restore(mutex, mutex->owner);
        mutex->owner = NULL;
    }
    
    /* 恢复所有挂起的线程 */
    rtos_ipc_object_resume_all_thread(&(mutex->parent));
    
    rtos_exit_critical();
    
    /* 从对象系统删除 */
    rtos_object_delete(&(mutex->parent.parent));
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 获取互斥量
 */
rtos_result_t rtos_mutex_take(rtos_mutex_t *mutex, rtos_time_ns_t time)
{
    rtos_task_t *task;
    rtos_result_t result;
    
    if (!rtos_mutex_is_valid(mutex)) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    task = rtos_task_self();
    if (!task) {
        return RTOS_ERROR;
    }
    
    /* 如果不等待且互斥量不可用，直接返回 */
    if (time == RTOS_WAITING_NO && mutex->value == 0) {
        return RTOS_ERROR_TIMEOUT;
    }
    
    rtos_enter_critical();
    
    /* 如果互斥量可用，直接获取 */
    if (mutex->value > 0) {
        mutex->value = 0;
        mutex->owner = task;
        mutex->original_priority = task->current_priority;
        mutex->hold_count = 1;
        mutex->recursive_count = 1;
        
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 检查是否为递归锁定 */
    if (mutex->owner == task) {
        mutex->recursive_count++;
        mutex->hold_count++;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 互斥量不可用，需要等待 */
    if (time == RTOS_WAITING_NO) {
        rtos_exit_critical();
        return RTOS_ERROR_TIMEOUT;
    }
    
    /* 实现优先级继承 */
    if (mutex->flag & RTOS_MUTEX_FLAG_INHERIT) {
        rtos_mutex_priority_inherit(mutex, task);
    }
    
    /* 将任务挂起到互斥量的等待队列 */
    result = rtos_ipc_object_suspend_thread(&(mutex->parent), task, 0, (int32_t)time);
    if (result != RTOS_OK) {
        rtos_exit_critical();
        return result;
    }
    
    /* 如果有超时设置，处理超时 */
    if (time != RTOS_WAITING_FOREVER) {
        result = rtos_mutex_take_with_timeout(mutex, time);
    } else {
        rtos_exit_critical();
        
        /* 触发调度，等待被唤醒 */
        rtos_schedule();
        
        rtos_enter_critical();
        
        /* 检查是否成功获取互斥量 */
        if (task->error == RTOS_OK) {
            result = RTOS_OK;
        } else {
            result = task->error;
        }
    }
    
    rtos_exit_critical();
    return result;
}

/**
 * @brief 尝试获取互斥量(不阻塞)
 */
rtos_result_t rtos_mutex_trytake(rtos_mutex_t *mutex)
{
    return rtos_mutex_take(mutex, RTOS_WAITING_NO);
}

/**
 * @brief 释放互斥量
 */
rtos_result_t rtos_mutex_release(rtos_mutex_t *mutex)
{
    rtos_task_t *task;
    rtos_task_t *current_task;
    bool need_schedule = false;
    
    if (!rtos_mutex_is_valid(mutex)) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    current_task = rtos_task_self();
    if (!current_task) {
        return RTOS_ERROR;
    }
    
    rtos_enter_critical();
    
    /* 检查是否为当前任务持有 */
    if (mutex->owner != current_task) {
        rtos_exit_critical();
        return RTOS_ERROR;
    }
    
    /* 处理递归锁定 */
    mutex->recursive_count--;
    if (mutex->recursive_count > 0) {
        mutex->hold_count--;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 完全释放互斥量 */
    mutex->hold_count = 0;
    
    /* 恢复原始优先级 */
    if (mutex->flag & RTOS_MUTEX_FLAG_INHERIT) {
        rtos_mutex_priority_restore(mutex, current_task);
    }
    
    /* 如果有等待的任务，传递给下一个任务 */
    if (mutex->parent.suspend_thread) {
        /* 恢复等待的任务 */
        task = rtos_ipc_object_resume_thread(&(mutex->parent));
        if (task) {
            /* 将互斥量传递给恢复的任务 */
            mutex->owner = task;
            mutex->original_priority = task->current_priority;
            mutex->hold_count = 1;
            mutex->recursive_count = 1;
            mutex->value = 0;
            
            task->error = RTOS_OK;
            
            /* 如果恢复的任务优先级更高，需要调度 */
            if (task->current_priority < current_task->current_priority) {
                need_schedule = true;
            }
        }
    } else {
        /* 没有等待任务，释放互斥量 */
        mutex->owner = NULL;
        mutex->value = 1;
    }
    
    rtos_exit_critical();
    
    /* 如果需要调度，触发调度 */
    if (need_schedule) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 控制互斥量
 */
rtos_result_t rtos_mutex_control(rtos_mutex_t *mutex, int cmd, void *arg)
{
    if (!rtos_mutex_is_valid(mutex)) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    switch (cmd) {
        case RTOS_MUTEX_CTRL_RESET:
            /* 重置互斥量 */
            if (mutex->owner) {
                rtos_mutex_priority_restore(mutex, mutex->owner);
            }
            mutex->owner = NULL;
            mutex->value = 1;
            mutex->hold_count = 0;
            mutex->recursive_count = 0;
            /* 恢复所有等待的任务 */
            rtos_ipc_object_resume_all_thread(&(mutex->parent));
            break;
            
        case RTOS_MUTEX_CTRL_GET_OWNER:
            if (arg) {
                *(rtos_task_t **)arg = mutex->owner;
            } else {
                rtos_exit_critical();
                return RTOS_ERROR_INVALID_PARAM;
            }
            break;
            
        default:
            rtos_exit_critical();
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_exit_critical();
    
    /* 如果是重置操作，可能需要调度 */
    if (cmd == RTOS_MUTEX_CTRL_RESET) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 获取互斥量持有者
 */
rtos_task_t *rtos_mutex_get_owner(rtos_mutex_t *mutex)
{
    if (!rtos_mutex_is_valid(mutex)) {
        return NULL;
    }
    
    return mutex->owner;
}

/**
 * @brief 查找互斥量
 */
rtos_mutex_t *rtos_mutex_find(const char *name)
{
    rtos_object_t *object;
    
    object = rtos_object_find(RTOS_OBJECT_CLASS_MUTEX, name);
    if (object) {
        return rtos_container_of(object, rtos_mutex_t, parent.parent);
    }
    
    return NULL;
}

/**
 * @brief 获取互斥量信息
 */
rtos_result_t rtos_mutex_get_info(rtos_mutex_t *mutex, rtos_mutex_info_t *info)
{
    rtos_task_t *task;
    uint32_t count = 0;
    
    if (!rtos_mutex_is_valid(mutex) || !info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 复制基本信息 */
    strncpy(info->name, mutex->parent.parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    info->value = mutex->value;
    info->hold_count = mutex->hold_count;
    
    /* 持有者信息 */
    if (mutex->owner) {
        strncpy(info->owner_name, mutex->owner->parent.name, sizeof(info->owner_name) - 1);
        info->owner_name[sizeof(info->owner_name) - 1] = '\0';
        info->owner_priority = mutex->owner->current_priority;
    } else {
        strcpy(info->owner_name, "None");
        info->owner_priority = 0;
    }
    
    /* 统计挂起线程数量 */
    task = mutex->parent.suspend_thread;
    while (task) {
        count++;
        task = task->tlist;
    }
    info->suspend_thread_count = count;
    
    rtos_exit_critical();
    
    return RTOS_OK;
}

/* ========== 内部函数实现 ========== */

/**
 * @brief 带超时的互斥量获取
 */
static rtos_result_t rtos_mutex_take_with_timeout(rtos_mutex_t *mutex, rtos_time_ns_t timeout)
{
    rtos_task_t *task;
    rtos_result_t result;
    
    task = rtos_task_self();
    if (!task) {
        return RTOS_ERROR;
    }
    
    /* 设置超时定时器(简化实现) */
    task->remaining_tick = timeout;
    
    rtos_exit_critical();
    
    /* 触发调度，等待被唤醒或超时 */
    rtos_schedule();
    
    rtos_enter_critical();
    
    /* 检查任务状态 */
    if (task->remaining_tick == 0) {
        /* 超时了 */
        result = RTOS_ERROR_TIMEOUT;
        task->error = RTOS_ERROR_TIMEOUT;
        
        /* 从等待队列中移除任务 */
        rtos_task_t **task_ptr = &(mutex->parent.suspend_thread);
        while (*task_ptr) {
            if (*task_ptr == task) {
                *task_ptr = task->tlist;
                task->tlist = NULL;
                break;
            }
            task_ptr = &((*task_ptr)->tlist);
        }
    } else {
        /* 正常获取到互斥量 */
        result = RTOS_OK;
        task->error = RTOS_OK;
    }
    
    return result;
}

/**
 * @brief 优先级继承
 */
static void rtos_mutex_priority_inherit(rtos_mutex_t *mutex, rtos_task_t *task)
{
    rtos_task_t *owner = mutex->owner;
    
    if (!owner || !task) {
        return;
    }
    
    /* 如果等待任务的优先级更高，提升持有者的优先级 */
    if (task->current_priority < owner->current_priority) {
        /* 从就绪队列移除持有者 */
        if (owner->stat == RTOS_TASK_READY) {
            rtos_schedule_remove_task(owner);
        }
        
        /* 提升优先级 */
        owner->current_priority = task->current_priority;
        
        /* 重新加入就绪队列 */
        if (owner->stat == RTOS_TASK_READY) {
            rtos_schedule_insert_task(owner);
        }
    }
}

/**
 * @brief 恢复原始优先级
 */
static void rtos_mutex_priority_restore(rtos_mutex_t *mutex, rtos_task_t *task)
{
    if (!task) {
        return;
    }
    
    /* 简化实现：直接恢复到初始优先级 */
    /* 实际应该检查是否还有其他互斥量需要更高优先级 */
    if (task->current_priority != task->init_priority) {
        /* 从就绪队列移除任务 */
        if (task->stat == RTOS_TASK_READY) {
            rtos_schedule_remove_task(task);
        }
        
        /* 恢复优先级 */
        task->current_priority = task->init_priority;
        
        /* 重新加入就绪队列 */
        if (task->stat == RTOS_TASK_READY) {
            rtos_schedule_insert_task(task);
        }
    }
}