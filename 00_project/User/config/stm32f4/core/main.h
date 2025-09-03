/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/main.h 
  * @author  MCD Application Team
  * @version V1.8.0
  * @date    04-November-2016
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
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
void TimingDelay_Decrement(void);
void LED_Init(void);
void Delay_Init(void);
void Delay_ms(uint32_t ms);

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
