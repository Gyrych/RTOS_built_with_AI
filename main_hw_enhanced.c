/**
 * @file main_hw_enhanced.c
 * @brief RTOS硬件抽象层增强版主程序
 * 展示所有新增硬件抽象模块的使用方法
 * @author Assistant
 * @date 2024
 */

#include "rtos.h"
#include "rtos/hw/hw_abstraction.h"
#include "rtos/hw/power_management.h"
#include "rtos/hw/memory_monitor.h"
#include "rtos/hw/watchdog_manager.h"
#include "rtos/hw/gpio_abstraction.h"
#include "rtos/hw/uart_abstraction.h"
#include "rtos/hw/hw_comprehensive_test.h"
#include <stdio.h>
#include <string.h>

/* 应用任务定义 */
#define LED_TASK_ID         1
#define MONITOR_TASK_ID     2
#define COMM_TASK_ID        3

/* GPIO句柄 */
static rtos_gpio_handle_t *g_led_gpio = NULL;
static rtos_gpio_handle_t *g_button_gpio = NULL;

/* 应用状态 */
static bool g_system_running = true;
static uint32_t g_led_toggle_count = 0;
static uint32_t g_button_press_count = 0;

/* 函数声明 */
static void app_init_hardware(void);
static void app_init_tasks(void);
static void app_led_task(void);
static void app_monitor_task(void);
static void app_communication_task(void);
static void app_button_interrupt_callback(rtos_gpio_port_t port, rtos_gpio_pin_t pin, void *context);
static void app_power_event_callback(rtos_power_event_t event, void *context);
static void app_watchdog_event_callback(rtos_watchdog_event_t event, uint32_t watchdog_id, void *context);
static void app_uart_event_callback(rtos_uart_port_t port, rtos_uart_event_t event, void *context);

/**
 * @brief 主函数
 */
int main(void)
{
    printf("\n");
    printf("=====================================\n");
    printf("RTOS硬件抽象层增强版演示程序\n");
    printf("STM32F407 + 完整硬件抽象层\n");
    printf("=====================================\n");
    
    /* 初始化RTOS系统 */
    printf("初始化RTOS系统...");
    if (rtos_init() != RTOS_OK) {
        printf("失败！\n");
        while (1);
    }
    printf("完成\n");
    
    /* 初始化硬件 */
    printf("初始化硬件模块...");
    app_init_hardware();
    printf("完成\n");
    
    /* 初始化应用任务 */
    printf("初始化应用任务...");
    app_init_tasks();
    printf("完成\n");
    
    /* 运行综合测试 */
    printf("运行硬件抽象层综合测试...\n");
    rtos_result_t test_result = rtos_hw_run_comprehensive_tests();
    
    if (test_result == RTOS_OK) {
        printf("✓ 所有测试通过！\n");
    } else {
        printf("✗ 部分测试失败\n");
    }
    
    /* 生成测试报告 */
    char test_report[2048];
    uint32_t report_len = rtos_hw_generate_test_report(test_report, sizeof(test_report));
    if (report_len > 0) {
        printf("\n%s\n", test_report);
    }
    
    printf("\n开始运行应用程序...\n");
    printf("=====================================\n");
    
    /* 启动RTOS调度器 */
    rtos_start();
    
    /* 主循环 */
    while (g_system_running) {
        /* 执行后台任务 */
        app_led_task();
        app_monitor_task();
        app_communication_task();
        
        /* 系统空闲处理 */
        rtos_hw_delay_ms(100);
    }
    
    printf("系统关闭\n");
    return 0;
}

/**
 * @brief 初始化硬件
 */
