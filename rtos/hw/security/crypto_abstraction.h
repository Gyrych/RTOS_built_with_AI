/**
 * @file crypto_abstraction.h
 * @brief RTOS加密抽象模块 - 面向对象的加密功能管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_CRYPTO_ABSTRACTION_H__
#define __RTOS_CRYPTO_ABSTRACTION_H__

#include "../../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 加密算法定义 */
typedef enum {
    RTOS_CRYPTO_ALG_AES128 = 0,         /**< AES-128 */
    RTOS_CRYPTO_ALG_AES192,             /**< AES-192 */
    RTOS_CRYPTO_ALG_AES256,             /**< AES-256 */
    RTOS_CRYPTO_ALG_DES,                /**< DES */
    RTOS_CRYPTO_ALG_3DES,               /**< 3DES */
    RTOS_CRYPTO_ALG_CHACHA20,           /**< ChaCha20 */
    RTOS_CRYPTO_ALG_MAX
} rtos_crypto_algorithm_t;

/* 加密模式定义 */
typedef enum {
    RTOS_CRYPTO_MODE_ECB = 0,           /**< ECB模式 */
    RTOS_CRYPTO_MODE_CBC,               /**< CBC模式 */
    RTOS_CRYPTO_MODE_CFB,               /**< CFB模式 */
    RTOS_CRYPTO_MODE_OFB,               /**< OFB模式 */
    RTOS_CRYPTO_MODE_CTR,               /**< CTR模式 */
    RTOS_CRYPTO_MODE_GCM,               /**< GCM模式 */
    RTOS_CRYPTO_MODE_CCM,               /**< CCM模式 */
    RTOS_CRYPTO_MODE_MAX
} rtos_crypto_mode_t;

/* 哈希算法定义 */
typedef enum {
    RTOS_HASH_ALG_MD5 = 0,              /**< MD5 */
    RTOS_HASH_ALG_SHA1,                 /**< SHA-1 */
    RTOS_HASH_ALG_SHA224,               /**< SHA-224 */
    RTOS_HASH_ALG_SHA256,               /**< SHA-256 */
    RTOS_HASH_ALG_SHA384,               /**< SHA-384 */
    RTOS_HASH_ALG_SHA512,               /**< SHA-512 */
    RTOS_HASH_ALG_MAX
} rtos_hash_alg_t;

/* 随机数生成器类型 */
typedef enum {
    RTOS_RNG_TYPE_HARDWARE = 0,         /**< 硬件随机数 */
    RTOS_RNG_TYPE_PSEUDO,               /**< 伪随机数 */
    RTOS_RNG_TYPE_TRUE,                 /**< 真随机数 */
    RTOS_RNG_TYPE_MAX
} rtos_rng_type_t;

/* 加密上下文结构 */
typedef struct {
    rtos_crypto_algorithm_t algorithm;  /**< 加密算法 */
    rtos_crypto_mode_t mode;            /**< 加密模式 */
    uint8_t *key;                       /**< 密钥 */
    uint32_t key_size;                  /**< 密钥大小 */
    uint8_t *iv;                        /**< 初始化向量 */
    uint32_t iv_size;                   /**< IV大小 */
    uint32_t block_size;                /**< 块大小 */
    bool initialized;                   /**< 是否已初始化 */
    void *platform_context;             /**< 平台相关上下文 */
} rtos_crypto_context_t;

/* 哈希上下文结构 */
typedef struct {
    rtos_hash_alg_t algorithm;          /**< 哈希算法 */
    uint32_t hash_size;                 /**< 哈希大小 */
    uint64_t total_length;              /**< 总长度 */
    bool initialized;                   /**< 是否已初始化 */
    void *platform_context;             /**< 平台相关上下文 */
} rtos_hash_context_t;

/* 随机数生成器配置 */
typedef struct {
    rtos_rng_type_t type;               /**< 随机数类型 */
    uint32_t seed;                      /**< 种子值 */
    bool auto_reseed;                   /**< 自动重新播种 */
    uint32_t reseed_interval;           /**< 重新播种间隔 */
} rtos_rng_config_t;

/* 加密统计信息 */
typedef struct {
    uint32_t encrypt_operations;        /**< 加密操作次数 */
    uint32_t decrypt_operations;        /**< 解密操作次数 */
    uint32_t hash_operations;           /**< 哈希操作次数 */
    uint32_t rng_operations;            /**< 随机数生成次数 */
    uint64_t total_bytes_processed;     /**< 总处理字节数 */
    uint32_t error_count;               /**< 错误次数 */
    uint32_t max_operation_time_ms;     /**< 最大操作时间 */
    uint32_t avg_operation_time_ms;     /**< 平均操作时间 */
} rtos_crypto_stats_t;

