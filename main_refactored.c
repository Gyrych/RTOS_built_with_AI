/**
 * @file main_refactored.c
 * @brief RTOS重构后功能演示程序
 * @author Assistant
 * @date 2024
 */

#include "rtos.h"
#include <stdio.h>
#include <string.h>

/* 任务栈定义 */
static uint32_t task1_stack[1024];
static uint32_t task2_stack[1024];
static uint32_t task3_stack[1024];

/* 任务控制块定义 */
static rtos_task_t task1;
static rtos_task_t task2;
static rtos_task_t task3;

/* 同步对象定义 */
static rtos_semaphore_t sem1;
static rtos_mutex_t mutex1;
static rtos_queue_t queue1;
static uint8_t queue_buffer[256];

/* 任务函数声明 */
static void task1_entry(void *parameter);
static void task2_entry(void *parameter);
static void task3_entry(void *parameter);

/* 系统钩子函数 */
static void system_startup_hook(void);
static void system_idle_hook(void);

/**
 * @brief 主函数
 */
int main(void)
{
    printf("RTOS重构后功能演示程序启动\n");
    printf("版本: %s\n", rtos_system_get_version());
    
    /* 设置系统钩子函数 */
    rtos_system_set_startup_hook(system_startup_hook);
    rtos_system_set_idle_hook(system_idle_hook);
    
    /* 初始化RTOS系统 */
    rtos_result_t result = rtos_system_init();
    if (result != RTOS_OK) {
        printf("RTOS系统初始化失败: %d\n", result);
        return -1;
    }
    printf("RTOS系统初始化成功\n");
    
    /* 创建任务1 - 高优先级任务 */
    rtos_task_create_params_t task1_params = {
        .name = "Task1",
        .entry = task1_entry,
        .parameter = NULL,
        .stack_size = sizeof(task1_stack),
        .priority = RTOS_PRIORITY_HIGH,
        .timeslice = RTOS_TIMESLICE_DEFAULT,
        .flags = RTOS_TASK_FLAG_NONE
    };
    
    result = rtos_task_create_static(&task1, &task1_params);
    if (result != RTOS_OK) {
        printf("任务1创建失败: %d\n", result);
        return -1;
    }
    printf("任务1创建成功\n");
    
    /* 创建任务2 - 中优先级任务 */
    rtos_task_create_params_t task2_params = {
        .name = "Task2",
        .entry = task2_entry,
        .parameter = NULL,
        .stack_size = sizeof(task2_stack),
        .priority = RTOS_PRIORITY_NORMAL,
        .timeslice = RTOS_TIMESLICE_DEFAULT,
        .flags = RTOS_TASK_FLAG_NONE
    };
    
    result = rtos_task_create_static(&task2, &task2_params);
    if (result != RTOS_OK) {
        printf("任务2创建失败: %d\n", result);
        return -1;
    }
    printf("任务2创建成功\n");
    
    /* 创建任务3 - 低优先级任务 */
    rtos_task_create_params_t task3_params = {
        .name = "Task3",
        .entry = task3_entry,
        .parameter = NULL,
        .stack_size = sizeof(task3_stack),
        .priority = RTOS_PRIORITY_LOW,
        .timeslice = RTOS_TIMESLICE_DEFAULT,
        .flags = RTOS_TASK_FLAG_NONE
    };
    
    result = rtos_task_create_static(&task3, &task3_params);
    if (result != RTOS_OK) {
        printf("任务3创建失败: %d\n", result);
        return -1;
    }
    printf("任务3创建成功\n");
    
    /* 创建信号量 */
    rtos_semaphore_create_params_t sem1_params = {
        .name = "Sem1",
        .initial_count = 0,
        .max_count = 10
    };
    
    result = rtos_semaphore_init(&sem1, &sem1_params);
    if (result != RTOS_OK) {
        printf("信号量创建失败: %d\n", result);
        return -1;
    }
    printf("信号量创建成功\n");
    
    /* 创建互斥量 */
    rtos_mutex_create_params_t mutex1_params = {
        .name = "Mutex1",
        .recursive = true,
        .ceiling_priority = RTOS_PRIORITY_HIGH
    };
    
    result = rtos_mutex_init(&mutex1, &mutex1_params);
    if (result != RTOS_OK) {
        printf("互斥量创建失败: %d\n", result);
        return -1;
    }
    printf("互斥量创建成功\n");
    
    /* 创建消息队列 */
    rtos_queue_create_params_t queue1_params = {
        .name = "Queue1",
        .item_size = 32,
        .max_items = 8
    };
    
    result = rtos_queue_init(&queue1, &queue1_params, queue_buffer);
    if (result != RTOS_OK) {
        printf("消息队列创建失败: %d\n", result);
        return -1;
    }
    printf("消息队列创建成功\n");
    
    /* 启动所有任务 */
    result = rtos_task_start(&task1);
    if (result != RTOS_OK) {
        printf("任务1启动失败: %d\n", result);
        return -1;
    }
    
    result = rtos_task_start(&task2);
    if (result != RTOS_OK) {
        printf("任务2启动失败: %d\n", result);
        return -1;
    }
    
    result = rtos_task_start(&task3);
    if (result != RTOS_OK) {
        printf("任务3启动失败: %d\n", result);
        return -1;
    }
    
    printf("所有任务启动成功\n");
    
    /* 启动RTOS系统 */
    result = rtos_system_start();
    if (result != RTOS_OK) {
        printf("RTOS系统启动失败: %d\n", result);
        return -1;
    }
    
    printf("RTOS系统启动成功\n");
    
    /* 主函数不应该到达这里 */
    while (1) {
        rtos_system_idle();
    }
    
    return 0;
}

