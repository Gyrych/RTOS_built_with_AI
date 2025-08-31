/**
 * @file spi_abstraction.c
 * @brief RTOS SPI抽象模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "spi_abstraction.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_spi.h"
#include "fwlib/inc/stm32f4xx_gpio.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局SPI管理器实例 */
static rtos_spi_manager_t g_spi_manager;
static bool g_spi_manager_initialized = false;

/* STM32F4平台相关数据 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
static SPI_TypeDef* const g_spi_instances[RTOS_SPI_PORT_MAX] = {
    SPI1, SPI2, SPI3
};

static const uint32_t g_spi_rcc_clocks[RTOS_SPI_PORT_MAX] = {
    RCC_APB2Periph_SPI1, RCC_APB1Periph_SPI2, RCC_APB1Periph_SPI3
};

static const IRQn_Type g_spi_irq_numbers[RTOS_SPI_PORT_MAX] = {
    SPI1_IRQn, SPI2_IRQn, SPI3_IRQn
};

static const bool g_spi_is_apb2[RTOS_SPI_PORT_MAX] = {
    true, false, false  /* SPI1在APB2，SPI2/3在APB1 */
};
#endif

/* 内部函数声明 */
static rtos_result_t rtos_spi_platform_init_port(rtos_spi_port_t port, const rtos_spi_config_t *config);
static rtos_result_t rtos_spi_platform_deinit_port(rtos_spi_port_t port);
static rtos_result_t rtos_spi_platform_transfer_byte(rtos_spi_port_t port, uint8_t tx_data, uint8_t *rx_data);
static void rtos_spi_trigger_event(rtos_spi_port_t port, rtos_spi_event_t event);

/**
 * @brief 初始化SPI管理器
 */
rtos_result_t rtos_spi_manager_init(void)
{
    if (g_spi_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_spi_manager, 0, sizeof(g_spi_manager));
    
    /* 初始化所有SPI句柄 */
    for (uint32_t i = 0; i < RTOS_SPI_PORT_MAX; i++) {
        g_spi_manager.spi_handles[i].port = (rtos_spi_port_t)i;
        g_spi_manager.spi_handles[i].state = RTOS_SPI_STATE_RESET;
        g_spi_manager.spi_handles[i].initialized = false;
    }
    
    g_spi_manager.initialized = true;
    g_spi_manager_initialized = true;
    
    RTOS_SPI_DEBUG_PRINT("SPI manager initialized");
    return RTOS_OK;
}

/**
 * @brief 反初始化SPI管理器
 */
