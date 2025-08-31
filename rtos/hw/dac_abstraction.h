/**
 * @file dac_abstraction.h
 * @brief RTOS DAC抽象模块 - 面向对象的DAC管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_DAC_ABSTRACTION_H__
#define __RTOS_DAC_ABSTRACTION_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DAC通道定义 */
typedef enum {
    RTOS_DAC_CHANNEL_1 = 0,
    RTOS_DAC_CHANNEL_2,
    RTOS_DAC_CHANNEL_MAX
} rtos_dac_channel_t;

/* DAC触发源定义 */
typedef enum {
    RTOS_DAC_TRIGGER_NONE = 0,          /**< 无触发 */
    RTOS_DAC_TRIGGER_SOFTWARE,          /**< 软件触发 */
    RTOS_DAC_TRIGGER_TIMER,             /**< 定时器触发 */
    RTOS_DAC_TRIGGER_EXTERNAL,          /**< 外部触发 */
    RTOS_DAC_TRIGGER_MAX
} rtos_dac_trigger_t;

/* DAC输出缓冲定义 */
typedef enum {
    RTOS_DAC_BUFFER_DISABLE = 0,        /**< 禁用输出缓冲 */
    RTOS_DAC_BUFFER_ENABLE,             /**< 使能输出缓冲 */
    RTOS_DAC_BUFFER_MAX
} rtos_dac_buffer_t;

/* DAC波形类型定义 */
typedef enum {
    RTOS_DAC_WAVE_NONE = 0,             /**< 无波形 */
    RTOS_DAC_WAVE_NOISE,                /**< 噪声波形 */
    RTOS_DAC_WAVE_TRIANGLE,             /**< 三角波 */
    RTOS_DAC_WAVE_SAWTOOTH,             /**< 锯齿波 */
    RTOS_DAC_WAVE_SINE,                 /**< 正弦波 */
    RTOS_DAC_WAVE_SQUARE,               /**< 方波 */
    RTOS_DAC_WAVE_MAX
} rtos_dac_wave_t;

/* DAC状态定义 */
typedef enum {
    RTOS_DAC_STATE_RESET = 0,           /**< 复位状态 */
    RTOS_DAC_STATE_READY,               /**< 就绪状态 */
    RTOS_DAC_STATE_BUSY,                /**< 输出中 */
    RTOS_DAC_STATE_ERROR,               /**< 错误状态 */
    RTOS_DAC_STATE_MAX
} rtos_dac_state_t;

/* DAC配置结构 */
typedef struct {
    rtos_dac_trigger_t trigger;         /**< 触发源 */
    rtos_dac_buffer_t output_buffer;    /**< 输出缓冲 */
    rtos_dac_wave_t wave_generation;    /**< 波形生成 */
    uint32_t wave_amplitude;            /**< 波形幅度 */
    bool dma_enable;                    /**< DMA使能 */
    uint32_t sample_rate_hz;            /**< 采样率 */
    float reference_voltage;            /**< 参考电压 */
    uint32_t timeout_ms;                /**< 超时时间 */
} rtos_dac_config_t;

/* DAC波形参数 */
typedef struct {
    rtos_dac_wave_t wave_type;          /**< 波形类型 */
    uint32_t frequency_hz;              /**< 频率 */
    uint16_t amplitude;                 /**< 幅度 */
    uint16_t offset;                    /**< 偏移 */
    uint16_t phase;                     /**< 相位 */
    uint32_t sample_count;              /**< 采样点数 */
    uint16_t *wave_data;                /**< 波形数据 */
} rtos_dac_wave_params_t;

/* DAC事件类型定义 */
typedef enum {
    RTOS_DAC_EVENT_CONVERSION_COMPLETE = 0, /**< 转换完成 */
    RTOS_DAC_EVENT_HALF_CONVERSION,         /**< 半转换完成 */
    RTOS_DAC_EVENT_ERROR,                   /**< 错误事件 */
    RTOS_DAC_EVENT_UNDERRUN,                /**< 欠载事件 */
    RTOS_DAC_EVENT_MAX
} rtos_dac_event_t;

/* DAC事件回调函数类型 */
typedef void (*rtos_dac_event_callback_t)(rtos_dac_channel_t channel, 
                                          rtos_dac_event_t event, 
                                          void *context);

