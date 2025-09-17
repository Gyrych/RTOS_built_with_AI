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
#include <stdint.h>
#include "rtos_config.h"
#include "hal/rtos_hal_timer.h"

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

/* 计时基准（对齐原宏命名，映射到配置项，保持对外兼容性） */
#define TIM2_CLOCK_FREQ         RTOS_TIMER_FREQ_HZ
/* 基于 TIM2_CLOCK_FREQ 的精确换算（整数近似） */
#define TIM2_NS_PER_TICK        (1000000000UL / TIM2_CLOCK_FREQ)
#define TIM2_US_PER_TICK        (TIM2_CLOCK_FREQ / 1000000UL)
#define TIM2_MS_PER_TICK        (TIM2_CLOCK_FREQ / 1000UL)

/* 延时精度定义 */
#define DELAY_MIN_NS            100UL         /* 最小延时100ns */
#define DELAY_MAX_MS            4294967UL     /* 最大延时约49.7天 (2^32/84MHz) */

/* Exported macro ------------------------------------------------------------*/

/* 时间转换宏 */
#define NS_TO_TICKS(ns)         ((ns) / TIM2_NS_PER_TICK)
#define US_TO_TICKS(us)         ((us) * TIM2_US_PER_TICK)
#define MS_TO_TICKS(ms)         ((ms) * TIM2_MS_PER_TICK)

/* 反向换算（调试用途，注意可能存在整数溢出，必要时改用64位） */
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

/**
 * 文件功能：
 * - 提供基于 TIM2@84MHz 的高精度无滴答延时能力（ns/us/ms），并与 RTOS 调度协同。
 * - 线程调用 Delay_*() 时当前任务挂起；TIM2 比较中断到期后恢复任务并触发一次调度。
 *
 * 调用方法：
 * - 在系统初始化后依次调用：Time_Init();
 * - 在任务中直接调用：Delay_ns/Delay_us/Delay_ms；禁止在中断中调用阻塞延时。
 * - 退出前可调用：Time_DeInit();
 *
 * 依赖与中断优先级：
 * - 依赖标准外设库 TIM2；TIM2_IRQn 优先级建议设置为 3（高于 PendSV=15，低于 SVC=0）。
 *
 * 注意事项：
 * - 使用 32 位自增计数并支持回绕判断；大量并发延时通过队列管理并选择最近比较点。
 * - TICKS_TO_* 换算宏主要用于调试，存在整数溢出风险时请使用 64 位计算。
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIME_H__ */

/************************ (C) COPYRIGHT RTOS Team *****END OF FILE****/
