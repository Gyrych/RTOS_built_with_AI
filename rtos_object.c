/**
 * @file rtos_object.c
 * @brief RTOS内核对象管理器实现
 */

#include "rtos_object.h"
#include "rtos_kernel.h"
#include <string.h>

/* 对象容器数组 */
static rtos_object_information_t rtos_object_container[RTOS_OBJECT_CLASS_UNKNOWN];
static bool object_system_initialized = false;

/* 内部函数声明 */
static void rtos_object_attach(rtos_object_t *object, rtos_object_information_t *information);
static void rtos_object_detach_from_container(rtos_object_t *object, rtos_object_information_t *information);

/**
 * @brief 初始化对象系统
 */
void rtos_object_system_init(void)
{
    int i;
    
    /* 清零对象容器 */
    memset(rtos_object_container, 0, sizeof(rtos_object_container));
    
    /* 初始化各类对象容器 */
    for (i = 0; i < RTOS_OBJECT_CLASS_UNKNOWN; i++) {
        rtos_object_container[i].type = (rtos_object_class_type_t)i;
        rtos_object_container[i].object_list = NULL;
        rtos_object_container[i].object_size = 0;
        rtos_object_container[i].object_count = 0;
        rtos_object_container[i].object_max_count = 0;
    }
    
    /* 设置具体对象类型的大小和最大数量 */
    rtos_object_container[RTOS_OBJECT_CLASS_THREAD].object_size = sizeof(rtos_task_t);
    rtos_object_container[RTOS_OBJECT_CLASS_THREAD].object_max_count = RTOS_MAX_TASKS;
    
    rtos_object_container[RTOS_OBJECT_CLASS_SEMAPHORE].object_size = sizeof(rtos_semaphore_t);
    rtos_object_container[RTOS_OBJECT_CLASS_SEMAPHORE].object_max_count = RTOS_MAX_SEMAPHORES;
    
    rtos_object_container[RTOS_OBJECT_CLASS_MUTEX].object_size = sizeof(rtos_mutex_t);
    rtos_object_container[RTOS_OBJECT_CLASS_MUTEX].object_max_count = RTOS_MAX_MUTEXES;
    
    rtos_object_container[RTOS_OBJECT_CLASS_MESSAGEQUEUE].object_size = sizeof(rtos_queue_t);
    rtos_object_container[RTOS_OBJECT_CLASS_MESSAGEQUEUE].object_max_count = RTOS_MAX_QUEUES;
    
    rtos_object_container[RTOS_OBJECT_CLASS_EVENT].object_size = sizeof(rtos_event_group_t);
    rtos_object_container[RTOS_OBJECT_CLASS_EVENT].object_max_count = RTOS_MAX_EVENT_GROUPS;
    
    rtos_object_container[RTOS_OBJECT_CLASS_MEMPOOL].object_size = sizeof(rtos_memory_pool_t);
    rtos_object_container[RTOS_OBJECT_CLASS_MEMPOOL].object_max_count = RTOS_MAX_MEMORY_POOLS;
    
    rtos_object_container[RTOS_OBJECT_CLASS_TIMER].object_size = sizeof(rtos_sw_timer_t);
    rtos_object_container[RTOS_OBJECT_CLASS_TIMER].object_max_count = RTOS_MAX_SW_TIMERS;
    
    object_system_initialized = true;
}

/**
 * @brief 初始化对象
 */
void rtos_object_init(rtos_object_t          *object,
                     rtos_object_class_type_t type,
                     const char              *name)
{
    if (!object) return;
    
    /* 设置对象基本信息 */
    object->type = type;
    object->flag = RTOS_OBJECT_FLAG_STATIC;
    
    /* 复制对象名称 */
    if (name) {
        strncpy(object->name, name, sizeof(object->name) - 1);
        object->name[sizeof(object->name) - 1] = '\0';
    } else {
        strcpy(object->name, "NoName");
    }
    
    /* 将对象加入到对象容器中 */
    if (type < RTOS_OBJECT_CLASS_UNKNOWN) {
        rtos_object_attach(object, &rtos_object_container[type]);
    }
}

/**
 * @brief 分离对象
 */
