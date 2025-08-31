/**
 * @file uart_abstraction.c
 * @brief RTOS UART抽象模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "uart_abstraction.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_usart.h"
#include "fwlib/inc/stm32f4xx_gpio.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/stm32f4xx_dma.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局UART管理器实例 */
static rtos_uart_manager_t g_uart_manager;
static bool g_uart_manager_initialized = false;

/* STM32F4平台相关数据 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
static USART_TypeDef* const g_uart_instances[RTOS_UART_PORT_MAX] = {
    USART1, USART2, USART3, UART4, UART5, USART6
};

static const uint32_t g_uart_rcc_clocks[RTOS_UART_PORT_MAX] = {
    RCC_APB2Periph_USART1, RCC_APB1Periph_USART2, RCC_APB1Periph_USART3,
    RCC_APB1Periph_UART4, RCC_APB1Periph_UART5, RCC_APB2Periph_USART6
};

static const IRQn_Type g_uart_irq_numbers[RTOS_UART_PORT_MAX] = {
    USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn, USART6_IRQn
};

static const bool g_uart_is_apb2[RTOS_UART_PORT_MAX] = {
    true, false, false, false, false, true  /* USART1,6在APB2，其他在APB1 */
};
#endif

/* 内部函数声明 */
static rtos_result_t rtos_uart_platform_init_port(rtos_uart_port_t port, const rtos_uart_config_t *config);
static rtos_result_t rtos_uart_platform_deinit_port(rtos_uart_port_t port);
static rtos_result_t rtos_uart_platform_send_byte(rtos_uart_port_t port, uint8_t data);
static rtos_result_t rtos_uart_platform_receive_byte(rtos_uart_port_t port, uint8_t *data);
static bool rtos_uart_platform_is_tx_complete(rtos_uart_port_t port);
static bool rtos_uart_platform_is_rx_ready(rtos_uart_port_t port);
static uint32_t rtos_uart_platform_get_errors(rtos_uart_port_t port);
static void rtos_uart_platform_clear_errors(rtos_uart_port_t port);
static void rtos_uart_trigger_event(rtos_uart_port_t port, rtos_uart_event_t event);
static rtos_result_t rtos_uart_process_tx_interrupt(rtos_uart_port_t port);
static rtos_result_t rtos_uart_process_rx_interrupt(rtos_uart_port_t port);

/**
 * @brief 初始化UART管理器
 */
rtos_result_t rtos_uart_manager_init(void)
{
    if (g_uart_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_uart_manager, 0, sizeof(g_uart_manager));
    
    /* 初始化所有UART句柄 */
    for (uint32_t i = 0; i < RTOS_UART_PORT_MAX; i++) {
        g_uart_manager.uart_handles[i].port = (rtos_uart_port_t)i;
        g_uart_manager.uart_handles[i].state = RTOS_UART_STATE_RESET;
        g_uart_manager.uart_handles[i].initialized = false;
    }
    
    g_uart_manager.initialized = true;
    g_uart_manager_initialized = true;
    
    RTOS_UART_DEBUG_PRINT("UART manager initialized");
    return RTOS_OK;
}

/**
 * @brief 反初始化UART管理器
 */
