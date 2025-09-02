/**
 * @file i2c_abstraction.c
 * @brief RTOS I2C抽象模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "i2c_abstraction.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_i2c.h"
#include "fwlib/inc/stm32f4xx_gpio.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局I2C管理器实例 */
static rtos_i2c_manager_t g_i2c_manager;
static bool g_i2c_manager_initialized = false;

/* STM32F4平台相关数据 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
static I2C_TypeDef* const g_i2c_instances[RTOS_I2C_PORT_MAX] = {
    I2C1, I2C2, I2C3
};

static const uint32_t g_i2c_rcc_clocks[RTOS_I2C_PORT_MAX] = {
    RCC_APB1Periph_I2C1, RCC_APB1Periph_I2C2, RCC_APB1Periph_I2C3
};

static const IRQn_Type g_i2c_ev_irq_numbers[RTOS_I2C_PORT_MAX] = {
    I2C1_EV_IRQn, I2C2_EV_IRQn, I2C3_EV_IRQn
};

static const IRQn_Type g_i2c_er_irq_numbers[RTOS_I2C_PORT_MAX] = {
    I2C1_ER_IRQn, I2C2_ER_IRQn, I2C3_ER_IRQn
};
#endif

/* 内部函数声明 */
static rtos_result_t rtos_i2c_platform_init_port(rtos_i2c_port_t port, const rtos_i2c_config_t *config);
static rtos_result_t rtos_i2c_platform_deinit_port(rtos_i2c_port_t port);
static rtos_result_t rtos_i2c_platform_master_send(rtos_i2c_port_t port, uint16_t addr, const uint8_t *data, uint32_t length);
static rtos_result_t rtos_i2c_platform_master_receive(rtos_i2c_port_t port, uint16_t addr, uint8_t *buffer, uint32_t length);

/**
 * @brief 初始化I2C管理器
 */
rtos_result_t rtos_i2c_manager_init(void)
{
    if (g_i2c_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_i2c_manager, 0, sizeof(g_i2c_manager));
    
    /* 初始化所有I2C句柄 */
    for (uint32_t i = 0; i < RTOS_I2C_PORT_MAX; i++) {
        g_i2c_manager.i2c_handles[i].port = (rtos_i2c_port_t)i;
        g_i2c_manager.i2c_handles[i].state = RTOS_I2C_STATE_RESET;
        g_i2c_manager.i2c_handles[i].initialized = false;
    }
    
    g_i2c_manager.initialized = true;
    g_i2c_manager_initialized = true;
    
    RTOS_I2C_DEBUG_PRINT("I2C manager initialized");
    return RTOS_OK;
}

/**
 * @brief 初始化I2C端口
 */
