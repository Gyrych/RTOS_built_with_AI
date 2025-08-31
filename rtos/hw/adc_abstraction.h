/**
 * @file adc_abstraction.h
 * @brief RTOS ADC抽象模块 - 面向对象的ADC管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_ADC_ABSTRACTION_H__
#define __RTOS_ADC_ABSTRACTION_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ADC控制器定义 */
typedef enum {
    RTOS_ADC_CONTROLLER_1 = 0,
    RTOS_ADC_CONTROLLER_2,
    RTOS_ADC_CONTROLLER_3,
    RTOS_ADC_CONTROLLER_MAX
} rtos_adc_controller_t;

/* ADC通道定义 */
typedef enum {
    RTOS_ADC_CHANNEL_0 = 0,
    RTOS_ADC_CHANNEL_1,
    RTOS_ADC_CHANNEL_2,
    RTOS_ADC_CHANNEL_3,
    RTOS_ADC_CHANNEL_4,
    RTOS_ADC_CHANNEL_5,
    RTOS_ADC_CHANNEL_6,
    RTOS_ADC_CHANNEL_7,
    RTOS_ADC_CHANNEL_8,
    RTOS_ADC_CHANNEL_9,
    RTOS_ADC_CHANNEL_10,
    RTOS_ADC_CHANNEL_11,
    RTOS_ADC_CHANNEL_12,
    RTOS_ADC_CHANNEL_13,
    RTOS_ADC_CHANNEL_14,
    RTOS_ADC_CHANNEL_15,
    RTOS_ADC_CHANNEL_TEMPSENSOR,        /**< 温度传感器 */
    RTOS_ADC_CHANNEL_VREFINT,           /**< 内部参考电压 */
    RTOS_ADC_CHANNEL_VBAT,              /**< 电池电压 */
    RTOS_ADC_CHANNEL_MAX
} rtos_adc_channel_t;

/* ADC分辨率定义 */
typedef enum {
    RTOS_ADC_RESOLUTION_12BIT = 0,      /**< 12位分辨率 */
    RTOS_ADC_RESOLUTION_10BIT,          /**< 10位分辨率 */
    RTOS_ADC_RESOLUTION_8BIT,           /**< 8位分辨率 */
    RTOS_ADC_RESOLUTION_6BIT,           /**< 6位分辨率 */
    RTOS_ADC_RESOLUTION_MAX
} rtos_adc_resolution_t;

/* ADC采样时间定义 */
typedef enum {
    RTOS_ADC_SAMPLETIME_3CYCLES = 0,
    RTOS_ADC_SAMPLETIME_15CYCLES,
    RTOS_ADC_SAMPLETIME_28CYCLES,
    RTOS_ADC_SAMPLETIME_56CYCLES,
    RTOS_ADC_SAMPLETIME_84CYCLES,
    RTOS_ADC_SAMPLETIME_112CYCLES,
    RTOS_ADC_SAMPLETIME_144CYCLES,
    RTOS_ADC_SAMPLETIME_480CYCLES,
    RTOS_ADC_SAMPLETIME_MAX
} rtos_adc_sampletime_t;

/* ADC转换模式定义 */
typedef enum {
    RTOS_ADC_MODE_SINGLE = 0,           /**< 单次转换 */
    RTOS_ADC_MODE_CONTINUOUS,           /**< 连续转换 */
    RTOS_ADC_MODE_SCAN,                 /**< 扫描模式 */
    RTOS_ADC_MODE_DISCONTINUOUS,        /**< 间断模式 */
    RTOS_ADC_MODE_MAX
} rtos_adc_mode_t;

/* ADC触发源定义 */
typedef enum {
    RTOS_ADC_TRIGGER_SOFTWARE = 0,      /**< 软件触发 */
    RTOS_ADC_TRIGGER_TIMER,             /**< 定时器触发 */
    RTOS_ADC_TRIGGER_EXTERNAL,          /**< 外部触发 */
    RTOS_ADC_TRIGGER_MAX
} rtos_adc_trigger_t;

/* ADC状态定义 */
typedef enum {
    RTOS_ADC_STATE_RESET = 0,           /**< 复位状态 */
    RTOS_ADC_STATE_READY,               /**< 就绪状态 */
    RTOS_ADC_STATE_BUSY,                /**< 转换中 */
    RTOS_ADC_STATE_MULTIMODE_SLAVE,     /**< 多ADC从机模式 */
    RTOS_ADC_STATE_ERROR,               /**< 错误状态 */
    RTOS_ADC_STATE_MAX
} rtos_adc_state_t;

