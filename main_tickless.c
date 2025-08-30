/**
 * @file main_tickless.c
 * @brief RTOS Tickless系统演示程序
 * @author Assistant
 * @date 2024
 */

#include "rtos.h"
#include <stdio.h>
#include <string.h>

/* 任务栈定义 */
static uint32_t high_priority_task_stack[1024];
static uint32_t medium_priority_task_stack[1024];
static uint32_t low_priority_task_stack[1024];

/* 任务控制块定义 */
static rtos_task_t high_priority_task;
static rtos_task_t medium_priority_task;
static rtos_task_t low_priority_task;

/* 同步对象定义 */
static rtos_semaphore_t preemption_sem;
static rtos_mutex_t shared_resource_mutex;

/* 软件定时器定义 */
static rtos_sw_timer_t periodic_timer;
static rtos_sw_timer_t oneshot_timer;

/* 任务函数声明 */
static void high_priority_task_entry(void *parameter);
static void medium_priority_task_entry(void *parameter);
static void low_priority_task_entry(void *parameter);

/* 定时器回调函数 */
static void periodic_timer_callback(void *parameter);
static void oneshot_timer_callback(void *parameter);

/* 系统钩子函数 */
static void tickless_startup_hook(void);
static void tickless_idle_hook(void);

/**
 * @brief 主函数
 */
int main(void)
{
    printf("=== RTOS Tickless系统演示程序 ===\n");
    printf("版本: %s\n", rtos_system_get_version());
    printf("特性: 无滴答时钟 + 完全抢占式调度 + 动态定时器延时\n\n");
    
    /* 设置系统钩子函数 */
    rtos_system_set_startup_hook(tickless_startup_hook);
    rtos_system_set_idle_hook(tickless_idle_hook);
    
    /* 初始化RTOS系统 */
    rtos_result_t result = rtos_system_init();
    if (result != RTOS_OK) {
        printf("ERROR: RTOS系统初始化失败: %d\n", result);
        return -1;
    }
    printf("✓ RTOS系统初始化成功\n");
    
    /* 创建高优先级任务 - 演示抢占式调度 */
    rtos_task_create_params_t high_task_params = {
        .name = "HighPriorityTask",
        .entry = high_priority_task_entry,
        .parameter = NULL,
        .stack_size = sizeof(high_priority_task_stack),
        .priority = RTOS_PRIORITY_CRITICAL,  /* 最高优先级 */
        .timeslice = RTOS_TIMESLICE_INF,     /* 无时间片限制 */
        .flags = RTOS_TASK_FLAG_NONE
    };
    
    result = rtos_task_create_static(&high_priority_task, &high_task_params);
    if (result != RTOS_OK) {
        printf("ERROR: 高优先级任务创建失败: %d\n", result);
        return -1;
    }
    printf("✓ 高优先级任务创建成功\n");
    
    /* 创建中优先级任务 */
    rtos_task_create_params_t medium_task_params = {
        .name = "MediumPriorityTask",
        .entry = medium_priority_task_entry,
        .parameter = NULL,
        .stack_size = sizeof(medium_priority_task_stack),
        .priority = RTOS_PRIORITY_NORMAL,
        .timeslice = RTOS_TIMESLICE_DEFAULT,
        .flags = RTOS_TASK_FLAG_NONE
    };
    
    result = rtos_task_create_static(&medium_priority_task, &medium_task_params);
    if (result != RTOS_OK) {
        printf("ERROR: 中优先级任务创建失败: %d\n", result);
        return -1;
    }
    printf("✓ 中优先级任务创建成功\n");
    
    /* 创建低优先级任务 */
    rtos_task_create_params_t low_task_params = {
        .name = "LowPriorityTask",
        .entry = low_priority_task_entry,
        .parameter = NULL,
        .stack_size = sizeof(low_priority_task_stack),
        .priority = RTOS_PRIORITY_LOW,
        .timeslice = RTOS_TIMESLICE_DEFAULT,
        .flags = RTOS_TASK_FLAG_NONE
    };
    
    result = rtos_task_create_static(&low_priority_task, &low_task_params);
    if (result != RTOS_OK) {
        printf("ERROR: 低优先级任务创建失败: %d\n", result);
        return -1;
    }
    printf("✓ 低优先级任务创建成功\n");
    
    /* 创建抢占演示信号量 */
    rtos_semaphore_create_params_t sem_params = {
        .name = "PreemptionSem",
        .initial_count = 0,
        .max_count = 1
    };
    
    result = rtos_semaphore_init(&preemption_sem, &sem_params);
    if (result != RTOS_OK) {
        printf("ERROR: 信号量创建失败: %d\n", result);
        return -1;
    }
    printf("✓ 抢占演示信号量创建成功\n");
    
    /* 创建共享资源互斥量 */
    rtos_mutex_create_params_t mutex_params = {
        .name = "SharedResourceMutex",
        .recursive = false,
        .ceiling_priority = RTOS_PRIORITY_CRITICAL
    };
    
    result = rtos_mutex_init(&shared_resource_mutex, &mutex_params);
    if (result != RTOS_OK) {
        printf("ERROR: 互斥量创建失败: %d\n", result);
        return -1;
    }
    printf("✓ 共享资源互斥量创建成功\n");
    
    /* 创建周期性定时器 - 演示动态定时器延时 */
    rtos_sw_timer_create_params_t periodic_timer_params = {
        .name = "PeriodicTimer",
        .callback = periodic_timer_callback,
        .parameter = NULL,
        .period = RTOS_TIMER_MS_TO_NS(500), /* 500ms周期 */
        .auto_reload = true
    };
    
    result = rtos_sw_timer_init(&periodic_timer, &periodic_timer_params);
    if (result != RTOS_OK) {
        printf("ERROR: 周期性定时器创建失败: %d\n", result);
        return -1;
    }
    printf("✓ 周期性定时器创建成功\n");
    
    /* 创建单次定时器 */
    rtos_sw_timer_create_params_t oneshot_timer_params = {
        .name = "OneshotTimer",
        .callback = oneshot_timer_callback,
        .parameter = NULL,
        .period = RTOS_TIMER_MS_TO_NS(2000), /* 2秒后触发 */
        .auto_reload = false
    };
    
    result = rtos_sw_timer_init(&oneshot_timer, &oneshot_timer_params);
    if (result != RTOS_OK) {
        printf("ERROR: 单次定时器创建失败: %d\n", result);
        return -1;
    }
    printf("✓ 单次定时器创建成功\n");
    
    /* 启动所有任务 */
    rtos_task_start(&high_priority_task);
    rtos_task_start(&medium_priority_task);
    rtos_task_start(&low_priority_task);
    printf("✓ 所有任务启动成功\n");
    
    /* 启动定时器 */
    rtos_sw_timer_start(&periodic_timer);
    rtos_sw_timer_start(&oneshot_timer);
    printf("✓ 定时器启动成功\n");
    
    printf("\n=== 开始Tickless RTOS演示 ===\n");
    
    /* 启动RTOS系统 */
    result = rtos_system_start();
    if (result != RTOS_OK) {
        printf("ERROR: RTOS系统启动失败: %d\n", result);
        return -1;
    }
    
    /* 主函数不应该到达这里 */
    while (1) {
        rtos_system_idle();
    }
    
    return 0;
}

