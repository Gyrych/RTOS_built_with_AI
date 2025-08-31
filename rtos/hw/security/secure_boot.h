/**
 * @file secure_boot.h
 * @brief RTOS安全启动模块 - 面向对象的安全启动管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_SECURE_BOOT_H__
#define __RTOS_SECURE_BOOT_H__

#include "../../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 安全启动状态定义 */
typedef enum {
    RTOS_SECURE_BOOT_STATE_DISABLED = 0,   /**< 禁用状态 */
    RTOS_SECURE_BOOT_STATE_ENABLED,        /**< 启用状态 */
    RTOS_SECURE_BOOT_STATE_VERIFYING,      /**< 验证中 */
    RTOS_SECURE_BOOT_STATE_VERIFIED,       /**< 验证通过 */
    RTOS_SECURE_BOOT_STATE_FAILED,         /**< 验证失败 */
    RTOS_SECURE_BOOT_STATE_MAX
} rtos_secure_boot_state_t;

/* 签名算法定义 */
typedef enum {
    RTOS_SIGNATURE_ALG_RSA2048 = 0,     /**< RSA-2048 */
    RTOS_SIGNATURE_ALG_RSA4096,         /**< RSA-4096 */
    RTOS_SIGNATURE_ALG_ECDSA_P256,      /**< ECDSA P-256 */
    RTOS_SIGNATURE_ALG_ECDSA_P384,      /**< ECDSA P-384 */
    RTOS_SIGNATURE_ALG_ED25519,         /**< Ed25519 */
    RTOS_SIGNATURE_ALG_MAX
} rtos_signature_algorithm_t;

/* 哈希算法定义 */
typedef enum {
    RTOS_HASH_ALG_SHA256 = 0,           /**< SHA-256 */
    RTOS_HASH_ALG_SHA384,               /**< SHA-384 */
    RTOS_HASH_ALG_SHA512,               /**< SHA-512 */
    RTOS_HASH_ALG_SHA3_256,             /**< SHA3-256 */
    RTOS_HASH_ALG_MAX
} rtos_hash_algorithm_t;

/* 安全启动配置 */
typedef struct {
    bool enabled;                       /**< 是否启用安全启动 */
    rtos_signature_algorithm_t sig_alg; /**< 签名算法 */
    rtos_hash_algorithm_t hash_alg;     /**< 哈希算法 */
    uint32_t public_key_size;           /**< 公钥大小 */
    uint32_t signature_size;            /**< 签名大小 */
    uint32_t hash_size;                 /**< 哈希大小 */
    bool rollback_protection;          /**< 回滚保护 */
    uint32_t min_version;               /**< 最小版本号 */
    bool debug_disable_on_fail;        /**< 验证失败时禁用调试 */
    uint32_t max_boot_attempts;        /**< 最大启动尝试次数 */
} rtos_secure_boot_config_t;

/* 固件信息结构 */
typedef struct {
    uint32_t magic;                     /**< 魔数 */
    uint32_t version;                   /**< 版本号 */
    uint32_t firmware_size;             /**< 固件大小 */
    uint32_t firmware_crc;              /**< 固件CRC */
    uint32_t load_address;              /**< 加载地址 */
    uint32_t entry_point;               /**< 入口点 */
    uint32_t timestamp;                 /**< 时间戳 */
    uint8_t hash[64];                   /**< 固件哈希 */
    uint8_t signature[512];             /**< 数字签名 */
    uint8_t public_key[512];            /**< 公钥 */
    uint32_t reserved[16];              /**< 保留字段 */
} rtos_firmware_header_t;

/* 安全启动信息 */
typedef struct {
    rtos_firmware_header_t header;      /**< 固件头 */
    rtos_secure_boot_state_t state;     /**< 启动状态 */
    uint32_t verification_time_ms;      /**< 验证耗时 */
    uint32_t boot_count;                /**< 启动次数 */
    uint32_t failed_count;              /**< 失败次数 */
    uint32_t last_boot_time;            /**< 最后启动时间 */
    bool debug_enabled;                 /**< 调试是否启用 */
} rtos_secure_boot_info_t;

/* 安全启动事件类型 */
typedef enum {
    RTOS_SECURE_BOOT_EVENT_VERIFY_START = 0, /**< 验证开始 */
    RTOS_SECURE_BOOT_EVENT_VERIFY_SUCCESS,   /**< 验证成功 */
    RTOS_SECURE_BOOT_EVENT_VERIFY_FAILED,    /**< 验证失败 */
    RTOS_SECURE_BOOT_EVENT_ROLLBACK_DETECTED, /**< 检测到回滚 */
    RTOS_SECURE_BOOT_EVENT_DEBUG_DISABLED,   /**< 调试被禁用 */
    RTOS_SECURE_BOOT_EVENT_MAX
} rtos_secure_boot_event_t;

/* 安全启动事件回调函数类型 */
typedef void (*rtos_secure_boot_event_callback_t)(rtos_secure_boot_event_t event, 
                                                  const rtos_secure_boot_info_t *info,
                                                  void *context);

