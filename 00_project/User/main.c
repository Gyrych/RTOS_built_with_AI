/**
  ******************************************************************************
  * @file    main.c
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   STM32F407 RTOS多任务演示程序
  *          基于STM32F407标准库和自定义RTOS系统
  *          集成TIM2高精度延时功能和UART1串口输出
  ******************************************************************************
  * @attention
  *
  * 本程序演示了自定义RTOS系统的多任务调度功能：
  * 1. 绿色LED闪烁任务 - PF11引脚，周期100ms
  * 2. 红色LED闪烁任务 - PF12引脚，周期500ms
  * 3. 串口打印任务 - UART1输出"Hellow rtos!"，周期1000ms
  * 
  * 硬件平台：星火一号开发板 (STM32F407VGTx)
  * LED引脚：绿色LED - PF11，红色LED - PF12
  * 串口：UART1 - PA9(TX), PA10(RX)，波特率115200
  *
  ******************************************************************************
  */
#include "main.h"
#include "../../02_rtos/core.h"
#include "../../02_rtos/time.h"
#include <stdio.h>

/* 私有变量定义 - 已移除废弃的TimingDelay变量 */

/* 示例任务函数声明 */
void task_led_g_blink(void* arg);
void task_led_r_blink(void* arg);
void task_serial_print(void* arg);

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
    
    /* UART1初始化 */
    UART1_Init();
    
    /* 高精度延时系统初始化 */
    Time_Init();
    
    /* 配置中断优先级 - Tickless RTOS系统 */
    NVIC_SetPriority(SVCall_IRQn, 0);      /* SVC中断优先级设为最高 */
    NVIC_SetPriority(PendSV_IRQn, 15);     /* PendSV中断优先级设为最低 */
    /* 注意：不使用SysTick中断，系统采用事件驱动架构 */
    
    /* RTOS初始化 */
    rtos_init();
    
    /* 创建多个任务 */
    task_create(task_led_g_blink, NULL, 1);    /* 高优先级绿色LED闪烁任务 */
    task_create(task_led_r_blink, NULL, 2);    /* 中等优先级红色LED闪烁任务 */
    task_create(task_serial_print, NULL, 3);   /* 低优先级串口打印任务 */
    
    /* 启动RTOS调度器 */
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
  * @brief  绿色LED闪烁任务 - 周期100ms
  * @param  arg: 任务参数（未使用）
  * @retval None
  */
void task_led_g_blink(void* arg)
{
    while(1)
    {
        LED_G_ON();
        Delay_ms(50);     /* 绿色LED亮50ms */
        
        LED_G_OFF();
        Delay_ms(50);     /* 绿色LED灭50ms，总周期100ms */
    }
}

/**
  * @brief  串口打印任务 - 定时1000ms输出"Hellow rtos!"
  * @param  arg: 任务参数（未使用）
  * @retval None
  */
void task_serial_print(void* arg)
{
    uint32_t counter = 0;
    while(1)
    {
        /* 使用printf输出字符串 */
        printf("Hellow rtos! Counter: %lu\r\n", counter++);
        
        /* 延时1000ms */
        Delay_ms(1000);
    }
}

/**
  * @brief  红色LED闪烁任务 - 周期500ms
  * @param  arg: 任务参数（未使用）
  * @retval None
  */
void task_led_r_blink(void* arg)
{
    while(1)
    {
        LED_R_ON();
        Delay_ms(250);    /* 红色LED亮250ms */
        
        LED_R_OFF();
        Delay_ms(250);    /* 红色LED灭250ms，总周期500ms */
    }
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

/* Delay_Init函数已移除 - 请使用Time_Init()替代 */

/* 
 * Tickless RTOS系统 - 不使用SysTick
 * 所有延时都通过TIM2高精度定时器实现
 * 系统采用事件驱动架构，无需周期性时钟中断
 */