rtos_result_t rtos_spi_manager_deinit(void)
{
    if (!g_spi_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 反初始化所有SPI端口 */
    for (uint32_t i = 0; i < RTOS_SPI_PORT_MAX; i++) {
        if (g_spi_manager.spi_handles[i].initialized) {
            rtos_spi_manager_deinit_port((rtos_spi_port_t)i);
        }
    }
    
    g_spi_manager_initialized = false;
    
    RTOS_SPI_DEBUG_PRINT("SPI manager deinitialized");
    return RTOS_OK;
}

/**
 * @brief 获取SPI管理器实例
 */
rtos_spi_manager_t* rtos_spi_manager_get_instance(void)
{
    if (!g_spi_manager_initialized) {
        return NULL;
    }
    return &g_spi_manager;
}

/**
 * @brief 初始化SPI端口
 */
rtos_result_t rtos_spi_manager_init_port(rtos_spi_port_t port, 
                                        const rtos_spi_config_t *config,
                                        uint32_t max_devices)
{
    RTOS_SPI_CHECK_PARAM(config != NULL);
    RTOS_SPI_CHECK_PORT(port);
    RTOS_SPI_CHECK_INIT();
    
    rtos_spi_handle_t *handle = &g_spi_manager.spi_handles[port];
    
    if (handle->initialized) {
        RTOS_SPI_DEBUG_PRINT("SPI%d already initialized", port + 1);
        return RTOS_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置 */
    handle->config = *config;
    
    /* 分配设备数组 */
    if (max_devices > 0) {
        handle->devices = malloc(sizeof(rtos_spi_device_config_t) * max_devices);
        if (!handle->devices) {
            RTOS_SPI_DEBUG_PRINT("Failed to allocate SPI device array");
            return RTOS_ERROR_NO_MEMORY;
        }
        handle->max_devices = max_devices;
        memset(handle->devices, 0, sizeof(rtos_spi_device_config_t) * max_devices);
    }
    
    /* 调用平台相关初始化 */
    rtos_result_t result = rtos_spi_platform_init_port(port, config);
    if (result != RTOS_OK) {
        if (handle->devices) {
            free(handle->devices);
            handle->devices = NULL;
        }
        RTOS_SPI_DEBUG_PRINT("SPI%d platform init failed: %d", port + 1, result);
        return result;
    }
    
    /* 设置状态 */
    handle->state = RTOS_SPI_STATE_READY;
    handle->initialized = true;
    g_spi_manager.active_ports++;
    
    RTOS_SPI_DEBUG_PRINT("SPI%d initialized (max devices: %lu)", port + 1, max_devices);
    return RTOS_OK;
}

/**
 * @brief 反初始化SPI端口
 */
rtos_result_t rtos_spi_manager_deinit_port(rtos_spi_port_t port)
{
    RTOS_SPI_CHECK_PORT(port);
    RTOS_SPI_CHECK_INIT();
    
    rtos_spi_handle_t *handle = &g_spi_manager.spi_handles[port];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 中止当前传输 */
    rtos_spi_manager_abort(port);
    
    /* 调用平台相关反初始化 */
    rtos_spi_platform_deinit_port(port);
    
    /* 释放设备数组 */
    if (handle->devices) {
        free(handle->devices);
        handle->devices = NULL;
    }
    
    /* 清空事件回调 */
    for (int i = 0; i < RTOS_SPI_EVENT_MAX; i++) {
        handle->event_callbacks[i] = NULL;
        handle->event_contexts[i] = NULL;
    }
    
    /* 重置句柄 */
    memset(handle, 0, sizeof(rtos_spi_handle_t));
    handle->port = port;
    handle->state = RTOS_SPI_STATE_RESET;
    
    g_spi_manager.active_ports--;
    
    RTOS_SPI_DEBUG_PRINT("SPI%d deinitialized", port + 1);
    return RTOS_OK;
}

/**
 * @brief SPI传输数据
 */
rtos_result_t rtos_spi_manager_transfer(rtos_spi_port_t port,
                                       uint32_t device_id,
                                       const uint8_t *tx_data,
                                       uint8_t *rx_data,
                                       uint32_t length)
{
    RTOS_SPI_CHECK_PORT(port);
    RTOS_SPI_CHECK_PARAM(tx_data != NULL || rx_data != NULL);
    RTOS_SPI_CHECK_PARAM(length > 0);
    RTOS_SPI_CHECK_INIT();
    
    rtos_spi_handle_t *handle = &g_spi_manager.spi_handles[port];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->state != RTOS_SPI_STATE_READY) {
        return RTOS_ERROR_BUSY;
    }
    
    /* 检查设备ID */
    if (device_id >= handle->device_count) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    handle->state = RTOS_SPI_STATE_BUSY_TX_RX;
    handle->transfer_start_time = rtos_hw_get_system_time_ms();
    
    /* 执行传输 */
    rtos_result_t result = RTOS_OK;
    for (uint32_t i = 0; i < length; i++) {
        uint8_t tx_byte = tx_data ? tx_data[i] : 0xFF;
        uint8_t rx_byte = 0;
        
        result = rtos_spi_platform_transfer_byte(port, tx_byte, &rx_byte);
        if (result != RTOS_OK) {
            handle->stats.tx_errors++;
            break;
        }
        
        if (rx_data) {
            rx_data[i] = rx_byte;
        }
    }
    
    /* 更新统计信息 */
    uint32_t transfer_time = rtos_hw_get_system_time_ms() - handle->transfer_start_time;
    
    if (result == RTOS_OK) {
        if (tx_data) {
            handle->stats.tx_bytes += length;
            handle->stats.tx_packets++;
        }
        if (rx_data) {
            handle->stats.rx_bytes += length;
            handle->stats.rx_packets++;
        }
        
        if (transfer_time > handle->stats.max_transfer_time_ms) {
            handle->stats.max_transfer_time_ms = transfer_time;
        }
        
        rtos_spi_trigger_event(port, RTOS_SPI_EVENT_TX_RX_COMPLETE);
    } else {
        rtos_spi_trigger_event(port, RTOS_SPI_EVENT_ERROR);
    }
    
    handle->state = RTOS_SPI_STATE_READY;
    
    RTOS_SPI_DEBUG_PRINT("SPI%d transfer: %lu bytes in %lu ms", port + 1, length, transfer_time);
    return result;
}

/**
 * @brief 获取SPI状态
 */
rtos_spi_state_t rtos_spi_manager_get_state(rtos_spi_port_t port)
{
    if (!g_spi_manager_initialized || port >= RTOS_SPI_PORT_MAX) {
        return RTOS_SPI_STATE_ERROR;
    }
    
    return g_spi_manager.spi_handles[port].state;
}

/**
 * @brief 中止SPI传输
 */
rtos_result_t rtos_spi_manager_abort(rtos_spi_port_t port)
{
    RTOS_SPI_CHECK_PORT(port);
    RTOS_SPI_CHECK_INIT();
    
    rtos_spi_handle_t *handle = &g_spi_manager.spi_handles[port];
    
    if (handle->state == RTOS_SPI_STATE_READY) {
        return RTOS_OK;
    }
    
    handle->state = RTOS_SPI_STATE_READY;
    
    RTOS_SPI_DEBUG_PRINT("SPI%d transfer aborted", port + 1);
    return RTOS_OK;
}

/**
 * @brief 获取SPI管理器统计信息
 */
uint32_t rtos_spi_manager_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_spi_manager_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "SPI Manager Statistics:\n"
        "  Active Ports: %lu/%d\n"
        "  Total TX Bytes: %lu\n"
        "  Total RX Bytes: %lu\n"
        "  Total Errors: %lu\n",
        g_spi_manager.active_ports,
        RTOS_SPI_PORT_MAX,
        g_spi_manager.total_tx_bytes,
        g_spi_manager.total_rx_bytes,
        g_spi_manager.total_errors);
    
    if (len < 0) {
        return 0;
    }
    
    return (len >= (int)size) ? (size - 1) : (uint32_t)len;
}

