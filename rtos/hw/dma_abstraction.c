/**
 * @file dma_abstraction.c
 * @brief RTOS DMA抽象模块实现 - STM32F4平台
 * @author Assistant
 * @date 2024
 */

#include "dma_abstraction.h"
#include "hw_config.h"
#include "hw_abstraction.h"
#include "../core/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 包含STM32F4标准固件库头文件 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#include "fwlib/CMSIS/STM32F4xx/Include/stm32f4xx.h"
#include "fwlib/inc/stm32f4xx_dma.h"
#include "fwlib/inc/stm32f4xx_rcc.h"
#include "fwlib/inc/misc.h"
#endif

/* 全局DMA管理器实例 */
static rtos_dma_manager_t g_dma_manager;
static bool g_dma_manager_initialized = false;

/* STM32F4平台相关数据 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
static DMA_TypeDef* const g_dma_controllers[RTOS_DMA_CONTROLLER_MAX] = {
    DMA1, DMA2
};

static DMA_Stream_TypeDef* const g_dma_streams[RTOS_DMA_CONTROLLER_MAX][RTOS_DMA_STREAM_MAX] = {
    {DMA1_Stream0, DMA1_Stream1, DMA1_Stream2, DMA1_Stream3, DMA1_Stream4, DMA1_Stream5, DMA1_Stream6, DMA1_Stream7},
    {DMA2_Stream0, DMA2_Stream1, DMA2_Stream2, DMA2_Stream3, DMA2_Stream4, DMA2_Stream5, DMA2_Stream6, DMA2_Stream7}
};

static const uint32_t g_dma_rcc_clocks[RTOS_DMA_CONTROLLER_MAX] = {
    RCC_AHB1Periph_DMA1, RCC_AHB1Periph_DMA2
};

static const IRQn_Type g_dma_irq_numbers[RTOS_DMA_CONTROLLER_MAX][RTOS_DMA_STREAM_MAX] = {
    {DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn,
     DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn},
    {DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn,
     DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn}
};
#endif

/* 内部函数声明 */
static rtos_result_t rtos_dma_platform_init(void);
static rtos_result_t rtos_dma_platform_config_stream(rtos_dma_handle_t *handle);
static rtos_result_t rtos_dma_platform_start_transfer(rtos_dma_handle_t *handle);
static rtos_result_t rtos_dma_platform_stop_transfer(rtos_dma_handle_t *handle);
static uint32_t rtos_dma_platform_get_remaining_data(rtos_dma_handle_t *handle);
static uint32_t rtos_dma_platform_get_transfer_flags(rtos_dma_handle_t *handle);
static void rtos_dma_platform_clear_flags(rtos_dma_handle_t *handle, uint32_t flags);
static void rtos_dma_trigger_event(rtos_dma_handle_t *handle, rtos_dma_event_t event);
static rtos_result_t rtos_dma_validate_config(const rtos_dma_config_t *config);
static rtos_result_t rtos_dma_calculate_transfer_time(rtos_dma_handle_t *handle, uint32_t *time_ms);

/**
 * @brief 初始化DMA管理器
 */
rtos_result_t rtos_dma_manager_init(uint32_t zero_copy_queue_size)
{
    if (g_dma_manager_initialized) {
        return RTOS_OK;
    }
    
    /* 清零管理器结构 */
    memset(&g_dma_manager, 0, sizeof(g_dma_manager));
    
    /* 初始化DMA句柄数组 */
    for (uint32_t ctrl = 0; ctrl < RTOS_DMA_CONTROLLER_MAX; ctrl++) {
        for (uint32_t stream = 0; stream < RTOS_DMA_STREAM_MAX; stream++) {
            rtos_dma_handle_t *handle = &g_dma_manager.dma_handles[ctrl][stream];
            handle->controller = (rtos_dma_controller_t)ctrl;
            handle->stream = (rtos_dma_stream_t)stream;
            handle->state = RTOS_DMA_STATE_RESET;
            handle->initialized = false;
            handle->allocated = false;
        }
    }
    
    /* 初始化零拷贝队列 */
    if (zero_copy_queue_size > 0) {
        g_dma_manager.zero_copy_queue.queue = malloc(sizeof(rtos_dma_transfer_t) * zero_copy_queue_size);
        if (!g_dma_manager.zero_copy_queue.queue) {
            RTOS_DMA_DEBUG_PRINT("Failed to allocate zero copy queue");
            return RTOS_ERROR_NO_MEMORY;
        }
        
        g_dma_manager.zero_copy_queue.size = zero_copy_queue_size;
        g_dma_manager.zero_copy_queue.head = 0;
        g_dma_manager.zero_copy_queue.tail = 0;
        g_dma_manager.zero_copy_queue.count = 0;
    }
    
    /* 调用平台相关初始化 */
    rtos_result_t result = rtos_dma_platform_init();
    if (result != RTOS_OK) {
        if (g_dma_manager.zero_copy_queue.queue) {
            free(g_dma_manager.zero_copy_queue.queue);
        }
        RTOS_DMA_DEBUG_PRINT("Platform initialization failed: %d", result);
        return result;
    }
    
    g_dma_manager.initialized = true;
    g_dma_manager_initialized = true;
    
    RTOS_DMA_DEBUG_PRINT("DMA manager initialized (queue size: %lu)", zero_copy_queue_size);
    return RTOS_OK;
}

