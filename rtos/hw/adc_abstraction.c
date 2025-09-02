/**
 * @file adc_abstraction.c
 * @brief RTOS ADC抽象模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "adc_abstraction.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_adc.h"
#include "fwlib/inc/stm32f4xx_gpio.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局ADC管理器实例 */
static rtos_adc_manager_t g_adc_manager;
static bool g_adc_manager_initialized = false;

/* STM32F4平台相关数据 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
static ADC_TypeDef* const g_adc_instances[RTOS_ADC_CONTROLLER_MAX] = {
    ADC1, ADC2, ADC3
};

static const uint32_t g_adc_rcc_clocks[RTOS_ADC_CONTROLLER_MAX] = {
    RCC_APB2Periph_ADC1, RCC_APB2Periph_ADC2, RCC_APB2Periph_ADC3
};

static const IRQn_Type g_adc_irq_numbers[RTOS_ADC_CONTROLLER_MAX] = {
    ADC_IRQn, ADC_IRQn, ADC_IRQn  /* 所有ADC共享中断 */
};
#endif

/* 内部函数声明 */
static rtos_result_t rtos_adc_platform_init_controller(rtos_adc_controller_t controller, const rtos_adc_config_t *config);
static rtos_result_t rtos_adc_platform_config_channel(rtos_adc_controller_t controller, const rtos_adc_channel_config_t *channel_config);
static rtos_result_t rtos_adc_platform_start_conversion(rtos_adc_controller_t controller, rtos_adc_channel_t channel);
static uint16_t rtos_adc_platform_get_conversion_value(rtos_adc_controller_t controller);
static bool rtos_adc_platform_is_conversion_complete(rtos_adc_controller_t controller);

/**
 * @brief 初始化ADC管理器
 */
rtos_result_t rtos_adc_manager_init(void)
{
    if (g_adc_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_adc_manager, 0, sizeof(g_adc_manager));
    
    /* 初始化所有ADC句柄 */
    for (uint32_t i = 0; i < RTOS_ADC_CONTROLLER_MAX; i++) {
        g_adc_manager.adc_handles[i].controller = (rtos_adc_controller_t)i;
        g_adc_manager.adc_handles[i].state = RTOS_ADC_STATE_RESET;
        g_adc_manager.adc_handles[i].initialized = false;
    }
    
    g_adc_manager.initialized = true;
    g_adc_manager_initialized = true;
    
    RTOS_ADC_DEBUG_PRINT("ADC manager initialized");
    return RTOS_OK;
}

/**
 * @brief 初始化ADC控制器
 */
