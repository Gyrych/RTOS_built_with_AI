/**
 * @file config.h
 * @brief RTOS系统配置 - 重构后的配置管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_CONFIG_H__
#define __RTOS_CONFIG_H__

/* 系统功能开关 */
#define RTOS_CONFIG_TASK_MANAGEMENT     1   /* 任务管理 */
#define RTOS_CONFIG_SCHEDULER           1   /* 调度器 */
#define RTOS_CONFIG_SEMAPHORE           1   /* 信号量 */
#define RTOS_CONFIG_MUTEX               1   /* 互斥量 */
#define RTOS_CONFIG_QUEUE               1   /* 消息队列 */
#define RTOS_CONFIG_EVENT_GROUP         1   /* 事件组 */
#define RTOS_CONFIG_SW_TIMER            1   /* 软件定时器 */
#define RTOS_CONFIG_MEMORY_POOL         1   /* 内存池 */
#define RTOS_CONFIG_DYNAMIC_MEMORY      1   /* 动态内存 */
#define RTOS_CONFIG_MPU_PROTECTION      0   /* MPU保护 */
#define RTOS_CONFIG_POWER_MANAGEMENT    0   /* 功耗管理 */
#define RTOS_CONFIG_DEBUG_MONITORING    1   /* 调试监控 */
#define RTOS_CONFIG_SYSTEM_TRACE        0   /* 系统跟踪 */

/* 系统限制配置 */
#define RTOS_CONFIG_MAX_TASKS           16  /* 最大任务数 */
#define RTOS_CONFIG_MAX_SEMAPHORES      8   /* 最大信号量数 */
#define RTOS_CONFIG_MAX_MUTEXES         8   /* 最大互斥量数 */
#define RTOS_CONFIG_MAX_QUEUES          8   /* 最大消息队列数 */
#define RTOS_CONFIG_MAX_SW_TIMERS       16  /* 最大软件定时器数 */
#define RTOS_CONFIG_MAX_EVENT_GROUPS    8   /* 最大事件组数 */
#define RTOS_CONFIG_MAX_MEMORY_POOLS    8   /* 最大内存池数 */
#define RTOS_CONFIG_MAX_PRIORITY        32  /* 最大优先级数 */

/* 内存配置 */
#define RTOS_CONFIG_STACK_SIZE_MIN      256 /* 最小栈大小 */
#define RTOS_CONFIG_STACK_SIZE_DEFAULT  1024 /* 默认栈大小 */
#define RTOS_CONFIG_STACK_SIZE_MAX      65536 /* 最大栈大小 */
#define RTOS_CONFIG_HEAP_SIZE           16384 /* 堆大小 */

/* 时间配置 */
#define RTOS_CONFIG_TIMESLICE_DEFAULT   10000 /* 默认时间片(纳秒) */
#define RTOS_CONFIG_TIMER_RESOLUTION    1000  /* 定时器分辨率(纳秒) */

/* 调试配置 */
#define RTOS_CONFIG_DEBUG               1   /* 调试功能 */
#define RTOS_CONFIG_ASSERT              1   /* 断言检查 */
#define RTOS_CONFIG_STACK_CHECK         1   /* 栈检查 */
#define RTOS_CONFIG_PERFORMANCE_MONITOR 1   /* 性能监控 */

/* 硬件特定配置 */
#define RTOS_CONFIG_CPU_FREQUENCY       168000000 /* CPU频率(Hz) */
#define RTOS_CONFIG_FPU_ENABLE          1   /* FPU使能 */
#define RTOS_CONFIG_MPU_ENABLE          0   /* MPU使能 */

/* 编译配置 */
#define RTOS_CONFIG_STATIC_ALLOCATION   1   /* 静态分配 */
#define RTOS_CONFIG_DYNAMIC_ALLOCATION  1   /* 动态分配 */
#define RTOS_CONFIG_INLINE_FUNCTIONS    1   /* 内联函数 */

/* 兼容性配置 */
#define RTOS_CONFIG_LEGACY_API          0   /* 遗留API支持 */
#define RTOS_CONFIG_FREERTOS_COMPAT     0   /* FreeRTOS兼容性 */

/* 性能优化配置 */
#define RTOS_CONFIG_OPTIMIZE_SCHEDULER  1   /* 优化调度器 */
#define RTOS_CONFIG_OPTIMIZE_CONTEXT_SWITCH 1 /* 优化上下文切换 */
#define RTOS_CONFIG_OPTIMIZE_MEMORY     1   /* 优化内存使用 */

/* 安全配置 */
#define RTOS_CONFIG_STACK_OVERFLOW_CHECK 1  /* 栈溢出检查 */
#define RTOS_CONFIG_MEMORY_PROTECTION    0  /* 内存保护 */
#define RTOS_CONFIG_ACCESS_CONTROL       0  /* 访问控制 */

