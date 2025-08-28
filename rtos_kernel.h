/**
 * @file rtos_kernel.h
 * @brief 基于优先级抢占调度的RTOS内核头文件
 * @author Assistant
 * @date 2024
 * 
 * 特性：
 * 1. 完全基于优先级抢占调度
 * 2. 无系统滴答时钟，基于动态定时器
 * 3. 精准微秒级定时功能
 * 4. 针对STM32F407优化
 */

#ifndef __RTOS_KERNEL_H__
#define __RTOS_KERNEL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 系统配置参数 */
#define RTOS_MAX_TASKS          16      /* 最大任务数量 */
#define RTOS_MAX_SEMAPHORES     8       /* 最大信号量数量 */
#define RTOS_MAX_MUTEXES        8       /* 最大互斥量数量 */
#define RTOS_MAX_QUEUES         8       /* 最大消息队列数量 */
#define RTOS_STACK_SIZE_MIN     256     /* 最小任务栈大小(字节) */
#define RTOS_PRIORITY_LEVELS    32      /* 优先级级别数 (0-31, 0最高) */
#define RTOS_QUEUE_ITEM_SIZE    32      /* 队列项目大小(字节) */
#define RTOS_MAX_SW_TIMERS      16      /* 最大软件定时器数量 */
#define RTOS_MAX_EVENT_GROUPS   8       /* 最大事件组数量 */
#define RTOS_MAX_MEMORY_POOLS   8       /* 最大内存池数量 */

/* 时间精度配置 */
#define RTOS_TIME_UNIT_NS       1       /* 纳秒级时间单位 */
#define RTOS_TIME_UNIT_US       1000    /* 微秒级时间单位 */
#define RTOS_TIME_UNIT_MS       1000000 /* 毫秒级时间单位 */

/* 任务状态定义 */
typedef enum {
    TASK_STATE_READY = 0,       /* 就绪状态 */
    TASK_STATE_RUNNING,         /* 运行状态 */
    TASK_STATE_BLOCKED,         /* 阻塞状态 */
    TASK_STATE_SUSPENDED,       /* 挂起状态 */
    TASK_STATE_DELETED          /* 删除状态 */
} rtos_task_state_t;

/* 错误代码定义 */
typedef enum {
    RTOS_OK = 0,
    RTOS_ERROR,
    RTOS_ERROR_TIMEOUT,
    RTOS_ERROR_NO_MEMORY,
    RTOS_ERROR_INVALID_PARAM,
    RTOS_ERROR_RESOURCE_BUSY,
    RTOS_ERROR_DEADLOCK,
    RTOS_ERROR_STACK_OVERFLOW,
    RTOS_ERROR_MEMORY_CORRUPTION
} rtos_result_t;

/* 时间单位定义 */
typedef uint32_t rtos_time_unit_t;

#ifndef RTOS_TYPES_DEFINED
#define RTOS_TYPES_DEFINED
typedef uint64_t rtos_time_ns_t;
typedef enum {
    RTOS_SLEEP_NONE = 0,
    RTOS_SLEEP_LIGHT,    /* 浅度睡眠 */
    RTOS_SLEEP_DEEP,     /* 深度睡眠 */
    RTOS_SLEEP_STANDBY   /* 待机模式 */
} rtos_sleep_mode_t;
#endif

/* 错误类型定义 */
typedef enum {
    RTOS_ERROR_TYPE_STACK_OVERFLOW,
    RTOS_ERROR_TYPE_DEADLOCK,
    RTOS_ERROR_TYPE_PRIORITY_INVERSION,
    RTOS_ERROR_TYPE_MEMORY_CORRUPTION
} rtos_error_type_t;

/* 任务控制块 */
typedef struct rtos_task {
    uint32_t *stack_ptr;           /* 栈指针 */
    uint32_t *stack_base;          /* 栈基址 */
    uint32_t stack_size;           /* 栈大小 */
    uint8_t priority;              /* 任务优先级 */
    uint8_t original_priority;     /* 原始优先级(优先级继承用) */
    rtos_task_state_t state;       /* 任务状态 */
    rtos_time_ns_t delay_time_ns;  /* 延时时间(纳秒) */
    struct rtos_task *next;        /* 链表指针 */
    char name[16];                 /* 任务名称 */
    void (*task_func)(void *);     /* 任务函数 */
    void *param;                   /* 任务参数 */
    
    /* 调试和安全信息 */
    uint32_t task_switch_count;    /* 任务切换计数 */
    uint32_t stack_high_water;     /* 栈使用高水位 */
    uint32_t runtime_us;           /* 运行时间(微秒) */
    bool stack_check_enabled;     /* 栈检查使能 */
    uint32_t stack_canary;         /* 栈保护字 */
    
    /* MPU保护区域 */
    uint8_t mpu_region_count;      /* MPU区域数量 */
    void *mpu_regions[8];          /* MPU区域配置 */
} rtos_task_t;

