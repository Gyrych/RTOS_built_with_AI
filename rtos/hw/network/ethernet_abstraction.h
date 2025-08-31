/**
 * @file ethernet_abstraction.h
 * @brief RTOS以太网抽象模块 - 面向对象的以太网通信管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_ETHERNET_ABSTRACTION_H__
#define __RTOS_ETHERNET_ABSTRACTION_H__

#include "../../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 以太网速度定义 */
typedef enum {
    RTOS_ETH_SPEED_10M = 0,             /**< 10Mbps */
    RTOS_ETH_SPEED_100M,                /**< 100Mbps */
    RTOS_ETH_SPEED_1000M,               /**< 1000Mbps */
    RTOS_ETH_SPEED_AUTO,                /**< 自动协商 */
    RTOS_ETH_SPEED_MAX
} rtos_eth_speed_t;

/* 以太网双工模式定义 */
typedef enum {
    RTOS_ETH_DUPLEX_HALF = 0,           /**< 半双工 */
    RTOS_ETH_DUPLEX_FULL,               /**< 全双工 */
    RTOS_ETH_DUPLEX_AUTO,               /**< 自动协商 */
    RTOS_ETH_DUPLEX_MAX
} rtos_eth_duplex_t;

/* 以太网PHY接口定义 */
typedef enum {
    RTOS_ETH_PHY_MII = 0,               /**< MII接口 */
    RTOS_ETH_PHY_RMII,                  /**< RMII接口 */
    RTOS_ETH_PHY_RGMII,                 /**< RGMII接口 */
    RTOS_ETH_PHY_MAX
} rtos_eth_phy_interface_t;

/* 以太网状态定义 */
typedef enum {
    RTOS_ETH_STATE_RESET = 0,           /**< 复位状态 */
    RTOS_ETH_STATE_READY,               /**< 就绪状态 */
    RTOS_ETH_STATE_BUSY,                /**< 忙状态 */
    RTOS_ETH_STATE_STARTED,             /**< 已启动 */
    RTOS_ETH_STATE_ERROR,               /**< 错误状态 */
    RTOS_ETH_STATE_MAX
} rtos_eth_state_t;

/* 以太网链路状态定义 */
typedef enum {
    RTOS_ETH_LINK_DOWN = 0,             /**< 链路断开 */
    RTOS_ETH_LINK_UP,                   /**< 链路连接 */
    RTOS_ETH_LINK_UNKNOWN,              /**< 链路状态未知 */
    RTOS_ETH_LINK_MAX
} rtos_eth_link_state_t;

/* 以太网配置结构 */
typedef struct {
    uint8_t mac_address[6];             /**< MAC地址 */
    rtos_eth_speed_t speed;             /**< 速度 */
    rtos_eth_duplex_t duplex;           /**< 双工模式 */
    rtos_eth_phy_interface_t phy_interface; /**< PHY接口 */
    uint32_t phy_address;               /**< PHY地址 */
    bool auto_negotiation;              /**< 自动协商 */
    bool loopback_mode;                 /**< 回环模式 */
    bool promiscuous_mode;              /**< 混杂模式 */
    bool broadcast_filter;              /**< 广播过滤 */
    uint32_t rx_buffer_size;            /**< 接收缓冲区大小 */
    uint32_t tx_buffer_size;            /**< 发送缓冲区大小 */
    uint32_t timeout_ms;                /**< 超时时间 */
} rtos_eth_config_t;

/* 以太网包结构 */
typedef struct {
    uint8_t *data;                      /**< 包数据 */
    uint32_t length;                    /**< 包长度 */
    uint32_t timestamp;                 /**< 时间戳 */
    uint16_t vlan_tag;                  /**< VLAN标签 */
    uint8_t priority;                   /**< 优先级 */
    bool broadcast;                     /**< 是否广播包 */
    bool multicast;                     /**< 是否组播包 */
} rtos_eth_packet_t;

/* 以太网事件类型定义 */
typedef enum {
    RTOS_ETH_EVENT_LINK_UP = 0,         /**< 链路连接 */
    RTOS_ETH_EVENT_LINK_DOWN,           /**< 链路断开 */
    RTOS_ETH_EVENT_PACKET_RECEIVED,     /**< 包接收 */
    RTOS_ETH_EVENT_PACKET_SENT,         /**< 包发送 */
    RTOS_ETH_EVENT_ERROR,               /**< 错误事件 */
    RTOS_ETH_EVENT_MAX
} rtos_eth_event_t;

/* 以太网事件回调函数类型 */
typedef void (*rtos_eth_event_callback_t)(rtos_eth_event_t event, 
                                          const rtos_eth_packet_t *packet,
                                          void *context);