/* ADC配置结构 */
typedef struct {
    rtos_adc_resolution_t resolution;   /**< 分辨率 */
    rtos_adc_mode_t mode;               /**< 转换模式 */
    rtos_adc_trigger_t trigger;         /**< 触发源 */
    bool external_trigger_enable;       /**< 外部触发使能 */
    uint32_t external_trigger_edge;     /**< 外部触发边沿 */
    bool continuous_mode;               /**< 连续模式 */
    bool dma_enable;                    /**< DMA使能 */
    bool dma_continuous;                /**< DMA连续模式 */
    uint32_t nb_conversions;            /**< 转换数量 */
    bool scan_mode;                     /**< 扫描模式 */
    bool discontinuous_mode;            /**< 间断模式 */
    uint32_t nb_discontinuous;          /**< 间断转换数 */
} rtos_adc_config_t;

/* ADC通道配置 */
typedef struct {
    rtos_adc_channel_t channel;         /**< ADC通道 */
    uint32_t rank;                      /**< 转换序列 */
    rtos_adc_sampletime_t sample_time;  /**< 采样时间 */
    uint32_t offset;                    /**< 偏移值 */
    float gain;                         /**< 增益 */
    float reference_voltage;            /**< 参考电压 */
    const char *channel_name;           /**< 通道名称 */
} rtos_adc_channel_config_t;

/* ADC事件类型定义 */
typedef enum {
    RTOS_ADC_EVENT_CONVERSION_COMPLETE = 0, /**< 转换完成 */
    RTOS_ADC_EVENT_SEQUENCE_COMPLETE,       /**< 序列完成 */
    RTOS_ADC_EVENT_HALF_CONVERSION,         /**< 半转换完成 */
    RTOS_ADC_EVENT_ERROR,                   /**< 错误事件 */
    RTOS_ADC_EVENT_OVERRUN,                 /**< 溢出事件 */
    RTOS_ADC_EVENT_MAX
} rtos_adc_event_t;

/* ADC事件回调函数类型 */
typedef void (*rtos_adc_event_callback_t)(rtos_adc_controller_t controller, 
                                          rtos_adc_event_t event, 
                                          void *context);

/* ADC统计信息 */
typedef struct {
    uint32_t conversion_count;          /**< 转换次数 */
    uint32_t sequence_count;            /**< 序列次数 */
    uint32_t error_count;               /**< 错误次数 */
    uint32_t overrun_count;             /**< 溢出次数 */
    uint32_t max_conversion_time_us;    /**< 最大转换时间 */
    uint32_t avg_conversion_time_us;    /**< 平均转换时间 */
    uint32_t min_value;                 /**< 最小值 */
    uint32_t max_value;                 /**< 最大值 */
    uint32_t avg_value;                 /**< 平均值 */
} rtos_adc_stats_t;

/* ADC句柄结构 */
typedef struct {
    rtos_adc_controller_t controller;   /**< ADC控制器 */
    rtos_adc_config_t config;           /**< ADC配置 */
    rtos_adc_state_t state;             /**< ADC状态 */
    rtos_adc_stats_t stats;             /**< 统计信息 */
    
    /* 通道配置 */
    rtos_adc_channel_config_t *channels;/**< 通道配置数组 */
    uint32_t channel_count;             /**< 配置的通道数 */
    uint32_t max_channels;              /**< 最大通道数 */
    
    /* 转换结果 */
    uint16_t *conversion_buffer;        /**< 转换结果缓冲区 */
    uint32_t buffer_size;               /**< 缓冲区大小 */
    uint32_t conversion_index;          /**< 当前转换索引 */
    
    /* 事件回调 */
    rtos_adc_event_callback_t event_callbacks[RTOS_ADC_EVENT_MAX];
    void *event_contexts[RTOS_ADC_EVENT_MAX];
    
    /* DMA句柄 */
    void *dma_handle;                   /**< DMA句柄 */
    
    /* 校准数据 */
    struct {
        bool calibrated;
        float gain_correction;
        int16_t offset_correction;
        float temp_sensor_slope;
        float temp_sensor_v25;
        float vrefint_cal;
    } calibration;
    
    /* 时间戳 */
    uint32_t conversion_start_time;
    
    bool initialized;
} rtos_adc_handle_t;

