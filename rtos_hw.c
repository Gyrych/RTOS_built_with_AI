/**
 * @file rtos_hw.c
 * @brief STM32F407硬件抽象层实现
 */

#include "rtos_hw.h"
#include "rtos_kernel.h"

/* STM32F407系统时钟配置(假设168MHz) */
#define SYSTEM_CLOCK_HZ         168000000
#define TIMER_CLOCK_HZ          84000000    /* TIM2-7时钟 */

/* TIM2寄存器定义(用于精确定时) */
#define TIM2_BASE               0x40000000
#define TIM2_CR1                (*(volatile uint32_t *)(TIM2_BASE + 0x00))
#define TIM2_DIER               (*(volatile uint32_t *)(TIM2_BASE + 0x0C))
#define TIM2_SR                 (*(volatile uint32_t *)(TIM2_BASE + 0x10))
#define TIM2_CNT                (*(volatile uint32_t *)(TIM2_BASE + 0x24))
#define TIM2_PSC                (*(volatile uint32_t *)(TIM2_BASE + 0x28))
#define TIM2_ARR                (*(volatile uint32_t *)(TIM2_BASE + 0x2C))

/* DWT寄存器定义(用于获取精确时间) */
#define DWT_CTRL                (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT              (*(volatile uint32_t *)0xE0001004)
#define DWT_CYCCNTENA           (1UL << 0)

/* RCC寄存器定义 */
#define RCC_APB1ENR             (*(volatile uint32_t *)0x40023840)
#define RCC_TIM2EN              (1UL << 0)

/* NVIC寄存器定义 */
#define NVIC_ISER1              (*(volatile uint32_t *)0xE000E104)
#define TIM2_IRQn               28
#define NVIC_TIM2_ENABLE        (1UL << (TIM2_IRQn - 32))

/* 内部变量 */
static volatile uint32_t timer_overflow_count = 0;
static volatile uint64_t system_time_us = 0;

/**
 * @brief 硬件初始化
 */
void rtos_hw_init(void)
{
    /* 启用DWT计数器(用于精确时间测量) */
    DWT_CTRL |= DWT_CYCCNTENA;
    
    /* 设置PendSV和SysTick为最低优先级 */
    *(volatile uint32_t *)NVIC_SYSPRI2 |= NVIC_PENDSV_PRI;
}

/**
 * @brief 定时器硬件初始化
 */
void rtos_hw_timer_init(void)
{
    /* 使能TIM2时钟 */
    RCC_APB1ENR |= RCC_TIM2EN;
    
    /* 配置TIM2为微秒级定时器 */
    TIM2_PSC = (TIMER_CLOCK_HZ / 1000000) - 1;  /* 1MHz计数频率 */
    TIM2_ARR = 0xFFFFFFFF;                      /* 最大计数值 */
    
    /* 使能更新中断 */
    TIM2_DIER |= 0x01;
    
    /* 使能TIM2中断 */
    NVIC_ISER1 |= NVIC_TIM2_ENABLE;
    
    /* 启动定时器 */
    TIM2_CR1 |= 0x01;
}

/**
 * @brief 设置定时器
 * @param microseconds 微秒数
 */
void rtos_hw_timer_set(uint32_t microseconds)
{
    /* 停止定时器 */
    TIM2_CR1 &= ~0x01;
    
    /* 清除计数器 */
    TIM2_CNT = 0;
    
    /* 设置自动重装载值 */
    if (microseconds > 0xFFFFFFFF) {
        TIM2_ARR = 0xFFFFFFFF;
    } else {
        TIM2_ARR = microseconds;
    }
    
    /* 清除中断标志 */
    TIM2_SR &= ~0x01;
    
    /* 启动定时器 */
    TIM2_CR1 |= 0x01;
}

/**
 * @brief 停止定时器
 */
void rtos_hw_timer_stop(void)
{
    TIM2_CR1 &= ~0x01;
}

/**
 * @brief 获取当前时间(微秒)
 */
uint32_t rtos_hw_get_time_us(void)
{
    /* 使用DWT计数器获取更精确的时间 */
    static uint32_t last_dwt_count = 0;
    uint32_t current_dwt_count = DWT_CYCCNT;
    
    /* 处理计数器溢出 */
    if (current_dwt_count < last_dwt_count) {
        timer_overflow_count++;
    }
    last_dwt_count = current_dwt_count;
    
    /* 转换为微秒 */
    uint64_t total_cycles = ((uint64_t)timer_overflow_count << 32) + current_dwt_count;
    return (uint32_t)(total_cycles / (SYSTEM_CLOCK_HZ / 1000000));
}

/**
 * @brief 关闭中断
 */
void rtos_hw_disable_interrupts(void)
{
    __asm volatile ("cpsid i" : : : "memory");
}

/**
 * @brief 开启中断
 */
void rtos_hw_enable_interrupts(void)
{
    __asm volatile ("cpsie i" : : : "memory");
}

/**
 * @brief 触发上下文切换
 */
void rtos_context_switch(void)
{
    /* 触发PendSV中断 */
    *(volatile uint32_t *)NVIC_INT_CTRL = NVIC_PENDSVSET;
}

/**
 * @brief 初始化任务栈
 * @param stack_top 栈顶指针
 * @param task_func 任务函数
 * @param param 任务参数
 * @return 初始化后的栈指针
 */
uint32_t *rtos_task_stack_init(uint32_t *stack_top, void (*task_func)(void *), void *param)
{
    uint32_t *stack_ptr = stack_top;
    
    /* Cortex-M4寄存器初始化(异常栈帧) */
    *(--stack_ptr) = PSR_THUMB_BIT;             /* xPSR */
    *(--stack_ptr) = (uint32_t)task_func;       /* PC */
    *(--stack_ptr) = 0x0000000E;                /* LR (异常返回值) */
    *(--stack_ptr) = 0x0000000C;                /* R12 */
    *(--stack_ptr) = 0x00000003;                /* R3 */
    *(--stack_ptr) = 0x00000002;                /* R2 */
    *(--stack_ptr) = 0x00000001;                /* R1 */
    *(--stack_ptr) = (uint32_t)param;           /* R0 (任务参数) */
    
    /* 非异常栈帧寄存器 */
    *(--stack_ptr) = 0x0000000B;                /* R11 */
    *(--stack_ptr) = 0x0000000A;                /* R10 */
    *(--stack_ptr) = 0x00000009;                /* R9 */
    *(--stack_ptr) = 0x00000008;                /* R8 */
    *(--stack_ptr) = 0x00000007;                /* R7 */
    *(--stack_ptr) = 0x00000006;                /* R6 */
    *(--stack_ptr) = 0x00000005;                /* R5 */
    *(--stack_ptr) = 0x00000004;                /* R4 */
    
    return stack_ptr;
}

/**
 * @brief TIM2中断服务程序
 */
void TIM2_IRQHandler(void)
{
    if (TIM2_SR & 0x01) {
        TIM2_SR &= ~0x01;  /* 清除中断标志 */
        
        /* 停止定时器 */
        TIM2_CR1 &= ~0x01;
        
        /* 调用RTOS定时器中断处理 */
        rtos_timer_isr();
    }
}