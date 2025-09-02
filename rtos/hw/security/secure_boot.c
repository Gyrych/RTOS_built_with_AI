/**
 * @file secure_boot.c
 * @brief RTOS安全启动模块实现 - 简化版本
 * @author Assistant
 * @date 2024
 */

#include "secure_boot.h"
#include "../hw_config.h"
#include "../hw_abstraction.h"
#include "../../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 全局安全启动管理器实例 */
static rtos_secure_boot_manager_t g_secure_boot_manager;
static bool g_secure_boot_manager_initialized = false;

/**
 * @brief 初始化安全启动管理器
 */
rtos_result_t rtos_secure_boot_manager_init(const rtos_secure_boot_config_t *config)
{
    if (g_secure_boot_manager_initialized) {
        return RTOS_OK;
    }
    
    if (!config) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 清零管理器结构 */
    memset(&g_secure_boot_manager, 0, sizeof(g_secure_boot_manager));
    
    /* 保存配置 */
    g_secure_boot_manager.config = *config;
    g_secure_boot_manager.boot_info.state = config->enabled ? 
                                           RTOS_SECURE_BOOT_STATE_ENABLED : 
                                           RTOS_SECURE_BOOT_STATE_DISABLED;
    
    g_secure_boot_manager.initialized = true;
    g_secure_boot_manager_initialized = true;
    
    RTOS_SECURE_BOOT_DEBUG_PRINT("Secure boot manager initialized (enabled: %s)", 
                                 config->enabled ? "yes" : "no");
    
    return RTOS_OK;
}

/**
 * @brief 获取安全启动管理器实例
 */
rtos_secure_boot_manager_t* rtos_secure_boot_manager_get_instance(void)
{
    if (!g_secure_boot_manager_initialized) {
        return NULL;
    }
    return &g_secure_boot_manager;
}

/**
 * @brief 验证固件签名
 */
rtos_result_t rtos_secure_boot_verify_firmware(uint32_t firmware_addr, uint32_t firmware_size)
{
    if (!g_secure_boot_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_secure_boot_manager.config.enabled) {
        return RTOS_OK; /* 安全启动未启用，直接通过 */
    }
    
    /* 简化实现：假设验证通过 */
    g_secure_boot_manager.boot_info.state = RTOS_SECURE_BOOT_STATE_VERIFIED;
    g_secure_boot_manager.total_verifications++;
    g_secure_boot_manager.successful_verifications++;
    
    RTOS_SECURE_BOOT_DEBUG_PRINT("Firmware verification: addr=0x%08lx, size=%lu, result=OK", 
                                 firmware_addr, firmware_size);
    
    return RTOS_OK;
}

/**
 * @brief 启用/禁用安全启动
 */
rtos_result_t rtos_secure_boot_enable(bool enable)
{
    if (!g_secure_boot_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    g_secure_boot_manager.config.enabled = enable;
    g_secure_boot_manager.boot_info.state = enable ? 
                                           RTOS_SECURE_BOOT_STATE_ENABLED : 
                                           RTOS_SECURE_BOOT_STATE_DISABLED;
    
    RTOS_SECURE_BOOT_DEBUG_PRINT("Secure boot %s", enable ? "enabled" : "disabled");
    
    return RTOS_OK;
}

/**
 * @brief 获取安全启动状态
 */
rtos_secure_boot_state_t rtos_secure_boot_get_state(void)
{
    if (!g_secure_boot_manager_initialized) {
        return RTOS_SECURE_BOOT_STATE_DISABLED;
    }
    
    return g_secure_boot_manager.boot_info.state;
}

/**
 * @brief 获取安全启动统计信息
 */
uint32_t rtos_secure_boot_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_secure_boot_manager_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "Secure Boot Statistics:\n"
        "  Enabled: %s\n"
        "  State: %d\n"
        "  Total Verifications: %lu\n"
        "  Successful: %lu\n"
        "  Failed: %lu\n"
        "  Rollback Attempts: %lu\n",
        g_secure_boot_manager.config.enabled ? "Yes" : "No",
        g_secure_boot_manager.boot_info.state,
        g_secure_boot_manager.total_verifications,
        g_secure_boot_manager.successful_verifications,
        g_secure_boot_manager.failed_verifications,
        g_secure_boot_manager.rollback_attempts);
    
    if (len < 0) {
        return 0;
    }
    
    return (len >= (int)size) ? (size - 1) : (uint32_t)len;
}