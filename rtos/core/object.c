/**
 * @file object.c
 * @brief RTOS内核对象基类实现
 * @author Assistant
 * @date 2024
 */

#include "object.h"
#include <string.h>
#include <stddef.h>

/* 全局对象容器数组 */
static rtos_object_information_t g_object_containers[RTOS_OBJECT_TYPE_DEVICE + 1];

/* 全局对象链表头 */
static rtos_object_t g_object_list_head = {0};

/**
 * @brief 初始化对象
 */
void rtos_object_init(rtos_object_t *object, 
                     rtos_object_type_t type,
                     const char *name,
                     rtos_object_flag_t flags)
{
    if (object == NULL) {
        return;
    }
    
    /* 设置对象基本信息 */
    strncpy(object->name, name ? name : "unnamed", sizeof(object->name) - 1);
    object->name[sizeof(object->name) - 1] = '\0';
    object->type = type;
    object->flags = flags;
    object->create_time = 0; /* 将在系统时钟初始化后设置 */
    object->ref_count = 1;   /* 初始引用计数为1 */
    
    /* 初始化链表指针 */
    object->next = NULL;
    object->prev = NULL;
    
    /* 添加到全局对象链表 */
    if (g_object_list_head.next == NULL) {
        /* 第一个对象，初始化链表头 */
        g_object_list_head.next = object;
        g_object_list_head.prev = object;
        object->next = &g_object_list_head;
        object->prev = &g_object_list_head;
    } else {
        /* 添加到链表末尾 */
        object->next = &g_object_list_head;
        object->prev = g_object_list_head.prev;
        g_object_list_head.prev->next = object;
        g_object_list_head.prev = object;
    }
}

/**
 * @brief 设置对象名称
 */
void rtos_object_set_name(rtos_object_t *object, const char *name)
{
    if (object == NULL || name == NULL) {
        return;
    }
    
    strncpy(object->name, name, sizeof(object->name) - 1);
    object->name[sizeof(object->name) - 1] = '\0';
}

/**
 * @brief 获取对象名称
 */
const char *rtos_object_get_name(const rtos_object_t *object)
{
    if (object == NULL) {
        return NULL;
    }
    
    return object->name;
}

/**
 * @brief 获取对象类型
 */
rtos_object_type_t rtos_object_get_type(const rtos_object_t *object)
{
    if (object == NULL) {
        return RTOS_OBJECT_TYPE_NULL;
    }
    
    return object->type;
}

/**
 * @brief 获取对象标志
 */
rtos_object_flag_t rtos_object_get_flags(const rtos_object_t *object)
{
    if (object == NULL) {
        return RTOS_OBJECT_FLAG_NONE;
    }
    
    return object->flags;
}

/**
 * @brief 设置对象标志
 */
void rtos_object_set_flags(rtos_object_t *object, rtos_object_flag_t flags)
{
    if (object == NULL) {
        return;
    }
    
    object->flags = flags;
}

/**
 * @brief 检查对象是否为静态对象
 */
bool rtos_object_is_static(const rtos_object_t *object)
{
    if (object == NULL) {
        return false;
    }
    
    return (object->flags & RTOS_OBJECT_FLAG_STATIC) != 0;
}

/**
 * @brief 检查对象是否为动态对象
 */
bool rtos_object_is_dynamic(const rtos_object_t *object)
{
    if (object == NULL) {
        return false;
    }
    
    return (object->flags & RTOS_OBJECT_FLAG_DYNAMIC) != 0;
}

/**
 * @brief 检查对象是否为系统对象
 */
bool rtos_object_is_system(const rtos_object_t *object)
{
    if (object == NULL) {
        return false;
    }
    
    return (object->flags & RTOS_OBJECT_FLAG_SYSTEM) != 0;
}

/* 引用计数管理 */

/**
 * @brief 增加对象引用计数
 */
uint32_t rtos_object_ref_inc(rtos_object_t *object)
{
    if (object == NULL) {
        return 0;
    }
    
    object->ref_count++;
    return object->ref_count;
}

/**
 * @brief 减少对象引用计数
 */
uint32_t rtos_object_ref_dec(rtos_object_t *object)
{
    if (object == NULL) {
        return 0;
    }
    
    if (object->ref_count > 0) {
        object->ref_count--;
    }
    
    return object->ref_count;
}

/**
 * @brief 获取对象引用计数
 */
