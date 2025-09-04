/**
  ******************************************************************************
  * @file    main.h
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   STM32F407 RTOS项目主头文件
  *          定义硬件抽象层接口和LED控制宏
  ******************************************************************************
  * @attention
  *
  * 本文件定义了STM32F407 RTOS项目的硬件抽象层：
  * 1. LED控制宏定义（星火一号开发板）
  * 2. 硬件初始化函数声明
  * 3. 兼容性函数声明
  * 
  * 硬件平台：星火一号开发板 (STM32F407VGTx)
  * LED引脚：GPIOF Pin 11
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stddef.h>

/* LED引脚定义 - 星火一号开发版 */
#define LED_GPIO_PORT                GPIOF
#define LED_GPIO_CLK                 RCC_AHB1Periph_GPIOF
#define LED_GPIO_PIN                 GPIO_Pin_11
#define LED_GPIO_PIN_SOURCE          GPIO_PinSource11

/* LED控制宏 */
#define LED_ON()                     GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN)
#define LED_OFF()                    GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN)
#define LED_TOGGLE()                 GPIO_WriteBit(LED_GPIO_PORT, LED_GPIO_PIN, (BitAction)!GPIO_ReadOutputDataBit(LED_GPIO_PORT, LED_GPIO_PIN))

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void LED_Init(void);

/* 高精度延时函数声明 - 在time.h中定义 */
/* void Delay_ns(uint32_t ns);   - 在time.h中声明 */
/* void Delay_us(uint32_t us);   - 在time.h中声明 */
/* void Delay_ms(uint32_t ms);   - 在time.h中声明 */
/* void Time_Init(void);         - 在time.h中声明 */

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
