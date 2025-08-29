/**
 * @file event.c
 * @brief RTOS事件组实现 - 重构后的事件组系统
 * @author Assistant
 * @date 2024
 */

#include "event.h"
#include "../core/object.h"
#include "../core/types.h"
#include "../task/task.h"
#include <string.h>

/**
 * @brief 初始化事件组 - 静态方式
 */
rtos_result_t rtos_event_group_init(rtos_event_group_t *event_group,
                                    const rtos_event_group_create_params_t *params)
{
    if (!event_group || !params) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化对象基类 */
    rtos_object_init(&event_group->parent, RTOS_OBJECT_TYPE_EVENT_GROUP, params->name);
    
    /* 初始化事件组属性 */
    event_group->bits = params->initial_bits;
    event_group->bits_set = params->initial_bits;
    event_group->bits_clear = ~params->initial_bits;
    
    /* 初始化等待队列 */
    rtos_wait_queue_init(&event_group->wait_queue);
    event_group->wait_count = 0;
    
    /* 初始化统计信息 */
    event_group->set_count = 0;
    event_group->clear_count = 0;
    event_group->wait_count_total = 0;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 创建事件组 - 动态方式
 */
rtos_event_group_t *rtos_event_group_create(const rtos_event_group_create_params_t *params)
{
    rtos_event_group_t *event_group;
    
    if (!params) {
        return NULL;
    }
    
    /* 分配内存 */
    event_group = (rtos_event_group_t *)rtos_malloc(sizeof(rtos_event_group_t));
    if (!event_group) {
        return NULL;
    }
    
    /* 初始化事件组 */
    if (rtos_event_group_init(event_group, params) != RTOS_SUCCESS) {
        rtos_free(event_group);
        return NULL;
    }
    
    return event_group;
}

/**
 * @brief 删除事件组
 */
rtos_result_t rtos_event_group_delete(rtos_event_group_t *event_group)
{
    if (!event_group) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 唤醒所有等待的任务 */
    rtos_wait_queue_wake_all(&event_group->wait_queue, RTOS_ERROR_DELETED);
    
    /* 释放内存 */
    rtos_free(event_group);
    
    return RTOS_SUCCESS;
}

/**
 * @brief 设置事件位
 */
rtos_event_bits_t rtos_event_group_set_bits(rtos_event_group_t *event_group,
                                            rtos_event_bits_t bits)
{
    rtos_event_bits_t new_bits;
    rtos_wait_node_t *wait_node;
    rtos_task_t *task;
    bool should_wake = false;
    
    if (!event_group) {
        return 0;
    }
    
    /* 设置事件位 */
    new_bits = event_group->bits | bits;
    event_group->bits = new_bits;
    event_group->bits_set |= bits;
    event_group->bits_clear &= ~bits;
    event_group->set_count++;
    
    /* 检查是否有等待的任务需要唤醒 */
    wait_node = event_group->wait_queue.next;
    while (wait_node != &event_group->wait_queue) {
        task = rtos_wait_node_get_task(wait_node);
        if (task) {
            /* 检查是否满足等待条件 */
            rtos_event_bits_t wait_bits = (rtos_event_bits_t)rtos_wait_node_get_data(wait_node);
            bool wait_for_all = (rtos_wait_node_get_flags(wait_node) & RTOS_WAIT_FLAG_ALL) != 0;
            
            bool condition_met;
            if (wait_for_all) {
                /* 等待所有位 */
                condition_met = ((new_bits & wait_bits) == wait_bits);
            } else {
                /* 等待任意位 */
                condition_met = ((new_bits & wait_bits) != 0);
            }
            
            if (condition_met) {
                should_wake = true;
                break;
            }
        }
        wait_node = wait_node->next;
    }
    
    /* 如果有任务需要唤醒，唤醒所有满足条件的任务 */
    if (should_wake) {
        rtos_wait_queue_wake_all(&event_group->wait_queue, RTOS_SUCCESS);
    }
    
    return new_bits;
}

/**
 * @brief 清除事件位
 */
rtos_event_bits_t rtos_event_group_clear_bits(rtos_event_group_t *event_group,
                                              rtos_event_bits_t bits)
{
    rtos_event_bits_t new_bits;
    
    if (!event_group) {
        return 0;
    }
    
    /* 清除事件位 */
    new_bits = event_group->bits & ~bits;
    event_group->bits = new_bits;
    event_group->bits_set &= ~bits;
    event_group->bits_clear |= bits;
    event_group->clear_count++;
    
    return new_bits;
}

/**
 * @brief 等待事件位
 */
rtos_event_bits_t rtos_event_group_wait_bits(rtos_event_group_t *event_group,
                                             rtos_event_bits_t bits,
                                             bool clear_on_exit,
                                             bool wait_for_all,
                                             rtos_timeout_t timeout)
{
    rtos_event_bits_t current_bits;
    rtos_event_bits_t wait_result = 0;
    rtos_wait_node_t wait_node;
    rtos_result_t result;
    
    if (!event_group) {
        return 0;
    }
    
    if (bits == 0) {
        return 0;
    }
    
    /* 检查当前事件位是否已满足条件 */
    current_bits = event_group->bits;
    bool condition_met;
    if (wait_for_all) {
        condition_met = ((current_bits & bits) == bits);
    } else {
        condition_met = ((current_bits & bits) != 0);
    }
    
    if (condition_met) {
        /* 条件已满足，直接返回 */
        wait_result = current_bits & bits;
        if (clear_on_exit) {
            rtos_event_group_clear_bits(event_group, wait_result);
        }
        return wait_result;
    }
    
    /* 条件不满足，需要等待 */
    event_group->wait_count++;
    event_group->wait_count_total++;
    
    /* 准备等待节点 */
    uint32_t flags = 0;
    if (wait_for_all) {
        flags |= RTOS_WAIT_FLAG_ALL;
    }
    if (clear_on_exit) {
        flags |= RTOS_WAIT_FLAG_CLEAR_ON_EXIT;
    }
    
    rtos_wait_node_init(&wait_node, rtos_task_get_current(), (void *)(uintptr_t)bits, flags);
    
    /* 添加到等待队列 */
    rtos_wait_queue_add(&event_group->wait_queue, &wait_node);
    
    /* 等待事件 */
    result = rtos_wait_node_wait(&wait_node, timeout);
    
    /* 从等待队列中移除 */
    rtos_wait_queue_remove(&wait_node);
    
    if (result == RTOS_SUCCESS) {
        /* 等待成功，获取满足条件的事件位 */
        current_bits = event_group->bits;
        if (wait_for_all) {
            wait_result = current_bits & bits;
        } else {
            wait_result = current_bits & bits;
        }
        
        /* 如果需要清除，清除已满足的事件位 */
        if (clear_on_exit && wait_result != 0) {
            rtos_event_group_clear_bits(event_group, wait_result);
        }
    }
    
    event_group->wait_count--;
    
    return wait_result;
}

/**
 * @brief 获取当前事件位
 */
rtos_event_bits_t rtos_event_group_get_bits(const rtos_event_group_t *event_group)
{
    if (!event_group) {
        return 0;
    }
    
    return event_group->bits;
}

/**
 * @brief 尝试设置事件位(非阻塞)
 */
rtos_event_bits_t rtos_event_group_try_set_bits(rtos_event_group_t *event_group,
                                                rtos_event_bits_t bits)
{
    return rtos_event_group_set_bits(event_group, bits);
}

/**
 * @brief 尝试清除事件位(非阻塞)
 */
rtos_event_bits_t rtos_event_group_try_clear_bits(rtos_event_group_t *event_group,
                                                  rtos_event_bits_t bits)
{
    return rtos_event_group_clear_bits(event_group, bits);
}

/**
 * @brief 获取事件组信息
 */
rtos_result_t rtos_event_group_get_info(const rtos_event_group_t *event_group,
                                        rtos_event_group_info_t *info)
{
    if (!event_group || !info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 复制基本信息 */
    strncpy(info->name, event_group->parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    info->bits = event_group->bits;
    info->wait_count = event_group->wait_count;
    info->set_count = event_group->set_count;
    info->clear_count = event_group->clear_count;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 重置事件组
 */
rtos_result_t rtos_event_group_reset(rtos_event_group_t *event_group,
                                     rtos_event_bits_t bits)
{
    if (!event_group) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 重置事件位 */
    event_group->bits = bits;
    event_group->bits_set = bits;
    event_group->bits_clear = ~bits;
    
    /* 重置统计信息 */
    event_group->set_count = 0;
    event_group->clear_count = 0;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 同步事件位
 */
rtos_result_t rtos_event_group_sync(rtos_event_group_t *event_group,
                                    rtos_event_bits_t bits)
{
    rtos_event_bits_t current_bits;
    rtos_event_bits_t sync_bits;
    
    if (!event_group) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (bits == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 获取当前事件位 */
    current_bits = event_group->bits;
    
    /* 计算需要同步的事件位 */
    sync_bits = bits & ~current_bits;
    
    if (sync_bits != 0) {
        /* 设置需要同步的事件位 */
        rtos_event_group_set_bits(event_group, sync_bits);
    }
    
    return RTOS_SUCCESS;
}

/**
 * @brief 检查事件位是否已设置
 */
bool rtos_event_group_is_bits_set(const rtos_event_group_t *event_group,
                                  rtos_event_bits_t bits)
{
    if (!event_group) {
        return false;
    }
    
    return ((event_group->bits & bits) == bits);
}

/**
 * @brief 检查事件位是否已清除
 */
bool rtos_event_group_is_bits_clear(const rtos_event_group_t *event_group,
                                    rtos_event_bits_t bits)
{
    if (!event_group) {
        return true;
    }
    
    return ((event_group->bits & bits) == 0);
}

/**
 * @brief 获取事件组等待任务数量
 */
uint32_t rtos_event_group_get_wait_count(const rtos_event_group_t *event_group)
{
    if (!event_group) {
        return 0;
    }
    
    return event_group->wait_count;
}

/**
 * @brief 获取事件组统计信息
 */
rtos_result_t rtos_event_group_get_stats(const rtos_event_group_t *event_group,
                                         uint32_t *set_count,
                                         uint32_t *clear_count,
                                         uint32_t *wait_count_total)
{
    if (!event_group) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (set_count) {
        *set_count = event_group->set_count;
    }
    
    if (clear_count) {
        *clear_count = event_group->clear_count;
    }
    
    if (wait_count_total) {
        *wait_count_total = event_group->wait_count_total;
    }
    
    return RTOS_SUCCESS;
}

/**
 * @brief 事件组位操作宏的辅助函数
 */
rtos_event_bits_t rtos_event_group_bit_set(rtos_event_group_t *event_group, uint8_t bit_num)
{
    if (bit_num >= 32) {
        return 0;
    }
    
    rtos_event_bits_t bit = 1UL << bit_num;
    return rtos_event_group_set_bits(event_group, bit);
}

rtos_event_bits_t rtos_event_group_bit_clear(rtos_event_group_t *event_group, uint8_t bit_num)
{
    if (bit_num >= 32) {
        return 0;
    }
    
    rtos_event_bits_t bit = 1UL << bit_num;
    return rtos_event_group_clear_bits(event_group, bit);
}

bool rtos_event_group_bit_is_set(const rtos_event_group_t *event_group, uint8_t bit_num)
{
    if (bit_num >= 32) {
        return false;
    }
    
    rtos_event_bits_t bit = 1UL << bit_num;
    return rtos_event_group_is_bits_set(event_group, bit);
}

bool rtos_event_group_bit_is_clear(const rtos_event_group_t *event_group, uint8_t bit_num)
{
    if (bit_num >= 32) {
        return true;
    }
    
    rtos_event_bits_t bit = 1UL << bit_num;
    return rtos_event_group_is_bits_clear(event_group, bit);
}
