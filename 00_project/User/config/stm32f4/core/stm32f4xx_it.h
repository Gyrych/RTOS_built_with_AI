/**
  ******************************************************************************
  * @file    stm32f4xx_it.h
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   STM32F407 RTOS项目中断处理函数头文件
  *          声明RTOS系统所需的中断处理函数
  ******************************************************************************
  * @attention
  *
  * 本文件声明了Tickless RTOS系统所需的关键中断处理函数：
  * 1. 系统异常处理函数 (NMI, HardFault, MemManage, BusFault, UsageFault)
  * 2. RTOS核心中断处理函数 (SVC, PendSV)
  * 3. 外设中断处理函数 (TIM2)
  * 4. 兼容性中断处理函数 (SysTick - 保留但为空)
  *
  * 注意：这是一个Tickless RTOS系统，不使用SysTick周期性中断
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void TIM2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