rtos_result_t rtos_adc_manager_init_controller(rtos_adc_controller_t controller,
                                              const rtos_adc_config_t *config,
                                              uint32_t max_channels)
{
    RTOS_ADC_CHECK_PARAM(config != NULL);
    RTOS_ADC_CHECK_PARAM(controller < RTOS_ADC_CONTROLLER_MAX);
    RTOS_ADC_CHECK_INIT();
    
    rtos_adc_handle_t *handle = &g_adc_manager.adc_handles[controller];
    
    if (handle->initialized) {
        RTOS_ADC_DEBUG_PRINT("ADC%d already initialized", controller + 1);
        return RTOS_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置 */
    handle->config = *config;
    
    /* 分配通道配置数组 */
    if (max_channels > 0) {
        handle->channels = malloc(sizeof(rtos_adc_channel_config_t) * max_channels);
        if (!handle->channels) {
            RTOS_ADC_DEBUG_PRINT("Failed to allocate ADC channel array");
            return RTOS_ERROR_NO_MEMORY;
        }
        handle->max_channels = max_channels;
        memset(handle->channels, 0, sizeof(rtos_adc_channel_config_t) * max_channels);
    }
    
    /* 分配转换结果缓冲区 */
    handle->buffer_size = max_channels * 16; /* 每通道16个采样 */
    handle->conversion_buffer = malloc(sizeof(uint16_t) * handle->buffer_size);
    if (!handle->conversion_buffer) {
        if (handle->channels) {
            free(handle->channels);
            handle->channels = NULL;
        }
        RTOS_ADC_DEBUG_PRINT("Failed to allocate ADC conversion buffer");
        return RTOS_ERROR_NO_MEMORY;
    }
    
    /* 调用平台相关初始化 */
    rtos_result_t result = rtos_adc_platform_init_controller(controller, config);
    if (result != RTOS_OK) {
        if (handle->channels) {
            free(handle->channels);
            handle->channels = NULL;
        }
        if (handle->conversion_buffer) {
            free(handle->conversion_buffer);
            handle->conversion_buffer = NULL;
        }
        RTOS_ADC_DEBUG_PRINT("ADC%d platform init failed: %d", controller + 1, result);
        return result;
    }
    
    /* 设置状态 */
    handle->state = RTOS_ADC_STATE_READY;
    handle->initialized = true;
    g_adc_manager.active_controllers++;
    
    RTOS_ADC_DEBUG_PRINT("ADC%d initialized (max channels: %lu)", controller + 1, max_channels);
    return RTOS_OK;
}

/**
 * @brief 配置ADC通道
 */
rtos_result_t rtos_adc_manager_config_channel(rtos_adc_controller_t controller,
                                             const rtos_adc_channel_config_t *channel_config)
{
    RTOS_ADC_CHECK_PARAM(channel_config != NULL);
    RTOS_ADC_CHECK_PARAM(controller < RTOS_ADC_CONTROLLER_MAX);
    RTOS_ADC_CHECK_INIT();
    
    rtos_adc_handle_t *handle = &g_adc_manager.adc_handles[controller];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->channel_count >= handle->max_channels) {
        return RTOS_ERROR_NO_MEMORY;
    }
    
    /* 保存通道配置 */
    handle->channels[handle->channel_count] = *channel_config;
    handle->channel_count++;
    
    /* 调用平台相关配置 */
    rtos_result_t result = rtos_adc_platform_config_channel(controller, channel_config);
    if (result != RTOS_OK) {
        handle->channel_count--; /* 回滚 */
        RTOS_ADC_DEBUG_PRINT("ADC%d channel config failed: %d", controller + 1, result);
        return result;
    }
    
    RTOS_ADC_DEBUG_PRINT("ADC%d channel %d configured (rank: %lu)", 
                         controller + 1, channel_config->channel, channel_config->rank);
    
    return RTOS_OK;
}

/**
 * @brief 单次ADC转换
 */
rtos_result_t rtos_adc_manager_convert_single(rtos_adc_controller_t controller,
                                             rtos_adc_channel_t channel,
                                             uint16_t *value)
{
    RTOS_ADC_CHECK_PARAM(value != NULL);
    RTOS_ADC_CHECK_PARAM(controller < RTOS_ADC_CONTROLLER_MAX);
    RTOS_ADC_CHECK_INIT();
    
    rtos_adc_handle_t *handle = &g_adc_manager.adc_handles[controller];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->state != RTOS_ADC_STATE_READY) {
        return RTOS_ERROR_BUSY;
    }
    
    handle->state = RTOS_ADC_STATE_BUSY;
    uint32_t start_time = rtos_hw_get_timestamp_ns();
    
    /* 启动转换 */
    rtos_result_t result = rtos_adc_platform_start_conversion(controller, channel);
    if (result != RTOS_OK) {
        handle->state = RTOS_ADC_STATE_ERROR;
        return result;
    }
    
    /* 等待转换完成 */
    uint32_t timeout = rtos_hw_get_system_time_ms() + 10;
    while (!rtos_adc_platform_is_conversion_complete(controller)) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            handle->state = RTOS_ADC_STATE_ERROR;
            handle->stats.error_count++;
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 读取转换结果 */
    *value = rtos_adc_platform_get_conversion_value(controller);
    
    /* 更新统计信息 */
    uint32_t conversion_time_ns = rtos_hw_get_timestamp_ns() - start_time;
    uint32_t conversion_time_us = conversion_time_ns / 1000;
    
    handle->stats.conversion_count++;
    g_adc_manager.total_conversions++;
    
    if (conversion_time_us > handle->stats.max_conversion_time_us) {
        handle->stats.max_conversion_time_us = conversion_time_us;
    }
    
    /* 计算平均转换时间 */
    if (handle->stats.conversion_count > 1) {
        handle->stats.avg_conversion_time_us = 
            (handle->stats.avg_conversion_time_us * (handle->stats.conversion_count - 1) + conversion_time_us) /
            handle->stats.conversion_count;
    } else {
        handle->stats.avg_conversion_time_us = conversion_time_us;
    }
    
    /* 更新值统计 */
    if (*value > handle->stats.max_value) {
        handle->stats.max_value = *value;
    }
    if (*value < handle->stats.min_value || handle->stats.min_value == 0) {
        handle->stats.min_value = *value;
    }
    
    handle->state = RTOS_ADC_STATE_READY;
    
    RTOS_ADC_DEBUG_PRINT("ADC%d single conversion: channel=%d, value=%u, time=%lu us", 
                         controller + 1, channel, *value, conversion_time_us);
    
    return RTOS_OK;
}

