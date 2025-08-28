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
#define NVIC_TIM2_ENABLE        (1UL << (TIM2_IRQn % 32))

/* 内部变量 */
static volatile uint32_t timer_overflow_count = 0;
static volatile uint64_t system_time_us = 0;
static volatile uint64_t system_time_ns = 0;
static uint32_t cpu_frequency_hz = SYSTEM_CLOCK_HZ;

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
 * @brief 设置定时器(纳秒级)
 * @param nanoseconds 纳秒数
 */
void rtos_hw_timer_set_ns(rtos_time_ns_t nanoseconds)
{
    /* 转换纳秒到微秒 */
    uint32_t microseconds = (uint32_t)(nanoseconds / 1000);
    if (microseconds == 0 && nanoseconds > 0) {
        microseconds = 1; /* 最小1微秒 */
    }
    rtos_hw_timer_set(microseconds);
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
    return (uint32_t)(total_cycles / (cpu_frequency_hz / 1000000));
}

/**
 * @brief 获取当前时间(纳秒)
 */
rtos_time_ns_t rtos_hw_get_time_ns(void)
{
    /* 使用DWT计数器获取更精确的时间 */
    static uint32_t last_dwt_count = 0;
    uint32_t current_dwt_count = DWT_CYCCNT;
    
    /* 处理计数器溢出 */
    if (current_dwt_count < last_dwt_count) {
        timer_overflow_count++;
    }
    last_dwt_count = current_dwt_count;
    
    /* 转换为纳秒 */
    uint64_t total_cycles = ((uint64_t)timer_overflow_count << 32) + current_dwt_count;
    return (total_cycles * 1000000000ULL) / cpu_frequency_hz;
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

/* ========== 功耗管理实现 ========== */

/**
 * @brief 进入睡眠模式
 */
void rtos_hw_enter_sleep(rtos_sleep_mode_t mode)
{
    switch (mode) {
        case RTOS_SLEEP_LIGHT:
            /* 进入WFI睡眠 */
            __asm volatile ("wfi");
            break;
            
        case RTOS_SLEEP_DEEP:
            /* 配置深度睡眠模式 */
            *(volatile uint32_t *)0xE000ED10 |= 0x04; /* SLEEPDEEP位 */
            __asm volatile ("wfi");
            *(volatile uint32_t *)0xE000ED10 &= ~0x04; /* 清除SLEEPDEEP位 */
            break;
            
        case RTOS_SLEEP_STANDBY:
            /* 进入待机模式(需要复位唤醒) */
            *(volatile uint32_t *)0x40007000 |= 0x02; /* PWR_CR PDDS位 */
            *(volatile uint32_t *)0xE000ED10 |= 0x04; /* SLEEPDEEP位 */
            __asm volatile ("wfi");
            break;
            
        default:
            break;
    }
}

/**
 * @brief 配置唤醒源
 */
void rtos_hw_configure_wakeup(uint32_t sources)
{
    /* 配置外部中断作为唤醒源 */
    if (sources & 0x01) {
        /* 配置EXTI唤醒 */
        *(volatile uint32_t *)0x40013C00 |= 0x01; /* EXTI_IMR */
    }
    
    if (sources & 0x02) {
        /* 配置RTC唤醒 */
        *(volatile uint32_t *)0x40002800 |= 0x400; /* RTC_CR WUTE位 */
    }
}

/**
 * @brief 设置CPU频率
 */
void rtos_hw_set_cpu_frequency(uint32_t frequency_hz)
{
    /* 更新内部频率记录 */
    cpu_frequency_hz = frequency_hz;
    
    /* 这里可以添加实际的频率切换代码 */
    /* 注意：实际实现需要重新配置PLL和时钟分频器 */
}

/* ========== MPU内存保护实现 ========== */

/* MPU寄存器定义 */
#define MPU_TYPE     (*(volatile uint32_t *)0xE000ED90)
#define MPU_CTRL     (*(volatile uint32_t *)0xE000ED94)
#define MPU_RNR      (*(volatile uint32_t *)0xE000ED98)
#define MPU_RBAR     (*(volatile uint32_t *)0xE000ED9C)
#define MPU_RASR     (*(volatile uint32_t *)0xE000EDA0)

/**
 * @brief MPU初始化
 */
void rtos_hw_mpu_init(void)
{
    /* 检查MPU是否存在 */
    uint32_t mpu_type = MPU_TYPE;
    if ((mpu_type >> 8) == 0) {
        return; /* MPU不存在 */
    }
    
    /* 禁用MPU */
    MPU_CTRL = 0;
    
    /* 清除所有区域 */
    for (int i = 0; i < 8; i++) {
        MPU_RNR = i;
        MPU_RBAR = 0;
        MPU_RASR = 0;
    }
}

/**
 * @brief 配置MPU区域
 */
void rtos_hw_mpu_configure_region(uint8_t region_id, uint32_t base_addr, 
                                 uint32_t size, uint32_t permissions)
{
    if (region_id >= 8) return;
    
    /* 选择区域 */
    MPU_RNR = region_id;
    
    /* 设置基地址 */
    MPU_RBAR = base_addr | (1 << 4); /* VALID位 */
    
    /* 设置大小和权限 */
    uint32_t size_bits = 0;
    uint32_t temp_size = size;
    while (temp_size > 1) {
        temp_size >>= 1;
        size_bits++;
    }
    
    MPU_RASR = (permissions << 24) | ((size_bits - 1) << 1) | 1; /* ENABLE位 */
}

/**
 * @brief 使能MPU
 */
void rtos_hw_mpu_enable(void)
{
    /* 使能MPU，启用默认内存映射 */
    MPU_CTRL = 0x07; /* ENABLE | HFNMIENA | PRIVDEFENA */
}

/**
 * @brief 禁用MPU
 */
void rtos_hw_mpu_disable(void)
{
    MPU_CTRL = 0;
}