/**
 * @file hw_abstraction.c
 * @brief RTOS硬件抽象层实现 - 重构后的硬件抽象接口
 * @author Assistant
 * @date 2024
 */

#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>

/* 硬件平台信息 */
static rtos_hw_platform_t g_hw_platform = RTOS_HW_PLATFORM_UNKNOWN;
static uint32_t g_cpu_count = 1;
static uint32_t g_system_clock_freq = 0;
static uint32_t g_cpu_clock_freq = 0;

/* 高精度时间管理 - 移除滴答时钟依赖 */
static rtos_time_ns_t g_system_start_time = 0;
static rtos_time_ns_t g_hardware_timer_period = 0;
static volatile bool g_hardware_timer_running = false;

/* 硬件抽象层初始化 */
rtos_result_t rtos_hw_abstraction_init(void)
{
    /* 检测硬件平台 */
    #if defined(__ARM_ARCH_7M__)
        g_hw_platform = RTOS_HW_PLATFORM_ARM_CORTEX_M3;
    #elif defined(__ARM_ARCH_7EM__)
        g_hw_platform = RTOS_HW_PLATFORM_ARM_CORTEX_M4;
    #elif defined(__ARM_ARCH_7M__) && defined(__ARM_FEATURE_DSP)
        g_hw_platform = RTOS_HW_PLATFORM_ARM_CORTEX_M7;
    #elif defined(__ARM_ARCH_7A__)
        g_hw_platform = RTOS_HW_PLATFORM_ARM_CORTEX_A7;
    #elif defined(__ARM_ARCH_7A__) && defined(__ARM_FEATURE_NEON)
        g_hw_platform = RTOS_HW_PLATFORM_ARM_CORTEX_A9;
    #elif defined(__aarch64__)
        g_hw_platform = RTOS_HW_PLATFORM_ARM_CORTEX_A53;
    #elif defined(__riscv)
        g_hw_platform = RTOS_HW_PLATFORM_RISC_V;
    #elif defined(__i386__)
        g_hw_platform = RTOS_HW_PLATFORM_X86;
    #elif defined(__x86_64__)
        g_hw_platform = RTOS_HW_PLATFORM_X86_64;
    #else
        g_hw_platform = RTOS_HW_PLATFORM_UNKNOWN;
    #endif
    
    /* 设置默认时钟频率 */
    g_system_clock_freq = 168000000;  /* 168 MHz for STM32F4 */
    g_cpu_clock_freq = g_system_clock_freq;
    
    return RTOS_OK;
}

/**
 * @brief 获取硬件平台类型
 */
rtos_hw_platform_t rtos_hw_get_platform(void)
{
    return g_hw_platform;
}

/**
 * @brief 获取CPU核心数量
 */
uint32_t rtos_hw_get_cpu_count(void)
{
    return g_cpu_count;
}

/**
 * @brief 获取当前CPU核心ID
 */
uint32_t rtos_hw_get_current_cpu_id(void)
{
    return 0; /* 单核系统 */
}

/**
 * @brief 获取系统时钟频率
 */
uint32_t rtos_hw_get_system_clock_frequency(void)
{
    return g_system_clock_freq;
}

/**
 * @brief 获取CPU时钟频率
 */
uint32_t rtos_hw_get_cpu_clock_frequency(void)
{
    return g_cpu_clock_freq;
}

/**
 * @brief 获取外设时钟频率
 */
uint32_t rtos_hw_get_peripheral_clock_frequency(uint32_t peripheral)
{
    /* 简单实现：返回系统时钟频率 */
    (void)peripheral;
    return g_system_clock_freq;
}

/**
 * @brief 系统延时(微秒)
 */
void rtos_hw_delay_us(uint32_t us)
{
    /* 简单的忙等待延时 */
    uint32_t cycles = (us * g_cpu_clock_freq) / 1000000;
    for (volatile uint32_t i = 0; i < cycles; i++) {
        __asm volatile("nop");
    }
}

/**
 * @brief 系统延时(毫秒)
 */
void rtos_hw_delay_ms(uint32_t ms)
{
    rtos_hw_delay_us(ms * 1000);
}

/**
 * @brief 获取高精度时间戳(纳秒)
 */