/**
 * @brief 高优先级任务 - 演示抢占式调度
 */
static void high_priority_task_entry(void *parameter)
{
    (void)parameter;
    uint32_t cycle_count = 0;
    
    printf("[HIGH] 高优先级任务开始运行\n");
    
    while (1) {
        /* 等待信号量触发 */
        rtos_result_t result = rtos_semaphore_take(&preemption_sem, RTOS_TIMEOUT_INF);
        if (result == RTOS_OK) {
            printf("[HIGH] 收到信号量，开始执行关键任务 (周期 %u)\n", ++cycle_count);
            
            /* 获取共享资源 */
            result = rtos_mutex_take(&shared_resource_mutex, RTOS_TIMEOUT_INF);
            if (result == RTOS_OK) {
                printf("[HIGH] 获取共享资源，执行关键操作\n");
                
                /* 模拟关键操作 - 使用精确的纳秒级延时 */
                rtos_task_delay_ns(50000000); /* 50ms */
                
                printf("[HIGH] 关键操作完成，释放共享资源\n");
                rtos_mutex_release(&shared_resource_mutex);
            }
            
            printf("[HIGH] 高优先级任务执行完毕，进入等待\n");
        }
    }
}

/**
 * @brief 中优先级任务 - 演示正常调度
 */
