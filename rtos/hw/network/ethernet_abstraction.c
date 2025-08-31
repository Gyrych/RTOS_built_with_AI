/**
 * @file ethernet_abstraction.c
 * @brief RTOS以太网抽象模块实现 - 简化版本
 * @author Assistant
 * @date 2024
 */

#include "ethernet_abstraction.h"
#include "../hw_config.h"
#include "../hw_abstraction.h"
#include "../../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 全局以太网管理器实例 */
static rtos_ethernet_manager_t g_ethernet_manager;
static bool g_ethernet_manager_initialized = false;

/**
 * @brief 初始化以太网管理器
 */
rtos_result_t rtos_ethernet_manager_init(const rtos_eth_config_t *config,
                                        uint32_t rx_buffer_count,
                                        uint32_t tx_buffer_count)
{
    if (g_ethernet_manager_initialized) {
        return RTOS_OK;
    }
    
    if (!config) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 清零管理器结构 */
    memset(&g_ethernet_manager, 0, sizeof(g_ethernet_manager));
    
    /* 保存配置 */
    g_ethernet_manager.config = *config;
    g_ethernet_manager.state = RTOS_ETH_STATE_RESET;
    g_ethernet_manager.link_state = RTOS_ETH_LINK_DOWN;
    
    /* 分配接收缓冲区 */
    if (rx_buffer_count > 0) {
        g_ethernet_manager.rx_packets = malloc(sizeof(rtos_eth_packet_t) * rx_buffer_count);
        if (!g_ethernet_manager.rx_packets) {
            RTOS_ETH_DEBUG_PRINT("Failed to allocate RX buffers");
            return RTOS_ERROR_NO_MEMORY;
        }
        g_ethernet_manager.rx_buffer_count = rx_buffer_count;
        memset(g_ethernet_manager.rx_packets, 0, sizeof(rtos_eth_packet_t) * rx_buffer_count);
    }
    
    /* 分配发送缓冲区 */
    if (tx_buffer_count > 0) {
        g_ethernet_manager.tx_packets = malloc(sizeof(rtos_eth_packet_t) * tx_buffer_count);
        if (!g_ethernet_manager.tx_packets) {
            if (g_ethernet_manager.rx_packets) {
                free(g_ethernet_manager.rx_packets);
            }
            RTOS_ETH_DEBUG_PRINT("Failed to allocate TX buffers");
            return RTOS_ERROR_NO_MEMORY;
        }
        g_ethernet_manager.tx_buffer_count = tx_buffer_count;
        memset(g_ethernet_manager.tx_packets, 0, sizeof(rtos_eth_packet_t) * tx_buffer_count);
    }
    
    g_ethernet_manager.state = RTOS_ETH_STATE_READY;
    g_ethernet_manager.link_check_interval_ms = 1000;
    g_ethernet_manager.initialized = true;
    g_ethernet_manager_initialized = true;
    
    RTOS_ETH_DEBUG_PRINT("Ethernet manager initialized (RX:%lu, TX:%lu)", 
                         rx_buffer_count, tx_buffer_count);
    
    return RTOS_OK;
}

/**
 * @brief 启动以太网
 */
rtos_result_t rtos_ethernet_manager_start(void)
{
    if (!g_ethernet_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (g_ethernet_manager.state == RTOS_ETH_STATE_STARTED) {
        return RTOS_OK;
    }
    
    /* 简化实现：假设启动成功 */
    g_ethernet_manager.state = RTOS_ETH_STATE_STARTED;
    g_ethernet_manager.link_state = RTOS_ETH_LINK_UP;
    
    RTOS_ETH_DEBUG_PRINT("Ethernet started");
    return RTOS_OK;
}

/**
 * @brief 停止以太网
 */
rtos_result_t rtos_ethernet_manager_stop(void)
{
    if (!g_ethernet_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    g_ethernet_manager.state = RTOS_ETH_STATE_READY;
    g_ethernet_manager.link_state = RTOS_ETH_LINK_DOWN;
    
    RTOS_ETH_DEBUG_PRINT("Ethernet stopped");
    return RTOS_OK;
}

/**
 * @brief 发送以太网包
 */
rtos_result_t rtos_ethernet_manager_send_packet(const rtos_eth_packet_t *packet)
{
    if (!packet || !packet->data || packet->length == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!g_ethernet_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (g_ethernet_manager.state != RTOS_ETH_STATE_STARTED) {
        return RTOS_ERROR_INVALID_STATE;
    }
    
    /* 简化实现：更新统计信息 */
    g_ethernet_manager.stats.tx_packets++;
    g_ethernet_manager.stats.tx_bytes += packet->length;
    
    RTOS_ETH_DEBUG_PRINT("Packet sent: %lu bytes", packet->length);
    return RTOS_OK;
}

/**
 * @brief 获取链路状态
 */
rtos_eth_link_state_t rtos_ethernet_manager_get_link_state(void)
{
    if (!g_ethernet_manager_initialized) {
        return RTOS_ETH_LINK_DOWN;
    }
    
    return g_ethernet_manager.link_state;
}

/**
 * @brief 获取以太网统计信息
 */
rtos_result_t rtos_ethernet_manager_get_stats(rtos_eth_stats_t *stats)
{
    if (!stats) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!g_ethernet_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    *stats = g_ethernet_manager.stats;
    return RTOS_OK;
}

/**
 * @brief 以太网周期性任务
 */
void rtos_ethernet_manager_periodic_task(void)
{
    if (!g_ethernet_manager_initialized) {
        return;
    }
    
    uint32_t current_time = rtos_hw_get_system_time_ms();
    
    /* 定期检查链路状态 */
    if (current_time - g_ethernet_manager.last_link_check_time >= g_ethernet_manager.link_check_interval_ms) {
        /* 简化实现：假设链路始终连接 */
        if (g_ethernet_manager.state == RTOS_ETH_STATE_STARTED) {
            g_ethernet_manager.link_state = RTOS_ETH_LINK_UP;
        }
        
        g_ethernet_manager.last_link_check_time = current_time;
    }
}