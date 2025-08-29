/**
 * @file task.h
 * @brief RTOS任务管理 - 重构后的任务系统
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_TASK_H__
#define __RTOS_TASK_H__

#include "../core/types.h"
#include "../core/object.h"

/* 任务链表节点 - 统一链表设计 */
typedef struct rtos_task_node {
    struct rtos_task      *task;               /* 任务指针 */
    struct rtos_task_node *next;               /* 下一个节点 */
    struct rtos_task_node *prev;               /* 上一个节点 */
} rtos_task_node_t;

/* 任务控制块结构体 */
typedef struct rtos_task {
    rtos_object_t          parent;             /* 继承对象基类 */
    
    /* 任务基本信息 */
    void                  (*entry)(void *);    /* 任务入口函数 */
    void                  *parameter;          /* 任务参数 */
    uint32_t              *stack_addr;         /* 栈基址 */
    uint32_t              *stack_top;          /* 栈顶指针 */
    uint32_t              *sp;                 /* 当前栈指针 */
    rtos_stack_size_t     stack_size;          /* 栈大小 */
    
    /* 调度相关 */
    rtos_priority_t       priority;            /* 当前优先级 */
    rtos_priority_t       base_priority;       /* 基础优先级 */
    rtos_timeslice_t      timeslice;           /* 时间片 */
    rtos_timeslice_t      remaining_timeslice; /* 剩余时间片 */
    
    /* 状态管理 */
    rtos_task_state_t     state;               /* 任务状态 */
    rtos_task_flag_t      flags;               /* 任务标志 */
    uint32_t              suspend_count;       /* 挂起计数 */
    
    /* 链表节点 - 使用统一节点设计 */
    rtos_task_node_t      ready_node;          /* 就绪队列节点 */
    rtos_task_node_t      block_node;          /* 阻塞队列节点 */
    
    /* 同步相关 */
    struct rtos_object    *wait_object;        /* 等待的对象 */
    rtos_timeout_t        timeout;             /* 超时时间 */
    
    /* 统计信息 */
    rtos_time_ns_t        total_runtime;       /* 总运行时间 */
    uint32_t              switch_count;        /* 切换次数 */
    uint32_t              max_stack_usage;     /* 最大栈使用量 */
    uint32_t              current_stack_usage; /* 当前栈使用量 */
    rtos_time_ns_t        last_run_time;       /* 上次运行时间 */
    
    /* 栈溢出检测 - 新增 */
    uint32_t              stack_magic_start;   /* 栈起始魔数 */
    uint32_t              stack_magic_end;     /* 栈结束魔数 */
    bool                  stack_overflow_flag; /* 栈溢出标志 */
    
    /* 清理函数 */
    void                  (*cleanup)(struct rtos_task *); /* 清理函数 */
} rtos_task_t;

/* 任务创建参数结构体 */
typedef struct {
    const char            *name;               /* 任务名称 */
    void                  (*entry)(void *);    /* 任务入口函数 */
    void                  *parameter;          /* 任务参数 */
    rtos_stack_size_t     stack_size;          /* 栈大小 */
    rtos_priority_t       priority;            /* 优先级 */
    rtos_timeslice_t      timeslice;           /* 时间片 */
    rtos_task_flag_t      flags;               /* 任务标志 */
} rtos_task_create_params_t;

/* 任务信息结构体 */
typedef struct {
    char                  name[16];            /* 任务名称 */
    rtos_task_state_t     state;               /* 任务状态 */
    rtos_priority_t       priority;            /* 优先级 */
    rtos_stack_size_t     stack_size;          /* 栈大小 */
    uint32_t              free_stack;          /* 剩余栈空间 */
    uint32_t              switch_count;        /* 切换次数 */
    rtos_time_ns_t        total_runtime;       /* 总运行时间 */
    bool                  stack_overflow;      /* 栈溢出标志 */
} rtos_task_info_t;

/* 任务管理API函数声明 */

/**
 * @brief 初始化任务系统
 */
void rtos_task_system_init(void);

/**
 * @brief 创建任务 - 静态方式
 * @param task 任务控制块指针
 * @param params 任务创建参数
 * @return 操作结果
 */
rtos_result_t rtos_task_create_static(rtos_task_t *task,
                                      const rtos_task_create_params_t *params);

/**
 * @brief 创建任务 - 动态方式
 * @param params 任务创建参数
 * @return 任务指针，失败返回NULL
 */
rtos_task_t *rtos_task_create_dynamic(const rtos_task_create_params_t *params);

/**
 * @brief 删除任务
 * @param task 任务指针
 * @return 操作结果
 */
rtos_result_t rtos_task_delete(rtos_task_t *task);

/**
 * @brief 启动任务
 * @param task 任务指针
 * @return 操作结果
 */
rtos_result_t rtos_task_start(rtos_task_t *task);

/**
 * @brief 挂起任务
 * @param task 任务指针
 * @return 操作结果
 */
rtos_result_t rtos_task_suspend(rtos_task_t *task);

/**
 * @brief 恢复任务
 * @param task 任务指针
 * @return 操作结果
 */
rtos_result_t rtos_task_resume(rtos_task_t *task);