/* 信号量控制块 */
typedef struct {
    uint32_t count;                /* 信号量计数值 */
    uint32_t max_count;            /* 最大计数值 */
    rtos_task_t *wait_list;        /* 等待任务链表 */
    bool is_valid;                 /* 是否有效 */
} rtos_semaphore_t;

/* 互斥量控制块 */
typedef struct {
    rtos_task_t *owner;            /* 持有者任务 */
    uint32_t nest_count;           /* 嵌套计数 */
    uint8_t original_priority;     /* 原始优先级(优先级继承) */
    rtos_task_t *wait_list;        /* 等待任务链表 */
    bool is_valid;                 /* 是否有效 */
} rtos_mutex_t;

/* 消息队列控制块 */
typedef struct {
    uint8_t *buffer;               /* 缓冲区 */
    uint32_t item_size;            /* 单个消息大小 */
    uint32_t max_items;            /* 最大消息数 */
    uint32_t head;                 /* 队列头 */
    uint32_t tail;                 /* 队列尾 */
    uint32_t count;                /* 当前消息数 */
    rtos_task_t *send_wait_list;   /* 发送等待链表 */
    rtos_task_t *recv_wait_list;   /* 接收等待链表 */
    bool is_valid;                 /* 是否有效 */
} rtos_queue_t;

/* 定时器控制块 */
typedef struct rtos_timer {
    rtos_time_ns_t expire_time_ns; /* 到期时间(纳秒) */
    rtos_task_t *task;             /* 关联任务 */
    struct rtos_timer *next;       /* 链表指针 */
    bool is_active;                /* 是否激活 */
} rtos_timer_t;

/* 软件定时器控制块 */
typedef struct rtos_sw_timer {
    rtos_time_ns_t period_ns;      /* 周期时间(纳秒) */
    rtos_time_ns_t next_expire_ns; /* 下次到期时间 */
    bool auto_reload;              /* 自动重装载 */
    bool is_active;                /* 是否激活 */
    void (*callback)(void *param); /* 回调函数 */
    void *param;                   /* 回调参数 */
    char name[16];                 /* 定时器名称 */
    struct rtos_sw_timer *next;    /* 链表指针 */
} rtos_sw_timer_t;

/* 事件标志组控制块 */
typedef struct {
    uint32_t flags;                /* 事件标志位 */
    rtos_task_t *wait_list;        /* 等待任务链表 */
    bool is_valid;                 /* 是否有效 */
} rtos_event_group_t;

/* 内存池控制块 */
typedef struct {
    void *pool_buffer;             /* 内存池缓冲区 */
    uint32_t block_size;           /* 块大小 */
    uint32_t block_count;          /* 块数量 */
    uint32_t free_blocks;          /* 空闲块数 */
    void *free_list;               /* 空闲链表 */
    rtos_task_t *wait_list;        /* 等待链表 */
    bool is_valid;                 /* 是否有效 */
} rtos_memory_pool_t;

/* MPU保护区域配置 */
typedef struct {
    uint32_t base_addr;            /* 基地址 */
    uint32_t size;                 /* 大小 */
    uint32_t permissions;          /* 权限 */
    bool enable;                   /* 使能 */
} rtos_mpu_region_t;

/* 调试信息结构 */
typedef struct {
    uint32_t task_switch_count;    /* 任务切换次数 */
    uint32_t interrupt_count;      /* 中断次数 */
    uint32_t cpu_usage_percent;    /* CPU使用率 */
    uint32_t free_stack_size;      /* 剩余栈大小 */
    uint32_t heap_free_size;       /* 剩余堆大小 */
    rtos_time_ns_t uptime_ns;      /* 系统运行时间 */
} rtos_debug_info_t;

/* 跟踪事件类型 */
typedef enum {
    RTOS_TRACE_TASK_SWITCH,
    RTOS_TRACE_INTERRUPT_ENTER,
    RTOS_TRACE_INTERRUPT_EXIT,
    RTOS_TRACE_API_CALL,
    RTOS_TRACE_TIMER_EXPIRE
} rtos_trace_event_t;