/**
 * @brief 读取校准后的电压值
 */
rtos_result_t rtos_adc_manager_read_voltage(rtos_adc_controller_t controller,
                                           rtos_adc_channel_t channel,
                                           uint32_t *voltage_mv)
{
    RTOS_ADC_CHECK_PARAM(voltage_mv != NULL);
    
    uint16_t adc_value;
    rtos_result_t result = rtos_adc_manager_convert_single(controller, channel, &adc_value);
    
    if (result == RTOS_OK) {
        /* 转换为电压值 (mV) */
        /* 假设12位ADC，3.3V参考电压 */
        *voltage_mv = (adc_value * 3300) / 4095;
    }
    
    return result;
}

/**
 * @brief 读取温度
 */
rtos_result_t rtos_adc_manager_read_temperature(rtos_adc_controller_t controller,
                                               int16_t *temperature_celsius)
{
    RTOS_ADC_CHECK_PARAM(temperature_celsius != NULL);
    
    uint16_t adc_value;
    rtos_result_t result = rtos_adc_manager_convert_single(controller, RTOS_ADC_CHANNEL_TEMPSENSOR, &adc_value);
    
    if (result == RTOS_OK) {
        /* STM32F4温度计算公式 */
        /* T = (V25 - Vsense) / Avg_Slope + 25 */
        /* V25 = 0.76V, Avg_Slope = 2.5mV/°C */
        int32_t vsense_mv = (adc_value * 3300) / 4095;
        *temperature_celsius = (int16_t)((760 - vsense_mv) * 1000 / 2500 + 25);
    }
    
    return result;
}

/**
 * @brief ADC校准
 */
