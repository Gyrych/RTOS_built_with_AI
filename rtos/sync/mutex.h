/**
 * @file mutex.h
 * @brief RTOS互斥量 - 重构后的互斥量系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_MUTEX_H__
#define __RTOS_MUTEX_H__

#include "../core/types.h"
#include "../core/object.h"

/* 互斥量结构体 */
typedef struct rtos_mutex {
    rtos_object_t          parent;             /* 继承对象基类 */
    
    /* 互斥量属性 */
    rtos_priority_t        owner_priority;     /* 所有者优先级 */
    rtos_priority_t        original_priority;  /* 原始优先级 */
    struct rtos_task       *owner;             /* 所有者任务 */
    uint32_t               lock_count;         /* 锁定计数 */
    bool                   is_recursive;       /* 是否递归 */
    
    /* 等待队列 - 使用统一等待队列设计 */
    rtos_wait_node_t       wait_queue;         /* 等待队列头 */
    uint32_t               wait_count;         /* 等待任务数量 */
    
    /* 统计信息 */
    uint32_t               lock_count_total;   /* 总锁定次数 */
    uint32_t               unlock_count_total; /* 总解锁次数 */
    rtos_time_ns_t         max_hold_time;      /* 最大持有时间 */
} rtos_mutex_t;

/* 互斥量创建参数 */
typedef struct {
    const char            *name;               /* 互斥量名称 */
    bool                   recursive;          /* 是否递归 */
    rtos_priority_t        ceiling_priority;   /* 天花板优先级 */
} rtos_mutex_create_params_t;

/* 互斥量信息 */
typedef struct {
    char                   name[16];           /* 互斥量名称 */
    bool                   is_locked;          /* 是否被锁定 */
    rtos_priority_t        owner_priority;     /* 所有者优先级 */
    uint32_t               lock_count;         /* 锁定计数 */
    bool                   is_recursive;       /* 是否递归 */
    uint32_t               wait_count;         /* 等待任务数量 */
    uint32_t               lock_count_total;   /* 总锁定次数 */
    uint32_t               unlock_count_total; /* 总解锁次数 */
} rtos_mutex_info_t;

/* 互斥量API函数声明 */

/**
 * @brief 初始化互斥量 - 静态方式
 * @param mutex 互斥量指针
 * @param params 创建参数
 * @return 操作结果
 */
rtos_result_t rtos_mutex_init(rtos_mutex_t *mutex,
                              const rtos_mutex_create_params_t *params);

/**
 * @brief 创建互斥量 - 动态方式
 * @param params 创建参数
 * @return 互斥量指针，失败返回NULL
 */
rtos_mutex_t *rtos_mutex_create(const rtos_mutex_create_params_t *params);

/**
 * @brief 删除互斥量
 * @param mutex 互斥量指针
 * @return 操作结果
 */
rtos_result_t rtos_mutex_delete(rtos_mutex_t *mutex);

/**
 * @brief 锁定互斥量
 * @param mutex 互斥量指针
 * @param timeout 超时时间
 * @return 操作结果
 */
rtos_result_t rtos_mutex_take(rtos_mutex_t *mutex, rtos_timeout_t timeout);

/**
 * @brief 解锁互斥量
 * @param mutex 互斥量指针
 * @return 操作结果
 */
rtos_result_t rtos_mutex_release(rtos_mutex_t *mutex);

/**
 * @brief 尝试锁定互斥量(非阻塞)
 * @param mutex 互斥量指针
 * @return 操作结果
 */
rtos_result_t rtos_mutex_try_take(rtos_mutex_t *mutex);

/**
 * @brief 获取互斥量信息
 * @param mutex 互斥量指针
 * @param info 信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_mutex_get_info(const rtos_mutex_t *mutex,
                                  rtos_mutex_info_t *info);

/**
 * @brief 设置互斥量天花板优先级
 * @param mutex 互斥量指针
 * @param ceiling_priority 天花板优先级
 * @return 操作结果
 */
rtos_result_t rtos_mutex_set_ceiling_priority(rtos_mutex_t *mutex,
                                              rtos_priority_t ceiling_priority);

/**
 * @brief 获取互斥量天花板优先级
 * @param mutex 互斥量指针
 * @return 天花板优先级
 */
rtos_priority_t rtos_mutex_get_ceiling_priority(const rtos_mutex_t *mutex);

/**
 * @brief 检查互斥量是否被当前任务持有
 * @param mutex 互斥量指针
 * @return 是否被当前任务持有
 */
bool rtos_mutex_is_owner(const rtos_mutex_t *mutex);

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_mutex_lock rtos_mutex_take
#define rtos_mutex_unlock rtos_mutex_release

#endif /* __RTOS_MUTEX_H__ */
