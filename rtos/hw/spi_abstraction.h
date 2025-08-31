/**
 * @file spi_abstraction.h
 * @brief RTOS SPI抽象模块 - 面向对象的SPI通信管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_SPI_ABSTRACTION_H__
#define __RTOS_SPI_ABSTRACTION_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SPI端口定义 */
typedef enum {
    RTOS_SPI_PORT_1 = 0,
    RTOS_SPI_PORT_2,
    RTOS_SPI_PORT_3,
    RTOS_SPI_PORT_MAX
} rtos_spi_port_t;

/* SPI模式定义 */
typedef enum {
    RTOS_SPI_MODE_MASTER = 0,           /**< 主机模式 */
    RTOS_SPI_MODE_SLAVE,                /**< 从机模式 */
    RTOS_SPI_MODE_MAX
} rtos_spi_mode_t;

/* SPI数据格式定义 */
typedef enum {
    RTOS_SPI_DATASIZE_8BIT = 0,         /**< 8位数据 */
    RTOS_SPI_DATASIZE_16BIT,            /**< 16位数据 */
    RTOS_SPI_DATASIZE_MAX
} rtos_spi_datasize_t;

/* SPI时钟极性定义 */
typedef enum {
    RTOS_SPI_CPOL_LOW = 0,              /**< 时钟空闲时为低电平 */
    RTOS_SPI_CPOL_HIGH,                 /**< 时钟空闲时为高电平 */
    RTOS_SPI_CPOL_MAX
} rtos_spi_cpol_t;

/* SPI时钟相位定义 */
typedef enum {
    RTOS_SPI_CPHA_1EDGE = 0,            /**< 第一个时钟边沿采样 */
    RTOS_SPI_CPHA_2EDGE,                /**< 第二个时钟边沿采样 */
    RTOS_SPI_CPHA_MAX
} rtos_spi_cpha_t;

/* SPI NSS管理定义 */
typedef enum {
    RTOS_SPI_NSS_SOFT = 0,              /**< 软件NSS管理 */
    RTOS_SPI_NSS_HARD_INPUT,            /**< 硬件NSS输入 */
    RTOS_SPI_NSS_HARD_OUTPUT,           /**< 硬件NSS输出 */
    RTOS_SPI_NSS_MAX
} rtos_spi_nss_t;

/* SPI传输模式定义 */
typedef enum {
    RTOS_SPI_TRANSFER_POLLING = 0,      /**< 轮询模式 */
    RTOS_SPI_TRANSFER_INTERRUPT,        /**< 中断模式 */
    RTOS_SPI_TRANSFER_DMA,              /**< DMA模式 */
    RTOS_SPI_TRANSFER_MAX
} rtos_spi_transfer_mode_t;

/* SPI状态定义 */
typedef enum {
    RTOS_SPI_STATE_RESET = 0,           /**< 复位状态 */
    RTOS_SPI_STATE_READY,               /**< 就绪状态 */
    RTOS_SPI_STATE_BUSY_TX,             /**< 发送忙 */
    RTOS_SPI_STATE_BUSY_RX,             /**< 接收忙 */
    RTOS_SPI_STATE_BUSY_TX_RX,          /**< 收发忙 */
    RTOS_SPI_STATE_ERROR,               /**< 错误状态 */
    RTOS_SPI_STATE_MAX
} rtos_spi_state_t;

/* SPI错误类型定义 */
typedef enum {
    RTOS_SPI_ERROR_NONE = 0,
    RTOS_SPI_ERROR_MODF = (1 << 0),     /**< 模式错误 */
    RTOS_SPI_ERROR_CRC = (1 << 1),      /**< CRC错误 */
    RTOS_SPI_ERROR_OVR = (1 << 2),      /**< 溢出错误 */
    RTOS_SPI_ERROR_FRE = (1 << 3),      /**< 帧错误 */
    RTOS_SPI_ERROR_DMA = (1 << 4),      /**< DMA错误 */
    RTOS_SPI_ERROR_TIMEOUT = (1 << 5)   /**< 超时错误 */
} rtos_spi_error_t;

