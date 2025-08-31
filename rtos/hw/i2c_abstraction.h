/**
 * @file i2c_abstraction.h
 * @brief RTOS I2C抽象模块 - 面向对象的I2C通信管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_I2C_ABSTRACTION_H__
#define __RTOS_I2C_ABSTRACTION_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C端口定义 */
typedef enum {
    RTOS_I2C_PORT_1 = 0,
    RTOS_I2C_PORT_2,
    RTOS_I2C_PORT_3,
    RTOS_I2C_PORT_MAX
} rtos_i2c_port_t;

/* I2C模式定义 */
typedef enum {
    RTOS_I2C_MODE_STANDARD = 0,         /**< 标准模式 100kHz */
    RTOS_I2C_MODE_FAST,                 /**< 快速模式 400kHz */
    RTOS_I2C_MODE_FAST_PLUS,            /**< 快速+模式 1MHz */
    RTOS_I2C_MODE_HIGH_SPEED,           /**< 高速模式 3.4MHz */
    RTOS_I2C_MODE_MAX
} rtos_i2c_mode_t;

/* I2C地址模式定义 */
typedef enum {
    RTOS_I2C_ADDR_7BIT = 0,             /**< 7位地址 */
    RTOS_I2C_ADDR_10BIT,                /**< 10位地址 */
    RTOS_I2C_ADDR_MAX
} rtos_i2c_addr_mode_t;

/* I2C传输模式定义 */
typedef enum {
    RTOS_I2C_TRANSFER_POLLING = 0,      /**< 轮询模式 */
    RTOS_I2C_TRANSFER_INTERRUPT,        /**< 中断模式 */
    RTOS_I2C_TRANSFER_DMA,              /**< DMA模式 */
    RTOS_I2C_TRANSFER_MAX
} rtos_i2c_transfer_mode_t;

/* I2C状态定义 */
typedef enum {
    RTOS_I2C_STATE_RESET = 0,           /**< 复位状态 */
    RTOS_I2C_STATE_READY,               /**< 就绪状态 */
    RTOS_I2C_STATE_BUSY_TX,             /**< 发送忙 */
    RTOS_I2C_STATE_BUSY_RX,             /**< 接收忙 */
    RTOS_I2C_STATE_LISTEN,              /**< 监听状态(从机) */
    RTOS_I2C_STATE_ERROR,               /**< 错误状态 */
    RTOS_I2C_STATE_MAX
} rtos_i2c_state_t;

/* I2C错误类型定义 */
typedef enum {
    RTOS_I2C_ERROR_NONE = 0,
    RTOS_I2C_ERROR_BERR = (1 << 0),     /**< 总线错误 */
    RTOS_I2C_ERROR_ARLO = (1 << 1),     /**< 仲裁丢失 */
    RTOS_I2C_ERROR_AF = (1 << 2),       /**< 应答失败 */
    RTOS_I2C_ERROR_OVR = (1 << 3),      /**< 溢出错误 */
    RTOS_I2C_ERROR_DMA = (1 << 4),      /**< DMA错误 */
    RTOS_I2C_ERROR_TIMEOUT = (1 << 5),  /**< 超时错误 */
    RTOS_I2C_ERROR_SIZE = (1 << 6),     /**< 大小错误 */
    RTOS_I2C_ERROR_DMA_PARAM = (1 << 7) /**< DMA参数错误 */
} rtos_i2c_error_t;

/* I2C配置结构 */
typedef struct {
    rtos_i2c_mode_t mode;               /**< I2C模式 */
    rtos_i2c_addr_mode_t addr_mode;     /**< 地址模式 */
    rtos_i2c_transfer_mode_t transfer_mode; /**< 传输模式 */
    uint32_t clock_speed;               /**< 时钟速度 */
    uint16_t own_address;               /**< 自身地址(从机模式) */
    bool dual_addr_enable;              /**< 双地址使能 */
    uint16_t own_address2;              /**< 第二地址 */
    bool general_call_enable;           /**< 广播呼叫使能 */
    bool no_stretch_enable;             /**< 禁用时钟延展 */
    uint32_t timeout_ms;                /**< 超时时间 */
} rtos_i2c_config_t;

/* I2C设备配置 */
typedef struct {
    uint16_t device_address;            /**< 设备地址 */
    rtos_i2c_addr_mode_t addr_mode;     /**< 地址模式 */
    uint32_t max_speed_hz;              /**< 最大速度 */
    uint32_t page_size;                 /**< 页大小(EEPROM等) */
    uint32_t timeout_ms;                /**< 设备超时 */
    const char *device_name;            /**< 设备名称 */
    bool memory_device;                 /**< 是否为存储设备 */
} rtos_i2c_device_config_t;

