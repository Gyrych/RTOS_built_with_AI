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
#include <stdio.h>

/* 私有变量定义 - 已移除废弃的TimingDelay变量 */
static task_t* green_led_task = NULL;
static task_t* red_led_task = NULL;

/* 示例任务函数声明 */
void task_led_g_blink(void* arg);
void task_led_r_blink(void* arg);

/* 简单延时函数 */
void simple_delay(uint32_t count);

/* UART和系统信息打印函数 */
void UART1_Init(void);
int fputc(int ch, FILE *f);
int fgetc(FILE *f);
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
    
    /* UART1初始化 - 用于系统信息打印 */
    UART1_Init();
    
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
    
    /* RTOS初始化 */
    rtos_init();
    
    /* 打印调度器信息 */
    rtos_debug_print_scheduler_info();
    
    /* 创建两个LED控制任务 */
    printf("\r\n=== Creating LED Control Tasks ===\r\n");
    green_led_task = task_create(task_led_g_blink, NULL, 1);    /* 绿色LED控制任务 */
    red_led_task = task_create(task_led_r_blink, NULL, 2);      /* 红色LED控制任务 */
    
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
        printf("[GREEN-TASK] Cycle %d: Turning ON green LED\r\n", cycle_count);
        LED_G_ON();
        simple_delay(1000000);  /* 简单延时 */
        
        printf("[GREEN-TASK] Cycle %d: Turning OFF green LED\r\n", cycle_count);
        LED_G_OFF();
        simple_delay(1000000);  /* 简单延时 */
        
        printf("[GREEN-TASK] Cycle %d: Resuming red task and suspending self\r\n", cycle_count);
        /* 恢复红色LED任务，然后挂起自己 */
        task_resume(red_led_task);
        task_suspend(green_led_task);  /* 直接挂起绿色任务，而不是current_task */
        rtos_schedule();
        
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
        printf("[RED-TASK] Cycle %d: Turning ON red LED\r\n", cycle_count);
        LED_R_ON();
        simple_delay(1000000);  /* 简单延时 */
        
        printf("[RED-TASK] Cycle %d: Turning OFF red LED\r\n", cycle_count);
        LED_R_OFF();
        simple_delay(1000000);  /* 简单延时 */
        
        printf("[RED-TASK] Cycle %d: Resuming green task and suspending self\r\n", cycle_count);
        /* 恢复绿色LED任务，然后挂起自己 */
        task_resume(green_led_task);
        task_suspend(red_led_task);  /* 直接挂起红色任务，而不是current_task */
        rtos_schedule();
        
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

/**
  * @brief  LED初始化函数（兼容性函数）
  * @param  None
  * @retval None
  */
void LED_Init(void)
{
    LED_G_Init();
}

/**
  * @brief  UART1初始化函数
  * @param  None
  * @retval None
  */
void UART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    
    /* 使能UART1和GPIOA时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    
    /* 配置UART1引脚 - PA9(TX), PA10(RX) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 配置UART1引脚复用功能 */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    
    /* 配置UART1参数 */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    /* 使能UART1 */
    USART_Cmd(USART1, ENABLE);
    
    /* 等待UART1发送完成 */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

/**
  * @brief  printf重定向函数
  * @param  ch: 要输出的字符
  * @param  f: 文件指针（未使用）
  * @retval 输出的字符
  */
int fputc(int ch, FILE *f)
{
    /* 等待发送寄存器空 */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    
    /* 发送字符 */
    USART_SendData(USART1, (uint8_t)ch);
    
    return ch;
}

/**
  * @brief  fgetc重定向函数（nano.specs需要）
  * @param  f: 文件指针（未使用）
  * @retval 读取的字符
  */
int fgetc(FILE *f)
{
    /* 等待接收数据 */
    while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
    
    /* 读取字符 */
    return (int)USART_ReceiveData(USART1);
}

/**
  * @brief  _write重定向函数（nano.specs需要）
  * @param  file: 文件描述符
  * @param  ptr: 数据指针
  * @param  len: 数据长度
  * @retval 写入的字节数
  */