rtos_result_t rtos_uart_manager_deinit(void)
{
    if (!g_uart_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 反初始化所有UART端口 */
    for (uint32_t i = 0; i < RTOS_UART_PORT_MAX; i++) {
        if (g_uart_manager.uart_handles[i].initialized) {
            rtos_uart_manager_deinit_port((rtos_uart_port_t)i);
        }
    }
    
    g_uart_manager_initialized = false;
    
    RTOS_UART_DEBUG_PRINT("UART manager deinitialized");
    return RTOS_OK;
}

/**
 * @brief 获取UART管理器实例
 */
rtos_uart_manager_t* rtos_uart_manager_get_instance(void)
{
    if (!g_uart_manager_initialized) {
        return NULL;
    }
    return &g_uart_manager;
}

/**
 * @brief 初始化UART端口
 */
rtos_result_t rtos_uart_manager_init_port(rtos_uart_port_t port, 
                                         const rtos_uart_config_t *config,
                                         const rtos_uart_buffer_config_t *buffer_config)
{
    RTOS_UART_CHECK_PARAM(config != NULL);
    RTOS_UART_CHECK_PORT(port);
    RTOS_UART_CHECK_PARAM(config->baudrate > 0);
    RTOS_UART_CHECK_INIT();
    
    rtos_uart_handle_t *handle = &g_uart_manager.uart_handles[port];
    
    if (handle->initialized) {
        RTOS_UART_DEBUG_PRINT("UART%d already initialized", port + 1);
        return RTOS_ERROR_ALREADY_INITIALIZED;
    }
    
    /* 保存配置 */
    handle->config = *config;
    
    /* 保存缓冲区配置 */
    if (buffer_config) {
        handle->buffer_config = *buffer_config;
    } else {
        /* 使用默认缓冲区配置 */
        memset(&handle->buffer_config, 0, sizeof(rtos_uart_buffer_config_t));
    }
    
    /* 调用平台相关初始化 */
    rtos_result_t result = rtos_uart_platform_init_port(port, config);
    if (result != RTOS_OK) {
        RTOS_UART_DEBUG_PRINT("UART%d platform init failed: %d", port + 1, result);
        return result;
    }
    
    /* 设置状态 */
    handle->state = RTOS_UART_STATE_READY;
    handle->initialized = true;
    g_uart_manager.active_ports++;
    
    RTOS_UART_DEBUG_PRINT("UART%d initialized (baudrate: %lu)", port + 1, config->baudrate);
    return RTOS_OK;
}

/**
 * @brief 反初始化UART端口
 */
rtos_result_t rtos_uart_manager_deinit_port(rtos_uart_port_t port)
{
    RTOS_UART_CHECK_PORT(port);
    RTOS_UART_CHECK_INIT();
    
    rtos_uart_handle_t *handle = &g_uart_manager.uart_handles[port];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 中止当前传输 */
    rtos_uart_manager_abort(port);
    
    /* 调用平台相关反初始化 */
    rtos_uart_platform_deinit_port(port);
    
    /* 清空事件回调 */
    for (int i = 0; i < RTOS_UART_EVENT_MAX; i++) {
        handle->event_callbacks[i] = NULL;
        handle->event_contexts[i] = NULL;
    }
    
    /* 重置句柄 */
    memset(handle, 0, sizeof(rtos_uart_handle_t));
    handle->port = port;
    handle->state = RTOS_UART_STATE_RESET;
    
    g_uart_manager.active_ports--;
    
    RTOS_UART_DEBUG_PRINT("UART%d deinitialized", port + 1);
    return RTOS_OK;
}

/**
 * @brief 发送数据
 */
rtos_result_t rtos_uart_manager_send(rtos_uart_port_t port, 
                                    const uint8_t *data, 
                                    uint32_t length)
{
    RTOS_UART_CHECK_PORT(port);
    RTOS_UART_CHECK_PARAM(data != NULL);
    RTOS_UART_CHECK_PARAM(length > 0);
    RTOS_UART_CHECK_INIT();
    
    rtos_uart_handle_t *handle = &g_uart_manager.uart_handles[port];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->state == RTOS_UART_STATE_BUSY_TX || 
        handle->state == RTOS_UART_STATE_BUSY_TX_RX) {
        return RTOS_ERROR_BUSY;
    }
    
    /* 设置发送状态 */
    handle->tx_data = data;
    handle->tx_length = length;
    handle->tx_position = 0;
    handle->tx_start_time = rtos_hw_get_system_time_ms();
    
    if (handle->state == RTOS_UART_STATE_BUSY_RX) {
        handle->state = RTOS_UART_STATE_BUSY_TX_RX;
    } else {
        handle->state = RTOS_UART_STATE_BUSY_TX;
    }
    
    if (handle->config.mode == RTOS_UART_MODE_POLLING) {
        /* 轮询模式：同步发送 */
        for (uint32_t i = 0; i < length; i++) {
            rtos_result_t result = rtos_uart_platform_send_byte(port, data[i]);
            if (result != RTOS_OK) {
                handle->stats.tx_errors++;
                handle->state = RTOS_UART_STATE_ERROR;
                return result;
            }
            handle->tx_position++;
        }
        
        /* 发送完成 */
        uint32_t tx_time = rtos_hw_get_system_time_ms() - handle->tx_start_time;
        handle->stats.tx_bytes += length;
        handle->stats.tx_packets++;
        
        if (tx_time > handle->stats.max_tx_time_ms) {
            handle->stats.max_tx_time_ms = tx_time;
        }
        
        g_uart_manager.total_tx_bytes += length;
        
        if (handle->state == RTOS_UART_STATE_BUSY_TX_RX) {
            handle->state = RTOS_UART_STATE_BUSY_RX;
        } else {
            handle->state = RTOS_UART_STATE_READY;
        }
        
        rtos_uart_trigger_event(port, RTOS_UART_EVENT_TX_COMPLETE);
        
        RTOS_UART_DEBUG_PRINT("UART%d sent %lu bytes in %lu ms", port + 1, length, tx_time);
        
    } else {
        /* 中断模式：启动发送 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        USART_TypeDef *uart = g_uart_instances[port];
        
        /* 使能发送完成中断 */
        USART_ITConfig(uart, USART_IT_TXE, ENABLE);
        
        /* 发送第一个字节 */
        if (length > 0) {
            USART_SendData(uart, data[0]);
            handle->tx_position = 1;
        }
#endif
        
        RTOS_UART_DEBUG_PRINT("UART%d async send started: %lu bytes", port + 1, length);
    }
    
    return RTOS_OK;
}

