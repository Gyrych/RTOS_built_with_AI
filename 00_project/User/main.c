/**
  ******************************************************************************
  * @file    main.c
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   STM32F407 RTOS双任务交替执行演示程序
  *          基于STM32F407标准库和自定义RTOS系统
  *          实现任务主动挂起和恢复机制
  ******************************************************************************
  * @attention
  *
  * 本程序演示了自定义RTOS系统的任务主动调度功能：
  * 1. 绿色LED控制任务 - PF11引脚，主动挂起让红色LED任务执行
  * 2. 红色LED控制任务 - PF12引脚，主动挂起让绿色LED任务执行
  * 
  * 硬件平台：星火一号开发板 (STM32F407VGTx)
  * LED引脚：绿色LED - PF11，红色LED - PF12
  * 任务调度：两个任务交替执行，不使用Delay_ms挂起
  *
  ******************************************************************************
  */
#include "main.h"
#include "../../02_rtos/core.h"
#include "../../02_rtos/time.h"
#include <stdio.h>
#include "../../02_rtos/uart.h"
#include "../../02_rtos/log.h"

/* 私有变量定义 - 已移除废弃的TimingDelay变量 */
static task_t* green_led_task = NULL;
static task_t* red_led_task = NULL;

/* 示例任务函数声明 */
void task_led_g_blink(void* arg);
void task_led_r_blink(void* arg);

/* 简单延时函数 */
void simple_delay(uint32_t count);

/* UART和系统信息打印函数（由 RTOS UART 接管） */
int _write(int file, char *ptr, int len);
int _read(int file, char *ptr, int len);
void print_system_banner(void);

/**
  * @brief  主函数
  * @param  None
  * @retval None
  */
int main(void)
{
    /* 系统时钟初始化 */
    SystemInit();
    
    /* LED初始化 */
    LED_G_Init();
    LED_R_Init();
    
    /* RTOS UART 初始化（USART1 + DMA） - 用于系统信息打印 */
    rtos_uart_init();
    /* 初始化异步日志系统（后台冲刷） */
    rtos_log_init();
    
    /* 打印炫酷的系统启动横幅 */
    print_system_banner();
    
    /* 打印调试配置信息 */
    printf("\r\n=== Debug Configuration ===\r\n");
    printf("RTOS Debug Enabled: %s\r\n", RTOS_DEBUG_ENABLE ? "YES" : "NO");
    printf("RTOS Debug Level: %d\r\n", RTOS_DEBUG_LEVEL);
    printf("Debug Levels: 0=Off, 1=Basic, 2=Detailed, 3=Complete\r\n");
    printf("=============================\r\n");
    
    /* 配置中断优先级 - Tickless RTOS系统 */
    NVIC_SetPriority(SVCall_IRQn, 0);      /* SVC中断优先级设为最高 */
    NVIC_SetPriority(PendSV_IRQn, 15);     /* PendSV中断优先级设为最低 */
    /* 注意：不使用SysTick中断，系统采用事件驱动架构 */
    
    /* 初始化高精度延时子系统（TIM2@84MHz） */
    Time_Init();
    printf("\r\n[Time] High-precision delay subsystem enabled (TIM2 @ %lu Hz)\r\n", (unsigned long)TIM2_CLOCK_FREQ);
    
    /* RTOS初始化 */
    rtos_init();

    /* 创建日志后台任务（最低优先级） */
    task_create(rtos_log_task, NULL, 30);
    
    /* 打印调度器信息 */
    rtos_debug_print_scheduler_info();
    
    /* 创建两个LED控制任务 */
    printf("\r\n=== Creating LED Control Tasks ===\r\n");
    /* 调整优先级：红(1) > 绿(2)，确保红灯到期后更先运行 */
    red_led_task = task_create(task_led_r_blink, NULL, 1);      /* 红色LED控制任务 */
    green_led_task = task_create(task_led_g_blink, NULL, 2);    /* 绿色LED控制任务 */
    
    /* 打印任务创建后的调度器信息 */
    printf("\r\n=== After Task Creation ===\r\n");
    rtos_debug_print_scheduler_info();
    
    /* 打印任务详细信息 */
    if (green_led_task) {
        printf("\r\n=== Green LED Task Info ===\r\n");
        rtos_debug_print_task_info(green_led_task);
    }
    
    if (red_led_task) {
        printf("\r\n=== Red LED Task Info ===\r\n");
        rtos_debug_print_task_info(red_led_task);
    }
    
    /* 启动RTOS调度器 */
    printf("\r\n=== Starting RTOS Scheduler ===\r\n");
    rtos_start();
    
    /* 程序不会执行到这里，因为RTOS会接管控制权 */
    /* 如果执行到这里，说明RTOS启动失败 */
    while(1)
    {
        /* 如果RTOS启动失败，LED会快速闪烁表示错误 */
        LED_TOGGLE();
        /* 使用简单的循环延时，避免调用Delay_ms */
        for(volatile uint32_t i = 0; i < 1000000; i++);
    }
}

