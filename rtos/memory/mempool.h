/**
 * @file mempool.h
 * @brief RTOS内存池 - 重构后的内存池系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_MEMPOOL_H__
#define __RTOS_MEMPOOL_H__

#include "../core/types.h"
#include "../core/object.h"

/* 内存池结构体 */
typedef struct rtos_memory_pool {
    rtos_object_t          parent;             /* 继承对象基类 */
    
    /* 内存池属性 */
    uint8_t               *buffer;             /* 内存池缓冲区 */
    uint32_t               block_size;         /* 块大小 */
    uint32_t               block_count;        /* 块数量 */
    uint32_t               free_count;         /* 空闲块数量 */
    
    /* 空闲块链表 */
    void                  **free_list;         /* 空闲块链表 */
    uint32_t               free_list_head;     /* 空闲链表头 */
    
    /* 统计信息 */
    uint32_t               alloc_count;        /* 分配次数 */
    uint32_t               free_count_total;   /* 释放次数 */
    uint32_t               peak_usage;         /* 峰值使用量 */
    uint32_t               fragmentation;      /* 碎片化程度 */
} rtos_memory_pool_t;

/* 内存池创建参数 */
typedef struct {
    const char            *name;               /* 内存池名称 */
    uint32_t               block_size;         /* 块大小 */
    uint32_t               block_count;        /* 块数量 */
} rtos_memory_pool_create_params_t;

/* 内存池信息 */
typedef struct {
    char                   name[16];           /* 内存池名称 */
    uint32_t               block_size;         /* 块大小 */
    uint32_t               block_count;        /* 块数量 */
    uint32_t               free_count;         /* 空闲块数量 */
    uint32_t               used_count;         /* 已使用块数量 */
    uint32_t               alloc_count;        /* 分配次数 */
    uint32_t               free_count_total;   /* 释放次数 */
    uint32_t               peak_usage;         /* 峰值使用量 */
    uint32_t               fragmentation;      /* 碎片化程度 */
} rtos_memory_pool_info_t;

/* 内存池API函数声明 */

/**
 * @brief 初始化内存池 - 静态方式
 * @param mempool 内存池指针
 * @param params 创建参数
 * @param buffer 缓冲区指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_pool_init(rtos_memory_pool_t *mempool,
                                    const rtos_memory_pool_create_params_t *params,
                                    void *buffer);

/**
 * @brief 创建内存池 - 动态方式
 * @param params 创建参数
 * @return 内存池指针，失败返回NULL
 */
rtos_memory_pool_t *rtos_memory_pool_create(const rtos_memory_pool_create_params_t *params);

/**
 * @brief 删除内存池
 * @param mempool 内存池指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_pool_delete(rtos_memory_pool_t *mempool);

/**
 * @brief 从内存池分配内存块
 * @param mempool 内存池指针
 * @param timeout 超时时间
 * @return 内存块指针，失败返回NULL
 */
void *rtos_memory_pool_alloc(rtos_memory_pool_t *mempool, rtos_timeout_t timeout);

/**
 * @brief 释放内存块到内存池
 * @param mempool 内存池指针
 * @param block 内存块指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_pool_free(rtos_memory_pool_t *mempool, void *block);

/**
 * @brief 尝试从内存池分配内存块(非阻塞)
 * @param mempool 内存池指针
 * @return 内存块指针，失败返回NULL
 */
void *rtos_memory_pool_try_alloc(rtos_memory_pool_t *mempool);

/**
 * @brief 获取内存池信息
 * @param mempool 内存池指针
 * @param info 信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_pool_get_info(const rtos_memory_pool_t *mempool,
                                        rtos_memory_pool_info_t *info);

/**
 * @brief 重置内存池
 * @param mempool 内存池指针
 * @return 操作结果
 */
rtos_result_t rtos_memory_pool_reset(rtos_memory_pool_t *mempool);

/**
 * @brief 检查内存池是否为空
 * @param mempool 内存池指针
 * @return 是否为空
 */
bool rtos_memory_pool_is_empty(const rtos_memory_pool_t *mempool);

/**
 * @brief 检查内存池是否已满
 * @param mempool 内存池指针
 * @return 是否已满
 */
bool rtos_memory_pool_is_full(const rtos_memory_pool_t *mempool);

/**
 * @brief 获取内存池中空闲块数量
 * @param mempool 内存池指针
 * @return 空闲块数量
 */
uint32_t rtos_memory_pool_get_free_count(const rtos_memory_pool_t *mempool);

/**
 * @brief 获取内存池中已使用块数量
 * @param mempool 内存池指针
 * @return 已使用块数量
 */
uint32_t rtos_memory_pool_get_used_count(const rtos_memory_pool_t *mempool);

/**
 * @brief 获取内存池块大小
 * @param mempool 内存池指针
 * @return 块大小
 */
uint32_t rtos_memory_pool_get_block_size(const rtos_memory_pool_t *mempool);

/**
 * @brief 获取内存池总块数量
 * @param mempool 内存池指针
 * @return 总块数量
 */
uint32_t rtos_memory_pool_get_block_count(const rtos_memory_pool_t *mempool);

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_memory_pool_t rtos_memory_pool_t

#endif /* __RTOS_MEMPOOL_H__ */