rtos_result_t rtos_i2c_manager_init_port(rtos_i2c_port_t port, 
                                        const rtos_i2c_config_t *config,
                                        uint32_t max_devices)
{
    RTOS_I2C_CHECK_PARAM(config != NULL);
    RTOS_I2C_CHECK_PORT(port);
    RTOS_I2C_CHECK_INIT();
    
    rtos_i2c_handle_t *handle = &g_i2c_manager.i2c_handles[port];
    
    if (handle->initialized) {
        RTOS_I2C_DEBUG_PRINT("I2C%d already initialized", port + 1);
        return RTOS_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置 */
    handle->config = *config;
    
    /* 分配设备数组 */
    if (max_devices > 0) {
        handle->devices = malloc(sizeof(rtos_i2c_device_config_t) * max_devices);
        if (!handle->devices) {
            RTOS_I2C_DEBUG_PRINT("Failed to allocate I2C device array");
            return RTOS_ERROR_NO_MEMORY;
        }
        handle->max_devices = max_devices;
        memset(handle->devices, 0, sizeof(rtos_i2c_device_config_t) * max_devices);
    }
    
    /* 调用平台相关初始化 */
    rtos_result_t result = rtos_i2c_platform_init_port(port, config);
    if (result != RTOS_OK) {
        if (handle->devices) {
            free(handle->devices);
            handle->devices = NULL;
        }
        RTOS_I2C_DEBUG_PRINT("I2C%d platform init failed: %d", port + 1, result);
        return result;
    }
    
    /* 设置状态 */
    handle->state = RTOS_I2C_STATE_READY;
    handle->initialized = true;
    g_i2c_manager.active_ports++;
    
    RTOS_I2C_DEBUG_PRINT("I2C%d initialized (speed: %lu Hz, max devices: %lu)", 
                         port + 1, config->clock_speed, max_devices);
    return RTOS_OK;
}

/**
 * @brief 主机模式发送数据
 */
rtos_result_t rtos_i2c_manager_master_send(rtos_i2c_port_t port,
                                          uint16_t device_addr,
                                          const uint8_t *data,
                                          uint32_t length)
{
    RTOS_I2C_CHECK_PORT(port);
    RTOS_I2C_CHECK_PARAM(data != NULL);
    RTOS_I2C_CHECK_PARAM(length > 0);
    RTOS_I2C_CHECK_INIT();
    
    rtos_i2c_handle_t *handle = &g_i2c_manager.i2c_handles[port];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->state != RTOS_I2C_STATE_READY) {
        return RTOS_ERROR_BUSY;
    }
    
    handle->state = RTOS_I2C_STATE_BUSY_TX;
    uint32_t start_time = rtos_hw_get_system_time_ms();
    
    rtos_result_t result = rtos_i2c_platform_master_send(port, device_addr, data, length);
    
    uint32_t transfer_time = rtos_hw_get_system_time_ms() - start_time;
    
    if (result == RTOS_OK) {
        handle->stats.master_tx_count++;
        handle->stats.total_bytes += length;
        g_i2c_manager.total_transfers++;
        g_i2c_manager.total_bytes += length;
    } else {
        handle->stats.error_count++;
        g_i2c_manager.total_errors++;
    }
    
    handle->state = RTOS_I2C_STATE_READY;
    
    RTOS_I2C_DEBUG_PRINT("I2C%d master send: addr=0x%02X, %lu bytes in %lu ms, result=%d", 
                         port + 1, device_addr, length, transfer_time, result);
    
    return result;
}

/**
 * @brief 主机模式接收数据
 */
rtos_result_t rtos_i2c_manager_master_receive(rtos_i2c_port_t port,
                                             uint16_t device_addr,
                                             uint8_t *buffer,
                                             uint32_t length)
{
    RTOS_I2C_CHECK_PORT(port);
    RTOS_I2C_CHECK_PARAM(buffer != NULL);
    RTOS_I2C_CHECK_PARAM(length > 0);
    RTOS_I2C_CHECK_INIT();
    
    rtos_i2c_handle_t *handle = &g_i2c_manager.i2c_handles[port];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->state != RTOS_I2C_STATE_READY) {
        return RTOS_ERROR_BUSY;
    }
    
    handle->state = RTOS_I2C_STATE_BUSY_RX;
    uint32_t start_time = rtos_hw_get_system_time_ms();
    
    rtos_result_t result = rtos_i2c_platform_master_receive(port, device_addr, buffer, length);
    
    uint32_t transfer_time = rtos_hw_get_system_time_ms() - start_time;
    
    if (result == RTOS_OK) {
        handle->stats.master_rx_count++;
        handle->stats.total_bytes += length;
        g_i2c_manager.total_transfers++;
        g_i2c_manager.total_bytes += length;
    } else {
        handle->stats.error_count++;
        g_i2c_manager.total_errors++;
    }
    
    handle->state = RTOS_I2C_STATE_READY;
    
    RTOS_I2C_DEBUG_PRINT("I2C%d master receive: addr=0x%02X, %lu bytes in %lu ms, result=%d", 
                         port + 1, device_addr, length, transfer_time, result);
    
    return result;
}

/**
 * @brief 检查设备是否就绪
 */
