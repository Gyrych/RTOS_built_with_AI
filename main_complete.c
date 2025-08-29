/**
 * @file main_complete.c
 * @brief RTOS完整功能演示程序 - 展示所有重构后的模块功能
 * @author Assistant
 * @date 2024
 */

#include "rtos.h"
#include <stdio.h>
#include <string.h>

/* 任务句柄 */
static rtos_task_t g_task1, g_task2, g_task3, g_task4;
static rtos_semaphore_t g_semaphore;
static rtos_mutex_t g_mutex;
static rtos_queue_t g_queue;
static rtos_event_group_t g_event_group;
static rtos_sw_timer_t g_timer1, g_timer2;
static rtos_memory_pool_t g_memory_pool;

/* 任务栈 */
static uint8_t g_task1_stack[1024];
static uint8_t g_task2_stack[1024];
static uint8_t g_task3_stack[1024];
static uint8_t g_task4_stack[1024];

/* 内存池缓冲区 */
static uint8_t g_mempool_buffer[4096];

/* 队列缓冲区 */
static uint8_t g_queue_buffer[256];

/* 任务函数声明 */
static void task1_function(void *parameter);
static void task2_function(void *parameter);
static void task3_function(void *parameter);
static void task4_function(void *parameter);

/* 定时器回调函数 */
static void timer1_callback(void *parameter);
static void timer2_callback(void *parameter);

/* 系统钩子函数 */
static void system_startup_hook(void);
static void system_idle_hook(void);
static void system_shutdown_hook(void);

/**
 * @brief 主函数
 */
