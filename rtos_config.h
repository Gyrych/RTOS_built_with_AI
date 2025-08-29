/**
 * @file rtos_config.h
 * @brief RTOS系统配置文件
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_CONFIG_H__
#define __RTOS_CONFIG_H__

/* 系统配置参数 */
#define RTOS_MAX_TASKS          16      /* 最大任务数量 */
#define RTOS_MAX_SEMAPHORES     8       /* 最大信号量数量 */
#define RTOS_MAX_MUTEXES        8       /* 最大互斥量数量 */
#define RTOS_MAX_QUEUES         8       /* 最大消息队列数量 */
#define RTOS_STACK_SIZE_MIN     256     /* 最小任务栈大小(字节) */
#define RTOS_PRIORITY_LEVELS    32      /* 优先级级别数 (0-31, 0最高) */
#define RTOS_QUEUE_ITEM_SIZE    32      /* 队列项目大小(字节) */
#define RTOS_MAX_SW_TIMERS      16      /* 最大软件定时器数量 */
#define RTOS_MAX_EVENT_GROUPS   8       /* 最大事件组数量 */
#define RTOS_MAX_MEMORY_POOLS   8       /* 最大内存池数量 */

/* 时间精度配置 */
#define RTOS_TIME_UNIT_NS       1       /* 纳秒级时间单位 */
#define RTOS_TIME_UNIT_US       1000    /* 微秒级时间单位 */
#define RTOS_TIME_UNIT_MS       1000000 /* 毫秒级时间单位 */

/* 功能特性开关 */
#define RTOS_USING_OBJECT_SYSTEM    1   /* 使用对象系统 */
#define RTOS_USING_SEMAPHORE        1   /* 使用信号量 */
#define RTOS_USING_MUTEX            1   /* 使用互斥量 */
#define RTOS_USING_EVENT            1   /* 使用事件组 */
#define RTOS_USING_MESSAGEQUEUE     1   /* 使用消息队列 */
#define RTOS_USING_TIMER            1   /* 使用定时器 */
#define RTOS_USING_MEMPOOL          1   /* 使用内存池 */
#define RTOS_USING_MEMHEAP          0   /* 使用内存堆 */

/* 调试和诊断 */
#define RTOS_DEBUG                  1   /* 开启调试功能 */
#define RTOS_USING_OVERFLOW_CHECK   1   /* 栈溢出检查 */
#define RTOS_USING_TRACE            0   /* 系统跟踪 */

/* 硬件平台配置 */
#define RTOS_CPU_CACHE_LINE_SZ      32  /* CPU缓存行大小 */
#define RTOS_ALIGN_SIZE             4   /* 内存对齐大小 */

/* 任务配置 */
#define RTOS_TASK_PRIORITY_MAX      32  /* 最大优先级数 */
#define RTOS_IDLE_THREAD_STACK_SIZE 512 /* 空闲任务栈大小 */
#define RTOS_MAIN_THREAD_STACK_SIZE 2048/* 主任务栈大小 */

/* 内存管理配置 */
#define RTOS_HEAP_SIZE              (64 * 1024)  /* 堆大小 */
#define RTOS_USING_SMALL_MEM        1   /* 使用小内存算法 */

/* 控制台配置 */
#define RTOS_USING_CONSOLE          1   /* 使用控制台 */
#define RTOS_CONSOLEBUF_SIZE        128 /* 控制台缓冲区大小 */

#endif /* __RTOS_CONFIG_H__ */