/**
  * 文件功能：
  * - 提供基于 UART DMA 的异步日志能力：线程环境格式化并入队，低优先级后台任务持续冲刷；ISR 环境提供二进制快速入队。
  *
  * 调用方法：
  * - 在系统启动后调用 `rtos_log_init()` 初始化环形缓冲；创建一个低优先级任务运行 `rtos_log_task()` 即可自动冲刷。
  * - 线程环境调用 `rtos_log_printf()`；中断环境使用 `rtos_log_enqueue_from_isr()` 追加原始数据。
  *
  * 注意事项：
  * - 避免在 ISR 里做格式化（如 printf/vsnprintf），仅做最小化入队；
  * - 若日志量大，建议适当增大 `RTOS_LOG_BUF_SIZE`，并确保日志任务优先级低于业务任务。
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