rtos_time_ns_t rtos_hw_get_timestamp_ns(void)
{
    /* 使用系统定时器获取高精度时间戳 */
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        /* ARM Cortex-M系列：使用DWT CYCCNT寄存器 */
        static uint32_t *DWT_CYCCNT = (uint32_t *)0xE0001004;
        static uint32_t *DWT_CTRL = (uint32_t *)0xE0001000;
        static bool dwt_initialized = false;
        
        if (!dwt_initialized) {
            *DWT_CTRL |= 1; /* 使能CYCCNT */
            dwt_initialized = true;
        }
        
        uint32_t cycles = *DWT_CYCCNT;
        /* 将时钟周期转换为纳秒 */
        return g_system_start_time + ((uint64_t)cycles * 1000000000ULL) / g_cpu_clock_freq;
    #else
        /* 其他平台：使用软件计数器 */
        static volatile uint64_t software_counter = 0;
        return g_system_start_time + software_counter++;
    #endif
}

/**
 * @brief 获取系统运行时间(纳秒)
 */
rtos_time_ns_t rtos_hw_get_system_time_ns(void)
{
    return rtos_hw_get_timestamp_ns() - g_system_start_time;
}

/**
 * @brief 获取系统运行时间(微秒)
 */
uint64_t rtos_hw_get_system_time_us(void)
{
    return rtos_hw_get_system_time_ns() / 1000ULL;
}

/**
 * @brief 获取系统运行时间(毫秒)
 */
uint64_t rtos_hw_get_system_time_ms(void)
{
    return rtos_hw_get_system_time_ns() / 1000000ULL;
}

/**
 * @brief 进入临界区
 */
rtos_irq_state_t rtos_hw_enter_critical(void)
{
    rtos_irq_state_t irq_state = rtos_hw_disable_interrupts();
    RTOS_HW_MEMORY_BARRIER();
    return irq_state;
}

/**
 * @brief 退出临界区
 */
void rtos_hw_exit_critical(rtos_irq_state_t irq_state)
{
    RTOS_HW_MEMORY_BARRIER();
    rtos_hw_enable_interrupts(irq_state);
}

/**
 * @brief 禁用中断
 */
rtos_irq_state_t rtos_hw_disable_interrupts(void)
{
    rtos_irq_state_t irq_state;
    
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        __asm volatile("mrs %0, primask" : "=r" (irq_state));
        __asm volatile("cpsid i");
    #elif defined(__riscv)
        __asm volatile("csrr %0, mstatus" : "=r" (irq_state));
        __asm volatile("csrci mstatus, 8");
    #else
        /* 通用实现 */
        irq_state = RTOS_IRQ_STATE_ENABLED;
    #endif
    
    return irq_state;
}

/**
 * @brief 启用中断
 */
void rtos_hw_enable_interrupts(rtos_irq_state_t irq_state)
{
    if (irq_state == RTOS_IRQ_STATE_ENABLED) {
        #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
            __asm volatile("cpsie i");
        #elif defined(__riscv)
            __asm volatile("csrsi mstatus, 8");
        #endif
    }
}

/**
 * @brief 设置中断优先级
 */
rtos_result_t rtos_hw_set_irq_priority(uint32_t irq_num, rtos_irq_priority_t priority)
{
    (void)irq_num;
    (void)priority;
    
    /* 简单实现：总是成功 */
    return RTOS_OK;
}

/**
 * @brief 获取中断优先级
 */
rtos_irq_priority_t rtos_hw_get_irq_priority(uint32_t irq_num)
{
    (void)irq_num;
    
    /* 简单实现：返回正常优先级 */
    return RTOS_IRQ_PRIORITY_NORMAL;
}

/**
 * @brief 启用中断
 */
rtos_result_t rtos_hw_enable_irq(uint32_t irq_num)
{
    (void)irq_num;
    
    /* 简单实现：总是成功 */
    return RTOS_OK;
}

/**
 * @brief 禁用中断
 */
rtos_result_t rtos_hw_disable_irq(uint32_t irq_num)
{
    (void)irq_num;
    
    /* 简单实现：总是成功 */
    return RTOS_OK;
}

