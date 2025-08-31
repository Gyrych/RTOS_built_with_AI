/**
 * @file ota_manager.h
 * @brief RTOS OTA更新管理模块 - 面向对象的固件更新管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_OTA_MANAGER_H__
#define __RTOS_OTA_MANAGER_H__

#include "../../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OTA更新状态定义 */
typedef enum {
    RTOS_OTA_STATE_IDLE = 0,            /**< 空闲状态 */
    RTOS_OTA_STATE_CHECKING,            /**< 检查更新中 */
    RTOS_OTA_STATE_DOWNLOADING,         /**< 下载中 */
    RTOS_OTA_STATE_VERIFYING,           /**< 验证中 */
    RTOS_OTA_STATE_INSTALLING,          /**< 安装中 */
    RTOS_OTA_STATE_COMPLETE,            /**< 完成 */
    RTOS_OTA_STATE_ERROR,               /**< 错误状态 */
    RTOS_OTA_STATE_MAX
} rtos_ota_state_t;

/* OTA更新类型定义 */
typedef enum {
    RTOS_OTA_TYPE_FULL = 0,             /**< 完整更新 */
    RTOS_OTA_TYPE_INCREMENTAL,          /**< 增量更新 */
    RTOS_OTA_TYPE_PATCH,                /**< 补丁更新 */
    RTOS_OTA_TYPE_ROLLBACK,             /**< 回滚更新 */
    RTOS_OTA_TYPE_MAX
} rtos_ota_type_t;

/* OTA传输方式定义 */
typedef enum {
    RTOS_OTA_TRANSPORT_HTTP = 0,        /**< HTTP传输 */
    RTOS_OTA_TRANSPORT_HTTPS,           /**< HTTPS传输 */
    RTOS_OTA_TRANSPORT_FTP,             /**< FTP传输 */
    RTOS_OTA_TRANSPORT_MQTT,            /**< MQTT传输 */
    RTOS_OTA_TRANSPORT_COAP,            /**< CoAP传输 */
    RTOS_OTA_TRANSPORT_SERIAL,          /**< 串口传输 */
    RTOS_OTA_TRANSPORT_MAX
} rtos_ota_transport_t;

/* OTA错误类型定义 */
typedef enum {
    RTOS_OTA_ERROR_NONE = 0,
    RTOS_OTA_ERROR_NETWORK = (1 << 0),      /**< 网络错误 */
    RTOS_OTA_ERROR_DOWNLOAD = (1 << 1),     /**< 下载错误 */
    RTOS_OTA_ERROR_VERIFICATION = (1 << 2), /**< 验证错误 */
    RTOS_OTA_ERROR_INSTALLATION = (1 << 3), /**< 安装错误 */
    RTOS_OTA_ERROR_STORAGE = (1 << 4),      /**< 存储错误 */
    RTOS_OTA_ERROR_SIGNATURE = (1 << 5),    /**< 签名错误 */
    RTOS_OTA_ERROR_VERSION = (1 << 6),      /**< 版本错误 */
    RTOS_OTA_ERROR_ROLLBACK = (1 << 7)      /**< 回滚错误 */
} rtos_ota_error_t;

/* OTA固件信息 */
typedef struct {
    uint32_t version;                   /**< 版本号 */
    uint32_t size;                      /**< 固件大小 */
    uint32_t crc32;                     /**< CRC32校验 */
    uint8_t sha256[32];                 /**< SHA256哈希 */
    uint8_t signature[256];             /**< 数字签名 */
    char version_string[32];            /**< 版本字符串 */
    char description[128];              /**< 更新描述 */
    char download_url[256];             /**< 下载URL */
    uint32_t release_time;              /**< 发布时间 */
    bool mandatory;                     /**< 是否强制更新 */
    bool rollback_allowed;              /**< 是否允许回滚 */
} rtos_ota_firmware_info_t;

/* OTA配置结构 */
typedef struct {
    rtos_ota_transport_t transport;     /**< 传输方式 */
    char server_url[256];               /**< 服务器URL */
    char device_id[64];                 /**< 设备ID */
    char api_key[128];                  /**< API密钥 */
    uint32_t check_interval_ms;         /**< 检查间隔 */
    uint32_t download_timeout_ms;       /**< 下载超时 */
    uint32_t max_retry_count;           /**< 最大重试次数 */
    bool auto_check_enable;             /**< 自动检查使能 */
    bool auto_install_enable;           /**< 自动安装使能 */
    bool backup_enable;                 /**< 备份使能 */
    uint32_t storage_partition_size;    /**< 存储分区大小 */
} rtos_ota_config_t;

/* OTA进度信息 */
typedef struct {
    rtos_ota_state_t state;             /**< 当前状态 */
    rtos_ota_type_t type;               /**< 更新类型 */
    uint32_t total_size;                /**< 总大小 */
    uint32_t downloaded_size;           /**< 已下载大小 */
    uint32_t progress_percent;          /**< 进度百分比 */
    uint32_t download_speed_kbps;       /**< 下载速度 */
    uint32_t estimated_time_remaining;  /**< 预计剩余时间 */
    uint32_t error_code;                /**< 错误代码 */
    char status_message[128];           /**< 状态消息 */
} rtos_ota_progress_t;

