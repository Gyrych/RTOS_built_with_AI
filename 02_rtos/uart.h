/**
  ******************************************************************************
  * @file    uart.h
  * @author  RTOS Team
  * @brief   RTOS UART (USART1) interface based on DMA (RX circular + IDLE, TX DMA)
  *          This module is part of RTOS core to provide low-CPU-overhead logging/IO.
  ******************************************************************************
  */

#ifndef __RTOS_UART_H__
#define __RTOS_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32f4xx.h"

/* ================== Configuration ================== */

/* Enable RTOS UART DMA driver (set to 0 to fallback to legacy polling UART) */
#ifndef RTOS_UART_ENABLE
#define RTOS_UART_ENABLE                    1
#endif

/* USART1 default parameters */
#ifndef RTOS_UART_BAUDRATE
#define RTOS_UART_BAUDRATE                 115200u
#endif

/* RX ring buffer size (bytes). Must be > 0 and fit in uint16_t */
#ifndef RTOS_UART_RX_BUF_SIZE
#define RTOS_UART_RX_BUF_SIZE              1024u
#endif

/* Convert '\n' to "\r\n" automatically on transmit */
#ifndef RTOS_UART_AUTO_CRLF
#define RTOS_UART_AUTO_CRLF                0
#endif

/* Interrupt priority for USART1 and DMA streams (lower priority than SVC, higher than PendSV) */
#ifndef RTOS_UART_IRQn_PRIORITY
#define RTOS_UART_IRQn_PRIORITY            6
#endif

/* Optional: allow weak mapping customization via macros */
#ifndef RTOS_UART_RX_DMA
#define RTOS_UART_RX_DMA                   DMA2
#endif
#ifndef RTOS_UART_RX_DMA_STREAM
#define RTOS_UART_RX_DMA_STREAM            DMA2_Stream5
#endif
#ifndef RTOS_UART_RX_DMA_CHANNEL
#define RTOS_UART_RX_DMA_CHANNEL           DMA_Channel_4
#endif

#ifndef RTOS_UART_TX_DMA
#define RTOS_UART_TX_DMA                   DMA2
#endif
#ifndef RTOS_UART_TX_DMA_STREAM
#define RTOS_UART_TX_DMA_STREAM            DMA2_Stream7
#endif
#ifndef RTOS_UART_TX_DMA_CHANNEL
#define RTOS_UART_TX_DMA_CHANNEL           DMA_Channel_4
#endif

/* ================== Public API ================== */

typedef void (*rtos_uart_rx_cb_t)(const uint8_t* data, uint16_t length);

/* Initialize USART1 with DMA RX circular + IDLE and TX DMA */
void rtos_uart_init(void);

/* Set RX callback (called from ISR context on IDLE-delimited segments) */
void rtos_uart_set_rx_callback(rtos_uart_rx_cb_t callback);

/* Non-blocking write: returns bytes queued (==len) or negative on busy/error */
int rtos_uart_write(const uint8_t* data, uint16_t length);

/* Blocking write using DMA; minimizes CPU by yielding or waiting for IRQ */
int rtos_uart_write_blocking(const uint8_t* data, uint16_t length);

/* ================== ISR forwarding (used by stm32f4xx_it.c) ================== */
void rtos_uart_usart1_irq_handler(void);
void rtos_uart_dma_tx_irq_handler(void);
void rtos_uart_dma_rx_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_UART_H__ */


