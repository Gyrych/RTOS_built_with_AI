/**
 * @file hw_abstraction.c
 * @brief RTOS硬件抽象层实现 - 重构后的硬件抽象接口
 * @author Assistant
 * @date 2024
 */

#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>

/* 硬件平台信息 */
static rtos_hw_platform_t g_hw_platform = RTOS_HW_PLATFORM_UNKNOWN;
static uint32_t g_cpu_count = 1;
static uint32_t g_system_clock_freq = 0;
static uint32_t g_cpu_clock_freq = 0;
static uint32_t g_tick_count = 0;
static uint64_t g_system_time_ns = 0;
static uint64_t g_system_time_us = 0;
static uint64_t g_system_time_ms = 0;

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
    
    return RTOS_SUCCESS;
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
 * @brief 获取系统滴答计数
 */
uint32_t rtos_hw_get_tick_count(void)
{
    return g_tick_count;
}

/**
 * @brief 更新系统滴答计数
 */
void rtos_hw_tick_update(void)
{
    g_tick_count++;
    g_system_time_ns += 1000000;  /* 假设1ms滴答 */
    g_system_time_us += 1000;
    g_system_time_ms += 1;
}

/**
 * @brief 获取系统运行时间(纳秒)
 */
rtos_time_ns_t rtos_hw_get_system_time_ns(void)
{
    return g_system_time_ns;
}

/**
 * @brief 获取系统运行时间(微秒)
 */
uint64_t rtos_hw_get_system_time_us(void)
{
    return g_system_time_us;
}

/**
 * @brief 获取系统运行时间(毫秒)
 */
uint64_t rtos_hw_get_system_time_ms(void)
{
    return g_system_time_ms;
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
    return RTOS_SUCCESS;
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
    return RTOS_SUCCESS;
}

/**
 * @brief 禁用中断
 */
rtos_result_t rtos_hw_disable_irq(uint32_t irq_num)
{
    (void)irq_num;
    
    /* 简单实现：总是成功 */
    return RTOS_SUCCESS;
}

/**
 * @brief 设置中断向量表
 */
rtos_result_t rtos_hw_set_vector_table(uint32_t vector_table)
{
    (void)vector_table;
    
    /* 简单实现：总是成功 */
    return RTOS_SUCCESS;
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
    
    return RTOS_SUCCESS;
}

/**
 * @brief 硬件看门狗初始化
 */
rtos_result_t rtos_hw_watchdog_init(uint32_t timeout_ms)
{
    (void)timeout_ms;
    
    /* 简单实现：总是成功 */
    return RTOS_SUCCESS;
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
                       "CPU Count: %u\n"
                       "System Clock: %u Hz\n"
                       "CPU Clock: %u Hz\n"
                       "Tick Count: %u\n"
                       "System Time: %llu ms",
                       platform_name,
                       g_cpu_count,
                       g_system_clock_freq,
                       g_cpu_clock_freq,
                       g_tick_count,
                       g_system_time_ms);
    
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
