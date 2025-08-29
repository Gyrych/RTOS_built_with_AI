/**
 * @file rtos_queue.h
 * @brief RTOS消息队列模块头文件 - 基于IPC对象实现
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_QUEUE_H__
#define __RTOS_QUEUE_H__

#include "rtos_object.h"
#include "rtos_task.h"

/**
 * @brief 消息队列控制块结构体
 * 继承自IPC对象基类
 */
typedef struct rtos_messagequeue {
    struct rtos_ipc_object parent;              /* 继承IPC对象基类 */
    
    /* 消息池相关 */
    void               *msg_pool;               /* 消息池指针 */
    uint16_t            msg_size;               /* 消息大小 */
    uint16_t            max_msgs;               /* 最大消息数 */
    
    /* 队列状态 */
    uint16_t            entry;                  /* 队列中消息数 */
    void               *msg_queue_head;         /* 消息队列头 */
    void               *msg_queue_tail;         /* 消息队列尾 */
    void               *msg_queue_free;         /* 空闲消息链表 */
    
    /* 等待线程 */
    struct rtos_task   *sender_thread;          /* 发送等待线程 */
    struct rtos_task   *receiver_thread;        /* 接收等待线程 */
} rtos_messagequeue_t;

/* 消息队列API函数声明 */

/**
 * @brief 初始化消息队列 - 静态方式
 * @param mq 消息队列控制块指针
 * @param name 消息队列名称
 * @param msgpool 消息池指针
 * @param msg_size 消息大小
 * @param pool_size 消息池大小
 * @param flag 标志
 * @return 结果码
 */
rtos_result_t rtos_mq_init(rtos_messagequeue_t *mq,
                          const char          *name,
                          void                *msgpool,
                          uint32_t             msg_size,
                          uint32_t             pool_size,
                          uint8_t              flag);

/**
 * @brief 创建消息队列 - 动态方式
 * @param name 消息队列名称
 * @param msg_size 消息大小
 * @param max_msgs 最大消息数
 * @param flag 标志
 * @return 消息队列指针
 */
rtos_messagequeue_t *rtos_mq_create(const char *name,
                                   uint32_t    msg_size,
                                   uint32_t    max_msgs,
                                   uint8_t     flag);

/**
 * @brief 分离消息队列
 * @param mq 消息队列指针
 * @return 结果码
 */
rtos_result_t rtos_mq_detach(rtos_messagequeue_t *mq);

/**
 * @brief 删除消息队列
 * @param mq 消息队列指针
 * @return 结果码
 */
rtos_result_t rtos_mq_delete(rtos_messagequeue_t *mq);

/**
 * @brief 发送消息到队列
 * @param mq 消息队列指针
 * @param buffer 消息缓冲区
 * @param size 消息大小
 * @param timeout 超时时间(纳秒)，0表示不等待，RTOS_WAITING_FOREVER表示永久等待
 * @return 结果码
 */
rtos_result_t rtos_mq_send(rtos_messagequeue_t *mq,
                          const void          *buffer,
                          uint32_t             size,
                          rtos_time_ns_t       timeout);

/**
 * @brief 紧急发送消息(插入队头)
 * @param mq 消息队列指针
 * @param buffer 消息缓冲区
 * @param size 消息大小
 * @param timeout 超时时间(纳秒)
 * @return 结果码
 */
rtos_result_t rtos_mq_urgent(rtos_messagequeue_t *mq,
                            const void          *buffer,
                            uint32_t             size,
                            rtos_time_ns_t       timeout);

/**
 * @brief 从队列接收消息
 * @param mq 消息队列指针
 * @param buffer 接收缓冲区
 * @param size 缓冲区大小
 * @param timeout 超时时间(纳秒)，0表示不等待，RTOS_WAITING_FOREVER表示永久等待
 * @return 实际接收的消息大小，错误时返回负值
 */
int rtos_mq_recv(rtos_messagequeue_t *mq,
                void                *buffer,
                uint32_t             size,
                rtos_time_ns_t       timeout);

/**
 * @brief 控制消息队列
 * @param mq 消息队列指针
 * @param cmd 控制命令
 * @param arg 参数
 * @return 结果码
 */
rtos_result_t rtos_mq_control(rtos_messagequeue_t *mq, int cmd, void *arg);

