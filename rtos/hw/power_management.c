/**
 * @file power_management.c
 * @brief RTOS电源管理模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "power_management.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_pwr.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/stm32f4xx_adc.h"
#include "fwlib/inc/stm32f4xx_rtc.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局电源管理器实例 */
static rtos_power_manager_t g_power_manager;
static bool g_power_manager_initialized = false;

/* 内部函数声明 */
static rtos_result_t rtos_power_platform_init(void);
static rtos_result_t rtos_power_platform_set_mode(rtos_power_mode_t mode);
static rtos_result_t rtos_power_platform_configure_wakeup(uint32_t sources);
static rtos_result_t rtos_power_platform_read_voltage(uint32_t *vdd_mv, uint32_t *vbat_mv);
static rtos_result_t rtos_power_platform_read_temperature(int16_t *temp_celsius);
static void rtos_power_trigger_event(rtos_power_event_t event);
static void rtos_power_update_statistics(rtos_power_mode_t old_mode, rtos_power_mode_t new_mode);

/**
 * @brief 初始化电源管理器
 */
rtos_result_t rtos_power_manager_init(void)
{
    if (g_power_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_power_manager, 0, sizeof(g_power_manager));
    
    /* 设置默认状态 */
    g_power_manager.current_mode = RTOS_POWER_MODE_RUN;
    
    /* 设置默认策略 */
    g_power_manager.policy.auto_sleep_enable = false;
    g_power_manager.policy.idle_timeout_ms = 1000;
    g_power_manager.policy.deep_sleep_threshold_ms = 5000;
    g_power_manager.policy.max_sleep_mode = RTOS_POWER_MODE_STOP;
    g_power_manager.policy.wakeup_sources = RTOS_WAKEUP_SOURCE_ALL;
    g_power_manager.policy.voltage_scaling_enable = true;
    g_power_manager.policy.min_voltage_mv = 2700;
    
    /* 初始化平台相关功能 */
    rtos_result_t result = rtos_power_platform_init();
    if (result != RTOS_OK) {
        RTOS_POWER_DEBUG_PRINT("Platform initialization failed: %d", result);
        return result;
    }
    
    /* 读取初始状态 */
    rtos_power_manager_get_status(&g_power_manager.status);
    
    g_power_manager_initialized = true;
    
    RTOS_POWER_DEBUG_PRINT("Power manager initialized successfully");
    return RTOS_OK;
}

/**
 * @brief 反初始化电源管理器
 */
rtos_result_t rtos_power_manager_deinit(void)
{
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 退出低功耗模式 */
    if (g_power_manager.current_mode != RTOS_POWER_MODE_RUN) {
        rtos_power_manager_exit_low_power();
    }
    
    /* 清空回调函数 */
    for (int i = 0; i < RTOS_POWER_EVENT_MAX; i++) {
        g_power_manager.event_callbacks[i] = NULL;
        g_power_manager.event_contexts[i] = NULL;
    }
    
    g_power_manager_initialized = false;
    
    RTOS_POWER_DEBUG_PRINT("Power manager deinitialized");
    return RTOS_OK;
}

/**
 * @brief 获取电源管理器实例
 */
rtos_power_manager_t* rtos_power_manager_get_instance(void)
{
    if (!g_power_manager_initialized) {
        return NULL;
    }
    return &g_power_manager;
}

/**
 * @brief 设置电源模式
 */
rtos_result_t rtos_power_manager_set_mode(rtos_power_mode_t mode)
{
    RTOS_POWER_CHECK_PARAM(mode < RTOS_POWER_MODE_MAX);
    
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    rtos_power_mode_t old_mode = g_power_manager.current_mode;
    
    if (old_mode == mode) {
        return RTOS_OK; /* 模式未改变 */
    }
    
    /* 调用平台相关实现 */
    rtos_result_t result = rtos_power_platform_set_mode(mode);
    if (result != RTOS_OK) {
        RTOS_POWER_DEBUG_PRINT("Failed to set power mode %d: %d", mode, result);
        return result;
    }
    
    /* 更新状态 */
    g_power_manager.current_mode = mode;
    g_power_manager.status.current_mode = mode;
    
    /* 更新统计信息 */
    rtos_power_update_statistics(old_mode, mode);
    
    /* 触发模式改变事件 */
    rtos_power_trigger_event(RTOS_POWER_EVENT_MODE_CHANGED);
    
    RTOS_POWER_DEBUG_PRINT("Power mode changed: %d -> %d", old_mode, mode);
    return RTOS_OK;
}