rtos_result_t rtos_i2c_manager_is_device_ready(rtos_i2c_port_t port,
                                              uint16_t device_addr,
                                              uint32_t trials)
{
    RTOS_I2C_CHECK_PORT(port);
    RTOS_I2C_CHECK_INIT();
    
    rtos_i2c_handle_t *handle = &g_i2c_manager.i2c_handles[port];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 简化实现：尝试发送设备地址 */
    for (uint32_t i = 0; i < trials; i++) {
        rtos_result_t result = rtos_i2c_platform_master_send(port, device_addr, NULL, 0);
        if (result == RTOS_OK) {
            return RTOS_OK;
        }
        rtos_hw_delay_ms(1); /* 短暂延时后重试 */
    }
    
    return RTOS_ERROR_TIMEOUT;
}

/**
 * @brief 获取I2C状态
 */
rtos_i2c_state_t rtos_i2c_manager_get_state(rtos_i2c_port_t port)
{
    if (!g_i2c_manager_initialized || port >= RTOS_I2C_PORT_MAX) {
        return RTOS_I2C_STATE_ERROR;
    }
    
    return g_i2c_manager.i2c_handles[port].state;
}

/**
 * @brief 获取I2C管理器统计信息
 */
uint32_t rtos_i2c_manager_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_i2c_manager_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "I2C Manager Statistics:\n"
        "  Active Ports: %lu/%d\n"
        "  Total Transfers: %lu\n"
        "  Total Bytes: %lu\n"
        "  Total Errors: %lu\n",
        g_i2c_manager.active_ports,
        RTOS_I2C_PORT_MAX,
        g_i2c_manager.total_transfers,
        g_i2c_manager.total_bytes,
        g_i2c_manager.total_errors);
    
    if (len < 0) {
        return 0;
    }
    
    return (len >= (int)size) ? (size - 1) : (uint32_t)len;
}

/* 内部函数实现 */

/**
 * @brief 平台相关I2C初始化
 */
static rtos_result_t rtos_i2c_platform_init_port(rtos_i2c_port_t port, const rtos_i2c_config_t *config)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    I2C_TypeDef *i2c = g_i2c_instances[port];
    
    /* 使能I2C时钟 */
    RCC_APB1PeriphClockCmd(g_i2c_rcc_clocks[port], ENABLE);
    
    /* 配置I2C参数 */
    I2C_InitTypeDef i2c_init;
    I2C_StructInit(&i2c_init);
    
    i2c_init.I2C_ClockSpeed = config->clock_speed;
    i2c_init.I2C_Mode = I2C_Mode_I2C;
    
    if (config->mode == RTOS_I2C_MODE_FAST) {
        i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
    } else {
        i2c_init.I2C_DutyCycle = I2C_DutyCycle_16_9;
    }
    
    i2c_init.I2C_OwnAddress1 = config->own_address;
    i2c_init.I2C_Ack = I2C_Ack_Enable;
    i2c_init.I2C_AcknowledgedAddress = (config->addr_mode == RTOS_I2C_ADDR_7BIT) ? 
                                       I2C_AcknowledgedAddress_7bit : I2C_AcknowledgedAddress_10bit;
    
    I2C_Init(i2c, &i2c_init);
    
    /* 配置双地址 */
    if (config->dual_addr_enable) {
        I2C_DualAddressCmd(i2c, ENABLE);
        I2C_OwnAddress2Config(i2c, config->own_address2);
    }
    
    /* 配置广播呼叫 */
    if (config->general_call_enable) {
        I2C_GeneralCallCmd(i2c, ENABLE);
    }
    
    /* 配置时钟延展 */
    if (config->no_stretch_enable) {
        I2C_StretchClockCmd(i2c, DISABLE);
    }
    
    /* 使能I2C */
    I2C_Cmd(i2c, ENABLE);
    
    return RTOS_OK;
#else
    (void)port;
    (void)config;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关I2C反初始化
 */