/**
 * @brief 设置中断向量表
 */
rtos_result_t rtos_hw_set_vector_table(uint32_t vector_table)
{
    (void)vector_table;
    
    /* 简单实现：总是成功 */
    return RTOS_OK;
}

/**
 * @brief 获取中断向量表
 */
uint32_t rtos_hw_get_vector_table(void)
{
    /* 简单实现：返回0 */
    return 0;
}

/**
 * @brief 系统复位
 */
void rtos_hw_system_reset(void)
{
    /* 简单实现：无限循环 */
    while (1) {
        __asm volatile("nop");
    }
}

/**
 * @brief 进入低功耗模式
 */
void rtos_hw_enter_low_power_mode(uint32_t mode)
{
    (void)mode;
    
    /* 简单实现：进入睡眠模式 */
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        __asm volatile("wfi");
    #elif defined(__riscv)
        __asm volatile("wfi");
    #endif
}

/**
 * @brief 退出低功耗模式
 */
void rtos_hw_exit_low_power_mode(void)
{
    /* 简单实现：无操作 */
}

/**
 * @brief 获取电源状态
 */
uint32_t rtos_hw_get_power_status(void)
{
    /* 简单实现：返回正常状态 */
    return 0;
}

/**
 * @brief 获取温度
 */
int32_t rtos_hw_get_temperature(void)
{
    /* 简单实现：返回25度 */
    return 25;
}

/**
 * @brief 获取电压
 */
uint32_t rtos_hw_get_voltage(uint32_t channel)
{
    (void)channel;
    
    /* 简单实现：返回3.3V */
    return 3300;
}

/**
 * @brief 获取内存信息
 */
void rtos_hw_get_memory_info(uint32_t *total_memory, uint32_t *free_memory, uint32_t *used_memory)
{
    /* 简单实现：返回默认值 */
    if (total_memory) {
        *total_memory = 128 * 1024;  /* 128KB */
    }
    
    if (free_memory) {
        *free_memory = 64 * 1024;    /* 64KB */
    }
    
    if (used_memory) {
        *used_memory = 64 * 1024;    /* 64KB */
    }
}

/**
 * @brief 获取堆栈使用情况
 */
rtos_result_t rtos_hw_get_stack_usage(uint32_t task_id, uint32_t *stack_used, uint32_t *stack_free)
{
    (void)task_id;
    
    /* 简单实现：返回默认值 */
    if (stack_used) {
        *stack_used = 512;  /* 512字节 */
    }
    
    if (stack_free) {
        *stack_free = 512;  /* 512字节 */
    }
    
    return RTOS_OK;
}

/**
 * @brief 硬件看门狗初始化
 */
rtos_result_t rtos_hw_watchdog_init(uint32_t timeout_ms)
{
    (void)timeout_ms;
    
    /* 简单实现：总是成功 */
    return RTOS_OK;
}

/**
 * @brief 硬件看门狗喂狗
 */
void rtos_hw_watchdog_feed(void)
{
    /* 简单实现：无操作 */
}

/**
 * @brief 硬件看门狗停止
 */
void rtos_hw_watchdog_stop(void)
{
    /* 简单实现：无操作 */
}

/**
 * @brief 设置硬件定时器
 */