/* 跟踪事件记录 */
typedef struct {
    rtos_trace_event_t event_type;
    rtos_time_ns_t timestamp_ns;
    uint32_t task_id;
    uint32_t data;
} rtos_trace_record_t;

/* 系统控制块 */
typedef struct {
    rtos_task_t *current_task;     /* 当前运行任务 */
    rtos_task_t *ready_list[RTOS_PRIORITY_LEVELS];  /* 就绪队列 */
    uint32_t ready_bitmap;         /* 就绪位图 */
    rtos_timer_t *timer_list;      /* 硬件定时器链表 */
    rtos_sw_timer_t *sw_timer_list; /* 软件定时器链表 */
    rtos_time_ns_t system_time_ns; /* 系统时间(纳秒) */
    bool scheduler_running;        /* 调度器是否运行 */
    bool in_critical;              /* 是否在临界区 */
    uint32_t critical_nesting;     /* 临界区嵌套计数 */
    
    /* 功耗管理 */
    rtos_sleep_mode_t sleep_mode;  /* 当前睡眠模式 */
    uint32_t wakeup_sources;       /* 唤醒源配置 */
    
    /* 调试和跟踪 */
    bool trace_enabled;            /* 跟踪使能 */
    rtos_trace_record_t *trace_buffer; /* 跟踪缓冲区 */
    uint32_t trace_buffer_size;    /* 跟踪缓冲区大小 */
    uint32_t trace_index;          /* 当前跟踪索引 */
    
    /* 统计信息 */
    uint32_t total_task_switches;  /* 总任务切换次数 */
    uint32_t total_interrupts;     /* 总中断次数 */
    rtos_time_ns_t idle_time_ns;   /* 空闲时间 */
    
    /* 错误处理 */
    void (*error_handlers[4])(void); /* 错误处理函数 */
} rtos_system_t;

/* 系统API声明 */

/* 系统初始化和启动 */
rtos_result_t rtos_init(void);
rtos_result_t rtos_start(void);

/* 任务管理 */
rtos_result_t rtos_task_create(rtos_task_t *task, 
                              const char *name,
                              void (*task_func)(void *),
                              void *param,
                              uint8_t priority,
                              uint32_t *stack,
                              uint32_t stack_size);
rtos_result_t rtos_task_delete(rtos_task_t *task);
rtos_result_t rtos_task_suspend(rtos_task_t *task);
rtos_result_t rtos_task_resume(rtos_task_t *task);
void rtos_task_yield(void);
rtos_task_t *rtos_get_current_task(void);

/* 延时功能 */
rtos_result_t rtos_delay_us(uint32_t microseconds);
rtos_result_t rtos_delay_ms(uint32_t milliseconds);
rtos_result_t rtos_delay_ns(rtos_time_ns_t nanoseconds);
rtos_result_t rtos_delay_ticks(uint64_t ticks, rtos_time_unit_t unit);
rtos_result_t rtos_delay_until(rtos_time_ns_t absolute_time_ns);

/* 信号量 */
rtos_result_t rtos_semaphore_create(rtos_semaphore_t *sem, uint32_t initial_count, uint32_t max_count);
rtos_result_t rtos_semaphore_take(rtos_semaphore_t *sem, uint32_t timeout_us);
rtos_result_t rtos_semaphore_give(rtos_semaphore_t *sem);
rtos_result_t rtos_semaphore_delete(rtos_semaphore_t *sem);

/* 互斥量 */
rtos_result_t rtos_mutex_create(rtos_mutex_t *mutex);
rtos_result_t rtos_mutex_lock(rtos_mutex_t *mutex, uint32_t timeout_us);
rtos_result_t rtos_mutex_unlock(rtos_mutex_t *mutex);
rtos_result_t rtos_mutex_delete(rtos_mutex_t *mutex);

/* 消息队列 */
rtos_result_t rtos_queue_create(rtos_queue_t *queue, void *buffer, uint32_t item_size, uint32_t max_items);
rtos_result_t rtos_queue_send(rtos_queue_t *queue, const void *item, uint32_t timeout_us);
rtos_result_t rtos_queue_receive(rtos_queue_t *queue, void *item, uint32_t timeout_us);
rtos_result_t rtos_queue_delete(rtos_queue_t *queue);

/* 临界区管理 */
void rtos_enter_critical(void);
void rtos_exit_critical(void);

/* 系统时间 */
uint32_t rtos_get_time_us(void);
uint32_t rtos_get_time_ms(void);
rtos_time_ns_t rtos_get_time_ns(void);

/* 调度器控制 */
void rtos_scheduler_suspend(void);
void rtos_scheduler_resume(void);