/**
 * @brief 让出CPU
 * @return 操作结果
 */
rtos_result_t rtos_task_yield(void);

/**
 * @brief 延时指定时间(毫秒)
 * @param ms 毫秒数
 * @return 操作结果
 */
rtos_result_t rtos_task_delay_ms(uint32_t ms);

/**
 * @brief 延时指定时间(微秒)
 * @param us 微秒数
 * @return 操作结果
 */
rtos_result_t rtos_task_delay_us(uint32_t us);

/**
 * @brief 延时指定时间(纳秒)
 * @param ns 纳秒数
 * @return 操作结果
 */
rtos_result_t rtos_task_delay_ns(rtos_time_ns_t ns);

/**
 * @brief 延时到指定时间点
 * @param absolute_time 绝对时间点
 * @return 操作结果
 */
rtos_result_t rtos_task_delay_until(rtos_time_ns_t absolute_time);

/**
 * @brief 设置任务优先级
 * @param task 任务指针
 * @param priority 新优先级
 * @return 旧优先级
 */
rtos_priority_t rtos_task_set_priority(rtos_task_t *task, rtos_priority_t priority);

/**
 * @brief 获取任务优先级
 * @param task 任务指针
 * @return 优先级
 */
rtos_priority_t rtos_task_get_priority(const rtos_task_t *task);

/**
 * @brief 获取当前任务
 * @return 当前任务指针
 */
rtos_task_t *rtos_task_get_current(void);

/**
 * @brief 查找任务
 * @param name 任务名称
 * @return 任务指针，未找到返回NULL
 */
rtos_task_t *rtos_task_find(const char *name);

/**
 * @brief 获取任务信息
 * @param task 任务指针
 * @param info 信息结构体指针
 * @return 操作结果
 */
rtos_result_t rtos_task_get_info(const rtos_task_t *task, rtos_task_info_t *info);

/**
 * @brief 设置任务清理函数
 * @param task 任务指针
 * @param cleanup 清理函数
 */
void rtos_task_set_cleanup(rtos_task_t *task, void (*cleanup)(rtos_task_t *));

/* 栈管理相关函数 - 新增 */

/**
 * @brief 检查任务栈溢出
 * @param task 任务指针
 * @return 是否溢出
 */
bool rtos_task_check_stack_overflow(const rtos_task_t *task);

/**
 * @brief 获取任务栈使用量
 * @param task 任务指针
 * @return 栈使用量
 */
uint32_t rtos_task_get_stack_usage(const rtos_task_t *task);

/**
 * @brief 获取任务栈剩余量
 * @param task 任务指针
 * @return 栈剩余量
 */
uint32_t rtos_task_get_stack_free(const rtos_task_t *task);

/* 任务调度器相关函数 */

/**
 * @brief 初始化调度器
 */
void rtos_scheduler_init(void);

/**
 * @brief 启动调度器
 */
void rtos_scheduler_start(void);

/**
 * @brief 停止调度器
 */
void rtos_scheduler_stop(void);

/**
 * @brief 检查调度器是否运行
 * @return 是否运行
 */
bool rtos_scheduler_is_running(void);

/**
 * @brief 执行调度
 */
void rtos_scheduler_schedule(void);

/**
 * @brief 锁定调度器
 * @return 锁定级别
 */
uint32_t rtos_scheduler_lock(void);

/**
 * @brief 解锁调度器
 * @param level 锁定级别
 */
void rtos_scheduler_unlock(uint32_t level);

/* 空闲任务相关函数 */

/**
 * @brief 空闲任务入口函数
 * @param parameter 参数
 */
void rtos_task_idle_entry(void *parameter);

/**
 * @brief 设置空闲任务钩子函数
 * @param hook 钩子函数
 */
void rtos_task_set_idle_hook(void (*hook)(void));

/**
 * @brief 删除空闲任务钩子函数
 * @param hook 钩子函数
 */
void rtos_task_delete_idle_hook(void (*hook)(void));

/* 内部函数声明(供其他模块使用) */

/**
 * @brief 任务栈初始化
 * @param task_entry 任务入口
 * @param parameter 参数
 * @param stack_addr 栈地址
 * @param stack_size 栈大小
 * @return 栈指针
 */
uint32_t *rtos_task_stack_init(void (*task_entry)(void *),
                               void *parameter,
                               uint32_t *stack_addr,
                               rtos_stack_size_t stack_size);

/**
 * @brief 任务切换钩子函数类型 */
typedef void (*rtos_task_switch_hook_t)(rtos_task_t *from, rtos_task_t *to);

/**
 * @brief 设置任务切换钩子
 * @param hook 钩子函数
 */
void rtos_task_set_switch_hook(rtos_task_switch_hook_t hook);

/* 兼容性定义 - 保持与现有代码的兼容性 */
#define rtos_task_init rtos_task_create_static
#define rtos_task_startup rtos_task_start
#define rtos_task_mdelay rtos_task_delay_ms
#define rtos_task_udelay rtos_task_delay_us
#define rtos_task_delay rtos_task_delay_ns

#endif /* __RTOS_TASK_H__ */
