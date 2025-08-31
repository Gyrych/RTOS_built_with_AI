/**
 * @file hw_abstraction.h
 * @brief RTOS硬件抽象层 - 重构后的硬件抽象接口
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_HW_ABSTRACTION_H__
#define __RTOS_HW_ABSTRACTION_H__

#include "../core/types.h"

/* 包含新增的硬件抽象模块 */
#include "power_management.h"
#include "memory_monitor.h"
#include "watchdog_manager.h"
#include "gpio_abstraction.h"
#include "uart_abstraction.h"

/* 硬件平台类型定义 */
typedef enum {
    RTOS_HW_PLATFORM_UNKNOWN = 0,
    RTOS_HW_PLATFORM_ARM_CORTEX_M3,
    RTOS_HW_PLATFORM_ARM_CORTEX_M4,
    RTOS_HW_PLATFORM_ARM_CORTEX_M7,
    RTOS_HW_PLATFORM_ARM_CORTEX_A7,
    RTOS_HW_PLATFORM_ARM_CORTEX_A9,
    RTOS_HW_PLATFORM_ARM_CORTEX_A53,
    RTOS_HW_PLATFORM_ARM_CORTEX_A72,
    RTOS_HW_PLATFORM_RISC_V,
    RTOS_HW_PLATFORM_X86,
    RTOS_HW_PLATFORM_X86_64,
    RTOS_HW_PLATFORM_MAX
} rtos_hw_platform_t;

/* 中断优先级定义 */
typedef enum {
    RTOS_IRQ_PRIORITY_LOWEST = 0,
    RTOS_IRQ_PRIORITY_LOW = 1,
    RTOS_IRQ_PRIORITY_NORMAL = 2,
    RTOS_IRQ_PRIORITY_HIGH = 3,
    RTOS_IRQ_PRIORITY_HIGHEST = 4,
    RTOS_IRQ_PRIORITY_CRITICAL = 5
} rtos_irq_priority_t;

/* 中断状态定义 */
typedef enum {
    RTOS_IRQ_STATE_DISABLED = 0,
    RTOS_IRQ_STATE_ENABLED = 1
} rtos_irq_state_t;

/* 时钟源类型定义 */
typedef enum {
    RTOS_CLOCK_SOURCE_INTERNAL = 0,
    RTOS_CLOCK_SOURCE_EXTERNAL = 1,
    RTOS_CLOCK_SOURCE_PLL = 2,
    RTOS_CLOCK_SOURCE_USB = 3
} rtos_clock_source_t;

/* 硬件抽象层API函数声明 */

/**
 * @brief 初始化硬件抽象层
 * @return 操作结果
 */
rtos_result_t rtos_hw_abstraction_init(void);

/**
 * @brief 获取硬件平台类型
 * @return 硬件平台类型
 */
rtos_hw_platform_t rtos_hw_get_platform(void);

/**
 * @brief 获取CPU核心数量
 * @return CPU核心数量
 */
uint32_t rtos_hw_get_cpu_count(void);

/**
 * @brief 获取当前CPU核心ID
 * @return 当前CPU核心ID
 */
uint32_t rtos_hw_get_current_cpu_id(void);

/**
 * @brief 获取系统时钟频率
 * @return 系统时钟频率(Hz)
 */
uint32_t rtos_hw_get_system_clock_frequency(void);

/**
 * @brief 获取CPU时钟频率
 * @return CPU时钟频率(Hz)
 */
uint32_t rtos_hw_get_cpu_clock_frequency(void);

/**
 * @brief 获取外设时钟频率
 * @param peripheral 外设ID
 * @return 外设时钟频率(Hz)
 */
uint32_t rtos_hw_get_peripheral_clock_frequency(uint32_t peripheral);

/**
 * @brief 系统延时(微秒)
 * @param us 延时时间(微秒)
 */
void rtos_hw_delay_us(uint32_t us);

/**
 * @brief 系统延时(毫秒)
 * @param ms 延时时间(毫秒)
 */
void rtos_hw_delay_ms(uint32_t ms);

/**
 * @brief 获取高精度时间戳(纳秒)
 * @return 高精度时间戳(纳秒)
 */
rtos_time_ns_t rtos_hw_get_timestamp_ns(void);

/**
 * @brief 获取系统运行时间(纳秒)
 * @return 系统运行时间(纳秒)
 */
rtos_time_ns_t rtos_hw_get_system_time_ns(void);

