/**
 * @file ota_manager.c
 * @brief RTOS OTA更新管理模块实现 - 简化版本
 * @author Assistant
 * @date 2024
 */

#include "ota_manager.h"
#include "../hw_config.h"
#include "../hw_abstraction.h"
#include "../../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 全局OTA管理器实例 */
static rtos_ota_manager_t g_ota_manager;
static bool g_ota_manager_initialized = false;

/**
 * @brief 初始化OTA管理器
 */
rtos_result_t rtos_ota_manager_init(const rtos_ota_config_t *config)
{
    if (g_ota_manager_initialized) {
        return RTOS_OK;
    }
    
    if (!config) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 清零管理器结构 */
    memset(&g_ota_manager, 0, sizeof(g_ota_manager));
    
    /* 保存配置 */
    g_ota_manager.config = *config;
    
    /* 初始化当前固件信息 */
    g_ota_manager.current_firmware.version = 0x00010000; /* v1.0.0 */
    strcpy(g_ota_manager.current_firmware.version_string, "v1.0.0");
    strcpy(g_ota_manager.current_firmware.description, "Initial firmware version");
    g_ota_manager.current_firmware.size = 64 * 1024; /* 64KB */
    g_ota_manager.current_firmware.release_time = rtos_hw_get_system_time_ms();
    
    /* 初始化存储配置 */
    g_ota_manager.storage.download_partition_addr = 0x08040000; /* Flash地址示例 */
    g_ota_manager.storage.backup_partition_addr = 0x08080000;
    g_ota_manager.storage.partition_size = config->storage_partition_size;
    
    /* 初始化进度信息 */
    g_ota_manager.progress.state = RTOS_OTA_STATE_IDLE;
    strcpy(g_ota_manager.progress.status_message, "OTA Manager Ready");
    
    g_ota_manager.initialized = true;
    g_ota_manager_initialized = true;
    
    RTOS_OTA_DEBUG_PRINT("OTA manager initialized (device: %s)", config->device_id);
    return RTOS_OK;
}

/**
 * @brief 反初始化OTA管理器
 */
rtos_result_t rtos_ota_manager_deinit(void)
{
    if (!g_ota_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 如果有更新正在进行，先停止 */
    if (g_ota_manager.update_in_progress) {
        g_ota_manager.update_in_progress = false;
        g_ota_manager.progress.state = RTOS_OTA_STATE_IDLE;
    }
    
    g_ota_manager_initialized = false;
    
    RTOS_OTA_DEBUG_PRINT("OTA manager deinitialized");
    return RTOS_OK;
}

/**
 * @brief 检查固件更新
 */
rtos_result_t rtos_ota_manager_check_update(void)
{
    if (!g_ota_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (g_ota_manager.update_in_progress) {
        return RTOS_ERROR_BUSY;
    }
    
    g_ota_manager.progress.state = RTOS_OTA_STATE_CHECKING;
    strcpy(g_ota_manager.progress.status_message, "Checking for updates...");
    
    /* 简化实现：假设有可用更新 */
    g_ota_manager.available_firmware.version = 0x00010001; /* v1.0.1 */
    strcpy(g_ota_manager.available_firmware.version_string, "v1.0.1");
    strcpy(g_ota_manager.available_firmware.description, "Bug fixes and improvements");
    g_ota_manager.available_firmware.size = 66 * 1024; /* 66KB */
    g_ota_manager.available_firmware.mandatory = false;
    
    g_ota_manager.stats.check_count++;
    g_ota_manager.progress.state = RTOS_OTA_STATE_IDLE;
    strcpy(g_ota_manager.progress.status_message, "Update available");
    
    RTOS_OTA_DEBUG_PRINT("Update check completed: v%s available", 
                         g_ota_manager.available_firmware.version_string);
    
    return RTOS_OK;
}

/**
 * @brief 下载固件更新
 */
rtos_result_t rtos_ota_manager_download_firmware(const rtos_ota_firmware_info_t *firmware_info)
{
    if (!firmware_info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!g_ota_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    if (g_ota_manager.update_in_progress) {
        return RTOS_ERROR_BUSY;
    }
    
    g_ota_manager.update_in_progress = true;
    g_ota_manager.progress.state = RTOS_OTA_STATE_DOWNLOADING;
    g_ota_manager.progress.total_size = firmware_info->size;
    g_ota_manager.progress.downloaded_size = 0;
    strcpy(g_ota_manager.progress.status_message, "Downloading firmware...");
    
    /* 简化实现：模拟下载过程 */
    for (uint32_t i = 0; i < 10; i++) {
        g_ota_manager.progress.downloaded_size = (firmware_info->size * (i + 1)) / 10;
        g_ota_manager.progress.progress_percent = ((i + 1) * 100) / 10;
        
        rtos_hw_delay_ms(100); /* 模拟下载延时 */
    }
    
    g_ota_manager.stats.download_count++;
    g_ota_manager.stats.total_downloaded_bytes += firmware_info->size;
    g_ota_manager.progress.state = RTOS_OTA_STATE_COMPLETE;
    strcpy(g_ota_manager.progress.status_message, "Download completed");
    g_ota_manager.update_in_progress = false;
    
    RTOS_OTA_DEBUG_PRINT("Firmware download completed: %lu bytes", firmware_info->size);
    return RTOS_OK;
}

/**
 * @brief 获取当前固件信息
 */
rtos_result_t rtos_ota_manager_get_current_firmware_info(rtos_ota_firmware_info_t *firmware_info)
{
    if (!firmware_info) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!g_ota_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    *firmware_info = g_ota_manager.current_firmware;
    return RTOS_OK;
}

/**
 * @brief 获取OTA进度
 */
rtos_result_t rtos_ota_manager_get_progress(rtos_ota_progress_t *progress)
{
    if (!progress) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!g_ota_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    *progress = g_ota_manager.progress;
    return RTOS_OK;
}

/**
 * @brief OTA管理器周期性任务
 */
void rtos_ota_manager_periodic_task(void)
{
    if (!g_ota_manager_initialized) {
        return;
    }
    
    /* 自动检查更新 */
    if (g_ota_manager.config.auto_check_enable && !g_ota_manager.update_in_progress) {
        static uint32_t last_check_time = 0;
        uint32_t current_time = rtos_hw_get_system_time_ms();
        
        if (current_time - last_check_time >= g_ota_manager.config.check_interval_ms) {
            rtos_ota_manager_check_update();
            last_check_time = current_time;
        }
    }
}