/**
 * @brief 接收数据
 */
rtos_result_t rtos_uart_manager_receive(rtos_uart_port_t port, 
                                       uint8_t *buffer, 
                                       uint32_t length,
                                       uint32_t *received)
{
    RTOS_UART_CHECK_PORT(port);
    RTOS_UART_CHECK_PARAM(buffer != NULL);
    RTOS_UART_CHECK_PARAM(length > 0);
    RTOS_UART_CHECK_INIT();
    
    rtos_uart_handle_t *handle = &g_uart_manager.uart_handles[port];
    
    if (!handle->initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->state == RTOS_UART_STATE_BUSY_RX || 
        handle->state == RTOS_UART_STATE_BUSY_TX_RX) {
        return RTOS_ERROR_BUSY;
    }
    
    uint32_t received_count = 0;
    handle->rx_start_time = rtos_hw_get_system_time_ms();
    
    if (handle->config.mode == RTOS_UART_MODE_POLLING) {
        /* 轮询模式：同步接收 */
        for (uint32_t i = 0; i < length; i++) {
            uint8_t byte;
            
            /* 等待数据或超时 */
            uint32_t start_time = rtos_hw_get_system_time_ms();
            while (!rtos_uart_platform_is_rx_ready(port)) {
                if (rtos_hw_get_system_time_ms() - start_time >= handle->config.timeout_ms) {
                    handle->stats.timeouts++;
                    break;
                }
            }
            
            if (rtos_uart_platform_is_rx_ready(port)) {
                rtos_result_t result = rtos_uart_platform_receive_byte(port, &byte);
                if (result == RTOS_OK) {
                    buffer[i] = byte;
                    received_count++;
                } else {
                    handle->stats.rx_errors++;
                    break;
                }
            } else {
                break; /* 超时 */
            }
        }
        
        /* 接收完成 */
        uint32_t rx_time = rtos_hw_get_system_time_ms() - handle->rx_start_time;
        handle->stats.rx_bytes += received_count;
        
        if (received_count > 0) {
            handle->stats.rx_packets++;
        }
        
        if (rx_time > handle->stats.max_rx_time_ms) {
            handle->stats.max_rx_time_ms = rx_time;
        }
        
        g_uart_manager.total_rx_bytes += received_count;
        
        if (received) {
            *received = received_count;
        }
        
        RTOS_UART_DEBUG_PRINT("UART%d received %lu/%lu bytes in %lu ms", 
                              port + 1, received_count, length, rx_time);
        
    } else {
        /* 中断模式：启动接收 */
        handle->rx_data = buffer;
        handle->rx_length = length;
        handle->rx_position = 0;
        
        if (handle->state == RTOS_UART_STATE_BUSY_TX) {
            handle->state = RTOS_UART_STATE_BUSY_TX_RX;
        } else {
            handle->state = RTOS_UART_STATE_BUSY_RX;
        }
        
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        USART_TypeDef *uart = g_uart_instances[port];
        
        /* 使能接收中断 */
        USART_ITConfig(uart, USART_IT_RXNE, ENABLE);
#endif
        
        RTOS_UART_DEBUG_PRINT("UART%d async receive started: %lu bytes", port + 1, length);
    }
    
    return RTOS_OK;
}