/**
 * @brief 获取系统运行时间(微秒)
 * @return 系统运行时间(微秒)
 */
uint64_t rtos_hw_get_system_time_us(void);

/**
 * @brief 获取系统运行时间(毫秒)
 * @return 系统运行时间(毫秒)
 */
uint64_t rtos_hw_get_system_time_ms(void);

/**
 * @brief 进入临界区
 * @return 进入前的IRQ状态
 */
rtos_irq_state_t rtos_hw_enter_critical(void);

/**
 * @brief 退出临界区
 * @param irq_state 进入前的IRQ状态
 */
void rtos_hw_exit_critical(rtos_irq_state_t irq_state);

/**
 * @brief 禁用中断
 * @return 禁用前的IRQ状态
 */
rtos_irq_state_t rtos_hw_disable_interrupts(void);

/**
 * @brief 启用中断
 * @param irq_state 禁用前的IRQ状态
 */
void rtos_hw_enable_interrupts(rtos_irq_state_t irq_state);

/**
 * @brief 设置中断优先级
 * @param irq_num 中断号
 * @param priority 优先级
 * @return 操作结果
 */
rtos_result_t rtos_hw_set_irq_priority(uint32_t irq_num, rtos_irq_priority_t priority);

/**
 * @brief 获取中断优先级
 * @param irq_num 中断号
 * @return 中断优先级
 */
rtos_irq_priority_t rtos_hw_get_irq_priority(uint32_t irq_num);

/**
 * @brief 启用中断
 * @param irq_num 中断号
 * @return 操作结果
 */
rtos_result_t rtos_hw_enable_irq(uint32_t irq_num);

/**
 * @brief 禁用中断
 * @param irq_num 中断号
 * @return 操作结果
 */
rtos_result_t rtos_hw_disable_irq(uint32_t irq_num);

/**
 * @brief 设置中断向量表
 * @param vector_table 中断向量表地址
 * @return 操作结果
 */
rtos_result_t rtos_hw_set_vector_table(uint32_t vector_table);

/**
 * @brief 获取中断向量表
 * @return 中断向量表地址
 */
uint32_t rtos_hw_get_vector_table(void);

/**
 * @brief 系统复位
 */
void rtos_hw_system_reset(void);

/**
 * @brief 进入低功耗模式
 * @param mode 低功耗模式
 */
void rtos_hw_enter_low_power_mode(uint32_t mode);

/**
 * @brief 退出低功耗模式
 */
void rtos_hw_exit_low_power_mode(void);

/**
 * @brief 获取电源状态
 * @return 电源状态
 */
uint32_t rtos_hw_get_power_status(void);

/**
 * @brief 获取温度
 * @return 温度值(摄氏度)
 */
int32_t rtos_hw_get_temperature(void);

/**
 * @brief 获取电压
 * @param channel 电压通道
 * @return 电压值(mV)
 */
uint32_t rtos_hw_get_voltage(uint32_t channel);

/**
 * @brief 获取内存信息
 * @param total_memory 总内存大小
 * @param free_memory 空闲内存大小
 * @param used_memory 已使用内存大小
 */
void rtos_hw_get_memory_info(uint32_t *total_memory, uint32_t *free_memory, uint32_t *used_memory);

/**
 * @brief 获取堆栈使用情况
 * @param task_id 任务ID
 * @param stack_used 已使用堆栈大小
 * @param stack_free 空闲堆栈大小
 * @return 操作结果
 */
rtos_result_t rtos_hw_get_stack_usage(uint32_t task_id, uint32_t *stack_used, uint32_t *stack_free);

/**
 * @brief 硬件看门狗初始化
 * @param timeout_ms 超时时间(毫秒)
 * @return 操作结果
 */
rtos_result_t rtos_hw_watchdog_init(uint32_t timeout_ms);

/**
 * @brief 硬件看门狗喂狗
 */
void rtos_hw_watchdog_feed(void);

/**
 * @brief 硬件看门狗停止
 */
void rtos_hw_watchdog_stop(void);

/**
 * @brief 设置硬件定时器
 * @param timeout_ns 超时时间(纳秒)
 * @return 操作结果
 */
rtos_result_t rtos_hw_set_timer(rtos_time_ns_t timeout_ns);

/**
 * @brief 停止硬件定时器
 * @return 操作结果
 */
rtos_result_t rtos_hw_stop_timer(void);