/**
 * @brief 反初始化DMA管理器
 */
rtos_result_t rtos_dma_manager_deinit(void)
{
    if (!g_dma_manager_initialized) {
        return RTOS_ERROR_NOT_INITIALIZED;
    }
    
    /* 停止所有活动的DMA传输 */
    for (uint32_t ctrl = 0; ctrl < RTOS_DMA_CONTROLLER_MAX; ctrl++) {
        for (uint32_t stream = 0; stream < RTOS_DMA_STREAM_MAX; stream++) {
            rtos_dma_handle_t *handle = &g_dma_manager.dma_handles[ctrl][stream];
            if (handle->allocated) {
                rtos_dma_manager_free_stream(handle);
            }
        }
    }
    
    /* 释放零拷贝队列 */
    if (g_dma_manager.zero_copy_queue.queue) {
        free(g_dma_manager.zero_copy_queue.queue);
        g_dma_manager.zero_copy_queue.queue = NULL;
    }
    
    g_dma_manager_initialized = false;
    
    RTOS_DMA_DEBUG_PRINT("DMA manager deinitialized");
    return RTOS_OK;
}

/**
 * @brief 获取DMA管理器实例
 */
rtos_dma_manager_t* rtos_dma_manager_get_instance(void)
{
    if (!g_dma_manager_initialized) {
        return NULL;
    }
    return &g_dma_manager;
}

/**
 * @brief 分配DMA流
 */
rtos_result_t rtos_dma_manager_allocate_stream(rtos_dma_controller_t controller,
                                              rtos_dma_stream_t stream,
                                              const rtos_dma_config_t *config,
                                              rtos_dma_handle_t **handle)
{
    RTOS_DMA_CHECK_PARAM(controller < RTOS_DMA_CONTROLLER_MAX);
    RTOS_DMA_CHECK_PARAM(stream < RTOS_DMA_STREAM_MAX);
    RTOS_DMA_CHECK_PARAM(config != NULL);
    RTOS_DMA_CHECK_PARAM(handle != NULL);
    RTOS_DMA_CHECK_INIT();
    
    /* 验证配置参数 */
    rtos_result_t result = rtos_dma_validate_config(config);
    if (result != RTOS_OK) {
        return result;
    }
    
    rtos_dma_handle_t *dma_handle = &g_dma_manager.dma_handles[controller][stream];
    
    /* 检查流是否已被分配 */
    if (dma_handle->allocated) {
        RTOS_DMA_DEBUG_PRINT("DMA stream already allocated: DMA%d Stream%d", 
                             controller + 1, stream);
        return RTOS_ERROR_RESOURCE_BUSY;
    }
    
    /* 配置DMA句柄 */
    dma_handle->config = *config;
    dma_handle->state = RTOS_DMA_STATE_READY;
    dma_handle->allocated = true;
    
    /* 清零统计信息 */
    memset(&dma_handle->stats, 0, sizeof(rtos_dma_stats_t));
    
    /* 调用平台相关配置 */
    result = rtos_dma_platform_config_stream(dma_handle);
    if (result != RTOS_OK) {
        dma_handle->allocated = false;
        dma_handle->state = RTOS_DMA_STATE_RESET;
        RTOS_DMA_DEBUG_PRINT("Platform config failed: %d", result);
        return result;
    }
    
    dma_handle->initialized = true;
    g_dma_manager.active_streams++;
    
    *handle = dma_handle;
    
    RTOS_DMA_DEBUG_PRINT("DMA stream allocated: DMA%d Stream%d Channel%d", 
                         controller + 1, stream, config->channel);
    
    return RTOS_OK;
}

/**
 * @brief 释放DMA流
 */