rtos_result_t rtos_object_detach(rtos_object_t *object)
{
    if (!object || !rtos_object_is_static(object)) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 从对象容器中移除 */
    if (object->type < RTOS_OBJECT_CLASS_UNKNOWN) {
        rtos_object_detach_from_container(object, &rtos_object_container[object->type]);
    }
    
    return RTOS_OK;
}

/**
 * @brief 分配对象
 */
rtos_object_t *rtos_object_allocate(rtos_object_class_type_t type,
                                   const char              *name)
{
    rtos_object_t *object;
    rtos_object_information_t *information;
    
    if (type >= RTOS_OBJECT_CLASS_UNKNOWN) {
        return NULL;
    }
    
    information = &rtos_object_container[type];
    
    /* 检查对象数量限制 */
    if (information->object_count >= information->object_max_count) {
        return NULL;
    }
    
    /* 分配内存 */
    object = (rtos_object_t *)rtos_malloc(information->object_size);
    if (!object) {
        return NULL;
    }
    
    /* 清零对象内存 */
    memset(object, 0, information->object_size);
    
    /* 初始化对象 */
    object->type = type;
    object->flag = RTOS_OBJECT_FLAG_DYNAMIC;
    
    /* 复制对象名称 */
    if (name) {
        strncpy(object->name, name, sizeof(object->name) - 1);
        object->name[sizeof(object->name) - 1] = '\0';
    } else {
        strcpy(object->name, "NoName");
    }
    
    /* 将对象加入到对象容器中 */
    rtos_object_attach(object, information);
    
    return object;
}

/**
 * @brief 删除对象
 */
void rtos_object_delete(rtos_object_t *object)
{
    if (!object || !rtos_object_is_dynamic(object)) {
        return;
    }
    
    /* 从对象容器中移除 */
    if (object->type < RTOS_OBJECT_CLASS_UNKNOWN) {
        rtos_object_detach_from_container(object, &rtos_object_container[object->type]);
    }
    
    /* 释放内存 */
    rtos_free(object);
}

/**
 * @brief 查找对象
 */
rtos_object_t *rtos_object_find(rtos_object_class_type_t type,
                               const char              *name)
{
    rtos_object_t *object;
    rtos_object_information_t *information;
    
    if (type >= RTOS_OBJECT_CLASS_UNKNOWN || !name) {
        return NULL;
    }
    
    information = &rtos_object_container[type];
    
    /* 遍历对象链表查找 */
    object = information->object_list;
    while (object) {
        if (strcmp(object->name, name) == 0) {
            return object;
        }
        object = object->next;
    }
    
    return NULL;
}

/**
 * @brief 获取对象类型
 */
rtos_object_class_type_t rtos_object_get_type(rtos_object_t *object)
{
    if (!object) {
        return RTOS_OBJECT_CLASS_UNKNOWN;
    }
    
    return object->type;
}

/**
 * @brief 获取对象名称
 */
const char *rtos_object_get_name(rtos_object_t *object)
{
    if (!object) {
        return NULL;
    }
    
    return object->name;
}

/**
 * @brief 获取对象信息
 */
rtos_object_information_t *rtos_object_get_information(rtos_object_class_type_t type)
{
    if (type >= RTOS_OBJECT_CLASS_UNKNOWN) {
        return NULL;
    }
    
    return &rtos_object_container[type];
}

/**
 * @brief 遍历指定类型的所有对象
 */
void rtos_object_for_each(rtos_object_class_type_t type,
                         rtos_object_iterator_t   iterator,
                         void                    *data)
{
    rtos_object_t *object;
    rtos_object_information_t *information;
    
    if (type >= RTOS_OBJECT_CLASS_UNKNOWN || !iterator) {
        return;
    }
    
    information = &rtos_object_container[type];
    
    /* 遍历对象链表 */
    object = information->object_list;
    while (object) {
        rtos_object_t *next = object->next;
        
        /* 调用迭代器函数 */
        if (!iterator(object, data)) {
            break;
        }
        
        object = next;
    }
}

/**
 * @brief 检查对象有效性
 */
bool rtos_object_is_valid(rtos_object_t *object, rtos_object_class_type_t type)
{
    if (!object) {
        return false;
    }
    
    return (object->type == type);
}