static void app_init_hardware(void)
{
    /* 配置LED GPIO (PA0) */
    rtos_gpio_config_t led_config = RTOS_GPIO_MAKE_CONFIG(
        RTOS_GPIO_PORT_A, RTOS_GPIO_PIN_0, RTOS_GPIO_MODE_OUTPUT_PP);
    led_config.initial_value = false;
    led_config.speed = RTOS_GPIO_SPEED_HIGH;
    
    if (rtos_gpio_manager_config_pin(&led_config, &g_led_gpio) == RTOS_OK) {
        printf("  LED GPIO配置完成 (PA0)\n");
    }
    
    /* 配置按键GPIO (PA1) */
    rtos_gpio_config_t button_config = RTOS_GPIO_MAKE_CONFIG(
        RTOS_GPIO_PORT_A, RTOS_GPIO_PIN_1, RTOS_GPIO_MODE_INPUT);
    button_config.pull = RTOS_GPIO_PULL_UP;
    button_config.trigger = RTOS_GPIO_TRIGGER_FALLING;
    
    if (rtos_gpio_manager_config_pin(&button_config, &g_button_gpio) == RTOS_OK) {
        /* 设置按键中断回调 */
        rtos_gpio_manager_set_interrupt_callback(g_button_gpio, app_button_interrupt_callback, NULL);
        rtos_gpio_manager_enable_interrupt(g_button_gpio);
        printf("  按键GPIO配置完成 (PA1)\n");
    }
    
    /* 初始化UART1 */
    rtos_uart_config_t uart_config = RTOS_UART_DEFAULT_CONFIG(115200);
    if (rtos_uart_manager_init_port(RTOS_UART_PORT_1, &uart_config, NULL) == RTOS_OK) {
        /* 注册UART事件回调 */
        rtos_uart_manager_register_event_callback(RTOS_UART_PORT_1, 
                                                  RTOS_UART_EVENT_TX_COMPLETE,
                                                  app_uart_event_callback, NULL);
        printf("  UART1配置完成 (115200 bps)\n");
    }
    
    /* 配置电源管理策略 */
    rtos_power_policy_t power_policy = {
        .auto_sleep_enable = true,
        .idle_timeout_ms = 5000,
        .deep_sleep_threshold_ms = 10000,
        .max_sleep_mode = RTOS_POWER_MODE_SLEEP,
        .wakeup_sources = RTOS_WAKEUP_SOURCE_RTC | RTOS_WAKEUP_SOURCE_WKUP_PIN,
        .voltage_scaling_enable = true,
        .min_voltage_mv = 2700
    };
    
    if (rtos_power_manager_set_policy(&power_policy) == RTOS_OK) {
        /* 注册电源事件回调 */
        rtos_power_manager_register_event_callback(RTOS_POWER_EVENT_VOLTAGE_LOW,
                                                   app_power_event_callback, NULL);
        rtos_power_manager_register_event_callback(RTOS_POWER_EVENT_TEMPERATURE_HIGH,
                                                   app_power_event_callback, NULL);
        printf("  电源管理策略配置完成\n");
    }
    
    /* 配置硬件看门狗 */
    rtos_watchdog_config_t wdt_config = {
        .timeout_ms = 2000,
        .auto_reload = true,
        .reset_on_timeout = false, /* 演示模式不复位 */
        .prescaler = 4
    };
    
    if (rtos_watchdog_manager_hw_init(&wdt_config) == RTOS_OK) {
        /* 注册看门狗事件回调 */
        rtos_watchdog_manager_register_event_callback(RTOS_WATCHDOG_EVENT_TIMEOUT,
                                                      app_watchdog_event_callback, NULL);
        printf("  硬件看门狗配置完成 (2s超时)\n");
    }
    
    /* 启用内存泄漏检测 */
    if (rtos_memory_monitor_enable_leak_detection(true) == RTOS_OK) {
        printf("  内存泄漏检测启用\n");
    }
}

/**
 * @brief 初始化应用任务
 */
