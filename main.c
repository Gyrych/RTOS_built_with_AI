/**
 * @file main.c
 * @brief RTOS系统初始化和示例应用
 */

#include "rtos_kernel.h"
#include <stdio.h>

/* 任务栈定义 */
static uint32_t task1_stack[512];
static uint32_t task2_stack[512];
static uint32_t task3_stack[512];

/* 任务控制块 */
static rtos_task_t task1;
static rtos_task_t task2;
static rtos_task_t task3;

/* 同步对象 */
static rtos_semaphore_t led_semaphore;
static rtos_mutex_t uart_mutex;
static rtos_queue_t data_queue;

/* 消息队列缓冲区 */
static uint8_t queue_buffer[10 * sizeof(uint32_t)];

/* 任务函数声明 */
void led_task(void *param);
void sensor_task(void *param);
void communication_task(void *param);

/* 硬件初始化 */
void hardware_init(void);

/**
 * @brief 主函数
 */
int main(void)
{
    /* 硬件初始化 */
    hardware_init();
    
    /* RTOS初始化 */
    if (rtos_init() != RTOS_OK) {
        printf("RTOS初始化失败!\n");
        while(1);
    }
    
    /* 创建同步对象 */
    rtos_semaphore_create(&led_semaphore, 1, 1);
    rtos_mutex_create(&uart_mutex);
    rtos_queue_create(&data_queue, queue_buffer, sizeof(uint32_t), 10);
    
    /* 创建任务 */
    rtos_task_create(&task1, "LED_Task", led_task, NULL, 2, 
                     task1_stack, sizeof(task1_stack));
    
    rtos_task_create(&task2, "Sensor_Task", sensor_task, NULL, 1, 
                     task2_stack, sizeof(task2_stack));
    
    rtos_task_create(&task3, "Comm_Task", communication_task, NULL, 3, 
                     task3_stack, sizeof(task3_stack));
    
    printf("RTOS系统启动...\n");
    
    /* 启动RTOS调度器 */
    if (rtos_start() != RTOS_OK) {
        printf("RTOS启动失败!\n");
        while(1);
    }
    
    /* 不应该到达这里 */
    while(1);
}

/**
 * @brief LED任务 - 演示信号量使用
 */
void led_task(void *param)
{
    static uint32_t led_count = 0;
    
    while(1) {
        /* 获取LED信号量 */
        if (rtos_semaphore_take(&led_semaphore, 1000000) == RTOS_OK) { /* 1秒超时 */
            printf("[%u] LED任务: LED闪烁 #%u\n", rtos_get_time_ms(), led_count++);
            
            /* 模拟LED操作耗时 */
            rtos_delay_us(100000); /* 100ms */
            
            /* 释放信号量 */
            rtos_semaphore_give(&led_semaphore);
        }
        
        /* 任务周期延时 */
        rtos_delay_ms(500);
    }
}

/**
 * @brief 传感器任务 - 演示消息队列发送
 */
void sensor_task(void *param)
{
    static uint32_t sensor_data = 1000;
    
    while(1) {
        /* 模拟传感器读取 */
        sensor_data += (rtos_get_time_ms() % 100) - 50; /* 随机变化 */
        
        /* 发送数据到队列 */
        if (rtos_queue_send(&data_queue, &sensor_data, 500000) == RTOS_OK) { /* 500ms超时 */
            printf("[%u] 传感器任务: 发送数据 %u\n", rtos_get_time_ms(), sensor_data);
        } else {
            printf("[%u] 传感器任务: 队列满，数据丢失\n", rtos_get_time_ms());
        }
        
        /* 任务周期延时 */
        rtos_delay_ms(300);
    }
}

/**
 * @brief 通信任务 - 演示互斥量和消息队列接收
 */
void communication_task(void *param)
{
    uint32_t received_data;
    
    while(1) {
        /* 从队列接收数据 */
        if (rtos_queue_receive(&data_queue, &received_data, 2000000) == RTOS_OK) { /* 2秒超时 */
            /* 获取UART互斥量 */
            if (rtos_mutex_lock(&uart_mutex, 1000000) == RTOS_OK) { /* 1秒超时 */
                printf("[%u] 通信任务: 处理数据 %u\n", rtos_get_time_ms(), received_data);
                
                /* 模拟数据处理和发送 */
                rtos_delay_us(50000); /* 50ms */
                
                printf("[%u] 通信任务: 数据 %u 发送完成\n", rtos_get_time_ms(), received_data);
                
                /* 释放互斥量 */
                rtos_mutex_unlock(&uart_mutex);
            } else {
                printf("[%u] 通信任务: 获取UART互斥量超时\n", rtos_get_time_ms());
            }
        } else {
            printf("[%u] 通信任务: 等待数据超时\n", rtos_get_time_ms());
        }
        
        /* 任务让出CPU */
        rtos_task_yield();
    }
}

/**
 * @brief 硬件初始化
 */
void hardware_init(void)
{
    /* 这里应该包含STM32F407的具体硬件初始化代码 */
    /* 如：时钟配置、GPIO配置、UART配置等 */
    
    printf("硬件初始化完成\n");
}

/**
 * @brief 系统错误处理
 */
void system_error_handler(void)
{
    printf("系统错误！\n");
    while(1);
}

/**
 * @brief 内存管理错误处理
 */
void memory_error_handler(void)
{
    printf("内存错误！\n");
    while(1);
}