/* I2C传输结构 */
typedef struct {
    uint16_t device_addr;               /**< 设备地址 */
    uint16_t memory_addr;               /**< 内存地址(存储设备) */
    uint8_t memory_addr_size;           /**< 内存地址大小 */
    uint8_t *data;                      /**< 数据指针 */
    uint32_t length;                    /**< 数据长度 */
    bool read_operation;                /**< 是否为读操作 */
    uint32_t timeout_ms;                /**< 超时时间 */
} rtos_i2c_transfer_t;

/* I2C事件类型定义 */
typedef enum {
    RTOS_I2C_EVENT_MASTER_TX_COMPLETE = 0, /**< 主机发送完成 */
    RTOS_I2C_EVENT_MASTER_RX_COMPLETE,     /**< 主机接收完成 */
    RTOS_I2C_EVENT_SLAVE_TX_COMPLETE,      /**< 从机发送完成 */
    RTOS_I2C_EVENT_SLAVE_RX_COMPLETE,      /**< 从机接收完成 */
    RTOS_I2C_EVENT_LISTEN_COMPLETE,        /**< 监听完成 */
    RTOS_I2C_EVENT_ERROR,                  /**< 错误事件 */
    RTOS_I2C_EVENT_MAX
} rtos_i2c_event_t;

/* I2C事件回调函数类型 */
typedef void (*rtos_i2c_event_callback_t)(rtos_i2c_port_t port, 
                                          rtos_i2c_event_t event, 
                                          void *context);

/* I2C统计信息 */
typedef struct {
    uint32_t master_tx_count;           /**< 主机发送次数 */
    uint32_t master_rx_count;           /**< 主机接收次数 */
    uint32_t slave_tx_count;            /**< 从机发送次数 */
    uint32_t slave_rx_count;            /**< 从机接收次数 */
    uint32_t total_bytes;               /**< 总字节数 */
    uint32_t error_count;               /**< 错误次数 */
    uint32_t timeout_count;             /**< 超时次数 */
    uint32_t arbitration_lost_count;    /**< 仲裁丢失次数 */
    uint32_t nack_count;                /**< NACK次数 */
    uint32_t max_transfer_time_ms;      /**< 最大传输时间 */
    uint32_t avg_transfer_time_ms;      /**< 平均传输时间 */
} rtos_i2c_stats_t;

/* I2C句柄结构 */
typedef struct {
    rtos_i2c_port_t port;               /**< I2C端口 */
    rtos_i2c_config_t config;           /**< I2C配置 */
    rtos_i2c_state_t state;             /**< I2C状态 */
    rtos_i2c_stats_t stats;             /**< 统计信息 */
    
    /* 事件回调 */
    rtos_i2c_event_callback_t event_callbacks[RTOS_I2C_EVENT_MAX];
    void *event_contexts[RTOS_I2C_EVENT_MAX];
    
    /* 当前传输信息 */
    rtos_i2c_transfer_t current_transfer;
    uint32_t transfer_position;
    uint32_t transfer_start_time;
    
    /* DMA句柄 */
    void *tx_dma_handle;                /**< 发送DMA句柄 */
    void *rx_dma_handle;                /**< 接收DMA句柄 */
    
    /* 设备管理 */
    rtos_i2c_device_config_t *devices;  /**< 挂载的设备 */
    uint32_t device_count;              /**< 设备数量 */
    uint32_t max_devices;               /**< 最大设备数 */
    
    /* 从机模式缓冲区 */
    uint8_t *slave_tx_buffer;           /**< 从机发送缓冲区 */
    uint8_t *slave_rx_buffer;           /**< 从机接收缓冲区 */
    uint32_t slave_buffer_size;         /**< 从机缓冲区大小 */
    
    bool initialized;
} rtos_i2c_handle_t;

/* I2C管理器类结构 */
typedef struct {
    /* I2C句柄数组 */
    rtos_i2c_handle_t i2c_handles[RTOS_I2C_PORT_MAX];
    
    /* 全局统计 */
    uint32_t total_transfers;
    uint32_t total_bytes;
    uint32_t total_errors;
    uint32_t active_ports;
    
    /* 设备扫描 */
    struct {
        bool scanning;
        uint8_t scan_port;
        uint8_t scan_address;
        uint8_t found_devices[128];
        uint32_t found_count;
    } device_scan;
    
    /* 状态标志 */
    bool initialized;
    
} rtos_i2c_manager_t;

/**
 * @brief 初始化I2C管理器
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_init(void);

/**
 * @brief 反初始化I2C管理器
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_deinit(void);

/**
 * @brief 获取I2C管理器实例
 * @return I2C管理器指针
 */
rtos_i2c_manager_t* rtos_i2c_manager_get_instance(void);

