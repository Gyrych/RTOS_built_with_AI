/**
  ******************************************************************************
  * @file    time.c
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   基于STM32F407 TIM2的高精度延时函数实现
  *          支持毫秒、微秒、纳秒级延时，集成RTOS任务调度
  ******************************************************************************
  * @attention
  *
  * 实现原理：
  * 1. 使用TIM2作为高精度定时器，时钟频率84MHz
  * 2. 延时开始时挂起当前任务，设置定时器目标值
  * 3. 定时器中断触发时恢复任务，进行任务调度
  * 4. 支持100ns级别的精确延时
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include "core.h"
#include "../User/config/stm32f4/core/main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 延时控制结构体 */
static delay_control_t delay_ctrl = {
    .state = DELAY_IDLE,
    .target_count = 0,
    .waiting_task = NULL
};

/* 延时开始时的基准计数值 */
static uint32_t delay_start_count = 0;

/* Private function prototypes -----------------------------------------------*/
static void tim2_config(void);
static void tim2_start_delay(uint32_t ticks);
static void tim2_stop_delay(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  TIM2定时器配置
  * @param  None
  * @retval None
  */
static void tim2_config(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    /* 使能TIM2时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    /* 配置TIM2时基单元 */
    TIM_TimeBaseStructure.TIM_Period = 0xFFFFFFFF;        /* 32位最大值 */
    TIM_TimeBaseStructure.TIM_Prescaler = 0;              /* 无分频，直接使用84MHz */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    /* 配置TIM2输出比较通道1 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;   /* 输出比较模式：定时模式 */
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable; /* 禁用输出 */
    TIM_OCInitStructure.TIM_Pulse = 0;                    /* 初始比较值 */
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);
    
    /* 使能TIM2比较中断 */
    TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
    
    /* 设置TIM2中断优先级 */
    NVIC_SetPriority(TIM2_IRQn, 3);  /* 优先级高于PendSV(15)，低于SVC(0) */
    NVIC_EnableIRQ(TIM2_IRQn);
    
    /* 启动TIM2 */
    TIM_Cmd(TIM2, ENABLE);
}

/**
  * @brief  启动TIM2延时
  * @param  ticks: 延时时钟周期数
  * @retval None
  */
static void tim2_start_delay(uint32_t ticks)
{
    uint32_t current_count;
    
    /* 获取当前计数值 */
    current_count = TIM_GetCounter(TIM2);
    
    /* 计算目标计数值 */
    delay_ctrl.target_count = current_count + ticks;
    
    /* 设置比较值 */
    TIM_SetCompare1(TIM2, delay_ctrl.target_count);
    
    /* 设置延时状态 */
    delay_ctrl.state = DELAY_ACTIVE;
    delay_ctrl.waiting_task = (void*)scheduler.current_task;
    
    /* 挂起当前任务 */
    if (delay_ctrl.waiting_task) {
        task_suspend((task_t*)delay_ctrl.waiting_task);
    }
    
    /* 进行任务调度 - 让出CPU给其他任务 */
    rtos_schedule();
    
    /* 延时完成后，任务会从这里继续执行 */
}

/**
  * @brief  停止TIM2延时
  * @param  None
  * @retval None
  */
static void tim2_stop_delay(void)
{
    /* 清除延时状态 */
    delay_ctrl.state = DELAY_IDLE;
    
    /* 恢复等待的任务 */
    if (delay_ctrl.waiting_task) {
        task_resume((task_t*)delay_ctrl.waiting_task);
        delay_ctrl.waiting_task = NULL;
    }
    
    /* 进行任务调度 */
    rtos_schedule();
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  延时系统初始化
  * @param  None
  * @retval None
  */
void Time_Init(void)
{
    /* 配置TIM2定时器 */
    tim2_config();
    
    /* 初始化延时控制结构体 */
    delay_ctrl.state = DELAY_IDLE;
    delay_ctrl.target_count = 0;
    delay_ctrl.waiting_task = NULL;
    delay_start_count = 0;
}

/**
  * @brief  延时系统反初始化
  * @param  None
  * @retval None
  */
void Time_DeInit(void)
{
    /* 停止TIM2 */
    TIM_Cmd(TIM2, DISABLE);
    
    /* 禁用TIM2中断 */
    TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE);
    NVIC_DisableIRQ(TIM2_IRQn);
    
    /* 禁用TIM2时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, DISABLE);
    
    /* 重置延时控制结构体 */
    delay_ctrl.state = DELAY_IDLE;
    delay_ctrl.target_count = 0;
    delay_ctrl.waiting_task = NULL;
    delay_start_count = 0;
}

