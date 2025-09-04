/**
  ******************************************************************************
  * @file    main.c
  * @author  RTOS Team
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   STM32F407 RTOS多任务演示程序
  *          基于STM32F407标准库和自定义RTOS系统
  *          集成TIM2高精度延时功能（毫秒/微秒/纳秒级）
  ******************************************************************************
  * @attention
  *
  * 本程序演示了自定义RTOS系统的多任务调度功能：
  * 1. LED闪烁任务 - 测试高精度延时功能
  * 2. 串口打印任务 - 测试不同精度延时
  * 3. 按钮检测任务 - 测试高频率延时
  * 
  * 硬件平台：星火一号开发板 (STM32F407VGTx)
  * LED引脚：GPIOF Pin 11
  *
  ******************************************************************************
  */
#include "main.h"
#include "../../02_rtos/core.h"
#include "../../02_rtos/time.h"

/* 私有变量定义 - 已移除废弃的TimingDelay变量 */

/* 示例任务函数声明 */
void task_led_blink(void* arg);
void task_serial_print(void* arg);
void task_button_check(void* arg);

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
    LED_Init();
    
    /* 高精度延时系统初始化 */
    Time_Init();
    
    /* 配置中断优先级 - Tickless RTOS系统 */
    NVIC_SetPriority(SVCall_IRQn, 0);      /* SVC中断优先级设为最高 */
    NVIC_SetPriority(PendSV_IRQn, 15);     /* PendSV中断优先级设为最低 */
    /* 注意：不使用SysTick中断，系统采用事件驱动架构 */
    
    /* RTOS初始化 */
    rtos_init();
    
    /* 创建多个任务 */
    task_create(task_led_blink, NULL, 1);      /* 高优先级LED闪烁任务 */
    task_create(task_serial_print, NULL, 2);   /* 中等优先级串口打印任务 */
    task_create(task_button_check, NULL, 3);   /* 低优先级按钮检测任务 */
    
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
  * @brief  LED闪烁任务 - 测试高精度延时
  * @param  arg: 任务参数（未使用）
  * @retval None
  */
void task_led_blink(void* arg)
{
    while(1)
    {
        LED_ON();
        Delay_ms(200);    /* LED亮200ms */
        
        LED_OFF();
        Delay_ms(800);    /* LED灭800ms */
        
        /* 测试微秒级延时 */
        LED_ON();
        Delay_us(1000);   /* LED亮1ms */
        
        LED_OFF();
        Delay_us(5000);   /* LED灭5ms */
        
        /* 测试纳秒级延时 */
        LED_ON();
        Delay_ns(100000); /* LED亮100us */
        
        LED_OFF();
        Delay_ns(500000); /* LED灭500us */
    }
}

/**
  * @brief  串口打印任务 - 测试不同精度延时
  * @param  arg: 任务参数（未使用）
  * @retval None
  */
void task_serial_print(void* arg)
{
    uint32_t counter = 0;
    while(1)
    {
        /* 这里可以添加串口打印代码 */
        /* printf("Task2 - Counter: %lu\r\n", counter++); */
        
        /* 测试不同精度的延时 */
        Delay_ms(500);    /* 500ms延时 */
        Delay_us(100000); /* 100ms延时 */
        Delay_ns(1000000); /* 1ms延时 */
        
        counter++;
    }
}

/**
  * @brief  按钮检测任务 - 测试高频率延时
  * @param  arg: 任务参数（未使用）
  * @retval None
  */
void task_button_check(void* arg)
{
    while(1)
    {
        /* 这里可以添加按钮检测代码 */
        /* if (GPIO_ReadInputDataBit(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN)) */
        /* { */
        /*     LED_TOGGLE(); */
        /* } */
        
        /* 测试高频率延时 */
        Delay_us(10000);  /* 10ms延时 */
        Delay_ns(100000); /* 100us延时 */
    }
}

/**
  * @brief  LED初始化函数
  * @param  None
  * @retval None
  */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能GPIOF时钟 */
    RCC_AHB1PeriphClockCmd(LED_GPIO_CLK, ENABLE);
    
    /* 配置LED引脚为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);
    
    /* 初始状态：LED关闭 */
    LED_OFF();
}

/* Delay_Init函数已移除 - 请使用Time_Init()替代 */

/* 
 * Tickless RTOS系统 - 不使用SysTick
 * 所有延时都通过TIM2高精度定时器实现
 * 系统采用事件驱动架构，无需周期性时钟中断
 */
