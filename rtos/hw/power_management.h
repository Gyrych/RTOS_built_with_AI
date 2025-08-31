/**
 * @file power_management.h
 * @brief RTOS电源管理模块 - 面向对象的电源管理抽象
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_POWER_MANAGEMENT_H__
#define __RTOS_POWER_MANAGEMENT_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 电源模式定义 */
typedef enum {
    RTOS_POWER_MODE_RUN = 0,        /**< 正常运行模式 */
    RTOS_POWER_MODE_SLEEP,          /**< 睡眠模式 */
    RTOS_POWER_MODE_STOP,           /**< 停止模式 */
    RTOS_POWER_MODE_STANDBY,        /**< 待机模式 */
    RTOS_POWER_MODE_MAX
} rtos_power_mode_t;

/* 唤醒源定义 */
typedef enum {
    RTOS_WAKEUP_SOURCE_NONE = 0,
    RTOS_WAKEUP_SOURCE_RTC = (1 << 0),
    RTOS_WAKEUP_SOURCE_WKUP_PIN = (1 << 1),
    RTOS_WAKEUP_SOURCE_TAMPER = (1 << 2),
    RTOS_WAKEUP_SOURCE_RESET = (1 << 3),
    RTOS_WAKEUP_SOURCE_IWDG = (1 << 4),
    RTOS_WAKEUP_SOURCE_WWDG = (1 << 5),
    RTOS_WAKEUP_SOURCE_LPWR = (1 << 6),
    RTOS_WAKEUP_SOURCE_ALL = 0xFFFFFFFF
} rtos_wakeup_source_t;

/* 电源状态结构 */
typedef struct {
    uint32_t vdd_voltage_mv;        /**< VDD电压 (mV) */
    uint32_t vbat_voltage_mv;       /**< VBAT电压 (mV) */
    int16_t temperature_celsius;    /**< 温度 (摄氏度) */
    rtos_power_mode_t current_mode; /**< 当前电源模式 */
    uint32_t wakeup_count;          /**< 唤醒计数 */
    uint32_t wakeup_source;         /**< 唤醒源 */
    uint32_t sleep_time_ms;         /**< 睡眠时间 (ms) */
    uint32_t run_time_ms;           /**< 运行时间 (ms) */
} rtos_power_status_t;

/* 电源策略配置 */
typedef struct {
    bool auto_sleep_enable;         /**< 自动睡眠使能 */
    uint32_t idle_timeout_ms;       /**< 空闲超时时间 (ms) */
    uint32_t deep_sleep_threshold_ms; /**< 深度睡眠阈值 (ms) */
    rtos_power_mode_t max_sleep_mode; /**< 最大睡眠模式 */
    uint32_t wakeup_sources;        /**< 允许的唤醒源 */
    bool voltage_scaling_enable;    /**< 电压调节使能 */
    uint32_t min_voltage_mv;        /**< 最小工作电压 (mV) */
} rtos_power_policy_t;

/* 电源事件回调类型 */
typedef enum {
    RTOS_POWER_EVENT_MODE_CHANGED = 0,
    RTOS_POWER_EVENT_VOLTAGE_LOW,
    RTOS_POWER_EVENT_TEMPERATURE_HIGH,
    RTOS_POWER_EVENT_WAKEUP,
    RTOS_POWER_EVENT_MAX
} rtos_power_event_t;

/* 电源事件回调函数类型 */
typedef void (*rtos_power_event_callback_t)(rtos_power_event_t event, void *context);

/* 电源管理器类结构 */
typedef struct {
    /* 私有成员 */
    rtos_power_mode_t current_mode;
    rtos_power_policy_t policy;
    rtos_power_status_t status;
    rtos_power_event_callback_t event_callbacks[RTOS_POWER_EVENT_MAX];
    void *event_contexts[RTOS_POWER_EVENT_MAX];
    
    /* 统计信息 */
    uint32_t mode_switch_count[RTOS_POWER_MODE_MAX];
    uint32_t total_sleep_time_ms;
    uint32_t total_run_time_ms;
    uint32_t power_events_count[RTOS_POWER_EVENT_MAX];
    
    /* 平台相关数据 */
    void *platform_data;
} rtos_power_manager_t;

