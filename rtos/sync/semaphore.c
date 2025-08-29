/**
 * @file semaphore.c
 * @brief RTOS信号量系统实现
 * @author Assistant
 * @date 2024
 */

#include "semaphore.h"
#include "../task/task.h"
#include <string.h>
#include <stdlib.h>

/* 内部函数声明 */
static void rtos_semaphore_wakeup_waiting_tasks(rtos_semaphore_t *sem);

/**
 * @brief 初始化信号量 - 静态方式
 */
rtos_result_t rtos_semaphore_init(rtos_semaphore_t *sem,
                                  const rtos_semaphore_create_params_t *params)
{
    if (sem == NULL || params == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (params->initial_count > params->max_count) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化对象基类 */
    rtos_object_init(&sem->parent, RTOS_OBJECT_TYPE_SEMAPHORE, 
                     params->name, RTOS_OBJECT_FLAG_STATIC);
    
    /* 设置信号量属性 */
    sem->count = params->initial_count;
    sem->max_count = params->max_count;
    
    /* 初始化等待队列 */
    rtos_wait_queue_init(&sem->wait_queue);
    sem->wait_count = 0;
    
    /* 初始化统计信息 */
    sem->take_count = 0;
    sem->give_count = 0;
    
    /* 添加到信号量容器 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_SEMAPHORE);
    if (container) {
        rtos_object_container_add(container, &sem->parent);
    }
    
    return RTOS_OK;
}

/**
 * @brief 创建信号量 - 动态方式
 */
rtos_semaphore_t *rtos_semaphore_create(const rtos_semaphore_create_params_t *params)
{
    if (params == NULL) {
        return NULL;
    }
    
    /* 分配信号量控制块 */
    rtos_semaphore_t *sem = (rtos_semaphore_t *)malloc(sizeof(rtos_semaphore_t));
    if (sem == NULL) {
        return NULL;
    }
    
    /* 初始化信号量 */
    rtos_result_t result = rtos_semaphore_init(sem, params);
    if (result != RTOS_OK) {
        free(sem);
        return NULL;
    }
    
    /* 设置动态分配标志 */
    rtos_object_set_flags(&sem->parent, RTOS_OBJECT_FLAG_DYNAMIC);
    
    return sem;
}

/**
 * @brief 删除信号量
 */
rtos_result_t rtos_semaphore_delete(rtos_semaphore_t *sem)
{
    if (sem == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否有任务在等待 */
    if (sem->wait_count > 0) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 从容器中移除 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_SEMAPHORE);
    if (container) {
        rtos_object_container_remove(container, &sem->parent);
    }
    
    /* 如果是动态分配的，释放内存 */
    if (rtos_object_is_dynamic(&sem->parent)) {
        free(sem);
    }
    
    return RTOS_OK;
}

/**
 * @brief 获取信号量
 */
rtos_result_t rtos_semaphore_take(rtos_semaphore_t *sem, rtos_timeout_t timeout)
{
    if (sem == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_task_t *current_task = rtos_task_get_current();
    if (current_task == NULL) {
        return RTOS_ERROR;
    }
    
    /* 检查信号量是否可用 */
    if (sem->count > 0) {
        sem->count--;
        sem->take_count++;
        return RTOS_OK;
    }
    
    /* 如果超时时间为0，立即返回 */
    if (timeout == RTOS_TIMEOUT_IMMEDIATE) {
        return RTOS_ERROR_TIMEOUT;
    }
    
    /* 添加到等待队列 */
    rtos_result_t result = rtos_wait_queue_add(&sem->wait_queue, current_task, timeout);
    if (result != RTOS_OK) {
        return result;
    }
    
    sem->wait_count++;
    
    /* 阻塞当前任务 */
    current_task->state = RTOS_TASK_STATE_BLOCKED;
    current_task->wait_object = &sem->parent;
    current_task->timeout = timeout;
    
    /* 触发调度 */
    rtos_task_yield();
    
    return RTOS_OK;
}

/**
 * @brief 释放信号量
 */
rtos_result_t rtos_semaphore_give(rtos_semaphore_t *sem)
{
    if (sem == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否超过最大计数值 */
    if (sem->count >= sem->max_count) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 增加计数值 */
    sem->count++;
    sem->give_count++;
    
    /* 唤醒等待的任务 */
    rtos_semaphore_wakeup_waiting_tasks(sem);
    
    return RTOS_OK;
}

/**
 * @brief 尝试获取信号量(非阻塞)
 */
rtos_result_t rtos_semaphore_try_take(rtos_semaphore_t *sem)
{
    return rtos_semaphore_take(sem, RTOS_TIMEOUT_IMMEDIATE);
}

/**
 * @brief 获取信号量信息
 */
rtos_result_t rtos_semaphore_get_info(const rtos_semaphore_t *sem,
                                      rtos_semaphore_info_t *info)
{
    if (sem == NULL || info == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    strncpy(info->name, sem->parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    info->count = sem->count;
    info->max_count = sem->max_count;
    info->wait_count = sem->wait_count;
    info->take_count = sem->take_count;
    info->give_count = sem->give_count;
    
    return RTOS_OK;
}

/**
 * @brief 重置信号量
 */
rtos_result_t rtos_semaphore_reset(rtos_semaphore_t *sem, uint32_t count)
{
    if (sem == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (count > sem->max_count) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否有任务在等待 */
    if (sem->wait_count > 0) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 重置计数值 */
    sem->count = count;
    
    return RTOS_OK;
}

/* 内部函数实现 */

/**
 * @brief 唤醒等待的任务
 */
static void rtos_semaphore_wakeup_waiting_tasks(rtos_semaphore_t *sem)
{
    if (sem->wait_count == 0) {
        return;
    }
    
    /* 获取第一个等待的任务 */
    rtos_task_t *waiting_task = rtos_wait_queue_get_first(&sem->wait_queue);
    if (waiting_task == NULL) {
        return;
    }
    
    /* 从等待队列移除 */
    rtos_wait_queue_remove(&sem->wait_queue, waiting_task);
    sem->wait_count--;
    
    /* 恢复任务状态 */
    waiting_task->state = RTOS_TASK_STATE_READY;
    waiting_task->wait_object = NULL;
    waiting_task->timeout = 0;
    
    /* 添加到就绪队列 */
    /* 这里需要调用任务管理模块的函数 */
    /* 暂时使用简单的状态设置 */
}
