/**
 * @file dma_abstraction.h
 * @brief RTOS DMA抽象模块 - 面向对象的DMA管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_DMA_ABSTRACTION_H__
#define __RTOS_DMA_ABSTRACTION_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DMA控制器定义 */
typedef enum {
    RTOS_DMA_CONTROLLER_1 = 0,
    RTOS_DMA_CONTROLLER_2,
    RTOS_DMA_CONTROLLER_MAX
} rtos_dma_controller_t;

/* DMA流定义 */
typedef enum {
    RTOS_DMA_STREAM_0 = 0,
    RTOS_DMA_STREAM_1,
    RTOS_DMA_STREAM_2,
    RTOS_DMA_STREAM_3,
    RTOS_DMA_STREAM_4,
    RTOS_DMA_STREAM_5,
    RTOS_DMA_STREAM_6,
    RTOS_DMA_STREAM_7,
    RTOS_DMA_STREAM_MAX
} rtos_dma_stream_t;

/* DMA通道定义 */
typedef enum {
    RTOS_DMA_CHANNEL_0 = 0,
    RTOS_DMA_CHANNEL_1,
    RTOS_DMA_CHANNEL_2,
    RTOS_DMA_CHANNEL_3,
    RTOS_DMA_CHANNEL_4,
    RTOS_DMA_CHANNEL_5,
    RTOS_DMA_CHANNEL_6,
    RTOS_DMA_CHANNEL_7,
    RTOS_DMA_CHANNEL_MAX
} rtos_dma_channel_t;

/* DMA传输方向定义 */
typedef enum {
    RTOS_DMA_DIR_PERIPH_TO_MEMORY = 0,  /**< 外设到内存 */
    RTOS_DMA_DIR_MEMORY_TO_PERIPH,      /**< 内存到外设 */
    RTOS_DMA_DIR_MEMORY_TO_MEMORY,      /**< 内存到内存 */
    RTOS_DMA_DIR_MAX
} rtos_dma_direction_t;

/* DMA数据大小定义 */
typedef enum {
    RTOS_DMA_DATA_SIZE_BYTE = 0,        /**< 8位 */
    RTOS_DMA_DATA_SIZE_HALFWORD,        /**< 16位 */
    RTOS_DMA_DATA_SIZE_WORD,            /**< 32位 */
    RTOS_DMA_DATA_SIZE_MAX
} rtos_dma_data_size_t;

/* DMA优先级定义 */
typedef enum {
    RTOS_DMA_PRIORITY_LOW = 0,
    RTOS_DMA_PRIORITY_MEDIUM,
    RTOS_DMA_PRIORITY_HIGH,
    RTOS_DMA_PRIORITY_VERY_HIGH,
    RTOS_DMA_PRIORITY_MAX
} rtos_dma_priority_t;

/* DMA传输模式定义 */
typedef enum {
    RTOS_DMA_MODE_NORMAL = 0,           /**< 正常模式 */
    RTOS_DMA_MODE_CIRCULAR,             /**< 循环模式 */
    RTOS_DMA_MODE_DOUBLE_BUFFER,        /**< 双缓冲模式 */
    RTOS_DMA_MODE_MAX
} rtos_dma_mode_t;

/* DMA状态定义 */
typedef enum {
    RTOS_DMA_STATE_RESET = 0,           /**< 复位状态 */
    RTOS_DMA_STATE_READY,               /**< 就绪状态 */
    RTOS_DMA_STATE_BUSY,                /**< 传输中 */
    RTOS_DMA_STATE_COMPLETE,            /**< 传输完成 */
    RTOS_DMA_STATE_ERROR,               /**< 错误状态 */
    RTOS_DMA_STATE_ABORT,               /**< 中止状态 */
    RTOS_DMA_STATE_MAX
} rtos_dma_state_t;

