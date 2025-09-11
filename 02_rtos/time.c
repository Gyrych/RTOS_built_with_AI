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

/* Private typedef -----------------------------------------------------------*/
/* 并发延时队列条目 */
typedef struct {
    task_t* task;               /* 等待的任务 */
    uint32_t target_count;      /* 目标计数值（相对 TIM2 计数器） */
} delay_entry_t;

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
/* 删除未使用的延时起始快照变量：不影响任何功能路径 */

/* 延时等待队列与计数 */
static delay_entry_t delay_queue[MAX_TASKS];
static uint8_t delay_queue_count = 0;

/* Private function prototypes -----------------------------------------------*/
static void tim2_config(void);
static void tim2_start_delay(uint32_t ticks);
static void tim2_stop_delay(void);
static void delay_queue_add(task_t* task, uint32_t target_count);
static void delay_queue_remove_task(task_t* task);
static void delay_queue_schedule_next_compare(void);
static int  delay_queue_resume_due_and_rearm(void);
static int  find_delay_entry_index_for_task(task_t* task);
static inline uint32_t ticks_now(void) { return TIM_GetCounter(TIM2); }
static inline void tim2_irq_disable(void) { NVIC_DisableIRQ(TIM2_IRQn); }
static inline void tim2_irq_enable(void)  { NVIC_EnableIRQ(TIM2_IRQn); }
static void delay_queue_dump(const char* tag);

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
    uint32_t current_count = ticks_now();
    uint32_t target = current_count + ticks;
    task_t* current_task = scheduler.current_task;

    /* 将当前任务加入延时队列（去重）并根据最近到期点更新CCR */
    /* 禁止 TIM2 中断，避免与 ISR 并发修改队列 */
    tim2_irq_disable();
    rtos_enter_critical();
    delay_queue_remove_task(current_task);
    delay_queue_add(current_task, target);
    delay_ctrl.state = DELAY_ACTIVE;
    delay_ctrl.target_count = target; /* 记录最近一次加入的目标值（统计用途） */
    delay_ctrl.waiting_task = NULL;   /* 并发模型下不再使用单一等待者 */
    delay_queue_schedule_next_compare();
    RTOS_DEBUG_PRINT(3, "[TIME] enqueue: task=%p now=%u ticks=%u target=%u CCR1=%u qcnt=%d",
                     current_task, current_count, ticks, target, TIM_GetCapture1(TIM2), delay_queue_count);
    delay_queue_dump("after-enqueue");
    rtos_exit_critical();
    tim2_irq_enable();

    /* 挂起当前任务并请求一次调度 */
    if (current_task) {
        task_suspend(current_task);
    }
    rtos_schedule();
}

/**
  * @brief  停止TIM2延时
  * @param  None
  * @retval None
  */
/* 删除未使用的停止接口：当前模型按比较点自动重装，保留会造成误导 */

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  延时系统初始化
  * @param  None
  * @retval None
  */
void Time_Init(void)
{
    RTOS_DEBUG_PRINT(1, "=== Time System Initialization Started ===");

    /* 配置TIM2定时器 */
    tim2_config();
    RTOS_DEBUG_PRINT(2, "TIM2 timer configured");

    /* 初始化延时控制结构体 */
    delay_ctrl.state = DELAY_IDLE;
    delay_ctrl.target_count = 0;
    delay_ctrl.waiting_task = NULL;
    /* 去除未用变量的初始化 */

    RTOS_DEBUG_PRINT(1, "Delay control structure initialized");
    RTOS_DEBUG_PRINT(2, "TIM2 clock frequency: %d Hz", TIM2_CLOCK_FREQ);
    RTOS_DEBUG_PRINT(2, "Minimum delay: %d ns", DELAY_MIN_NS);
    RTOS_DEBUG_PRINT(1, "=== Time System Initialization Completed ===");
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
    /* 去除未用变量的复位 */
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

    RTOS_DEBUG_PRINT(3, "Delay_ms called: %d ms", ms);

    /* 参数检查 */
    if (ms == 0) {
        RTOS_DEBUG_PRINT(3, "Delay_ms: zero delay, returning");
        return;
    }

    /* 转换为时钟周期数 */
    ticks = MS_TO_TICKS(ms);
    RTOS_DEBUG_PRINT(3, "Delay_ms: %d ms = %d ticks", ms, ticks);

    /* 检查是否超出最大延时范围 */
    if (ticks > 0xFFFFFFF0) {  /* 留一些余量避免溢出 */
        ticks = 0xFFFFFFF0;
        RTOS_DEBUG_PRINT(2, "Delay_ms: ticks limited to 0xFFFFFFF0");
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
        /* 恢复到期任务并重装下一比较点（无打印，避免中断内阻塞IO） */
        (void)delay_queue_resume_due_and_rearm();
    }
}

/**
  * @brief  获取当前延时状态
  * @param  None
  * @retval 延时状态
  */
delay_state_t Time_GetDelayState(void)
{
    return (delay_queue_count > 0) ? DELAY_ACTIVE : DELAY_IDLE;
}