static rtos_result_t rtos_i2c_platform_deinit_port(rtos_i2c_port_t port)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    I2C_TypeDef *i2c = g_i2c_instances[port];
    
    /* 禁用I2C */
    I2C_Cmd(i2c, DISABLE);
    
    /* 禁用中断 */
    NVIC_DisableIRQ(g_i2c_ev_irq_numbers[port]);
    NVIC_DisableIRQ(g_i2c_er_irq_numbers[port]);
    
    /* 禁用时钟 */
    RCC_APB1PeriphClockCmd(g_i2c_rcc_clocks[port], DISABLE);
    
    return RTOS_OK;
#else
    (void)port;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关I2C主机发送
 */
static rtos_result_t rtos_i2c_platform_master_send(rtos_i2c_port_t port, uint16_t addr, const uint8_t *data, uint32_t length)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    I2C_TypeDef *i2c = g_i2c_instances[port];
    
    /* 等待总线空闲 */
    uint32_t timeout = rtos_hw_get_system_time_ms() + 100;
    while (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY)) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 发送起始条件 */
    I2C_GenerateSTART(i2c, ENABLE);
    
    /* 等待起始条件发送完成 */
    timeout = rtos_hw_get_system_time_ms() + 10;
    while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT)) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 发送设备地址 */
    I2C_Send7bitAddress(i2c, (uint8_t)addr, I2C_Direction_Transmitter);
    
    /* 等待地址发送完成 */
    timeout = rtos_hw_get_system_time_ms() + 10;
    while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            I2C_GenerateSTOP(i2c, ENABLE);
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 发送数据 */
    for (uint32_t i = 0; i < length; i++) {
        I2C_SendData(i2c, data[i]);
        
        /* 等待数据发送完成 */
        timeout = rtos_hw_get_system_time_ms() + 10;
        while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if (rtos_hw_get_system_time_ms() >= timeout) {
                I2C_GenerateSTOP(i2c, ENABLE);
                return RTOS_ERROR_TIMEOUT;
            }
        }
    }
    
    /* 发送停止条件 */
    I2C_GenerateSTOP(i2c, ENABLE);
    
    return RTOS_OK;
#else
    (void)port;
    (void)addr;
    (void)data;
    (void)length;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关I2C主机接收
 */
static rtos_result_t rtos_i2c_platform_master_receive(rtos_i2c_port_t port, uint16_t addr, uint8_t *buffer, uint32_t length)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    I2C_TypeDef *i2c = g_i2c_instances[port];
    
    /* 等待总线空闲 */
    uint32_t timeout = rtos_hw_get_system_time_ms() + 100;
    while (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY)) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 配置ACK */
    I2C_AcknowledgeConfig(i2c, ENABLE);
    
    /* 发送起始条件 */
    I2C_GenerateSTART(i2c, ENABLE);
    
    /* 等待起始条件发送完成 */
    timeout = rtos_hw_get_system_time_ms() + 10;
    while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT)) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 发送设备地址 */
    I2C_Send7bitAddress(i2c, (uint8_t)addr, I2C_Direction_Receiver);
    
    /* 等待地址发送完成 */
    timeout = rtos_hw_get_system_time_ms() + 10;
    while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            I2C_GenerateSTOP(i2c, ENABLE);
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 接收数据 */
    for (uint32_t i = 0; i < length; i++) {
        if (i == length - 1) {
            /* 最后一个字节：发送NACK */
            I2C_AcknowledgeConfig(i2c, DISABLE);
        }
        
        /* 等待数据接收 */
        timeout = rtos_hw_get_system_time_ms() + 10;
        while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
            if (rtos_hw_get_system_time_ms() >= timeout) {
                I2C_GenerateSTOP(i2c, ENABLE);
                return RTOS_ERROR_TIMEOUT;
            }
        }
        
        buffer[i] = I2C_ReceiveData(i2c);
    }
    
    /* 发送停止条件 */
    I2C_GenerateSTOP(i2c, ENABLE);
    
    return RTOS_OK;
#else
    (void)port;
    (void)addr;
    (void)buffer;
    (void)length;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}