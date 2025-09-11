/**
  ******************************************************************************
  * @file    log.h
  * @brief   Asynchronous logging over UART DMA: ISR-safe enqueue + background flush
  ******************************************************************************
  */

#ifndef __RTOS_LOG_H__
#define __RTOS_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef RTOS_LOG_ENABLE
#define RTOS_LOG_ENABLE             1
#endif

#ifndef RTOS_LOG_BUF_SIZE
#define RTOS_LOG_BUF_SIZE           2048u
#endif

void rtos_log_init(void);
void rtos_log_task(void* arg);

/* Thread-context logging: format then enqueue */
int rtos_log_printf(const char* fmt, ...);

/* ISR-safe binary enqueue (no format) */
int rtos_log_enqueue_from_isr(const uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_LOG_H__ */