/* ADC管理器类结构 */
typedef struct {
    /* ADC句柄数组 */
    rtos_adc_handle_t adc_handles[RTOS_ADC_CONTROLLER_MAX];
    
    /* 全局统计 */
    uint32_t total_conversions;
    uint32_t total_errors;
    uint32_t active_controllers;
    
    /* 多ADC同步 */
    struct {
        bool sync_mode_enabled;
        rtos_adc_controller_t master_adc;
        uint32_t sync_channels;
    } multi_adc;
    
    /* 状态标志 */
    bool initialized;
    
} rtos_adc_manager_t;

/**
 * @brief 初始化ADC管理器
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_init(void);

/**
 * @brief 初始化ADC控制器
 * @param controller ADC控制器
 * @param config ADC配置
 * @param max_channels 最大通道数
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_init_controller(rtos_adc_controller_t controller,
                                              const rtos_adc_config_t *config,
                                              uint32_t max_channels);

/**
 * @brief 配置ADC通道
 * @param controller ADC控制器
 * @param channel_config 通道配置
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_config_channel(rtos_adc_controller_t controller,
                                             const rtos_adc_channel_config_t *channel_config);

/**
 * @brief 单次ADC转换
 * @param controller ADC控制器
 * @param channel ADC通道
 * @param value 转换结果指针
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_convert_single(rtos_adc_controller_t controller,
                                             rtos_adc_channel_t channel,
                                             uint16_t *value);

/**
 * @brief 启动连续转换
 * @param controller ADC控制器
 * @param buffer 结果缓冲区
 * @param buffer_size 缓冲区大小
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_start_continuous(rtos_adc_controller_t controller,
                                               uint16_t *buffer,
                                               uint32_t buffer_size);

/**
 * @brief 停止连续转换
 * @param controller ADC控制器
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_stop_continuous(rtos_adc_controller_t controller);

/**
 * @brief 读取校准后的电压值
 * @param controller ADC控制器
 * @param channel ADC通道
 * @param voltage_mv 电压值指针(mV)
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_read_voltage(rtos_adc_controller_t controller,
                                           rtos_adc_channel_t channel,
                                           uint32_t *voltage_mv);

/**
 * @brief 读取温度
 * @param controller ADC控制器
 * @param temperature_celsius 温度指针(摄氏度)
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_read_temperature(rtos_adc_controller_t controller,
                                               int16_t *temperature_celsius);

/**
 * @brief ADC校准
 * @param controller ADC控制器
 * @return 操作结果
 */
rtos_result_t rtos_adc_manager_calibrate(rtos_adc_controller_t controller);

/**
 * @brief 获取ADC管理器实例
 * @return ADC管理器指针
 */
rtos_adc_manager_t* rtos_adc_manager_get_instance(void);

/**
 * @brief ADC中断处理函数
 * @param controller ADC控制器
 */
void rtos_adc_manager_interrupt_handler(rtos_adc_controller_t controller);

/* 便利宏定义 */
#define RTOS_ADC_DEFAULT_CONFIG() \
    { .resolution = RTOS_ADC_RESOLUTION_12BIT, \
      .mode = RTOS_ADC_MODE_SINGLE, \
      .trigger = RTOS_ADC_TRIGGER_SOFTWARE, \
      .external_trigger_enable = false, \
      .continuous_mode = false, \
      .dma_enable = false, \
      .scan_mode = false, \
      .nb_conversions = 1 }

#define RTOS_ADC_CHANNEL_CONFIG(ch, rank, sample_time, name) \
    { .channel = (ch), .rank = (rank), .sample_time = (sample_time), \
      .offset = 0, .gain = 1.0f, .reference_voltage = 3.3f, .channel_name = (name) }

/* 调试宏定义 */
#ifdef RTOS_ADC_DEBUG
#define RTOS_ADC_DEBUG_PRINT(fmt, ...) \
    printf("[ADC] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_ADC_DEBUG_PRINT(fmt, ...)
#endif

/* 错误检查宏定义 */
#ifdef RTOS_ADC_ERROR_CHECK
#define RTOS_ADC_CHECK_PARAM(param) \
    do { \
        if (!(param)) { \
            RTOS_ADC_DEBUG_PRINT("Parameter check failed: %s", #param); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
    
#define RTOS_ADC_CHECK_INIT() \
    do { \
        if (!rtos_adc_manager_get_instance()) { \
            RTOS_ADC_DEBUG_PRINT("ADC manager not initialized"); \
            return RTOS_ERROR_NOT_INITIALIZED; \
        } \
    } while(0)
#else
#define RTOS_ADC_CHECK_PARAM(param)
#define RTOS_ADC_CHECK_INIT()
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_ADC_ABSTRACTION_H__ */