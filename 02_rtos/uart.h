/**
  * 文件功能：
  * - 提供 USART1 的 DMA 串口驱动（RX 环形缓冲 + IDLE 分包，TX DMA 队列），显著降低 CPU 占用。
  * - 作为 RTOS 内核的一部分，服务于 `printf` 重定向、异步日志与交互 IO。
  *
  * 调用方法：
  * - 系统初始化阶段调用 `rtos_uart_init()` 完成 GPIO、USART1、DMA 与中断配置。
  * - 发送：使用 `rtos_uart_write()`（非阻塞，入队）或 `rtos_uart_write_blocking()`（阻塞直到队列清空）。
  * - 接收：可注册 `rtos_uart_set_rx_callback()` 获得 IDLE 分段数据（在中断上下文回调）。
  * - 在 `stm32f4xx_it.c` 中将相关中断转发到 `rtos_uart_*_irq_handler()`。
  *
  * 依赖与优先级：
  * - 使用 DMA2（Stream5 RX / Stream7 TX）与 USART1，建议中断优先级低于 SVC(0)、高于 PendSV(15)。
  *
  * 注意事项：
  * - RX 使用环形 DMA + IDLE 分包；如需基于长度帧，可在回调中自行拼包。
  * - TX 采用环形队列 + DMA 续传；请避免在 ISR 中进行格式化操作。
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


