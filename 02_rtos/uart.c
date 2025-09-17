/**
  ******************************************************************************
  * @file    uart.c
  * @author  RTOS Team
  * @brief   RTOS UART (USART1) DMA driver: RX circular + IDLE, TX DMA
  ******************************************************************************
  */

#define RTOS_CMSIS_FALLBACK 1
#include "uart.h"
#include "core.h"

#if RTOS_UART_ENABLE

/* Internal TX buffer for blocking/non-blocking copies */
#ifndef RTOS_UART_TX_BUF_SIZE
#define RTOS_UART_TX_BUF_SIZE           512u
#endif

/* ================== Private variables ================== */

static volatile uint8_t  s_rx_ring[RTOS_UART_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0; /* last processed index */
static rtos_uart_rx_cb_t s_rx_cb = 0;

static volatile uint8_t  s_tx_busy = 0;
static uint8_t           s_tx_buf[RTOS_UART_TX_BUF_SIZE];

/* TX queue and active length for non-blocking DMA continuation */
static volatile uint16_t s_tx_len_active = 0;

#ifndef RTOS_UART_TXQ_SIZE
#define RTOS_UART_TXQ_SIZE                2048u
#endif
static volatile uint32_t s_txq_head = 0; /* write index */
static volatile uint32_t s_txq_tail = 0; /* read index */
static uint8_t           s_txq[RTOS_UART_TXQ_SIZE];

static inline uint32_t txq_free_space(void)
{
    uint32_t h = s_txq_head, t = s_txq_tail;
    if (h >= t) return (RTOS_UART_TXQ_SIZE - (h - t) - 1);
    return (t - h - 1);
}

static inline uint32_t txq_data_size(void)
{
    uint32_t h = s_txq_head, t = s_txq_tail;
    if (h >= t) return (h - t);
    return (RTOS_UART_TXQ_SIZE - (t - h));
}

static inline void txq_push_byte(uint8_t b)
{
    s_txq[s_txq_head] = b;
    s_txq_head = (s_txq_head + 1) % RTOS_UART_TXQ_SIZE;
}

static inline uint32_t txq_peek_linear(uint8_t** ptr)
{
    *ptr = (uint8_t*)&s_txq[s_txq_tail];
    if (s_txq_head >= s_txq_tail) return (s_txq_head - s_txq_tail);
    return (RTOS_UART_TXQ_SIZE - s_txq_tail);
}

static inline void txq_consume(uint32_t n)
{
    s_txq_tail = (s_txq_tail + n) % RTOS_UART_TXQ_SIZE;
}

/* ================== Helpers ================== */

/* No device-specific definitions here; all hardware interactions are in HAL. */

/* ================== Public API ================== */

void rtos_uart_init(void)
{
    /* Provide RX DMA ring buffer first, then init HAL to program DMA */
    rtos_hal_uart_set_rx_buffer(s_rx_ring, (uint16_t)RTOS_UART_RX_BUF_SIZE);
    rtos_hal_uart_init();
}

void rtos_uart_set_rx_callback(rtos_uart_rx_cb_t callback)
{
    s_rx_cb = callback;
}

/* 删除未使用的单次 DMA 启动函数：当前 TX 由队列 + DMA 中断驱动 */

static void uart_kick_tx_if_idle(void)
{
    if (s_tx_busy) return;
    uint8_t* ptr;
    uint32_t avail = txq_peek_linear(&ptr);
    if (avail == 0) return;
    uint16_t len = (uint16_t)((avail > 0xFFFFu) ? 0xFFFFu : avail);
    s_tx_busy = 1;
    s_tx_len_active = len;
    rtos_hal_uart_tx_start(ptr, len);
}

int rtos_uart_write(const uint8_t* data, uint16_t length)
{
    if (!data || length == 0) return 0;
    uint32_t written = 0;
    rtos_enter_critical();
    while (written < length && txq_free_space() > 0) {
#if RTOS_UART_AUTO_CRLF
        uint8_t ch = data[written++];
        if (ch == '\n') {
            if (txq_free_space() > 0) txq_push_byte('\r');
            txq_push_byte('\n');
        } else {
            txq_push_byte(ch);
        }
#else
        txq_push_byte(data[written++]);
#endif
    }
    uart_kick_tx_if_idle();
    rtos_exit_critical();
    return (int)written;
}

int rtos_uart_write_blocking(const uint8_t* data, uint16_t length)
{
    if (!data || length == 0) return 0;
    int queued = rtos_uart_write(data, length);
    /* Wait for queue drain; provide ISR-free fallback by polling HAL TC flag */
    for (;;) {
        if (!(txq_data_size() || s_tx_busy)) break;
        if (rtos_hal_uart_dma_tx_irq_is_tc_and_clear()) {
            if (s_tx_len_active) {
                txq_consume(s_tx_len_active);
                s_tx_len_active = 0;
            }
            s_tx_busy = 0;
            rtos_enter_critical();
            uart_kick_tx_if_idle();
            rtos_exit_critical();
        }
    }
    return queued;
}

/* ================== ISRs ================== */

void rtos_uart_irq_handler(void)
{
    /* IDLE detection (HAL clears the event if set) */
    if (rtos_hal_uart_irq_is_idle_and_clear()) {

        uint16_t write_index = rtos_hal_uart_get_rx_write_index();
        uint16_t data_len = 0;

        if (write_index >= s_rx_head) {
            data_len = (uint16_t)(write_index - s_rx_head);
            if (data_len && s_rx_cb) {
                s_rx_cb((const uint8_t*)&s_rx_ring[s_rx_head], data_len);
            }
        } else {
            /* Wrapped region: head..end, then 0..write_index */
            data_len = (uint16_t)(RTOS_UART_RX_BUF_SIZE - s_rx_head);
            if (data_len && s_rx_cb) {
                s_rx_cb((const uint8_t*)&s_rx_ring[s_rx_head], data_len);
            }
            if (write_index && s_rx_cb) {
                s_rx_cb((const uint8_t*)&s_rx_ring[0], write_index);
            }
        }

        s_rx_head = write_index;
    }
}

void rtos_uart_dma_tx_irq_handler(void)
{
    if (rtos_hal_uart_dma_tx_irq_is_tc_and_clear()) {
        /* consume the sent bytes */
        if (s_tx_len_active) {
            txq_consume(s_tx_len_active);
            s_tx_len_active = 0;
        }
        s_tx_busy = 0;
    }
    /* kick next segment if any */
    rtos_enter_critical();
    uart_kick_tx_if_idle();
    rtos_exit_critical();
}

void rtos_uart_dma_rx_irq_handler(void)
{
    /* Clear DMA RX flags if any (optional depending on implementation) */
    rtos_hal_uart_dma_rx_irq_clear_all();
}

#endif /* RTOS_UART_ENABLE */