/* 内部函数实现 */

/**
 * @brief 平台相关SPI初始化
 */
static rtos_result_t rtos_spi_platform_init_port(rtos_spi_port_t port, const rtos_spi_config_t *config)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    SPI_TypeDef *spi = g_spi_instances[port];
    
    /* 使能SPI时钟 */
    if (g_spi_is_apb2[port]) {
        RCC_APB2PeriphClockCmd(g_spi_rcc_clocks[port], ENABLE);
    } else {
        RCC_APB1PeriphClockCmd(g_spi_rcc_clocks[port], ENABLE);
    }
    
    /* 配置SPI参数 */
    SPI_InitTypeDef spi_init;
    SPI_StructInit(&spi_init);
    
    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = (config->mode == RTOS_SPI_MODE_MASTER) ? SPI_Mode_Master : SPI_Mode_Slave;
    spi_init.SPI_DataSize = (config->datasize == RTOS_SPI_DATASIZE_16BIT) ? SPI_DataSize_16b : SPI_DataSize_8b;
    spi_init.SPI_CPOL = (config->cpol == RTOS_SPI_CPOL_HIGH) ? SPI_CPOL_High : SPI_CPOL_Low;
    spi_init.SPI_CPHA = (config->cpha == RTOS_SPI_CPHA_2EDGE) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
    
    switch (config->nss) {
        case RTOS_SPI_NSS_SOFT:
            spi_init.SPI_NSS = SPI_NSS_Soft;
            break;
        case RTOS_SPI_NSS_HARD_INPUT:
            spi_init.SPI_NSS = SPI_NSS_Hard;
            break;
        case RTOS_SPI_NSS_HARD_OUTPUT:
            spi_init.SPI_NSS = SPI_NSS_Hard;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 设置波特率预分频 */
    switch (config->baudrate_prescaler) {
        case 2:   spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; break;
        case 4:   spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; break;
        case 8:   spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; break;
        case 16:  spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; break;
        case 32:  spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; break;
        case 64:  spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64; break;
        case 128: spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128; break;
        case 256: spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; break;
        default:  spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; break;
    }
    
    spi_init.SPI_FirstBit = config->first_bit_msb ? SPI_FirstBit_MSB : SPI_FirstBit_LSB;
    spi_init.SPI_CRCPolynomial = config->crc_polynomial;
    
    SPI_Init(spi, &spi_init);
    
    /* 配置CRC */
    if (config->crc_enable) {
        SPI_CalculateCRC(spi, ENABLE);
    } else {
        SPI_CalculateCRC(spi, DISABLE);
    }
    
    /* 配置中断 */
    if (config->transfer_mode == RTOS_SPI_TRANSFER_INTERRUPT) {
        NVIC_InitTypeDef nvic_init;
        nvic_init.NVIC_IRQChannel = g_spi_irq_numbers[port];
        nvic_init.NVIC_IRQChannelPreemptionPriority = 3;
        nvic_init.NVIC_IRQChannelSubPriority = 0;
        nvic_init.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic_init);
    }
    
    /* 使能SPI */
    SPI_Cmd(spi, ENABLE);
    
    return RTOS_OK;