/**
  * @brief  纳秒级延时函数
  * @param  ns: 延时时间，单位纳秒
  * @retval None
  */
void Delay_ns(uint32_t ns)
{
    uint32_t ticks;
    
    /* 参数检查 */
    if (ns < DELAY_MIN_NS) {
        ns = DELAY_MIN_NS;  /* 最小延时100ns */
    }
    
    /* 转换为时钟周期数 */
    ticks = NS_TO_TICKS(ns);
    
    /* 检查是否超出最大延时范围 */
    if (ticks > 0xFFFFFFF0) {  /* 留一些余量避免溢出 */
        ticks = 0xFFFFFFF0;
    }
    
    /* 启动延时 */
    tim2_start_delay(ticks);
}

/**
  * @brief  微秒级延时函数
  * @param  us: 延时时间，单位微秒
  * @retval None
  */
void Delay_us(uint32_t us)
{
    uint32_t ticks;
    
    /* 参数检查 */
    if (us == 0) {
        return;
    }
    
    /* 转换为时钟周期数 */
    ticks = US_TO_TICKS(us);
    
    /* 检查是否超出最大延时范围 */
    if (ticks > 0xFFFFFFF0) {  /* 留一些余量避免溢出 */
        ticks = 0xFFFFFFF0;
    }
    
    /* 启动延时 */
    tim2_start_delay(ticks);
}

/**
  * @brief  毫秒级延时函数
  * @param  ms: 延时时间，单位毫秒
  * @retval None
  */
void Delay_ms(uint32_t ms)
{
    uint32_t ticks;
    
    /* 参数检查 */
    if (ms == 0) {
        return;
    }
    
    /* 转换为时钟周期数 */
    ticks = MS_TO_TICKS(ms);
    
    /* 检查是否超出最大延时范围 */
    if (ticks > 0xFFFFFFF0) {  /* 留一些余量避免溢出 */
        ticks = 0xFFFFFFF0;
    }
    
    /* 启动延时 */
    tim2_start_delay(ticks);
}

/**
  * @brief  TIM2中断处理函数（内部调用）
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler_Internal(void)
{
    /* 检查TIM2比较中断 */
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET) {
        /* 清除中断标志 */
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
        
        /* 检查是否在延时状态 */
        if (delay_ctrl.state == DELAY_ACTIVE) {
            /* 延时完成，停止延时 */
            tim2_stop_delay();
        }
    }
}

/**
  * @brief  获取当前延时状态
  * @param  None
  * @retval 延时状态
  */
delay_state_t Time_GetDelayState(void)
{
    return delay_ctrl.state;
}

/**
  * @brief  获取剩余延时时钟周期数
  * @param  None
  * @retval 剩余时钟周期数
  */
uint32_t Time_GetRemainingTicks(void)
{
    uint32_t current_count;
    uint32_t remaining_ticks = 0;
    
    if (delay_ctrl.state == DELAY_ACTIVE) {
        current_count = TIM_GetCounter(TIM2);
        if (current_count < delay_ctrl.target_count) {
            remaining_ticks = delay_ctrl.target_count - current_count;
        }
    }
    
    return remaining_ticks;
}

/************************ (C) COPYRIGHT RTOS Team *****END OF FILE****/