/**
 * @brief 查找消息队列
 * @param name 消息队列名称
 * @return 消息队列指针
 */
rtos_messagequeue_t *rtos_mq_find(const char *name);

/* 消息队列标志定义 */
#define RTOS_MQ_FLAG_FIFO              0x00    /* FIFO方式等待 */
#define RTOS_MQ_FLAG_PRIO              0x01    /* 优先级方式等待 */

/* 消息队列控制命令 */
#define RTOS_MQ_CTRL_GET_INFO          0x01    /* 获取队列信息 */
#define RTOS_MQ_CTRL_FLUSH             0x02    /* 清空队列 */
#define RTOS_MQ_CTRL_RESET             0x03    /* 重置队列 */

/* 消息队列超时定义 */
#define RTOS_WAITING_NO                0       /* 不等待 */
#define RTOS_WAITING_FOREVER           UINT64_MAX  /* 永久等待 */

/* 消息队列信息结构 */
typedef struct {
    char        name[16];                       /* 队列名称 */
    uint32_t    entry;                          /* 当前消息数 */
    uint32_t    max_msgs;                       /* 最大消息数 */
    uint32_t    msg_size;                       /* 消息大小 */
    uint32_t    suspend_sender_count;           /* 挂起发送线程数 */
    uint32_t    suspend_receiver_count;         /* 挂起接收线程数 */
} rtos_mq_info_t;

/**
 * @brief 获取消息队列信息
 * @param mq 消息队列指针
 * @param info 信息结构体指针
 * @return 结果码
 */
rtos_result_t rtos_mq_get_info(rtos_messagequeue_t *mq, rtos_mq_info_t *info);

/* 内联辅助函数 */

/**
 * @brief 检查消息队列有效性
 * @param mq 消息队列指针
 * @return 是否有效
 */
static inline bool rtos_mq_is_valid(rtos_messagequeue_t *mq)
{
    return mq && rtos_object_is_valid(&(mq->parent.parent), RTOS_OBJECT_CLASS_MESSAGEQUEUE);
}

/**
 * @brief 获取消息队列名称
 * @param mq 消息队列指针
 * @return 队列名称
 */
static inline const char *rtos_mq_get_name(rtos_messagequeue_t *mq)
{
    if (!rtos_mq_is_valid(mq)) {
        return NULL;
    }
    return mq->parent.parent.name;
}

/**
 * @brief 检查消息队列是否为空
 * @param mq 消息队列指针
 * @return 是否为空
 */
static inline bool rtos_mq_is_empty(rtos_messagequeue_t *mq)
{
    if (!rtos_mq_is_valid(mq)) {
        return true;
    }
    return mq->entry == 0;
}

/**
 * @brief 检查消息队列是否已满
 * @param mq 消息队列指针
 * @return 是否已满
 */
static inline bool rtos_mq_is_full(rtos_messagequeue_t *mq)
{
    if (!rtos_mq_is_valid(mq)) {
        return true;
    }
    return mq->entry >= mq->max_msgs;
}

/**
 * @brief 获取队列中消息数量
 * @param mq 消息队列指针
 * @return 消息数量
 */
static inline uint32_t rtos_mq_get_count(rtos_messagequeue_t *mq)
{
    if (!rtos_mq_is_valid(mq)) {
        return 0;
    }
    return mq->entry;
}

/* 兼容性宏定义(保持与旧版本API的兼容) */
#define rtos_queue_create(queue, buffer, item_size, max_items) \
    rtos_mq_init((rtos_messagequeue_t *)(queue), "queue", buffer, item_size, (item_size) * (max_items), 0)

#define rtos_queue_send(queue, item, timeout_us) \
    rtos_mq_send((rtos_messagequeue_t *)(queue), item, (queue)->msg_size, (rtos_time_ns_t)(timeout_us) * 1000)

#define rtos_queue_receive(queue, item, timeout_us) \
    rtos_mq_recv((rtos_messagequeue_t *)(queue), item, (queue)->msg_size, (rtos_time_ns_t)(timeout_us) * 1000)

#define rtos_queue_delete(queue) \
    rtos_mq_delete((rtos_messagequeue_t *)(queue))

#endif /* __RTOS_QUEUE_H__ */