/**
 * @file rtos_timer.h
 * @brief RTOS定时器模块头文件 - 基于内核对象实现
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_TIMER_H__
#define __RTOS_TIMER_H__

#include "rtos_object.h"

/**
 * @brief 定时器控制块结构体
 * 继承自内核对象基类
 */
typedef struct rtos_timer {
    struct rtos_object  parent;                 /* 继承对象基类 */
    
    /* 定时器参数 */
    rtos_time_ns_t      timeout_tick;           /* 超时时间 */
    rtos_time_ns_t      init_tick;              /* 初始时间 */
    uint8_t             flag;                   /* 定时器标志 */
    
    /* 回调函数 */
    void (*timeout_func)(void *parameter);      /* 超时回调函数 */
    void               *parameter;              /* 回调参数 */
    
    /* 定时器状态 */
    struct rtos_timer  *next;                   /* 定时器链表 */
} rtos_timer_t;

/* 定时器标志 */
#define RTOS_TIMER_FLAG_DEACTIVATED    0x0     /* 停止状态 */
#define RTOS_TIMER_FLAG_ACTIVATED      0x1     /* 激活状态 */
#define RTOS_TIMER_FLAG_ONE_SHOT       0x0     /* 单次定时器 */
#define RTOS_TIMER_FLAG_PERIODIC       0x2     /* 周期定时器 */

/* 定时器API函数声明 */
rtos_result_t rtos_timer_init(rtos_timer_t *timer,
                             const char   *name,
                             void (*timeout)(void *parameter),
                             void         *parameter,
                             rtos_time_ns_t time,
                             uint8_t       flag);

rtos_timer_t *rtos_timer_create(const char *name,
                               void (*timeout)(void *parameter),
                               void       *parameter,
                               rtos_time_ns_t time,
                               uint8_t     flag);

rtos_result_t rtos_timer_delete(rtos_timer_t *timer);
rtos_result_t rtos_timer_start(rtos_timer_t *timer);
rtos_result_t rtos_timer_stop(rtos_timer_t *timer);
rtos_result_t rtos_timer_control(rtos_timer_t *timer, int cmd, void *arg);

#endif /* __RTOS_TIMER_H__ */