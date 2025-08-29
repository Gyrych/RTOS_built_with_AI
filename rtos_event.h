/**
 * @file rtos_event.h
 * @brief RTOS事件组模块头文件 - 基于IPC对象实现
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_EVENT_H__
#define __RTOS_EVENT_H__

#include "rtos_object.h"

/**
 * @brief 事件组控制块结构体
 * 继承自IPC对象基类
 */
typedef struct rtos_event {
    struct rtos_ipc_object parent;              /* 继承IPC对象基类 */
    uint32_t               set;                 /* 事件集合 */
} rtos_event_t;

/* 事件API函数声明 */
rtos_result_t rtos_event_init(rtos_event_t *event, const char *name, uint8_t flag);
rtos_event_t *rtos_event_create(const char *name, uint8_t flag);
rtos_result_t rtos_event_delete(rtos_event_t *event);
rtos_result_t rtos_event_send(rtos_event_t *event, uint32_t set);
rtos_result_t rtos_event_recv(rtos_event_t *event, uint32_t set, uint8_t option, 
                             rtos_time_ns_t timeout, uint32_t *recved);

#endif /* __RTOS_EVENT_H__ */