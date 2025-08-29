/**
 * @file rtos_mempool.h
 * @brief RTOS内存池模块头文件 - 基于内核对象实现
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_MEMPOOL_H__
#define __RTOS_MEMPOOL_H__

#include "rtos_object.h"

/**
 * @brief 内存池控制块结构体
 * 继承自IPC对象基类
 */
typedef struct rtos_mempool {
    struct rtos_ipc_object parent;              /* 继承IPC对象基类 */
    
    void     *start_address;                    /* 内存池起始地址 */
    uint32_t  size;                             /* 内存池大小 */
    uint32_t  block_size;                       /* 内存块大小 */
    void     *block_list;                       /* 空闲内存块链表 */
    uint32_t  block_total_count;                /* 内存块总数 */
    uint32_t  block_free_count;                 /* 空闲内存块数 */
    
    struct rtos_task *suspend_thread;           /* 挂起的线程链表 */
    uint32_t          suspend_thread_count;     /* 挂起线程数量 */
} rtos_mempool_t;

/* 内存池API函数声明 */
rtos_result_t rtos_mp_init(rtos_mempool_t *mp, const char *name, void *start, 
                          uint32_t size, uint32_t block_size);
rtos_mempool_t *rtos_mp_create(const char *name, uint32_t block_count, uint32_t block_size);
rtos_result_t rtos_mp_delete(rtos_mempool_t *mp);
void *rtos_mp_alloc(rtos_mempool_t *mp, rtos_time_ns_t time);
void rtos_mp_free(void *block);

#endif /* __RTOS_MEMPOOL_H__ */