/**
 * @file interrupt_handler.c
 * @brief RTOS中断处理程序 - Tickless系统集成
 * @author Assistant
 * @date 2024
 */

#include "hw_abstraction.h"
#include "hw_config.h"
#include "../time/tickless.h"
#include "../time/dynamic_delay.h"
#include "../task/task.h"

/* 包含STM32F4标准固件库头文件 */
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_tim.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/misc.h"

/**
 * @brief 硬件定时器中断处理程序
 * 替代SysTick中断，实现tickless调度
 */
void TIM2_IRQHandler(void)
{
    /* 检查TIM2更新中断标志 */
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        /* 清除中断标志 */
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        
        /* 调用硬件抽象层中断处理 */
        rtos_hw_timer_interrupt_handler();
        
        /* 调用动态延时管理器中断处理 */
        rtos_dynamic_delay_interrupt_handler();
    }
}

/**
 * @brief 系统时钟中断处理程序（禁用）
 * 在tickless系统中不再使用SysTick
 */
void SysTick_Handler(void)
{
    /* Tickless系统中不使用SysTick */
    /* 如果意外触发，直接返回 */
}

/**
 * @brief PendSV中断处理程序
 * 用于任务上下文切换
 */
void PendSV_Handler(void)
{
    /* 执行任务上下文切换 */
    /* 这里需要实现具体的上下文切换汇编代码 */
    
    /* 保存当前任务上下文 */
    /* 恢复新任务上下文 */
    
    /* 简化实现：调用调度器 */
    extern void rtos_scheduler_schedule(void);
    rtos_scheduler_schedule();
}

/**
 * @brief 系统调用中断处理程序
 * 用于从中断中触发调度
 */
void SVC_Handler(void)
{
    /* 处理系统调用 */
    /* 可以用于实现系统调用接口 */
}

/**
 * @brief 初始化中断系统
 * 配置tickless系统所需的中断
 */
void rtos_interrupt_system_init(void)
{
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        /* 配置TIM2中断优先级 */
        NVIC_InitTypeDef nvic_config;
        
        /* TIM2中断优先级配置 */
        nvic_config.NVIC_IRQChannel = TIM2_IRQn;
        nvic_config.NVIC_IRQChannelPreemptionPriority = RTOS_HW_TIMER_IRQ_PRIORITY;
        nvic_config.NVIC_IRQChannelSubPriority = 0;
        nvic_config.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic_config);
        
        /* 配置PendSV中断优先级（最低优先级） */
        nvic_config.NVIC_IRQChannel = PendSV_IRQn;
        nvic_config.NVIC_IRQChannelPreemptionPriority = RTOS_HW_PENDSV_IRQ_PRIORITY;
        nvic_config.NVIC_IRQChannelSubPriority = 0;
        nvic_config.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic_config);
        
        /* 配置SVC中断优先级 */
        nvic_config.NVIC_IRQChannel = SVCall_IRQn;
        nvic_config.NVIC_IRQChannelPreemptionPriority = RTOS_HW_SVC_IRQ_PRIORITY;
        nvic_config.NVIC_IRQChannelSubPriority = 0;
        nvic_config.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic_config);
        
        /* 禁用SysTick中断 */
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    #endif
}

/**
 * @brief 触发调度中断
 * 用于在中断中安全地触发任务调度
 */
void rtos_trigger_schedule_interrupt(void)
{
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        /* 触发PendSV中断来执行调度 */
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    #endif
}