/**
 * @brief 获取当前电源模式
 */
rtos_power_mode_t rtos_power_manager_get_current_mode(void)
{
    if (!g_power_manager_initialized) {
        return RTOS_POWER_MODE_RUN;
    }
    return g_power_manager.current_mode;
}

/**
 * @brief 进入低功耗模式
 */
rtos_result_t rtos_power_manager_enter_low_power(rtos_power_mode_t mode, uint32_t timeout_ms)
{
    RTOS_POWER_CHECK_PARAM(mode < RTOS_POWER_MODE_MAX);
    RTOS_POWER_CHECK_PARAM(RTOS_POWER_IS_LOW_POWER_MODE(mode));
    
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 检查策略限制 */
    if (mode > g_power_manager.policy.max_sleep_mode) {
        mode = g_power_manager.policy.max_sleep_mode;
        RTOS_POWER_DEBUG_PRINT("Power mode limited by policy: %d", mode);
    }
    
    /* 配置唤醒超时 */
    if (timeout_ms > 0) {
        /* 这里可以配置RTC唤醒 */
        RTOS_POWER_DEBUG_PRINT("Sleep timeout set: %lu ms", timeout_ms);
    }
    
    /* 记录进入睡眠的时间 */
    uint32_t enter_time = rtos_hw_get_system_time_ms();
    
    /* 设置电源模式 */
    rtos_result_t result = rtos_power_manager_set_mode(mode);
    if (result != RTOS_OK) {
        return result;
    }
    
    /* 等待唤醒 */
    /* 在实际硬件上，这里会进入低功耗状态，直到被唤醒 */
    
    /* 记录睡眠时间 */
    uint32_t wake_time = rtos_hw_get_system_time_ms();
    uint32_t sleep_duration = wake_time - enter_time;
    g_power_manager.total_sleep_time_ms += sleep_duration;
    g_power_manager.status.sleep_time_ms = sleep_duration;
    
    RTOS_POWER_DEBUG_PRINT("Woke up after %lu ms", sleep_duration);
    
    /* 触发唤醒事件 */
    rtos_power_trigger_event(RTOS_POWER_EVENT_WAKEUP);
    
    return RTOS_OK;
}

/**
 * @brief 退出低功耗模式
 */
rtos_result_t rtos_power_manager_exit_low_power(void)
{
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (g_power_manager.current_mode == RTOS_POWER_MODE_RUN) {
        return RTOS_OK; /* 已经在运行模式 */
    }
    
    return rtos_power_manager_set_mode(RTOS_POWER_MODE_RUN);
}

/**
 * @brief 配置唤醒源
 */
rtos_result_t rtos_power_manager_configure_wakeup_sources(uint32_t sources)
{
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    rtos_result_t result = rtos_power_platform_configure_wakeup(sources);
    if (result == RTOS_OK) {
        g_power_manager.policy.wakeup_sources = sources;
        RTOS_POWER_DEBUG_PRINT("Wakeup sources configured: 0x%08lx", sources);
    }
    
    return result;
}

/**
 * @brief 获取唤醒源
 */
uint32_t rtos_power_manager_get_wakeup_sources(void)
{
    if (!g_power_manager_initialized) {
        return 0;
    }
    return g_power_manager.policy.wakeup_sources;
}

/**
 * @brief 获取电源状态
 */
rtos_result_t rtos_power_manager_get_status(rtos_power_status_t *status)
{
    RTOS_POWER_CHECK_PARAM(status != NULL);
    
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 读取电压 */
    rtos_power_platform_read_voltage(&g_power_manager.status.vdd_voltage_mv, 
                                   &g_power_manager.status.vbat_voltage_mv);
    
    /* 读取温度 */
    rtos_power_platform_read_temperature(&g_power_manager.status.temperature_celsius);
    
    /* 更新其他状态信息 */
    g_power_manager.status.current_mode = g_power_manager.current_mode;
    g_power_manager.status.run_time_ms = rtos_hw_get_system_time_ms();
    
    /* 复制状态 */
    *status = g_power_manager.status;
    
    return RTOS_OK;
}

/**
 * @brief 设置电源策略
 */
