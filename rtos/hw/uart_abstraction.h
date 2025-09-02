/**
 * @file uart_abstraction.h
 * @brief RTOS UART抽象模块 - 面向对象的串口通信管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_UART_ABSTRACTION_H__
#define __RTOS_UART_ABSTRACTION_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UART端口定义 */
typedef enum {
    RTOS_UART_PORT_1 = 0,
    RTOS_UART_PORT_2,
    RTOS_UART_PORT_3,
    RTOS_UART_PORT_4,
    RTOS_UART_PORT_5,
    RTOS_UART_PORT_6,
    RTOS_UART_PORT_MAX
} rtos_uart_port_t;

/* UART数据位定义 */
typedef enum {
    RTOS_UART_DATABITS_8 = 0,
    RTOS_UART_DATABITS_9,
    RTOS_UART_DATABITS_MAX
} rtos_uart_databits_t;

/* UART停止位定义 */
typedef enum {
    RTOS_UART_STOPBITS_1 = 0,
    RTOS_UART_STOPBITS_2,
    RTOS_UART_STOPBITS_MAX
} rtos_uart_stopbits_t;

/* UART校验位定义 */
typedef enum {
    RTOS_UART_PARITY_NONE = 0,
    RTOS_UART_PARITY_EVEN,
    RTOS_UART_PARITY_ODD,
    RTOS_UART_PARITY_MAX
} rtos_uart_parity_t;

/* UART流控制定义 */
typedef enum {
    RTOS_UART_FLOWCTRL_NONE = 0,
    RTOS_UART_FLOWCTRL_RTS,
    RTOS_UART_FLOWCTRL_CTS,
    RTOS_UART_FLOWCTRL_RTS_CTS,
    RTOS_UART_FLOWCTRL_MAX
} rtos_uart_flowctrl_t;

/* UART传输模式定义 */
typedef enum {
    RTOS_UART_MODE_POLLING = 0,     /**< 轮询模式 */
    RTOS_UART_MODE_INTERRUPT,       /**< 中断模式 */
    RTOS_UART_MODE_DMA,             /**< DMA模式 */
    RTOS_UART_MODE_MAX
} rtos_uart_mode_t;

/* UART状态定义 */
typedef enum {
    RTOS_UART_STATE_RESET = 0,      /**< 复位状态 */
    RTOS_UART_STATE_READY,          /**< 就绪状态 */
    RTOS_UART_STATE_BUSY_TX,        /**< 发送忙 */
    RTOS_UART_STATE_BUSY_RX,        /**< 接收忙 */
    RTOS_UART_STATE_BUSY_TX_RX,     /**< 收发忙 */
    RTOS_UART_STATE_ERROR,          /**< 错误状态 */
    RTOS_UART_STATE_MAX
} rtos_uart_state_t;

/* UART错误类型定义 */
typedef enum {
    RTOS_UART_ERROR_NONE = 0,
    RTOS_UART_ERROR_PARITY = (1 << 0),
    RTOS_UART_ERROR_NOISE = (1 << 1),
    RTOS_UART_ERROR_FRAME = (1 << 2),
    RTOS_UART_ERROR_OVERRUN = (1 << 3),
    RTOS_UART_ERROR_DMA = (1 << 4),
    RTOS_UART_ERROR_TIMEOUT = (1 << 5)
} rtos_uart_error_t;

/* UART配置结构 */
typedef struct {
    uint32_t baudrate;              /**< 波特率 */
    rtos_uart_databits_t databits;  /**< 数据位 */
    rtos_uart_stopbits_t stopbits;  /**< 停止位 */
    rtos_uart_parity_t parity;      /**< 校验位 */
    rtos_uart_flowctrl_t flowctrl;  /**< 流控制 */
    rtos_uart_mode_t mode;          /**< 传输模式 */
    uint32_t timeout_ms;            /**< 超时时间 */
    bool auto_baudrate;             /**< 自动波特率检测 */
} rtos_uart_config_t;

/* UART缓冲区配置 */
typedef struct {
    uint8_t *tx_buffer;             /**< 发送缓冲区 */
    uint32_t tx_buffer_size;        /**< 发送缓冲区大小 */
    uint8_t *rx_buffer;             /**< 接收缓冲区 */
    uint32_t rx_buffer_size;        /**< 接收缓冲区大小 */
    bool circular_mode;             /**< 循环模式 */
} rtos_uart_buffer_config_t;