/**
 * @brief 获取硬件定时器剩余时间
 * @return 剩余时间(纳秒)
 */
rtos_time_ns_t rtos_hw_get_timer_remaining(void);

/**
 * @brief 硬件定时器中断处理函数
 * 此函数由硬件定时器中断调用
 */
void rtos_hw_timer_interrupt_handler(void);

/**
 * @brief 获取硬件信息字符串
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 * @return 硬件信息字符串长度
 */
uint32_t rtos_hw_get_info_string(char *buffer, uint32_t size);

/* 硬件相关的宏定义 */

/* 内存屏障 */
#define RTOS_HW_MEMORY_BARRIER() __asm volatile("" : : : "memory")

/* 数据同步屏障 */
#define RTOS_HW_DSB() __asm volatile("dsb 0xf" : : : "memory")

/* 指令同步屏障 */
#define RTOS_HW_ISB() __asm volatile("isb 0xf" : : : "memory")

/* 数据内存屏障 */
#define RTOS_HW_DMB() __asm volatile("dmb 0xf" : : : "memory")

/* 原子操作 */
#define RTOS_HW_ATOMIC_ADD(ptr, val) __sync_fetch_and_add(ptr, val)
#define RTOS_HW_ATOMIC_SUB(ptr, val) __sync_fetch_and_sub(ptr, val)
#define RTOS_HW_ATOMIC_OR(ptr, val) __sync_fetch_and_or(ptr, val)
#define RTOS_HW_ATOMIC_AND(ptr, val) __sync_fetch_and_and(ptr, val)
#define RTOS_HW_ATOMIC_XOR(ptr, val) __sync_fetch_and_xor(ptr, val)

/* 原子比较交换 */
#define RTOS_HW_ATOMIC_CAS(ptr, oldval, newval) __sync_val_compare_and_swap(ptr, oldval, newval)

/* 原子交换 */
#define RTOS_HW_ATOMIC_SWAP(ptr, newval) __sync_lock_test_and_set(ptr, newval)

/* 原子释放 */
#define RTOS_HW_ATOMIC_RELEASE(ptr) __sync_lock_release(ptr)

/* 位操作 */
#define RTOS_HW_BIT_SET(reg, bit) ((reg) |= (1UL << (bit)))
#define RTOS_HW_BIT_CLEAR(reg, bit) ((reg) &= ~(1UL << (bit)))
#define RTOS_HW_BIT_TOGGLE(reg, bit) ((reg) ^= (1UL << (bit)))
#define RTOS_HW_BIT_READ(reg, bit) (((reg) >> (bit)) & 1UL)

/* 位域操作 */
#define RTOS_HW_BITFIELD_SET(reg, mask, shift, val) \
    ((reg) = ((reg) & ~((mask) << (shift))) | (((val) & (mask)) << (shift)))

#define RTOS_HW_BITFIELD_READ(reg, mask, shift) \
    (((reg) >> (shift)) & (mask))

/* 字节序转换 */
#define RTOS_HW_SWAP16(x) ((((x) & 0xFF) << 8) | (((x) & 0xFF00) >> 8))
#define RTOS_HW_SWAP32(x) ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | \
                           (((x) & 0xFF0000) >> 8) | (((x) & 0xFF000000) >> 24))

/* 对齐宏 */
#define RTOS_HW_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define RTOS_HW_ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define RTOS_HW_IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

/* 最小值/最大值 */
#define RTOS_HW_MIN(a, b) ((a) < (b) ? (a) : (b))
#define RTOS_HW_MAX(a, b) ((a) > (b) ? (a) : (b))

/* 数组大小 */
#define RTOS_HW_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* 字符串长度 */
#define RTOS_HW_STRLEN(str) (sizeof(str) - 1)

/* 条件编译 */
#ifdef RTOS_HW_DEBUG
#define RTOS_HW_DEBUG_PRINT(fmt, ...) printf("[HW] " fmt, ##__VA_ARGS__)
#else
#define RTOS_HW_DEBUG_PRINT(fmt, ...)
#endif

#ifdef RTOS_HW_ERROR_CHECK
#define RTOS_HW_ERROR_CHECK(condition, error_code) \
    do { \
        if (!(condition)) { \
            return (error_code); \
        } \
    } while(0)
#else
#define RTOS_HW_ERROR_CHECK(condition, error_code)
#endif

#endif /* __RTOS_HW_ABSTRACTION_H__ */
