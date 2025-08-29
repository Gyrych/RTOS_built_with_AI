/**
 * @file rtos_task.h
 * @brief RTOS任务模块头文件 - 基于内核对象实现
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_TASK_H__
#define __RTOS_TASK_H__

#include "rtos_object.h"
#include <stdint.h>
#include <stdbool.h>

/* 任务状态定义 */
typedef enum {
    RTOS_TASK_INIT = 0,             /* 初始状态 */
    RTOS_TASK_READY,                /* 就绪状态 */
    RTOS_TASK_SUSPEND,              /* 挂起状态 */
    RTOS_TASK_RUNNING,              /* 运行状态 */
    RTOS_TASK_BLOCK,                /* 阻塞状态 */
    RTOS_TASK_CLOSE                 /* 关闭状态 */
} rtos_task_state_t;

/* 任务控制标志 */
#define RTOS_TASK_FLAG_NONE            0x00
#define RTOS_TASK_FLAG_STACK_CHK       0x01    /* 栈检查使能 */
#define RTOS_TASK_FLAG_CLEANUP         0x02    /* 清理标志 */

/**
 * @brief 任务控制块结构体
 * 继承自内核对象基类
 */
typedef struct rtos_task {
    struct rtos_object  parent;                 /* 继承对象基类 */
    
    /* 任务基本信息 */
    uint32_t           *sp;                     /* 栈指针 */
    uint32_t           *stack_addr;             /* 栈基址 */
    uint32_t            stack_size;             /* 栈大小 */
    
    /* 调度相关 */
    uint8_t             current_priority;       /* 当前优先级 */
    uint8_t             init_priority;          /* 初始优先级 */
    uint8_t             number_mask;            /* 位图掩码 */
    uint8_t             high_mask;              /* 高位掩码 */
    uint32_t            number;                 /* 任务编号 */
    
    /* 状态和标志 */
    rtos_task_state_t   stat;                   /* 任务状态 */
    uint32_t            task_flags;             /* 任务标志 */
    
    /* 链表节点 */
    struct rtos_task   *tlist;                  /* 任务链表 */
    
    /* 定时器相关 */
    rtos_time_ns_t      remaining_tick;         /* 剩余时间片 */
    rtos_time_ns_t      init_tick;              /* 初始时间片 */
    
    /* 任务函数和参数 */
    void               (*entry)(void *parameter); /* 任务入口函数 */
    void               *parameter;              /* 任务参数 */
    
    /* 栈监控 */
    uint32_t           *stack_top;              /* 栈顶指针 */
    uint32_t            max_used_stack;         /* 最大栈使用量 */
    
    /* 错误代码 */
    rtos_result_t       error;                  /* 错误代码 */
    
    /* 用户数据 */
    uint32_t            user_data;              /* 用户数据 */
    
    /* 清理函数 */
    void               (*cleanup)(struct rtos_task *tid); /* 任务清理函数 */
    
    /* 事件集 */
    uint32_t            event_set;              /* 事件集合 */
    uint32_t            event_info;             /* 事件信息 */
    
    /* 统计信息 */
    uint32_t            switch_count;           /* 切换次数 */
    rtos_time_ns_t      total_runtime;          /* 总运行时间 */
    rtos_time_ns_t      last_start_time;        /* 上次开始时间 */
} rtos_task_t;

/* 任务优先级定义 */
#define RTOS_TASK_PRIORITY_MAX          32
#define RTOS_TASK_PRIORITY_MIN          0

/* 空闲任务和主任务优先级 */
#define RTOS_IDLE_THREAD_PRIORITY       (RTOS_TASK_PRIORITY_MAX - 1)
#define RTOS_MAIN_THREAD_PRIORITY       (RTOS_TASK_PRIORITY_MAX / 3)

/* 默认时间片 */
#define RTOS_TASK_TIMESLICE_DEFAULT     10000   /* 10ms */

/* 任务状态宏 */
#define RTOS_TASK_STAT_MASK             0x07
#define RTOS_TASK_STAT_SIGNAL           0x08
#define RTOS_TASK_STAT_SIGNAL_READY     (RTOS_TASK_READY | RTOS_TASK_STAT_SIGNAL)
#define RTOS_TASK_STAT_SIGNAL_SUSPEND   (RTOS_TASK_SUSPEND | RTOS_TASK_STAT_SIGNAL)
#define RTOS_TASK_STAT_SIGNAL_MASK      0x0f

/* 任务API函数声明 */

/**
 * @brief 初始化任务系统
 */
void rtos_task_system_init(void);

/**
 * @brief 创建任务 - 静态方式
 * @param task 任务控制块指针
 * @param name 任务名称
 * @param entry 任务入口函数
 * @param parameter 任务参数
 * @param stack_start 栈起始地址
 * @param stack_size 栈大小
 * @param priority 任务优先级
 * @param tick 时间片
 * @return 结果码
 */
rtos_result_t rtos_task_init(rtos_task_t *task,
                            const char  *name,
                            void        (*entry)(void *parameter),
                            void        *parameter,
                            void        *stack_start,
                            uint32_t     stack_size,
                            uint8_t      priority,
                            uint32_t     tick);

/**
 * @brief 创建任务 - 动态方式
 * @param name 任务名称
 * @param entry 任务入口函数
 * @param parameter 任务参数
 * @param stack_size 栈大小
 * @param priority 任务优先级
 * @param tick 时间片
 * @return 任务指针
 */