static void app_init_tasks(void)
{
    /* 注册软件看门狗任务 */
    rtos_watchdog_manager_soft_register_task(LED_TASK_ID, 1000, "LED_Task");
    rtos_watchdog_manager_soft_register_task(MONITOR_TASK_ID, 2000, "Monitor_Task");
    rtos_watchdog_manager_soft_register_task(COMM_TASK_ID, 3000, "Communication_Task");
    
    printf("  软件看门狗任务注册完成\n");
    
    /* 启动硬件看门狗 */
    if (rtos_watchdog_manager_hw_start() == RTOS_OK) {
        printf("  硬件看门狗启动完成\n");
    }
}

/**
 * @brief LED任务
 */
static void app_led_task(void)
{
    static uint32_t last_toggle_time = 0;
    uint32_t current_time = rtos_hw_get_system_time_ms();
    
    /* 每500ms翻转一次LED */
    if (current_time - last_toggle_time >= 500) {
        if (g_led_gpio) {
            rtos_gpio_manager_toggle_pin(g_led_gpio);
            g_led_toggle_count++;
            
            /* 通过UART发送LED状态 */
            char led_msg[64];
            snprintf(led_msg, sizeof(led_msg), "LED Toggle #%lu\r\n", g_led_toggle_count);
            rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)led_msg, strlen(led_msg));
        }
        
        last_toggle_time = current_time;
        
        /* 喂软件看门狗 */
        rtos_watchdog_manager_soft_feed_task(LED_TASK_ID);
    }
}

/**
 * @brief 监控任务
 */
static void app_monitor_task(void)
{
    static uint32_t last_monitor_time = 0;
    uint32_t current_time = rtos_hw_get_system_time_ms();
    
    /* 每2秒执行一次监控 */
    if (current_time - last_monitor_time >= 2000) {
        /* 执行周期性任务 */
        rtos_power_manager_periodic_task();
        rtos_memory_monitor_periodic_task();
        rtos_watchdog_manager_periodic_task();
        
        /* 获取系统状态 */
        rtos_power_status_t power_status;
        rtos_memory_stats_t memory_stats;
        
        if (rtos_power_manager_get_status(&power_status) == RTOS_OK &&
            rtos_memory_monitor_get_stats(&memory_stats) == RTOS_OK) {
            
            /* 通过UART发送状态信息 */
            char status_msg[256];
            snprintf(status_msg, sizeof(status_msg),
                "Status: VDD=%lumV, Temp=%dC, RAM=%lu/%luKB, Heap=%lu/%luKB\r\n",
                power_status.vdd_voltage_mv,
                power_status.temperature_celsius,
                memory_stats.used_ram / 1024,
                memory_stats.total_ram / 1024,
                memory_stats.heap_used / 1024,
                memory_stats.heap_total / 1024);
            
            rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)status_msg, strlen(status_msg));
        }
        
        /* 喂硬件看门狗 */
        rtos_watchdog_manager_hw_feed();
        
        /* 喂软件看门狗 */
        rtos_watchdog_manager_soft_feed_task(MONITOR_TASK_ID);
        
        last_monitor_time = current_time;
    }
}

/**
 * @brief 通信任务
 */
static void app_communication_task(void)
{
    static uint32_t last_comm_time = 0;
    uint32_t current_time = rtos_hw_get_system_time_ms();
    
    /* 每3秒发送一次统计信息 */
    if (current_time - last_comm_time >= 3000) {
        /* 获取各模块统计信息 */
        char stats_buffer[1024];
        uint32_t stats_len;
        
        /* GPIO统计 */
        stats_len = rtos_gpio_manager_get_statistics(stats_buffer, sizeof(stats_buffer));
        if (stats_len > 0) {
            rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)stats_buffer, stats_len);
            rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)"\r\n", 2);
        }
        
        /* 内存统计 */
        stats_len = rtos_memory_monitor_get_statistics(stats_buffer, sizeof(stats_buffer));
        if (stats_len > 0) {
            rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)stats_buffer, stats_len);
            rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)"\r\n", 2);
        }
        
        /* 看门狗统计 */
        stats_len = rtos_watchdog_manager_get_statistics(stats_buffer, sizeof(stats_buffer));
        if (stats_len > 0) {
            rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)stats_buffer, stats_len);
            rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)"\r\n", 2);
        }
        
        /* 喂软件看门狗 */
        rtos_watchdog_manager_soft_feed_task(COMM_TASK_ID);
        
        last_comm_time = current_time;
    }
}