/**
 * @brief 获取对象容器链表头
 */
rtos_object_t *rtos_object_get_list(rtos_object_class_type_t type)
{
    if (type >= RTOS_OBJECT_CLASS_UNKNOWN) {
        return NULL;
    }
    
    return rtos_object_container[type].object_list;
}

/**
 * @brief 初始化IPC对象
 */
void rtos_ipc_object_init(rtos_ipc_object_t       *ipc,
                         rtos_object_class_type_t type,
                         const char              *name)
{
    if (!ipc) return;
    
    /* 初始化基类对象 */
    rtos_object_init(&(ipc->parent), type, name);
    
    /* 初始化IPC特有字段 */
    ipc->suspend_thread = NULL;
}

/**
 * @brief 挂起线程到IPC对象
 */
rtos_result_t rtos_ipc_object_suspend_thread(rtos_ipc_object_t *ipc,
                                             struct rtos_task  *thread,
                                             uint8_t           flag,
                                             int32_t           timeout)
{
    if (!ipc || !thread) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 挂起线程 */
    thread->state = TASK_STATE_BLOCKED;
    
    /* 从就绪队列移除 */
    rtos_remove_task_from_ready_list(thread);
    
    /* 按优先级顺序插入到等待列表 */
    if (ipc->suspend_thread == NULL || 
        ipc->suspend_thread->priority > thread->priority) {
        thread->next = ipc->suspend_thread;
        ipc->suspend_thread = thread;
    } else {
        struct rtos_task *current = ipc->suspend_thread;
        while (current->next != NULL && 
               current->next->priority <= thread->priority) {
            current = current->next;
        }
        thread->next = current->next;
        current->next = thread;
    }
    
    /* 如果有超时，设置定时器 */
    if (timeout > 0) {
        /* 设置超时定时器的简化实现 */
        thread->delay_time_ns = (rtos_time_ns_t)timeout * 1000;
    }
    
    return RTOS_OK;
}

/**
 * @brief 从IPC对象恢复线程
 */
struct rtos_task *rtos_ipc_object_resume_thread(rtos_ipc_object_t *ipc)
{
    struct rtos_task *thread;
    
    if (!ipc || !ipc->suspend_thread) {
        return NULL;
    }
    
    /* 从等待列表取出第一个线程 */
    thread = ipc->suspend_thread;
    ipc->suspend_thread = thread->next;
    thread->next = NULL;
    
    /* 恢复线程到就绪状态 */
    thread->state = TASK_STATE_READY;
    rtos_add_task_to_ready_list(thread);
    
    return thread;
}

/**
 * @brief 从IPC对象恢复所有线程
 */
uint32_t rtos_ipc_object_resume_all_thread(rtos_ipc_object_t *ipc)
{
    uint32_t count = 0;
    struct rtos_task *thread;
    
    if (!ipc) {
        return 0;
    }
    
    while (ipc->suspend_thread) {
        thread = rtos_ipc_object_resume_thread(ipc);
        if (thread) {
            count++;
        }
    }
    
    return count;
}

/* ========== 内部函数实现 ========== */

/**
 * @brief 将对象加入到对象容器
 */
static void rtos_object_attach(rtos_object_t *object, rtos_object_information_t *information)
{
    if (!object || !information) return;
    
    rtos_enter_critical();
    
    /* 加入到链表头 */
    object->prev = NULL;
    object->next = information->object_list;
    
    if (information->object_list) {
        information->object_list->prev = object;
    }
    information->object_list = object;
    
    /* 增加对象计数 */
    information->object_count++;
    
    rtos_exit_critical();
}

/**
 * @brief 从对象容器中移除对象
 */
static void rtos_object_detach_from_container(rtos_object_t *object, rtos_object_information_t *information)
{
    if (!object || !information) return;
    
    rtos_enter_critical();
    
    /* 从双向链表中移除 */
    if (object->prev) {
        object->prev->next = object->next;
    } else {
        information->object_list = object->next;
    }
    
    if (object->next) {
        object->next->prev = object->prev;
    }
    
    object->prev = NULL;
    object->next = NULL;
    
    /* 减少对象计数 */
    if (information->object_count > 0) {
        information->object_count--;
    }
    
    rtos_exit_critical();
}