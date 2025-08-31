/* Main函数
  -----------------------------------
  星火一号开发版LED闪烁程序
  基于STM32F407标准库
*/
#include "main.h"

/* 私有变量定义 */
static __IO uint32_t TimingDelay;

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
    
    /* 主循环 */
    while(1)
    {
        /* LED闪烁 */
        LED_ON();
        Delay_ms(500);    /* 延时500ms */
        
        LED_OFF();
        Delay_ms(500);    /* 延时500ms */
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
