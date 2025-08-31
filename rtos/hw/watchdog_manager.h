/**
 * @file watchdog_manager.h
 * @brief RTOS看门狗管理模块 - 面向对象的看门狗抽象
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_WATCHDOG_MANAGER_H__
#define __RTOS_WATCHDOG_MANAGER_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 看门狗类型定义 */
typedef enum {
    RTOS_WATCHDOG_TYPE_HARDWARE = 0,    /**< 硬件看门狗 */
    RTOS_WATCHDOG_TYPE_SOFTWARE,        /**< 软件看门狗 */
    RTOS_WATCHDOG_TYPE_MAX
} rtos_watchdog_type_t;

/* 看门狗状态定义 */
typedef enum {
    RTOS_WATCHDOG_STATE_STOPPED = 0,    /**< 停止状态 */
    RTOS_WATCHDOG_STATE_RUNNING,        /**< 运行状态 */
    RTOS_WATCHDOG_STATE_EXPIRED,        /**< 超时状态 */
    RTOS_WATCHDOG_STATE_ERROR           /**< 错误状态 */
} rtos_watchdog_state_t;

/* 硬件看门狗配置 */
typedef struct {
    uint32_t timeout_ms;            /**< 超时时间 (ms) */
    bool auto_reload;               /**< 自动重载 */
    bool reset_on_timeout;          /**< 超时时复位系统 */
    uint32_t prescaler;             /**< 预分频器 */
    uint32_t window_min_ms;         /**< 窗口看门狗最小时间 */
    uint32_t window_max_ms;         /**< 窗口看门狗最大时间 */
} rtos_watchdog_config_t;

/* 软件看门狗任务配置 */
typedef struct {
    uint32_t task_id;               /**< 任务ID */
    uint32_t timeout_ms;            /**< 超时时间 (ms) */
    uint32_t last_feed_time;        /**< 最后喂狗时间 */
    bool enabled;                   /**< 是否启用 */
    uint32_t timeout_count;         /**< 超时次数 */
    const char *task_name;          /**< 任务名称 */
} rtos_soft_watchdog_task_t;

/* 看门狗事件类型 */
typedef enum {
    RTOS_WATCHDOG_EVENT_TIMEOUT = 0,    /**< 看门狗超时 */
    RTOS_WATCHDOG_EVENT_FEED,           /**< 看门狗喂狗 */
    RTOS_WATCHDOG_EVENT_RESET,          /**< 看门狗复位 */
    RTOS_WATCHDOG_EVENT_ERROR,          /**< 看门狗错误 */
    RTOS_WATCHDOG_EVENT_MAX
} rtos_watchdog_event_t;

/* 看门狗事件回调函数类型 */
typedef void (*rtos_watchdog_event_callback_t)(rtos_watchdog_event_t event, 
                                               uint32_t watchdog_id, 
                                               void *context);

/* 看门狗统计信息 */
typedef struct {
    uint32_t feed_count;            /**< 喂狗次数 */
    uint32_t timeout_count;         /**< 超时次数 */
    uint32_t reset_count;           /**< 复位次数 */
    uint32_t error_count;           /**< 错误次数 */
    uint32_t last_feed_time;        /**< 最后喂狗时间 */
    uint32_t max_feed_interval;     /**< 最大喂狗间隔 */
    uint32_t min_feed_interval;     /**< 最小喂狗间隔 */
    uint32_t avg_feed_interval;     /**< 平均喂狗间隔 */
} rtos_watchdog_stats_t;

/* 看门狗管理器类结构 */
typedef struct {
    /* 硬件看门狗 */
    rtos_watchdog_config_t hw_config;
    rtos_watchdog_state_t hw_state;
    rtos_watchdog_stats_t hw_stats;
    
    /* 软件看门狗 */
    rtos_soft_watchdog_task_t *soft_tasks;
    uint32_t max_soft_tasks;
    uint32_t soft_task_count;
    bool soft_watchdog_enabled;
    
    /* 事件回调 */
    rtos_watchdog_event_callback_t event_callbacks[RTOS_WATCHDOG_EVENT_MAX];
    void *event_contexts[RTOS_WATCHDOG_EVENT_MAX];
    
    /* 全局统计 */
    uint32_t global_feed_count;
    uint32_t global_timeout_count;
    uint32_t monitor_cycles;
    
    /* 状态标志 */
    bool initialized;
    bool hw_watchdog_enabled;
    
} rtos_watchdog_manager_t;

/**
 * @brief 初始化看门狗管理器
 * @param max_soft_tasks 最大软件看门狗任务数
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_init(uint32_t max_soft_tasks);

/**
 * @brief 反初始化看门狗管理器
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_deinit(void);

/**
 * @brief 获取看门狗管理器实例
 * @return 看门狗管理器指针
 */
rtos_watchdog_manager_t* rtos_watchdog_manager_get_instance(void);

/* 硬件看门狗接口 */

