/**
 * @file rtos_mutex.h
 * @brief RTOS互斥量模块头文件 - 基于IPC对象实现
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_MUTEX_H__
#define __RTOS_MUTEX_H__

#include "rtos_object.h"
#include "rtos_task.h"

/**
 * @brief 互斥量控制块结构体
 * 继承自IPC对象基类
 */
typedef struct rtos_mutex {
    struct rtos_ipc_object parent;              /* 继承IPC对象基类 */
    uint16_t               value;               /* 互斥量值(0表示被锁定，1表示可用) */
    uint16_t               recursive_count;     /* 递归锁定计数 */
    struct rtos_task      *owner;               /* 持有互斥量的任务 */
    uint8_t                original_priority;   /* 持有者任务的原始优先级 */
    uint8_t                hold_count;          /* 持有计数(用于嵌套锁定) */
    uint8_t                flag;                /* 互斥量标志 */
    uint8_t                reserved;            /* 保留字段 */
} rtos_mutex_t;

/* 互斥量API函数声明 */

/**
 * @brief 初始化互斥量 - 静态方式
 * @param mutex 互斥量控制块指针
 * @param name 互斥量名称
 * @param flag 标志
 * @return 结果码
 */
rtos_result_t rtos_mutex_init(rtos_mutex_t *mutex,
                             const char   *name,
                             uint8_t       flag);

/**
 * @brief 创建互斥量 - 动态方式
 * @param name 互斥量名称
 * @param flag 标志
 * @return 互斥量指针
 */
rtos_mutex_t *rtos_mutex_create(const char *name, uint8_t flag);

/**
 * @brief 分离互斥量
 * @param mutex 互斥量指针
 * @return 结果码
 */
rtos_result_t rtos_mutex_detach(rtos_mutex_t *mutex);

/**
 * @brief 删除互斥量
 * @param mutex 互斥量指针
 * @return 结果码
 */
rtos_result_t rtos_mutex_delete(rtos_mutex_t *mutex);

/**
 * @brief 获取互斥量
 * @param mutex 互斥量指针
 * @param time 超时时间(纳秒)，0表示不等待，RTOS_WAITING_FOREVER表示永久等待
 * @return 结果码
 */
rtos_result_t rtos_mutex_take(rtos_mutex_t *mutex, rtos_time_ns_t time);

/**
 * @brief 尝试获取互斥量(不阻塞)
 * @param mutex 互斥量指针
 * @return 结果码
 */
rtos_result_t rtos_mutex_trytake(rtos_mutex_t *mutex);

/**
 * @brief 释放互斥量
 * @param mutex 互斥量指针
 * @return 结果码
 */
rtos_result_t rtos_mutex_release(rtos_mutex_t *mutex);

/**
 * @brief 控制互斥量
 * @param mutex 互斥量指针
 * @param cmd 控制命令
 * @param arg 参数
 * @return 结果码
 */
rtos_result_t rtos_mutex_control(rtos_mutex_t *mutex, int cmd, void *arg);

/**
 * @brief 获取互斥量持有者
 * @param mutex 互斥量指针
 * @return 持有者任务指针
 */
rtos_task_t *rtos_mutex_get_owner(rtos_mutex_t *mutex);

/**
 * @brief 查找互斥量
 * @param name 互斥量名称
 * @return 互斥量指针
 */
rtos_mutex_t *rtos_mutex_find(const char *name);

/* 互斥量标志定义 */
#define RTOS_MUTEX_FLAG_FIFO           0x00    /* FIFO方式等待 */
#define RTOS_MUTEX_FLAG_PRIO           0x01    /* 优先级方式等待 */
#define RTOS_MUTEX_FLAG_INHERIT        0x02    /* 支持优先级继承 */
#define RTOS_MUTEX_FLAG_PROTECT        0x03    /* 支持优先级保护 */

/* 互斥量控制命令 */
#define RTOS_MUTEX_CTRL_RESET          0x01    /* 重置互斥量 */
#define RTOS_MUTEX_CTRL_GET_OWNER      0x02    /* 获取持有者 */
#define RTOS_MUTEX_CTRL_SET_CEILING    0x03    /* 设置优先级天花板 */

/* 互斥量超时定义 */
#define RTOS_WAITING_NO                0       /* 不等待 */
#define RTOS_WAITING_FOREVER           UINT64_MAX  /* 永久等待 */

/* 互斥量信息结构 */
typedef struct {
    char        name[16];                       /* 互斥量名称 */
    uint8_t     value;                          /* 当前值 */
    uint8_t     hold_count;                     /* 持有计数 */
    char        owner_name[16];                 /* 持有者名称 */
    uint8_t     owner_priority;                 /* 持有者优先级 */
    uint32_t    suspend_thread_count;           /* 挂起线程数量 */
} rtos_mutex_info_t;

/**
 * @brief 获取互斥量信息
 * @param mutex 互斥量指针
 * @param info 信息结构体指针
 * @return 结果码
 */
rtos_result_t rtos_mutex_get_info(rtos_mutex_t *mutex, rtos_mutex_info_t *info);

/* 内联辅助函数 */

/**
 * @brief 检查互斥量有效性
 * @param mutex 互斥量指针
 * @return 是否有效
 */
static inline bool rtos_mutex_is_valid(rtos_mutex_t *mutex)
{
    return mutex && rtos_object_is_valid(&(mutex->parent.parent), RTOS_OBJECT_CLASS_MUTEX);
}

/**
 * @brief 获取互斥量名称
 * @param mutex 互斥量指针
 * @return 互斥量名称
 */
static inline const char *rtos_mutex_get_name(rtos_mutex_t *mutex)
{
    if (!rtos_mutex_is_valid(mutex)) {
        return NULL;
    }
    return mutex->parent.parent.name;
}

/**
 * @brief 检查互斥量是否被锁定
 * @param mutex 互斥量指针
 * @return 是否被锁定
 */
static inline bool rtos_mutex_is_locked(rtos_mutex_t *mutex)
{
    if (!rtos_mutex_is_valid(mutex)) {
        return false;
    }
    return mutex->value == 0;
}

/**
 * @brief 检查当前任务是否持有互斥量
 * @param mutex 互斥量指针
 * @return 是否持有
 */
static inline bool rtos_mutex_is_owned_by_current_task(rtos_mutex_t *mutex)
{
    if (!rtos_mutex_is_valid(mutex)) {
        return false;
    }
    return mutex->owner == rtos_task_self();
}

/**
 * @brief 检查互斥量是否有挂起的线程
 * @param mutex 互斥量指针
 * @return 是否有挂起线程
 */
static inline bool rtos_mutex_has_suspend_thread(rtos_mutex_t *mutex)
{
    if (!rtos_mutex_is_valid(mutex)) {
        return false;
    }
    return mutex->parent.suspend_thread != NULL;
}

/* 兼容性宏定义(保持与旧版本API的兼容) */
#define rtos_mutex_lock(mutex, timeout_us) \
    rtos_mutex_take(mutex, (rtos_time_ns_t)(timeout_us) * 1000)

#define rtos_mutex_unlock(mutex) \
    rtos_mutex_release(mutex)

#endif /* __RTOS_MUTEX_H__ */