/* SPI配置结构 */
typedef struct {
    rtos_spi_mode_t mode;               /**< SPI模式 */
    rtos_spi_datasize_t datasize;       /**< 数据大小 */
    rtos_spi_cpol_t cpol;               /**< 时钟极性 */
    rtos_spi_cpha_t cpha;               /**< 时钟相位 */
    rtos_spi_nss_t nss;                 /**< NSS管理 */
    rtos_spi_transfer_mode_t transfer_mode; /**< 传输模式 */
    uint32_t baudrate_prescaler;        /**< 波特率预分频 */
    bool first_bit_msb;                 /**< 是否MSB优先 */
    bool crc_enable;                    /**< CRC使能 */
    uint16_t crc_polynomial;            /**< CRC多项式 */
    uint32_t timeout_ms;                /**< 超时时间 */
} rtos_spi_config_t;

/* SPI设备配置 */
typedef struct {
    rtos_spi_port_t port;               /**< SPI端口 */
    uint32_t cs_port;                   /**< CS端口 */
    uint32_t cs_pin;                    /**< CS引脚 */
    bool cs_active_low;                 /**< CS低电平有效 */
    uint32_t cs_setup_time_us;          /**< CS建立时间 */
    uint32_t cs_hold_time_us;           /**< CS保持时间 */
    uint32_t max_speed_hz;              /**< 最大速度 */
    const char *device_name;            /**< 设备名称 */
} rtos_spi_device_config_t;

/* SPI事件类型定义 */
typedef enum {
    RTOS_SPI_EVENT_TX_COMPLETE = 0,     /**< 发送完成 */
    RTOS_SPI_EVENT_RX_COMPLETE,         /**< 接收完成 */
    RTOS_SPI_EVENT_TX_RX_COMPLETE,      /**< 收发完成 */
    RTOS_SPI_EVENT_ERROR,               /**< 错误事件 */
    RTOS_SPI_EVENT_MAX
} rtos_spi_event_t;

/* SPI事件回调函数类型 */
typedef void (*rtos_spi_event_callback_t)(rtos_spi_port_t port, 
                                          rtos_spi_event_t event, 
                                          void *context);

/* SPI统计信息 */
typedef struct {
    uint32_t tx_bytes;                  /**< 发送字节数 */
    uint32_t rx_bytes;                  /**< 接收字节数 */
    uint32_t tx_packets;                /**< 发送包数 */
    uint32_t rx_packets;                /**< 接收包数 */
    uint32_t tx_errors;                 /**< 发送错误数 */
    uint32_t rx_errors;                 /**< 接收错误数 */
    uint32_t timeouts;                  /**< 超时次数 */
    uint32_t cs_operations;             /**< CS操作次数 */
    uint32_t max_transfer_time_ms;      /**< 最大传输时间 */
    uint32_t avg_transfer_time_ms;      /**< 平均传输时间 */
    uint32_t max_speed_kbps;            /**< 最大速度 */
} rtos_spi_stats_t;

/* SPI句柄结构 */
typedef struct {
    rtos_spi_port_t port;               /**< SPI端口 */
    rtos_spi_config_t config;           /**< SPI配置 */
    rtos_spi_state_t state;             /**< SPI状态 */
    rtos_spi_stats_t stats;             /**< 统计信息 */
    
    /* 事件回调 */
    rtos_spi_event_callback_t event_callbacks[RTOS_SPI_EVENT_MAX];
    void *event_contexts[RTOS_SPI_EVENT_MAX];
    
    /* 传输状态 */
    const uint8_t *tx_data;             /**< 发送数据 */
    uint8_t *rx_data;                   /**< 接收数据 */
    uint32_t tx_length;                 /**< 发送长度 */
    uint32_t rx_length;                 /**< 接收长度 */
    uint32_t tx_position;               /**< 发送位置 */
    uint32_t rx_position;               /**< 接收位置 */
    
    /* DMA句柄 */
    void *tx_dma_handle;                /**< 发送DMA句柄 */
    void *rx_dma_handle;                /**< 接收DMA句柄 */
    
    /* 时间戳 */
    uint32_t transfer_start_time;       /**< 传输开始时间 */
    
    /* 设备管理 */
    rtos_spi_device_config_t *devices;  /**< 挂载的设备 */
    uint32_t device_count;              /**< 设备数量 */
    uint32_t max_devices;               /**< 最大设备数 */
    
    bool initialized;
} rtos_spi_handle_t;

