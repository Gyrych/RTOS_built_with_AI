/**
 * @file crypto_abstraction.c
 * @brief RTOS加密抽象模块实现 - 简化版本
 * @author Assistant
 * @date 2024
 */

#include "crypto_abstraction.h"
#include "../hw_config.h"
#include "../hw_abstraction.h"
#include "../../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 全局加密管理器实例 */
static rtos_crypto_manager_t g_crypto_manager;
static bool g_crypto_manager_initialized = false;

/* 简单的伪随机数生成器状态 */
static uint32_t g_rng_state = 12345;

/**
 * @brief 初始化加密管理器
 */
rtos_result_t rtos_crypto_manager_init(uint32_t max_crypto_contexts,
                                      uint32_t max_hash_contexts,
                                      uint32_t key_storage_size)
{
    if (g_crypto_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_crypto_manager, 0, sizeof(g_crypto_manager));
    
    /* 分配加密上下文数组 */
    if (max_crypto_contexts > 0) {
        g_crypto_manager.crypto_contexts = malloc(sizeof(rtos_crypto_context_t) * max_crypto_contexts);
        if (!g_crypto_manager.crypto_contexts) {
            RTOS_CRYPTO_DEBUG_PRINT("Failed to allocate crypto contexts");
            return RTOS_ERROR_NO_MEMORY;
        }
        g_crypto_manager.max_crypto_contexts = max_crypto_contexts;
        memset(g_crypto_manager.crypto_contexts, 0, sizeof(rtos_crypto_context_t) * max_crypto_contexts);
    }
    
    /* 分配哈希上下文数组 */
    if (max_hash_contexts > 0) {
        g_crypto_manager.hash_contexts = malloc(sizeof(rtos_hash_context_t) * max_hash_contexts);
        if (!g_crypto_manager.hash_contexts) {
            if (g_crypto_manager.crypto_contexts) {
                free(g_crypto_manager.crypto_contexts);
            }
            RTOS_CRYPTO_DEBUG_PRINT("Failed to allocate hash contexts");
            return RTOS_ERROR_NO_MEMORY;
        }
        g_crypto_manager.max_hash_contexts = max_hash_contexts;
        memset(g_crypto_manager.hash_contexts, 0, sizeof(rtos_hash_context_t) * max_hash_contexts);
    }
    
    /* 分配密钥存储 */
    if (key_storage_size > 0) {
        g_crypto_manager.key_store.key_storage = malloc(key_storage_size);
        if (!g_crypto_manager.key_store.key_storage) {
            if (g_crypto_manager.crypto_contexts) {
                free(g_crypto_manager.crypto_contexts);
            }
            if (g_crypto_manager.hash_contexts) {
                free(g_crypto_manager.hash_contexts);
            }
            RTOS_CRYPTO_DEBUG_PRINT("Failed to allocate key storage");
            return RTOS_ERROR_NO_MEMORY;
        }
        g_crypto_manager.key_store.storage_size = key_storage_size;
        memset(g_crypto_manager.key_store.key_storage, 0, key_storage_size);
    }
    
    /* 检查硬件加密支持 */
    g_crypto_manager.hardware_crypto_available = false; /* STM32F4不支持硬件加密 */
    
    /* 初始化随机数生成器 */
    g_crypto_manager.rng_config.type = RTOS_RNG_TYPE_PSEUDO;
    g_crypto_manager.rng_config.seed = rtos_hw_get_system_time_ms();
    g_rng_state = g_crypto_manager.rng_config.seed;
    
    g_crypto_manager.initialized = true;
    g_crypto_manager_initialized = true;
    
    RTOS_CRYPTO_DEBUG_PRINT("Crypto manager initialized (crypto:%lu, hash:%lu, storage:%lu)", 
                           max_crypto_contexts, max_hash_contexts, key_storage_size);
    
    return RTOS_OK;
}

/**
 * @brief 反初始化加密管理器
 */
