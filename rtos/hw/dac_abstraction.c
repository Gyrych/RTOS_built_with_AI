/**
 * @file dac_abstraction.c
 * @brief RTOS DAC抽象模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "dac_abstraction.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_dac.h"
#include "fwlib/inc/stm32f4xx_gpio.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局DAC管理器实例 */
static rtos_dac_manager_t g_dac_manager;
static bool g_dac_manager_initialized = false;

/* 内部函数声明 */
static rtos_result_t rtos_dac_platform_init_channel(rtos_dac_channel_t channel, const rtos_dac_config_t *config);
static rtos_result_t rtos_dac_platform_set_value(rtos_dac_channel_t channel, uint16_t value);
static rtos_result_t rtos_dac_platform_start_wave_generation(rtos_dac_channel_t channel, const rtos_dac_wave_params_t *wave_params);
static rtos_result_t rtos_dac_platform_stop_wave_generation(rtos_dac_channel_t channel);

/**
 * @brief 初始化DAC管理器
 */
rtos_result_t rtos_dac_manager_init(void)
{
    if (g_dac_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_dac_manager, 0, sizeof(g_dac_manager));
    
    /* 初始化所有DAC句柄 */
    for (uint32_t i = 0; i < RTOS_DAC_CHANNEL_MAX; i++) {
        g_dac_manager.dac_handles[i].channel = (rtos_dac_channel_t)i;
        g_dac_manager.dac_handles[i].state = RTOS_DAC_STATE_RESET;
        g_dac_manager.dac_handles[i].initialized = false;
    }
    
    g_dac_manager.initialized = true;
    g_dac_manager_initialized = true;
    
    RTOS_DAC_DEBUG_PRINT("DAC manager initialized");
    return RTOS_OK;
}

/**
 * @brief 初始化DAC通道
 */
rtos_result_t rtos_dac_manager_init_channel(rtos_dac_channel_t channel,
                                           const rtos_dac_config_t *config)
{
    RTOS_DAC_CHECK_PARAM(config != NULL);
    RTOS_DAC_CHECK_PARAM(channel < RTOS_DAC_CHANNEL_MAX);
    RTOS_DAC_CHECK_INIT();
    
    rtos_dac_handle_t *handle = &g_dac_manager.dac_handles[channel];
    
    if (handle->initialized) {
        RTOS_DAC_DEBUG_PRINT("DAC channel %d already initialized", channel + 1);
        return RTOS_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置 */
    handle->config = *config;
    
    /* 调用平台相关初始化 */
    rtos_result_t result = rtos_dac_platform_init_channel(channel, config);
    if (result != RTOS_OK) {
        RTOS_DAC_DEBUG_PRINT("DAC channel %d platform init failed: %d", channel + 1, result);
        return result;
    }
    
    /* 设置状态 */
    handle->state = RTOS_DAC_STATE_READY;
    handle->initialized = true;
    g_dac_manager.active_channels++;
    
    RTOS_DAC_DEBUG_PRINT("DAC channel %d initialized", channel + 1);
    return RTOS_OK;
}

/**
 * @brief 设置DAC输出值
 */
rtos_result_t rtos_dac_manager_set_value(rtos_dac_channel_t channel, uint16_t value)
{
    RTOS_DAC_CHECK_PARAM(channel < RTOS_DAC_CHANNEL_MAX);
    RTOS_DAC_CHECK_INIT();
    
    rtos_dac_handle_t *handle = &g_dac_manager.dac_handles[channel];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    rtos_result_t result = rtos_dac_platform_set_value(channel, value);
    
    if (result == RTOS_OK) {
        handle->stats.output_count++;
        g_dac_manager.total_outputs++;
        
        /* 更新值统计 */
        if (value > handle->stats.max_output_value) {
            handle->stats.max_output_value = value;
        }
        if (value < handle->stats.min_output_value || handle->stats.min_output_value == 0) {
            handle->stats.min_output_value = value;
        }
        
        RTOS_DAC_DEBUG_PRINT("DAC channel %d set value: %u", channel + 1, value);
    }
    
    return result;
}

/**
 * @brief 设置DAC输出电压
 */
rtos_result_t rtos_dac_manager_set_voltage(rtos_dac_channel_t channel, uint32_t voltage_mv)
{
    /* 转换电压为DAC值 */
    uint16_t dac_value = RTOS_DAC_VOLTAGE_TO_VALUE(voltage_mv, 3300);
    
    RTOS_DAC_DEBUG_PRINT("DAC channel %d set voltage: %lu mV (value: %u)", 
                         channel + 1, voltage_mv, dac_value);
    
    return rtos_dac_manager_set_value(channel, dac_value);
}

/**
 * @brief 生成DAC波形
 */
rtos_result_t rtos_dac_manager_generate_wave(rtos_dac_channel_t channel,
                                            const rtos_dac_wave_params_t *wave_params)
{
    RTOS_DAC_CHECK_PARAM(wave_params != NULL);
    RTOS_DAC_CHECK_PARAM(channel < RTOS_DAC_CHANNEL_MAX);
    RTOS_DAC_CHECK_INIT();
    
    rtos_dac_handle_t *handle = &g_dac_manager.dac_handles[channel];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->state == RTOS_DAC_STATE_BUSY) {
        return RTOS_ERROR_BUSY;
    }
    
    /* 保存波形参数 */
    handle->wave_params = *wave_params;
    handle->wave_position = 0;
    handle->state = RTOS_DAC_STATE_BUSY;
    
    /* 调用平台相关波形生成 */
    rtos_result_t result = rtos_dac_platform_start_wave_generation(channel, wave_params);
    
    if (result == RTOS_OK) {
        handle->stats.wave_cycles++;
        RTOS_DAC_DEBUG_PRINT("DAC channel %d wave generation started: type=%d, freq=%lu Hz", 
                             channel + 1, wave_params->wave_type, wave_params->frequency_hz);
    } else {
        handle->state = RTOS_DAC_STATE_ERROR;
        handle->stats.error_count++;
    }
    
    return result;
}