/**
 * @brief 初始化I2C端口
 * @param port I2C端口
 * @param config I2C配置
 * @param max_devices 最大设备数
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_init_port(rtos_i2c_port_t port, 
                                        const rtos_i2c_config_t *config,
                                        uint32_t max_devices);

/**
 * @brief 主机模式发送数据
 * @param port I2C端口
 * @param device_addr 设备地址
 * @param data 发送数据
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_master_send(rtos_i2c_port_t port,
                                          uint16_t device_addr,
                                          const uint8_t *data,
                                          uint32_t length);

/**
 * @brief 主机模式接收数据
 * @param port I2C端口
 * @param device_addr 设备地址
 * @param buffer 接收缓冲区
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_master_receive(rtos_i2c_port_t port,
                                             uint16_t device_addr,
                                             uint8_t *buffer,
                                             uint32_t length);

/**
 * @brief 内存设备写操作
 * @param port I2C端口
 * @param device_addr 设备地址
 * @param memory_addr 内存地址
 * @param memory_addr_size 内存地址大小
 * @param data 写入数据
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_memory_write(rtos_i2c_port_t port,
                                           uint16_t device_addr,
                                           uint16_t memory_addr,
                                           uint8_t memory_addr_size,
                                           const uint8_t *data,
                                           uint32_t length);

/**
 * @brief 内存设备读操作
 * @param port I2C端口
 * @param device_addr 设备地址
 * @param memory_addr 内存地址
 * @param memory_addr_size 内存地址大小
 * @param buffer 读取缓冲区
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_memory_read(rtos_i2c_port_t port,
                                          uint16_t device_addr,
                                          uint16_t memory_addr,
                                          uint8_t memory_addr_size,
                                          uint8_t *buffer,
                                          uint32_t length);

/**
 * @brief 检查设备是否就绪
 * @param port I2C端口
 * @param device_addr 设备地址
 * @param trials 尝试次数
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_is_device_ready(rtos_i2c_port_t port,
                                              uint16_t device_addr,
                                              uint32_t trials);

/**
 * @brief 扫描I2C总线设备
 * @param port I2C端口
 * @param found_devices 找到的设备地址数组
 * @param max_devices 最大设备数
 * @param found_count 实际找到的设备数指针
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_scan_bus(rtos_i2c_port_t port,
                                       uint8_t *found_devices,
                                       uint32_t max_devices,
                                       uint32_t *found_count);

/**
 * @brief 添加I2C设备
 * @param port I2C端口
 * @param device_config 设备配置
 * @param device_id 返回的设备ID指针
 * @return 操作结果
 */
rtos_result_t rtos_i2c_manager_add_device(rtos_i2c_port_t port,
                                         const rtos_i2c_device_config_t *device_config,
                                         uint32_t *device_id);

/**
 * @brief 获取I2C状态
 * @param port I2C端口
 * @return I2C状态
 */
rtos_i2c_state_t rtos_i2c_manager_get_state(rtos_i2c_port_t port);

/**
 * @brief I2C中断处理函数
 * @param port I2C端口
 */
void rtos_i2c_manager_interrupt_handler(rtos_i2c_port_t port);

/**
 * @brief 获取I2C管理器统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_i2c_manager_get_statistics(char *buffer, uint32_t size);

/* 便利宏定义 */
#define RTOS_I2C_DEFAULT_CONFIG(speed_mode) \
    { .mode = (speed_mode), \
      .addr_mode = RTOS_I2C_ADDR_7BIT, \
      .transfer_mode = RTOS_I2C_TRANSFER_INTERRUPT, \
      .clock_speed = 100000, \
      .own_address = 0x00, \
      .dual_addr_enable = false, \
      .own_address2 = 0x00, \
      .general_call_enable = false, \
      .no_stretch_enable = false, \
      .timeout_ms = 1000 }

#define RTOS_I2C_DEVICE_CONFIG(addr, name) \
    { .device_address = (addr), \
      .addr_mode = RTOS_I2C_ADDR_7BIT, \
      .max_speed_hz = 100000, \
      .page_size = 32, \
      .timeout_ms = 100, \
      .device_name = (name), \
      .memory_device = false }

/* 常用I2C速度定义 */
#define RTOS_I2C_SPEED_STANDARD     100000  /* 100kHz */
#define RTOS_I2C_SPEED_FAST         400000  /* 400kHz */
#define RTOS_I2C_SPEED_FAST_PLUS    1000000 /* 1MHz */

/* 调试宏定义 */
#ifdef RTOS_I2C_DEBUG
#define RTOS_I2C_DEBUG_PRINT(fmt, ...) \
    printf("[I2C] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_I2C_DEBUG_PRINT(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_I2C_ABSTRACTION_H__ */