/**
 * @brief 按键中断回调函数
 */
static void app_button_interrupt_callback(rtos_gpio_port_t port, rtos_gpio_pin_t pin, void *context)
{
    (void)port;
    (void)pin;
    (void)context;
    
    g_button_press_count++;
    
    char button_msg[64];
    snprintf(button_msg, sizeof(button_msg), "Button Pressed #%lu\r\n", g_button_press_count);
    rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)button_msg, strlen(button_msg));
    
    /* 按键按下时进入短暂睡眠模式演示 */
    if (g_button_press_count % 5 == 0) {
        printf("演示进入睡眠模式...\n");
        rtos_power_manager_enter_low_power(RTOS_POWER_MODE_SLEEP, 1000);
        printf("从睡眠模式唤醒\n");
    }
}

/**
 * @brief 电源事件回调函数
 */
static void app_power_event_callback(rtos_power_event_t event, void *context)
{
    (void)context;
    
    switch (event) {
        case RTOS_POWER_EVENT_VOLTAGE_LOW:
            printf("警告: 电压过低！\n");
            rtos_uart_manager_send(RTOS_UART_PORT_1, 
                                  (const uint8_t*)"WARNING: Low Voltage!\r\n", 23);
            break;
            
        case RTOS_POWER_EVENT_TEMPERATURE_HIGH:
            printf("警告: 温度过高！\n");
            rtos_uart_manager_send(RTOS_UART_PORT_1, 
                                  (const uint8_t*)"WARNING: High Temperature!\r\n", 29);
            break;
            
        case RTOS_POWER_EVENT_WAKEUP:
            printf("系统从低功耗模式唤醒\n");
            break;
            
        case RTOS_POWER_EVENT_MODE_CHANGED:
            printf("电源模式改变\n");
            break;
            
        default:
            break;
    }
}

/**
 * @brief 看门狗事件回调函数
 */
static void app_watchdog_event_callback(rtos_watchdog_event_t event, uint32_t watchdog_id, void *context)
{
    (void)context;
    
    switch (event) {
        case RTOS_WATCHDOG_EVENT_TIMEOUT:
            printf("看门狗超时: ID=%lu\n", watchdog_id);
            rtos_uart_manager_send(RTOS_UART_PORT_1, 
                                  (const uint8_t*)"Watchdog Timeout!\r\n", 19);
            break;
            
        case RTOS_WATCHDOG_EVENT_FEED:
            /* 正常喂狗，不打印信息 */
            break;
            
        case RTOS_WATCHDOG_EVENT_RESET:
            printf("看门狗复位系统\n");
            break;
            
        default:
            break;
    }
}

/**
 * @brief UART事件回调函数
 */
static void app_uart_event_callback(rtos_uart_port_t port, rtos_uart_event_t event, void *context)
{
    (void)context;
    
    switch (event) {
        case RTOS_UART_EVENT_TX_COMPLETE:
            /* 发送完成，可以继续发送下一个数据包 */
            break;
            
        case RTOS_UART_EVENT_RX_COMPLETE:
            printf("UART%d 接收完成\n", port + 1);
            break;
            
        case RTOS_UART_EVENT_ERROR:
            printf("UART%d 错误\n", port + 1);
            break;
            
        case RTOS_UART_EVENT_IDLE:
            printf("UART%d 空闲\n", port + 1);
            break;
            
        default:
            break;
    }
}

/**
 * @brief 演示内存管理功能
 */
