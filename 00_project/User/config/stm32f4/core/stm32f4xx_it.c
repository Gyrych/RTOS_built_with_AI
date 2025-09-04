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
//void HardFault_Handler(void)
//{
//  /* Go to infinite loop when Hard Fault exception occurs */
//  while (1)
//  {
//  }
//}

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
    extern void svc_handler(void);
    svc_handler();
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
    extern void pend_sv_handler(void);
    pend_sv_handler();
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
    extern void TIM2_IRQHandler_Internal(void);
    TIM2_IRQHandler_Internal();
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