int main(void)
{
    rtos_result_t result;
    
    printf("=== RTOS完整功能演示程序 ===\n");
    
    /* 设置系统钩子函数 */
    rtos_system_set_startup_hook(system_startup_hook);
    rtos_system_set_idle_hook(system_idle_hook);
    rtos_system_set_shutdown_hook(system_shutdown_hook);
    
    /* 初始化RTOS系统 */
    printf("初始化RTOS系统...\n");
    result = rtos_system_init();
    if (result != RTOS_SUCCESS) {
        printf("系统初始化失败: %d\n", result);
        return -1;
    }
    printf("系统初始化成功\n");
    
    /* 创建同步对象 */
    printf("创建同步对象...\n");
    
    /* 创建信号量 */
    rtos_semaphore_create_params_t sem_params = {
        .name = "DemoSem",
        .initial_count = 1,
        .max_count = 5
    };
    result = rtos_semaphore_init(&g_semaphore, &sem_params);
    if (result != RTOS_SUCCESS) {
        printf("信号量初始化失败: %d\n", result);
        return -1;
    }
    
    /* 创建互斥量 */
    rtos_mutex_create_params_t mutex_params = {
        .name = "DemoMutex",
        .recursive = true,
        .priority_inheritance = true
    };
    result = rtos_mutex_init(&g_mutex, &mutex_params);
    if (result != RTOS_SUCCESS) {
        printf("互斥量初始化失败: %d\n", result);
        return -1;
    }
    
    /* 创建消息队列 */
    rtos_queue_create_params_t queue_params = {
        .name = "DemoQueue",
        .item_size = 32,
        .max_items = 8
    };
    result = rtos_queue_init(&g_queue, &queue_params, g_queue_buffer);
    if (result != RTOS_SUCCESS) {
        printf("消息队列初始化失败: %d\n", result);
        return -1;
    }
    
    /* 创建事件组 */
    rtos_event_group_create_params_t event_params = {
        .name = "DemoEvent",
        .initial_bits = RTOS_EVENT_BIT_0
    };
    result = rtos_event_group_init(&g_event_group, &event_params);
    if (result != RTOS_SUCCESS) {
        printf("事件组初始化失败: %d\n", result);
        return -1;
    }
    
    /* 创建内存池 */
    rtos_memory_pool_create_params_t mempool_params = {
        .name = "DemoPool",
        .block_size = 64,
        .block_count = 16
    };
    result = rtos_memory_pool_init(&g_memory_pool, &mempool_params, g_mempool_buffer);
    if (result != RTOS_SUCCESS) {
        printf("内存池初始化失败: %d\n", result);
        return -1;
    }
    
    /* 创建定时器 */
    rtos_sw_timer_create_params_t timer1_params = {
        .name = "Timer1",
        .callback = timer1_callback,
        .parameter = NULL,
        .period = RTOS_TIMER_PERIOD_100MS,
        .auto_reload = true
    };
    result = rtos_sw_timer_init(&g_timer1, &timer1_params);
    if (result != RTOS_SUCCESS) {
        printf("定时器1初始化失败: %d\n", result);
        return -1;
    }
    
    rtos_sw_timer_create_params_t timer2_params = {
        .name = "Timer2",
        .callback = timer2_callback,
        .parameter = NULL,
        .period = RTOS_TIMER_PERIOD_1S,
        .auto_reload = false
    };
    result = rtos_sw_timer_init(&g_timer2, &timer2_params);
    if (result != RTOS_SUCCESS) {
        printf("定时器2初始化失败: %d\n", result);
        return -1;
    }
    
    printf("同步对象创建成功\n");
    
    /* 创建任务 */
    printf("创建任务...\n");
    
    rtos_task_create_params_t task1_params = {
        .name = "Task1",
        .function = task1_function,
        .parameter = NULL,
        .priority = RTOS_TASK_PRIORITY_HIGH,
        .stack = g_task1_stack,
        .stack_size = sizeof(g_task1_stack)
    };
    result = rtos_task_create_static(&g_task1, &task1_params);
    if (result != RTOS_SUCCESS) {
        printf("任务1创建失败: %d\n", result);
        return -1;
    }
    
    rtos_task_create_params_t task2_params = {
        .name = "Task2",
        .function = task2_function,
        .parameter = NULL,
        .priority = RTOS_TASK_PRIORITY_NORMAL,
        .stack = g_task2_stack,
        .stack_size = sizeof(g_task2_stack)
    };
    result = rtos_task_create_static(&g_task2, &task2_params);
    if (result != RTOS_SUCCESS) {
        printf("任务2创建失败: %d\n", result);
        return -1;
    }
    
    rtos_task_create_params_t task3_params = {
        .name = "Task3",
        .function = task3_function,
        .parameter = NULL,
        .priority = RTOS_TASK_PRIORITY_NORMAL,
        .stack = g_task3_stack,
        .stack_size = sizeof(g_task3_stack)
    };
    result = rtos_task_create_static(&g_task3, &task3_params);
    if (result != RTOS_SUCCESS) {
        printf("任务3创建失败: %d\n", result);
        return -1;
    }
    
    rtos_task_create_params_t task4_params = {
        .name = "Task4",
        .function = task4_function,
        .parameter = NULL,
        .priority = RTOS_TASK_PRIORITY_LOW,
        .stack = g_task4_stack,
        .stack_size = sizeof(g_task4_stack)
    };
    result = rtos_task_create_static(&g_task4, &task4_params);
    if (result != RTOS_SUCCESS) {
        printf("任务4创建失败: %d\n", result);
        return -1;
    }
    
    printf("任务创建成功\n");
    
    /* 启动定时器 */
    printf("启动定时器...\n");
    rtos_sw_timer_start(&g_timer1);
    rtos_sw_timer_start(&g_timer2);
    
    /* 启动任务 */
    printf("启动任务...\n");
    rtos_task_start(&g_task1);
    rtos_task_start(&g_task2);
    rtos_task_start(&g_task3);
    rtos_task_start(&g_task4);
    
    /* 启动RTOS系统 */
    printf("启动RTOS系统...\n");
    result = rtos_system_start();
    if (result != RTOS_SUCCESS) {
        printf("系统启动失败: %d\n", result);
        return -1;
    }
    
    /* 主循环 */
    printf("系统运行中...\n");
    while (1) {
        /* 主循环中可以进行一些系统级操作 */
        rtos_hw_delay_ms(1000);
        
        /* 显示系统信息 */
        rtos_system_stats_t stats;
        if (rtos_system_get_stats(&stats) == RTOS_SUCCESS) {
            printf("系统运行时间: %llu ms\n", stats.system_uptime);
        }
    }
    
    return 0;
}

/**
 * @brief 任务1函数 - 高优先级任务，演示信号量和互斥量
 */
static void task1_function(void *parameter)
{
    (void)parameter;
    uint32_t counter = 0;
    
    printf("任务1启动\n");
    
    while (1) {
        /* 获取信号量 */
        if (rtos_semaphore_take(&g_semaphore, RTOS_WAIT_FOREVER) == RTOS_SUCCESS) {
            printf("任务1获取信号量，计数器: %u\n", counter);
            
            /* 获取互斥量 */
            if (rtos_mutex_take(&g_mutex, RTOS_WAIT_FOREVER) == RTOS_SUCCESS) {
                printf("任务1获取互斥量\n");
                
                /* 模拟临界区操作 */
                rtos_hw_delay_ms(10);
                
                rtos_mutex_give(&g_mutex);
                printf("任务1释放互斥量\n");
            }
            
            rtos_semaphore_give(&g_semaphore);
            printf("任务1释放信号量\n");
        }
        
        counter++;
        rtos_task_delay(RTOS_MS_TO_TICKS(200));
    }
}

/**
 * @brief 任务2函数 - 演示消息队列和事件组
 */
