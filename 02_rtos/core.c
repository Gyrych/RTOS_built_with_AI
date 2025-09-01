#include "core.h"
#include <string.h>

scheduler_t scheduler;  /* 全局调度器实例 */

/* 空闲任务 - 当没有其他任务运行时执行 */
static void idle_task(void* arg) {
    while (1) {
        __asm("wfi");  /* 等待中断指令，降低功耗 */
    }
}

/* RTOS初始化函数 */
void rtos_init(void) {
    memset(&scheduler, 0, sizeof(scheduler_t));  /* 清空调度器结构体 */
    
    task_create(idle_task, NULL, MAX_PRIORITY);  /* 创建空闲任务 */
}

/* 启动RTOS调度 */
void rtos_start(void) {
    if (scheduler.task_count == 0) {
        return;  /* 没有任务可调度 */
    }
    
    __asm volatile(
        "cpsie i\n"  /* 开启全局中断 */
        "svc 0\n"    /* 触发SVC异常，进入调度器 */
    );
}

/* 创建新任务 */
task_t* task_create(void (*func)(void*), void* arg, uint32_t priority) {
    if (scheduler.task_count >= MAX_TASKS || priority > MAX_PRIORITY) {
        return NULL;  /* 任务数量或优先级超出限制 */
    }
    
    task_t* task = &scheduler.tasks[scheduler.task_count];
    task->task_func = func;      /* 设置任务函数 */
    task->arg = arg;             /* 设置任务参数 */
    task->priority = priority;   /* 设置任务优先级 */
    task->state = TASK_READY;    /* 设置任务状态为就绪 */
    
    /* 初始化任务堆栈 - 模拟异常返回时的堆栈帧 */
    uint32_t* stack_top = &task->stack[STACK_SIZE - 16];
    
    stack_top[0] = 0x01000000;       /* xPSR - 默认Thumb状态 */
    stack_top[1] = (uint32_t)func;   /* PC - 任务入口地址 */
    stack_top[2] = (uint32_t)arg;    /* LR - 任务参数 */
    stack_top[3] = 0;                /* R12 */
    stack_top[4] = 0;                /* R3 */
    stack_top[5] = 0;                /* R2 */
    stack_top[6] = 0;                /* R1 */
    stack_top[7] = 0;                /* R0 */
    stack_top[8] = 0;                /* R11 */
    stack_top[9] = 0;                /* R10 */
    stack_top[10] = 0;               /* R9 */
    stack_top[11] = 0;               /* R8 */
    stack_top[12] = 0;               /* R7 */
    stack_top[13] = 0;               /* R6 */
    stack_top[14] = 0;               /* R5 */
    stack_top[15] = 0;               /* R4 */
    
    task->stack_ptr = stack_top;     /* 设置初始堆栈指针 */
    
    scheduler.tasks[scheduler.task_count] = task;
    scheduler.task_count++;
    
    return task;
}

/* 挂起指定任务 */
void task_suspend(task_t* task) {
    if (task) {
        task->state = TASK_SUSPENDED;  /* 将任务状态设置为挂起 */
    }
}

/* 恢复挂起的任务 */
void task_resume(task_t* task) {
    if (task && task->state == TASK_SUSPENDED) {
        task->state = TASK_READY;  /* 将任务状态恢复为就绪 */
    }
}

/* 删除任务 */
void task_delete(task_t* task) {
    if (!task) return;
    
    /* 在任务数组中查找并移除指定任务 */
    for (uint8_t i = 0; i < scheduler.task_count; i++) {
        if (scheduler.tasks[i] == task) {
            for (uint8_t j = i; j < scheduler.task_count - 1; j++) {
                scheduler.tasks[j] = scheduler.tasks[j + 1];  /* 前移后续任务 */
            }
            scheduler.task_count--;  /* 减少任务计数 */
            break;
        }
    }
}

/* 查找最高优先级的就绪任务 */
task_t* find_highest_priority_task(void) {
    task_t* highest_priority_task = NULL;
    uint32_t highest_priority = MAX_PRIORITY + 1;  /* 初始化为比最大优先级更大的值 */
    
    /* 遍历所有任务，找到优先级最高的就绪任务 */
    for (uint8_t i = 0; i < scheduler.task_count; i++) {
        task_t* task = scheduler.tasks[i];
        if (task->state == TASK_READY && task->priority < highest_priority) {
            highest_priority = task->priority;
            highest_priority_task = task;
        }
    }
    
    return highest_priority_task;
}

/* 调度器核心函数 - 执行任务切换 */
void rtos_schedule(void) {
    task_t* next_task = find_highest_priority_task();  /* 找到最高优先级的就绪任务 */
    
    if (next_task && next_task != scheduler.current_task) {
        if (scheduler.current_task) {
            scheduler.current_task->state = TASK_READY;  /* 当前任务状态改为就绪 */
        }
        
        next_task->state = TASK_RUNNING;      /* 新任务状态改为运行 */
        scheduler.current_task = next_task;   /* 更新当前运行任务 */
        
        __asm volatile(
            "cpsid i\n"  /* 禁用全局中断 */
            "svc 0\n"    /* 触发SVC异常，执行上下文切换 */
        );
    }
}

/* PendSV中断处理函数 - 执行实际的上下文切换 */
void __attribute__((naked)) pend_sv_handler(void) {
    __asm volatile(
        "cpsid i\n"                     /* 禁用中断 */
        "mrs r0, psp\n"                 /* 读取进程堆栈指针 */
        "stmdb r0!, {r4-r11}\n"         /* 保存当前任务的寄存器R4-R11到堆栈 */
        "str r0, [%0]\n"               /* 保存更新后的堆栈指针到当前任务的stack_ptr */
        
        "bl find_highest_priority_task\n"  /* 调用C函数找到下一个要运行的任务 */
        "str r0, [%1]\n"               /* 保存新任务指针到current_task */
        
        "ldr r0, [r0]\n"               /* 加载新任务的堆栈指针 */
        "ldmia r0!, {r4-r11}\n"         /* 从新任务的堆栈恢复寄存器R4-R11 */
        "msr psp, r0\n"                /* 更新进程堆栈指针 */
        
        "cpsie i\n"                    /* 启用中断 */
        "bx lr\n"                      /* 返回，自动恢复剩余的寄存器 */
        :
        : "r" (&scheduler.current_task->stack_ptr), "r" (&scheduler.current_task)
    );
}

/* SVC中断处理函数 - 处理系统调用 */
void __attribute__((naked)) svc_handler(void) {
    __asm volatile(
        "tst lr, #4\n"                 /* 检查使用的是主堆栈还是进程堆栈 */
        "ite eq\n"                     /* if-then-else指令 */
        "mrseq r0, msp\n"              /* 如果使用主堆栈，读取MSP */
        "mrsne r0, psp\n"              /* 如果使用进程堆栈，读取PSP */
        
        "ldr r1, [r0, #24]\n"         /* 从堆栈中加载PC值（SVC调用地址） */
        "ldrb r1, [r1, #-2]\n"        /* 读取SVC指令的操作数 */
        
        "cmp r1, #0\n"                /* 检查SVC编号 */
        "beq schedule\n"              /* 如果SVC 0，跳转到调度处理 */
        
        "bx lr\n"                     /* 返回，不处理其他SVC调用 */
        
        "schedule:\n"                 /* 调度处理标签 */
        "bl rtos_schedule\n"          /* 调用调度器函数 */
        "bx lr\n"                     /* 返回 */
    );
}