void app_demo_memory_management(void)
{
    printf("\n--- 内存管理功能演示 ---\n");
    
    /* 创建内存池 */
    rtos_memory_pool_config_t pool_config = {
        .block_size = 128,
        .block_count = 10,
        .alignment = 8,
        .thread_safe = true,
        .name = "demo_pool"
    };
    
    uint32_t pool_id;
    if (rtos_memory_monitor_create_pool(&pool_config, &pool_id) == RTOS_OK) {
        printf("内存池创建成功: ID=%lu\n", pool_id);
        
        /* 分配和释放内存块 */
        void *blocks[5];
        for (int i = 0; i < 5; i++) {
            blocks[i] = rtos_memory_monitor_pool_alloc(pool_id);
            if (blocks[i]) {
                printf("分配内存块 %d: %p\n", i, blocks[i]);
            }
        }
        
        for (int i = 0; i < 5; i++) {
            if (blocks[i]) {
                rtos_memory_monitor_pool_free(pool_id, blocks[i]);
                printf("释放内存块 %d\n", i);
            }
        }
        
        /* 获取内存池统计 */
        rtos_memory_pool_stats_t pool_stats;
        if (rtos_memory_monitor_get_pool_stats(pool_id, &pool_stats) == RTOS_OK) {
            printf("内存池统计: 总块数=%lu, 空闲=%lu, 已用=%lu\n",
                   pool_stats.total_blocks, pool_stats.free_blocks, pool_stats.used_blocks);
        }
    }
    
    /* 演示内存泄漏检测 */
    printf("\n演示内存泄漏检测:\n");
    void *leak_ptr = rtos_malloc(256);
    printf("分配256字节内存: %p\n", leak_ptr);
    
    /* 故意不释放，演示泄漏检测 */
    rtos_memory_monitor_periodic_task();
    
    rtos_memory_leak_stats_t leak_stats;
    if (rtos_memory_monitor_get_leak_stats(&leak_stats) == RTOS_OK) {
        printf("泄漏统计: 分配=%lu, 释放=%lu, 泄漏=%lu\n",
               leak_stats.alloc_count, leak_stats.free_count, leak_stats.leak_count);
    }
    
    /* 释放内存 */
    rtos_free(leak_ptr);
    printf("内存已释放\n");
}

/**
 * @brief 演示GPIO功能
 */
void app_demo_gpio_functionality(void)
{
    printf("\n--- GPIO功能演示 ---\n");
    
    /* 配置额外的GPIO用于演示 */
    rtos_gpio_config_t demo_gpio_config = RTOS_GPIO_MAKE_CONFIG(
        RTOS_GPIO_PORT_A, RTOS_GPIO_PIN_2, RTOS_GPIO_MODE_OUTPUT_PP);
    
    rtos_gpio_handle_t *demo_gpio = NULL;
    if (rtos_gpio_manager_config_pin(&demo_gpio_config, &demo_gpio) == RTOS_OK) {
        printf("演示GPIO配置完成 (PA2)\n");
        
        /* 演示GPIO操作 */
        for (int i = 0; i < 10; i++) {
            rtos_gpio_manager_write_pin(demo_gpio, true);
            rtos_hw_delay_ms(50);
            rtos_gpio_manager_write_pin(demo_gpio, false);
            rtos_hw_delay_ms(50);
        }
        
        printf("GPIO演示完成: 10次闪烁\n");
        
        /* 反配置GPIO */
        rtos_gpio_manager_deconfig_pin(demo_gpio);
    }
    
    /* 演示端口批量操作 */
    uint16_t port_value;
    if (rtos_gpio_manager_read_port(RTOS_GPIO_PORT_A, &port_value) == RTOS_OK) {
        printf("PORTA当前值: 0x%04X\n", port_value);
        
        /* 设置PA3-PA7为高电平 */
        rtos_gpio_manager_write_port(RTOS_GPIO_PORT_A, 0x00F8, 0x00F8);
        rtos_hw_delay_ms(100);
        rtos_gpio_manager_write_port(RTOS_GPIO_PORT_A, 0x00F8, 0x0000);
        
        printf("端口批量操作演示完成\n");
    }
}