/**
  * @brief  获取剩余延时时钟周期数
  * @param  None
  * @retval 剩余时钟周期数
  */
uint32_t Time_GetRemainingTicks(void)
{
    uint32_t now = ticks_now();
    uint32_t remaining_ticks = 0;

    int idx = find_delay_entry_index_for_task(scheduler.current_task);
    if (idx >= 0) {
        /* 计算从 now 到目标的剩余 ticks（支持回绕的无符号差） */
        remaining_ticks = (uint32_t)(delay_queue[idx].target_count - now);
    }

    return remaining_ticks;
}

/* ============================ 内部辅助函数 ============================ */

/* 将任务加入延时队列（不去重） */
static void delay_queue_add(task_t* task, uint32_t target_count)
{
    if (!task) return;
    if (delay_queue_count >= MAX_TASKS) {
        /* 队列已满：为安全起见直接恢复该任务（避免死锁） */
        task_resume(task);
        return;
    }
    delay_queue[delay_queue_count].task = task;
    delay_queue[delay_queue_count].target_count = target_count;
    delay_queue_count++;
}

/* 移除队列中指定任务的所有条目 */
static void delay_queue_remove_task(task_t* task)
{
    if (!task || delay_queue_count == 0) return;
    for (uint8_t i = 0; i < delay_queue_count; ) {
        if (delay_queue[i].task == task) {
            for (uint8_t j = i; j + 1 < delay_queue_count; j++) {
                delay_queue[j] = delay_queue[j + 1];
            }
            delay_queue_count--;
            continue;
        }
        i++;
    }
}

/* 装载下一即将到期的比较值到 CCR1 */
static void delay_queue_schedule_next_compare(void)
{
    if (delay_queue_count == 0) {
        delay_ctrl.state = DELAY_IDLE;
        return;
    }

    uint32_t now = ticks_now();
    uint32_t min_delta = 0xFFFFFFFFu;
    uint32_t next_target = now + min_delta;

    for (uint8_t i = 0; i < delay_queue_count; i++) {
        uint32_t target = delay_queue[i].target_count;
        uint32_t delta = (uint32_t)(target - now); /* 无符号差，天然支持回绕 */
        if (delta < min_delta) {
            min_delta = delta;
            next_target = target;
        }
    }

    /* 避免在设置CCR时窗口滑过比较点：若过近则推迟2个tick */
    uint32_t now2 = ticks_now();
    if ((int32_t)(next_target - now2) <= 1) {
        next_target = now2 + 2;
    }
    /* 设置CCR1为最近到期的目标 */
    TIM_SetCompare1(TIM2, next_target);
    delay_ctrl.target_count = next_target;
    delay_ctrl.state = DELAY_ACTIVE;
}

/* 恢复所有到期任务，并重装下一比较点；返回恢复任务数 */
static int delay_queue_resume_due_and_rearm(void)
{
    int resumed = 0;
    rtos_enter_critical();
    uint32_t now = ticks_now();

    /* 遍历并恢复所有已到期的任务（支持回绕判断） */
    for (uint8_t i = 0; i < delay_queue_count; ) {
        delay_entry_t entry = delay_queue[i];
        if ((int32_t)(now - entry.target_count) >= 0) {
            task_resume(entry.task);
            resumed++;
            /* 移除该条目（紧缩） */
            for (uint8_t j = i; j + 1 < delay_queue_count; j++) {
                delay_queue[j] = delay_queue[j + 1];
            }
            delay_queue_count--;
            continue; /* 紧缩后当前位置继续检查 */
        }
        i++;
    }

    /* 重装下一比较点（若队列非空） */
    if (delay_queue_count > 0) {
        delay_queue_schedule_next_compare();
    } else {
        delay_ctrl.state = DELAY_IDLE;
    }

    rtos_exit_critical();

    if (resumed) {
        /* 请求从中断进行一次上下文切换 */
        rtos_request_context_switch_from_isr();
    }
    return resumed;
}

/* 查找指定任务在延时队列中的索引（未找到返回 -1） */
static int find_delay_entry_index_for_task(task_t* task)
{
    if (!task) return -1;
    for (uint8_t i = 0; i < delay_queue_count; i++) {
        if (delay_queue[i].task == task) return (int)i;
    }
    return -1;
}

/* 打印队列内容（调试级别3下有效） */
static void delay_queue_dump(const char* tag)
{
    uint32_t now = ticks_now();
    RTOS_DEBUG_PRINT(3, "[TIME][DUMP-%s] now=%u qcnt=%d", tag ? tag : "", now, delay_queue_count);
    for (uint8_t i = 0; i < delay_queue_count; i++) {
        uint32_t target = delay_queue[i].target_count;
        uint32_t delta = (uint32_t)(target - now);
        RTOS_DEBUG_PRINT(3, "  [%d] task=%p target=%u delta_ticks=%u", i, delay_queue[i].task, target, delta);
    }
}

/************************ (C) COPYRIGHT RTOS Team *****END OF FILE****/
