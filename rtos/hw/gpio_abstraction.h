/**
 * @file gpio_abstraction.h
 * @brief RTOS GPIO抽象模块 - 面向对象的GPIO管理
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_GPIO_ABSTRACTION_H__
#define __RTOS_GPIO_ABSTRACTION_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GPIO端口定义 */
typedef enum {
    RTOS_GPIO_PORT_A = 0,
    RTOS_GPIO_PORT_B,
    RTOS_GPIO_PORT_C,
    RTOS_GPIO_PORT_D,
    RTOS_GPIO_PORT_E,
    RTOS_GPIO_PORT_F,
    RTOS_GPIO_PORT_G,
    RTOS_GPIO_PORT_H,
    RTOS_GPIO_PORT_I,
    RTOS_GPIO_PORT_MAX
} rtos_gpio_port_t;

/* GPIO引脚定义 */
typedef enum {
    RTOS_GPIO_PIN_0 = 0,
    RTOS_GPIO_PIN_1,
    RTOS_GPIO_PIN_2,
    RTOS_GPIO_PIN_3,
    RTOS_GPIO_PIN_4,
    RTOS_GPIO_PIN_5,
    RTOS_GPIO_PIN_6,
    RTOS_GPIO_PIN_7,
    RTOS_GPIO_PIN_8,
    RTOS_GPIO_PIN_9,
    RTOS_GPIO_PIN_10,
    RTOS_GPIO_PIN_11,
    RTOS_GPIO_PIN_12,
    RTOS_GPIO_PIN_13,
    RTOS_GPIO_PIN_14,
    RTOS_GPIO_PIN_15,
    RTOS_GPIO_PIN_MAX
} rtos_gpio_pin_t;

/* GPIO模式定义 */
typedef enum {
    RTOS_GPIO_MODE_INPUT = 0,       /**< 输入模式 */
    RTOS_GPIO_MODE_OUTPUT_PP,       /**< 推挽输出模式 */
    RTOS_GPIO_MODE_OUTPUT_OD,       /**< 开漏输出模式 */
    RTOS_GPIO_MODE_ALTERNATE_PP,    /**< 复用推挽模式 */
    RTOS_GPIO_MODE_ALTERNATE_OD,    /**< 复用开漏模式 */
    RTOS_GPIO_MODE_ANALOG,          /**< 模拟模式 */
    RTOS_GPIO_MODE_MAX
} rtos_gpio_mode_t;

/* GPIO上拉下拉定义 */
typedef enum {
    RTOS_GPIO_PULL_NONE = 0,        /**< 无上拉下拉 */
    RTOS_GPIO_PULL_UP,              /**< 上拉 */
    RTOS_GPIO_PULL_DOWN,            /**< 下拉 */
    RTOS_GPIO_PULL_MAX
} rtos_gpio_pull_t;

/* GPIO速度定义 */
typedef enum {
    RTOS_GPIO_SPEED_LOW = 0,        /**< 低速 */
    RTOS_GPIO_SPEED_MEDIUM,         /**< 中速 */
    RTOS_GPIO_SPEED_HIGH,           /**< 高速 */
    RTOS_GPIO_SPEED_VERY_HIGH,      /**< 极高速 */
    RTOS_GPIO_SPEED_MAX
} rtos_gpio_speed_t;

/* GPIO中断触发类型 */
typedef enum {
    RTOS_GPIO_TRIGGER_NONE = 0,     /**< 无中断 */
    RTOS_GPIO_TRIGGER_RISING,       /**< 上升沿触发 */
    RTOS_GPIO_TRIGGER_FALLING,      /**< 下降沿触发 */
    RTOS_GPIO_TRIGGER_BOTH,         /**< 双边沿触发 */
    RTOS_GPIO_TRIGGER_MAX
} rtos_gpio_trigger_t;