/* 安全启动管理器类结构 */
typedef struct {
    rtos_secure_boot_config_t config;   /**< 配置 */
    rtos_secure_boot_info_t boot_info;  /**< 启动信息 */
    
    /* 密钥管理 */
    uint8_t *root_public_key;           /**< 根公钥 */
    uint32_t root_key_size;             /**< 根公钥大小 */
    uint8_t *trusted_keys;              /**< 受信任密钥列表 */
    uint32_t trusted_key_count;         /**< 受信任密钥数量 */
    
    /* 事件回调 */
    rtos_secure_boot_event_callback_t event_callbacks[RTOS_SECURE_BOOT_EVENT_MAX];
    void *event_contexts[RTOS_SECURE_BOOT_EVENT_MAX];
    
    /* 统计信息 */
    uint32_t total_verifications;       /**< 总验证次数 */
    uint32_t successful_verifications;  /**< 成功验证次数 */
    uint32_t failed_verifications;      /**< 失败验证次数 */
    uint32_t rollback_attempts;         /**< 回滚尝试次数 */
    
    /* 平台相关数据 */
    void *platform_data;
    
    bool initialized;
} rtos_secure_boot_manager_t;

/**
 * @brief 初始化安全启动管理器
 * @param config 安全启动配置
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_manager_init(const rtos_secure_boot_config_t *config);

/**
 * @brief 反初始化安全启动管理器
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_manager_deinit(void);

/**
 * @brief 获取安全启动管理器实例
 * @return 安全启动管理器指针
 */
rtos_secure_boot_manager_t* rtos_secure_boot_manager_get_instance(void);

/**
 * @brief 验证固件签名
 * @param firmware_addr 固件地址
 * @param firmware_size 固件大小
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_verify_firmware(uint32_t firmware_addr, uint32_t firmware_size);

/**
 * @brief 设置根公钥
 * @param public_key 公钥数据
 * @param key_size 公钥大小
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_set_root_key(const uint8_t *public_key, uint32_t key_size);

/**
 * @brief 添加受信任公钥
 * @param public_key 公钥数据
 * @param key_size 公钥大小
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_add_trusted_key(const uint8_t *public_key, uint32_t key_size);

/**
 * @brief 启用/禁用安全启动
 * @param enable 是否启用
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_enable(bool enable);

/**
 * @brief 获取安全启动状态
 * @return 安全启动状态
 */
rtos_secure_boot_state_t rtos_secure_boot_get_state(void);

/**
 * @brief 获取安全启动信息
 * @param info 启动信息指针
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_get_info(rtos_secure_boot_info_t *info);

/**
 * @brief 注册安全启动事件回调
 * @param event 事件类型
 * @param callback 回调函数
 * @param context 用户上下文
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_register_event_callback(rtos_secure_boot_event_t event,
                                                      rtos_secure_boot_event_callback_t callback,
                                                      void *context);

/**
 * @brief 生成固件哈希
 * @param firmware_addr 固件地址
 * @param firmware_size 固件大小
 * @param hash_output 哈希输出缓冲区
 * @param hash_size 哈希大小
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_calculate_hash(uint32_t firmware_addr,
                                             uint32_t firmware_size,
                                             uint8_t *hash_output,
                                             uint32_t hash_size);

/**
 * @brief 验证数字签名
 * @param hash 哈希值
 * @param hash_size 哈希大小
 * @param signature 签名
 * @param signature_size 签名大小
 * @param public_key 公钥
 * @param key_size 公钥大小
 * @return 操作结果
 */
rtos_result_t rtos_secure_boot_verify_signature(const uint8_t *hash,
                                               uint32_t hash_size,
                                               const uint8_t *signature,
                                               uint32_t signature_size,
                                               const uint8_t *public_key,
                                               uint32_t key_size);

/**
 * @brief 获取安全启动统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_secure_boot_get_statistics(char *buffer, uint32_t size);

/* 便利宏定义 */
#define RTOS_SECURE_BOOT_MAGIC          0x53454342  /* "SECB" */
#define RTOS_SECURE_BOOT_VERSION        0x00010000  /* v1.0.0 */

#define RTOS_SECURE_BOOT_DEFAULT_CONFIG() \
    { .enabled = false, \
      .sig_alg = RTOS_SIGNATURE_ALG_RSA2048, \
      .hash_alg = RTOS_HASH_ALG_SHA256, \
      .public_key_size = 256, \
      .signature_size = 256, \
      .hash_size = 32, \
      .rollback_protection = true, \
      .min_version = 0, \
      .debug_disable_on_fail = false, \
      .max_boot_attempts = 3 }

/* 调试宏定义 */
#ifdef RTOS_SECURE_BOOT_DEBUG
#define RTOS_SECURE_BOOT_DEBUG_PRINT(fmt, ...) \
    printf("[SECURE_BOOT] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_SECURE_BOOT_DEBUG_PRINT(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_SECURE_BOOT_H__ */