/* OTA事件类型定义 */
typedef enum {
    RTOS_OTA_EVENT_CHECK_START = 0,     /**< 检查开始 */
    RTOS_OTA_EVENT_UPDATE_AVAILABLE,    /**< 有可用更新 */
    RTOS_OTA_EVENT_DOWNLOAD_START,      /**< 下载开始 */
    RTOS_OTA_EVENT_DOWNLOAD_PROGRESS,   /**< 下载进度 */
    RTOS_OTA_EVENT_DOWNLOAD_COMPLETE,   /**< 下载完成 */
    RTOS_OTA_EVENT_VERIFY_START,        /**< 验证开始 */
    RTOS_OTA_EVENT_VERIFY_COMPLETE,     /**< 验证完成 */
    RTOS_OTA_EVENT_INSTALL_START,       /**< 安装开始 */
    RTOS_OTA_EVENT_INSTALL_COMPLETE,    /**< 安装完成 */
    RTOS_OTA_EVENT_ERROR,               /**< 错误事件 */
    RTOS_OTA_EVENT_MAX
} rtos_ota_event_t;

/* OTA事件回调函数类型 */
typedef void (*rtos_ota_event_callback_t)(rtos_ota_event_t event, 
                                          const rtos_ota_progress_t *progress,
                                          void *context);

/* OTA统计信息 */
typedef struct {
    uint32_t check_count;               /**< 检查次数 */
    uint32_t download_count;            /**< 下载次数 */
    uint32_t successful_updates;        /**< 成功更新次数 */
    uint32_t failed_updates;            /**< 失败更新次数 */
    uint32_t rollback_count;            /**< 回滚次数 */
    uint64_t total_downloaded_bytes;    /**< 总下载字节数 */
    uint32_t max_download_time_ms;      /**< 最大下载时间 */
    uint32_t avg_download_speed_kbps;   /**< 平均下载速度 */
    uint32_t last_update_time;          /**< 最后更新时间 */
} rtos_ota_stats_t;

/* OTA管理器类结构 */
typedef struct {
    rtos_ota_config_t config;           /**< OTA配置 */
    rtos_ota_progress_t progress;       /**< 进度信息 */
    rtos_ota_stats_t stats;             /**< 统计信息 */
    
    /* 固件信息 */
    rtos_ota_firmware_info_t current_firmware;  /**< 当前固件信息 */
    rtos_ota_firmware_info_t available_firmware; /**< 可用固件信息 */
    rtos_ota_firmware_info_t backup_firmware;   /**< 备份固件信息 */
    
    /* 存储管理 */
    struct {
        uint32_t download_partition_addr;   /**< 下载分区地址 */
        uint32_t backup_partition_addr;     /**< 备份分区地址 */
        uint32_t partition_size;            /**< 分区大小 */
        uint32_t current_offset;            /**< 当前偏移 */
    } storage;
    
    /* 网络连接 */
    void *network_handle;               /**< 网络句柄 */
    
    /* 事件回调 */
    rtos_ota_event_callback_t event_callbacks[RTOS_OTA_EVENT_MAX];
    void *event_contexts[RTOS_OTA_EVENT_MAX];
    
    /* 状态标志 */
    bool initialized;
    bool update_in_progress;
    bool rollback_available;
    
} rtos_ota_manager_t;

/**
 * @brief 初始化OTA管理器
 * @param config OTA配置
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_init(const rtos_ota_config_t *config);

/**
 * @brief 检查固件更新
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_check_update(void);

/**
 * @brief 下载固件更新
 * @param firmware_info 固件信息
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_download_firmware(const rtos_ota_firmware_info_t *firmware_info);

/**
 * @brief 验证下载的固件
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_verify_firmware(void);

/**
 * @brief 安装固件更新
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_install_update(void);

/**
 * @brief 回滚到备份固件
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_rollback(void);

/**
 * @brief 获取OTA进度
 * @param progress 进度信息指针
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_get_progress(rtos_ota_progress_t *progress);

/**
 * @brief 获取当前固件信息
 * @param firmware_info 固件信息指针
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_get_current_firmware_info(rtos_ota_firmware_info_t *firmware_info);

/**
 * @brief OTA管理器周期性任务
 */
void rtos_ota_manager_periodic_task(void);

/**
 * @brief 注册OTA事件回调
 * @param event 事件类型
 * @param callback 回调函数
 * @param context 用户上下文
 * @return 操作结果
 */
rtos_result_t rtos_ota_manager_register_event_callback(rtos_ota_event_t event,
                                                      rtos_ota_event_callback_t callback,
                                                      void *context);

/* 便利宏定义 */
#define RTOS_OTA_DEFAULT_CONFIG() \
    { .transport = RTOS_OTA_TRANSPORT_HTTPS, \
      .server_url = "", \
      .device_id = "", \
      .api_key = "", \
      .check_interval_ms = 3600000, \
      .download_timeout_ms = 300000, \
      .max_retry_count = 3, \
      .auto_check_enable = false, \
      .auto_install_enable = false, \
      .backup_enable = true, \
      .storage_partition_size = 512 * 1024 }

/* 调试宏定义 */
#ifdef RTOS_OTA_DEBUG
#define RTOS_OTA_DEBUG_PRINT(fmt, ...) \
    printf("[OTA] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_OTA_DEBUG_PRINT(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_OTA_MANAGER_H__ */