/* GPIO配置结构 */
typedef struct {
    rtos_gpio_port_t port;          /**< GPIO端口 */
    rtos_gpio_pin_t pin;            /**< GPIO引脚 */
    rtos_gpio_mode_t mode;          /**< GPIO模式 */
    rtos_gpio_pull_t pull;          /**< 上拉下拉 */
    rtos_gpio_speed_t speed;        /**< 输出速度 */
    uint32_t alternate_function;    /**< 复用功能 */
    rtos_gpio_trigger_t trigger;    /**< 中断触发类型 */
    bool initial_value;             /**< 初始输出值 */
} rtos_gpio_config_t;

/* GPIO中断回调函数类型 */
typedef void (*rtos_gpio_interrupt_callback_t)(rtos_gpio_port_t port, 
                                               rtos_gpio_pin_t pin, 
                                               void *context);

/* GPIO引脚句柄 */
typedef struct {
    rtos_gpio_port_t port;
    rtos_gpio_pin_t pin;
    rtos_gpio_config_t config;
    rtos_gpio_interrupt_callback_t interrupt_callback;
    void *interrupt_context;
    bool configured;
    uint32_t toggle_count;
    uint32_t last_change_time;
} rtos_gpio_handle_t;

/* GPIO管理器类结构 */
typedef struct {
    /* GPIO句柄数组 */
    rtos_gpio_handle_t *gpio_handles;
    uint32_t max_gpio_count;
    uint32_t configured_gpio_count;
    
    /* 中断管理 */
    rtos_gpio_interrupt_callback_t exti_callbacks[16]; /* EXTI0-15 */
    void *exti_contexts[16];
    
    /* 统计信息 */
    uint32_t config_operations;
    uint32_t read_operations;
    uint32_t write_operations;
    uint32_t interrupt_count;
    
    /* 状态标志 */
    bool initialized;
    
} rtos_gpio_manager_t;

/**
 * @brief 初始化GPIO管理器
 * @param max_gpio_count 最大GPIO数量
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_init(uint32_t max_gpio_count);

/**
 * @brief 反初始化GPIO管理器
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_deinit(void);

/**
 * @brief 获取GPIO管理器实例
 * @return GPIO管理器指针
 */
rtos_gpio_manager_t* rtos_gpio_manager_get_instance(void);

/**
 * @brief 配置GPIO引脚
 * @param config GPIO配置结构
 * @param handle 返回的GPIO句柄指针
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_config_pin(const rtos_gpio_config_t *config, 
                                          rtos_gpio_handle_t **handle);

/**
 * @brief 反配置GPIO引脚
 * @param handle GPIO句柄
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_deconfig_pin(rtos_gpio_handle_t *handle);

/**
 * @brief 写GPIO引脚
 * @param handle GPIO句柄
 * @param value 输出值
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_write_pin(rtos_gpio_handle_t *handle, bool value);

/**
 * @brief 读GPIO引脚
 * @param handle GPIO句柄
 * @param value 读取值指针
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_read_pin(rtos_gpio_handle_t *handle, bool *value);

/**
 * @brief 翻转GPIO引脚
 * @param handle GPIO句柄
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_toggle_pin(rtos_gpio_handle_t *handle);

/**
 * @brief 设置GPIO中断回调
 * @param handle GPIO句柄
 * @param callback 中断回调函数
 * @param context 用户上下文
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_set_interrupt_callback(rtos_gpio_handle_t *handle,
                                                      rtos_gpio_interrupt_callback_t callback,
                                                      void *context);

/**
 * @brief 启用GPIO中断
 * @param handle GPIO句柄
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_enable_interrupt(rtos_gpio_handle_t *handle);

/**
 * @brief 禁用GPIO中断
 * @param handle GPIO句柄
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_disable_interrupt(rtos_gpio_handle_t *handle);

/**
 * @brief 批量写GPIO端口
 * @param port GPIO端口
 * @param mask 引脚掩码
 * @param value 输出值
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_write_port(rtos_gpio_port_t port, uint16_t mask, uint16_t value);

/**
 * @brief 批量读GPIO端口
 * @param port GPIO端口
 * @param value 读取值指针
 * @return 操作结果
 */