rtos_task_t *rtos_task_create(const char *name,
                             void       (*entry)(void *parameter),
                             void       *parameter,
                             uint32_t    stack_size,
                             uint8_t     priority,
                             uint32_t    tick);

/**
 * @brief 分离任务
 * @param task 任务指针
 * @return 结果码
 */
rtos_result_t rtos_task_detach(rtos_task_t *task);

/**
 * @brief 删除任务
 * @param task 任务指针
 * @return 结果码
 */
rtos_result_t rtos_task_delete(rtos_task_t *task);

/**
 * @brief 启动任务
 * @param task 任务指针
 * @return 结果码
 */
rtos_result_t rtos_task_startup(rtos_task_t *task);

/**
 * @brief 获取当前任务
 * @return 当前任务指针
 */
rtos_task_t *rtos_task_self(void);

/**
 * @brief 查找任务
 * @param name 任务名称
 * @return 任务指针
 */
rtos_task_t *rtos_task_find(const char *name);

/**
 * @brief 让出CPU
 * @return 结果码
 */
rtos_result_t rtos_task_yield(void);

/**
 * @brief 睡眠指定时间(毫秒)
 * @param ms 毫秒数
 * @return 结果码
 */
rtos_result_t rtos_task_mdelay(uint32_t ms);

/**
 * @brief 睡眠指定时间(微秒)
 * @param us 微秒数
 * @return 结果码
 */
rtos_result_t rtos_task_udelay(uint32_t us);

/**
 * @brief 睡眠指定tick数
 * @param tick tick数
 * @return 结果码
 */
rtos_result_t rtos_task_delay(rtos_time_ns_t tick);

/**
 * @brief 延时到指定时间点
 * @param tick 绝对时间点
 * @return 结果码
 */
rtos_result_t rtos_task_delay_until(rtos_time_ns_t *tick);

/**
 * @brief 控制任务状态
 * @param task 任务指针
 * @param cmd 控制命令
 * @param arg 参数
 * @return 结果码
 */
rtos_result_t rtos_task_control(rtos_task_t *task, int cmd, void *arg);

/**
 * @brief 挂起任务
 * @param task 任务指针
 * @return 结果码
 */
rtos_result_t rtos_task_suspend(rtos_task_t *task);

/**
 * @brief 恢复任务
 * @param task 任务指针
 * @return 结果码
 */
rtos_result_t rtos_task_resume(rtos_task_t *task);

/**
 * @brief 设置任务优先级
 * @param task 任务指针
 * @param priority 新优先级
 * @return 旧优先级
 */
uint8_t rtos_task_set_priority(rtos_task_t *task, uint8_t priority);

/**
 * @brief 获取任务优先级
 * @param task 任务指针
 * @return 优先级
 */
uint8_t rtos_task_get_priority(rtos_task_t *task);

/**
 * @brief 设置任务错误码
 * @param task 任务指针
 * @param error 错误码
 */
void rtos_task_set_errno(rtos_task_t *task, rtos_result_t error);

/**
 * @brief 获取任务错误码
 * @param task 任务指针
 * @return 错误码
 */
rtos_result_t rtos_task_get_errno(rtos_task_t *task);

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
 * @brief 调度器是否已启动
 * @return 是否已启动
 */
bool rtos_scheduler_is_running(void);

/**
 * @brief 执行调度
 */
void rtos_schedule(void);

/**
 * @brief 插入任务到就绪队列
 * @param task 任务指针
 */
void rtos_schedule_insert_task(rtos_task_t *task);

/**
 * @brief 从就绪队列移除任务
 * @param task 任务指针
 */
void rtos_schedule_remove_task(rtos_task_t *task);

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
 * @param texit 任务退出函数
 * @return 栈指针
 */
uint32_t *rtos_hw_stack_init(void       *task_entry,
                            void       *parameter,
                            uint32_t   *stack_addr,
                            void       *texit);

/* 任务钩子函数类型 */
typedef void (*rtos_task_switch_hook_t)(rtos_task_t *from, rtos_task_t *to);
typedef void (*rtos_task_inout_hook_t)(rtos_task_t *task);

/**
 * @brief 设置任务切换钩子
 * @param hook 钩子函数
 */
void rtos_task_set_switch_hook(rtos_task_switch_hook_t hook);

/**
 * @brief 设置任务进入钩子
 * @param hook 钩子函数
 */
void rtos_task_set_enter_hook(rtos_task_inout_hook_t hook);

/**
 * @brief 设置任务退出钩子
 * @param hook 钩子函数
 */
void rtos_task_set_exit_hook(rtos_task_inout_hook_t hook);

/* 任务信息获取 */
typedef struct {
    rtos_task_state_t   state;                  /* 任务状态 */
    uint8_t             priority;               /* 优先级 */
    uint8_t             max_priority;           /* 最大优先级 */
    uint32_t            stack_size;             /* 栈大小 */
    uint32_t            free_stack;             /* 剩余栈空间 */
    char                name[16];               /* 任务名称 */
} rtos_task_info_t;

/**
 * @brief 获取任务信息
 * @param task 任务指针
 * @param info 信息结构体指针
 * @return 结果码
 */
rtos_result_t rtos_task_get_info(rtos_task_t *task, rtos_task_info_t *info);

#endif /* __RTOS_TASK_H__ */