/* UART事件类型 */
typedef enum {
    RTOS_UART_EVENT_TX_COMPLETE = 0, /**< 发送完成 */
    RTOS_UART_EVENT_RX_COMPLETE,     /**< 接收完成 */
    RTOS_UART_EVENT_ERROR,           /**< 错误事件 */
    RTOS_UART_EVENT_IDLE,            /**< 空闲事件 */
    RTOS_UART_EVENT_MAX
} rtos_uart_event_t;

/* UART事件回调函数类型 */
typedef void (*rtos_uart_event_callback_t)(rtos_uart_port_t port, 
                                           rtos_uart_event_t event, 
                                           void *context);

/* UART统计信息 */
typedef struct {
    uint32_t tx_bytes;              /**< 发送字节数 */
    uint32_t rx_bytes;              /**< 接收字节数 */
    uint32_t tx_packets;            /**< 发送包数 */
    uint32_t rx_packets;            /**< 接收包数 */
    uint32_t tx_errors;             /**< 发送错误数 */
    uint32_t rx_errors;             /**< 接收错误数 */
    uint32_t timeouts;              /**< 超时次数 */
    uint32_t overruns;              /**< 溢出次数 */
    uint32_t last_error;            /**< 最后错误代码 */
    uint32_t max_tx_time_ms;        /**< 最大发送时间 */
    uint32_t max_rx_time_ms;        /**< 最大接收时间 */
} rtos_uart_stats_t;

/* UART句柄结构 */
typedef struct {
    rtos_uart_port_t port;          /**< UART端口 */
    rtos_uart_config_t config;      /**< UART配置 */
    rtos_uart_buffer_config_t buffer_config; /**< 缓冲区配置 */
    rtos_uart_state_t state;        /**< UART状态 */
    rtos_uart_stats_t stats;        /**< 统计信息 */
    
    /* 事件回调 */
    rtos_uart_event_callback_t event_callbacks[RTOS_UART_EVENT_MAX];
    void *event_contexts[RTOS_UART_EVENT_MAX];
    
    /* 发送状态 */
    const uint8_t *tx_data;         /**< 当前发送数据 */
    uint32_t tx_length;             /**< 发送长度 */
    uint32_t tx_position;           /**< 发送位置 */
    
    /* 接收状态 */
    uint8_t *rx_data;               /**< 当前接收数据 */
    uint32_t rx_length;             /**< 接收长度 */
    uint32_t rx_position;           /**< 接收位置 */
    
    /* 时间戳 */
    uint32_t tx_start_time;         /**< 发送开始时间 */
    uint32_t rx_start_time;         /**< 接收开始时间 */
    
    /* 平台相关数据 */
    void *platform_data;
    
    bool initialized;
} rtos_uart_handle_t;

/* UART管理器类结构 */
typedef struct {
    /* UART句柄数组 */
    rtos_uart_handle_t uart_handles[RTOS_UART_PORT_MAX];
    
    /* 全局统计 */
    uint32_t total_tx_bytes;
    uint32_t total_rx_bytes;
    uint32_t total_errors;
    uint32_t active_ports;
    
    /* 状态标志 */
    bool initialized;
    
} rtos_uart_manager_t;

/**
 * @brief 初始化UART管理器
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_init(void);

/**
 * @brief 反初始化UART管理器
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_deinit(void);

/**
 * @brief 获取UART管理器实例
 * @return UART管理器指针
 */
rtos_uart_manager_t* rtos_uart_manager_get_instance(void);

/**
 * @brief 初始化UART端口
 * @param port UART端口
 * @param config UART配置
 * @param buffer_config 缓冲区配置（可选）
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_init_port(rtos_uart_port_t port, 
                                         const rtos_uart_config_t *config,
                                         const rtos_uart_buffer_config_t *buffer_config);

/**
 * @brief 反初始化UART端口
 * @param port UART端口
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_deinit_port(rtos_uart_port_t port);

/**
 * @brief 发送数据
 * @param port UART端口
 * @param data 发送数据
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_send(rtos_uart_port_t port, 
                                    const uint8_t *data, 
                                    uint32_t length);

/**
 * @brief 异步发送数据
 * @param port UART端口
 * @param data 发送数据
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_send_async(rtos_uart_port_t port, 
                                          const uint8_t *data, 
                                          uint32_t length);

/**
 * @brief 接收数据
 * @param port UART端口
 * @param buffer 接收缓冲区
 * @param length 期望接收长度
 * @param received 实际接收长度指针
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_receive(rtos_uart_port_t port, 
                                       uint8_t *buffer, 
                                       uint32_t length,
                                       uint32_t *received);

/**
 * @brief 异步接收数据
 * @param port UART端口
 * @param buffer 接收缓冲区
 * @param length 期望接收长度
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_receive_async(rtos_uart_port_t port, 
                                             uint8_t *buffer, 
                                             uint32_t length);

/**
 * @brief 获取UART状态
 * @param port UART端口
 * @return UART状态
 */