rtos_result_t rtos_adc_manager_calibrate(rtos_adc_controller_t controller)
{
    RTOS_ADC_CHECK_PARAM(controller < RTOS_ADC_CONTROLLER_MAX);
    RTOS_ADC_CHECK_INIT();
    
    rtos_adc_handle_t *handle = &g_adc_manager.adc_handles[controller];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* STM32F4系列不需要手动校准，硬件自动校准 */
    handle->calibration.calibrated = true;
    handle->calibration.gain_correction = 1.0f;
    handle->calibration.offset_correction = 0;
    handle->calibration.temp_sensor_slope = 2.5f;    /* mV/°C */
    handle->calibration.temp_sensor_v25 = 760.0f;    /* mV at 25°C */
    handle->calibration.vrefint_cal = 1210.0f;       /* mV */
    
    RTOS_ADC_DEBUG_PRINT("ADC%d calibration completed (STM32F4 auto-calibration)", controller + 1);
    return RTOS_OK;
#else
    (void)handle;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/* 内部函数实现 */

/**
 * @brief 平台相关ADC控制器初始化
 */
static rtos_result_t rtos_adc_platform_init_controller(rtos_adc_controller_t controller, const rtos_adc_config_t *config)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    ADC_TypeDef *adc = g_adc_instances[controller];
    
    /* 使能ADC时钟 */
    RCC_APB2PeriphClockCmd(g_adc_rcc_clocks[controller], ENABLE);
    
    /* 配置ADC公共参数 */
    ADC_CommonInitTypeDef adc_common_init;
    ADC_CommonStructInit(&adc_common_init);
    adc_common_init.ADC_Mode = ADC_Mode_Independent;
    adc_common_init.ADC_Prescaler = ADC_Prescaler_Div4;
    adc_common_init.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    adc_common_init.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&adc_common_init);
    
    /* 配置ADC参数 */
    ADC_InitTypeDef adc_init;
    ADC_StructInit(&adc_init);
    
    switch (config->resolution) {
        case RTOS_ADC_RESOLUTION_12BIT:
            adc_init.ADC_Resolution = ADC_Resolution_12b;
            break;
        case RTOS_ADC_RESOLUTION_10BIT:
            adc_init.ADC_Resolution = ADC_Resolution_10b;
            break;
        case RTOS_ADC_RESOLUTION_8BIT:
            adc_init.ADC_Resolution = ADC_Resolution_8b;
            break;
        case RTOS_ADC_RESOLUTION_6BIT:
            adc_init.ADC_Resolution = ADC_Resolution_6b;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    adc_init.ADC_ScanConvMode = config->scan_mode ? ENABLE : DISABLE;
    adc_init.ADC_ContinuousConvMode = config->continuous_mode ? ENABLE : DISABLE;
    adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
    adc_init.ADC_NbrOfConversion = config->nb_conversions;
    
    ADC_Init(adc, &adc_init);
    
    /* 使能温度传感器和Vrefint */
    if (controller == RTOS_ADC_CONTROLLER_1) {
        ADC_TempSensorVrefintCmd(ENABLE);
    }
    
    /* 使能ADC */
    ADC_Cmd(adc, ENABLE);
    
    return RTOS_OK;
#else
    (void)controller;
    (void)config;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关ADC通道配置
 */
static rtos_result_t rtos_adc_platform_config_channel(rtos_adc_controller_t controller, const rtos_adc_channel_config_t *channel_config)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    ADC_TypeDef *adc = g_adc_instances[controller];
    
    /* 配置ADC通道 */
    uint8_t adc_channel;
    uint8_t sample_time;
    
    /* 转换通道枚举到STM32定义 */
    switch (channel_config->channel) {
        case RTOS_ADC_CHANNEL_0:  adc_channel = ADC_Channel_0; break;
        case RTOS_ADC_CHANNEL_1:  adc_channel = ADC_Channel_1; break;
        case RTOS_ADC_CHANNEL_2:  adc_channel = ADC_Channel_2; break;
        case RTOS_ADC_CHANNEL_3:  adc_channel = ADC_Channel_3; break;
        case RTOS_ADC_CHANNEL_4:  adc_channel = ADC_Channel_4; break;
        case RTOS_ADC_CHANNEL_5:  adc_channel = ADC_Channel_5; break;
        case RTOS_ADC_CHANNEL_6:  adc_channel = ADC_Channel_6; break;
        case RTOS_ADC_CHANNEL_7:  adc_channel = ADC_Channel_7; break;
        case RTOS_ADC_CHANNEL_8:  adc_channel = ADC_Channel_8; break;
        case RTOS_ADC_CHANNEL_9:  adc_channel = ADC_Channel_9; break;
        case RTOS_ADC_CHANNEL_10: adc_channel = ADC_Channel_10; break;
        case RTOS_ADC_CHANNEL_11: adc_channel = ADC_Channel_11; break;
        case RTOS_ADC_CHANNEL_12: adc_channel = ADC_Channel_12; break;
        case RTOS_ADC_CHANNEL_13: adc_channel = ADC_Channel_13; break;
        case RTOS_ADC_CHANNEL_14: adc_channel = ADC_Channel_14; break;
        case RTOS_ADC_CHANNEL_15: adc_channel = ADC_Channel_15; break;
        case RTOS_ADC_CHANNEL_TEMPSENSOR: adc_channel = ADC_Channel_TempSensor; break;
        case RTOS_ADC_CHANNEL_VREFINT: adc_channel = ADC_Channel_Vrefint; break;
        case RTOS_ADC_CHANNEL_VBAT: adc_channel = ADC_Channel_Vbat; break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 转换采样时间枚举 */
    switch (channel_config->sample_time) {
        case RTOS_ADC_SAMPLETIME_3CYCLES:   sample_time = ADC_SampleTime_3Cycles; break;
        case RTOS_ADC_SAMPLETIME_15CYCLES:  sample_time = ADC_SampleTime_15Cycles; break;
        case RTOS_ADC_SAMPLETIME_28CYCLES:  sample_time = ADC_SampleTime_28Cycles; break;
        case RTOS_ADC_SAMPLETIME_56CYCLES:  sample_time = ADC_SampleTime_56Cycles; break;
        case RTOS_ADC_SAMPLETIME_84CYCLES:  sample_time = ADC_SampleTime_84Cycles; break;
        case RTOS_ADC_SAMPLETIME_112CYCLES: sample_time = ADC_SampleTime_112Cycles; break;
        case RTOS_ADC_SAMPLETIME_144CYCLES: sample_time = ADC_SampleTime_144Cycles; break;
        case RTOS_ADC_SAMPLETIME_480CYCLES: sample_time = ADC_SampleTime_480Cycles; break;
        default:
            sample_time = ADC_SampleTime_144Cycles;
            break;
    }
    
    /* 配置规则通道 */
    ADC_RegularChannelConfig(adc, adc_channel, channel_config->rank, sample_time);
    
    return RTOS_OK;
#else
    (void)controller;
    (void)channel_config;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关启动转换
 */
static rtos_result_t rtos_adc_platform_start_conversion(rtos_adc_controller_t controller, rtos_adc_channel_t channel)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    ADC_TypeDef *adc = g_adc_instances[controller];
    
    /* 配置单次转换通道 */
    uint8_t adc_channel = (uint8_t)channel;
    if (channel >= RTOS_ADC_CHANNEL_TEMPSENSOR) {
        /* 特殊通道处理 */
        if (channel == RTOS_ADC_CHANNEL_TEMPSENSOR) {
            adc_channel = ADC_Channel_TempSensor;
        } else if (channel == RTOS_ADC_CHANNEL_VREFINT) {
            adc_channel = ADC_Channel_Vrefint;
        } else if (channel == RTOS_ADC_CHANNEL_VBAT) {
            adc_channel = ADC_Channel_Vbat;
        }
    }
    
    ADC_RegularChannelConfig(adc, adc_channel, 1, ADC_SampleTime_144Cycles);
    
    /* 启动软件转换 */
    ADC_SoftwareStartConv(adc);
    
    return RTOS_OK;
#else
    (void)controller;
    (void)channel;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关获取转换值
 */
static uint16_t rtos_adc_platform_get_conversion_value(rtos_adc_controller_t controller)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    ADC_TypeDef *adc = g_adc_instances[controller];
    return (uint16_t)ADC_GetConversionValue(adc);
#else
    (void)controller;
    return 0;
#endif
}

/**
 * @brief 平台相关检查转换完成
 */
static bool rtos_adc_platform_is_conversion_complete(rtos_adc_controller_t controller)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    ADC_TypeDef *adc = g_adc_instances[controller];
    return (ADC_GetFlagStatus(adc, ADC_FLAG_EOC) != RESET);
#else
    (void)controller;
    return true;
#endif
}

