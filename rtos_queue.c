/**
 * @file rtos_queue.c
 * @brief RTOS消息队列模块实现 - 基于IPC对象
 */

#include "rtos_queue.h"
#include "rtos_object.h"
#include "rtos_task.h"
#include "rtos_kernel.h"
#include <string.h>

/* 消息结构体 */
typedef struct rtos_message {
    struct rtos_message *next;
} rtos_message_t;

/* 内部函数声明 */
static void *rtos_mq_alloc_msg(rtos_messagequeue_t *mq);
static void rtos_mq_free_msg(rtos_messagequeue_t *mq, void *msg);
static void rtos_mq_put_msg(rtos_messagequeue_t *mq, void *msg);
static void *rtos_mq_get_msg(rtos_messagequeue_t *mq);

/**
 * @brief 初始化消息队列 - 静态方式
 */
rtos_result_t rtos_mq_init(rtos_messagequeue_t *mq,
                          const char          *name,
                          void                *msgpool,
                          uint32_t             msg_size,
                          uint32_t             pool_size,
                          uint8_t              flag)
{
    rtos_message_t *head;
    register rtos_message_t *temp;
    register char *msg_ptr;
    register uint32_t i;
    
    if (!mq || !msgpool || msg_size == 0 || pool_size == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 初始化IPC对象基类 */
    rtos_ipc_object_init(&(mq->parent), RTOS_OBJECT_CLASS_MESSAGEQUEUE, name);
    
    /* 初始化消息队列特有字段 */
    mq->msg_pool = msgpool;
    mq->msg_size = msg_size;
    mq->max_msgs = pool_size / (msg_size + sizeof(rtos_message_t));
    mq->entry = 0;
    mq->msg_queue_head = NULL;
    mq->msg_queue_tail = NULL;
    mq->sender_thread = NULL;
    mq->receiver_thread = NULL;
    
    /* 初始化空闲消息链表 */
    msg_ptr = (char *)msgpool;
    head = (rtos_message_t *)msg_ptr;
    mq->msg_queue_free = head;
    
    for (i = 0; i < mq->max_msgs; i++) {
        temp = (rtos_message_t *)msg_ptr;
        if (i == mq->max_msgs - 1) {
            temp->next = NULL;
        } else {
            temp->next = (rtos_message_t *)(msg_ptr + msg_size + sizeof(rtos_message_t));
        }
        msg_ptr += msg_size + sizeof(rtos_message_t);
    }
    
    return RTOS_OK;
}

/**
 * @brief 创建消息队列 - 动态方式
 */
rtos_messagequeue_t *rtos_mq_create(const char *name,
                                   uint32_t    msg_size,
                                   uint32_t    max_msgs,
                                   uint8_t     flag)
{
    rtos_messagequeue_t *mq;
    void *msgpool;
    uint32_t pool_size;
    
    /* 分配消息队列对象 */
    mq = (rtos_messagequeue_t *)rtos_object_allocate(RTOS_OBJECT_CLASS_MESSAGEQUEUE, name);
    if (!mq) {
        return NULL;
    }
    
    /* 分配消息池 */
    pool_size = max_msgs * (msg_size + sizeof(rtos_message_t));
    msgpool = rtos_malloc(pool_size);
    if (!msgpool) {
        rtos_object_delete(&(mq->parent.parent));
        return NULL;
    }
    
    /* 初始化消息队列 */
    if (rtos_mq_init(mq, name, msgpool, msg_size, pool_size, flag) != RTOS_OK) {
        rtos_free(msgpool);
        rtos_object_delete(&(mq->parent.parent));
        return NULL;
    }
    
    return mq;
}

/**
 * @brief 发送消息到队列
 */
rtos_result_t rtos_mq_send(rtos_messagequeue_t *mq,
                          const void          *buffer,
                          uint32_t             size,
                          rtos_time_ns_t       timeout)
{
    rtos_message_t *msg;
    rtos_task_t *task;
    
    if (!rtos_mq_is_valid(mq) || !buffer || size == 0 || size > mq->msg_size) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 如果有接收等待的任务，直接传递消息 */
    if (mq->receiver_thread) {
        task = mq->receiver_thread;
        mq->receiver_thread = task->tlist;
        task->tlist = NULL;
        
        /* 直接拷贝消息到任务的接收缓冲区 */
        memcpy((void *)task->user_data, buffer, size);
        task->error = size; /* 返回消息大小 */
        
        /* 恢复任务 */
        task->stat = RTOS_TASK_READY;
        rtos_schedule_insert_task(task);
        
        rtos_exit_critical();
        rtos_schedule();
        return RTOS_OK;
    }
    
    /* 分配消息空间 */
    msg = (rtos_message_t *)rtos_mq_alloc_msg(mq);
    if (!msg) {
        if (timeout == RTOS_WAITING_NO) {
            rtos_exit_critical();
            return RTOS_ERROR_TIMEOUT;
        }
        
        /* 队列已满，挂起当前任务 */
        task = rtos_task_self();
        if (!task) {
            rtos_exit_critical();
            return RTOS_ERROR;
        }
        
        /* 保存消息信息 */
        task->user_data = (uint32_t)buffer;
        task->parameter = (void *)size;
        
        /* 挂起任务 */
        task->stat = RTOS_TASK_BLOCK;
        rtos_schedule_remove_task(task);
        
        /* 加入发送等待队列 */
        task->tlist = mq->sender_thread;
        mq->sender_thread = task;
        
        rtos_exit_critical();
        rtos_schedule();
        
        /* 被唤醒后检查结果 */
        return task->error;
    }
    
    /* 复制消息内容 */
    memcpy((char *)msg + sizeof(rtos_message_t), buffer, size);
    
    /* 将消息放入队列 */
    rtos_mq_put_msg(mq, msg);
    
    rtos_exit_critical();
    return RTOS_OK;
}

/**
 * @brief 从队列接收消息
 */
int rtos_mq_recv(rtos_messagequeue_t *mq,
                void                *buffer,
                uint32_t             size,
                rtos_time_ns_t       timeout)
{
    rtos_message_t *msg;
    rtos_task_t *task;
    uint32_t msg_size;
    
    if (!rtos_mq_is_valid(mq) || !buffer || size == 0) {
        return -RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 获取消息 */
    msg = (rtos_message_t *)rtos_mq_get_msg(mq);
    if (msg) {
        /* 复制消息内容 */
        msg_size = (size < mq->msg_size) ? size : mq->msg_size;
        memcpy(buffer, (char *)msg + sizeof(rtos_message_t), msg_size);
        
        /* 释放消息 */
        rtos_mq_free_msg(mq, msg);
        
        /* 如果有发送等待的任务，唤醒 */
        if (mq->sender_thread) {
            task = mq->sender_thread;
            mq->sender_thread = task->tlist;
            task->tlist = NULL;
            
            task->error = RTOS_OK;
            task->stat = RTOS_TASK_READY;
            rtos_schedule_insert_task(task);
        }
        
        rtos_exit_critical();
        return msg_size;
    }
    
    /* 队列为空 */
    if (timeout == RTOS_WAITING_NO) {
        rtos_exit_critical();
        return -RTOS_ERROR_TIMEOUT;
    }
    
    /* 挂起当前任务等待消息 */
    task = rtos_task_self();
    if (!task) {
        rtos_exit_critical();
        return -RTOS_ERROR;
    }
    
    /* 保存接收缓冲区信息 */
    task->user_data = (uint32_t)buffer;
    task->parameter = (void *)size;
    
    /* 挂起任务 */
    task->stat = RTOS_TASK_BLOCK;
    rtos_schedule_remove_task(task);
    
    /* 加入接收等待队列 */
    task->tlist = mq->receiver_thread;
    mq->receiver_thread = task;
    
    rtos_exit_critical();
    rtos_schedule();
    
    /* 被唤醒后检查结果 */
    return task->error;
}

/**
 * @brief 删除消息队列
 */
rtos_result_t rtos_mq_delete(rtos_messagequeue_t *mq)
{
    void *msgpool;
    
    if (!rtos_mq_is_valid(mq) || !rtos_object_is_dynamic(&(mq->parent.parent))) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    rtos_enter_critical();
    
    /* 保存消息池地址 */
    msgpool = mq->msg_pool;
    
    /* 恢复所有等待的任务 */
    while (mq->sender_thread) {
        rtos_task_t *task = mq->sender_thread;
        mq->sender_thread = task->tlist;
        task->tlist = NULL;
        task->error = RTOS_ERROR;
        task->stat = RTOS_TASK_READY;
        rtos_schedule_insert_task(task);
    }
    
    while (mq->receiver_thread) {
        rtos_task_t *task = mq->receiver_thread;
        mq->receiver_thread = task->tlist;
        task->tlist = NULL;
        task->error = -RTOS_ERROR;
        task->stat = RTOS_TASK_READY;
        rtos_schedule_insert_task(task);
    }
    
    rtos_exit_critical();
    
    /* 释放消息池 */
    if (msgpool) {
        rtos_free(msgpool);
    }
    
    /* 从对象系统删除 */
    rtos_object_delete(&(mq->parent.parent));
    
    /* 触发调度 */
    rtos_schedule();
    
    return RTOS_OK;
}

/* ========== 内部函数实现 ========== */

static void *rtos_mq_alloc_msg(rtos_messagequeue_t *mq)
{
    rtos_message_t *msg;
    
    if (mq->msg_queue_free) {
        msg = (rtos_message_t *)mq->msg_queue_free;
        mq->msg_queue_free = msg->next;
        return msg;
    }
    
    return NULL;
}

static void rtos_mq_free_msg(rtos_messagequeue_t *mq, void *msg)
{
    rtos_message_t *message = (rtos_message_t *)msg;
    
    message->next = (rtos_message_t *)mq->msg_queue_free;
    mq->msg_queue_free = message;
}

static void rtos_mq_put_msg(rtos_messagequeue_t *mq, void *msg)
{
    rtos_message_t *message = (rtos_message_t *)msg;
    
    message->next = NULL;
    
    if (mq->msg_queue_tail) {
        ((rtos_message_t *)mq->msg_queue_tail)->next = message;
    } else {
        mq->msg_queue_head = message;
    }
    mq->msg_queue_tail = message;
    mq->entry++;
}

static void *rtos_mq_get_msg(rtos_messagequeue_t *mq)
{
    rtos_message_t *msg;
    
    if (mq->msg_queue_head) {
        msg = (rtos_message_t *)mq->msg_queue_head;
        mq->msg_queue_head = msg->next;
        
        if (!mq->msg_queue_head) {
            mq->msg_queue_tail = NULL;
        }
        
        mq->entry--;
        return msg;
    }
    
    return NULL;
}