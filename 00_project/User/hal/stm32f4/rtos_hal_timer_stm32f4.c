#include "rtos_config.h"
#include "../../../../02_rtos/hal/rtos_hal_timer.h"
#include "../../config/stm32f4/core/stm32f4xx.h"
#include "../../config/stm32f4/config/stm32f4xx_conf.h"

/* Implementation: use TIM2 as free-running 32-bit timer with CC1 compare */

static inline TIM_TypeDef* timer_inst(void) { return TIM2; }
static inline IRQn_Type timer_irqn(void) { return TIM2_IRQn; }

void rtos_hal_timer_init(void)
{
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef oc;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    tb.TIM_Period = 0xFFFFFFFFu;
    tb.TIM_Prescaler = 0; /* feed with APB clock directly */
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(timer_inst(), &tb);

    oc.TIM_OCMode = TIM_OCMode_Timing;
    oc.TIM_OutputState = TIM_OutputState_Disable;
    oc.TIM_Pulse = 0;
    oc.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(timer_inst(), &oc);
    TIM_OC1PreloadConfig(timer_inst(), TIM_OCPreload_Disable);

    TIM_ITConfig(timer_inst(), TIM_IT_CC1, ENABLE);

    NVIC_SetPriority(timer_irqn(), RTOS_TIMER_IRQ_PRIORITY);
    NVIC_EnableIRQ(timer_irqn());

    TIM_Cmd(timer_inst(), ENABLE);
}

void rtos_hal_timer_deinit(void)
{
    TIM_Cmd(timer_inst(), DISABLE);
    TIM_ITConfig(timer_inst(), TIM_IT_CC1, DISABLE);
    NVIC_DisableIRQ(timer_irqn());
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, DISABLE);
}

uint32_t rtos_hal_timer_get_counter(void)
{
    return TIM_GetCounter(timer_inst());
}

void rtos_hal_timer_set_compare(uint32_t target_count)
{
    TIM_SetCompare1(timer_inst(), target_count);
}

int rtos_hal_timer_check_and_clear_compare_irq(void)
{
    if (TIM_GetITStatus(timer_inst(), TIM_IT_CC1) != RESET) {
        TIM_ClearITPendingBit(timer_inst(), TIM_IT_CC1);
        return 1;
    }
    return 0;
}

void rtos_hal_timer_irq_disable(void)
{
    NVIC_DisableIRQ(timer_irqn());
}

void rtos_hal_timer_irq_enable(void)
{
    NVIC_EnableIRQ(timer_irqn());
}

uint32_t rtos_hal_timer_get_freq_hz(void)
{
    return (uint32_t)RTOS_TIMER_FREQ_HZ;
}