/**
 * @brief 获取UART状态
 */
rtos_uart_state_t rtos_uart_manager_get_state(rtos_uart_port_t port)
{
    if (!g_uart_manager_initialized || port >= RTOS_UART_PORT_MAX) {
        return RTOS_UART_STATE_ERROR;
    }
    
    return g_uart_manager.uart_handles[port].state;
}

/**
 * @brief UART中断处理函数
 */
void rtos_uart_manager_interrupt_handler(rtos_uart_port_t port)
{
    if (!g_uart_manager_initialized || port >= RTOS_UART_PORT_MAX) {
        return;
    }
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    
    /* 处理发送中断 */
    if (USART_GetITStatus(uart, USART_IT_TXE) != RESET) {
        rtos_uart_process_tx_interrupt(port);
    }
    
    /* 处理接收中断 */
    if (USART_GetITStatus(uart, USART_IT_RXNE) != RESET) {
        rtos_uart_process_rx_interrupt(port);
    }
    
    /* 处理错误中断 */
    if (USART_GetITStatus(uart, USART_IT_PE) != RESET ||
        USART_GetITStatus(uart, USART_IT_FE) != RESET ||
        USART_GetITStatus(uart, USART_IT_NE) != RESET ||
        USART_GetITStatus(uart, USART_IT_ORE) != RESET) {
        
        uint32_t errors = rtos_uart_platform_get_errors(port);
        g_uart_manager.uart_handles[port].stats.last_error = errors;
        g_uart_manager.uart_handles[port].stats.rx_errors++;
        g_uart_manager.total_errors++;
        
        rtos_uart_platform_clear_errors(port);
        rtos_uart_trigger_event(port, RTOS_UART_EVENT_ERROR);
        
        RTOS_UART_DEBUG_PRINT("UART%d error: 0x%08lx", port + 1, errors);
    }
#endif
}

/**
 * @brief 获取UART管理器统计信息
 */