/**
 * @brief 初始化电源管理器
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_init(void);

/**
 * @brief 反初始化电源管理器
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_deinit(void);

/**
 * @brief 获取电源管理器实例
 * @return 电源管理器指针
 */
rtos_power_manager_t* rtos_power_manager_get_instance(void);

/**
 * @brief 设置电源模式
 * @param mode 电源模式
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_set_mode(rtos_power_mode_t mode);

/**
 * @brief 获取当前电源模式
 * @return 当前电源模式
 */
rtos_power_mode_t rtos_power_manager_get_current_mode(void);

/**
 * @brief 进入低功耗模式
 * @param mode 目标低功耗模式
 * @param timeout_ms 超时时间 (0表示无超时)
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_enter_low_power(rtos_power_mode_t mode, uint32_t timeout_ms);

/**
 * @brief 退出低功耗模式
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_exit_low_power(void);

/**
 * @brief 配置唤醒源
 * @param sources 唤醒源掩码
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_configure_wakeup_sources(uint32_t sources);

/**
 * @brief 获取唤醒源
 * @return 唤醒源掩码
 */
uint32_t rtos_power_manager_get_wakeup_sources(void);

/**
 * @brief 获取电源状态
 * @param status 状态结构指针
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_get_status(rtos_power_status_t *status);

/**
 * @brief 设置电源策略
 * @param policy 电源策略指针
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_set_policy(const rtos_power_policy_t *policy);

/**
 * @brief 获取电源策略
 * @param policy 电源策略指针
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_get_policy(rtos_power_policy_t *policy);

/**
 * @brief 注册电源事件回调
 * @param event 事件类型
 * @param callback 回调函数
 * @param context 用户上下文
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_register_event_callback(rtos_power_event_t event, 
                                                        rtos_power_event_callback_t callback, 
                                                        void *context);

/**
 * @brief 注销电源事件回调
 * @param event 事件类型
 * @return 操作结果
 */
rtos_result_t rtos_power_manager_unregister_event_callback(rtos_power_event_t event);

/**
 * @brief 电源管理器周期性任务
 * 应该被定时调用以处理电源管理逻辑
 */
void rtos_power_manager_periodic_task(void);

/**
 * @brief 电源中断处理函数
 * 由硬件中断调用
 */
void rtos_power_manager_interrupt_handler(void);

/**
 * @brief 获取电源统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_power_manager_get_statistics(char *buffer, uint32_t size);

/* 便利宏定义 */
#define RTOS_POWER_ENTER_SLEEP(timeout) \
    rtos_power_manager_enter_low_power(RTOS_POWER_MODE_SLEEP, (timeout))

#define RTOS_POWER_ENTER_STOP(timeout) \
    rtos_power_manager_enter_low_power(RTOS_POWER_MODE_STOP, (timeout))

#define RTOS_POWER_ENTER_STANDBY(timeout) \
    rtos_power_manager_enter_low_power(RTOS_POWER_MODE_STANDBY, (timeout))

#define RTOS_POWER_IS_LOW_POWER_MODE(mode) \
    ((mode) != RTOS_POWER_MODE_RUN)

#define RTOS_POWER_IS_DEEP_SLEEP_MODE(mode) \
    ((mode) == RTOS_POWER_MODE_STOP || (mode) == RTOS_POWER_MODE_STANDBY)

/* 调试宏定义 */
#ifdef RTOS_POWER_DEBUG
#define RTOS_POWER_DEBUG_PRINT(fmt, ...) \
    printf("[POWER] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_POWER_DEBUG_PRINT(fmt, ...)
#endif

/* 错误检查宏定义 */
#ifdef RTOS_POWER_ERROR_CHECK
#define RTOS_POWER_CHECK_PARAM(param) \
    do { \
        if (!(param)) { \
            RTOS_POWER_DEBUG_PRINT("Parameter check failed: %s", #param); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
#else
#define RTOS_POWER_CHECK_PARAM(param)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_POWER_MANAGEMENT_H__ */