/**
 * @brief 停止DAC波形生成
 */
rtos_result_t rtos_dac_manager_stop_wave(rtos_dac_channel_t channel)
{
    RTOS_DAC_CHECK_PARAM(channel < RTOS_DAC_CHANNEL_MAX);
    RTOS_DAC_CHECK_INIT();
    
    rtos_dac_handle_t *handle = &g_dac_manager.dac_handles[channel];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    rtos_result_t result = rtos_dac_platform_stop_wave_generation(channel);
    
    if (result == RTOS_OK) {
        handle->state = RTOS_DAC_STATE_READY;
        RTOS_DAC_DEBUG_PRINT("DAC channel %d wave generation stopped", channel + 1);
    }
    
    return result;
}

/* 内部函数实现 */

/**
 * @brief 平台相关DAC通道初始化
 */
static rtos_result_t rtos_dac_platform_init_channel(rtos_dac_channel_t channel, const rtos_dac_config_t *config)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 使能DAC时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    
    /* 配置DAC通道 */
    DAC_InitTypeDef dac_init;
    DAC_StructInit(&dac_init);
    
    /* 设置触发源 */
    switch (config->trigger) {
        case RTOS_DAC_TRIGGER_NONE:
            dac_init.DAC_Trigger = DAC_Trigger_None;
            break;
        case RTOS_DAC_TRIGGER_SOFTWARE:
            dac_init.DAC_Trigger = DAC_Trigger_Software;
            break;
        case RTOS_DAC_TRIGGER_TIMER:
            dac_init.DAC_Trigger = DAC_Trigger_T6_TRGO; /* 使用TIM6触发 */
            break;
        default:
            dac_init.DAC_Trigger = DAC_Trigger_None;
            break;
    }
    
    /* 设置波形生成 */
    switch (config->wave_generation) {
        case RTOS_DAC_WAVE_NONE:
            dac_init.DAC_WaveGeneration = DAC_WaveGeneration_None;
            break;
        case RTOS_DAC_WAVE_NOISE:
            dac_init.DAC_WaveGeneration = DAC_WaveGeneration_Noise;
            dac_init.DAC_LFSRUnmask_TriangleAmplitude = config->wave_amplitude;
            break;
        case RTOS_DAC_WAVE_TRIANGLE:
            dac_init.DAC_WaveGeneration = DAC_WaveGeneration_Triangle;
            dac_init.DAC_LFSRUnmask_TriangleAmplitude = config->wave_amplitude;
            break;
        default:
            dac_init.DAC_WaveGeneration = DAC_WaveGeneration_None;
            break;
    }
    
    /* 设置输出缓冲 */
    dac_init.DAC_OutputBuffer = (config->output_buffer == RTOS_DAC_BUFFER_ENABLE) ? 
                               DAC_OutputBuffer_Enable : DAC_OutputBuffer_Disable;
    
    /* 初始化DAC通道 */
    if (channel == RTOS_DAC_CHANNEL_1) {
        DAC_Init(DAC_Channel_1, &dac_init);
        DAC_Cmd(DAC_Channel_1, ENABLE);
    } else {
        DAC_Init(DAC_Channel_2, &dac_init);
        DAC_Cmd(DAC_Channel_2, ENABLE);
    }
    
    return RTOS_OK;
