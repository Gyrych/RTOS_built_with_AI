/**
 * @file rtos_semaphore.h
 * @brief RTOS信号量模块头文件 - 基于IPC对象实现
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_SEMAPHORE_H__
#define __RTOS_SEMAPHORE_H__

#include "rtos_object.h"
#include "rtos_task.h"

/**
 * @brief 信号量控制块结构体
 * 继承自IPC对象基类
 */
typedef struct rtos_semaphore {
    struct rtos_ipc_object parent;              /* 继承IPC对象基类 */
    uint16_t               value;               /* 信号量当前值 */
    uint16_t               reserved;            /* 保留字段，用于对齐 */
} rtos_semaphore_t;

/* 信号量API函数声明 */

/**
 * @brief 初始化信号量 - 静态方式
 * @param sem 信号量控制块指针
 * @param name 信号量名称
 * @param value 初始值
 * @param flag 标志(暂未使用)
 * @return 结果码
 */
rtos_result_t rtos_sem_init(rtos_semaphore_t *sem,
                           const char       *name,
                           uint32_t          value,
                           uint8_t           flag);

/**
 * @brief 创建信号量 - 动态方式
 * @param name 信号量名称
 * @param value 初始值
 * @param flag 标志(暂未使用)
 * @return 信号量指针
 */
rtos_semaphore_t *rtos_sem_create(const char *name,
                                 uint32_t    value,
                                 uint8_t     flag);

/**
 * @brief 分离信号量
 * @param sem 信号量指针
 * @return 结果码
 */
rtos_result_t rtos_sem_detach(rtos_semaphore_t *sem);

/**
 * @brief 删除信号量
 * @param sem 信号量指针
 * @return 结果码
 */
rtos_result_t rtos_sem_delete(rtos_semaphore_t *sem);

/**
 * @brief 获取信号量
 * @param sem 信号量指针
 * @param time 超时时间(纳秒)，0表示不等待，RTOS_WAITING_FOREVER表示永久等待
 * @return 结果码
 */
rtos_result_t rtos_sem_take(rtos_semaphore_t *sem, rtos_time_ns_t time);

/**
 * @brief 尝试获取信号量(不阻塞)
 * @param sem 信号量指针
 * @return 结果码
 */
rtos_result_t rtos_sem_trytake(rtos_semaphore_t *sem);

/**
 * @brief 释放信号量
 * @param sem 信号量指针
 * @return 结果码
 */
rtos_result_t rtos_sem_release(rtos_semaphore_t *sem);

/**
 * @brief 控制信号量
 * @param sem 信号量指针
 * @param cmd 控制命令
 * @param arg 参数
 * @return 结果码
 */
rtos_result_t rtos_sem_control(rtos_semaphore_t *sem, int cmd, void *arg);

/**
 * @brief 获取信号量当前值
 * @param sem 信号量指针
 * @return 当前值，错误时返回-1
 */
int rtos_sem_get_value(rtos_semaphore_t *sem);

/**
 * @brief 查找信号量
 * @param name 信号量名称
 * @return 信号量指针
 */
rtos_semaphore_t *rtos_sem_find(const char *name);

/* 信号量标志定义 */
#define RTOS_SEM_FLAG_FIFO         0x00    /* FIFO方式等待 */
#define RTOS_SEM_FLAG_PRIO         0x01    /* 优先级方式等待 */

/* 信号量控制命令 */
#define RTOS_SEM_CTRL_RESET        0x01    /* 重置信号量 */
#define RTOS_SEM_CTRL_SET_VALUE    0x02    /* 设置信号量值 */
#define RTOS_SEM_CTRL_GET_VALUE    0x03    /* 获取信号量值 */

/* 信号量超时定义 */
#define RTOS_WAITING_NO            0       /* 不等待 */
#define RTOS_WAITING_FOREVER       UINT64_MAX  /* 永久等待 */

/* 信号量信息结构 */
typedef struct {
    char        name[16];                   /* 信号量名称 */
    uint32_t    value;                      /* 当前值 */
    uint32_t    suspend_thread_count;       /* 挂起线程数量 */
} rtos_sem_info_t;

/**
 * @brief 获取信号量信息
 * @param sem 信号量指针
 * @param info 信息结构体指针
 * @return 结果码
 */
rtos_result_t rtos_sem_get_info(rtos_semaphore_t *sem, rtos_sem_info_t *info);

/* 内联辅助函数 */

/**
 * @brief 检查信号量有效性
 * @param sem 信号量指针
 * @return 是否有效
 */
static inline bool rtos_sem_is_valid(rtos_semaphore_t *sem)
{
    return sem && rtos_object_is_valid(&(sem->parent.parent), RTOS_OBJECT_CLASS_SEMAPHORE);
}

/**
 * @brief 获取信号量名称
 * @param sem 信号量指针
 * @return 信号量名称
 */
static inline const char *rtos_sem_get_name(rtos_semaphore_t *sem)
{
    if (!rtos_sem_is_valid(sem)) {
        return NULL;
    }
    return sem->parent.parent.name;
}

/**
 * @brief 检查信号量是否有挂起的线程
 * @param sem 信号量指针
 * @return 是否有挂起线程
 */
static inline bool rtos_sem_has_suspend_thread(rtos_semaphore_t *sem)
{
    if (!rtos_sem_is_valid(sem)) {
        return false;
    }
    return sem->parent.suspend_thread != NULL;
}

/* 兼容性宏定义(保持与旧版本API的兼容) */
#define rtos_semaphore_create(name, initial_count, max_count) \
    rtos_sem_create(name, initial_count, 0)

#define rtos_semaphore_take(sem, timeout_us) \
    rtos_sem_take(sem, (rtos_time_ns_t)(timeout_us) * 1000)

#define rtos_semaphore_give(sem) \
    rtos_sem_release(sem)

#define rtos_semaphore_delete(sem) \
    rtos_sem_delete(sem)

#endif /* __RTOS_SEMAPHORE_H__ */