/**
 * @brief 初始化硬件看门狗
 * @param config 看门狗配置
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_hw_init(const rtos_watchdog_config_t *config);

/**
 * @brief 启动硬件看门狗
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_hw_start(void);

/**
 * @brief 停止硬件看门狗
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_hw_stop(void);

/**
 * @brief 喂硬件看门狗
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_hw_feed(void);

/**
 * @brief 获取硬件看门狗剩余时间
 * @return 剩余时间 (ms)
 */
uint32_t rtos_watchdog_manager_hw_get_remaining_time(void);

/**
 * @brief 获取硬件看门狗状态
 * @return 看门狗状态
 */
rtos_watchdog_state_t rtos_watchdog_manager_hw_get_state(void);

/* 软件看门狗接口 */

/**
 * @brief 启用软件看门狗
 * @param enable 是否启用
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_soft_enable(bool enable);

/**
 * @brief 注册软件看门狗任务
 * @param task_id 任务ID
 * @param timeout_ms 超时时间 (ms)
 * @param task_name 任务名称
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_soft_register_task(uint32_t task_id, 
                                                      uint32_t timeout_ms, 
                                                      const char *task_name);

/**
 * @brief 注销软件看门狗任务
 * @param task_id 任务ID
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_soft_unregister_task(uint32_t task_id);

/**
 * @brief 喂软件看门狗
 * @param task_id 任务ID
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_soft_feed_task(uint32_t task_id);

/**
 * @brief 检查所有软件看门狗任务
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_soft_check_all_tasks(void);

/**
 * @brief 获取软件看门狗任务状态
 * @param task_id 任务ID
 * @param task_info 任务信息指针
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_soft_get_task_info(uint32_t task_id, 
                                                      rtos_soft_watchdog_task_t *task_info);

/* 事件管理接口 */

/**
 * @brief 注册看门狗事件回调
 * @param event 事件类型
 * @param callback 回调函数
 * @param context 用户上下文
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_register_event_callback(rtos_watchdog_event_t event,
                                                           rtos_watchdog_event_callback_t callback,
                                                           void *context);

/**
 * @brief 注销看门狗事件回调
 * @param event 事件类型
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_unregister_event_callback(rtos_watchdog_event_t event);

/* 统计和监控接口 */

/**
 * @brief 获取硬件看门狗统计
 * @param stats 统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_get_hw_stats(rtos_watchdog_stats_t *stats);

/**
 * @brief 获取软件看门狗统计
 * @param task_id 任务ID
 * @param stats 统计结构指针
 * @return 操作结果
 */
rtos_result_t rtos_watchdog_manager_get_soft_stats(uint32_t task_id, rtos_watchdog_stats_t *stats);

/**
 * @brief 看门狗管理器周期性任务
 * 应该被定时调用以检查软件看门狗状态
 */
void rtos_watchdog_manager_periodic_task(void);

/**
 * @brief 看门狗中断处理函数
 * 由硬件看门狗中断调用
 */
void rtos_watchdog_manager_interrupt_handler(void);

/**
 * @brief 获取看门狗管理器统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_watchdog_manager_get_statistics(char *buffer, uint32_t size);

/**
 * @brief 生成看门狗状态报告
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_watchdog_manager_generate_report(char *buffer, uint32_t size);

/* 便利宏定义 */
#define RTOS_WATCHDOG_HW_FEED() \
    rtos_watchdog_manager_hw_feed()

#define RTOS_WATCHDOG_SOFT_FEED(task_id) \
    rtos_watchdog_manager_soft_feed_task(task_id)

#define RTOS_WATCHDOG_REGISTER_TASK(task_id, timeout, name) \
    rtos_watchdog_manager_soft_register_task((task_id), (timeout), (name))

/* 看门狗超时处理宏 */
#define RTOS_WATCHDOG_TIMEOUT_ACTION_RESET      0
#define RTOS_WATCHDOG_TIMEOUT_ACTION_INTERRUPT  1
#define RTOS_WATCHDOG_TIMEOUT_ACTION_CALLBACK   2

/* 调试宏定义 */
#ifdef RTOS_WATCHDOG_DEBUG
#define RTOS_WATCHDOG_DEBUG_PRINT(fmt, ...) \
    printf("[WATCHDOG] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_WATCHDOG_DEBUG_PRINT(fmt, ...)
#endif

/* 错误检查宏定义 */
#ifdef RTOS_WATCHDOG_ERROR_CHECK
#define RTOS_WATCHDOG_CHECK_PARAM(param) \
    do { \
        if (!(param)) { \
            RTOS_WATCHDOG_DEBUG_PRINT("Parameter check failed: %s", #param); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
    
#define RTOS_WATCHDOG_CHECK_INIT() \
    do { \
        if (!rtos_watchdog_manager_get_instance()) { \
            RTOS_WATCHDOG_DEBUG_PRINT("Watchdog manager not initialized"); \
            return RTOS_ERROR_NOT_INITIALIZED; \
        } \
    } while(0)
#else
#define RTOS_WATCHDOG_CHECK_PARAM(param)
#define RTOS_WATCHDOG_CHECK_INIT()
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_WATCHDOG_MANAGER_H__ */