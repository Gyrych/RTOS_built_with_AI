/**
 * @file object.h
 * @brief RTOS内核对象基类 - 重构后的简化对象系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_OBJECT_H__
#define __RTOS_OBJECT_H__

#include "types.h"

/* 前向声明 */
struct rtos_object;
struct rtos_task;
struct rtos_semaphore;
struct rtos_mutex;
struct rtos_queue;
struct rtos_event_group;
struct rtos_sw_timer;
struct rtos_memory_pool;

/* 对象基类结构体 */
typedef struct rtos_object {
    char                    name[16];           /* 对象名称 */
    rtos_object_type_t     type;               /* 对象类型 */
    rtos_object_flag_t     flags;              /* 对象标志 */
    rtos_time_ns_t         create_time;        /* 创建时间 */
    uint32_t               ref_count;          /* 引用计数 */
    struct rtos_object     *next;              /* 链表下一个 */
    struct rtos_object     *prev;              /* 链表上一个 */
} rtos_object_t;

/* 对象信息结构体 */
typedef struct {
    rtos_object_type_t     type;               /* 对象类型 */
    uint32_t               count;              /* 当前数量 */
    uint32_t               max_count;          /* 最大数量 */
    rtos_object_t          *first;             /* 第一个对象 */
    rtos_object_t          *last;              /* 最后一个对象 */
} rtos_object_information_t;

/* 对象操作函数声明 */

/**
 * @brief 初始化对象
 * @param object 对象指针
 * @param type 对象类型
 * @param name 对象名称
 * @param flags 对象标志
 */
void rtos_object_init(rtos_object_t *object, 
                     rtos_object_type_t type,
                     const char *name,
                     rtos_object_flag_t flags);

/**
 * @brief 设置对象名称
 * @param object 对象指针
 * @param name 新名称
 */
void rtos_object_set_name(rtos_object_t *object, const char *name);

/**
 * @brief 获取对象名称
 * @param object 对象指针
 * @return 对象名称
 */
const char *rtos_object_get_name(const rtos_object_t *object);

/**
 * @brief 获取对象类型
 * @param object 对象指针
 * @return 对象类型
 */
rtos_object_type_t rtos_object_get_type(const rtos_object_t *object);

/**
 * @brief 获取对象标志
 * @param object 对象指针
 * @return 对象标志
 */
rtos_object_flag_t rtos_object_get_flags(const rtos_object_t *object);

/**
 * @brief 设置对象标志
 * @param object 对象指针
 * @param flags 新标志
 */
void rtos_object_set_flags(rtos_object_t *object, rtos_object_flag_t flags);

/**
 * @brief 检查对象是否为静态对象
 * @param object 对象指针
 * @return 是否为静态对象
 */
bool rtos_object_is_static(const rtos_object_t *object);

/**
 * @brief 检查对象是否为动态对象
 * @param object 对象指针
 * @return 是否为动态对象
 */
bool rtos_object_is_dynamic(const rtos_object_t *object);

/**
 * @brief 检查对象是否为系统对象
 * @param object 对象指针
 * @return 是否为系统对象
 */
bool rtos_object_is_system(const rtos_object_t *object);

/* 引用计数管理 - 新增 */

/**
 * @brief 增加对象引用计数
 * @param object 对象指针
 * @return 新的引用计数
 */
uint32_t rtos_object_ref_inc(rtos_object_t *object);

/**
 * @brief 减少对象引用计数
 * @param object 对象指针
 * @return 新的引用计数
 */
uint32_t rtos_object_ref_dec(rtos_object_t *object);

/**
 * @brief 获取对象引用计数
 * @param object 对象指针
 * @return 引用计数
 */
uint32_t rtos_object_get_ref_count(const rtos_object_t *object);

/* 对象容器管理函数 */

/**
 * @brief 初始化对象容器
 * @param info 容器信息指针
 * @param type 对象类型
 * @param max_count 最大对象数量
 */
void rtos_object_container_init(rtos_object_information_t *info,
                              rtos_object_type_t type,
                              uint32_t max_count);

/**
 * @brief 添加对象到容器
 * @param info 容器信息指针
 * @param object 对象指针
 * @return 操作结果
 */
rtos_result_t rtos_object_container_add(rtos_object_information_t *info,
                                       rtos_object_t *object);

/**
 * @brief 从容器移除对象
 * @param info 容器信息指针
 * @param object 对象指针
 * @return 操作结果
 */
rtos_result_t rtos_object_container_remove(rtos_object_information_t *info,
                                          rtos_object_t *object);

/**
 * @brief 查找对象
 * @param info 容器信息指针
 * @param name 对象名称
 * @return 对象指针，未找到返回NULL
 */
rtos_object_t *rtos_object_container_find(const rtos_object_information_t *info,
                                         const char *name);

/**
 * @brief 获取容器统计信息
 * @param info 容器信息指针
 * @return 对象数量
 */
uint32_t rtos_object_container_get_count(const rtos_object_information_t *info);

/**
 * @brief 遍历容器中的对象
 * @param info 容器信息指针
 * @param callback 回调函数
 * @param arg 参数
 */
void rtos_object_container_traverse(const rtos_object_information_t *info,
                                   void (*callback)(rtos_object_t *object, void *arg),
                                   void *arg);

/* 等待队列管理 - 新增统一等待队列设计 */

/**
 * @brief 初始化等待队列
 * @param head 队列头指针
 */
void rtos_wait_queue_init(rtos_wait_node_t *head);

/**
 * @brief 添加任务到等待队列
 * @param head 队列头指针
 * @param task 任务指针
 * @param timeout 超时时间
 * @return 操作结果
 */
rtos_result_t rtos_wait_queue_add(rtos_wait_node_t *head,
                                  struct rtos_task *task,
                                  rtos_timeout_t timeout);

/**
 * @brief 从等待队列移除任务
 * @param head 队列头指针
 * @param task 任务指针
 * @return 操作结果
 */
rtos_result_t rtos_wait_queue_remove(rtos_wait_node_t *head,
                                     struct rtos_task *task);

/**
 * @brief 获取等待队列中的第一个任务
 * @param head 队列头指针
 * @return 任务指针，队列为空返回NULL
 */
struct rtos_task *rtos_wait_queue_get_first(rtos_wait_node_t *head);

/**
 * @brief 检查等待队列是否为空
 * @param head 队列头指针
 * @return 是否为空
 */
bool rtos_wait_queue_is_empty(const rtos_wait_node_t *head);

/**
 * @brief 获取等待队列长度
 * @param head 队列头指针
 * @return 队列长度
 */
uint32_t rtos_wait_queue_get_length(const rtos_wait_node_t *head);

/* 链表操作宏 - 优化版本 */
#define rtos_list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define rtos_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define rtos_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#define rtos_list_for_each_entry(pos, head, member) \
    for (pos = rtos_list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = rtos_list_entry(pos->member.next, typeof(*pos), member))

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_object_information rtos_object_information_t

#endif /* __RTOS_OBJECT_H__ */