/**
 * @brief 系统信息显示
 */
void app_show_system_info(void)
{
    printf("\n=====================================\n");
    printf("系统运行信息\n");
    printf("=====================================\n");
    
    /* 硬件信息 */
    char hw_info[512];
    uint32_t hw_info_len = rtos_hw_get_info_string(hw_info, sizeof(hw_info));
    if (hw_info_len > 0) {
        printf("硬件信息:\n%s\n", hw_info);
    }
    
    /* 电源管理统计 */
    char power_stats[512];
    uint32_t power_stats_len = rtos_power_manager_get_statistics(power_stats, sizeof(power_stats));
    if (power_stats_len > 0) {
        printf("%s\n", power_stats);
    }
    
    /* 内存管理统计 */
    char memory_stats[512];
    uint32_t memory_stats_len = rtos_memory_monitor_get_statistics(memory_stats, sizeof(memory_stats));
    if (memory_stats_len > 0) {
        printf("%s\n", memory_stats);
    }
    
    /* 看门狗统计 */
    char watchdog_stats[512];
    uint32_t watchdog_stats_len = rtos_watchdog_manager_get_statistics(watchdog_stats, sizeof(watchdog_stats));
    if (watchdog_stats_len > 0) {
        printf("%s\n", watchdog_stats);
    }
    
    printf("=====================================\n");
}

/**
 * @brief 应用程序演示函数
 * 可以通过UART命令调用
 */
void app_run_demo(void)
{
    printf("\n开始硬件抽象层功能演示\n");
    
    /* 演示各模块功能 */
    app_demo_memory_management();
    app_demo_gpio_functionality();
    
    /* 运行性能基准测试 */
    printf("\n--- 性能基准测试 ---\n");
    rtos_hw_run_performance_benchmark();
    
    /* 显示系统信息 */
    app_show_system_info();
    
    printf("硬件抽象层功能演示完成\n");
}

/**
 * @brief 错误处理函数
 */
void app_error_handler(const char *error_msg)
{
    printf("应用错误: %s\n", error_msg);
    
    /* 通过UART发送错误信息 */
    char error_report[128];
    snprintf(error_report, sizeof(error_report), "ERROR: %s\r\n", error_msg);
    rtos_uart_manager_send(RTOS_UART_PORT_1, (const uint8_t*)error_report, strlen(error_report));
    
    /* 可以在这里添加错误恢复逻辑 */
}

/* 系统钩子函数 */

/**
 * @brief 空闲任务钩子
 */
void rtos_idle_hook(void)
{
    /* 系统空闲时执行低功耗策略 */
    static uint32_t idle_count = 0;
    idle_count++;
    
    /* 每1000次空闲循环检查一次电源策略 */
    if (idle_count % 1000 == 0) {
        rtos_power_manager_periodic_task();
    }
}

/**
 * @brief 堆栈溢出钩子
 */
void rtos_stack_overflow_hook(uint32_t task_id)
{
    char overflow_msg[64];
    snprintf(overflow_msg, sizeof(overflow_msg), "Stack Overflow in Task %lu", task_id);
    app_error_handler(overflow_msg);
    
    /* 记录堆栈溢出事件 */
    rtos_memory_monitor_check_stack_overflow(task_id);
}

/**
 * @brief 内存分配失败钩子
 */
void rtos_malloc_failed_hook(uint32_t size)
{
    char malloc_msg[64];
    snprintf(malloc_msg, sizeof(malloc_msg), "Malloc Failed: %lu bytes", size);
    app_error_handler(malloc_msg);
    
    /* 强制执行垃圾回收（如果实现了的话） */
    rtos_memory_monitor_periodic_task();
}