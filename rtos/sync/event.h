/**
 * @file event.h
 * @brief RTOS事件组 - 重构后的事件组系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_EVENT_H__
#define __RTOS_EVENT_H__

#include "../core/types.h"
#include "../core/object.h"

/* 事件标志位定义 */
typedef uint32_t rtos_event_bits_t;

/* 事件组结构体 */
typedef struct rtos_event_group {
    rtos_object_t          parent;             /* 继承对象基类 */
    
    /* 事件组属性 */
    rtos_event_bits_t      bits;               /* 当前事件位 */
    rtos_event_bits_t      bits_set;           /* 已设置的事件位 */
    rtos_event_bits_t      bits_clear;         /* 已清除的事件位 */
    
    /* 等待队列 - 使用统一等待队列设计 */
    rtos_wait_node_t       wait_queue;         /* 等待队列头 */
    uint32_t               wait_count;         /* 等待任务数量 */
    
    /* 统计信息 */
    uint32_t               set_count;          /* 设置次数 */
    uint32_t               clear_count;        /* 清除次数 */
    uint32_t               wait_count_total;   /* 总等待次数 */
} rtos_event_group_t;

/* 事件组创建参数 */
typedef struct {
    const char            *name;               /* 事件组名称 */
    rtos_event_bits_t      initial_bits;      /* 初始事件位 */
} rtos_event_group_create_params_t;

/* 事件组信息 */
typedef struct {
    char                   name[16];           /* 事件组名称 */
    rtos_event_bits_t      bits;               /* 当前事件位 */
    uint32_t               wait_count;         /* 等待任务数量 */
    uint32_t               set_count;          /* 设置次数 */
    uint32_t               clear_count;        /* 清除次数 */
} rtos_event_group_info_t;

/* 事件组API函数声明 */

/**
 * @brief 初始化事件组 - 静态方式
 * @param event_group 事件组指针
 * @param params 创建参数
 * @return 操作结果
 */
rtos_result_t rtos_event_group_init(rtos_event_group_t *event_group,
                                    const rtos_event_group_create_params_t *params);

/**
 * @brief 创建事件组 - 动态方式
 * @param params 创建参数
 * @return 事件组指针，失败返回NULL
 */
rtos_event_group_t *rtos_event_group_create(const rtos_event_group_create_params_t *params);

/**
 * @brief 删除事件组
 * @param event_group 事件组指针
 * @return 操作结果
 */
rtos_result_t rtos_event_group_delete(rtos_event_group_t *event_group);

/**
 * @brief 设置事件位
 * @param event_group 事件组指针
 * @param bits 要设置的事件位
 * @return 设置后的事件位
 */
rtos_event_bits_t rtos_event_group_set_bits(rtos_event_group_t *event_group,
                                            rtos_event_bits_t bits);

/**
 * @brief 清除事件位
 * @param event_group 事件组指针
 * @param bits 要清除的事件位
 * @return 清除后的事件位
 */
rtos_event_bits_t rtos_event_group_clear_bits(rtos_event_group_t *event_group,
                                              rtos_event_bits_t bits);

/**
 * @brief 等待事件位
 * @param event_group 事件组指针
 * @param bits 要等待的事件位
 * @param clear_on_exit 退出时是否清除
 * @param wait_for_all 是否等待所有位
 * @param timeout 超时时间
 * @return 满足条件的事件位，超时返回0
 */
rtos_event_bits_t rtos_event_group_wait_bits(rtos_event_group_t *event_group,
                                             rtos_event_bits_t bits,
                                             bool clear_on_exit,
                                             bool wait_for_all,
                                             rtos_timeout_t timeout);

/**
 * @brief 获取当前事件位
 * @param event_group 事件组指针
 * @return 当前事件位
 */
rtos_event_bits_t rtos_event_group_get_bits(const rtos_event_group_t *event_group);

/**
 * @brief 尝试设置事件位(非阻塞)
 * @param event_group 事件组指针
 * @param bits 要设置的事件位
 * @return 设置后的事件位
 */
rtos_event_bits_t rtos_event_group_try_set_bits(rtos_event_group_t *event_group,
                                                rtos_event_bits_t bits);

/**
 * @brief 尝试清除事件位(非阻塞)
 * @param event_group 事件组指针
 * @param bits 要清除的事件位
 * @return 清除后的事件位
 */
rtos_event_bits_t rtos_event_group_try_clear_bits(rtos_event_group_t *event_group,
                                                  rtos_event_bits_t bits);

/**
 * @brief 获取事件组信息
 * @param event_group 事件组指针
 * @param info 信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_event_group_get_info(const rtos_event_group_t *event_group,
                                        rtos_event_group_info_t *info);

/**
 * @brief 重置事件组
 * @param event_group 事件组指针
 * @param bits 新的事件位
 * @return 操作结果
 */
rtos_result_t rtos_event_group_reset(rtos_event_group_t *event_group,
                                     rtos_event_bits_t bits);

/**
 * @brief 同步事件位
 * @param event_group 事件组指针
 * @param bits 要同步的事件位
 * @return 操作结果
 */
rtos_result_t rtos_event_group_sync(rtos_event_group_t *event_group,
                                    rtos_event_bits_t bits);

/* 事件位操作宏 */
#define RTOS_EVENT_BIT_0     (1UL << 0)
#define RTOS_EVENT_BIT_1     (1UL << 1)
#define RTOS_EVENT_BIT_2     (1UL << 2)
#define RTOS_EVENT_BIT_3     (1UL << 3)
#define RTOS_EVENT_BIT_4     (1UL << 4)
#define RTOS_EVENT_BIT_5     (1UL << 5)
#define RTOS_EVENT_BIT_6     (1UL << 6)
#define RTOS_EVENT_BIT_7     (1UL << 7)
#define RTOS_EVENT_BIT_8     (1UL << 8)
#define RTOS_EVENT_BIT_9     (1UL << 9)
#define RTOS_EVENT_BIT_10    (1UL << 10)
#define RTOS_EVENT_BIT_11    (1UL << 11)
#define RTOS_EVENT_BIT_12    (1UL << 12)
#define RTOS_EVENT_BIT_13    (1UL << 13)
#define RTOS_EVENT_BIT_14    (1UL << 14)
#define RTOS_EVENT_BIT_15    (1UL << 15)
#define RTOS_EVENT_BIT_16    (1UL << 16)
#define RTOS_EVENT_BIT_17    (1UL << 17)
#define RTOS_EVENT_BIT_18    (1UL << 18)
#define RTOS_EVENT_BIT_19    (1UL << 19)
#define RTOS_EVENT_BIT_20    (1UL << 20)
#define RTOS_EVENT_BIT_21    (1UL << 21)
#define RTOS_EVENT_BIT_22    (1UL << 22)
#define RTOS_EVENT_BIT_23    (1UL << 23)
#define RTOS_EVENT_BIT_24    (1UL << 24)
#define RTOS_EVENT_BIT_25    (1UL << 25)
#define RTOS_EVENT_BIT_26    (1UL << 26)
#define RTOS_EVENT_BIT_27    (1UL << 27)
#define RTOS_EVENT_BIT_28    (1UL << 28)
#define RTOS_EVENT_BIT_29    (1UL << 29)
#define RTOS_EVENT_BIT_30    (1UL << 30)
#define RTOS_EVENT_BIT_31    (1UL << 31)

#define RTOS_EVENT_BIT_ALL   (0xFFFFFFFFUL)
#define RTOS_EVENT_BIT_NONE  (0x00000000UL)

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_event_group_t rtos_event_group_t

#endif /* __RTOS_EVENT_H__ */
