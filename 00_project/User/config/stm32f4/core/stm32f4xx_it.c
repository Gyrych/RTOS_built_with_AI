/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   STM32F407 RTOS项目中断服务程序
  *          提供RTOS系统所需的中断处理函数
  ******************************************************************************
  * @attention
  *
  * 本文件实现了Tickless RTOS系统所需的关键中断处理函数：
  * 1. SVC_Handler - 系统调用中断，用于RTOS任务调度
  * 2. PendSV_Handler - 可挂起系统调用中断，用于上下文切换
  * 3. SysTick_Handler - 保留为空，Tickless系统不使用
  * 4. TIM2_IRQHandler - TIM2中断，用于高精度延时系统
  *
  * 中断优先级配置：
  * - SVC: 0 (最高优先级)
  * - PendSV: 15 (最低优先级)
  * - TIM2: 3 (高优先级)
  * - SysTick: 不使用 (Tickless架构)
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "main.h"

/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
    /* 声明用于存储错误信息的变量 */
    uint32_t fault_address;
    uint32_t fault_status;
    uint32_t stack_pointer;
    uint32_t program_counter;
    uint32_t link_register;
    uint32_t xpsr;
    
    /* 获取当前堆栈指针 */
    __asm volatile("mrs %0, msp\n" : "=r" (stack_pointer));
    
    /* 从堆栈中读取异常时的寄存器状态 */
    uint32_t* stack_frame = (uint32_t*)stack_pointer;
    xpsr = stack_frame[0];           /* xPSR */
    program_counter = stack_frame[1]; /* PC */
    link_register = stack_frame[2];   /* LR */
    
    /* 获取故障状态寄存器 */
    fault_status = SCB->CFSR;
    fault_address = SCB->MMFAR;  /* 内存管理故障地址 */
    
    /* 打印详细的错误信息 */
    printf("\r\n");
    printf("========================================\r\n");
    printf("           HARD FAULT DETECTED!\r\n");
    printf("========================================\r\n");
    printf("Fault Status Register (CFSR): 0x%08X\r\n", fault_status);
    printf("Memory Management Fault Address: 0x%08X\r\n", fault_address);
    printf("Stack Pointer (MSP): 0x%08X\r\n", stack_pointer);
    printf("Program Counter (PC): 0x%08X\r\n", program_counter);
    printf("Link Register (LR): 0x%08X\r\n", link_register);
    printf("xPSR: 0x%08X\r\n", xpsr);
    
    /* 分析故障类型 */
    printf("\r\n--- Fault Analysis ---\r\n");
    
    if (fault_status & 0x80) {
        printf("Memory Management Fault:\r\n");
        if (fault_status & 0x10) printf("  - Instruction access violation\r\n");
        if (fault_status & 0x08) printf("  - Data access violation\r\n");
        if (fault_status & 0x02) printf("  - Memory management fault on unstacking\r\n");
        if (fault_status & 0x01) printf("  - Memory management fault on stacking\r\n");
    }
    
    if (fault_status & 0x8000) {
        printf("Bus Fault:\r\n");
        if (fault_status & 0x4000) printf("  - Instruction bus error\r\n");
        if (fault_status & 0x2000) printf("  - Precise data bus error\r\n");
        if (fault_status & 0x1000) printf("  - Imprecise data bus error\r\n");
        if (fault_status & 0x0200) printf("  - Bus fault on unstacking\r\n");
        if (fault_status & 0x0100) printf("  - Bus fault on stacking\r\n");
    }
    
    if (fault_status & 0x80000000) {
        printf("Usage Fault:\r\n");
        if (fault_status & 0x40000000) printf("  - Division by zero\r\n");
        if (fault_status & 0x20000000) printf("  - Unaligned access\r\n");
        if (fault_status & 0x10000000) printf("  - No coprocessor\r\n");
        if (fault_status & 0x00080000) printf("  - Invalid PC load\r\n");
        if (fault_status & 0x00040000) printf("  - Invalid state\r\n");
        if (fault_status & 0x00020000) printf("  - Undefined instruction\r\n");
    }
    
    /* 打印RTOS相关信息 */
    printf("\r\n--- RTOS Information ---\r\n");
    printf("Note: RTOS state may be corrupted during HardFault\r\n");
    printf("Check system memory and stack usage\r\n");
    
    printf("========================================\r\n");
    printf("System halted - check debug output above\r\n");
    printf("========================================\r\n");
    
    /* 进入无限循环，保持系统状态供调试 */
    while (1)
    {
        /* 可选：闪烁LED指示错误状态 */
        LED_R_ON();
        for(volatile uint32_t i = 0; i < 1000000; i++);
        LED_R_OFF();
        for(volatile uint32_t i = 0; i < 1000000; i++);
    }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
    printf("[SVC] System call interrupt triggered\r\n");
    extern void svc_handler(void);
    svc_handler();
    printf("[SVC] System call interrupt completed\r\n");
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
    printf("[PendSV] Context switch interrupt triggered\r\n");
    extern void pend_sv_handler(void);
    pend_sv_handler();
    printf("[PendSV] Context switch interrupt completed\r\n");
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  * @note   Tickless RTOS系统不使用SysTick
  *         此函数保留为空，避免系统异常
  */
void SysTick_Handler(void)
{
  /* Tickless RTOS - 不使用SysTick中断 */
  /* 所有延时通过TIM2高精度定时器实现 */
}

/**
  * @brief  This function handles TIM2 global interrupt.
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void)
{
    printf("[TIM2] Timer interrupt triggered\r\n");
    extern void TIM2_IRQHandler_Internal(void);
    TIM2_IRQHandler_Internal();
    printf("[TIM2] Timer interrupt completed\r\n");
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