/* SPI管理器类结构 */
typedef struct {
    /* SPI句柄数组 */
    rtos_spi_handle_t spi_handles[RTOS_SPI_PORT_MAX];
    
    /* 全局统计 */
    uint32_t total_tx_bytes;
    uint32_t total_rx_bytes;
    uint32_t total_errors;
    uint32_t active_ports;
    
    /* 状态标志 */
    bool initialized;
    
} rtos_spi_manager_t;

/**
 * @brief 初始化SPI管理器
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_init(void);

/**
 * @brief 反初始化SPI管理器
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_deinit(void);

/**
 * @brief 获取SPI管理器实例
 * @return SPI管理器指针
 */
rtos_spi_manager_t* rtos_spi_manager_get_instance(void);

/**
 * @brief 初始化SPI端口
 * @param port SPI端口
 * @param config SPI配置
 * @param max_devices 最大设备数
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_init_port(rtos_spi_port_t port, 
                                        const rtos_spi_config_t *config,
                                        uint32_t max_devices);

/**
 * @brief 反初始化SPI端口
 * @param port SPI端口
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_deinit_port(rtos_spi_port_t port);

/**
 * @brief 添加SPI设备
 * @param port SPI端口
 * @param device_config 设备配置
 * @param device_id 返回的设备ID指针
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_add_device(rtos_spi_port_t port,
                                         const rtos_spi_device_config_t *device_config,
                                         uint32_t *device_id);

/**
 * @brief 移除SPI设备
 * @param port SPI端口
 * @param device_id 设备ID
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_remove_device(rtos_spi_port_t port, uint32_t device_id);

/**
 * @brief SPI传输数据
 * @param port SPI端口
 * @param device_id 设备ID
 * @param tx_data 发送数据
 * @param rx_data 接收数据
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_transfer(rtos_spi_port_t port,
                                       uint32_t device_id,
                                       const uint8_t *tx_data,
                                       uint8_t *rx_data,
                                       uint32_t length);

/**
 * @brief SPI发送数据
 * @param port SPI端口
 * @param device_id 设备ID
 * @param data 发送数据
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_send(rtos_spi_port_t port,
                                   uint32_t device_id,
                                   const uint8_t *data,
                                   uint32_t length);

/**
 * @brief SPI接收数据
 * @param port SPI端口
 * @param device_id 设备ID
 * @param buffer 接收缓冲区
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_receive(rtos_spi_port_t port,
                                      uint32_t device_id,
                                      uint8_t *buffer,
                                      uint32_t length);

/**
 * @brief 异步SPI传输
 * @param port SPI端口
 * @param device_id 设备ID
 * @param tx_data 发送数据
 * @param rx_data 接收数据
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_transfer_async(rtos_spi_port_t port,
                                             uint32_t device_id,
                                             const uint8_t *tx_data,
                                             uint8_t *rx_data,
                                             uint32_t length);

/**
 * @brief 获取SPI状态
 * @param port SPI端口
 * @return SPI状态
 */
rtos_spi_state_t rtos_spi_manager_get_state(rtos_spi_port_t port);

/**
 * @brief 获取SPI错误状态
 * @param port SPI端口
 * @return 错误状态
 */
uint32_t rtos_spi_manager_get_error(rtos_spi_port_t port);

