/**
 * @file rtos_semaphore.c
 * @brief RTOS信号量模块实现 - 基于IPC对象
 */

#include "rtos_semaphore.h"
#include "rtos_object.h"
#include "rtos_task.h"
#include "rtos_kernel.h"
#include <string.h>

/* 内部函数声明 */
static rtos_result_t rtos_sem_take_with_timeout(rtos_semaphore_t *sem, rtos_time_ns_t timeout);

/**
 * @brief 初始化信号量 - 静态方式
 */
rtos_result_t rtos_sem_init(rtos_semaphore_t *sem,
                           const char       *name,
                           uint32_t          value,
                           uint8_t           flag)
{
    if (!sem) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化IPC对象基类 */
    rtos_ipc_object_init(&(sem->parent), RTOS_OBJECT_CLASS_SEMAPHORE, name);
    
    /* 初始化信号量特有字段 */
    sem->value = value & 0xFFFF;
    sem->reserved = 0;
    
    return RTOS_OK;
}

/**
 * @brief 创建信号量 - 动态方式
 */
rtos_semaphore_t *rtos_sem_create(const char *name,
                                 uint32_t    value,
                                 uint8_t     flag)
{
    rtos_semaphore_t *sem;
    
    /* 分配信号量对象 */
    sem = (rtos_semaphore_t *)rtos_object_allocate(RTOS_OBJECT_CLASS_SEMAPHORE, name);
    if (!sem) {
        return NULL;
    }
    
    /* 初始化信号量 */
    if (rtos_sem_init(sem, name, value, flag) != RTOS_OK) {
        rtos_object_delete(&(sem->parent.parent));
        return NULL;
    }
    
    return sem;
}

/**
 * @brief 分离信号量
 */