rtos_result_t rtos_dma_manager_free_stream(rtos_dma_handle_t *handle)
{
    RTOS_DMA_CHECK_HANDLE(handle);
    RTOS_DMA_CHECK_INIT();
    
    /* 停止传输 */
    if (handle->state == RTOS_DMA_STATE_BUSY) {
        rtos_dma_manager_abort_transfer(handle);
    }
    
    /* 调用平台相关释放 */
    rtos_dma_platform_stop_transfer(handle);
    
    /* 清空事件回调 */
    for (int i = 0; i < RTOS_DMA_EVENT_MAX; i++) {
        handle->event_callbacks[i] = NULL;
        handle->event_contexts[i] = NULL;
    }
    
    /* 重置句柄状态 */
    handle->allocated = false;
    handle->initialized = false;
    handle->state = RTOS_DMA_STATE_RESET;
    g_dma_manager.active_streams--;
    
    RTOS_DMA_DEBUG_PRINT("DMA stream freed: DMA%d Stream%d", 
                         handle->controller + 1, handle->stream);
    
    return RTOS_OK;
}

/**
 * @brief 启动DMA传输
 */
rtos_result_t rtos_dma_manager_start_transfer(rtos_dma_handle_t *handle,
                                             const rtos_dma_transfer_t *transfer)
{
    RTOS_DMA_CHECK_HANDLE(handle);
    RTOS_DMA_CHECK_PARAM(transfer != NULL);
    RTOS_DMA_CHECK_PARAM(transfer->src_addr != NULL);
    RTOS_DMA_CHECK_PARAM(transfer->dst_addr != NULL);
    RTOS_DMA_CHECK_PARAM(transfer->data_length > 0);
    RTOS_DMA_CHECK_INIT();
    
    if (handle->state == RTOS_DMA_STATE_BUSY) {
        return RTOS_ERROR_BUSY;
    }
    
    /* 保存传输信息 */
    handle->current_transfer = *transfer;
    handle->transfer_start_time = rtos_hw_get_system_time_ms();
    handle->state = RTOS_DMA_STATE_BUSY;
    
    /* 启动平台相关传输 */
    rtos_result_t result = rtos_dma_platform_start_transfer(handle);
    if (result != RTOS_OK) {
        handle->state = RTOS_DMA_STATE_ERROR;
        handle->stats.error_count++;
        RTOS_DMA_DEBUG_PRINT("Platform transfer start failed: %d", result);
        return result;
    }
    
    g_dma_manager.total_transfers++;
    handle->stats.transfer_count++;
    
    RTOS_DMA_DEBUG_PRINT("DMA transfer started: DMA%d Stream%d, %lu bytes, %s", 
                         handle->controller + 1, handle->stream, transfer->data_length,
                         transfer->blocking ? "blocking" : "async");
    
    /* 如果是阻塞模式，等待传输完成 */
    if (transfer->blocking) {
        uint32_t timeout_start = rtos_hw_get_system_time_ms();
        
        while (handle->state == RTOS_DMA_STATE_BUSY) {
            if (transfer->timeout_ms > 0 && 
                rtos_hw_get_system_time_ms() - timeout_start >= transfer->timeout_ms) {
                
                rtos_dma_manager_abort_transfer(handle);
                handle->stats.timeout_count++;
                RTOS_DMA_DEBUG_PRINT("DMA transfer timeout");
                return RTOS_ERROR_TIMEOUT;
            }
            
            /* 检查传输状态 */
            if (rtos_dma_manager_is_transfer_complete(handle)) {
                break;
            }
            
            rtos_hw_delay_us(10); /* 短暂延时 */
        }
    }
    
    return RTOS_OK;
}

/**
 * @brief 停止DMA传输
 */
rtos_result_t rtos_dma_manager_stop_transfer(rtos_dma_handle_t *handle)
{
    RTOS_DMA_CHECK_HANDLE(handle);
    RTOS_DMA_CHECK_INIT();
    
    if (handle->state != RTOS_DMA_STATE_BUSY) {
        return RTOS_OK; /* 已经停止 */
    }
    
    rtos_result_t result = rtos_dma_platform_stop_transfer(handle);
    if (result == RTOS_OK) {
        handle->state = RTOS_DMA_STATE_READY;
        
        /* 更新统计信息 */
        uint32_t transfer_time = rtos_hw_get_system_time_ms() - handle->transfer_start_time;
        rtos_dma_calculate_transfer_time(handle, &transfer_time);
        
        RTOS_DMA_DEBUG_PRINT("DMA transfer stopped: DMA%d Stream%d", 
                             handle->controller + 1, handle->stream);
    }
    
    return result;
}

/**
 * @brief 中止DMA传输
 */
rtos_result_t rtos_dma_manager_abort_transfer(rtos_dma_handle_t *handle)
{
    RTOS_DMA_CHECK_HANDLE(handle);
    RTOS_DMA_CHECK_INIT();
    
    rtos_result_t result = rtos_dma_platform_stop_transfer(handle);
    if (result == RTOS_OK) {
        handle->state = RTOS_DMA_STATE_ABORT;
        
        RTOS_DMA_DEBUG_PRINT("DMA transfer aborted: DMA%d Stream%d", 
                             handle->controller + 1, handle->stream);
    }
    
    return result;
}

