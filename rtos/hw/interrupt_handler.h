/**
 * @file interrupt_handler.h
 * @brief RTOS中断处理程序接口 - Tickless系统集成
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_INTERRUPT_HANDLER_H__
#define __RTOS_INTERRUPT_HANDLER_H__

#include "../core/types.h"

/**
 * @brief 硬件定时器中断处理程序
 * 替代SysTick中断，实现tickless调度
 */
void TIM2_IRQHandler(void);

/**
 * @brief 系统时钟中断处理程序（禁用）
 * 在tickless系统中不再使用SysTick
 */
void SysTick_Handler(void);

/**
 * @brief PendSV中断处理程序
 * 用于任务上下文切换
 */
void PendSV_Handler(void);

/**
 * @brief 系统调用中断处理程序
 * 用于从中断中触发调度
 */
void SVC_Handler(void);

/**
 * @brief 初始化中断系统
 * 配置tickless系统所需的中断
 */
void rtos_interrupt_system_init(void);

/**
 * @brief 触发调度中断
 * 用于在中断中安全地触发任务调度
 */
void rtos_trigger_schedule_interrupt(void);

#endif /* __RTOS_INTERRUPT_HANDLER_H__ */