/**
 * @brief 清除SPI错误
 * @param port SPI端口
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_clear_error(rtos_spi_port_t port);

/**
 * @brief 中止SPI传输
 * @param port SPI端口
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_abort(rtos_spi_port_t port);

/**
 * @brief 注册SPI事件回调
 * @param port SPI端口
 * @param event 事件类型
 * @param callback 回调函数
 * @param context 用户上下文
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_register_event_callback(rtos_spi_port_t port,
                                                      rtos_spi_event_t event,
                                                      rtos_spi_event_callback_t callback,
                                                      void *context);

/**
 * @brief SPI中断处理函数
 * @param port SPI端口
 */
void rtos_spi_manager_interrupt_handler(rtos_spi_port_t port);

/**
 * @brief 获取SPI统计信息
 * @param port SPI端口
 * @param stats 统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_spi_manager_get_stats(rtos_spi_port_t port, rtos_spi_stats_t *stats);

/**
 * @brief 获取SPI管理器统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_spi_manager_get_statistics(char *buffer, uint32_t size);

/* 便利宏定义 */
#define RTOS_SPI_DEFAULT_CONFIG() \
    { .mode = RTOS_SPI_MODE_MASTER, \
      .datasize = RTOS_SPI_DATASIZE_8BIT, \
      .cpol = RTOS_SPI_CPOL_LOW, \
      .cpha = RTOS_SPI_CPHA_1EDGE, \
      .nss = RTOS_SPI_NSS_SOFT, \
      .transfer_mode = RTOS_SPI_TRANSFER_INTERRUPT, \
      .baudrate_prescaler = 8, \
      .first_bit_msb = true, \
      .crc_enable = false, \
      .crc_polynomial = 7, \
      .timeout_ms = 1000 }

#define RTOS_SPI_DEVICE_CONFIG(port, cs_port, cs_pin, name) \
    { .port = (port), .cs_port = (cs_port), .cs_pin = (cs_pin), \
      .cs_active_low = true, .cs_setup_time_us = 1, .cs_hold_time_us = 1, \
      .max_speed_hz = 1000000, .device_name = (name) }

/* 常用SPI速度定义 */
#define RTOS_SPI_SPEED_125KHZ   256     /* 84MHz/256 = 328kHz */
#define RTOS_SPI_SPEED_250KHZ   128     /* 84MHz/128 = 656kHz */
#define RTOS_SPI_SPEED_500KHZ   64      /* 84MHz/64 = 1.3MHz */
#define RTOS_SPI_SPEED_1MHZ     32      /* 84MHz/32 = 2.6MHz */
#define RTOS_SPI_SPEED_2MHZ     16      /* 84MHz/16 = 5.25MHz */
#define RTOS_SPI_SPEED_5MHZ     8       /* 84MHz/8 = 10.5MHz */
#define RTOS_SPI_SPEED_10MHZ    4       /* 84MHz/4 = 21MHz */
#define RTOS_SPI_SPEED_20MHZ    2       /* 84MHz/2 = 42MHz */

/* 调试宏定义 */
#ifdef RTOS_SPI_DEBUG
#define RTOS_SPI_DEBUG_PRINT(fmt, ...) \
    printf("[SPI] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_SPI_DEBUG_PRINT(fmt, ...)
#endif

/* 错误检查宏定义 */
#ifdef RTOS_SPI_ERROR_CHECK
#define RTOS_SPI_CHECK_PARAM(param) \
    do { \
        if (!(param)) { \
            RTOS_SPI_DEBUG_PRINT("Parameter check failed: %s", #param); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
    
#define RTOS_SPI_CHECK_INIT() \
    do { \
        if (!rtos_spi_manager_get_instance()) { \
            RTOS_SPI_DEBUG_PRINT("SPI manager not initialized"); \
            return RTOS_ERROR_NOT_INITIALIZED; \
        } \
    } while(0)
    
#define RTOS_SPI_CHECK_PORT(port) \
    do { \
        if ((port) >= RTOS_SPI_PORT_MAX) { \
            RTOS_SPI_DEBUG_PRINT("Invalid SPI port: %d", port); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
#else
#define RTOS_SPI_CHECK_PARAM(param)
#define RTOS_SPI_CHECK_INIT()
#define RTOS_SPI_CHECK_PORT(port)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_SPI_ABSTRACTION_H__ */