/**
 * @brief 检查DMA传输是否完成
 */
bool rtos_dma_manager_is_transfer_complete(rtos_dma_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }
    
    if (handle->state != RTOS_DMA_STATE_BUSY) {
        return true;
    }
    
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    DMA_Stream_TypeDef *dma_stream = g_dma_streams[handle->controller][handle->stream];
    
    /* 检查传输完成标志 */
    uint32_t flags = rtos_dma_platform_get_transfer_flags(handle);
    uint32_t tc_flag = DMA_FLAG_TCIF0 << (handle->stream * 6);
    
    if (flags & tc_flag) {
        /* 传输完成 */
        handle->state = RTOS_DMA_STATE_COMPLETE;
        
        /* 更新统计信息 */
        uint32_t transfer_time = rtos_hw_get_system_time_ms() - handle->transfer_start_time;
        rtos_dma_calculate_transfer_time(handle, &transfer_time);
        
        handle->stats.transfer_bytes += handle->current_transfer.data_length;
        g_dma_manager.total_bytes += handle->current_transfer.data_length;
        
        /* 清除标志 */
        rtos_dma_platform_clear_flags(handle, tc_flag);
        
        /* 触发传输完成事件 */
        rtos_dma_trigger_event(handle, RTOS_DMA_EVENT_TRANSFER_COMPLETE);
        
        RTOS_DMA_DEBUG_PRINT("DMA transfer complete: DMA%d Stream%d, %lu bytes in %lu ms", 
                             handle->controller + 1, handle->stream, 
                             handle->current_transfer.data_length, transfer_time);
        
        return true;
    }
    
    /* 检查错误标志 */
    uint32_t error_flags = (DMA_FLAG_TEIF0 | DMA_FLAG_DMEIF0 | DMA_FLAG_FEIF0) << (handle->stream * 6);
    if (flags & error_flags) {
        handle->state = RTOS_DMA_STATE_ERROR;
        handle->stats.error_count++;
        g_dma_manager.total_errors++;
        
        /* 清除错误标志 */
        rtos_dma_platform_clear_flags(handle, error_flags);
        
        /* 触发错误事件 */
        if (flags & (DMA_FLAG_TEIF0 << (handle->stream * 6))) {
            rtos_dma_trigger_event(handle, RTOS_DMA_EVENT_TRANSFER_ERROR);
        }
        if (flags & (DMA_FLAG_FEIF0 << (handle->stream * 6))) {
            rtos_dma_trigger_event(handle, RTOS_DMA_EVENT_FIFO_ERROR);
        }
        if (flags & (DMA_FLAG_DMEIF0 << (handle->stream * 6))) {
            rtos_dma_trigger_event(handle, RTOS_DMA_EVENT_DIRECT_MODE_ERROR);
        }
        
        RTOS_DMA_DEBUG_PRINT("DMA transfer error: DMA%d Stream%d, flags=0x%08lx", 
                             handle->controller + 1, handle->stream, flags);
        
        return true; /* 错误也算传输结束 */
    }
    
    return false;
#else
    return true; /* 非STM32平台简化实现 */
#endif
}

/**
 * @brief 获取DMA传输进度
 */
rtos_result_t rtos_dma_manager_get_transfer_progress(rtos_dma_handle_t *handle,
                                                    uint32_t *transferred,
                                                    uint32_t *remaining)
{
    RTOS_DMA_CHECK_HANDLE(handle);
    RTOS_DMA_CHECK_INIT();
    
    if (handle->state != RTOS_DMA_STATE_BUSY) {
        if (transferred) *transferred = handle->current_transfer.data_length;
        if (remaining) *remaining = 0;
        return RTOS_OK;
    }
    
    uint32_t remaining_data = rtos_dma_platform_get_remaining_data(handle);
    uint32_t transferred_data = handle->current_transfer.data_length - remaining_data;
    
    if (transferred) *transferred = transferred_data;
    if (remaining) *remaining = remaining_data;
    
    return RTOS_OK;
}

/**
 * @brief 零拷贝发送数据
 */
