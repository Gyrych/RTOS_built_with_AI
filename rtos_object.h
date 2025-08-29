/**
 * @file rtos_object.h
 * @brief RTOS内核对象基类定义 - 参考RT-Thread设计
 * @author Assistant
 * @date 2024
 * 
 * 实现面向对象的内核对象管理系统:
 * 1. 统一的对象基类(rt_object)
 * 2. 对象容器管理
 * 3. 对象链表组织
 * 4. 动态/静态对象支持
 */

#ifndef __RTOS_OBJECT_H__
#define __RTOS_OBJECT_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 前向声明 */
struct rtos_task;

#ifndef RTOS_RESULT_T_DEFINED
#define RTOS_RESULT_T_DEFINED
typedef enum {
    RTOS_OK = 0,
    RTOS_ERROR,
    RTOS_ERROR_TIMEOUT,
    RTOS_ERROR_NO_MEMORY,
    RTOS_ERROR_INVALID_PARAM,
    RTOS_ERROR_RESOURCE_BUSY,
    RTOS_ERROR_DEADLOCK,
    RTOS_ERROR_STACK_OVERFLOW,
    RTOS_ERROR_MEMORY_CORRUPTION
} rtos_result_t;
#endif

/* 内核对象类型定义 */
typedef enum {
    RTOS_OBJECT_CLASS_NULL = 0,          /* 空对象 */
    RTOS_OBJECT_CLASS_THREAD,            /* 线程对象 */
    RTOS_OBJECT_CLASS_SEMAPHORE,         /* 信号量对象 */
    RTOS_OBJECT_CLASS_MUTEX,             /* 互斥量对象 */
    RTOS_OBJECT_CLASS_EVENT,             /* 事件对象 */
    RTOS_OBJECT_CLASS_MAILBOX,           /* 邮箱对象 */
    RTOS_OBJECT_CLASS_MESSAGEQUEUE,      /* 消息队列对象 */
    RTOS_OBJECT_CLASS_MEMHEAP,           /* 内存堆对象 */
    RTOS_OBJECT_CLASS_MEMPOOL,           /* 内存池对象 */
    RTOS_OBJECT_CLASS_DEVICE,            /* 设备对象 */
    RTOS_OBJECT_CLASS_TIMER,             /* 定时器对象 */
    RTOS_OBJECT_CLASS_MODULE,            /* 模块对象 */
    RTOS_OBJECT_CLASS_UNKNOWN,           /* 未知对象 */
    RTOS_OBJECT_CLASS_STATIC = 0x80      /* 静态对象标志 */
} rtos_object_class_type_t;

/* 对象标志定义 */
#define RTOS_OBJECT_FLAG_NONE           0x00
#define RTOS_OBJECT_FLAG_STATIC         0x01    /* 静态对象 */
#define RTOS_OBJECT_FLAG_DYNAMIC        0x02    /* 动态对象 */

/**
 * @brief 内核对象基类结构体
 * 所有内核对象都继承自此基类
 */
typedef struct rtos_object {
    char                    name[16];           /* 对象名称 */
    rtos_object_class_type_t type;              /* 对象类型 */
    uint8_t                 flag;               /* 对象标志 */
    struct rtos_object     *prev;               /* 链表前驱指针 */
    struct rtos_object     *next;               /* 链表后继指针 */
} rtos_object_t;

/**
 * @brief 对象信息结构体
 * 维护同类对象的容器信息
 */
typedef struct rtos_object_information {
    rtos_object_class_type_t type;              /* 对象类型 */
    struct rtos_object      *object_list;       /* 对象链表头 */
    uint32_t                object_size;        /* 对象大小 */
    uint32_t                object_count;       /* 对象数量 */
    uint32_t                object_max_count;   /* 最大对象数量 */
} rtos_object_information_t;

/**
 * @brief IPC对象基类结构体
 * 用于线程间通信的对象都继承自此基类
 */
typedef struct rtos_ipc_object {
    struct rtos_object      parent;             /* 继承对象基类 */
    struct rtos_task       *suspend_thread;     /* 挂起的线程链表 */
} rtos_ipc_object_t;

/* 内核对象管理函数声明 */

/**
 * @brief 初始化对象系统
 */
void rtos_object_system_init(void);

/**
 * @brief 初始化对象
 * @param object 对象指针
 * @param type 对象类型
 * @param name 对象名称
 */
void rtos_object_init(rtos_object_t          *object,
                     rtos_object_class_type_t type,
                     const char              *name);

/**
 * @brief 分离对象
 * @param object 对象指针
 */
rtos_result_t rtos_object_detach(rtos_object_t *object);