uint32_t rtos_uart_manager_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_uart_manager_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "UART Manager Statistics:\n"
        "  Active Ports: %lu/%d\n"
        "  Total TX Bytes: %lu\n"
        "  Total RX Bytes: %lu\n"
        "  Total Errors: %lu\n"
        "  Port Details:\n",
        g_uart_manager.active_ports,
        RTOS_UART_PORT_MAX,
        g_uart_manager.total_tx_bytes,
        g_uart_manager.total_rx_bytes,
        g_uart_manager.total_errors);
    
    /* 显示每个端口的详细信息 */
    for (uint32_t i = 0; i < RTOS_UART_PORT_MAX; i++) {
        rtos_uart_handle_t *handle = &g_uart_manager.uart_handles[i];
        
        if (handle->initialized) {
            int port_len = snprintf(buffer + len, size - len,
                "    UART%lu: %lu bps, TX:%lu RX:%lu, State:%d\n",
                i + 1,
                handle->config.baudrate,
                handle->stats.tx_bytes,
                handle->stats.rx_bytes,
                handle->state);
                
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
 * @brief 平台相关UART初始化
 */
static rtos_result_t rtos_uart_platform_init_port(rtos_uart_port_t port, const rtos_uart_config_t *config)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    
    /* 使能UART时钟 */
    if (g_uart_is_apb2[port]) {
        RCC_APB2PeriphClockCmd(g_uart_rcc_clocks[port], ENABLE);
    } else {
        RCC_APB1PeriphClockCmd(g_uart_rcc_clocks[port], ENABLE);
    }
    
    /* 配置UART参数 */
    USART_InitTypeDef usart_init;
    USART_StructInit(&usart_init);
    
    usart_init.USART_BaudRate = config->baudrate;
    usart_init.USART_WordLength = (config->databits == RTOS_UART_DATABITS_9) ? 
                                  USART_WordLength_9b : USART_WordLength_8b;
    usart_init.USART_StopBits = (config->stopbits == RTOS_UART_STOPBITS_2) ? 
                                USART_StopBits_2 : USART_StopBits_1;
    
    switch (config->parity) {
        case RTOS_UART_PARITY_NONE:
            usart_init.USART_Parity = USART_Parity_No;
            break;
        case RTOS_UART_PARITY_EVEN:
            usart_init.USART_Parity = USART_Parity_Even;
            break;
        case RTOS_UART_PARITY_ODD:
            usart_init.USART_Parity = USART_Parity_Odd;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    switch (config->flowctrl) {
        case RTOS_UART_FLOWCTRL_NONE:
            usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
            break;
        case RTOS_UART_FLOWCTRL_RTS:
            usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
            break;
        case RTOS_UART_FLOWCTRL_CTS:
            usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
            break;
        case RTOS_UART_FLOWCTRL_RTS_CTS:
            usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(uart, &usart_init);
    
    /* 配置中断 */
    if (config->mode == RTOS_UART_MODE_INTERRUPT) {
        NVIC_InitTypeDef nvic_init;
        nvic_init.NVIC_IRQChannel = g_uart_irq_numbers[port];
        nvic_init.NVIC_IRQChannelPreemptionPriority = 3;
        nvic_init.NVIC_IRQChannelSubPriority = 0;
        nvic_init.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic_init);
    }
    
    /* 使能UART */
    USART_Cmd(uart, ENABLE);
    
    return RTOS_OK;
#else
    (void)port;
    (void)config;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关UART反初始化
 */
static rtos_result_t rtos_uart_platform_deinit_port(rtos_uart_port_t port)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    
    /* 禁用UART */
    USART_Cmd(uart, DISABLE);
    
    /* 禁用中断 */
    NVIC_DisableIRQ(g_uart_irq_numbers[port]);
    
    /* 禁用时钟 */
    if (g_uart_is_apb2[port]) {
        RCC_APB2PeriphClockCmd(g_uart_rcc_clocks[port], DISABLE);
    } else {
        RCC_APB1PeriphClockCmd(g_uart_rcc_clocks[port], DISABLE);
    }
    
    return RTOS_OK;
#else
    (void)port;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关发送字节
 */
static rtos_result_t rtos_uart_platform_send_byte(rtos_uart_port_t port, uint8_t data)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    
    /* 等待发送缓冲区空 */
    uint32_t timeout = rtos_hw_get_system_time_ms() + 10; /* 10ms超时 */
    while (USART_GetFlagStatus(uart, USART_FLAG_TXE) == RESET) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 发送数据 */
    USART_SendData(uart, data);
    
    /* 等待发送完成 */
    timeout = rtos_hw_get_system_time_ms() + 10;
    while (USART_GetFlagStatus(uart, USART_FLAG_TC) == RESET) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    return RTOS_OK;
#else
    (void)port;
    (void)data;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关接收字节
 */
static rtos_result_t rtos_uart_platform_receive_byte(rtos_uart_port_t port, uint8_t *data)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    
    if (USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
        *data = (uint8_t)USART_ReceiveData(uart);
        return RTOS_OK;
    }
    
    return RTOS_ERROR_NO_DATA;
#else
    (void)port;
    *data = 0;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 检查发送是否完成
 */
static bool rtos_uart_platform_is_tx_complete(rtos_uart_port_t port)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    return (USART_GetFlagStatus(uart, USART_FLAG_TC) != RESET);
#else
    (void)port;
    return true;
#endif
}

/**
 * @brief 检查是否有接收数据
 */
static bool rtos_uart_platform_is_rx_ready(rtos_uart_port_t port)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    return (USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET);
#else
    (void)port;
    return false;
#endif
}

/**
 * @brief 获取UART错误状态
 */
static uint32_t rtos_uart_platform_get_errors(rtos_uart_port_t port)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    uint32_t errors = RTOS_UART_ERROR_NONE;
    
    if (USART_GetFlagStatus(uart, USART_FLAG_PE) != RESET) {
        errors |= RTOS_UART_ERROR_PARITY;
    }
    if (USART_GetFlagStatus(uart, USART_FLAG_FE) != RESET) {
        errors |= RTOS_UART_ERROR_FRAME;
    }
    if (USART_GetFlagStatus(uart, USART_FLAG_NE) != RESET) {
        errors |= RTOS_UART_ERROR_NOISE;
    }
    if (USART_GetFlagStatus(uart, USART_FLAG_ORE) != RESET) {
        errors |= RTOS_UART_ERROR_OVERRUN;
    }
    
    return errors;
#else
    (void)port;
    return RTOS_UART_ERROR_NONE;
#endif
}