static void task2_function(void *parameter)
{
    (void)parameter;
    uint32_t counter = 0;
    char message[32];
    
    printf("任务2启动\n");
    
    while (1) {
        /* 发送消息到队列 */
        snprintf(message, sizeof(message), "消息%d", counter);
        if (rtos_queue_send(&g_queue, message, strlen(message) + 1, RTOS_WAIT_FOREVER) == RTOS_SUCCESS) {
            printf("任务2发送消息: %s\n", message);
        }
        
        /* 设置事件位 */
        if (counter % 5 == 0) {
            rtos_event_group_set_bits(&g_event_group, RTOS_EVENT_BIT_1);
            printf("任务2设置事件位1\n");
        }
        
        counter++;
        rtos_task_delay(RTOS_MS_TO_TICKS(300));
    }
}

/**
 * @brief 任务3函数 - 演示内存池和事件等待
 */
static void task3_function(void *parameter)
{
    (void)parameter;
    uint32_t counter = 0;
    void *memory_block;
    
    printf("任务3启动\n");
    
    while (1) {
        /* 从内存池分配内存 */
        memory_block = rtos_memory_pool_alloc(&g_memory_pool, RTOS_WAIT_FOREVER);
        if (memory_block) {
            printf("任务3分配内存块: %p\n", memory_block);
            
            /* 模拟使用内存 */
            memset(memory_block, counter & 0xFF, 64);
            
            /* 释放内存 */
            rtos_memory_pool_free(&g_memory_pool, memory_block);
            printf("任务3释放内存块\n");
        }
        
        /* 等待事件位 */
        if (counter % 3 == 0) {
            rtos_event_bits_t bits = rtos_event_group_wait_bits(&g_event_group, 
                                                               RTOS_EVENT_BIT_1, 
                                                               true, false, RTOS_WAIT_FOREVER);
            if (bits & RTOS_EVENT_BIT_1) {
                printf("任务3收到事件位1\n");
            }
        }
        
        counter++;
        rtos_task_delay(RTOS_MS_TO_TICKS(400));
    }
}

/**
 * @brief 任务4函数 - 低优先级任务，演示消息接收
 */
static void task4_function(void *parameter)
{
    (void)parameter;
    char message[32];
    size_t received_size;
    
    printf("任务4启动\n");
    
    while (1) {
        /* 从队列接收消息 */
        if (rtos_queue_receive(&g_queue, message, sizeof(message), &received_size, RTOS_WAIT_FOREVER) == RTOS_SUCCESS) {
            printf("任务4接收消息: %s (大小: %zu)\n", message, received_size);
        }
        
        rtos_task_delay(RTOS_MS_TO_TICKS(500));
    }
}

/**
 * @brief 定时器1回调函数 - 100ms周期
 */
static void timer1_callback(void *parameter)
{
    (void)parameter;
    static uint32_t counter = 0;
    
    counter++;
    printf("定时器1触发: %u\n", counter);
    
    /* 设置事件位 */
    rtos_event_group_set_bits(&g_event_group, RTOS_EVENT_BIT_2);
}

/**
 * @brief 定时器2回调函数 - 1秒单次
 */
static void timer2_callback(void *parameter)
{
    (void)parameter;
    
    printf("定时器2触发 - 单次定时器\n");
    
    /* 设置事件位 */
    rtos_event_group_set_bits(&g_event_group, RTOS_EVENT_BIT_3);
}

/**
 * @brief 系统启动钩子函数
 */
static void system_startup_hook(void)
{
    printf("=== 系统启动钩子执行 ===\n");
    
    /* 显示硬件信息 */
    char hw_info[256];
    uint32_t len = rtos_hw_get_info_string(hw_info, sizeof(hw_info));
    if (len > 0) {
        printf("硬件信息:\n%s\n", hw_info);
    }
}

/**
 * @brief 系统空闲钩子函数
 */
static void system_idle_hook(void)
{
    static uint32_t idle_count = 0;
    
    idle_count++;
    if (idle_count % 1000 == 0) {
        printf("系统空闲计数: %u\n", idle_count);
    }
}

/**
 * @brief 系统关闭钩子函数
 */
static void system_shutdown_hook(void)
{
    printf("=== 系统关闭钩子执行 ===\n");
    
    /* 停止定时器 */
    rtos_sw_timer_stop(&g_timer1);
    rtos_sw_timer_stop(&g_timer2);
    
    /* 删除任务 */
    rtos_task_delete(&g_task1);
    rtos_task_delete(&g_task2);
    rtos_task_delete(&g_task3);
    rtos_task_delete(&g_task4);
    
    /* 删除同步对象 */
    rtos_semaphore_delete(&g_semaphore);
    rtos_mutex_delete(&g_mutex);
    rtos_queue_delete(&g_queue);
    rtos_event_group_delete(&g_event_group);
    rtos_memory_pool_delete(&g_memory_pool);
    rtos_sw_timer_delete(&g_timer1);
    rtos_sw_timer_delete(&g_timer2);
    
    printf("系统清理完成\n");
}