/**
 * @brief 分配对象
 * @param type 对象类型
 * @param name 对象名称
 * @return 对象指针
 */
rtos_object_t *rtos_object_allocate(rtos_object_class_type_t type,
                                   const char              *name);

/**
 * @brief 删除对象
 * @param object 对象指针
 */
void rtos_object_delete(rtos_object_t *object);

/**
 * @brief 查找对象
 * @param type 对象类型
 * @param name 对象名称
 * @return 对象指针
 */
rtos_object_t *rtos_object_find(rtos_object_class_type_t type,
                               const char              *name);

/**
 * @brief 获取对象类型
 * @param object 对象指针
 * @return 对象类型
 */
rtos_object_class_type_t rtos_object_get_type(rtos_object_t *object);

/**
 * @brief 获取对象名称
 * @param object 对象指针
 * @return 对象名称
 */
const char *rtos_object_get_name(rtos_object_t *object);

/**
 * @brief 获取对象信息
 * @param type 对象类型
 * @return 对象信息指针
 */
rtos_object_information_t *rtos_object_get_information(rtos_object_class_type_t type);

/**
 * @brief 对象迭代器回调函数类型
 * @param object 对象指针
 * @param data 用户数据
 * @return 是否继续迭代
 */
typedef bool (*rtos_object_iterator_t)(rtos_object_t *object, void *data);

/**
 * @brief 遍历指定类型的所有对象
 * @param type 对象类型
 * @param iterator 迭代器函数
 * @param data 用户数据
 */
void rtos_object_for_each(rtos_object_class_type_t type,
                         rtos_object_iterator_t   iterator,
                         void                    *data);

/**
 * @brief 检查对象有效性
 * @param object 对象指针
 * @param type 期望的对象类型
 * @return 是否有效
 */
bool rtos_object_is_valid(rtos_object_t *object, rtos_object_class_type_t type);

/**
 * @brief 获取对象容器链表头
 * @param type 对象类型
 * @return 链表头指针
 */
rtos_object_t *rtos_object_get_list(rtos_object_class_type_t type);

/**
 * @brief 初始化IPC对象
 * @param ipc IPC对象指针
 * @param type 对象类型
 * @param name 对象名称
 */
void rtos_ipc_object_init(rtos_ipc_object_t       *ipc,
                         rtos_object_class_type_t type,
                         const char              *name);

/**
 * @brief 挂起线程到IPC对象
 * @param ipc IPC对象指针
 * @param thread 线程指针
 * @param flag 挂起标志
 * @param timeout 超时时间
 * @return 结果
 */
rtos_result_t rtos_ipc_object_suspend_thread(rtos_ipc_object_t *ipc,
                                             struct rtos_task  *thread,
                                             uint8_t           flag,
                                             int32_t           timeout);

/**
 * @brief 从IPC对象恢复线程
 * @param ipc IPC对象指针
 * @return 恢复的线程指针
 */
struct rtos_task *rtos_ipc_object_resume_thread(rtos_ipc_object_t *ipc);

/**
 * @brief 从IPC对象恢复所有线程
 * @param ipc IPC对象指针
 * @return 恢复的线程数量
 */
uint32_t rtos_ipc_object_resume_all_thread(rtos_ipc_object_t *ipc);

/* 内联辅助函数 */

/**
 * @brief 获取对象容器指针
 */
static inline rtos_object_t *rtos_list_object(rtos_object_t *list)
{
    return list->next;
}

/**
 * @brief 检查对象是否为静态对象
 */
static inline bool rtos_object_is_static(rtos_object_t *object)
{
    return (object->flag & RTOS_OBJECT_FLAG_STATIC) != 0;
}

/**
 * @brief 检查对象是否为动态对象
 */
static inline bool rtos_object_is_dynamic(rtos_object_t *object)
{
    return (object->flag & RTOS_OBJECT_FLAG_DYNAMIC) != 0;
}

/* 对象容器访问宏 */
#define rtos_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define rtos_list_entry(node, type, member) \
    rtos_container_of(node, type, member)

/* 对象遍历宏 */
#define rtos_list_for_each_entry(pos, head, member)              \
    for (pos = rtos_list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head);                                   \
         pos = rtos_list_entry(pos->member.next, typeof(*pos), member))

#define rtos_list_for_each_entry_safe(pos, n, head, member)      \
    for (pos = rtos_list_entry((head)->next, typeof(*pos), member), \
         n = rtos_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                   \
         pos = n, n = rtos_list_entry(n->member.next, typeof(*n), member))

#endif /* __RTOS_OBJECT_H__ */