/* DMA错误类型定义 */
typedef enum {
    RTOS_DMA_ERROR_NONE = 0,
    RTOS_DMA_ERROR_TRANSFER = (1 << 0),     /**< 传输错误 */
    RTOS_DMA_ERROR_FIFO = (1 << 1),         /**< FIFO错误 */
    RTOS_DMA_ERROR_DIRECT_MODE = (1 << 2),  /**< 直接模式错误 */
    RTOS_DMA_ERROR_TIMEOUT = (1 << 3),      /**< 超时错误 */
    RTOS_DMA_ERROR_PARAM = (1 << 4),        /**< 参数错误 */
    RTOS_DMA_ERROR_NO_XFER = (1 << 5),      /**< 无传输错误 */
    RTOS_DMA_ERROR_NOT_SUPPORTED = (1 << 6) /**< 不支持的操作 */
} rtos_dma_error_t;

/* DMA配置结构 */
typedef struct {
    rtos_dma_controller_t controller;   /**< DMA控制器 */
    rtos_dma_stream_t stream;           /**< DMA流 */
    rtos_dma_channel_t channel;         /**< DMA通道 */
    rtos_dma_direction_t direction;     /**< 传输方向 */
    rtos_dma_data_size_t periph_data_size; /**< 外设数据大小 */
    rtos_dma_data_size_t memory_data_size;  /**< 内存数据大小 */
    rtos_dma_priority_t priority;       /**< 优先级 */
    rtos_dma_mode_t mode;               /**< 传输模式 */
    bool periph_increment;              /**< 外设地址递增 */
    bool memory_increment;              /**< 内存地址递增 */
    bool fifo_mode;                     /**< FIFO模式 */
    uint32_t fifo_threshold;            /**< FIFO阈值 */
    uint32_t memory_burst;              /**< 内存突发传输 */
    uint32_t periph_burst;              /**< 外设突发传输 */
} rtos_dma_config_t;

/* DMA传输请求结构 */
typedef struct {
    void *src_addr;                     /**< 源地址 */
    void *dst_addr;                     /**< 目标地址 */
    uint32_t data_length;               /**< 数据长度 */
    uint32_t timeout_ms;                /**< 超时时间 */
    bool blocking;                      /**< 是否阻塞等待 */
} rtos_dma_transfer_t;

/* DMA事件类型定义 */
typedef enum {
    RTOS_DMA_EVENT_TRANSFER_COMPLETE = 0, /**< 传输完成 */
    RTOS_DMA_EVENT_HALF_TRANSFER,         /**< 半传输完成 */
    RTOS_DMA_EVENT_TRANSFER_ERROR,        /**< 传输错误 */
    RTOS_DMA_EVENT_FIFO_ERROR,            /**< FIFO错误 */
    RTOS_DMA_EVENT_DIRECT_MODE_ERROR,     /**< 直接模式错误 */
    RTOS_DMA_EVENT_MAX
} rtos_dma_event_t;

/* DMA事件回调函数类型 */
typedef void (*rtos_dma_event_callback_t)(rtos_dma_controller_t controller,
                                          rtos_dma_stream_t stream,
                                          rtos_dma_event_t event,
                                          void *context);

/* DMA统计信息 */
typedef struct {
    uint32_t transfer_count;            /**< 传输次数 */
    uint32_t transfer_bytes;            /**< 传输字节数 */
    uint32_t error_count;               /**< 错误次数 */
    uint32_t timeout_count;             /**< 超时次数 */
    uint32_t max_transfer_time_ms;      /**< 最大传输时间 */
    uint32_t min_transfer_time_ms;      /**< 最小传输时间 */
    uint32_t avg_transfer_time_ms;      /**< 平均传输时间 */
    uint32_t max_throughput_mbps;       /**< 最大吞吐量 */
    uint32_t last_error;                /**< 最后错误代码 */
} rtos_dma_stats_t;

/* DMA句柄结构 */
typedef struct {
    rtos_dma_controller_t controller;   /**< DMA控制器 */
    rtos_dma_stream_t stream;           /**< DMA流 */
    rtos_dma_config_t config;           /**< DMA配置 */
    rtos_dma_state_t state;             /**< DMA状态 */
    rtos_dma_stats_t stats;             /**< 统计信息 */
    
    /* 当前传输信息 */
    rtos_dma_transfer_t current_transfer;
    uint32_t transfer_start_time;
    
    /* 事件回调 */
    rtos_dma_event_callback_t event_callbacks[RTOS_DMA_EVENT_MAX];
    void *event_contexts[RTOS_DMA_EVENT_MAX];
    
    /* 平台相关数据 */
    void *platform_data;
    
    bool initialized;
    bool allocated;
} rtos_dma_handle_t;