rtos_result_t rtos_hw_set_timer(rtos_time_ns_t timeout_ns)
{
    if (timeout_ns == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 停止当前定时器 */
    rtos_hw_stop_timer();
    
    /* 设置新的定时器周期 */
    g_hardware_timer_period = timeout_ns;
    g_hardware_timer_running = true;
    
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        /* STM32F407：使用TIM2作为高精度定时器 */
        /* 这里需要根据具体硬件配置定时器寄存器 */
        /* 计算定时器重载值 */
        /* uint32_t timer_ticks = (uint32_t)((timeout_ns * g_cpu_clock_freq) / 1000000000ULL); */
        
        /* 配置定时器（简化实现，实际需要操作寄存器） */
        /* TIM2->ARR = timer_ticks; */
        /* TIM2->CR1 |= TIM_CR1_CEN; */
    #endif
    
    return RTOS_OK;
}

/**
 * @brief 停止硬件定时器
 */
rtos_result_t rtos_hw_stop_timer(void)
{
    if (!g_hardware_timer_running) {
        return RTOS_OK;
    }
    
    g_hardware_timer_running = false;
    g_hardware_timer_period = 0;
    
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        /* 停止定时器（简化实现） */
        /* TIM2->CR1 &= ~TIM_CR1_CEN; */
    #endif
    
    return RTOS_OK;
}

/**
 * @brief 获取硬件定时器剩余时间
 */
rtos_time_ns_t rtos_hw_get_timer_remaining(void)
{
    if (!g_hardware_timer_running) {
        return 0;
    }
    
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        /* 获取定时器当前值（简化实现） */
        /* uint32_t current_count = TIM2->CNT; */
        /* uint32_t reload_value = TIM2->ARR; */
        /* return ((uint64_t)(reload_value - current_count) * 1000000000ULL) / g_cpu_clock_freq; */
        
        /* 简化实现：返回设置的周期 */
        return g_hardware_timer_period;
    #else
        return g_hardware_timer_period;
    #endif
}

/**
 * @brief 硬件定时器中断处理函数
 */
void rtos_hw_timer_interrupt_handler(void)
{
    if (g_hardware_timer_running) {
        g_hardware_timer_running = false;
        
        /* 触发调度器 */
        extern void rtos_scheduler_schedule(void);
        rtos_scheduler_schedule();
    }
}

/**
 * @brief 获取硬件信息字符串
 */
uint32_t rtos_hw_get_info_string(char *buffer, uint32_t size)
{
    if (!buffer || size == 0) {
        return 0;
    }
    
    const char *platform_name = "Unknown";
    switch (g_hw_platform) {
        case RTOS_HW_PLATFORM_ARM_CORTEX_M3:
            platform_name = "ARM Cortex-M3";
            break;
        case RTOS_HW_PLATFORM_ARM_CORTEX_M4:
            platform_name = "ARM Cortex-M4";
            break;
        case RTOS_HW_PLATFORM_ARM_CORTEX_M7:
            platform_name = "ARM Cortex-M7";
            break;
        case RTOS_HW_PLATFORM_ARM_CORTEX_A7:
            platform_name = "ARM Cortex-A7";
            break;
        case RTOS_HW_PLATFORM_ARM_CORTEX_A9:
            platform_name = "ARM Cortex-A9";
            break;
        case RTOS_HW_PLATFORM_ARM_CORTEX_A53:
            platform_name = "ARM Cortex-A53";
            break;
        case RTOS_HW_PLATFORM_ARM_CORTEX_A72:
            platform_name = "ARM Cortex-A72";
            break;
        case RTOS_HW_PLATFORM_RISC_V:
            platform_name = "RISC-V";
            break;
        case RTOS_HW_PLATFORM_X86:
            platform_name = "x86";
            break;
        case RTOS_HW_PLATFORM_X86_64:
            platform_name = "x86-64";
            break;
        default:
            break;
    }
    
    int len = snprintf(buffer, size, 
                       "Platform: %s\n"
                       "CPU Count: %lu\n"
                       "System Clock: %lu Hz\n"
                       "CPU Clock: %lu Hz\n"
                       "Timer Running: %s\n"
                       "System Time: %lu ms",
                       platform_name,
                       g_cpu_count,
                       g_system_clock_freq,
                       g_cpu_clock_freq,
                       g_hardware_timer_running ? "Yes" : "No",
                       (unsigned long)rtos_hw_get_system_time_ms());
    
    if (len < 0) {
        len = 0;
    } else if ((uint32_t)len >= size) {
        len = size - 1;
    }
    
    return (uint32_t)len;
}

/**
 * @brief 设置系统时钟频率
 */
void rtos_hw_set_system_clock_frequency(uint32_t freq)
{
    g_system_clock_freq = freq;
}

/**
 * @brief 设置CPU时钟频率
 */
void rtos_hw_set_cpu_clock_frequency(uint32_t freq)
{
    g_cpu_clock_freq = freq;
}

/**
 * @brief 设置CPU核心数量
 */
void rtos_hw_set_cpu_count(uint32_t count)
{
    g_cpu_count = count;
}
