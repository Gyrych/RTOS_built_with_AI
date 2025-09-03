#include "core.h"
#include <string.h>
#include "stm32f4xx.h"

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
    
    /* 找到第一个要运行的任务 */
    task_t* first_task = find_highest_priority_task();
    if (first_task == NULL) {
        return;  /* 没有就绪任务 */
    }
    
    /* 设置当前任务 */
    scheduler.current_task = first_task;
    first_task->state = TASK_RUNNING;
    
    /* 直接调用第一个任务，不使用复杂的上下文切换 */
    first_task->task_func(first_task->arg);
}

/* 创建新任务 */
task_t* task_create(void (*func)(void*), void* arg, uint32_t priority) {
    if (scheduler.task_count >= MAX_TASKS || priority > MAX_PRIORITY) {
        return NULL;  /* 任务数量或优先级超出限制 */
    }
    
    /* 分配任务控制块内存 */
    static task_t task_pool[MAX_TASKS];
    task_t* task = &task_pool[scheduler.task_count];
    
    task->task_func = func;      /* 设置任务函数 */
    task->arg = arg;             /* 设置任务参数 */
    task->priority = priority;   /* 设置任务优先级 */
    task->state = TASK_READY;    /* 设置任务状态为就绪 */
    
    /* 初始化任务堆栈 - 模拟异常返回时的堆栈帧 */
    /* 确保堆栈8字节对齐 */
    uint32_t* stack_top = &task->stack[STACK_SIZE - 16];
    stack_top = (uint32_t*)((uint32_t)stack_top & ~0x7);  /* 8字节对齐 */
    
    /* 按照Cortex-M4异常返回时的堆栈帧格式初始化 */
    /* 注意：Cortex-M4异常返回时自动恢复的寄存器顺序 */
    /* 堆栈帧格式：xPSR, PC, LR, R12, R3, R2, R1, R0, R11, R10, R9, R8, R7, R6, R5, R4 */
    stack_top[0] = 0x01000000;       /* xPSR - Thumb状态，无异常号 */
    stack_top[1] = (uint32_t)func;   /* PC - 任务入口地址 */
    stack_top[2] = 0xFFFFFFFD;       /* LR - 返回地址，使用特殊值表示从异常返回 */
    stack_top[3] = 0;                /* R12 */
    stack_top[4] = 0;                /* R3 */
    stack_top[5] = 0;                /* R2 */
    stack_top[6] = 0;                /* R1 */
    stack_top[7] = (uint32_t)arg;    /* R0 - 任务参数 */
    stack_top[8] = 0;                /* R11 */
    stack_top[9] = 0;                /* R10 */
    stack_top[10] = 0;               /* R9 */
    stack_top[11] = 0;               /* R8 */
    stack_top[12] = 0;               /* R7 */
    stack_top[13] = 0;               /* R6 */
    stack_top[14] = 0;               /* R5 */
    stack_top[15] = 0;               /* R4 */
    
    /* 堆栈指针应该指向堆栈帧的顶部（第一个寄存器） */
    task->stack_ptr = stack_top;
    
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
        
        /* 触发PendSV中断进行上下文切换 */
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}

/* PendSV中断处理函数 - 执行实际的上下文切换 */
void __attribute__((naked)) pend_sv_handler(void) {
    __asm volatile(
        "cpsid i\n"                     /* 禁用中断 */
        "mrs r0, psp\n"                 /* 读取进程堆栈指针 */
        "stmdb r0!, {r4-r11}\n"         /* 保存当前任务的寄存器R4-R11到堆栈 */
        
        /* 保存当前任务的堆栈指针 */
        "ldr r1, =scheduler\n"          /* 加载调度器地址 */
        "ldr r2, [r1, #8]\n"            /* 加载current_task指针 */
        "str r0, [r2, #8]\n"            /* 保存堆栈指针到当前任务的stack_ptr */
        
        /* 查找下一个要运行的任务 - 使用内联汇编实现 */
        "ldr r3, [r1, #4]\n"            /* 加载task_count */
        "mov r4, #0\n"                  /* 初始化循环计数器 */
        "mov r5, #32\n"                 /* 最大优先级值 */
        "mov r6, #0\n"                  /* 最高优先级任务指针 */
        
        "find_loop:\n"
        "cmp r4, r3\n"                  /* 检查是否遍历完所有任务 */
        "bge find_done\n"               /* 如果遍历完，跳转到完成 */
        
        "ldr r7, [r1, r4, lsl #2]\n"    /* 加载tasks[i] */
        "ldrb r8, [r7, #20]\n"          /* 加载任务状态 */
        "cmp r8, #0\n"                  /* 检查是否为TASK_READY */
        "bne find_next\n"               /* 如果不是就绪状态，跳过 */
        
        "ldr r8, [r7, #16]\n"           /* 加载任务优先级 */
        "cmp r8, r5\n"                  /* 比较优先级 */
        "bge find_next\n"               /* 如果优先级不更高，跳过 */
        
        "mov r5, r8\n"                  /* 更新最高优先级 */
        "mov r6, r7\n"                  /* 更新最高优先级任务指针 */
        
        "find_next:\n"
        "add r4, r4, #1\n"              /* 增加循环计数器 */
        "b find_loop\n"                 /* 继续循环 */
        
        "find_done:\n"
        "str r6, [r1, #8]\n"            /* 保存新任务指针到current_task */
        
        /* 恢复新任务的上下文 */
        "ldr r0, [r6, #8]\n"            /* 加载新任务的堆栈指针 */
        "ldmia r0!, {r4-r11}\n"         /* 从新任务的堆栈恢复寄存器R4-R11 */
        "msr psp, r0\n"                /* 更新进程堆栈指针 */
        
        "cpsie i\n"                    /* 启用中断 */
        "bx lr\n"                      /* 返回，自动恢复剩余的寄存器 */
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
        "ldr r0, =0xE000ED04\n"       /* 加载ICSR寄存器地址 */
        "ldr r1, =0x10000000\n"       /* PendSV挂起位 */
        "str r1, [r0]\n"              /* 触发PendSV中断 */
        "bx lr\n"                     /* 返回 */
    );
}