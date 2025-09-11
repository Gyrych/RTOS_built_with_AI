/**
  * 文件功能：
  * - 基于 UART1 DMA 的异步日志实现：线程态格式化入队，后台任务批量发送；ISR 态提供快速二进制入队。
  * 调用方法：见 `log.h` 头部注释说明。
  */

#include "log.h"
#include "uart.h"
#include "core.h"
#include <stdarg.h>
#include <stdio.h>

#if RTOS_LOG_ENABLE

typedef struct {
    volatile uint32_t head; /* write position */
    volatile uint32_t tail; /* read position */
    uint8_t buffer[RTOS_LOG_BUF_SIZE];
} rtos_log_ring_t;

static rtos_log_ring_t g_log_ring;

static inline uint32_t ring_free_space(void)
{
    uint32_t head = g_log_ring.head;
    uint32_t tail = g_log_ring.tail;
    if (head >= tail) return (RTOS_LOG_BUF_SIZE - (head - tail) - 1);
    return (tail - head - 1);
}

static inline uint32_t ring_data_size(void)
{
    uint32_t head = g_log_ring.head;
    uint32_t tail = g_log_ring.tail;
    if (head >= tail) return (head - tail);
    return (RTOS_LOG_BUF_SIZE - (tail - head));
}

static uint32_t ring_write(const uint8_t* data, uint32_t len)
{
    uint32_t written = 0;
    while (written < len && ring_free_space() > 0) {
        g_log_ring.buffer[g_log_ring.head] = data[written++];
        g_log_ring.head = (g_log_ring.head + 1) % RTOS_LOG_BUF_SIZE;
    }
    return written;
}

static uint32_t ring_peek_linear(uint8_t** ptr)
{
    uint32_t tail = g_log_ring.tail;
    uint32_t head = g_log_ring.head;
    *ptr = (uint8_t*)&g_log_ring.buffer[tail];
    if (head >= tail) return (head - tail);
    return (RTOS_LOG_BUF_SIZE - tail);
}

static void ring_consume(uint32_t len)
{
    g_log_ring.tail = (g_log_ring.tail + len) % RTOS_LOG_BUF_SIZE;
}

void rtos_log_init(void)
{
    g_log_ring.head = g_log_ring.tail = 0;
}

/* Called in a dedicated low-priority task */
void rtos_log_task(void* arg)
{
    (void)arg;
    for (;;) {
        /* 尽可能发送可用数据（分段以适配 DMA） */
        uint8_t* ptr;
        uint32_t avail = ring_peek_linear(&ptr);
        if (avail) {
            int sent = rtos_uart_write((const uint8_t*)ptr, (uint16_t)((avail > 0xFFFF) ? 0xFFFF : avail));
            if (sent > 0) {
                ring_consume((uint32_t)sent);
                continue;
            }
        }
        /* 若无数据或 UART 忙，让出执行权（协作式调度） */
        rtos_schedule();
    }
}

int rtos_log_printf(const char* fmt, ...)
{
    char tmp[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n <= 0) return 0;
    if (n > (int)sizeof(tmp)) n = (int)sizeof(tmp);
    rtos_enter_critical();
    uint32_t w = ring_write((const uint8_t*)tmp, (uint32_t)n);
    rtos_exit_critical();
    return (int)w;
}

int rtos_log_enqueue_from_isr(const uint8_t* data, uint16_t len)
{
    if (!data || len == 0) return 0;
    /* ISR: 禁止使用繁重原语，仅尝试写入可用空间 */
    uint32_t written = 0;
    while (written < len && ring_free_space() > 0) {
        g_log_ring.buffer[g_log_ring.head] = data[written++];
        g_log_ring.head = (g_log_ring.head + 1) % RTOS_LOG_BUF_SIZE;
    }
    return (int)written;
}

#endif /* RTOS_LOG_ENABLE */


