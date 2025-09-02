/**
 * @file gpio_abstraction.c
 * @brief RTOS GPIO抽象模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "gpio_abstraction.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_gpio.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/stm32f4xx_exti.h"
#include "fwlib/inc/stm32f4xx_syscfg.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局GPIO管理器实例 */
static rtos_gpio_manager_t g_gpio_manager;
static bool g_gpio_manager_initialized = false;

/* STM32F4平台相关数据 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
static GPIO_TypeDef* const g_gpio_ports[RTOS_GPIO_PORT_MAX] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI
};

static const uint32_t g_gpio_rcc_clocks[RTOS_GPIO_PORT_MAX] = {
    RCC_AHB1Periph_GPIOA, RCC_AHB1Periph_GPIOB, RCC_AHB1Periph_GPIOC,
    RCC_AHB1Periph_GPIOD, RCC_AHB1Periph_GPIOE, RCC_AHB1Periph_GPIOF,
    RCC_AHB1Periph_GPIOG, RCC_AHB1Periph_GPIOH, RCC_AHB1Periph_GPIOI
};

static const uint8_t g_gpio_port_sources[RTOS_GPIO_PORT_MAX] = {
    EXTI_PortSourceGPIOA, EXTI_PortSourceGPIOB, EXTI_PortSourceGPIOC,
    EXTI_PortSourceGPIOD, EXTI_PortSourceGPIOE, EXTI_PortSourceGPIOF,
    EXTI_PortSourceGPIOG, EXTI_PortSourceGPIOH, EXTI_PortSourceGPIOI
};
#endif

/* 内部函数声明 */
static rtos_result_t rtos_gpio_platform_config_pin(const rtos_gpio_config_t *config);
static rtos_result_t rtos_gpio_platform_write_pin(rtos_gpio_port_t port, rtos_gpio_pin_t pin, bool value);
static rtos_result_t rtos_gpio_platform_read_pin(rtos_gpio_port_t port, rtos_gpio_pin_t pin, bool *value);
static rtos_result_t rtos_gpio_platform_config_interrupt(rtos_gpio_port_t port, rtos_gpio_pin_t pin, rtos_gpio_trigger_t trigger);
static rtos_gpio_handle_t* rtos_gpio_find_handle(rtos_gpio_port_t port, rtos_gpio_pin_t pin);
static uint32_t rtos_gpio_get_handle_index(rtos_gpio_port_t port, rtos_gpio_pin_t pin);

/**
 * @brief 初始化GPIO管理器
 */
rtos_result_t rtos_gpio_manager_init(uint32_t max_gpio_count)
{
    if (g_gpio_manager_initialized) {
        return RTOS_OK;
    }
    
    if (max_gpio_count == 0) {
        max_gpio_count = RTOS_GPIO_PORT_MAX * RTOS_GPIO_PIN_MAX; /* 默认支持所有GPIO */
    }
    
    /* 清零管理器结构 */
    memset(&g_gpio_manager, 0, sizeof(g_gpio_manager));
    
    /* 分配GPIO句柄数组 */
    g_gpio_manager.gpio_handles = malloc(sizeof(rtos_gpio_handle_t) * max_gpio_count);
    if (!g_gpio_manager.gpio_handles) {
        RTOS_GPIO_DEBUG_PRINT("Failed to allocate GPIO handles");
        return RTOS_ERROR_NO_MEMORY;
    }
    
    memset(g_gpio_manager.gpio_handles, 0, sizeof(rtos_gpio_handle_t) * max_gpio_count);
    g_gpio_manager.max_gpio_count = max_gpio_count;
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 使能SYSCFG时钟（用于EXTI配置） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
#endif
    
    g_gpio_manager.initialized = true;
    g_gpio_manager_initialized = true;
    
    RTOS_GPIO_DEBUG_PRINT("GPIO manager initialized (max GPIOs: %lu)", max_gpio_count);
    return RTOS_OK;
}

/**
 * @brief 反初始化GPIO管理器
 */