/* DMA管理器类结构 */
typedef struct {
    /* DMA句柄数组 */
    rtos_dma_handle_t dma_handles[RTOS_DMA_CONTROLLER_MAX][RTOS_DMA_STREAM_MAX];
    
    /* 全局统计 */
    uint32_t total_transfers;
    uint64_t total_bytes;
    uint32_t total_errors;
    uint32_t active_streams;
    
    /* 零拷贝传输队列 */
    struct {
        rtos_dma_transfer_t *queue;
        uint32_t head;
        uint32_t tail;
        uint32_t size;
        uint32_t count;
    } zero_copy_queue;
    
    /* 状态标志 */
    bool initialized;
    
} rtos_dma_manager_t;

/**
 * @brief 初始化DMA管理器
 * @param zero_copy_queue_size 零拷贝队列大小
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_init(uint32_t zero_copy_queue_size);

/**
 * @brief 反初始化DMA管理器
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_deinit(void);

/**
 * @brief 获取DMA管理器实例
 * @return DMA管理器指针
 */
rtos_dma_manager_t* rtos_dma_manager_get_instance(void);

/**
 * @brief 分配DMA流
 * @param controller DMA控制器
 * @param stream DMA流
 * @param config DMA配置
 * @param handle 返回的DMA句柄指针
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_allocate_stream(rtos_dma_controller_t controller,
                                              rtos_dma_stream_t stream,
                                              const rtos_dma_config_t *config,
                                              rtos_dma_handle_t **handle);

/**
 * @brief 释放DMA流
 * @param handle DMA句柄
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_free_stream(rtos_dma_handle_t *handle);

/**
 * @brief 启动DMA传输
 * @param handle DMA句柄
 * @param transfer 传输请求
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_start_transfer(rtos_dma_handle_t *handle,
                                             const rtos_dma_transfer_t *transfer);

/**
 * @brief 停止DMA传输
 * @param handle DMA句柄
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_stop_transfer(rtos_dma_handle_t *handle);

/**
 * @brief 中止DMA传输
 * @param handle DMA句柄
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_abort_transfer(rtos_dma_handle_t *handle);

/**
 * @brief 检查DMA传输是否完成
 * @param handle DMA句柄
 * @return 是否完成
 */
bool rtos_dma_manager_is_transfer_complete(rtos_dma_handle_t *handle);

/**
 * @brief 获取DMA传输进度
 * @param handle DMA句柄
 * @param transferred 已传输字节数指针
 * @param remaining 剩余字节数指针
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_get_transfer_progress(rtos_dma_handle_t *handle,
                                                    uint32_t *transferred,
                                                    uint32_t *remaining);

/**
 * @brief 获取DMA状态
 * @param handle DMA句柄
 * @return DMA状态
 */
rtos_dma_state_t rtos_dma_manager_get_state(rtos_dma_handle_t *handle);

/**
 * @brief 获取DMA错误状态
 * @param handle DMA句柄
 * @return 错误状态
 */
uint32_t rtos_dma_manager_get_error(rtos_dma_handle_t *handle);

/**
 * @brief 清除DMA错误
 * @param handle DMA句柄
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_clear_error(rtos_dma_handle_t *handle);

/* 零拷贝传输接口 */

/**
 * @brief 零拷贝发送数据
 * @param peripheral_id 外设ID
 * @param data 发送数据
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_zero_copy_send(uint32_t peripheral_id, 
                                             const void *data, 
                                             uint32_t length);

/**
 * @brief 零拷贝接收数据
 * @param peripheral_id 外设ID
 * @param buffer 接收缓冲区
 * @param length 缓冲区长度
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_zero_copy_receive(uint32_t peripheral_id, 
                                                void *buffer, 
                                                uint32_t length);

/**
 * @brief 内存拷贝（使用DMA）
 * @param dst 目标地址
 * @param src 源地址
 * @param length 拷贝长度
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_memcpy(void *dst, const void *src, uint32_t length);

/**
 * @brief 内存设置（使用DMA）
 * @param dst 目标地址
 * @param value 设置值
 * @param length 设置长度
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_memset(void *dst, uint8_t value, uint32_t length);

/* 事件管理接口 */