/* 内部函数声明(供汇编和硬件层调用) */
void rtos_schedule(void);
void rtos_context_switch(void);
uint32_t *rtos_task_stack_init(uint32_t *stack_top, void (*task_func)(void *), void *param);

/* STM32F407硬件相关 */
void rtos_hw_init(void);
void rtos_hw_timer_init(void);
void rtos_hw_timer_set(uint32_t microseconds);
void rtos_hw_timer_stop(void);
uint32_t rtos_hw_get_time_us(void);
void rtos_hw_disable_interrupts(void);
void rtos_hw_enable_interrupts(void);

/* 软件定时器 */
rtos_result_t rtos_sw_timer_create(rtos_sw_timer_t *timer, const char *name,
                                  rtos_time_ns_t period_ns, bool auto_reload,
                                  void (*callback)(void *), void *param);
rtos_result_t rtos_sw_timer_start(rtos_sw_timer_t *timer);
rtos_result_t rtos_sw_timer_stop(rtos_sw_timer_t *timer);
rtos_result_t rtos_sw_timer_delete(rtos_sw_timer_t *timer);
rtos_result_t rtos_sw_timer_reset(rtos_sw_timer_t *timer);

/* 事件标志组 */
rtos_result_t rtos_event_group_create(rtos_event_group_t *group);
rtos_result_t rtos_event_group_set_bits(rtos_event_group_t *group, uint32_t bits);
rtos_result_t rtos_event_group_clear_bits(rtos_event_group_t *group, uint32_t bits);
rtos_result_t rtos_event_group_wait_bits(rtos_event_group_t *group, 
                                        uint32_t bits_to_wait,
                                        bool clear_on_exit,
                                        bool wait_for_all,
                                        uint32_t timeout_us);
rtos_result_t rtos_event_group_delete(rtos_event_group_t *group);

/* 内存池管理 */
rtos_result_t rtos_memory_pool_create(rtos_memory_pool_t *pool,
                                     void *buffer,
                                     uint32_t block_size,
                                     uint32_t block_count);
void *rtos_memory_pool_alloc(rtos_memory_pool_t *pool, uint32_t timeout_us);
rtos_result_t rtos_memory_pool_free(rtos_memory_pool_t *pool, void *block);
rtos_result_t rtos_memory_pool_delete(rtos_memory_pool_t *pool);

/* MPU内存保护 */
rtos_result_t rtos_mpu_init(void);
rtos_result_t rtos_mpu_configure_region(uint8_t region_id, rtos_mpu_region_t *config);
rtos_result_t rtos_mpu_enable_task_protection(rtos_task_t *task);
rtos_result_t rtos_mpu_disable_task_protection(rtos_task_t *task);

/* 功耗管理 */
rtos_result_t rtos_power_enter_sleep(rtos_sleep_mode_t mode);
rtos_result_t rtos_power_configure_wakeup(uint32_t sources);
rtos_result_t rtos_power_set_cpu_frequency(uint32_t frequency_hz);

/* 调试和监控 */
rtos_result_t rtos_debug_get_task_info(rtos_task_t *task, rtos_debug_info_t *info);
rtos_result_t rtos_debug_get_system_info(rtos_debug_info_t *info);
rtos_result_t rtos_debug_print_system_state(void);
rtos_result_t rtos_debug_enable_stack_checking(rtos_task_t *task);
rtos_result_t rtos_debug_disable_stack_checking(rtos_task_t *task);

/* 安全特性 */
rtos_result_t rtos_safety_register_error_handler(rtos_error_type_t type, 
                                                void (*handler)(void));
rtos_result_t rtos_safety_check_stack_overflow(rtos_task_t *task);
rtos_result_t rtos_safety_enable_watchdog(uint32_t timeout_ms);

/* 系统跟踪 */
rtos_result_t rtos_trace_start(rtos_trace_record_t *buffer, uint32_t buffer_size);
rtos_result_t rtos_trace_stop(void);
rtos_result_t rtos_trace_get_data(rtos_trace_record_t *buffer, uint32_t *count);
rtos_result_t rtos_trace_add_event(rtos_trace_event_t event, uint32_t data);

/* 高级同步 */
rtos_result_t rtos_wait_multiple(void **objects, uint32_t count, 
                               bool wait_all, uint32_t timeout_us);

/* 中断服务程序 */
void rtos_systick_handler(void);
void rtos_pendsv_handler(void);
void rtos_timer_isr(void);
void rtos_sw_timer_isr(void);

#endif /* __RTOS_KERNEL_H__ */