rtos_result_t rtos_gpio_manager_deinit(void)
{
    if (!g_gpio_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 反配置所有GPIO */
    for (uint32_t i = 0; i < g_gpio_manager.max_gpio_count; i++) {
        if (g_gpio_manager.gpio_handles[i].configured) {
            rtos_gpio_manager_deconfig_pin(&g_gpio_manager.gpio_handles[i]);
        }
    }
    
    /* 释放GPIO句柄数组 */
    if (g_gpio_manager.gpio_handles) {
        free(g_gpio_manager.gpio_handles);
        g_gpio_manager.gpio_handles = NULL;
    }
    
    /* 清空中断回调 */
    memset(g_gpio_manager.exti_callbacks, 0, sizeof(g_gpio_manager.exti_callbacks));
    memset(g_gpio_manager.exti_contexts, 0, sizeof(g_gpio_manager.exti_contexts));
    
    g_gpio_manager_initialized = false;
    
    RTOS_GPIO_DEBUG_PRINT("GPIO manager deinitialized");
    return RTOS_OK;
}

/**
 * @brief 获取GPIO管理器实例
 */
rtos_gpio_manager_t* rtos_gpio_manager_get_instance(void)
{
    if (!g_gpio_manager_initialized) {
        return NULL;
    }
    return &g_gpio_manager;
}

/**
 * @brief 配置GPIO引脚
 */
rtos_result_t rtos_gpio_manager_config_pin(const rtos_gpio_config_t *config, 
                                          rtos_gpio_handle_t **handle)
{
    RTOS_GPIO_CHECK_PARAM(config != NULL);
    RTOS_GPIO_CHECK_PARAM(handle != NULL);
    RTOS_GPIO_CHECK_PARAM(config->port < RTOS_GPIO_PORT_MAX);
    RTOS_GPIO_CHECK_PARAM(config->pin < RTOS_GPIO_PIN_MAX);
    RTOS_GPIO_CHECK_PARAM(config->mode < RTOS_GPIO_MODE_MAX);
    RTOS_GPIO_CHECK_INIT();
    
    /* 查找现有句柄或分配新句柄 */
    rtos_gpio_handle_t *gpio_handle = rtos_gpio_find_handle(config->port, config->pin);
    
    if (!gpio_handle) {
        /* 查找空闲句柄 */
        for (uint32_t i = 0; i < g_gpio_manager.max_gpio_count; i++) {
            if (!g_gpio_manager.gpio_handles[i].configured) {
                gpio_handle = &g_gpio_manager.gpio_handles[i];
                break;
            }
        }
        
        if (!gpio_handle) {
            RTOS_GPIO_DEBUG_PRINT("No free GPIO handle available");
            return RTOS_ERROR_NO_MEMORY;
        }
        
        g_gpio_manager.configured_gpio_count++;
    }
    
    /* 配置GPIO */
    rtos_result_t result = rtos_gpio_platform_config_pin(config);
    if (result != RTOS_OK) {
        RTOS_GPIO_DEBUG_PRINT("Platform GPIO config failed: %d", result);
        return result;
    }
    
    /* 初始化句柄 */
    gpio_handle->port = config->port;
    gpio_handle->pin = config->pin;
    gpio_handle->config = *config;
    gpio_handle->configured = true;
    gpio_handle->toggle_count = 0;
    gpio_handle->last_change_time = rtos_hw_get_system_time_ms();
    
    /* 设置初始值 */
    if (config->mode == RTOS_GPIO_MODE_OUTPUT_PP || config->mode == RTOS_GPIO_MODE_OUTPUT_OD) {
        rtos_gpio_manager_write_pin(gpio_handle, config->initial_value);
    }
    
    /* 配置中断 */
    if (config->trigger != RTOS_GPIO_TRIGGER_NONE) {
        result = rtos_gpio_platform_config_interrupt(config->port, config->pin, config->trigger);
        if (result != RTOS_OK) {
            RTOS_GPIO_DEBUG_PRINT("GPIO interrupt config failed: %d", result);
        }
    }
    
    *handle = gpio_handle;
    g_gpio_manager.config_operations++;
    
    RTOS_GPIO_DEBUG_PRINT("GPIO configured: P%c%d, mode=%d", 
                          'A' + config->port, config->pin, config->mode);
    
    return RTOS_OK;
}

/**
 * @brief 反配置GPIO引脚
 */
rtos_result_t rtos_gpio_manager_deconfig_pin(rtos_gpio_handle_t *handle)
{
    RTOS_GPIO_CHECK_HANDLE(handle);
    RTOS_GPIO_CHECK_INIT();
    
    /* 禁用中断 */
    if (handle->config.trigger != RTOS_GPIO_TRIGGER_NONE) {
        rtos_gpio_manager_disable_interrupt(handle);
    }
    
    /* 重置为输入模式 */
    rtos_gpio_config_t reset_config = {
        .port = handle->port,
        .pin = handle->pin,
        .mode = RTOS_GPIO_MODE_INPUT,
        .pull = RTOS_GPIO_PULL_NONE,
        .speed = RTOS_GPIO_SPEED_LOW,
        .alternate_function = 0,
        .trigger = RTOS_GPIO_TRIGGER_NONE,
        .initial_value = false
    };
    
    rtos_gpio_platform_config_pin(&reset_config);
    
    /* 清除句柄 */
    memset(handle, 0, sizeof(rtos_gpio_handle_t));
    g_gpio_manager.configured_gpio_count--;
    
    RTOS_GPIO_DEBUG_PRINT("GPIO deconfigured");
    return RTOS_OK;
}

/**
 * @brief 写GPIO引脚
 */
rtos_result_t rtos_gpio_manager_write_pin(rtos_gpio_handle_t *handle, bool value)
{
    RTOS_GPIO_CHECK_HANDLE(handle);
    RTOS_GPIO_CHECK_INIT();
    
    /* 检查是否为输出模式 */
    if (handle->config.mode != RTOS_GPIO_MODE_OUTPUT_PP && 
        handle->config.mode != RTOS_GPIO_MODE_OUTPUT_OD) {
        RTOS_GPIO_DEBUG_PRINT("GPIO not in output mode");
        return RTOS_ERROR_INVALID_STATE;
    }
    
    rtos_result_t result = rtos_gpio_platform_write_pin(handle->port, handle->pin, value);
    if (result == RTOS_OK) {
        handle->last_change_time = rtos_hw_get_system_time_ms();
        g_gpio_manager.write_operations++;
    }
    
    return result;
}

/**
 * @brief 读GPIO引脚
 */
rtos_result_t rtos_gpio_manager_read_pin(rtos_gpio_handle_t *handle, bool *value)
{
    RTOS_GPIO_CHECK_HANDLE(handle);
    RTOS_GPIO_CHECK_PARAM(value != NULL);
    RTOS_GPIO_CHECK_INIT();
    
    rtos_result_t result = rtos_gpio_platform_read_pin(handle->port, handle->pin, value);
    if (result == RTOS_OK) {
        g_gpio_manager.read_operations++;
    }
    
    return result;
}

/**
 * @brief 翻转GPIO引脚
 */
rtos_result_t rtos_gpio_manager_toggle_pin(rtos_gpio_handle_t *handle)
{
    RTOS_GPIO_CHECK_HANDLE(handle);
    RTOS_GPIO_CHECK_INIT();
    
    /* 读取当前值 */
    bool current_value;
    rtos_result_t result = rtos_gpio_manager_read_pin(handle, &current_value);
    if (result != RTOS_OK) {
        return result;
    }
    
    /* 写入相反值 */
    result = rtos_gpio_manager_write_pin(handle, !current_value);
    if (result == RTOS_OK) {
        handle->toggle_count++;
        RTOS_GPIO_DEBUG_PRINT("GPIO toggled: P%c%d, count=%lu", 
                              'A' + handle->port, handle->pin, handle->toggle_count);
    }
    
    return result;
}

/**
 * @brief 设置GPIO中断回调
 */
rtos_result_t rtos_gpio_manager_set_interrupt_callback(rtos_gpio_handle_t *handle,
                                                      rtos_gpio_interrupt_callback_t callback,
                                                      void *context)
{
    RTOS_GPIO_CHECK_HANDLE(handle);
    RTOS_GPIO_CHECK_PARAM(callback != NULL);
    RTOS_GPIO_CHECK_INIT();
    
    handle->interrupt_callback = callback;
    handle->interrupt_context = context;
    
    /* 同时设置全局EXTI回调 */
    if (handle->pin < 16) {
        g_gpio_manager.exti_callbacks[handle->pin] = callback;
        g_gpio_manager.exti_contexts[handle->pin] = context;
    }
    
    RTOS_GPIO_DEBUG_PRINT("GPIO interrupt callback set: P%c%d", 
                          'A' + handle->port, handle->pin);
    
    return RTOS_OK;
}

/**
 * @brief 启用GPIO中断
 */
rtos_result_t rtos_gpio_manager_enable_interrupt(rtos_gpio_handle_t *handle)
{
    RTOS_GPIO_CHECK_HANDLE(handle);
    RTOS_GPIO_CHECK_INIT();
    
    if (handle->config.trigger == RTOS_GPIO_TRIGGER_NONE) {
        RTOS_GPIO_DEBUG_PRINT("GPIO interrupt not configured");
        return RTOS_ERROR_INVALID_STATE;
    }
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 使能EXTI中断 */
    uint32_t exti_line = 1 << handle->pin;
    EXTI_InitTypeDef exti_init;
    
    exti_init.EXTI_Line = exti_line;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    
    switch (handle->config.trigger) {
        case RTOS_GPIO_TRIGGER_RISING:
            exti_init.EXTI_Trigger = EXTI_Trigger_Rising;
            break;
        case RTOS_GPIO_TRIGGER_FALLING:
            exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
            break;
        case RTOS_GPIO_TRIGGER_BOTH:
            exti_init.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);
    
    /* 配置NVIC */
    NVIC_InitTypeDef nvic_init;
    
    if (handle->pin <= 4) {
        nvic_init.NVIC_IRQChannel = EXTI0_IRQn + handle->pin;
    } else if (handle->pin <= 9) {
        nvic_init.NVIC_IRQChannel = EXTI9_5_IRQn;
    } else {
        nvic_init.NVIC_IRQChannel = EXTI15_10_IRQn;
    }
    
    nvic_init.NVIC_IRQChannelPreemptionPriority = 2;
    nvic_init.NVIC_IRQChannelSubPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
    
    RTOS_GPIO_DEBUG_PRINT("GPIO interrupt enabled: P%c%d", 
                          'A' + handle->port, handle->pin);
#endif
    
    return RTOS_OK;
}

/**
 * @brief 禁用GPIO中断
 */
rtos_result_t rtos_gpio_manager_disable_interrupt(rtos_gpio_handle_t *handle)
{
    RTOS_GPIO_CHECK_HANDLE(handle);
    RTOS_GPIO_CHECK_INIT();
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 禁用EXTI中断 */
    uint32_t exti_line = 1 << handle->pin;
    EXTI_InitTypeDef exti_init;
    
    exti_init.EXTI_Line = exti_line;
    exti_init.EXTI_LineCmd = DISABLE;
    EXTI_Init(&exti_init);
    
    RTOS_GPIO_DEBUG_PRINT("GPIO interrupt disabled: P%c%d", 
                          'A' + handle->port, handle->pin);
#endif
    
    /* 清除回调 */
    handle->interrupt_callback = NULL;
    handle->interrupt_context = NULL;
    
    if (handle->pin < 16) {
        g_gpio_manager.exti_callbacks[handle->pin] = NULL;
        g_gpio_manager.exti_contexts[handle->pin] = NULL;
    }
    
    return RTOS_OK;
}

/**
 * @brief 批量写GPIO端口
 */
rtos_result_t rtos_gpio_manager_write_port(rtos_gpio_port_t port, uint16_t mask, uint16_t value)
{
    RTOS_GPIO_CHECK_PARAM(port < RTOS_GPIO_PORT_MAX);
    RTOS_GPIO_CHECK_INIT();
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    GPIO_TypeDef *gpio_port = g_gpio_ports[port];
    
    /* 使用BSRR寄存器实现原子操作 */
    uint32_t bsrr_value = 0;
    
    for (int i = 0; i < 16; i++) {
        if (mask & (1 << i)) {
            if (value & (1 << i)) {
                bsrr_value |= (1 << i);        /* 设置位 */
            } else {
                bsrr_value |= (1 << (i + 16)); /* 复位位 */
            }
        }
    }
    
    gpio_port->BSRR = bsrr_value;
    
    g_gpio_manager.write_operations++;
    
    RTOS_GPIO_DEBUG_PRINT("GPIO port write: P%c, mask=0x%04x, value=0x%04x", 
                          'A' + port, mask, value);
    
    return RTOS_OK;
#else
    (void)port;
    (void)mask;
    (void)value;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 批量读GPIO端口
 */
rtos_result_t rtos_gpio_manager_read_port(rtos_gpio_port_t port, uint16_t *value)
{
    RTOS_GPIO_CHECK_PARAM(port < RTOS_GPIO_PORT_MAX);
    RTOS_GPIO_CHECK_PARAM(value != NULL);
    RTOS_GPIO_CHECK_INIT();
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    GPIO_TypeDef *gpio_port = g_gpio_ports[port];
    *value = (uint16_t)gpio_port->IDR;
    
    g_gpio_manager.read_operations++;
    
    return RTOS_OK;
#else
    *value = 0;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief GPIO中断处理函数
 */
void rtos_gpio_manager_interrupt_handler(uint32_t exti_line)
{
    if (!g_gpio_manager_initialized || exti_line >= 16) {
        return;
    }
    
    g_gpio_manager.interrupt_count++;
    
    /* 调用注册的回调函数 */
    if (g_gpio_manager.exti_callbacks[exti_line]) {
        /* 查找对应的GPIO句柄 */
        for (uint32_t i = 0; i < g_gpio_manager.max_gpio_count; i++) {
            if (g_gpio_manager.gpio_handles[i].configured && 
                g_gpio_manager.gpio_handles[i].pin == exti_line) {
                
                g_gpio_manager.exti_callbacks[exti_line](
                    g_gpio_manager.gpio_handles[i].port,
                    g_gpio_manager.gpio_handles[i].pin,
                    g_gpio_manager.exti_contexts[exti_line]);
                
                RTOS_GPIO_DEBUG_PRINT("GPIO interrupt handled: P%c%d", 
                                      'A' + g_gpio_manager.gpio_handles[i].port, exti_line);
                break;
            }
        }
    }
}

/**
 * @brief 获取GPIO统计信息
 */
uint32_t rtos_gpio_manager_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_gpio_manager_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "GPIO Manager Statistics:\n"
        "  Configured GPIOs: %lu/%lu\n"
        "  Operations:\n"
        "    Config: %lu\n"
        "    Read: %lu\n"
        "    Write: %lu\n"
        "    Interrupts: %lu\n"
        "  GPIO Usage by Port:\n",
        g_gpio_manager.configured_gpio_count,
        g_gpio_manager.max_gpio_count,
        g_gpio_manager.config_operations,
        g_gpio_manager.read_operations,
        g_gpio_manager.write_operations,
        g_gpio_manager.interrupt_count);
    
    /* 统计每个端口的使用情况 */
    for (uint32_t port = 0; port < RTOS_GPIO_PORT_MAX; port++) {
        uint32_t port_count = 0;
        for (uint32_t i = 0; i < g_gpio_manager.max_gpio_count; i++) {
            if (g_gpio_manager.gpio_handles[i].configured && 
                g_gpio_manager.gpio_handles[i].port == port) {
                port_count++;
            }
        }
        
        if (port_count > 0) {
            int port_len = snprintf(buffer + len, size - len, 
                                   "    Port %c: %lu pins\n", 'A' + port, port_count);
            if (port_len > 0 && len + port_len < (int)size) {
                len += port_len;
            }
        }
    }
    
    if (len < 0) {
        return 0;
    }
    
    return (len >= (int)size) ? (size - 1) : (uint32_t)len;
}

/* 内部函数实现 */

/**
 * @brief 平台相关GPIO配置
 */
static rtos_result_t rtos_gpio_platform_config_pin(const rtos_gpio_config_t *config)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 使能GPIO时钟 */
    RCC_AHB1PeriphClockCmd(g_gpio_rcc_clocks[config->port], ENABLE);
    
    /* 配置GPIO */
    GPIO_InitTypeDef gpio_init;
    GPIO_StructInit(&gpio_init);
    
    gpio_init.GPIO_Pin = 1 << config->pin;
    gpio_init.GPIO_Speed = (GPIOSpeed_TypeDef)config->speed;
    
    switch (config->mode) {
        case RTOS_GPIO_MODE_INPUT:
            gpio_init.GPIO_Mode = GPIO_Mode_IN;
            break;
        case RTOS_GPIO_MODE_OUTPUT_PP:
            gpio_init.GPIO_Mode = GPIO_Mode_OUT;
            gpio_init.GPIO_OType = GPIO_OType_PP;
            break;
        case RTOS_GPIO_MODE_OUTPUT_OD:
            gpio_init.GPIO_Mode = GPIO_Mode_OUT;
            gpio_init.GPIO_OType = GPIO_OType_OD;
            break;
        case RTOS_GPIO_MODE_ALTERNATE_PP:
            gpio_init.GPIO_Mode = GPIO_Mode_AF;
            gpio_init.GPIO_OType = GPIO_OType_PP;
            break;
        case RTOS_GPIO_MODE_ALTERNATE_OD:
            gpio_init.GPIO_Mode = GPIO_Mode_AF;
            gpio_init.GPIO_OType = GPIO_OType_OD;
            break;
        case RTOS_GPIO_MODE_ANALOG:
            gpio_init.GPIO_Mode = GPIO_Mode_AN;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    switch (config->pull) {
        case RTOS_GPIO_PULL_NONE:
            gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
            break;
        case RTOS_GPIO_PULL_UP:
            gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
            break;
        case RTOS_GPIO_PULL_DOWN:
            gpio_init.GPIO_PuPd = GPIO_PuPd_DOWN;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    GPIO_Init(g_gpio_ports[config->port], &gpio_init);
    
    /* 配置复用功能 */
    if (config->mode == RTOS_GPIO_MODE_ALTERNATE_PP || 
        config->mode == RTOS_GPIO_MODE_ALTERNATE_OD) {
        GPIO_PinAFConfig(g_gpio_ports[config->port], config->pin, config->alternate_function);
    }
    
    return RTOS_OK;
#else
    (void)config;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关GPIO写操作
 */
static rtos_result_t rtos_gpio_platform_write_pin(rtos_gpio_port_t port, rtos_gpio_pin_t pin, bool value)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    GPIO_TypeDef *gpio_port = g_gpio_ports[port];
    
    if (value) {
        GPIO_SetBits(gpio_port, 1 << pin);
    } else {
        GPIO_ResetBits(gpio_port, 1 << pin);
    }
    
    return RTOS_OK;
#else
    (void)port;
    (void)pin;
    (void)value;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关GPIO读操作
 */
static rtos_result_t rtos_gpio_platform_read_pin(rtos_gpio_port_t port, rtos_gpio_pin_t pin, bool *value)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    GPIO_TypeDef *gpio_port = g_gpio_ports[port];
    
    *value = (GPIO_ReadInputDataBit(gpio_port, 1 << pin) == Bit_SET);
    
    return RTOS_OK;
#else
    (void)port;
    (void)pin;
    *value = false;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关GPIO中断配置
 */
static rtos_result_t rtos_gpio_platform_config_interrupt(rtos_gpio_port_t port, rtos_gpio_pin_t pin, rtos_gpio_trigger_t trigger)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 连接EXTI线到GPIO引脚 */
    SYSCFG_EXTILineConfig(g_gpio_port_sources[port], pin);
    
    RTOS_GPIO_DEBUG_PRINT("GPIO interrupt configured: P%c%d, trigger=%d", 
                          'A' + port, pin, trigger);
    
    return RTOS_OK;
#else
    (void)port;
    (void)pin;
    (void)trigger;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 查找GPIO句柄
 */
static rtos_gpio_handle_t* rtos_gpio_find_handle(rtos_gpio_port_t port, rtos_gpio_pin_t pin)
{
    for (uint32_t i = 0; i < g_gpio_manager.max_gpio_count; i++) {
        if (g_gpio_manager.gpio_handles[i].configured &&
            g_gpio_manager.gpio_handles[i].port == port &&
            g_gpio_manager.gpio_handles[i].pin == pin) {
            return &g_gpio_manager.gpio_handles[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 获取GPIO句柄索引
 */
static uint32_t rtos_gpio_get_handle_index(rtos_gpio_port_t port, rtos_gpio_pin_t pin)
{
    for (uint32_t i = 0; i < g_gpio_manager.max_gpio_count; i++) {
        if (g_gpio_manager.gpio_handles[i].configured &&
            g_gpio_manager.gpio_handles[i].port == port &&
            g_gpio_manager.gpio_handles[i].pin == pin) {
            return i;
        }
    }
    
    return UINT32_MAX;
}