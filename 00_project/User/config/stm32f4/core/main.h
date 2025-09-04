/**
  ******************************************************************************
  * @file    main.h
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   STM32F407 RTOS项目主头文件
  *          定义硬件抽象层接口、LED控制宏和UART1串口
  ******************************************************************************
  * @attention
  *
  * 本文件定义了STM32F407 RTOS项目的硬件抽象层：
  * 1. 绿色LED控制宏定义（PF11引脚）
  * 2. 红色LED控制宏定义（PF12引脚）
  * 3. UART1串口配置和printf重定向
  * 4. 硬件初始化函数声明
  * 
  * 硬件平台：星火一号开发板 (STM32F407VGTx)
  * LED引脚：绿色LED - PF11，红色LED - PF12
  * 串口：UART1 - PA9(TX), PA10(RX)，波特率115200
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stddef.h>
#include <stdio.h>

/* LED引脚定义 - 星火一号开发版 */
/* 绿色LED - PF11 */
#define LED_G_GPIO_PORT              GPIOF
#define LED_G_GPIO_CLK               RCC_AHB1Periph_GPIOF
#define LED_G_GPIO_PIN               GPIO_Pin_11
#define LED_G_GPIO_PIN_SOURCE        GPIO_PinSource11

/* 红色LED - PF12 */
#define LED_R_GPIO_PORT              GPIOF
#define LED_R_GPIO_CLK               RCC_AHB1Periph_GPIOF
#define LED_R_GPIO_PIN               GPIO_Pin_12
#define LED_R_GPIO_PIN_SOURCE        GPIO_PinSource12

/* LED控制宏 - 绿色LED */
#define LED_G_ON()                   GPIO_SetBits(LED_G_GPIO_PORT, LED_G_GPIO_PIN)
#define LED_G_OFF()                  GPIO_ResetBits(LED_G_GPIO_PORT, LED_G_GPIO_PIN)
#define LED_G_TOGGLE()               GPIO_WriteBit(LED_G_GPIO_PORT, LED_G_GPIO_PIN, (BitAction)!GPIO_ReadOutputDataBit(LED_G_GPIO_PORT, LED_G_GPIO_PIN))

/* LED控制宏 - 红色LED */
#define LED_R_ON()                   GPIO_SetBits(LED_R_GPIO_PORT, LED_R_GPIO_PIN)
#define LED_R_OFF()                  GPIO_ResetBits(LED_R_GPIO_PORT, LED_R_GPIO_PIN)
#define LED_R_TOGGLE()               GPIO_WriteBit(LED_R_GPIO_PORT, LED_R_GPIO_PIN, (BitAction)!GPIO_ReadOutputDataBit(LED_R_GPIO_PORT, LED_R_GPIO_PIN))

/* 兼容性宏定义 - 保持向后兼容 */
#define LED_GPIO_PORT                LED_G_GPIO_PORT
#define LED_GPIO_CLK                 LED_G_GPIO_CLK
#define LED_GPIO_PIN                 LED_G_GPIO_PIN
#define LED_GPIO_PIN_SOURCE          LED_G_GPIO_PIN_SOURCE
#define LED_ON()                     LED_G_ON()
#define LED_OFF()                    LED_G_OFF()
#define LED_TOGGLE()                 LED_G_TOGGLE()

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void LED_Init(void);
void LED_G_Init(void);
void LED_R_Init(void);
void UART1_Init(void);
int fputc(int ch, FILE *f);

/* 高精度延时函数声明 - 在time.h中定义 */
/* void Delay_ns(uint32_t ns);   - 在time.h中声明 */
/* void Delay_us(uint32_t us);   - 在time.h中声明 */
/* void Delay_ms(uint32_t ms);   - 在time.h中声明 */
/* void Time_Init(void);         - 在time.h中声明 */

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