static void medium_priority_task_entry(void *parameter)
{
    (void)parameter;
    uint32_t cycle_count = 0;
    
    printf("[MEDIUM] 中优先级任务开始运行\n");
    
    while (1) {
        printf("[MEDIUM] 执行中优先级任务 (周期 %u)\n", ++cycle_count);
        
        /* 尝试获取共享资源 */
        rtos_result_t result = rtos_mutex_take(&shared_resource_mutex, RTOS_TIMER_MS_TO_NS(100));
        if (result == RTOS_OK) {
            printf("[MEDIUM] 获取共享资源成功\n");
            
            /* 模拟工作负载 */
            rtos_task_delay_us(200000); /* 200ms */
            
            rtos_mutex_release(&shared_resource_mutex);
            printf("[MEDIUM] 释放共享资源\n");
        } else {
            printf("[MEDIUM] 获取共享资源超时\n");
        }
        
        /* 使用动态延时 - 演示tickless特性 */
        rtos_task_delay_ms(800); /* 800ms延时 */
    }
}

/**
 * @brief 低优先级任务 - 演示后台处理
 */
static void low_priority_task_entry(void *parameter)
{
    (void)parameter;
    uint32_t cycle_count = 0;
    
    printf("[LOW] 低优先级任务开始运行\n");
    
    while (1) {
        printf("[LOW] 执行低优先级后台任务 (周期 %u)\n", ++cycle_count);
        
        /* 模拟后台处理工作 */
        for (int i = 0; i < 1000; i++) {
            /* 在循环中检查是否被抢占 */
            if (i % 100 == 0) {
                rtos_task_yield(); /* 主动让出CPU */
            }
        }
        
        /* 长时间延时 - 演示tickless节能 */
        rtos_task_delay_ms(3000); /* 3秒延时 */
    }
}

/**
 * @brief 周期性定时器回调函数
 */
static void periodic_timer_callback(void *parameter)
{
    (void)parameter;
    static uint32_t timer_count = 0;
    
    printf("[TIMER] 周期性定时器触发 (第 %u 次)\n", ++timer_count);
    
    /* 每5次触发释放信号量，演示抢占 */
    if (timer_count % 5 == 0) {
        rtos_semaphore_give(&preemption_sem);
        printf("[TIMER] 释放信号量，触发高优先级任务抢占\n");
    }
}

/**
 * @brief 单次定时器回调函数
 */
static void oneshot_timer_callback(void *parameter)
{
    (void)parameter;
    
    printf("[TIMER] 单次定时器触发 - 系统运行2秒后\n");
    printf("[TIMER] 演示动态定时器设置系统延时功能\n");
    
    /* 重新启动单次定时器，演示动态设置 */
    rtos_sw_timer_set_period(&oneshot_timer, RTOS_TIMER_MS_TO_NS(5000)); /* 改为5秒 */
    rtos_sw_timer_start(&oneshot_timer);
    printf("[TIMER] 重新设置单次定时器为5秒周期\n");
}

/**
 * @brief Tickless系统启动钩子函数
 */
static void tickless_startup_hook(void)
{
    printf("[HOOK] Tickless系统启动钩子执行\n");
    printf("[HOOK] 系统时钟频率: %u Hz\n", rtos_hw_get_cpu_clock_frequency());
    printf("[HOOK] 硬件平台: %d\n", rtos_hw_get_platform());
}

/**
 * @brief Tickless系统空闲钩子函数
 */
static void tickless_idle_hook(void)
{
    static uint32_t idle_count = 0;
    idle_count++;
    
    /* 每10000次空闲循环显示一次信息 */
    if (idle_count % 10000 == 0) {
        printf("[IDLE] 系统空闲处理 (计数: %u)\n", idle_count);
        printf("[IDLE] 当前事件数量: %u\n", rtos_tickless_get_event_count());
        
        /* 显示下一个事件时间 */
        rtos_time_ns_t next_event_time = rtos_tickless_get_next_expire_time();
        if (next_event_time > 0) {
            rtos_time_ns_t current_time = rtos_tickless_get_current_time();
            rtos_time_ns_t delay = next_event_time > current_time ? 
                                  next_event_time - current_time : 0;
            printf("[IDLE] 下一个事件延时: %lu ns\n", (unsigned long)delay);
        }
    }
    
    /* 在空闲时进入低功耗模式 */
    if (idle_count % 1000 == 0) {
        rtos_hw_enter_low_power_mode(0);
    }
}