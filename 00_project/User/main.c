/* Main函数
  -----------------------------------
  RTOS多任务演示程序
  基于STM32F407标准库和自定义RTOS
*/
#include "main.h"
#include "../../02_rtos/core.h"

/* 私有变量定义 */
static __IO uint32_t TimingDelay;

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
    
    /* 延时初始化 */
    Delay_Init();
    
    /* RTOS初始化 */
    rtos_init();
    
    /* 创建多个任务 */
    task_create(task_led_blink, NULL, 1);      /* 高优先级LED闪烁任务 */
    task_create(task_serial_print, NULL, 2);   /* 中等优先级串口打印任务 */
    task_create(task_button_check, NULL, 3);   /* 低优先级按钮检测任务 */
    
    /* 启动RTOS调度器 */
    rtos_start();
    
    /* 程序不会执行到这里，因为RTOS会接管控制权 */
    while(1)
    {
        /* 空闲时执行 */
    }
}

/**
  * @brief  LED闪烁任务
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
    }
}

/**
  * @brief  串口打印任务
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
        
        Delay_ms(1000);   /* 每秒执行一次 */
    }
}

/**
  * @brief  按钮检测任务
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
        
        Delay_ms(100);    /* 每100ms检测一次 */
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

/**
  * @brief  延时初始化函数
  * @param  None
  * @retval None
  */
void Delay_Init(void)
{
    /* 配置SysTick为1ms中断 */
    if(SysTick_Config(SystemCoreClock / 1000))
    {
        /* 如果配置失败，进入死循环 */
        while(1);
    }
}

/**
  * @brief  毫秒延时函数
  * @param  ms: 延时时间，单位毫秒
  * @retval None
  */
void Delay_ms(uint32_t ms)
{
    TimingDelay = ms;
    while(TimingDelay != 0);
}

/**
  * @brief  SysTick中断服务函数
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    if(TimingDelay != 0x00)
    {
        TimingDelay--;
    }
}

/**
  * @brief  延时递减函数（兼容性）
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
    if(TimingDelay != 0x00)
    {
        TimingDelay--;
    }
}
