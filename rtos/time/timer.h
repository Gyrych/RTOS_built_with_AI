/**
 * @file timer.h
 * @brief RTOS软件定时器 - 重构后的软件定时器系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_TIMER_H__
#define __RTOS_TIMER_H__

#include "../core/types.h"
#include "../core/object.h"

/* 定时器回调函数类型 */
typedef void (*rtos_timer_callback_t)(void *parameter);

/* 定时器结构体 */
typedef struct rtos_sw_timer {
    rtos_object_t          parent;             /* 继承对象基类 */
    
    /* 定时器属性 */
    rtos_timer_callback_t  callback;           /* 回调函数 */
    void                  *parameter;          /* 回调参数 */
    rtos_time_ns_t        period;              /* 周期时间 */
    rtos_time_ns_t        remaining_time;     /* 剩余时间 */
    bool                   auto_reload;        /* 自动重载 */
    bool                   is_running;         /* 是否运行 */
    
    /* 链表节点 */
    struct rtos_sw_timer  *next;               /* 下一个定时器 */
    struct rtos_sw_timer  *prev;               /* 上一个定时器 */
    
    /* 统计信息 */
    uint32_t               trigger_count;      /* 触发次数 */
    rtos_time_ns_t         last_trigger_time;  /* 上次触发时间 */
    rtos_time_ns_t         total_runtime;      /* 总运行时间 */
} rtos_sw_timer_t;

/* 定时器创建参数 */
typedef struct {
    const char            *name;               /* 定时器名称 */
    rtos_timer_callback_t  callback;           /* 回调函数 */
    void                  *parameter;          /* 回调参数 */
    rtos_time_ns_t        period;              /* 周期时间 */
    bool                   auto_reload;        /* 自动重载 */
} rtos_sw_timer_create_params_t;

/* 定时器信息 */
typedef struct {
    char                   name[16];           /* 定时器名称 */
    bool                   is_running;         /* 是否运行 */
    rtos_time_ns_t        period;              /* 周期时间 */
    rtos_time_ns_t        remaining_time;     /* 剩余时间 */
    bool                   auto_reload;        /* 自动重载 */
    uint32_t               trigger_count;      /* 触发次数 */
    rtos_time_ns_t         last_trigger_time;  /* 上次触发时间 */
} rtos_sw_timer_info_t;

/* 软件定时器API函数声明 */

/**
 * @brief 初始化软件定时器 - 静态方式
 * @param timer 定时器指针
 * @param params 创建参数
 * @return 操作结果
 */
rtos_result_t rtos_sw_timer_init(rtos_sw_timer_t *timer,
                                 const rtos_sw_timer_create_params_t *params);

/**
 * @brief 创建软件定时器 - 动态方式
 * @param params 创建参数
 * @return 定时器指针，失败返回NULL
 */
rtos_sw_timer_t *rtos_sw_timer_create(const rtos_sw_timer_create_params_t *params);

/**
 * @brief 删除软件定时器
 * @param timer 定时器指针
 * @return 操作结果
 */
rtos_result_t rtos_sw_timer_delete(rtos_sw_timer_t *timer);

/**
 * @brief 启动软件定时器
 * @param timer 定时器指针
 * @return 操作结果
 */
rtos_result_t rtos_sw_timer_start(rtos_sw_timer_t *timer);

/**
 * @brief 停止软件定时器
 * @param timer 定时器指针
 * @return 操作结果
 */
rtos_result_t rtos_sw_timer_stop(rtos_sw_timer_t *timer);

/**
 * @brief 重置软件定时器
 * @param timer 定时器指针
 * @return 操作结果
 */
rtos_result_t rtos_sw_timer_reset(rtos_sw_timer_t *timer);

/**
 * @brief 设置软件定时器周期
 * @param timer 定时器指针
 * @param period 新周期时间
 * @return 操作结果
 */
rtos_result_t rtos_sw_timer_set_period(rtos_sw_timer_t *timer, rtos_time_ns_t period);

/**
 * @brief 获取软件定时器周期
 * @param timer 定时器指针
 * @return 周期时间
 */
rtos_time_ns_t rtos_sw_timer_get_period(const rtos_sw_timer_t *timer);

/**
 * @brief 设置软件定时器回调函数
 * @param timer 定时器指针
 * @param callback 回调函数
 * @param parameter 回调参数
 * @return 操作结果
 */
rtos_result_t rtos_sw_timer_set_callback(rtos_sw_timer_t *timer,
                                         rtos_timer_callback_t callback,
                                         void *parameter);

/**
 * @brief 获取软件定时器信息
 * @param timer 定时器指针
 * @param info 信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_sw_timer_get_info(const rtos_sw_timer_t *timer,
                                     rtos_sw_timer_info_t *info);

/**
 * @brief 检查软件定时器是否运行
 * @param timer 定时器指针
 * @return 是否运行
 */
bool rtos_sw_timer_is_running(const rtos_sw_timer_t *timer);

/**
 * @brief 获取软件定时器剩余时间
 * @param timer 定时器指针
 * @return 剩余时间
 */
rtos_time_ns_t rtos_sw_timer_get_remaining_time(const rtos_sw_timer_t *timer);

/**
 * @brief 获取软件定时器触发次数
 * @param timer 定时器指针
 * @return 触发次数
 */
uint32_t rtos_sw_timer_get_trigger_count(const rtos_sw_timer_t *timer);

/* 时间单位转换宏 */
#define RTOS_TIMER_MS_TO_NS(ms)    ((rtos_time_ns_t)(ms) * 1000000ULL)
#define RTOS_TIMER_US_TO_NS(us)    ((rtos_time_ns_t)(us) * 1000ULL)
#define RTOS_TIMER_NS_TO_MS(ns)    ((uint32_t)((ns) / 1000000ULL))
#define RTOS_TIMER_NS_TO_US(ns)    ((uint32_t)((ns) / 1000ULL))

/* 常用时间周期定义 */
#define RTOS_TIMER_PERIOD_1MS       RTOS_TIMER_MS_TO_NS(1)
#define RTOS_TIMER_PERIOD_10MS      RTOS_TIMER_MS_TO_NS(10)
#define RTOS_TIMER_PERIOD_100MS     RTOS_TIMER_MS_TO_NS(100)
#define RTOS_TIMER_PERIOD_1S        RTOS_TIMER_MS_TO_NS(1000)

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_sw_timer_t rtos_sw_timer_t

#endif /* __RTOS_TIMER_H__ */
