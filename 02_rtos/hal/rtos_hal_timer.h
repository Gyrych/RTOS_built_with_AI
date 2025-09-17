#ifndef __RTOS_HAL_TIMER_H__
#define __RTOS_HAL_TIMER_H__

#include <stdint.h>

/*
 * HAL abstraction for high-precision one-shot/compare timer used by
 * the RTOS time subsystem (tickless delays and scheduling wakeups).
 *
 * Implementation note:
 * - The concrete implementation is provided under project HAL directory,
 *   e.g. 00_project/User/hal/<device_family>/rtos_hal_timer_*.c
 */

/* Initialize and start the free-running timer/counter and compare interrupt */
void rtos_hal_timer_init(void);

/* De-initialize timer module and associated interrupts */
void rtos_hal_timer_deinit(void);

/* Get current free-running counter value (monotonic, wrap-around allowed) */
uint32_t rtos_hal_timer_get_counter(void);

/* Program the next compare match (atomic if possible). */
void rtos_hal_timer_set_compare(uint32_t target_count);

/* If compare interrupt is pending, clear it and return 1; else return 0. */
int rtos_hal_timer_check_and_clear_compare_irq(void);

/* Disable/enable the timer compare interrupt (used to protect queue updates). */
void rtos_hal_timer_irq_disable(void);
void rtos_hal_timer_irq_enable(void);

/* Return the timer input clock in Hz (used for time conversions). */
uint32_t rtos_hal_timer_get_freq_hz(void);

#endif /* __RTOS_HAL_TIMER_H__ */