uint32_t rtos_object_get_ref_count(const rtos_object_t *object)
{
    if (object == NULL) {
        return 0;
    }
    
    return object->ref_count;
}

/* 对象容器管理函数 */

/**
 * @brief 初始化对象容器
 */
void rtos_object_container_init(rtos_object_information_t *info,
                              rtos_object_type_t type,
                              uint32_t max_count)
{
    if (info == NULL) {
        return;
    }
    
    info->type = type;
    info->count = 0;
    info->max_count = max_count;
    info->first = NULL;
    info->last = NULL;
}

/**
 * @brief 添加对象到容器
 */
rtos_result_t rtos_object_container_add(rtos_object_information_t *info,
                                       rtos_object_t *object)
{
    if (info == NULL || object == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (info->count >= info->max_count) {
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    if (info->first == NULL) {
        /* 第一个对象 */
        info->first = object;
        info->last = object;
        object->next = object;
        object->prev = object;
    } else {
        /* 添加到末尾 */
        object->next = info->first;
        object->prev = info->last;
        info->last->next = object;
        info->first->prev = object;
        info->last = object;
    }
    
    info->count++;
    return RTOS_OK;
}

/**
 * @brief 从容器移除对象
 */
rtos_result_t rtos_object_container_remove(rtos_object_information_t *info,
                                          rtos_object_t *object)
{
    if (info == NULL || object == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (info->count == 0) {
        return RTOS_ERROR;
    }
    
    if (info->count == 1) {
        /* 最后一个对象 */
        if (info->first == object) {
            info->first = NULL;
            info->last = NULL;
            object->next = NULL;
            object->prev = NULL;
            info->count = 0;
            return RTOS_OK;
        }
    } else {
        /* 从链表中移除 */
        object->prev->next = object->next;
        object->next->prev = object->prev;
        
        if (info->first == object) {
            info->first = object->next;
        }
        if (info->last == object) {
            info->last = object->prev;
        }
        
        object->next = NULL;
        object->prev = NULL;
        info->count--;
        return RTOS_OK;
    }
    
    return RTOS_ERROR;
}

/**
 * @brief 查找对象
 */
rtos_object_t *rtos_object_container_find(const rtos_object_information_t *info,
                                         const char *name)
{
    if (info == NULL || name == NULL || info->count == 0) {
        return NULL;
    }
    
    rtos_object_t *current = info->first;
    uint32_t count = 0;
    
    do {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
        count++;
    } while (current != info->first && count < info->count);
    
    return NULL;
}

/**
 * @brief 获取容器统计信息
 */
uint32_t rtos_object_container_get_count(const rtos_object_information_t *info)
{
    if (info == NULL) {
        return 0;
    }
    
    return info->count;
}

/**
 * @brief 遍历容器中的对象
 */
void rtos_object_container_traverse(const rtos_object_information_t *info,
                                   void (*callback)(rtos_object_t *object, void *arg),
                                   void *arg)
{
    if (info == NULL || callback == NULL || info->count == 0) {
        return;
    }
    
    rtos_object_t *current = info->first;
    uint32_t count = 0;
    
    do {
        callback(current, arg);
        current = current->next;
        count++;
    } while (current != info->first && count < info->count);
}

/* 等待队列管理 */

/**
 * @brief 初始化等待队列
 */
void rtos_wait_queue_init(rtos_wait_node_t *head)
{
    if (head == NULL) {
        return;
    }
    
    head->task = NULL;
    head->timeout = 0;
    head->next = head;
    head->prev = head;
}

/**
 * @brief 添加任务到等待队列
 */
rtos_result_t rtos_wait_queue_add(rtos_wait_node_t *head,
                                  struct rtos_task *task,
                                  rtos_timeout_t timeout)
{
    if (head == NULL || task == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_wait_node_t *node = (rtos_wait_node_t *)malloc(sizeof(rtos_wait_node_t));
    if (node == NULL) {
        return RTOS_ERROR_NO_MEMORY;
    }
    
    node->task = task;
    node->timeout = timeout;
    
    /* 添加到队列末尾 */
    node->next = head;
    node->prev = head->prev;
    head->prev->next = node;
    head->prev = node;
    
    return RTOS_OK;
}

/**
 * @brief 从等待队列移除任务
 */
rtos_result_t rtos_wait_queue_remove(rtos_wait_node_t *head,
                                     struct rtos_task *task)
{
    if (head == NULL || task == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_wait_node_t *current = head->next;
    
    while (current != head) {
        if (current->task == task) {
            /* 从链表中移除 */
            current->prev->next = current->next;
            current->next->prev = current->prev;
            free(current);
            return RTOS_OK;
        }
        current = current->next;
    }
    
    return RTOS_ERROR;
}

/**
 * @brief 获取等待队列中的第一个任务
 */
struct rtos_task *rtos_wait_queue_get_first(rtos_wait_node_t *head)
{
    if (head == NULL || head->next == head) {
        return NULL;
    }
    
    return head->next->task;
}

/**
 * @brief 检查等待队列是否为空
 */
bool rtos_wait_queue_is_empty(const rtos_wait_node_t *head)
{
    if (head == NULL) {
        return true;
    }
    
    return (head->next == head);
}

/**
 * @brief 获取等待队列长度
 */
uint32_t rtos_wait_queue_get_length(const rtos_wait_node_t *head)
{
    if (head == NULL) {
        return 0;
    }
    
    uint32_t count = 0;
    rtos_wait_node_t *current = head->next;
    
    while (current != head) {
        count++;
        current = current->next;
    }
    
    return count;
}

/* 系统初始化函数 */

/**
 * @brief 初始化对象系统
 */
void rtos_object_system_init(void)
{
    /* 初始化全局对象链表头 */
    memset(&g_object_list_head, 0, sizeof(g_object_list_head));
    g_object_list_head.next = &g_object_list_head;
    g_object_list_head.prev = &g_object_list_head;
    
    /* 初始化所有对象容器 */
    for (int i = 0; i <= RTOS_OBJECT_TYPE_DEVICE; i++) {
        rtos_object_container_init(&g_object_containers[i], i, 0);
    }
}

/**
 * @brief 获取对象容器
 */
rtos_object_information_t *rtos_object_get_container(rtos_object_type_t type)
{
    if (type > RTOS_OBJECT_TYPE_DEVICE) {
        return NULL;
    }
    
    return &g_object_containers[type];
}

/**
 * @brief 获取全局对象链表头
 */
rtos_object_t *rtos_object_get_list_head(void)
{
    return &g_object_list_head;
}

/* 等待队列辅助函数实现 */

/**
 * @brief 初始化等待节点
 */
void rtos_wait_node_init(rtos_wait_node_t *node, struct rtos_task *task, void *data, rtos_wait_flag_t flags)
{
    if (!node) {
        return;
    }
    
    node->task = task;
    node->timeout = 0;
    node->data = data;
    node->flags = flags;
    node->next = NULL;
    node->prev = NULL;
}

/**
 * @brief 获取等待节点的任务
 */
struct rtos_task *rtos_wait_node_get_task(const rtos_wait_node_t *node)
{
    if (!node) {
        return NULL;
    }
    
    return node->task;
}

/**
 * @brief 获取等待节点的数据
 */
void *rtos_wait_node_get_data(const rtos_wait_node_t *node)
{
    if (!node) {
        return NULL;
    }
    
    return node->data;
}

/**
 * @brief 获取等待节点的标志
 */
rtos_wait_flag_t rtos_wait_node_get_flags(const rtos_wait_node_t *node)
{
    if (!node) {
        return RTOS_WAIT_FLAG_NONE;
    }
    
    return node->flags;
}

/**
 * @brief 唤醒所有等待的任务
 */
void rtos_wait_queue_wake_all(rtos_wait_node_t *head, rtos_result_t result)
{
    if (!head) {
        return;
    }
    
    rtos_wait_node_t *current = head->next;
    rtos_wait_node_t *next;
    
    while (current != head) {
        next = current->next;
        
        if (current->task) {
            /* 唤醒任务 */
            extern void rtos_task_add_to_ready_queue_from_wait(struct rtos_task *task);
            rtos_task_add_to_ready_queue_from_wait(current->task);
        }
        
        /* 从队列中移除 */
        current->prev->next = current->next;
        current->next->prev = current->prev;
        
        current = next;
    }
}

/**
 * @brief 等待节点等待
 */
rtos_result_t rtos_wait_node_wait(rtos_wait_node_t *node, rtos_timeout_t timeout)
{
    if (!node || !node->task) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 简化实现：设置任务为阻塞状态 */
    node->timeout = timeout;
    
    /* 这里应该实现具体的等待逻辑 */
    /* 暂时返回成功 */
    return RTOS_OK;
}