rtos_result_t rtos_gpio_manager_read_port(rtos_gpio_port_t port, uint16_t *value);

/**
 * @brief GPIO中断处理函数
 * @param exti_line EXTI线号
 */
void rtos_gpio_manager_interrupt_handler(uint32_t exti_line);

/**
 * @brief 获取GPIO统计信息
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_gpio_manager_get_statistics(char *buffer, uint32_t size);

/**
 * @brief 生成GPIO配置报告
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际输出长度
 */
uint32_t rtos_gpio_manager_generate_config_report(char *buffer, uint32_t size);

/* 便利宏定义 */
#define RTOS_GPIO_MAKE_CONFIG(port, pin, mode) \
    { .port = (port), .pin = (pin), .mode = (mode), \
      .pull = RTOS_GPIO_PULL_NONE, .speed = RTOS_GPIO_SPEED_MEDIUM, \
      .alternate_function = 0, .trigger = RTOS_GPIO_TRIGGER_NONE, \
      .initial_value = false }

#define RTOS_GPIO_PIN_SET(handle) \
    rtos_gpio_manager_write_pin((handle), true)

#define RTOS_GPIO_PIN_RESET(handle) \
    rtos_gpio_manager_write_pin((handle), false)

#define RTOS_GPIO_PIN_TOGGLE(handle) \
    rtos_gpio_manager_toggle_pin(handle)

#define RTOS_GPIO_PIN_READ(handle, value) \
    rtos_gpio_manager_read_pin((handle), (value))

/* GPIO端口和引脚转换宏 */
#define RTOS_GPIO_PORT_TO_INDEX(port) ((uint32_t)(port))
#define RTOS_GPIO_PIN_TO_MASK(pin) (1U << (uint32_t)(pin))
#define RTOS_GPIO_MAKE_PIN_ID(port, pin) (((uint32_t)(port) << 16) | (uint32_t)(pin))
#define RTOS_GPIO_GET_PORT_FROM_ID(id) ((rtos_gpio_port_t)((id) >> 16))
#define RTOS_GPIO_GET_PIN_FROM_ID(id) ((rtos_gpio_pin_t)((id) & 0xFFFF))

/* 调试宏定义 */
#ifdef RTOS_GPIO_DEBUG
#define RTOS_GPIO_DEBUG_PRINT(fmt, ...) \
    printf("[GPIO] " fmt "\r\n", ##__VA_ARGS__)
#else
#define RTOS_GPIO_DEBUG_PRINT(fmt, ...)
#endif

/* 错误检查宏定义 */
#ifdef RTOS_GPIO_ERROR_CHECK
#define RTOS_GPIO_CHECK_PARAM(param) \
    do { \
        if (!(param)) { \
            RTOS_GPIO_DEBUG_PRINT("Parameter check failed: %s", #param); \
            return RTOS_ERROR_INVALID_PARAM; \
        } \
    } while(0)
    
#define RTOS_GPIO_CHECK_INIT() \
    do { \
        if (!rtos_gpio_manager_get_instance()) { \
            RTOS_GPIO_DEBUG_PRINT("GPIO manager not initialized"); \
            return RTOS_ERROR_NOT_INITIALIZED; \
        } \
    } while(0)
    
#define RTOS_GPIO_CHECK_HANDLE(handle) \
    do { \
        if (!(handle) || !(handle)->configured) { \
            RTOS_GPIO_DEBUG_PRINT("Invalid GPIO handle"); \
            return RTOS_ERROR_INVALID_HANDLE; \
        } \
    } while(0)
#else
#define RTOS_GPIO_CHECK_PARAM(param)
#define RTOS_GPIO_CHECK_INIT()
#define RTOS_GPIO_CHECK_HANDLE(handle)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_GPIO_ABSTRACTION_H__ */