rtos_result_t rtos_sem_detach(rtos_semaphore_t *sem)
{
    if (!rtos_sem_is_valid(sem) || !rtos_object_is_static(&(sem->parent.parent))) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 恢复所有挂起的线程 */
    rtos_ipc_object_resume_all_thread(&(sem->parent));
    
    /* 从对象系统分离 */
    rtos_object_detach(&(sem->parent.parent));
    
    rtos_exit_critical();
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 删除信号量
 */
rtos_result_t rtos_sem_delete(rtos_semaphore_t *sem)
{
    if (!rtos_sem_is_valid(sem) || !rtos_object_is_dynamic(&(sem->parent.parent))) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 恢复所有挂起的线程 */
    rtos_ipc_object_resume_all_thread(&(sem->parent));
    
    rtos_exit_critical();
    
    /* 从对象系统删除 */
    rtos_object_delete(&(sem->parent.parent));
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/**
 * @brief 获取信号量
 */
rtos_result_t rtos_sem_take(rtos_semaphore_t *sem, rtos_time_ns_t time)
{
    rtos_task_t *task;
    rtos_result_t result;
    
    if (!rtos_sem_is_valid(sem)) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 如果不等待且信号量不可用，直接返回 */
    if (time == RTOS_WAITING_NO && sem->value == 0) {
        return RTOS_ERROR_TIMEOUT;
    }
    
    rtos_enter_critical();
    
    /* 如果信号量可用，直接获取 */
    if (sem->value > 0) {
        sem->value--;
        rtos_exit_critical();
        return RTOS_OK;
    }
    
    /* 信号量不可用，需要等待 */
    if (time == RTOS_WAITING_NO) {
        rtos_exit_critical();
        return RTOS_ERROR_TIMEOUT;
    }
    
    /* 挂起当前任务 */
    task = rtos_task_self();
    if (!task) {
        rtos_exit_critical();
        return RTOS_ERROR;
    }
    
    /* 将任务挂起到信号量的等待队列 */
    result = rtos_ipc_object_suspend_thread(&(sem->parent), task, 0, (int32_t)time);
    if (result != RTOS_OK) {
        rtos_exit_critical();
        return result;
    }
    
    /* 如果有超时设置，处理超时 */
    if (time != RTOS_WAITING_FOREVER) {
        result = rtos_sem_take_with_timeout(sem, time);
    } else {
        rtos_exit_critical();
        
        /* 触发调度，等待被唤醒 */
        rtos_schedule();
        
        rtos_enter_critical();
        
        /* 检查是否成功获取信号量 */
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
 * @brief 尝试获取信号量(不阻塞)
 */
rtos_result_t rtos_sem_trytake(rtos_semaphore_t *sem)
{
    return rtos_sem_take(sem, RTOS_WAITING_NO);
}

/**
 * @brief 释放信号量
 */
rtos_result_t rtos_sem_release(rtos_semaphore_t *sem)
{
    rtos_task_t *task;
    bool need_schedule = false;
    
    if (!rtos_sem_is_valid(sem)) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 如果有等待的任务，直接唤醒 */
    if (sem->parent.suspend_thread) {
        /* 恢复等待的任务 */
        task = rtos_ipc_object_resume_thread(&(sem->parent));
        if (task) {
            task->error = RTOS_OK;
            
            /* 如果恢复的任务优先级更高，需要调度 */
            rtos_task_t *current = rtos_task_self();
            if (current && task->current_priority < current->current_priority) {
                need_schedule = true;
            }
        }
    } else {
        /* 没有等待任务，增加信号量值 */
        if (sem->value < 0xFFFF) {
            sem->value++;
        }
    }
    
    rtos_exit_critical();
    
    /* 如果需要调度，触发调度 */
    if (need_schedule) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 控制信号量
 */
rtos_result_t rtos_sem_control(rtos_semaphore_t *sem, int cmd, void *arg)
{
    if (!rtos_sem_is_valid(sem)) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    switch (cmd) {
        case RTOS_SEM_CTRL_RESET:
            /* 重置信号量 */
            sem->value = 0;
            /* 恢复所有等待的任务 */
            rtos_ipc_object_resume_all_thread(&(sem->parent));
            break;
            
        case RTOS_SEM_CTRL_SET_VALUE:
            if (arg) {
                uint32_t value = *(uint32_t *)arg;
                sem->value = value & 0xFFFF;
            } else {
                rtos_exit_critical();
                return RTOS_ERROR_INVALID_PARAM;
            }
            break;
            
        case RTOS_SEM_CTRL_GET_VALUE:
            if (arg) {
                *(uint32_t *)arg = sem->value;
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
    if (cmd == RTOS_SEM_CTRL_RESET) {
        rtos_schedule();
    }
    
    return RTOS_OK;
}

/**
 * @brief 获取信号量当前值
 */
int rtos_sem_get_value(rtos_semaphore_t *sem)
{
    if (!rtos_sem_is_valid(sem)) {
        return -1;
    }
    
    return (int)sem->value;
}

/**
 * @brief 查找信号量
 */
rtos_semaphore_t *rtos_sem_find(const char *name)
{
    rtos_object_t *object;
    
    object = rtos_object_find(RTOS_OBJECT_CLASS_SEMAPHORE, name);
    if (object) {
        return rtos_container_of(object, rtos_semaphore_t, parent.parent);
    }
    
    return NULL;
}

/**
 * @brief 获取信号量信息
 */
rtos_result_t rtos_sem_get_info(rtos_semaphore_t *sem, rtos_sem_info_t *info)
{
    rtos_task_t *task;
    uint32_t count = 0;
    
    if (!rtos_sem_is_valid(sem) || !info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 复制基本信息 */
    strncpy(info->name, sem->parent.parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    info->value = sem->value;
    
    /* 统计挂起线程数量 */
    task = sem->parent.suspend_thread;
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
 * @brief 带超时的信号量获取
 */
static rtos_result_t rtos_sem_take_with_timeout(rtos_semaphore_t *sem, rtos_time_ns_t timeout)
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
        rtos_task_t **task_ptr = &(sem->parent.suspend_thread);
        while (*task_ptr) {
            if (*task_ptr == task) {
                *task_ptr = task->tlist;
                task->tlist = NULL;
                break;
            }
            task_ptr = &((*task_ptr)->tlist);
        }
    } else {
        /* 正常获取到信号量 */
        result = RTOS_OK;
        task->error = RTOS_OK;
    }
    
    return result;
}