/**
 * @brief 清除UART错误
 */
static void rtos_uart_platform_clear_errors(rtos_uart_port_t port)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    
    /* 清除错误标志 */
    USART_ClearFlag(uart, USART_FLAG_PE | USART_FLAG_FE | USART_FLAG_NE | USART_FLAG_ORE);
#else
    (void)port;
#endif
}

/**
 * @brief 触发UART事件
 */
static void rtos_uart_trigger_event(rtos_uart_port_t port, rtos_uart_event_t event)
{
    if (port >= RTOS_UART_PORT_MAX || event >= RTOS_UART_EVENT_MAX) {
        return;
    }
    
    rtos_uart_handle_t *handle = &g_uart_manager.uart_handles[port];
    
    if (handle->event_callbacks[event]) {
        handle->event_callbacks[event](port, event, handle->event_contexts[event]);
    }
}

/**
 * @brief 处理发送中断
 */
static rtos_result_t rtos_uart_process_tx_interrupt(rtos_uart_port_t port)
{
    rtos_uart_handle_t *handle = &g_uart_manager.uart_handles[port];
    
    if (handle->tx_position < handle->tx_length) {
        /* 继续发送 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        USART_TypeDef *uart = g_uart_instances[port];
        USART_SendData(uart, handle->tx_data[handle->tx_position]);
#endif
        handle->tx_position++;
    } else {
        /* 发送完成 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        USART_TypeDef *uart = g_uart_instances[port];
        USART_ITConfig(uart, USART_IT_TXE, DISABLE);
#endif
        
        uint32_t tx_time = rtos_hw_get_system_time_ms() - handle->tx_start_time;
        handle->stats.tx_bytes += handle->tx_length;
        handle->stats.tx_packets++;
        g_uart_manager.total_tx_bytes += handle->tx_length;
        
        if (tx_time > handle->stats.max_tx_time_ms) {
            handle->stats.max_tx_time_ms = tx_time;
        }
        
        if (handle->state == RTOS_UART_STATE_BUSY_TX_RX) {
            handle->state = RTOS_UART_STATE_BUSY_RX;
        } else {
            handle->state = RTOS_UART_STATE_READY;
        }
        
        rtos_uart_trigger_event(port, RTOS_UART_EVENT_TX_COMPLETE);
        
        RTOS_UART_DEBUG_PRINT("UART%d TX complete: %lu bytes in %lu ms", 
                              port + 1, handle->tx_length, tx_time);
    }
    
    return RTOS_OK;
}

/**
 * @brief 处理接收中断
 */
static rtos_result_t rtos_uart_process_rx_interrupt(rtos_uart_port_t port)
{
    rtos_uart_handle_t *handle = &g_uart_manager.uart_handles[port];
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    USART_TypeDef *uart = g_uart_instances[port];
    
    if (handle->rx_position < handle->rx_length) {
        /* 继续接收 */
        handle->rx_data[handle->rx_position] = (uint8_t)USART_ReceiveData(uart);
        handle->rx_position++;
        
        if (handle->rx_position >= handle->rx_length) {
            /* 接收完成 */
            USART_ITConfig(uart, USART_IT_RXNE, DISABLE);
            
            uint32_t rx_time = rtos_hw_get_system_time_ms() - handle->rx_start_time;
            handle->stats.rx_bytes += handle->rx_length;
            handle->stats.rx_packets++;
            g_uart_manager.total_rx_bytes += handle->rx_length;
            
            if (rx_time > handle->stats.max_rx_time_ms) {
                handle->stats.max_rx_time_ms = rx_time;
            }
            
            if (handle->state == RTOS_UART_STATE_BUSY_TX_RX) {
                handle->state = RTOS_UART_STATE_BUSY_TX;
            } else {
                handle->state = RTOS_UART_STATE_READY;
            }
            
            rtos_uart_trigger_event(port, RTOS_UART_EVENT_RX_COMPLETE);
            
            RTOS_UART_DEBUG_PRINT("UART%d RX complete: %lu bytes in %lu ms", 
                                  port + 1, handle->rx_length, rx_time);
        }
    }
#endif
    
    return RTOS_OK;
}