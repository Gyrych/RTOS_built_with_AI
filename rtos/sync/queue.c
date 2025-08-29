/**
 * @file queue.c
 * @brief RTOS消息队列系统实现
 * @author Assistant
 * @date 2024
 */

#include "queue.h"
#include "../task/task.h"
#include <string.h>
#include <stdlib.h>

/* 内部函数声明 */
static void rtos_queue_wakeup_send_waiting_tasks(rtos_queue_t *queue);
static void rtos_queue_wakeup_recv_waiting_tasks(rtos_queue_t *queue);
static bool rtos_queue_is_empty_internal(const rtos_queue_t *queue);
static bool rtos_queue_is_full_internal(const rtos_queue_t *queue);

/**
 * @brief 初始化消息队列 - 静态方式
 */
rtos_result_t rtos_queue_init(rtos_queue_t *queue,
                              const rtos_queue_create_params_t *params,
                              void *buffer)
{
    if (queue == NULL || params == NULL || buffer == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (params->item_size == 0 || params->max_items == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化对象基类 */
    rtos_object_init(&queue->parent, RTOS_OBJECT_TYPE_QUEUE, 
                     params->name, RTOS_OBJECT_FLAG_STATIC);
    
    /* 设置队列属性 */
    queue->buffer = (uint8_t *)buffer;
    queue->item_size = params->item_size;
    queue->max_items = params->max_items;
    queue->item_count = 0;
    queue->head = 0;
    queue->tail = 0;
    
    /* 初始化等待队列 */
    rtos_wait_queue_init(&queue->send_wait_queue);
    rtos_wait_queue_init(&queue->recv_wait_queue);
    queue->send_wait_count = 0;
    queue->recv_wait_count = 0;
    
    /* 初始化统计信息 */
    queue->send_count = 0;
    queue->recv_count = 0;
    queue->overflow_count = 0;
    
    /* 添加到队列容器 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_QUEUE);
    if (container) {
        rtos_object_container_add(container, &queue->parent);
    }
    
    return RTOS_OK;
}

/**
 * @brief 创建消息队列 - 动态方式
 */
rtos_queue_t *rtos_queue_create(const rtos_queue_create_params_t *params)
{
    if (params == NULL) {
        return NULL;
    }
    
    if (params->item_size == 0 || params->max_items == 0) {
        return NULL;
    }
    
    /* 分配队列控制块 */
    rtos_queue_t *queue = (rtos_queue_t *)malloc(sizeof(rtos_queue_t));
    if (queue == NULL) {
        return NULL;
    }
    
    /* 分配缓冲区 */
    uint8_t *buffer = (uint8_t *)malloc(params->item_size * params->max_items);
    if (buffer == NULL) {
        free(queue);
        return NULL;
    }
    
    /* 初始化队列 */
    rtos_result_t result = rtos_queue_init(queue, params, buffer);
    if (result != RTOS_OK) {
        free(buffer);
        free(queue);
        return NULL;
    }
    
    /* 设置动态分配标志 */
    rtos_object_set_flags(&queue->parent, RTOS_OBJECT_FLAG_DYNAMIC);
    
    return queue;
}

/**
 * @brief 删除消息队列
 */
rtos_result_t rtos_queue_delete(rtos_queue_t *queue)
{
    if (queue == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否有任务在等待 */
    if (queue->send_wait_count > 0 || queue->recv_wait_count > 0) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 从容器中移除 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_QUEUE);
    if (container) {
        rtos_object_container_remove(container, &queue->parent);
    }
    
    /* 如果是动态分配的，释放内存 */
    if (rtos_object_is_dynamic(&queue->parent)) {
        if (queue->buffer) {
            free(queue->buffer);
        }
        free(queue);
    }
    
    return RTOS_OK;
}

/**
 * @brief 发送消息到队列
 */
rtos_result_t rtos_queue_send(rtos_queue_t *queue,
                              const void *item,
                              uint32_t item_size,
                              rtos_timeout_t timeout)
{
    if (queue == NULL || item == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (item_size > queue->item_size) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_task_t *current_task = rtos_task_get_current();
    if (current_task == NULL) {
        return RTOS_ERROR;
    }
    
    /* 检查队列是否有空间 */
    if (!rtos_queue_is_full_internal(queue)) {
        /* 复制数据到队列 */
        uint8_t *dest = queue->buffer + (queue->head * queue->item_size);
        memcpy(dest, item, item_size);
        
        /* 更新队列指针 */
        queue->head = (queue->head + 1) % queue->max_items;
        queue->item_count++;
        queue->send_count++;
        
        /* 唤醒等待接收的任务 */
        rtos_queue_wakeup_recv_waiting_tasks(queue);
        
        return RTOS_OK;
    }
    
    /* 如果超时时间为0，立即返回 */
    if (timeout == RTOS_TIMEOUT_IMMEDIATE) {
        queue->overflow_count++;
        return RTOS_ERROR_TIMEOUT;
    }
    
    /* 添加到发送等待队列 */
    rtos_result_t result = rtos_wait_queue_add(&queue->send_wait_queue, current_task, timeout);
    if (result != RTOS_OK) {
        return result;
    }
    
    queue->send_wait_count++;
    
    /* 阻塞当前任务 */
    current_task->state = RTOS_TASK_STATE_BLOCKED;
    current_task->wait_object = &queue->parent;
    current_task->timeout = timeout;
    
    /* 触发调度 */
    rtos_task_yield();
    
    return RTOS_OK;
}

/**
 * @brief 从队列接收消息
 */
int32_t rtos_queue_receive(rtos_queue_t *queue,
                           void *item,
                           uint32_t item_size,
                           rtos_timeout_t timeout)
{
    if (queue == NULL || item == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (item_size < queue->item_size) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_task_t *current_task = rtos_task_get_current();
    if (current_task == NULL) {
        return RTOS_ERROR;
    }
    
    /* 检查队列是否有数据 */
    if (!rtos_queue_is_empty_internal(queue)) {
        /* 从队列复制数据 */
        uint8_t *src = queue->buffer + (queue->tail * queue->item_size);
        memcpy(item, src, queue->item_size);
        
        /* 更新队列指针 */
        queue->tail = (queue->tail + 1) % queue->max_items;
        queue->item_count--;
        queue->recv_count++;
        
        /* 唤醒等待发送的任务 */
        rtos_queue_wakeup_send_waiting_tasks(queue);
        
        return queue->item_size;
    }
    
    /* 如果超时时间为0，立即返回 */
    if (timeout == RTOS_TIMEOUT_IMMEDIATE) {
        return RTOS_ERROR_TIMEOUT;
    }
    
    /* 添加到接收等待队列 */
    rtos_result_t result = rtos_wait_queue_add(&queue->recv_wait_queue, current_task, timeout);
    if (result != RTOS_OK) {
        return result;
    }
    
    queue->recv_wait_count++;
    
    /* 阻塞当前任务 */
    current_task->state = RTOS_TASK_STATE_BLOCKED;
    current_task->wait_object = &queue->parent;
    current_task->timeout = timeout;
    
    /* 触发调度 */
    rtos_task_yield();
    
    return RTOS_ERROR;
}

/**
 * @brief 尝试发送消息到队列(非阻塞)
 */
rtos_result_t rtos_queue_try_send(rtos_queue_t *queue,
                                  const void *item,
                                  uint32_t item_size)
{
    return rtos_queue_send(queue, item, item_size, RTOS_TIMEOUT_IMMEDIATE);
}

/**
 * @brief 尝试从队列接收消息(非阻塞)
 */
int32_t rtos_queue_try_receive(rtos_queue_t *queue,
                               void *item,
                               uint32_t item_size)
{
    return rtos_queue_receive(queue, item, item_size, RTOS_TIMEOUT_IMMEDIATE);
}

/**
 * @brief 获取队列信息
 */
rtos_result_t rtos_queue_get_info(const rtos_queue_t *queue,
                                  rtos_queue_info_t *info)
{
    if (queue == NULL || info == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    strncpy(info->name, queue->parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    info->item_size = queue->item_size;
    info->max_items = queue->max_items;
    info->item_count = queue->item_count;
    info->send_wait_count = queue->send_wait_count;
    info->recv_wait_count = queue->recv_wait_count;
    info->send_count = queue->send_count;
    info->recv_count = queue->recv_count;
    info->overflow_count = queue->overflow_count;
    
    return RTOS_OK;
}

/**
 * @brief 重置队列
 */
rtos_result_t rtos_queue_reset(rtos_queue_t *queue)
{
    if (queue == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否有任务在等待 */
    if (queue->send_wait_count > 0 || queue->recv_wait_count > 0) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 重置队列状态 */
    queue->item_count = 0;
    queue->head = 0;
    queue->tail = 0;
    
    return RTOS_OK;
}

/**
 * @brief 检查队列是否为空
 */
bool rtos_queue_is_empty(const rtos_queue_t *queue)
{
    if (queue == NULL) {
        return true;
    }
    
    return rtos_queue_is_empty_internal(queue);
}

/**
 * @brief 检查队列是否已满
 */
bool rtos_queue_is_full(const rtos_queue_t *queue)
{
    if (queue == NULL) {
        return true;
    }
    
    return rtos_queue_is_full_internal(queue);
}

/**
 * @brief 获取队列中项目数量
 */
uint32_t rtos_queue_get_count(const rtos_queue_t *queue)
{
    if (queue == NULL) {
        return 0;
    }
    
    return queue->item_count;
}

/**
 * @brief 获取队列剩余空间
 */
uint32_t rtos_queue_get_space(const rtos_queue_t *queue)
{
    if (queue == NULL) {
        return 0;
    }
    
    return queue->max_items - queue->item_count;
}

/* 内部函数实现 */

/**
 * @brief 唤醒等待发送的任务
 */
static void rtos_queue_wakeup_send_waiting_tasks(rtos_queue_t *queue)
{
    if (queue->send_wait_count == 0) {
        return;
    }
    
    /* 获取第一个等待发送的任务 */
    rtos_task_t *waiting_task = rtos_wait_queue_get_first(&queue->send_wait_queue);
    if (waiting_task == NULL) {
        return;
    }
    
    /* 从等待队列移除 */
    rtos_wait_queue_remove(&queue->send_wait_queue, waiting_task);
    queue->send_wait_count--;
    
    /* 恢复任务状态 */
    waiting_task->state = RTOS_TASK_STATE_READY;
    waiting_task->wait_object = NULL;
    waiting_task->timeout = 0;
    
    /* 添加到就绪队列 */
    /* 这里需要调用任务管理模块的函数 */
    /* 暂时使用简单的状态设置 */
}

/**
 * @brief 唤醒等待接收的任务
 */
static void rtos_queue_wakeup_recv_waiting_tasks(rtos_queue_t *queue)
{
    if (queue->recv_wait_count == 0) {
        return;
    }
    
    /* 获取第一个等待接收的任务 */
    rtos_task_t *waiting_task = rtos_wait_queue_get_first(&queue->recv_wait_queue);
    if (waiting_task == NULL) {
        return;
    }
    
    /* 从等待队列移除 */
    rtos_wait_queue_remove(&queue->recv_wait_queue, waiting_task);
    queue->recv_wait_count--;
    
    /* 恢复任务状态 */
    waiting_task->state = RTOS_TASK_STATE_READY;
    waiting_task->wait_object = NULL;
    waiting_task->timeout = 0;
    
    /* 添加到就绪队列 */
    /* 这里需要调用任务管理模块的函数 */
    /* 暂时使用简单的状态设置 */
}

/**
 * @brief 内部检查队列是否为空
 */
static bool rtos_queue_is_empty_internal(const rtos_queue_t *queue)
{
    return (queue->item_count == 0);
}

/**
 * @brief 内部检查队列是否已满
 */
static bool rtos_queue_is_full_internal(const rtos_queue_t *queue)
{
    return (queue->item_count >= queue->max_items);
}
