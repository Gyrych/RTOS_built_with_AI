/**
 * @file semaphore.h
 * @brief RTOS信号量 - 重构后的信号量系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_SEMAPHORE_H__
#define __RTOS_SEMAPHORE_H__

#include "../core/types.h"
#include "../core/object.h"

/* 信号量结构体 */
typedef struct rtos_semaphore {
    rtos_object_t          parent;             /* 继承对象基类 */
    
    /* 信号量属性 */
    uint32_t               count;              /* 当前计数值 */
    uint32_t               max_count;          /* 最大计数值 */
    
    /* 等待队列 - 使用统一等待队列设计 */
    rtos_wait_node_t       wait_queue;         /* 等待队列头 */
    uint32_t               wait_count;         /* 等待任务数量 */
    
    /* 统计信息 */
    uint32_t               take_count;         /* 获取次数 */
    uint32_t               give_count;          /* 释放次数 */
} rtos_semaphore_t;

/* 信号量创建参数 */
typedef struct {
    const char            *name;               /* 信号量名称 */
    uint32_t               initial_count;      /* 初始计数值 */
    uint32_t               max_count;          /* 最大计数值 */
} rtos_semaphore_create_params_t;

/* 信号量信息 */
typedef struct {
    char                   name[16];           /* 信号量名称 */
    uint32_t               count;              /* 当前计数值 */
    uint32_t               max_count;          /* 最大计数值 */
    uint32_t               wait_count;         /* 等待任务数量 */
    uint32_t               take_count;         /* 获取次数 */
    uint32_t               give_count;         /* 释放次数 */
} rtos_semaphore_info_t;

/* 信号量API函数声明 */

/**
 * @brief 初始化信号量 - 静态方式
 * @param sem 信号量指针
 * @param params 创建参数
 * @return 操作结果
 */
rtos_result_t rtos_semaphore_init(rtos_semaphore_t *sem,
                                  const rtos_semaphore_create_params_t *params);

/**
 * @brief 创建信号量 - 动态方式
 * @param params 创建参数
 * @return 信号量指针，失败返回NULL
 */
rtos_semaphore_t *rtos_semaphore_create(const rtos_semaphore_create_params_t *params);

/**
 * @brief 删除信号量
 * @param sem 信号量指针
 * @return 操作结果
 */
rtos_result_t rtos_semaphore_delete(rtos_semaphore_t *sem);

/**
 * @brief 获取信号量
 * @param sem 信号量指针
 * @param timeout 超时时间
 * @return 操作结果
 */
rtos_result_t rtos_semaphore_take(rtos_semaphore_t *sem, rtos_timeout_t timeout);

/**
 * @brief 释放信号量
 * @param sem 信号量指针
 * @return 操作结果
 */
rtos_result_t rtos_semaphore_give(rtos_semaphore_t *sem);

/**
 * @brief 尝试获取信号量(非阻塞)
 * @param sem 信号量指针
 * @return 操作结果
 */
rtos_result_t rtos_semaphore_try_take(rtos_semaphore_t *sem);

/**
 * @brief 获取信号量信息
 * @param sem 信号量指针
 * @param info 信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_semaphore_get_info(const rtos_semaphore_t *sem,
                                      rtos_semaphore_info_t *info);

/**
 * @brief 重置信号量
 * @param sem 信号量指针
 * @param count 新计数值
 * @return 操作结果
 */
rtos_result_t rtos_semaphore_reset(rtos_semaphore_t *sem, uint32_t count);

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_sem_init rtos_semaphore_init
#define rtos_sem_take rtos_semaphore_take
#define rtos_sem_release rtos_semaphore_give

#endif /* __RTOS_SEMAPHORE_H__ */