rtos_result_t rtos_power_manager_set_policy(const rtos_power_policy_t *policy)
{
    RTOS_POWER_CHECK_PARAM(policy != NULL);
    
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 验证策略参数 */
    if (policy->max_sleep_mode >= RTOS_POWER_MODE_MAX) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (policy->min_voltage_mv < 1800 || policy->min_voltage_mv > 3600) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 复制策略 */
    g_power_manager.policy = *policy;
    
    RTOS_POWER_DEBUG_PRINT("Power policy updated");
    return RTOS_OK;
}

/**
 * @brief 获取电源策略
 */
rtos_result_t rtos_power_manager_get_policy(rtos_power_policy_t *policy)
{
    RTOS_POWER_CHECK_PARAM(policy != NULL);
    
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    *policy = g_power_manager.policy;
    return RTOS_OK;
}

/**
 * @brief 注册电源事件回调
 */
rtos_result_t rtos_power_manager_register_event_callback(rtos_power_event_t event, 
                                                        rtos_power_event_callback_t callback, 
                                                        void *context)
{
    RTOS_POWER_CHECK_PARAM(event < RTOS_POWER_EVENT_MAX);
    RTOS_POWER_CHECK_PARAM(callback != NULL);
    
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    g_power_manager.event_callbacks[event] = callback;
    g_power_manager.event_contexts[event] = context;
    
    RTOS_POWER_DEBUG_PRINT("Event callback registered: %d", event);
    return RTOS_OK;
}

/**
 * @brief 注销电源事件回调
 */
rtos_result_t rtos_power_manager_unregister_event_callback(rtos_power_event_t event)
{
    RTOS_POWER_CHECK_PARAM(event < RTOS_POWER_EVENT_MAX);
    
    if (!g_power_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    g_power_manager.event_callbacks[event] = NULL;
    g_power_manager.event_contexts[event] = NULL;
    
    RTOS_POWER_DEBUG_PRINT("Event callback unregistered: %d", event);
    return RTOS_OK;
}

/**
 * @brief 电源管理器周期性任务
 */
void rtos_power_manager_periodic_task(void)
{
    if (!g_power_manager_initialized) {
        return;
    }
    
    /* 更新状态信息 */
    rtos_power_status_t current_status;
    rtos_power_manager_get_status(&current_status);
    
    /* 检查电压是否过低 */
    if (current_status.vdd_voltage_mv < g_power_manager.policy.min_voltage_mv) {
        RTOS_POWER_DEBUG_PRINT("Low voltage detected: %lu mV", current_status.vdd_voltage_mv);
        rtos_power_trigger_event(RTOS_POWER_EVENT_VOLTAGE_LOW);
    }
    
    /* 检查温度是否过高 */
    if (current_status.temperature_celsius > 85) {
        RTOS_POWER_DEBUG_PRINT("High temperature detected: %d C", current_status.temperature_celsius);
        rtos_power_trigger_event(RTOS_POWER_EVENT_TEMPERATURE_HIGH);
    }
    
    /* 自动睡眠逻辑 */
    if (g_power_manager.policy.auto_sleep_enable && 
        g_power_manager.current_mode == RTOS_POWER_MODE_RUN) {
        
        static uint32_t idle_start_time = 0;
        
        /* 这里应该检查系统是否空闲 */
        bool system_idle = true; /* 简化实现，实际应该检查任务状态 */
        
        if (system_idle) {
            if (idle_start_time == 0) {
                idle_start_time = rtos_hw_get_system_time_ms();
            } else {
                uint32_t idle_time = rtos_hw_get_system_time_ms() - idle_start_time;
                
                if (idle_time >= g_power_manager.policy.idle_timeout_ms) {
                    rtos_power_mode_t sleep_mode = RTOS_POWER_MODE_SLEEP;
                    
                    if (idle_time >= g_power_manager.policy.deep_sleep_threshold_ms) {
                        sleep_mode = g_power_manager.policy.max_sleep_mode;
                    }
                    
                    RTOS_POWER_DEBUG_PRINT("Auto entering sleep mode %d after %lu ms idle", 
                                         sleep_mode, idle_time);
                    
                    rtos_power_manager_enter_low_power(sleep_mode, 0);
                    idle_start_time = 0;
                }
            }
        } else {
            idle_start_time = 0;
        }
    }
}

/**
 * @brief 电源中断处理函数
 */
void rtos_power_manager_interrupt_handler(void)
{
    if (!g_power_manager_initialized) {
        return;
    }
    
    /* 更新唤醒计数 */
    g_power_manager.status.wakeup_count++;
    
    /* 检查唤醒源 */
    /* 这里应该读取硬件寄存器确定唤醒源 */
    g_power_manager.status.wakeup_source = RTOS_WAKEUP_SOURCE_RTC; /* 简化实现 */
    
    RTOS_POWER_DEBUG_PRINT("Power interrupt handled, wakeup source: 0x%08lx", 
                          g_power_manager.status.wakeup_source);
}

/**
 * @brief 获取电源统计信息
 */
uint32_t rtos_power_manager_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_power_manager_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "Power Manager Statistics:\n"
        "  Current Mode: %d\n"
        "  Total Sleep Time: %lu ms\n"
        "  Total Run Time: %lu ms\n"
        "  Wakeup Count: %lu\n"
        "  Mode Switches:\n"
        "    RUN: %lu\n"
        "    SLEEP: %lu\n"
        "    STOP: %lu\n"
        "    STANDBY: %lu\n"
        "  Power Events:\n"
        "    Mode Changed: %lu\n"
        "    Voltage Low: %lu\n"
        "    Temperature High: %lu\n"
        "    Wakeup: %lu\n"
        "  Current Status:\n"
        "    VDD: %lu mV\n"
        "    VBAT: %lu mV\n"
        "    Temperature: %d C\n",
        g_power_manager.current_mode,
        g_power_manager.total_sleep_time_ms,
        g_power_manager.total_run_time_ms,
        g_power_manager.status.wakeup_count,
        g_power_manager.mode_switch_count[RTOS_POWER_MODE_RUN],
        g_power_manager.mode_switch_count[RTOS_POWER_MODE_SLEEP],
        g_power_manager.mode_switch_count[RTOS_POWER_MODE_STOP],
        g_power_manager.mode_switch_count[RTOS_POWER_MODE_STANDBY],
        g_power_manager.power_events_count[RTOS_POWER_EVENT_MODE_CHANGED],
        g_power_manager.power_events_count[RTOS_POWER_EVENT_VOLTAGE_LOW],
        g_power_manager.power_events_count[RTOS_POWER_EVENT_TEMPERATURE_HIGH],
        g_power_manager.power_events_count[RTOS_POWER_EVENT_WAKEUP],
        g_power_manager.status.vdd_voltage_mv,
        g_power_manager.status.vbat_voltage_mv,
        g_power_manager.status.temperature_celsius);
    
    if (len < 0) {
        return 0;
    }
    
    return (len >= (int)size) ? (size - 1) : (uint32_t)len;
}