/**
  * @brief  绿色LED控制任务 - 主动挂起让红色LED任务执行
  * @param  arg: 任务参数（未使用）
  * @retval None
  */
void task_led_g_blink(void* arg)
{
    uint32_t cycle_count = 0;
    
    printf("\r\n[GREEN-TASK] Green LED task started\r\n");
    
    while(1)
    {
        LED_G_ON();
        printf("[GREEN-TASK] Cycle %d: Turning ON green LED (sleep 500ms)\r\n", cycle_count);
        Delay_ms(500);
        
        LED_G_OFF();
        printf("[GREEN-TASK] Cycle %d: Turning OFF green LED (sleep 500ms)\r\n", cycle_count);
        Delay_ms(500);
        
        cycle_count++;
    }
}


/**
  * @brief  红色LED控制任务 - 主动挂起让绿色LED任务执行
  * @param  arg: 任务参数（未使用）
  * @retval None
  */
void task_led_r_blink(void* arg)
{
    uint32_t cycle_count = 0;
    
    printf("\r\n[RED-TASK] Red LED task started\r\n");
    
    while(1)
    {
        LED_R_ON();
        printf("[RED-TASK] Cycle %d: Turning ON red LED (sleep 100ms)\r\n", cycle_count);
        Delay_ms(100);
        
        LED_R_OFF();
        printf("[RED-TASK] Cycle %d: Turning OFF red LED (sleep 100ms)\r\n", cycle_count);
        Delay_ms(100);
        
        cycle_count++;
    }
}

/**
  * @brief  简单延时函数
  * @param  count: 延时循环次数
  * @retval None
  */
void simple_delay(uint32_t count)
{
    volatile uint32_t i;
    for (i = 0; i < count; i++);
}

/**
  * @brief  绿色LED初始化函数
  * @param  None
  * @retval None
  */
void LED_G_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能GPIOF时钟 */
    RCC_AHB1PeriphClockCmd(LED_G_GPIO_CLK, ENABLE);
    
    /* 配置绿色LED引脚为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = LED_G_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LED_G_GPIO_PORT, &GPIO_InitStructure);
    
    /* 初始状态：绿色LED关闭 */
    LED_G_OFF();
}

/**
  * @brief  红色LED初始化函数
  * @param  None
  * @retval None
  */
void LED_R_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能GPIOF时钟 */
    RCC_AHB1PeriphClockCmd(LED_R_GPIO_CLK, ENABLE);
    
    /* 配置红色LED引脚为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = LED_R_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LED_R_GPIO_PORT, &GPIO_InitStructure);
    
    /* 初始状态：红色LED关闭 */
    LED_R_OFF();
}

/* 兼容性函数 LED_Init 已移除：未被使用，保留会造成冗余，删除不影响功能 */

/* 旧的 UART1_Init 与 fputc/fgetc 已由 RTOS UART 接管并移除 */

/**
  * @brief  printf重定向函数
  * @param  ch: 要输出的字符
  * @param  f: 文件指针（未使用）
  * @retval 输出的字符
  */
/* fputc 不再使用，_write 负责标准输出 */

/**
  * @brief  fgetc重定向函数（nano.specs需要）
  * @param  f: 文件指针（未使用）
  * @retval 读取的字符
  */
/* fgetc 不再使用，_read 负责标准输入 */

/**
  * @brief  _write重定向函数（nano.specs需要）
  * @param  file: 文件描述符
  * @param  ptr: 数据指针
  * @param  len: 数据长度
  * @retval 写入的字节数
  */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if (!ptr || len <= 0) return 0;
    return rtos_uart_write_blocking((const uint8_t*)ptr, (uint16_t)len);
}

/**
  * @brief  _read重定向函数（nano.specs需要）
  * @param  file: 文件描述符
  * @param  ptr: 数据指针
  * @param  len: 数据长度
  * @retval 读取的字节数
  */
int _read(int file, char *ptr, int len)
{
    /* 简化：当前不提供阻塞读取，直接返回0；后续可注册回调或提供队列 */
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

/**
  * @brief  打印炫酷的系统启动横幅
  * @param  None
  * @retval None
  */
void print_system_banner(void)
{
    /* 精简版启动信息：不使用大段 ASCII/emoji，仅输出关键配置 */
    printf("\r\n==== Tickless RTOS Startup ====\r\n");
    printf("MCU: STM32F407VGTx @ 168MHz\r\n");
    printf("Arch: Cortex-M4F | RTOS: Custom Tickless\r\n");
    printf("LED: PF11 (Green), PF12 (Red)\r\n");
    printf("UART1: 115200, DMA RX(IDLE)+TX DMA\r\n");
    printf("===============================\r\n");
}


/* Delay_Init函数已移除 - 请使用Time_Init()替代 */

/* 
 * Tickless RTOS系统 - 不使用SysTick
 * 所有延时都通过TIM2高精度定时器实现
 * 系统采用事件驱动架构，无需周期性时钟中断
 */
