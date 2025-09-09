#ifndef __CORE_H__
#define __CORE_H__

#include <stdint.h>
#include <stdio.h>

/* RTOS核心头文件 - 定义任务管理和调度器接口 */

/* 调试功能配置 */
#define RTOS_DEBUG_ENABLE 1    /* 启用RTOS调试功能 */
#define RTOS_DEBUG_LEVEL  2    /* 调试级别: 0=关闭, 1=基本, 2=详细, 3=完整 */

#if RTOS_DEBUG_ENABLE
    #define RTOS_DEBUG_PRINT(level, format, ...) \
        do { \
            if (level <= RTOS_DEBUG_LEVEL) { \
                printf("[RTOS-DEBUG] " format "\r\n", ##__VA_ARGS__); \
            } \
        } while(0)

    #define RTOS_DEBUG_PRINT_TASK(level, task, format, ...) \
        do { \
            if (level <= RTOS_DEBUG_LEVEL && task) { \
                printf("[RTOS-DEBUG] Task@%p(P%d,S%d) " format "\r\n", \
                       task, task->priority, task->state, ##__VA_ARGS__); \
            } \
        } while(0)
#else
    #define RTOS_DEBUG_PRINT(level, format, ...)
    #define RTOS_DEBUG_PRINT_TASK(level, task, format, ...)
#endif

#define MAX_TASKS 32         /* 最大任务数量 */
#define MAX_PRIORITY 31     /* 最大优先级值 (0最高, 31最低) */
#define STACK_SIZE 256      /* 每个任务的堆栈大小 */

/* 中断优先级定义 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    5   /* 系统调用中断优先级 */
#define configKERNEL_INTERRUPT_PRIORITY         15  /* 内核中断优先级 */

#define TASK_READY 0        /* 任务就绪状态 */
#define TASK_RUNNING 1      /* 任务运行状态 */
#define TASK_SUSPENDED 2    /* 任务挂起状态 */

/* 任务控制块结构体（为简化汇编访问，将 stack_ptr 置于偏移 0） */
typedef struct {
    uint32_t* stack_ptr;       /* 当前堆栈指针（偏移 0） */
    void (*task_func)(void*);  /* 任务函数指针 */
    void* arg;                 /* 任务参数 */
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

/* 在C侧暴露给汇编使用的全局指针（便于汇编读取，不要用硬编码偏移） */
extern volatile task_t * volatile pxCurrentTCB;
extern volatile task_t * volatile pxNextTCB;
extern volatile task_t * volatile * const pxSchedulerCurrentTaskPtr; /* 指向 scheduler.current_task 的指针 */

/* SVC 编号定义 */
#define SVC_YIELD               0   /* 线程态让出/请求调度 */
#define SVC_START_FIRST_TASK    1   /* 启动首任务 */

void rtos_init(void);        /* RTOS初始化 */
void rtos_start(void);       /* 启动RTOS调度 */
void rtos_schedule(void);    /* 调度器核心函数 */
void rtos_request_context_switch_from_isr(void); /* 在中断中请求上下文切换 */
int  rtos_schedule_decide_next(void);            /* 进行一次调度决策，返回是否需要切换 */

task_t* task_create(void (*func)(void*), void* arg, uint32_t priority);  /* 创建新任务 */
void task_suspend(task_t* task);  /* 挂起指定任务 */
void task_resume(task_t* task);   /* 恢复挂起的任务 */
void task_delete(task_t* task);   /* 删除任务 */
task_t* find_highest_priority_task(void);  /* 查找最高优先级任务 */

void __attribute__((naked)) pend_sv_handler(void);  /* PendSV中断处理函数 */
void __attribute__((naked)) svc_handler(void);       /* SVC中断处理函数 */

/* 中断控制函数 */
void rtos_enter_critical(void);     /* 进入临界区 */
void rtos_exit_critical(void);      /* 退出临界区 */

/* 调试相关函数 */
void rtos_debug_print_scheduler_info(void);          /* 打印调度器信息 */
void rtos_debug_print_task_info(task_t* task);       /* 打印任务信息 */
void rtos_debug_print_stack_usage(task_t* task);     /* 打印堆栈使用情况 */
const char* rtos_debug_get_state_name(uint8_t state); /* 获取状态名称 */

#endif