/**
 * @file queue.h
 * @brief RTOS消息队列 - 重构后的消息队列系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_QUEUE_H__
#define __RTOS_QUEUE_H__

#include "../core/types.h"
#include "../core/object.h"

/* 消息队列结构体 */
typedef struct rtos_queue {
    rtos_object_t          parent;             /* 继承对象基类 */
    
    /* 队列属性 */
    uint8_t               *buffer;             /* 队列缓冲区 */
    uint32_t               item_size;          /* 项目大小 */
    uint32_t               max_items;          /* 最大项目数 */
    uint32_t               item_count;         /* 当前项目数 */
    uint32_t               head;               /* 队列头 */
    uint32_t               tail;               /* 队列尾 */
    
    /* 等待队列 - 使用统一等待队列设计 */
    rtos_wait_node_t       send_wait_queue;    /* 发送等待队列头 */
    rtos_wait_node_t       recv_wait_queue;    /* 接收等待队列头 */
    uint32_t               send_wait_count;    /* 发送等待数量 */
    uint32_t               recv_wait_count;    /* 接收等待数量 */
    
    /* 统计信息 */
    uint32_t               send_count;         /* 发送次数 */
    uint32_t               recv_count;         /* 接收次数 */
    uint32_t               overflow_count;     /* 溢出次数 */
} rtos_queue_t;

/* 消息队列创建参数 */
typedef struct {
    const char            *name;               /* 队列名称 */
    uint32_t               item_size;          /* 项目大小 */
    uint32_t               max_items;          /* 最大项目数 */
} rtos_queue_create_params_t;

/* 消息队列信息 */
typedef struct {
    char                   name[16];           /* 队列名称 */
    uint32_t               item_size;          /* 项目大小 */
    uint32_t               max_items;          /* 最大项目数 */
    uint32_t               item_count;         /* 当前项目数 */
    uint32_t               send_wait_count;    /* 发送等待数量 */
    uint32_t               recv_wait_count;    /* 接收等待数量 */
    uint32_t               send_count;         /* 发送次数 */
    uint32_t               recv_count;         /* 接收次数 */
    uint32_t               overflow_count;     /* 溢出次数 */
} rtos_queue_info_t;

/* 消息队列API函数声明 */

/**
 * @brief 初始化消息队列 - 静态方式
 * @param queue 队列指针
 * @param params 创建参数
 * @param buffer 缓冲区指针
 * @return 操作结果
 */
rtos_result_t rtos_queue_init(rtos_queue_t *queue,
                              const rtos_queue_create_params_t *params,
                              void *buffer);

/**
 * @brief 创建消息队列 - 动态方式
 * @param params 创建参数
 * @return 队列指针，失败返回NULL
 */
rtos_queue_t *rtos_queue_create(const rtos_queue_create_params_t *params);

/**
 * @brief 删除消息队列
 * @param queue 队列指针
 * @return 操作结果
 */
rtos_result_t rtos_queue_delete(rtos_queue_t *queue);

/**
 * @brief 发送消息到队列
 * @param queue 队列指针
 * @param item 消息指针
 * @param item_size 消息大小
 * @param timeout 超时时间
 * @return 操作结果
 */
rtos_result_t rtos_queue_send(rtos_queue_t *queue,
                              const void *item,
                              uint32_t item_size,
                              rtos_timeout_t timeout);

/**
 * @brief 从队列接收消息
 * @param queue 队列指针
 * @param item 消息指针
 * @param item_size 消息大小
 * @param timeout 超时时间
 * @return 接收到的字节数，失败返回负数
 */
int32_t rtos_queue_receive(rtos_queue_t *queue,
                           void *item,
                           uint32_t item_size,
                           rtos_timeout_t timeout);

/**
 * @brief 尝试发送消息到队列(非阻塞)
 * @param queue 队列指针
 * @param item 消息指针
 * @param item_size 消息大小
 * @return 操作结果
 */
rtos_result_t rtos_queue_try_send(rtos_queue_t *queue,
                                  const void *item,
                                  uint32_t item_size);

/**
 * @brief 尝试从队列接收消息(非阻塞)
 * @param queue 队列指针
 * @param item 消息指针
 * @param item_size 消息大小
 * @return 接收到的字节数，失败返回负数
 */
int32_t rtos_queue_try_receive(rtos_queue_t *queue,
                               void *item,
                               uint32_t item_size);

/**
 * @brief 获取队列信息
 * @param queue 队列指针
 * @param info 信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_queue_get_info(const rtos_queue_t *queue,
                                  rtos_queue_info_t *info);

/**
 * @brief 重置队列
 * @param queue 队列指针
 * @return 操作结果
 */
rtos_result_t rtos_queue_reset(rtos_queue_t *queue);

/**
 * @brief 检查队列是否为空
 * @param queue 队列指针
 * @return 是否为空
 */
bool rtos_queue_is_empty(const rtos_queue_t *queue);

/**
 * @brief 检查队列是否已满
 * @param queue 队列指针
 * @return 是否已满
 */
bool rtos_queue_is_full(const rtos_queue_t *queue);

/**
 * @brief 获取队列中项目数量
 * @param queue 队列指针
 * @return 项目数量
 */
uint32_t rtos_queue_get_count(const rtos_queue_t *queue);

/**
 * @brief 获取队列剩余空间
 * @param queue 队列指针
 * @return 剩余空间
 */
uint32_t rtos_queue_get_space(const rtos_queue_t *queue);

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_messagequeue_t rtos_queue_t
#define rtos_mq_init rtos_queue_init
#define rtos_mq_send rtos_queue_send
#define rtos_mq_recv rtos_queue_receive

#endif /* __RTOS_QUEUE_H__ */