/**
 * @brief 任务1入口函数 - 高优先级任务
 */
static void task1_entry(void *parameter)
{
    (void)parameter;
    
    printf("任务1开始运行\n");
    
    while (1) {
        /* 获取互斥量 */
        rtos_result_t result = rtos_mutex_take(&mutex1, RTOS_TIMEOUT_INF);
        if (result == RTOS_OK) {
            printf("任务1获取互斥量\n");
            
            /* 模拟临界区操作 */
            rtos_task_delay_ms(100);
            
            /* 释放互斥量 */
            rtos_mutex_release(&mutex1);
            printf("任务1释放互斥量\n");
        }
        
        /* 发送消息到队列 */
        char message[] = "Hello from Task1";
        result = rtos_queue_send(&queue1, message, strlen(message) + 1, RTOS_TIMEOUT_INF);
        if (result == RTOS_OK) {
            printf("任务1发送消息到队列\n");
        }
        
        /* 延时1秒 */
        rtos_task_delay_ms(1000);
    }
}

/**
 * @brief 任务2入口函数 - 中优先级任务
 */
static void task2_entry(void *parameter)
{
    (void)parameter;
    
    printf("任务2开始运行\n");
    
    while (1) {
        /* 获取互斥量 */
        rtos_result_t result = rtos_mutex_take(&mutex1, RTOS_TIMEOUT_INF);
        if (result == RTOS_OK) {
            printf("任务2获取互斥量\n");
            
            /* 模拟临界区操作 */
            rtos_task_delay_ms(50);
            
            /* 释放互斥量 */
            rtos_mutex_release(&mutex1);
            printf("任务2释放互斥量\n");
        }
        
        /* 释放信号量 */
        result = rtos_semaphore_give(&sem1);
        if (result == RTOS_OK) {
            printf("任务2释放信号量\n");
        }
        
        /* 延时500毫秒 */
        rtos_task_delay_ms(500);
    }
}

/**
 * @brief 任务3入口函数 - 低优先级任务
 */
static void task3_entry(void *parameter)
{
    (void)parameter;
    
    printf("任务3开始运行\n");
    
    while (1) {
        /* 等待信号量 */
        rtos_result_t result = rtos_semaphore_take(&sem1, RTOS_TIMEOUT_INF);
        if (result == RTOS_OK) {
            printf("任务3获取信号量\n");
            
            /* 从队列接收消息 */
            char received_message[32];
            int32_t received_size = rtos_queue_receive(&queue1, received_message, 
                                                     sizeof(received_message), RTOS_TIMEOUT_INF);
            if (received_size > 0) {
                printf("任务3接收到消息: %s\n", received_message);
            }
        }
        
        /* 延时2秒 */
        rtos_task_delay_ms(2000);
    }
}

/**
 * @brief 系统启动钩子函数
 */
static void system_startup_hook(void)
{
    printf("系统启动钩子执行\n");
}

/**
 * @brief 系统空闲钩子函数
 */
static void system_idle_hook(void)
{
    /* 可以在这里执行低功耗操作 */
    static uint32_t idle_count = 0;
    idle_count++;
    
    if (idle_count % 1000 == 0) {
        printf("系统空闲处理: %lu\n", idle_count);
    }
}