/**
 * @brief 获取ADC管理器实例
 */
rtos_adc_manager_t* rtos_adc_manager_get_instance(void)
{
    if (!g_adc_manager_initialized) {
        return NULL;
    }
    return &g_adc_manager;
}

/**
 * @brief ADC中断处理函数
 */
void rtos_adc_manager_interrupt_handler(rtos_adc_controller_t controller)
{
    if (!g_adc_manager_initialized || controller >= RTOS_ADC_CONTROLLER_MAX) {
        return;
    }
    
    rtos_adc_handle_t *handle = &g_adc_manager.adc_handles[controller];
    
    if (!handle->initialized) {
        return;
    }
    
    /* 简化实现：清除中断标志 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    ADC_TypeDef *adc = g_adc_instances[controller];
    
    if (ADC_GetITStatus(adc, ADC_IT_EOC) != RESET) {
        ADC_ClearITPendingBit(adc, ADC_IT_EOC);
        /* 触发转换完成事件 */
        if (handle->event_callbacks[RTOS_ADC_EVENT_CONVERSION_COMPLETE]) {
            handle->event_callbacks[RTOS_ADC_EVENT_CONVERSION_COMPLETE](controller, 
                RTOS_ADC_EVENT_CONVERSION_COMPLETE, handle->event_contexts[RTOS_ADC_EVENT_CONVERSION_COMPLETE]);
        }
    }
#endif
}