rtos_result_t rtos_dma_manager_zero_copy_send(uint32_t peripheral_id, 
                                             const void *data, 
                                             uint32_t length)
{
    RTOS_DMA_CHECK_PARAM(data != NULL);
    RTOS_DMA_CHECK_PARAM(length > 0);
    RTOS_DMA_CHECK_INIT();
    
    /* 简化实现：使用DMA2 Stream6进行内存到外设传输 */
    rtos_dma_config_t config = RTOS_DMA_DEFAULT_CONFIG(
        RTOS_DMA_CONTROLLER_2, RTOS_DMA_STREAM_6, 
        RTOS_DMA_CHANNEL_4, RTOS_DMA_DIR_MEMORY_TO_PERIPH);
    
    config.memory_increment = true;
    config.periph_increment = false;
    config.priority = RTOS_DMA_PRIORITY_HIGH;
    
    rtos_dma_handle_t *handle;
    rtos_result_t result = rtos_dma_manager_allocate_stream(
        RTOS_DMA_CONTROLLER_2, RTOS_DMA_STREAM_6, &config, &handle);
    
    if (result != RTOS_OK) {
        return result;
    }
    
    /* 创建传输请求 */
    rtos_dma_transfer_t transfer = RTOS_DMA_MAKE_TRANSFER(data, (void*)peripheral_id, length);
    transfer.blocking = false; /* 零拷贝为异步传输 */
    
    result = rtos_dma_manager_start_transfer(handle, &transfer);
    
    RTOS_DMA_DEBUG_PRINT("Zero copy send started: peripheral=0x%08lx, %lu bytes", 
                         peripheral_id, length);
    
    return result;
}

/**
 * @brief 零拷贝接收数据
 */
rtos_result_t rtos_dma_manager_zero_copy_receive(uint32_t peripheral_id, 
                                                void *buffer, 
                                                uint32_t length)
{
    RTOS_DMA_CHECK_PARAM(buffer != NULL);
    RTOS_DMA_CHECK_PARAM(length > 0);
    RTOS_DMA_CHECK_INIT();
    
    /* 简化实现：使用DMA2 Stream5进行外设到内存传输 */
    rtos_dma_config_t config = RTOS_DMA_DEFAULT_CONFIG(
        RTOS_DMA_CONTROLLER_2, RTOS_DMA_STREAM_5, 
        RTOS_DMA_CHANNEL_4, RTOS_DMA_DIR_PERIPH_TO_MEMORY);
    
    config.memory_increment = true;
    config.periph_increment = false;
    config.priority = RTOS_DMA_PRIORITY_HIGH;
    
    rtos_dma_handle_t *handle;
    rtos_result_t result = rtos_dma_manager_allocate_stream(
        RTOS_DMA_CONTROLLER_2, RTOS_DMA_STREAM_5, &config, &handle);
    
    if (result != RTOS_OK) {
        return result;
    }
    
    /* 创建传输请求 */
    rtos_dma_transfer_t transfer = RTOS_DMA_MAKE_TRANSFER((void*)peripheral_id, buffer, length);
    transfer.blocking = false; /* 零拷贝为异步传输 */
    
    result = rtos_dma_manager_start_transfer(handle, &transfer);
    
    RTOS_DMA_DEBUG_PRINT("Zero copy receive started: peripheral=0x%08lx, %lu bytes", 
                         peripheral_id, length);
    
    return result;
}

/**
 * @brief 内存拷贝（使用DMA）
 */
rtos_result_t rtos_dma_manager_memcpy(void *dst, const void *src, uint32_t length)
{
    RTOS_DMA_CHECK_PARAM(dst != NULL);
    RTOS_DMA_CHECK_PARAM(src != NULL);
    RTOS_DMA_CHECK_PARAM(length > 0);
    RTOS_DMA_CHECK_INIT();
    
    /* 使用DMA2 Stream0进行内存到内存传输 */
    rtos_dma_config_t config = RTOS_DMA_DEFAULT_CONFIG(
        RTOS_DMA_CONTROLLER_2, RTOS_DMA_STREAM_0, 
        RTOS_DMA_CHANNEL_0, RTOS_DMA_DIR_MEMORY_TO_MEMORY);
    
    config.memory_increment = true;
    config.periph_increment = true; /* 内存到内存时，两端都递增 */
    config.priority = RTOS_DMA_PRIORITY_VERY_HIGH;
    config.memory_data_size = RTOS_DMA_DATA_SIZE_WORD; /* 32位传输 */
    config.periph_data_size = RTOS_DMA_DATA_SIZE_WORD;
    
    rtos_dma_handle_t *handle;
    rtos_result_t result = rtos_dma_manager_allocate_stream(
        RTOS_DMA_CONTROLLER_2, RTOS_DMA_STREAM_0, &config, &handle);
    
    if (result != RTOS_OK) {
        /* DMA不可用时回退到软件拷贝 */
        memcpy(dst, src, length);
        return RTOS_OK;
    }
    
    /* 创建传输请求 */
    rtos_dma_transfer_t transfer = RTOS_DMA_MAKE_TRANSFER(src, dst, length / 4); /* 32位传输 */
    transfer.blocking = true; /* 内存拷贝为同步操作 */
    transfer.timeout_ms = 100;
    
    result = rtos_dma_manager_start_transfer(handle, &transfer);
    
    /* 释放DMA流 */
    rtos_dma_manager_free_stream(handle);
    
    RTOS_DMA_DEBUG_PRINT("DMA memcpy: %p -> %p, %lu bytes", src, dst, length);
    
    return result;
}