int _write(int file, char *ptr, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        /* 等待发送寄存器空 */
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        
        /* 发送字符 */
        USART_SendData(USART1, (uint8_t)ptr[i]);
    }
    return len;
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
    int i;
    for (i = 0; i < len; i++) {
        /* 等待接收数据 */
        while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
        
        /* 读取字符 */
        ptr[i] = (char)USART_ReceiveData(USART1);
    }
    return len;
}

/**
  * @brief  打印炫酷的系统启动横幅
  * @param  None
  * @retval None
  */
void print_system_banner(void)
{
    /* 清屏并设置颜色 */
    printf("\033[2J\033[H");  /* 清屏并移动光标到左上角 */
    
    /* 打印ASCII艺术标题 */
    printf("\033[1;31m");     /* 设置红色粗体 */
    printf("    ████████╗██╗ ██████╗██╗  ██╗██╗     ███████╗███████╗███████╗\r\n");
    printf("    ╚══██╔══╝██║██╔════╝██║ ██╔╝██║     ██╔════╝██╔════╝██╔════╝\r\n");
    printf("       ██║   ██║██║     █████╔╝ ██║     █████╗  ███████╗███████╗\r\n");
    printf("       ██║   ██║██║     ██╔═██╗ ██║     ██╔══╝  ╚════██║╚════██║\r\n");
    printf("       ██║   ██║╚██████╗██║  ██╗███████╗███████╗███████║███████║\r\n");
    printf("       ╚═╝   ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚══════╝\r\n");
    
    printf("\033[1;33m");     /* 设置黄色粗体 */
    printf("    ██████╗ ████████╗ ██████╗ ███████╗\r\n");
    printf("    ██╔══██╗╚══██╔══╝██╔═══██╗██╔════╝\r\n");
    printf("    ██████╔╝   ██║   ██║   ██║███████╗\r\n");
    printf("    ██╔══██╗   ██║   ██║   ██║╚════██║\r\n");
    printf("    ██║  ██║   ██║   ╚██████╔╝███████║\r\n");
    printf("    ╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚══════╝\r\n");
    
    /* 打印装饰线 */
    printf("\033[1;36m");     /* 设置青色 */
    printf("    ═══════════════════════════════════════════════════════════\r\n");
    
    /* 打印系统信息 */
    printf("\033[1;32m");     /* 设置绿色 */
    printf("    🖥️  System: STM32F407VGTx @ 168MHz\r\n");
    printf("\033[1;35m");     /* 设置紫色 */
    printf("    🏗️  Architecture: Cortex-M4 with FPU\r\n");
    printf("\033[1;34m");     /* 设置蓝色 */
    printf("    ⚙️  RTOS: Custom Tickless Real-Time Operating System\r\n");
    printf("\033[1;37m");     /* 设置白色 */
    printf("    🔄 Tasks: 2 LED Control Tasks (Cooperative Scheduling)\r\n");
    
    /* 打印装饰线 */
    printf("\033[1;36m");     /* 设置青色 */
    printf("    ═══════════════════════════════════════════════════════════\r\n");
    
    /* 打印启动信息 */
    printf("\033[1;32m");     /* 设置绿色 */
    printf("    ✅ [INFO] System initialized successfully!\r\n");
    printf("\033[1;33m");     /* 设置黄色 */
    printf("    🚀 [INFO] Starting dual LED cooperative tasks...\r\n");
    printf("\033[1;37m");     /* 设置白色 */
    printf("    💡 [INFO] Green LED: PF11 | Red LED: PF12\r\n");
    
    /* 打印底部装饰 */
    printf("\033[1;31m");     /* 设置红色 */
    printf("    ████████████████████████████████████████████████████████████\r\n");
    printf("\033[0m");        /* 重置所有属性 */
    
    /* 添加一些延时让用户看到启动信息 */
    simple_delay(2000000);
}


/* Delay_Init函数已移除 - 请使用Time_Init()替代 */

/* 
 * Tickless RTOS系统 - 不使用SysTick
 * 所有延时都通过TIM2高精度定时器实现
 * 系统采用事件驱动架构，无需周期性时钟中断
 */
