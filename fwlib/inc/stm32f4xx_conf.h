/**
  ******************************************************************************
  * @file    stm32f4xx_conf.h
  * @author  MCD Application Team
  * @version V1.8.1
  * @date    27-January-2022
  * @brief   Library configuration file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_CONF_H
#define __STM32F4xx_CONF_H

/* Includes ------------------------------------------------------------------*/
/* Uncomment/Comment the line below to enable/disable peripheral header file inclusion */
#include "stm32f4xx_adc.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_crc.h"
#include "stm32f4xx_cryp.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_dbgmcu.h"
#include "stm32f4xx_dcmi.h"
#include "stm32f4xx_dfsdm.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_dma2d.h"
#include "stm32f4xx_dsi.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_flash.h"
#include "stm32f4xx_flash_ramfunc.h"
#include "stm32f4xx_fmc.h"
#include "stm32f4xx_fmpi2c.h"
#include "stm32f4xx_fsmc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_hash.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_iwdg.h"
#include "stm32f4xx_lptim.h"
#include "stm32f4xx_ltdc.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_qspi.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_rng.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_sai.h"
#include "stm32f4xx_sdio.h"
#include "stm32f4xx_spdifrx.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_wwdg.h"
#include "misc.h"

/* Comment the line below to disable the specific peripheral inclusion */
/* #include "stm32f4xx_ppp.h" */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* If the following defines are not included, the TIMx peripheral driver cannot
   work correctly. It is strongly recommended to include them in all user files
   using the TIMx peripheral driver. */
#if !defined (USE_TIM2)
 #define USE_TIM2
#endif

#if !defined (USE_TIM3)
 #define USE_TIM3
#endif

#if !defined (USE_TIM4)
 #define USE_TIM4
#endif

#if !defined (USE_TIM5)
 #define USE_TIM5
#endif

#if !defined (USE_TIM6)
 #define USE_TIM6
#endif

#if !defined (USE_TIM7)
 #define USE_TIM7
#endif

#if !defined (USE_TIM9)
 #define USE_TIM9
#endif

#if !defined (USE_TIM10)
 #define USE_TIM10
#endif

#if !defined (USE_TIM11)
 #define USE_TIM11
#endif

#if !defined (USE_TIM12)
 #define USE_TIM12
#endif

#if !defined (USE_TIM13)
 #define USE_TIM13
#endif

#if !defined (USE_TIM14)
 #define USE_TIM14
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT

/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls assert_failed function
  *   which reports the name of the source file and the source
  *   line number of the call that failed. 
  *   If expr is true, it returns no value.
  * @retval None
  */
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */

#endif /* __STM32F4xx_CONF_H */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