/* 内部函数实现 */

/**
 * @brief 平台相关初始化
 */
static rtos_result_t rtos_power_platform_init(void)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 使能PWR时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    
    /* 使能备份域访问 */
    PWR_BackupAccessCmd(ENABLE);
    
    /* 配置ADC用于电压监测 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    ADC_InitTypeDef adc_init;
    ADC_StructInit(&adc_init);
    adc_init.ADC_Resolution = ADC_Resolution_12b;
    adc_init.ADC_ScanConvMode = DISABLE;
    adc_init.ADC_ContinuousConvMode = DISABLE;
    adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
    adc_init.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &adc_init);
    
    /* 使能ADC */
    ADC_Cmd(ADC1, ENABLE);
    
    /* 使能温度传感器和Vrefint */
    ADC_TempSensorVrefintCmd(ENABLE);
    
    return RTOS_OK;
#else
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关模式设置
 */
static rtos_result_t rtos_power_platform_set_mode(rtos_power_mode_t mode)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    switch (mode) {
        case RTOS_POWER_MODE_RUN:
            /* 正常运行模式，无需特殊操作 */
            break;
            
        case RTOS_POWER_MODE_SLEEP:
            /* 进入睡眠模式 */
            __WFI(); /* 直接使用WFI指令进入睡眠 */
            break;
            
        case RTOS_POWER_MODE_STOP:
            /* 进入停止模式 */
            PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
            /* 唤醒后需要重新配置时钟 */
            SystemInit();
            break;
            
        case RTOS_POWER_MODE_STANDBY:
            /* 进入待机模式 */
            PWR_EnterSTANDBYMode();
            /* 注意：从待机模式唤醒会重启系统 */
            break;
            
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    return RTOS_OK;
#else
    (void)mode;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关唤醒源配置
 */
static rtos_result_t rtos_power_platform_configure_wakeup(uint32_t sources)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 清除所有唤醒标志 */
    PWR_ClearFlag(PWR_FLAG_WU);
    
    if (sources & RTOS_WAKEUP_SOURCE_WKUP_PIN) {
        /* 使能WKUP引脚唤醒 */
        PWR_WakeUpPinCmd(ENABLE);
    } else {
        PWR_WakeUpPinCmd(DISABLE);
    }
    
    /* 其他唤醒源配置... */
    
    return RTOS_OK;
#else
    (void)sources;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关电压读取
 */
static rtos_result_t rtos_power_platform_read_voltage(uint32_t *vdd_mv, uint32_t *vbat_mv)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    if (vdd_mv) {
        /* 读取Vrefint通道获取VDD电压 */
        ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 1, ADC_SampleTime_144Cycles);
        ADC_SoftwareStartConv(ADC1);
        
        while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
        uint32_t vrefint_data = ADC_GetConversionValue(ADC1);
        
        /* VDD = 1.21V * 4096 / ADC_Value */
        *vdd_mv = (1210 * 4096) / vrefint_data;
    }
    
    if (vbat_mv) {
        /* 读取VBAT通道 */
        ADC_RegularChannelConfig(ADC1, ADC_Channel_Vbat, 1, ADC_SampleTime_144Cycles);
        ADC_SoftwareStartConv(ADC1);
        
        while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
        uint32_t vbat_data = ADC_GetConversionValue(ADC1);
        
        /* VBAT = ADC_Value * VDD / 4096 * 2 (分压比) */
        uint32_t vdd = vdd_mv ? *vdd_mv : 3300;
        *vbat_mv = (vbat_data * vdd * 2) / 4096;
    }
    
    return RTOS_OK;