#else
    (void)port;
    (void)config;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关SPI反初始化
 */
static rtos_result_t rtos_spi_platform_deinit_port(rtos_spi_port_t port)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    SPI_TypeDef *spi = g_spi_instances[port];
    
    /* 禁用SPI */
    SPI_Cmd(spi, DISABLE);
    
    /* 禁用中断 */
    NVIC_DisableIRQ(g_spi_irq_numbers[port]);
    
    /* 禁用时钟 */
    if (g_spi_is_apb2[port]) {
        RCC_APB2PeriphClockCmd(g_spi_rcc_clocks[port], DISABLE);
    } else {
        RCC_APB1PeriphClockCmd(g_spi_rcc_clocks[port], DISABLE);
    }
    
    return RTOS_OK;
#else
    (void)port;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关SPI字节传输
 */
static rtos_result_t rtos_spi_platform_transfer_byte(rtos_spi_port_t port, uint8_t tx_data, uint8_t *rx_data)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    SPI_TypeDef *spi = g_spi_instances[port];
    
    /* 等待发送缓冲区空 */
    uint32_t timeout = rtos_hw_get_system_time_ms() + 10;
    while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 发送数据 */
    SPI_I2S_SendData(spi, tx_data);
    
    /* 等待接收缓冲区非空 */
    timeout = rtos_hw_get_system_time_ms() + 10;
    while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) == RESET) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 接收数据 */
    if (rx_data) {
        *rx_data = (uint8_t)SPI_I2S_ReceiveData(spi);
    } else {
        (void)SPI_I2S_ReceiveData(spi); /* 清空接收缓冲区 */
    }
    
    return RTOS_OK;
#else
    (void)port;
    (void)tx_data;
    if (rx_data) *rx_data = 0;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 触发SPI事件
 */
static void rtos_spi_trigger_event(rtos_spi_port_t port, rtos_spi_event_t event)
{
    if (port >= RTOS_SPI_PORT_MAX || event >= RTOS_SPI_EVENT_MAX) {
        return;
    }
    
    rtos_spi_handle_t *handle = &g_spi_manager.spi_handles[port];
    
    if (handle->event_callbacks[event]) {
        handle->event_callbacks[event](port, event, handle->event_contexts[event]);
    }
}