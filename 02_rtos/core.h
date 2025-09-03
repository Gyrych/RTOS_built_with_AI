#ifndef __CORE_H__
#define __CORE_H__

#include <stdint.h>

/* RTOS核心头文件 - 定义任务管理和调度器接口 */

#define MAX_TASKS 32         /* 最大任务数量 */
#define MAX_PRIORITY 31     /* 最大优先级值 (0最高, 31最低) */
#define STACK_SIZE 256      /* 每个任务的堆栈大小 */

#define TASK_READY 0        /* 任务就绪状态 */
#define TASK_RUNNING 1      /* 任务运行状态 */
#define TASK_SUSPENDED 2    /* 任务挂起状态 */

/* 任务控制块结构体 */
typedef struct {
    void (*task_func)(void*);  /* 任务函数指针 */
    void* arg;                 /* 任务参数 */
    uint32_t* stack_ptr;       /* 当前堆栈指针 */
    uint32_t priority;         /* 任务优先级 */
    uint8_t state;             /* 任务状态 */
    uint32_t stack[STACK_SIZE]; /* 任务堆栈空间 */
} task_t;

/* 调度器结构体 */
typedef struct {
    task_t* tasks[MAX_TASKS];  /* 任务指针数组 */
    uint8_t task_count;        /* 当前任务数量 */
    task_t* current_task;      /* 当前运行的任务 */
} scheduler_t;

extern scheduler_t scheduler;  /* 全局调度器实例 */

void rtos_init(void);        /* RTOS初始化 */
void rtos_start(void);       /* 启动RTOS调度 */
void rtos_schedule(void);    /* 调度器核心函数 */

task_t* task_create(void (*func)(void*), void* arg, uint32_t priority);  /* 创建新任务 */
void task_suspend(task_t* task);  /* 挂起指定任务 */
void task_resume(task_t* task);   /* 恢复挂起的任务 */
void task_delete(task_t* task);   /* 删除任务 */
task_t* find_highest_priority_task(void);  /* 查找最高优先级任务 */

void __attribute__((naked)) pend_sv_handler(void);  /* PendSV中断处理函数 */
void __attribute__((naked)) svc_handler(void);       /* SVC中断处理函数 */

#endif