#else
    (void)channel;
    (void)config;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关DAC设置值
 */
static rtos_result_t rtos_dac_platform_set_value(rtos_dac_channel_t channel, uint16_t value)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    if (channel == RTOS_DAC_CHANNEL_1) {
        DAC_SetChannel1Data(DAC_Align_12b_R, value);
        DAC_SoftwareTriggerCmd(DAC_Channel_1, ENABLE);
    } else {
        DAC_SetChannel2Data(DAC_Align_12b_R, value);
        DAC_SoftwareTriggerCmd(DAC_Channel_2, ENABLE);
    }
    
    return RTOS_OK;
#else
    (void)channel;
    (void)value;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关启动波形生成
 */
static rtos_result_t rtos_dac_platform_start_wave_generation(rtos_dac_channel_t channel, const rtos_dac_wave_params_t *wave_params)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 简化实现：设置基础波形参数 */
    if (wave_params->wave_type == RTOS_DAC_WAVE_TRIANGLE) {
        /* 配置三角波 */
        DAC_InitTypeDef dac_init;
        DAC_StructInit(&dac_init);
        dac_init.DAC_Trigger = DAC_Trigger_T6_TRGO;
        dac_init.DAC_WaveGeneration = DAC_WaveGeneration_Triangle;
        dac_init.DAC_LFSRUnmask_TriangleAmplitude = wave_params->amplitude;
        dac_init.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
        
        if (channel == RTOS_DAC_CHANNEL_1) {
            DAC_Init(DAC_Channel_1, &dac_init);
        } else {
            DAC_Init(DAC_Channel_2, &dac_init);
        }
    }
    
    return RTOS_OK;
#else
    (void)channel;
    (void)wave_params;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关停止波形生成
 */
static rtos_result_t rtos_dac_platform_stop_wave_generation(rtos_dac_channel_t channel)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 重新配置为无波形生成 */
    DAC_InitTypeDef dac_init;
    DAC_StructInit(&dac_init);
    dac_init.DAC_Trigger = DAC_Trigger_None;
    dac_init.DAC_WaveGeneration = DAC_WaveGeneration_None;
    dac_init.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    
    if (channel == RTOS_DAC_CHANNEL_1) {
        DAC_Init(DAC_Channel_1, &dac_init);
    } else {
        DAC_Init(DAC_Channel_2, &dac_init);
    }
    
    return RTOS_OK;
#else
    (void)channel;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 获取DAC管理器实例
 */
rtos_dac_manager_t* rtos_dac_manager_get_instance(void)
{
    if (!g_dac_manager_initialized) {
        return NULL;
    }
    return &g_dac_manager;
}

/**
 * @brief DAC中断处理函数
 */
void rtos_dac_manager_interrupt_handler(rtos_dac_channel_t channel)
{
    if (!g_dac_manager_initialized || channel >= RTOS_DAC_CHANNEL_MAX) {
        return;
    }
    
    rtos_dac_handle_t *handle = &g_dac_manager.dac_handles[channel];
    
    if (!handle->initialized) {
        return;
    }
    
    /* 简化实现：处理DAC中断 */
    handle->stats.output_count++;
    
    if (handle->event_callbacks[RTOS_DAC_EVENT_CONVERSION_COMPLETE]) {
        handle->event_callbacks[RTOS_DAC_EVENT_CONVERSION_COMPLETE](channel, 
            RTOS_DAC_EVENT_CONVERSION_COMPLETE, handle->event_contexts[RTOS_DAC_EVENT_CONVERSION_COMPLETE]);
    }
}