/* 加密管理器类结构 */
typedef struct {
    /* 配置和状态 */
    rtos_rng_config_t rng_config;       /**< 随机数配置 */
    rtos_crypto_stats_t stats;          /**< 统计信息 */
    
    /* 上下文池 */
    rtos_crypto_context_t *crypto_contexts;
    rtos_hash_context_t *hash_contexts;
    uint32_t max_crypto_contexts;
    uint32_t max_hash_contexts;
    uint32_t active_crypto_contexts;
    uint32_t active_hash_contexts;
    
    /* 密钥存储 */
    struct {
        uint8_t *key_storage;
        uint32_t storage_size;
        uint32_t used_size;
        bool encrypted;
    } key_store;
    
    /* 状态标志 */
    bool initialized;
    bool hardware_crypto_available;
    
} rtos_crypto_manager_t;

/**
 * @brief 初始化加密管理器
 * @param max_crypto_contexts 最大加密上下文数
 * @param max_hash_contexts 最大哈希上下文数
 * @param key_storage_size 密钥存储大小
 * @return 操作结果
 */
rtos_result_t rtos_crypto_manager_init(uint32_t max_crypto_contexts,
                                      uint32_t max_hash_contexts,
                                      uint32_t key_storage_size);

/**
 * @brief 创建加密上下文
 * @param algorithm 加密算法
 * @param mode 加密模式
 * @param key 密钥
 * @param key_size 密钥大小
 * @param context 返回的上下文指针
 * @return 操作结果
 */
rtos_result_t rtos_crypto_create_context(rtos_crypto_algorithm_t algorithm,
                                        rtos_crypto_mode_t mode,
                                        const uint8_t *key,
                                        uint32_t key_size,
                                        rtos_crypto_context_t **context);

/**
 * @brief 销毁加密上下文
 * @param context 加密上下文
 * @return 操作结果
 */
rtos_result_t rtos_crypto_destroy_context(rtos_crypto_context_t *context);

/**
 * @brief AES加密
 * @param context 加密上下文
 * @param input 输入数据
 * @param output 输出缓冲区
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_crypto_aes_encrypt(rtos_crypto_context_t *context,
                                     const uint8_t *input,
                                     uint8_t *output,
                                     uint32_t length);

/**
 * @brief AES解密
 * @param context 加密上下文
 * @param input 输入数据
 * @param output 输出缓冲区
 * @param length 数据长度
 * @return 操作结果
 */
rtos_result_t rtos_crypto_aes_decrypt(rtos_crypto_context_t *context,
                                     const uint8_t *input,
                                     uint8_t *output,
                                     uint32_t length);

/**
 * @brief 计算哈希值
 * @param algorithm 哈希算法
 * @param input 输入数据
 * @param input_length 输入长度
 * @param hash_output 哈希输出
 * @param hash_size 哈希大小
 * @return 操作结果
 */
rtos_result_t rtos_crypto_calculate_hash(rtos_hash_alg_t algorithm,
                                        const uint8_t *input,
                                        uint32_t input_length,
                                        uint8_t *hash_output,
                                        uint32_t hash_size);

/**
 * @brief 生成随机数
 * @param buffer 输出缓冲区
 * @param length 随机数长度
 * @return 操作结果
 */
rtos_result_t rtos_crypto_generate_random(uint8_t *buffer, uint32_t length);

/**
 * @brief 生成密钥
 * @param algorithm 加密算法
 * @param key_buffer 密钥缓冲区
 * @param key_size 密钥大小
 * @return 操作结果
 */
rtos_result_t rtos_crypto_generate_key(rtos_crypto_algorithm_t algorithm,
                                      uint8_t *key_buffer,
                                      uint32_t key_size);

/**
 * @brief 密钥派生
 * @param password 密码
 * @param password_length 密码长度
 * @param salt 盐值
 * @param salt_length 盐值长度
 * @param iterations 迭代次数
 * @param key_output 派生密钥输出
 * @param key_length 密钥长度
 * @return 操作结果
 */
rtos_result_t rtos_crypto_derive_key(const uint8_t *password,
                                    uint32_t password_length,
                                    const uint8_t *salt,
                                    uint32_t salt_length,
                                    uint32_t iterations,
                                    uint8_t *key_output,
                                    uint32_t key_length);

/* 便利宏定义 */
#define RTOS_CRYPTO_AES128_KEY_SIZE     16
#define RTOS_CRYPTO_AES192_KEY_SIZE     24
#define RTOS_CRYPTO_AES256_KEY_SIZE     32
#define RTOS_CRYPTO_AES_BLOCK_SIZE      16
#define RTOS_CRYPTO_AES_IV_SIZE         16

#define RTOS_HASH_MD5_SIZE              16
#define RTOS_HASH_SHA1_SIZE             20
#define RTOS_HASH_SHA224_SIZE           28
#define RTOS_HASH_SHA256_SIZE           32
#define RTOS_HASH_SHA384_SIZE           48
#define RTOS_HASH_SHA512_SIZE           64

/* 调试宏定义 */
#ifdef RTOS_CRYPTO_DEBUG
#define RTOS_CRYPTO_DEBUG_PRINT(fmt, ...) \
    printf("[CRYPTO] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_CRYPTO_DEBUG_PRINT(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_CRYPTO_ABSTRACTION_H__ */