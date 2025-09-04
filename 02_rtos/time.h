/**
  ******************************************************************************
  * @file    time.h
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   基于STM32F407 TIM2的高精度延时函数头文件
  *          支持毫秒、微秒、纳秒级延时，集成RTOS任务调度
  ******************************************************************************
  * @attention
  *
  * 本文件实现了基于TIM2定时器的高精度延时功能，具有以下特性：
  * 1. 支持毫秒(ms)、微秒(us)、纳秒(ns)级延时
  * 2. 延时期间任务挂起，进行RTOS任务调度
  * 3. 定时器到时后恢复任务，再次进行调度
  * 4. 纳秒级精度可达100ns级别
  *
  ******************************************************************************
  */

#ifndef __TIME_H__
#define __TIME_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/* 延时状态枚举 */
typedef enum {
    DELAY_IDLE = 0,     /* 空闲状态 */
    DELAY_ACTIVE,       /* 延时进行中 */
    DELAY_COMPLETED     /* 延时完成 */
} delay_state_t;

/* 延时控制结构体 */
typedef struct {
    delay_state_t state;        /* 延时状态 */
    uint32_t target_count;      /* 目标计数值 */
    void* waiting_task;         /* 等待延时的任务指针 */
} delay_control_t;

/* Exported constants --------------------------------------------------------*/

/* TIM2相关定义 */
#define TIM2_CLOCK_FREQ         84000000UL    /* TIM2时钟频率: 84MHz */
#define TIM2_NS_PER_TICK        12UL          /* 每个时钟周期约12ns (1/84MHz) */
#define TIM2_US_PER_TICK        1000UL        /* 每微秒需要的时钟周期数 */
#define TIM2_MS_PER_TICK        1000000UL     /* 每毫秒需要的时钟周期数 */

/* 延时精度定义 */
#define DELAY_MIN_NS            100UL         /* 最小延时100ns */
#define DELAY_MAX_MS            4294967UL     /* 最大延时约49.7天 (2^32/84MHz) */

/* Exported macro ------------------------------------------------------------*/

/* 时间转换宏 */
#define NS_TO_TICKS(ns)         ((ns) / TIM2_NS_PER_TICK)
#define US_TO_TICKS(us)         ((us) * TIM2_US_PER_TICK)
#define MS_TO_TICKS(ms)         ((ms) * TIM2_MS_PER_TICK)

#define TICKS_TO_NS(ticks)      ((ticks) * TIM2_NS_PER_TICK)
#define TICKS_TO_US(ticks)      ((ticks) / TIM2_US_PER_TICK)
#define TICKS_TO_MS(ticks)      ((ticks) / TIM2_MS_PER_TICK)

/* Exported functions ------------------------------------------------------- */

/* 延时函数 */
void Delay_ns(uint32_t ns);     /* 纳秒级延时 */
void Delay_us(uint32_t us);     /* 微秒级延时 */
void Delay_ms(uint32_t ms);     /* 毫秒级延时 */

/* 延时系统管理函数 */
void Time_Init(void);           /* 延时系统初始化 */
void Time_DeInit(void);         /* 延时系统反初始化 */

/* 内部函数（供中断处理使用） */
void TIM2_IRQHandler_Internal(void);  /* TIM2中断处理函数 */

/* 延时状态查询函数 */
delay_state_t Time_GetDelayState(void);  /* 获取当前延时状态 */
uint32_t Time_GetRemainingTicks(void);   /* 获取剩余延时时钟周期数 */

#ifdef __cplusplus
}
#endif

#endif /* __TIME_H__ */

/************************ (C) COPYRIGHT RTOS Team *****END OF FILE****/