/* 实时性配置 */
#define RTOS_CONFIG_MAX_ISR_LATENCY     5   /* 最大中断延迟(微秒) */
#define RTOS_CONFIG_MAX_SCHEDULER_LATENCY 10 /* 最大调度器延迟(微秒) */
#define RTOS_CONFIG_PRIORITY_INHERITANCE 1  /* 优先级继承 */

/* 错误处理配置 */
#define RTOS_CONFIG_ERROR_HANDLING      1   /* 错误处理 */
#define RTOS_CONFIG_ERROR_RECOVERY      1   /* 错误恢复 */
#define RTOS_CONFIG_ERROR_REPORTING     1   /* 错误报告 */

/* 统计信息配置 */
#define RTOS_CONFIG_TASK_STATS          1   /* 任务统计 */
#define RTOS_CONFIG_SYSTEM_STATS        1   /* 系统统计 */
#define RTOS_CONFIG_MEMORY_STATS        1   /* 内存统计 */
#define RTOS_CONFIG_PERFORMANCE_STATS   1   /* 性能统计 */

/* 钩子函数配置 */
#define RTOS_CONFIG_IDLE_HOOK           1   /* 空闲钩子 */
#define RTOS_CONFIG_TICK_HOOK           1   /* 滴答钩子 */
#define RTOS_CONFIG_TASK_SWITCH_HOOK    1   /* 任务切换钩子 */

/* 时间管理配置 */
#define RTOS_CONFIG_HIGH_RESOLUTION_TIMER 1 /* 高分辨率定时器 */
#define RTOS_CONFIG_ABSOLUTE_TIME       1   /* 绝对时间支持 */
#define RTOS_CONFIG_RELATIVE_TIME       1   /* 相对时间支持 */

/* 同步机制配置 */
#define RTOS_CONFIG_BINARY_SEMAPHORE    1   /* 二进制信号量 */
#define RTOS_CONFIG_COUNTING_SEMAPHORE  1   /* 计数信号量 */
#define RTOS_CONFIG_MUTEX               1   /* 互斥量 */
#define RTOS_CONFIG_RECURSIVE_MUTEX     1   /* 递归互斥量 */
#define RTOS_CONFIG_MESSAGE_QUEUE       1   /* 消息队列 */
#define RTOS_CONFIG_EVENT_FLAGS         1   /* 事件标志 */

/* 内存管理配置 */
#define RTOS_CONFIG_STATIC_MEMORY       1   /* 静态内存 */
#define RTOS_CONFIG_DYNAMIC_MEMORY      1   /* 动态内存 */
#define RTOS_CONFIG_MEMORY_POOL         1   /* 内存池 */
#define RTOS_CONFIG_MEMORY_HEAP         1   /* 内存堆 */

/* 中断配置 */
#define RTOS_CONFIG_INTERRUPT_PRIORITY  1   /* 中断优先级 */
#define RTOS_CONFIG_INTERRUPT_NESTING   1   /* 中断嵌套 */
#define RTOS_CONFIG_INTERRUPT_SAFE      1   /* 中断安全 */

/* 功耗管理配置 */
#define RTOS_CONFIG_SLEEP_MODE          0   /* 睡眠模式 */
#define RTOS_CONFIG_WAKEUP_SOURCES      0   /* 唤醒源 */
#define RTOS_CONFIG_CPU_FREQUENCY_SCALING 0 /* CPU频率调节 */

/* 通信配置 */
#define RTOS_CONFIG_INTER_TASK_COMM     1   /* 任务间通信 */
#define RTOS_CONFIG_SIGNAL_HANDLING     1   /* 信号处理 */
#define RTOS_CONFIG_MESSAGE_PASSING     1   /* 消息传递 */

/* 文件系统配置 */
#define RTOS_CONFIG_FILE_SYSTEM         0   /* 文件系统 */
#define RTOS_CONFIG_DEVICE_DRIVERS      0   /* 设备驱动 */
#define RTOS_CONFIG_NETWORK_STACK       0   /* 网络协议栈 */

/* 测试配置 */
#define RTOS_CONFIG_UNIT_TESTING        0   /* 单元测试 */
#define RTOS_CONFIG_INTEGRATION_TESTING 0   /* 集成测试 */
#define RTOS_CONFIG_PERFORMANCE_TESTING 1   /* 性能测试 */

/* 文档配置 */
#define RTOS_CONFIG_API_DOCUMENTATION   1   /* API文档 */
#define RTOS_CONFIG_CODE_COMMENTS       1   /* 代码注释 */
#define RTOS_CONFIG_EXAMPLES            1   /* 示例代码 */

#endif /* __RTOS_CONFIG_H__ */