/**
 * @brief DMA中断处理函数
 */
void rtos_dma_manager_interrupt_handler(rtos_dma_controller_t controller, rtos_dma_stream_t stream)
{
    if (!g_dma_manager_initialized || 
        controller >= RTOS_DMA_CONTROLLER_MAX || 
        stream >= RTOS_DMA_STREAM_MAX) {
        return;
    }
    
    rtos_dma_handle_t *handle = &g_dma_manager.dma_handles[controller][stream];
    
    if (!handle->allocated || !handle->initialized) {
        return;
    }
    
    /* 检查并处理传输状态 */
    rtos_dma_manager_is_transfer_complete(handle);
    
    RTOS_DMA_DEBUG_PRINT("DMA interrupt handled: DMA%d Stream%d", 
                         controller + 1, stream);
}

/**
 * @brief 获取DMA管理器统计信息
 */
uint32_t rtos_dma_manager_get_statistics(char *buffer, uint32_t size)
{
    if (!buffer || size == 0 || !g_dma_manager_initialized) {
        return 0;
    }
    
    int len = snprintf(buffer, size,
        "DMA Manager Statistics:\n"
        "  Active Streams: %lu\n"
        "  Total Transfers: %lu\n"
        "  Total Bytes: %llu\n"
        "  Total Errors: %lu\n"
        "  Zero Copy Queue: %lu/%lu\n"
        "  Stream Usage:\n",
        g_dma_manager.active_streams,
        g_dma_manager.total_transfers,
        g_dma_manager.total_bytes,
        g_dma_manager.total_errors,
        g_dma_manager.zero_copy_queue.count,
        g_dma_manager.zero_copy_queue.size);
    
    /* 显示每个流的使用情况 */
    for (uint32_t ctrl = 0; ctrl < RTOS_DMA_CONTROLLER_MAX; ctrl++) {
        for (uint32_t stream = 0; stream < RTOS_DMA_STREAM_MAX; stream++) {
            rtos_dma_handle_t *handle = &g_dma_manager.dma_handles[ctrl][stream];
            
            if (handle->allocated) {
                int stream_len = snprintf(buffer + len, size - len,
                    "    DMA%lu Stream%lu: %lu transfers, %lu bytes, %lu errors\n",
                    ctrl + 1, stream, handle->stats.transfer_count,
                    handle->stats.transfer_bytes, handle->stats.error_count);
                    
                if (stream_len > 0 && len + stream_len < (int)size) {
                    len += stream_len;
                }
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
 * @brief 平台相关DMA初始化
 */
static rtos_result_t rtos_dma_platform_init(void)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* 使能DMA时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    
    RTOS_DMA_DEBUG_PRINT("DMA controllers clock enabled");
    return RTOS_OK;
#else
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关DMA流配置
 */
static rtos_result_t rtos_dma_platform_config_stream(rtos_dma_handle_t *handle)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    DMA_Stream_TypeDef *dma_stream = g_dma_streams[handle->controller][handle->stream];
    
    /* 禁用DMA流 */
    DMA_Cmd(dma_stream, DISABLE);
    
    /* 等待DMA流禁用 */
    while (DMA_GetCmdStatus(dma_stream) != DISABLE);
    
    /* 配置DMA流 */
    DMA_InitTypeDef dma_init;
    DMA_StructInit(&dma_init);
    
    dma_init.DMA_Channel = handle->config.channel;
    dma_init.DMA_PeripheralBaseAddr = 0; /* 将在传输时设置 */
    dma_init.DMA_Memory0BaseAddr = 0;    /* 将在传输时设置 */
    dma_init.DMA_BufferSize = 0;         /* 将在传输时设置 */
    
    /* 设置传输方向 */
    switch (handle->config.direction) {
        case RTOS_DMA_DIR_PERIPH_TO_MEMORY:
            dma_init.DMA_DIR = DMA_DIR_PeripheralToMemory;
            break;
        case RTOS_DMA_DIR_MEMORY_TO_PERIPH:
            dma_init.DMA_DIR = DMA_DIR_MemoryToPeripheral;
            break;
        case RTOS_DMA_DIR_MEMORY_TO_MEMORY:
            dma_init.DMA_DIR = DMA_DIR_MemoryToMemory;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 设置数据大小 */
    dma_init.DMA_PeripheralDataSize = (handle->config.periph_data_size == RTOS_DMA_DATA_SIZE_BYTE) ? 
                                     DMA_PeripheralDataSize_Byte : 
                                     (handle->config.periph_data_size == RTOS_DMA_DATA_SIZE_HALFWORD) ?
                                     DMA_PeripheralDataSize_HalfWord : DMA_PeripheralDataSize_Word;
    
    dma_init.DMA_MemoryDataSize = (handle->config.memory_data_size == RTOS_DMA_DATA_SIZE_BYTE) ?
                                 DMA_MemoryDataSize_Byte :
                                 (handle->config.memory_data_size == RTOS_DMA_DATA_SIZE_HALFWORD) ?
                                 DMA_MemoryDataSize_HalfWord : DMA_MemoryDataSize_Word;
    
    /* 设置地址递增 */
    dma_init.DMA_PeripheralInc = handle->config.periph_increment ? DMA_PeripheralInc_Enable : DMA_PeripheralInc_Disable;
    dma_init.DMA_MemoryInc = handle->config.memory_increment ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable;
    
    /* 设置传输模式 */
    dma_init.DMA_Mode = (handle->config.mode == RTOS_DMA_MODE_CIRCULAR) ? 
                       DMA_Mode_Circular : DMA_Mode_Normal;
    
    /* 设置优先级 */
    switch (handle->config.priority) {
        case RTOS_DMA_PRIORITY_LOW:
            dma_init.DMA_Priority = DMA_Priority_Low;
            break;
        case RTOS_DMA_PRIORITY_MEDIUM:
            dma_init.DMA_Priority = DMA_Priority_Medium;
            break;
        case RTOS_DMA_PRIORITY_HIGH:
            dma_init.DMA_Priority = DMA_Priority_High;
            break;
        case RTOS_DMA_PRIORITY_VERY_HIGH:
            dma_init.DMA_Priority = DMA_Priority_VeryHigh;
            break;
        default:
            return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* FIFO配置 */
    dma_init.DMA_FIFOMode = handle->config.fifo_mode ? DMA_FIFOMode_Enable : DMA_FIFOMode_Disable;
    dma_init.DMA_FIFOThreshold = handle->config.fifo_threshold;
    dma_init.DMA_MemoryBurst = handle->config.memory_burst;
    dma_init.DMA_PeripheralBurst = handle->config.periph_burst;
    
    DMA_Init(dma_stream, &dma_init);
    
    /* 配置中断 */
    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = g_dma_irq_numbers[handle->controller][handle->stream];
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init.NVIC_IRQChannelSubPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
    
    /* 使能DMA中断 */
    DMA_ITConfig(dma_stream, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);
    
    return RTOS_OK;
#else
    (void)handle;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 平台相关DMA传输启动
 */
static rtos_result_t rtos_dma_platform_start_transfer(rtos_dma_handle_t *handle)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    DMA_Stream_TypeDef *dma_stream = g_dma_streams[handle->controller][handle->stream];
    
    /* 禁用DMA流 */
    DMA_Cmd(dma_stream, DISABLE);
    while (DMA_GetCmdStatus(dma_stream) != DISABLE);
    
    /* 设置传输地址和长度 */
    if (handle->config.direction == RTOS_DMA_DIR_MEMORY_TO_MEMORY) {
        dma_stream->PAR = (uint32_t)handle->current_transfer.src_addr;
        dma_stream->M0AR = (uint32_t)handle->current_transfer.dst_addr;
    } else if (handle->config.direction == RTOS_DMA_DIR_MEMORY_TO_PERIPH) {
        dma_stream->PAR = (uint32_t)handle->current_transfer.dst_addr;
        dma_stream->M0AR = (uint32_t)handle->current_transfer.src_addr;
    } else {
        dma_stream->PAR = (uint32_t)handle->current_transfer.src_addr;
        dma_stream->M0AR = (uint32_t)handle->current_transfer.dst_addr;
    }
    
    dma_stream->NDTR = handle->current_transfer.data_length;
    
    /* 启用DMA流 */
    DMA_Cmd(dma_stream, ENABLE);
    
    return RTOS_OK;
#else
    (void)handle;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 验证DMA配置
 */
static rtos_result_t rtos_dma_validate_config(const rtos_dma_config_t *config)
{
    if (config->controller >= RTOS_DMA_CONTROLLER_MAX ||
        config->stream >= RTOS_DMA_STREAM_MAX ||
        config->channel >= RTOS_DMA_CHANNEL_MAX ||
        config->direction >= RTOS_DMA_DIR_MAX ||
        config->priority >= RTOS_DMA_PRIORITY_MAX ||
        config->mode >= RTOS_DMA_MODE_MAX) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    return RTOS_OK;
}

/**
 * @brief 计算传输时间统计
 */
static rtos_result_t rtos_dma_calculate_transfer_time(rtos_dma_handle_t *handle, uint32_t *time_ms)
{
    if (!handle || !time_ms) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* 更新传输时间统计 */
    if (*time_ms > handle->stats.max_transfer_time_ms) {
        handle->stats.max_transfer_time_ms = *time_ms;
    }
    
    if (handle->stats.min_transfer_time_ms == 0 || *time_ms < handle->stats.min_transfer_time_ms) {
        handle->stats.min_transfer_time_ms = *time_ms;
    }
    
    /* 计算平均传输时间 */
    if (handle->stats.transfer_count > 1) {
        handle->stats.avg_transfer_time_ms = 
            (handle->stats.avg_transfer_time_ms * (handle->stats.transfer_count - 1) + *time_ms) /
            handle->stats.transfer_count;
    } else {
        handle->stats.avg_transfer_time_ms = *time_ms;
    }
    
    /* 计算吞吐量 */
    if (*time_ms > 0) {
        uint32_t throughput = (handle->current_transfer.data_length * 1000) / (*time_ms * 1024 * 1024); /* MB/s */
        if (throughput > handle->stats.max_throughput_mbps) {
            handle->stats.max_throughput_mbps = throughput;
        }
    }
    
    return RTOS_OK;
}

/* 平台相关辅助函数 */

/**
 * @brief 获取DMA剩余数据量
 */
static uint32_t rtos_dma_platform_get_remaining_data(rtos_dma_handle_t *handle)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    DMA_Stream_TypeDef *dma_stream = g_dma_streams[handle->controller][handle->stream];
    return DMA_GetCurrDataCounter(dma_stream);
#else
    (void)handle;
    return 0;
#endif
}

/**
 * @brief 获取DMA传输标志
 */
static uint32_t rtos_dma_platform_get_transfer_flags(rtos_dma_handle_t *handle)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    DMA_TypeDef *dma_controller = g_dma_controllers[handle->controller];
    
    if (handle->stream < 4) {
        return dma_controller->LISR;
    } else {
        return dma_controller->HISR;
    }
#else
    (void)handle;
    return 0;
#endif
}

/**
 * @brief 清除DMA标志
 */
static void rtos_dma_platform_clear_flags(rtos_dma_handle_t *handle, uint32_t flags)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    DMA_TypeDef *dma_controller = g_dma_controllers[handle->controller];
    
    if (handle->stream < 4) {
        dma_controller->LIFCR = flags;
    } else {
        dma_controller->HIFCR = flags;
    }
#else
    (void)handle;
    (void)flags;
#endif
}

/**
 * @brief 停止DMA传输
 */
static rtos_result_t rtos_dma_platform_stop_transfer(rtos_dma_handle_t *handle)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    DMA_Stream_TypeDef *dma_stream = g_dma_streams[handle->controller][handle->stream];
    
    /* 禁用DMA流 */
    DMA_Cmd(dma_stream, DISABLE);
    
    /* 等待DMA流禁用 */
    uint32_t timeout = rtos_hw_get_system_time_ms() + 100;
    while (DMA_GetCmdStatus(dma_stream) != DISABLE) {
        if (rtos_hw_get_system_time_ms() >= timeout) {
            RTOS_DMA_DEBUG_PRINT("DMA stream disable timeout");
            return RTOS_ERROR_TIMEOUT;
        }
    }
    
    /* 清除所有标志 */
    uint32_t all_flags = (DMA_FLAG_TCIF0 | DMA_FLAG_HTIF0 | DMA_FLAG_TEIF0 | 
                         DMA_FLAG_DMEIF0 | DMA_FLAG_FEIF0) << (handle->stream * 6);
    rtos_dma_platform_clear_flags(handle, all_flags);
    
    return RTOS_OK;
#else
    (void)handle;
    return RTOS_ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief 触发DMA事件
 */
static void rtos_dma_trigger_event(rtos_dma_handle_t *handle, rtos_dma_event_t event)
{
    if (!handle || event >= RTOS_DMA_EVENT_MAX) {
        return;
    }
    
    if (handle->event_callbacks[event]) {
        handle->event_callbacks[event](handle->controller, handle->stream, event, 
                                      handle->event_contexts[event]);
    }
}

/**
 * @brief 注册DMA事件回调
 */
rtos_result_t rtos_dma_manager_register_event_callback(rtos_dma_handle_t *handle,
                                                      rtos_dma_event_t event,
                                                      rtos_dma_event_callback_t callback,
                                                      void *context)
{
    RTOS_DMA_CHECK_HANDLE(handle);
    RTOS_DMA_CHECK_PARAM(event < RTOS_DMA_EVENT_MAX);
    RTOS_DMA_CHECK_PARAM(callback != NULL);
    
    handle->event_callbacks[event] = callback;
    handle->event_contexts[event] = context;
    
    RTOS_DMA_DEBUG_PRINT("DMA event callback registered: DMA%d Stream%d, event=%d", 
                         handle->controller + 1, handle->stream, event);
    
    return RTOS_OK;
}