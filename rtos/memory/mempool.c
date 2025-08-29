/**
 * @file mempool.c
 * @brief RTOS内存池实现 - 重构后的内存池系统
 * @author Assistant
 * @date 2024
 */

#include "mempool.h"
#include "../core/object.h"
#include "../core/types.h"
#include <string.h>

/**
 * @brief 初始化内存池 - 静态方式
 */
rtos_result_t rtos_memory_pool_init(rtos_memory_pool_t *mempool,
                                    const rtos_memory_pool_create_params_t *params,
                                    void *buffer)
{
    uint32_t i;
    uint8_t *block_ptr;
    
    if (!mempool || !params || !buffer) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (params->block_size == 0 || params->block_count == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化对象基类 */
    rtos_object_init(&mempool->parent, RTOS_OBJECT_TYPE_MEMORY_POOL, params->name);
    
    /* 初始化内存池属性 */
    mempool->buffer = (uint8_t *)buffer;
    mempool->block_size = params->block_size;
    mempool->block_count = params->block_count;
    mempool->free_count = params->block_count;
    
    /* 分配空闲链表数组 */
    mempool->free_list = (void **)rtos_malloc(params->block_count * sizeof(void *));
    if (!mempool->free_list) {
        return RTOS_ERROR_NO_MEMORY;
    }
    
    /* 初始化空闲链表 */
    mempool->free_list_head = 0;
    for (i = 0; i < params->block_count; i++) {
        block_ptr = mempool->buffer + (i * params->block_size);
        mempool->free_list[i] = (void *)block_ptr;
    }
    
    /* 初始化统计信息 */
    mempool->alloc_count = 0;
    mempool->free_count_total = 0;
    mempool->peak_usage = 0;
    mempool->fragmentation = 0;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 创建内存池 - 动态方式
 */
rtos_memory_pool_t *rtos_memory_pool_create(const rtos_memory_pool_create_params_t *params)
{
    rtos_memory_pool_t *mempool;
    void *buffer;
    
    if (!params) {
        return NULL;
    }
    
    if (params->block_size == 0 || params->block_count == 0) {
        return NULL;
    }
    
    /* 分配内存池结构体 */
    mempool = (rtos_memory_pool_t *)rtos_malloc(sizeof(rtos_memory_pool_t));
    if (!mempool) {
        return NULL;
    }
    
    /* 分配内存池缓冲区 */
    buffer = rtos_malloc(params->block_size * params->block_count);
    if (!buffer) {
        rtos_free(mempool);
        return NULL;
    }
    
    /* 初始化内存池 */
    if (rtos_memory_pool_init(mempool, params, buffer) != RTOS_SUCCESS) {
        rtos_free(buffer);
        rtos_free(mempool);
        return NULL;
    }
    
    return mempool;
}

/**
 * @brief 删除内存池
 */
rtos_result_t rtos_memory_pool_delete(rtos_memory_pool_t *mempool)
{
    if (!mempool) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 释放空闲链表数组 */
    if (mempool->free_list) {
        rtos_free(mempool->free_list);
        mempool->free_list = NULL;
    }
    
    /* 释放内存池缓冲区 */
    if (mempool->buffer) {
        rtos_free(mempool->buffer);
        mempool->buffer = NULL;
    }
    
    /* 释放内存池结构体 */
    rtos_free(mempool);
    
    return RTOS_SUCCESS;
}

/**
 * @brief 从内存池分配内存块
 */
void *rtos_memory_pool_alloc(rtos_memory_pool_t *mempool, rtos_timeout_t timeout)
{
    void *block = NULL;
    rtos_time_ns_t start_time, current_time;
    
    if (!mempool) {
        return NULL;
    }
    
    /* 记录开始时间 */
    start_time = rtos_get_system_time();
    
    /* 尝试分配，直到成功或超时 */
    while (!block) {
        /* 尝试非阻塞分配 */
        block = rtos_memory_pool_try_alloc(mempool);
        if (block) {
            break;
        }
        
        /* 检查是否超时 */
        if (timeout != RTOS_WAIT_FOREVER) {
            current_time = rtos_get_system_time();
            if ((current_time - start_time) >= timeout) {
                break;
            }
        }
        
        /* 让出CPU时间片 */
        rtos_task_yield();
    }
    
    return block;
}

/**
 * @brief 释放内存块到内存池
 */
rtos_result_t rtos_memory_pool_free(rtos_memory_pool_t *mempool, void *block)
{
    uint32_t i;
    
    if (!mempool || !block) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查块是否属于此内存池 */
    if ((uint8_t *)block < mempool->buffer ||
        (uint8_t *)block >= mempool->buffer + (mempool->block_count * mempool->block_size)) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查块是否已对齐 */
    if (((uintptr_t)block - (uintptr_t)mempool->buffer) % mempool->block_size != 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查块是否已经在空闲链表中 */
    for (i = 0; i < mempool->block_count; i++) {
        if (mempool->free_list[i] == block) {
            return RTOS_ERROR_ALREADY_EXISTS;
        }
    }
    
    /* 将块添加到空闲链表 */
    if (mempool->free_count < mempool->block_count) {
        mempool->free_list[mempool->free_count] = block;
        mempool->free_count++;
        mempool->free_count_total++;
        
        /* 更新峰值使用量 */
        uint32_t used_count = mempool->block_count - mempool->free_count;
        if (used_count > mempool->peak_usage) {
            mempool->peak_usage = used_count;
        }
        
        /* 计算碎片化程度 */
        mempool->fragmentation = (mempool->free_count * 100) / mempool->block_count;
        
        return RTOS_SUCCESS;
    }
    
    return RTOS_ERROR_NO_MEMORY;
}

/**
 * @brief 尝试从内存池分配内存块(非阻塞)
 */
void *rtos_memory_pool_try_alloc(rtos_memory_pool_t *mempool)
{
    void *block;
    
    if (!mempool) {
        return NULL;
    }
    
    /* 检查是否有空闲块 */
    if (mempool->free_count == 0) {
        return NULL;
    }
    
    /* 从空闲链表头部取出一个块 */
    mempool->free_count--;
    block = mempool->free_list[mempool->free_count];
    
    /* 更新统计信息 */
    mempool->alloc_count++;
    
    return block;
}

/**
 * @brief 获取内存池信息
 */
rtos_result_t rtos_memory_pool_get_info(const rtos_memory_pool_t *mempool,
                                        rtos_memory_pool_info_t *info)
{
    if (!mempool || !info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 复制基本信息 */
    strncpy(info->name, mempool->parent.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    info->block_size = mempool->block_size;
    info->block_count = mempool->block_count;
    info->free_count = mempool->free_count;
    info->used_count = mempool->block_count - mempool->free_count;
    info->alloc_count = mempool->alloc_count;
    info->free_count_total = mempool->free_count_total;
    info->peak_usage = mempool->peak_usage;
    info->fragmentation = mempool->fragmentation;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 重置内存池
 */
rtos_result_t rtos_memory_pool_reset(rtos_memory_pool_t *mempool)
{
    uint32_t i;
    uint8_t *block_ptr;
    
    if (!mempool) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 重新初始化空闲链表 */
    mempool->free_count = mempool->block_count;
    for (i = 0; i < mempool->block_count; i++) {
        block_ptr = mempool->buffer + (i * mempool->block_size);
        mempool->free_list[i] = (void *)block_ptr;
    }
    
    /* 重置统计信息 */
    mempool->alloc_count = 0;
    mempool->free_count_total = 0;
    mempool->peak_usage = 0;
    mempool->fragmentation = 0;
    
    return RTOS_SUCCESS;
}

/**
 * @brief 检查内存池是否为空
 */
bool rtos_memory_pool_is_empty(const rtos_memory_pool_t *mempool)
{
    if (!mempool) {
        return true;
    }
    
    return (mempool->free_count == 0);
}

/**
 * @brief 检查内存池是否已满
 */
bool rtos_memory_pool_is_full(const rtos_memory_pool_t *mempool)
{
    if (!mempool) {
        return true;
    }
    
    return (mempool->free_count == mempool->block_count);
}

/**
 * @brief 获取内存池中空闲块数量
 */
uint32_t rtos_memory_pool_get_free_count(const rtos_memory_pool_t *mempool)
{
    if (!mempool) {
        return 0;
    }
    
    return mempool->free_count;
}

/**
 * @brief 获取内存池中已使用块数量
 */
uint32_t rtos_memory_pool_get_used_count(const rtos_memory_pool_t *mempool)
{
    if (!mempool) {
        return 0;
    }
    
    return mempool->block_count - mempool->free_count;
}

/**
 * @brief 获取内存池块大小
 */
uint32_t rtos_memory_pool_get_block_size(const rtos_memory_pool_t *mempool)
{
    if (!mempool) {
        return 0;
    }
    
    return mempool->block_size;
}

/**
 * @brief 获取内存池总块数量
 */
uint32_t rtos_memory_pool_get_block_count(const rtos_memory_pool_t *mempool)
{
    if (!mempool) {
        return 0;
    }
    
    return mempool->block_count;
}

/**
 * @brief 获取内存池总大小
 */
uint32_t rtos_memory_pool_get_total_size(const rtos_memory_pool_t *mempool)
{
    if (!mempool) {
        return 0;
    }
    
    return mempool->block_size * mempool->block_count;
}

/**
 * @brief 获取内存池使用率
 */
uint32_t rtos_memory_pool_get_usage_percentage(const rtos_memory_pool_t *mempool)
{
    if (!mempool || mempool->block_count == 0) {
        return 0;
    }
    
    return ((mempool->block_count - mempool->free_count) * 100) / mempool->block_count;
}

/**
 * @brief 内存池完整性检查
 */
rtos_result_t rtos_memory_pool_integrity_check(const rtos_memory_pool_t *mempool)
{
    uint32_t i, j;
    bool found;
    
    if (!mempool) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 检查空闲链表中的块是否有效 */
    for (i = 0; i < mempool->free_count; i++) {
        void *block = mempool->free_list[i];
        
        /* 检查块指针是否在有效范围内 */
        if ((uint8_t *)block < mempool->buffer ||
            (uint8_t *)block >= mempool->buffer + (mempool->block_count * mempool->block_size)) {
            return RTOS_ERROR_CORRUPTED;
        }
        
        /* 检查块是否对齐 */
        if (((uintptr_t)block - (uintptr_t)mempool->buffer) % mempool->block_size != 0) {
            return RTOS_ERROR_CORRUPTED;
        }
        
        /* 检查是否有重复的块 */
        found = false;
        for (j = 0; j < i; j++) {
            if (mempool->free_list[j] == block) {
                return RTOS_ERROR_CORRUPTED;
            }
        }
    }
    
    return RTOS_SUCCESS;
}