/* DAC统计信息 */
typedef struct {
    uint32_t output_count;              /**< 输出次数 */
    uint32_t wave_cycles;               /**< 波形周期数 */
    uint32_t error_count;               /**< 错误次数 */
    uint32_t underrun_count;            /**< 欠载次数 */
    uint32_t max_frequency_hz;          /**< 最大频率 */
    uint32_t total_samples;             /**< 总采样数 */
    uint16_t min_output_value;          /**< 最小输出值 */
    uint16_t max_output_value;          /**< 最大输出值 */
} rtos_dac_stats_t;

/* DAC句柄结构 */
typedef struct {
    rtos_dac_channel_t channel;         /**< DAC通道 */
    rtos_dac_config_t config;           /**< DAC配置 */
    rtos_dac_state_t state;             /**< DAC状态 */
    rtos_dac_stats_t stats;             /**< 统计信息 */
    
    /* 波形生成 */
    rtos_dac_wave_params_t wave_params; /**< 波形参数 */
    uint32_t wave_position;             /**< 当前波形位置 */
    
    /* 事件回调 */
    rtos_dac_event_callback_t event_callbacks[RTOS_DAC_EVENT_MAX];
    void *event_contexts[RTOS_DAC_EVENT_MAX];
    
    /* DMA句柄 */
    void *dma_handle;                   /**< DMA句柄 */
    
    /* 时间戳 */
    uint32_t output_start_time;         /**< 输出开始时间 */
    
    bool initialized;
} rtos_dac_handle_t;

/* DAC管理器类结构 */
typedef struct {
    /* DAC句柄数组 */
    rtos_dac_handle_t dac_handles[RTOS_DAC_CHANNEL_MAX];
    
    /* 全局统计 */
    uint32_t total_outputs;
    uint32_t total_samples;
    uint32_t total_errors;
    uint32_t active_channels;
    
    /* 状态标志 */
    bool initialized;
    
} rtos_dac_manager_t;

/**
 * @brief 初始化DAC管理器
 * @return 操作结果
 */
rtos_result_t rtos_dac_manager_init(void);

/**
 * @brief 初始化DAC通道
 * @param channel DAC通道
 * @param config DAC配置
 * @return 操作结果
 */
rtos_result_t rtos_dac_manager_init_channel(rtos_dac_channel_t channel,
                                           const rtos_dac_config_t *config);

/**
 * @brief 设置DAC输出值
 * @param channel DAC通道
 * @param value 输出值
 * @return 操作结果
 */
rtos_result_t rtos_dac_manager_set_value(rtos_dac_channel_t channel, uint16_t value);

/**
 * @brief 设置DAC输出电压
 * @param channel DAC通道
 * @param voltage_mv 输出电压(mV)
 * @return 操作结果
 */
rtos_result_t rtos_dac_manager_set_voltage(rtos_dac_channel_t channel, uint32_t voltage_mv);

/**
 * @brief 生成DAC波形
 * @param channel DAC通道
 * @param wave_params 波形参数
 * @return 操作结果
 */
rtos_result_t rtos_dac_manager_generate_wave(rtos_dac_channel_t channel,
                                            const rtos_dac_wave_params_t *wave_params);

/**
 * @brief 停止DAC波形生成
 * @param channel DAC通道
 * @return 操作结果
 */
rtos_result_t rtos_dac_manager_stop_wave(rtos_dac_channel_t channel);

/**
 * @brief DAC中断处理函数
 * @param channel DAC通道
 */
void rtos_dac_manager_interrupt_handler(rtos_dac_channel_t channel);

/* 便利宏定义 */
#define RTOS_DAC_DEFAULT_CONFIG() \
    { .trigger = RTOS_DAC_TRIGGER_SOFTWARE, \
      .output_buffer = RTOS_DAC_BUFFER_ENABLE, \
      .wave_generation = RTOS_DAC_WAVE_NONE, \
      .dma_enable = false, \
      .sample_rate_hz = 1000, \
      .reference_voltage = 3.3f, \
      .timeout_ms = 100 }

#define RTOS_DAC_VOLTAGE_TO_VALUE(voltage_mv, ref_voltage_mv) \
    ((uint16_t)(((voltage_mv) * 4095) / (ref_voltage_mv)))

#define RTOS_DAC_VALUE_TO_VOLTAGE(value, ref_voltage_mv) \
    (((value) * (ref_voltage_mv)) / 4095)

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_DAC_ABSTRACTION_H__ */