/* 以太网统计信息 */
typedef struct {
    /* 发送统计 */
    uint32_t tx_packets;                /**< 发送包数 */
    uint64_t tx_bytes;                  /**< 发送字节数 */
    uint32_t tx_errors;                 /**< 发送错误数 */
    uint32_t tx_dropped;                /**< 发送丢弃数 */
    
    /* 接收统计 */
    uint32_t rx_packets;                /**< 接收包数 */
    uint64_t rx_bytes;                  /**< 接收字节数 */
    uint32_t rx_errors;                 /**< 接收错误数 */
    uint32_t rx_dropped;                /**< 接收丢弃数 */
    uint32_t rx_crc_errors;             /**< CRC错误数 */
    uint32_t rx_frame_errors;           /**< 帧错误数 */
    
    /* 性能统计 */
    uint32_t max_tx_time_ms;            /**< 最大发送时间 */
    uint32_t max_rx_time_ms;            /**< 最大接收时间 */
    uint32_t link_up_count;             /**< 链路连接次数 */
    uint32_t link_down_count;           /**< 链路断开次数 */
} rtos_eth_stats_t;

/* 以太网管理器类结构 */
typedef struct {
    rtos_eth_config_t config;           /**< 以太网配置 */
    rtos_eth_state_t state;             /**< 以太网状态 */
    rtos_eth_link_state_t link_state;   /**< 链路状态 */
    rtos_eth_stats_t stats;             /**< 统计信息 */
    
    /* 缓冲区管理 */
    rtos_eth_packet_t *rx_packets;      /**< 接收包缓冲区 */
    rtos_eth_packet_t *tx_packets;      /**< 发送包缓冲区 */
    uint32_t rx_buffer_count;           /**< 接收缓冲区数量 */
    uint32_t tx_buffer_count;           /**< 发送缓冲区数量 */
    uint32_t rx_head, rx_tail;          /**< 接收队列头尾 */
    uint32_t tx_head, tx_tail;          /**< 发送队列头尾 */
    
    /* 事件回调 */
    rtos_eth_event_callback_t event_callbacks[RTOS_ETH_EVENT_MAX];
    void *event_contexts[RTOS_ETH_EVENT_MAX];
    
    /* DMA句柄 */
    void *tx_dma_handle;                /**< 发送DMA句柄 */
    void *rx_dma_handle;                /**< 接收DMA句柄 */
    
    /* 链路监控 */
    uint32_t link_check_interval_ms;    /**< 链路检查间隔 */
    uint32_t last_link_check_time;      /**< 最后链路检查时间 */
    
    bool initialized;
} rtos_ethernet_manager_t;

/**
 * @brief 初始化以太网管理器
 * @param config 以太网配置
 * @param rx_buffer_count 接收缓冲区数量
 * @param tx_buffer_count 发送缓冲区数量
 * @return 操作结果
 */
rtos_result_t rtos_ethernet_manager_init(const rtos_eth_config_t *config,
                                        uint32_t rx_buffer_count,
                                        uint32_t tx_buffer_count);

/**
 * @brief 启动以太网
 * @return 操作结果
 */
rtos_result_t rtos_ethernet_manager_start(void);

/**
 * @brief 停止以太网
 * @return 操作结果
 */
rtos_result_t rtos_ethernet_manager_stop(void);

/**
 * @brief 发送以太网包
 * @param packet 包数据
 * @return 操作结果
 */
rtos_result_t rtos_ethernet_manager_send_packet(const rtos_eth_packet_t *packet);

/**
 * @brief 接收以太网包
 * @param packet 包缓冲区
 * @param timeout_ms 超时时间
 * @return 操作结果
 */
rtos_result_t rtos_ethernet_manager_receive_packet(rtos_eth_packet_t *packet, uint32_t timeout_ms);

/**
 * @brief 获取链路状态
 * @return 链路状态
 */
rtos_eth_link_state_t rtos_ethernet_manager_get_link_state(void);

/**
 * @brief 获取以太网统计信息
 * @param stats 统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_ethernet_manager_get_stats(rtos_eth_stats_t *stats);

/**
 * @brief 以太网中断处理函数
 */
void rtos_ethernet_manager_interrupt_handler(void);

/**
 * @brief 以太网周期性任务
 */
void rtos_ethernet_manager_periodic_task(void);

/* 便利宏定义 */
#define RTOS_ETH_DEFAULT_CONFIG() \
    { .mac_address = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56}, \
      .speed = RTOS_ETH_SPEED_AUTO, \
      .duplex = RTOS_ETH_DUPLEX_AUTO, \
      .phy_interface = RTOS_ETH_PHY_RMII, \
      .phy_address = 1, \
      .auto_negotiation = true, \
      .loopback_mode = false, \
      .promiscuous_mode = false, \
      .broadcast_filter = false, \
      .rx_buffer_size = 1524, \
      .tx_buffer_size = 1524, \
      .timeout_ms = 1000 }

#define RTOS_ETH_BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_ETHERNET_ABSTRACTION_H__ */