rtos_uart_state_t rtos_uart_manager_get_state(rtos_uart_port_t port);

/**
 * @brief 获取UART错误状态
 * @param port UART端口
 * @return 错误状态
 */
uint32_t rtos_uart_manager_get_error(rtos_uart_port_t port);

/**
 * @brief 清除UART错误
 * @param port UART端口
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_clear_error(rtos_uart_port_t port);

/**
 * @brief 中止UART传输
 * @param port UART端口
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_abort(rtos_uart_port_t port);

/**
 * @brief 注册UART事件回调
 * @param port UART端口
 * @param event 事件类型
 * @param callback 回调函数
 * @param context 用户上下文
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_register_event_callback(rtos_uart_port_t port,
                                                       rtos_uart_event_t event,
                                                       rtos_uart_event_callback_t callback,
                                                       void *context);

/**
 * @brief 注销UART事件回调
 * @param port UART端口
 * @param event 事件类型
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_unregister_event_callback(rtos_uart_port_t port,
                                                         rtos_uart_event_t event);

/**
 * @brief 获取UART统计信息
 * @param port UART端口
 * @param stats 统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_uart_manager_get_stats(rtos_uart_port_t port, rtos_uart_stats_t *stats);

/**
 * @brief UART中断处理函数
 * @param port UART端口
 */
void rtos_uart_manager_interrupt_handler(rtos_uart_port_t port);

/**
 * @brief UART DMA发送完成中断处理
 * @param port UART端口
 */
void rtos_uart_manager_dma_tx_complete_handler(rtos_uart_port_t port);

/**
 * @brief UART DMA接收完成中断处理
 * @param port UART端口
 */
void rtos_uart_manager_dma_rx_complete_handler(rtos_uart_port_t port);

/**
 * @brief 获取UART管理器统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_uart_manager_get_statistics(char *buffer, uint32_t size);

/**
 * @brief 生成UART配置报告
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_uart_manager_generate_config_report(char *buffer, uint32_t size);

/* 便利宏定义 */
#define RTOS_UART_DEFAULT_CONFIG(baudrate) \
    { .baudrate = (baudrate), \
      .databits = RTOS_UART_DATABITS_8, \
      .stopbits = RTOS_UART_STOPBITS_1, \
      .parity = RTOS_UART_PARITY_NONE, \
      .flowctrl = RTOS_UART_FLOWCTRL_NONE, \
      .timeout_ms = 1000, \
      .auto_baudrate = false }

#define RTOS_UART_SEND_STRING(port, str) \
    rtos_uart_manager_send((port), (const uint8_t*)(str), strlen(str))

#define RTOS_UART_SEND_BYTE(port, byte) \
    rtos_uart_manager_send((port), (const uint8_t*)&(byte), 1)

/* 常用波特率定义 */
#define RTOS_UART_BAUDRATE_9600     9600
#define RTOS_UART_BAUDRATE_19200    19200
#define RTOS_UART_BAUDRATE_38400    38400
#define RTOS_UART_BAUDRATE_57600    57600
#define RTOS_UART_BAUDRATE_115200   115200
#define RTOS_UART_BAUDRATE_230400   230400
#define RTOS_UART_BAUDRATE_460800   460800
#define RTOS_UART_BAUDRATE_921600   921600

/* 调试宏定义 */
#ifdef RTOS_UART_DEBUG
#define RTOS_UART_DEBUG_PRINT(fmt, ...) \
    printf("[UART] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_UART_DEBUG_PRINT(fmt, ...)
#endif

/* 错误检查宏定义 */
#ifdef RTOS_UART_ERROR_CHECK
#define RTOS_UART_CHECK_PARAM(param) \
    do { \
        if (!(param)) { \
            RTOS_UART_DEBUG_PRINT("Parameter check failed: %s", #param); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
    
#define RTOS_UART_CHECK_INIT() \
    do { \
        if (!rtos_uart_manager_get_instance()) { \
            RTOS_UART_DEBUG_PRINT("UART manager not initialized"); \
            return RTOS_ERROR_NOT_INITIALIZED; \
        } \
    } while(0)
    
#define RTOS_UART_CHECK_PORT(port) \
    do { \
        if ((port) >= RTOS_UART_PORT_MAX) { \
            RTOS_UART_DEBUG_PRINT("Invalid UART port: %d", port); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
#else
#define RTOS_UART_CHECK_PARAM(param)
#define RTOS_UART_CHECK_INIT()
#define RTOS_UART_CHECK_PORT(port)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_UART_ABSTRACTION_H__ */