/**
 * @file main_simple.c
 * @brief 简化的RTOS演示程序
 * @author Assistant
 * @date 2024
 */

#include "rtos_simple.h"

/* 任务栈定义 */
static uint32_t task1_stack[512];
static uint32_t task2_stack[512];

/* 任务控制块 */
static rtos_task_t task1;
static rtos_task_t task2;

/* 同步对象 */
static rtos_semaphore_t test_semaphore;

/* 任务函数声明 */
void task1_function(void *param);
void task2_function(void *param);

/**
 * @brief 主函数
 */
int main(void)
{
    printf("简化RTOS系统启动...\n");
    
    /* RTOS系统初始化 */
    if (rtos_system_init() != RTOS_OK) {
        printf("RTOS系统初始化失败!\n");
        return -1;
    }
    
    /* 创建信号量 */
    rtos_sem_init(&test_semaphore, "test_sem", 1, 0);
    
    /* 创建任务 */
    rtos_task_init(&task1, "Task1", task1_function, NULL,
                   task1_stack, sizeof(task1_stack), 1, 10000);
    
    rtos_task_init(&task2, "Task2", task2_function, NULL,
                   task2_stack, sizeof(task2_stack), 2, 10000);
    
    /* 启动任务 */
    rtos_task_startup(&task1);
    rtos_task_startup(&task2);
    
    /* 启动RTOS调度器 */
    if (rtos_system_start() != RTOS_OK) {
        printf("RTOS启动失败!\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief 任务1
 */
void task1_function(void *param)
{
    (void)param;
    uint32_t count = 0;
    
    while(1) {
        if (rtos_sem_take(&test_semaphore, 1000000000ULL) == RTOS_OK) {
            printf("[%lu] Task1 运行: #%lu\n", 
                   (unsigned long)rtos_system_get_time_ms(), 
                   (unsigned long)count++);
            
            rtos_system_delay_ms(100);
            rtos_sem_release(&test_semaphore);
        }
        
        rtos_task_mdelay(500);
    }
}

/**
 * @brief 任务2
 */
void task2_function(void *param)
{
    (void)param;
    uint32_t count = 0;
    
    while(1) {
        printf("[%lu] Task2 运行: #%lu\n", 
               (unsigned long)rtos_system_get_time_ms(), 
               (unsigned long)count++);
        
        rtos_task_mdelay(1000);
    }
}