#else
    /* 简化实现 */
    if (vdd_mv) *vdd_mv = 3300;
    if (vbat_mv) *vbat_mv = 3000;
    return RTOS_OK;
#endif
}

/**
 * @brief 平台相关温度读取
 */
static rtos_result_t rtos_power_platform_read_temperature(int16_t *temp_celsius)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    if (temp_celsius) {
        /* 读取温度传感器通道 */
        ADC_RegularChannelConfig(ADC1, ADC_Channel_TempSensor, 1, ADC_SampleTime_144Cycles);
        ADC_SoftwareStartConv(ADC1);
        
        while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
        uint32_t temp_data = ADC_GetConversionValue(ADC1);
        
        /* 温度计算公式：T = (V25 - Vsense) / Avg_Slope + 25 */
        /* V25 = 0.76V, Avg_Slope = 2.5mV/°C */
        int32_t vsense_mv = (temp_data * 3300) / 4096;
        *temp_celsius = (int16_t)((760 - vsense_mv) * 1000 / 2500 + 25);
    }
    
    return RTOS_OK;
#else
    /* 简化实现 */
    if (temp_celsius) *temp_celsius = 25;
    return RTOS_OK;
#endif
}

/**
 * @brief 触发电源事件
 */
static void rtos_power_trigger_event(rtos_power_event_t event)
{
    if (event >= RTOS_POWER_EVENT_MAX) {
        return;
    }
    
    /* 更新事件计数 */
    g_power_manager.power_events_count[event]++;
    
    /* 调用回调函数 */
    if (g_power_manager.event_callbacks[event]) {
        g_power_manager.event_callbacks[event](event, g_power_manager.event_contexts[event]);
    }
    
    RTOS_POWER_DEBUG_PRINT("Power event triggered: %d", event);
}

/**
 * @brief 更新统计信息
 */
static void rtos_power_update_statistics(rtos_power_mode_t old_mode, rtos_power_mode_t new_mode)
{
    /* 更新模式切换计数 */
    if (new_mode < RTOS_POWER_MODE_MAX) {
        g_power_manager.mode_switch_count[new_mode]++;
    }
    
    /* 更新运行时间统计 */
    uint32_t current_time = rtos_hw_get_system_time_ms();
    
    if (old_mode == RTOS_POWER_MODE_RUN) {
        g_power_manager.total_run_time_ms = current_time;
    }
}