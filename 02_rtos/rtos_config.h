#ifndef __RTOS_CONFIG_H__
#define __RTOS_CONFIG_H__

/*
 * Centralized RTOS configuration for cross-device compatibility within
 * the same CPU architecture family (e.g., STM32/GD32 Cortex-M series).
 *
 * Purpose:
 * - Provide defaults for NVIC priority bits and peripheral timings
 * - Keep OS core independent from specific MCU headers
 * - Allow per-project overrides via compiler defines if needed
 */

#include <stdint.h>

/* ---------------- NVIC / Interrupt configuration ---------------- */
/* Fallback for __NVIC_PRIO_BITS if device header doesn't provide it */
#ifndef RTOS_NVIC_PRIO_BITS
#define RTOS_NVIC_PRIO_BITS                 4
#endif

/* OS-decided default interrupt priorities (lower is higher priority) */
#ifndef RTOS_IRQ_PRIORITY_SVC
#define RTOS_IRQ_PRIORITY_SVC               0
#endif

#ifndef RTOS_IRQ_PRIORITY_PENDSV
#define RTOS_IRQ_PRIORITY_PENDSV            15
#endif

/* ---------------- High precision timer (used by time.c) ---------------- */
/* Timer clock frequency in Hz (maps to previous TIM2_CLOCK_FREQ) */
#ifndef RTOS_TIMER_FREQ_HZ
#define RTOS_TIMER_FREQ_HZ                  84000000UL
#endif

/* Timer compare IRQ priority (must be above PendSV, below/!= SVC) */
#ifndef RTOS_TIMER_IRQ_PRIORITY
#define RTOS_TIMER_IRQ_PRIORITY             3
#endif

/* ---------------- UART defaults (used by uart.c/log.c) ---------------- */
#ifndef RTOS_UART_BAUDRATE
#define RTOS_UART_BAUDRATE                  115200u
#endif

#ifndef RTOS_UART_IRQn_PRIORITY
#define RTOS_UART_IRQn_PRIORITY             6
#endif

#ifndef RTOS_UART_AUTO_CRLF
#define RTOS_UART_AUTO_CRLF                 0
#endif

#endif /* __RTOS_CONFIG_H__ */