/**
 * @brief 注册DMA事件回调
 * @param handle DMA句柄
 * @param event 事件类型
 * @param callback 回调函数
 * @param context 用户上下文
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_register_event_callback(rtos_dma_handle_t *handle,
                                                      rtos_dma_event_t event,
                                                      rtos_dma_event_callback_t callback,
                                                      void *context);

/**
 * @brief 注销DMA事件回调
 * @param handle DMA句柄
 * @param event 事件类型
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_unregister_event_callback(rtos_dma_handle_t *handle,
                                                        rtos_dma_event_t event);

/* 统计和监控接口 */

/**
 * @brief 获取DMA统计信息
 * @param handle DMA句柄
 * @param stats 统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_dma_manager_get_stats(rtos_dma_handle_t *handle, rtos_dma_stats_t *stats);

/**
 * @brief DMA中断处理函数
 * @param controller DMA控制器
 * @param stream DMA流
 */
void rtos_dma_manager_interrupt_handler(rtos_dma_controller_t controller, rtos_dma_stream_t stream);

/**
 * @brief 获取DMA管理器统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_dma_manager_get_statistics(char *buffer, uint32_t size);

/**
 * @brief 运行DMA性能测试
 * @return 测试结果
 */
rtos_result_t rtos_dma_manager_run_performance_test(void);

/* 便利宏定义 */
#define RTOS_DMA_DEFAULT_CONFIG(ctrl, strm, chan, dir) \
    { .controller = (ctrl), .stream = (strm), .channel = (chan), \
      .direction = (dir), .periph_data_size = RTOS_DMA_DATA_SIZE_BYTE, \
      .memory_data_size = RTOS_DMA_DATA_SIZE_BYTE, .priority = RTOS_DMA_PRIORITY_MEDIUM, \
      .mode = RTOS_DMA_MODE_NORMAL, .periph_increment = false, \
      .memory_increment = true, .fifo_mode = false, .fifo_threshold = 0, \
      .memory_burst = 0, .periph_burst = 0 }

#define RTOS_DMA_MAKE_TRANSFER(src, dst, len) \
    { .src_addr = (void*)(src), .dst_addr = (void*)(dst), \
      .data_length = (len), .timeout_ms = 1000, .blocking = false }

/* DMA流ID计算宏 */
#define RTOS_DMA_MAKE_STREAM_ID(controller, stream) \
    (((uint32_t)(controller) << 8) | (uint32_t)(stream))

#define RTOS_DMA_GET_CONTROLLER_FROM_ID(id) \
    ((rtos_dma_controller_t)((id) >> 8))

#define RTOS_DMA_GET_STREAM_FROM_ID(id) \
    ((rtos_dma_stream_t)((id) & 0xFF))

/* 调试宏定义 */
#ifdef RTOS_DMA_DEBUG
#define RTOS_DMA_DEBUG_PRINT(fmt, ...) \
    printf("[DMA] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_DMA_DEBUG_PRINT(fmt, ...)
#endif

/* 错误检查宏定义 */
#ifdef RTOS_DMA_ERROR_CHECK
#define RTOS_DMA_CHECK_PARAM(param) \
    do { \
        if (!(param)) { \
            RTOS_DMA_DEBUG_PRINT("Parameter check failed: %s", #param); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
    
#define RTOS_DMA_CHECK_INIT() \
    do { \
        if (!rtos_dma_manager_get_instance()) { \
            RTOS_DMA_DEBUG_PRINT("DMA manager not initialized"); \
            return RTOS_ERROR_NOT_INITIALIZED; \
        } \
    } while(0)
    
#define RTOS_DMA_CHECK_HANDLE(handle) \
    do { \
        if (!(handle) || !(handle)->initialized) { \
            RTOS_DMA_DEBUG_PRINT("Invalid DMA handle"); \
            return RTOS_ERROR_INVALID_HANDLE; \
        } \
    } while(0)
#else
#define RTOS_DMA_CHECK_PARAM(param)
#define RTOS_DMA_CHECK_INIT()
#define RTOS_DMA_CHECK_HANDLE(handle)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_DMA_ABSTRACTION_H__ */