#ifndef __RTOS_HAL_UART_H__
#define __RTOS_HAL_UART_H__

#include <stdint.h>

/*
 * HAL abstraction for UART (USART + DMA) used by RTOS UART/log subsystems.
 * Concrete implementations are provided under project HAL directory.
 */

/* Initialize UART (GPIO mux, USART, DMA, IRQs) with parameters from rtos_config.h */
void rtos_hal_uart_init(void);

/* Provide RX DMA ring buffer to HAL (address and size). */
void rtos_hal_uart_set_rx_buffer(volatile uint8_t* rx_buf, uint16_t buf_size);

/* Return DMA RX circular buffer write index (computed from NDTR). */
uint16_t rtos_hal_uart_get_rx_write_index(void);

/* Return 1 if IDLE interrupt was set and clear it; else 0. */
int rtos_hal_uart_irq_is_idle_and_clear(void);

/* Start a TX DMA transfer for [ptr, len]; implementation must handle busy state. */
void rtos_hal_uart_tx_start(const uint8_t* ptr, uint16_t len);

/* Return 1 if TX DMA transfer complete flag is set and clear it; else 0. */
int rtos_hal_uart_dma_tx_irq_is_tc_and_clear(void);

/* Clear RX DMA IRQ flags (errors/half/full transfer as needed). */
void rtos_hal_uart_dma_rx_irq_clear_all(void);

#endif /* __RTOS_HAL_UART_H__ */


