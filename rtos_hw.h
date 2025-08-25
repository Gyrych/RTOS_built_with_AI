/**
 * @file rtos_hw.h
 * @brief STM32F407硬件抽象层头文件
 */

#ifndef __RTOS_HW_H__
#define __RTOS_HW_H__

#include <stdint.h>

/* STM32F407寄存器定义 */
#define NVIC_INT_CTRL           0xE000ED04  /* 中断控制状态寄存器 */
#define NVIC_PENDSVSET          0x10000000  /* PendSV悬起位 */
#define NVIC_SYSPRI2            0xE000ED20  /* 系统优先级寄存器2 */
#define NVIC_PENDSV_PRI         0x00FF0000  /* PendSV优先级 */
#define NVIC_SYSTICK_PRI        0xFF000000  /* SysTick优先级 */

/* Cortex-M4 PSR寄存器位定义 */
#define PSR_THUMB_BIT           0x01000000  /* Thumb状态位 */

/* 硬件相关函数声明 */
void rtos_hw_init(void);
void rtos_hw_timer_init(void);
void rtos_hw_timer_set(uint32_t microseconds);
void rtos_hw_timer_stop(void);
uint32_t rtos_hw_get_time_us(void);
void rtos_hw_disable_interrupts(void);
void rtos_hw_enable_interrupts(void);
void rtos_hw_start_first_task(void);

/* 上下文切换相关 */
void rtos_context_switch(void);
uint32_t *rtos_task_stack_init(uint32_t *stack_top, void (*task_func)(void *), void *param);

#endif /* __RTOS_HW_H__ */