rtos_result_t rtos_crypto_manager_deinit(void)
{
    if (!g_crypto_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 释放加密上下文 */
    if (g_crypto_manager.crypto_contexts) {
        free(g_crypto_manager.crypto_contexts);
        g_crypto_manager.crypto_contexts = NULL;
    }
    
    /* 释放哈希上下文 */
    if (g_crypto_manager.hash_contexts) {
        free(g_crypto_manager.hash_contexts);
        g_crypto_manager.hash_contexts = NULL;
    }
    
    /* 释放密钥存储 */
    if (g_crypto_manager.key_store.key_storage) {
        /* 清零密钥数据 */
        memset(g_crypto_manager.key_store.key_storage, 0, g_crypto_manager.key_store.storage_size);
        free(g_crypto_manager.key_store.key_storage);
        g_crypto_manager.key_store.key_storage = NULL;
    }
    
    g_crypto_manager_initialized = false;
    
    RTOS_CRYPTO_DEBUG_PRINT("Crypto manager deinitialized");
    return RTOS_OK;
}

/**
 * @brief 生成随机数
 */
rtos_result_t rtos_crypto_generate_random(uint8_t *buffer, uint32_t length)
{
    if (!buffer || length == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!g_crypto_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 简单的伪随机数生成器 (线性同余生成器) */
    for (uint32_t i = 0; i < length; i++) {
        g_rng_state = g_rng_state * 1103515245 + 12345;
        buffer[i] = (uint8_t)(g_rng_state >> 16);
    }
    
    g_crypto_manager.stats.rng_operations++;
    
    RTOS_CRYPTO_DEBUG_PRINT("Generated %lu random bytes", length);
    return RTOS_OK;
}

/**
 * @brief 创建加密上下文
 */
rtos_result_t rtos_crypto_create_context(rtos_crypto_algorithm_t algorithm,
                                        rtos_crypto_mode_t mode,
                                        const uint8_t *key,
                                        uint32_t key_size,
                                        rtos_crypto_context_t **context)
{
    if (!key || key_size == 0 || !context) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!g_crypto_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 查找空闲上下文 */
    for (uint32_t i = 0; i < g_crypto_manager.max_crypto_contexts; i++) {
        if (!g_crypto_manager.crypto_contexts[i].initialized) {
            rtos_crypto_context_t *ctx = &g_crypto_manager.crypto_contexts[i];
            
            /* 初始化上下文 */
            ctx->algorithm = algorithm;
            ctx->mode = mode;
            ctx->key_size = key_size;
            ctx->block_size = (algorithm <= RTOS_CRYPTO_ALG_AES256) ? 16 : 8;
            
            /* 分配并复制密钥 */
            ctx->key = malloc(key_size);
            if (!ctx->key) {
                return RTOS_ERROR_NO_MEMORY;
            }
            memcpy(ctx->key, key, key_size);
            
            ctx->initialized = true;
            g_crypto_manager.active_crypto_contexts++;
            
            *context = ctx;
            
            RTOS_CRYPTO_DEBUG_PRINT("Crypto context created: alg=%d, mode=%d, key_size=%lu", 
                                   algorithm, mode, key_size);
            
            return RTOS_OK;
        }
    }
    
    return RTOS_ERROR_NO_MEMORY;
}

/**
 * @brief 销毁加密上下文
 */
rtos_result_t rtos_crypto_destroy_context(rtos_crypto_context_t *context)
{
    if (!context || !context->initialized) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 清零并释放密钥 */
    if (context->key) {
        memset(context->key, 0, context->key_size);
        free(context->key);
        context->key = NULL;
    }
    
    /* 清零并释放IV */
    if (context->iv) {
        memset(context->iv, 0, context->iv_size);
        free(context->iv);
        context->iv = NULL;
    }
    
    /* 重置上下文 */
    memset(context, 0, sizeof(rtos_crypto_context_t));
    g_crypto_manager.active_crypto_contexts--;
    
    RTOS_CRYPTO_DEBUG_PRINT("Crypto context destroyed");
    return RTOS_OK;
}

/**
 * @brief AES加密 (简化实现)
 */
rtos_result_t rtos_crypto_aes_encrypt(rtos_crypto_context_t *context,
                                     const uint8_t *input,
                                     uint8_t *output,
                                     uint32_t length)
{
    if (!context || !input || !output || length == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!context->initialized) {
        return RTOS_ERROR_INVALID_STATE;
    }
    
    /* 简化实现：简单的XOR加密 */
    for (uint32_t i = 0; i < length; i++) {
        output[i] = input[i] ^ context->key[i % context->key_size];
    }
    
    g_crypto_manager.stats.encrypt_operations++;
    g_crypto_manager.stats.total_bytes_processed += length;
    
    RTOS_CRYPTO_DEBUG_PRINT("AES encrypt: %lu bytes", length);
    return RTOS_OK;
}

/**
 * @brief AES解密 (简化实现)
 */
rtos_result_t rtos_crypto_aes_decrypt(rtos_crypto_context_t *context,
                                     const uint8_t *input,
                                     uint8_t *output,
                                     uint32_t length)
{
    if (!context || !input || !output || length == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!context->initialized) {
        return RTOS_ERROR_INVALID_STATE;
    }
    
    /* 简化实现：简单的XOR解密 (与加密相同) */
    for (uint32_t i = 0; i < length; i++) {
        output[i] = input[i] ^ context->key[i % context->key_size];
    }
    
    g_crypto_manager.stats.decrypt_operations++;
    g_crypto_manager.stats.total_bytes_processed += length;
    
    RTOS_CRYPTO_DEBUG_PRINT("AES decrypt: %lu bytes", length);
    return RTOS_OK;
}

/**
 * @brief 计算哈希值 (简化实现)
 */
rtos_result_t rtos_crypto_calculate_hash(rtos_hash_alg_t algorithm,
                                        const uint8_t *input,
                                        uint32_t input_length,
                                        uint8_t *hash_output,
                                        uint32_t hash_size)
{
    if (!input || input_length == 0 || !hash_output || hash_size == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!g_crypto_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 简化实现：简单的校验和哈希 */
    uint32_t hash_value = 0;
    for (uint32_t i = 0; i < input_length; i++) {
        hash_value = hash_value * 31 + input[i];
    }
    
    /* 填充哈希输出 */
    memset(hash_output, 0, hash_size);
    for (uint32_t i = 0; i < hash_size && i < 4; i++) {
        hash_output[i] = (uint8_t)(hash_value >> (i * 8));
    }
    
    g_crypto_manager.stats.hash_operations++;
    
    RTOS_CRYPTO_DEBUG_PRINT("Hash calculated: alg=%d, input=%lu bytes, output=%lu bytes", 
                           algorithm, input_length, hash_size);
    
    return RTOS_OK;
}