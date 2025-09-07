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
    RTOS_DEBUG_PRINT(1, "=== RTOS Initialization Started ===");
    
    memset(&scheduler, 0, sizeof(scheduler_t));  /* 清空调度器结构体 */
    RTOS_DEBUG_PRINT(2, "Scheduler structure cleared");
    
    task_create(idle_task, NULL, MAX_PRIORITY);  /* 创建空闲任务 */
    RTOS_DEBUG_PRINT(1, "Idle task created with priority %d", MAX_PRIORITY);
    
    RTOS_DEBUG_PRINT(1, "=== RTOS Initialization Completed ===");
    RTOS_DEBUG_PRINT(2, "Total tasks: %d", scheduler.task_count);
}

/* 启动RTOS调度 */
void rtos_start(void) {
    RTOS_DEBUG_PRINT(1, "=== RTOS Starting ===");
    
    if (scheduler.task_count == 0) {
        RTOS_DEBUG_PRINT(1, "ERROR: No tasks available for scheduling");
        return;  /* 没有任务可调度 */
    }
    
    RTOS_DEBUG_PRINT(2, "Total tasks available: %d", scheduler.task_count);
    
    /* 找到第一个要运行的任务 */
    task_t* first_task = find_highest_priority_task();
    if (first_task == NULL) {
        RTOS_DEBUG_PRINT(1, "ERROR: No ready tasks found");
        return;  /* 没有就绪任务 */
    }
    
    RTOS_DEBUG_PRINT_TASK(1, first_task, "Selected as first task to run");
    
    /* 设置当前任务 */
    scheduler.current_task = first_task;
    first_task->state = TASK_RUNNING;
    
    RTOS_DEBUG_PRINT(1, "=== Starting first task execution ===");
    
    /* 直接调用第一个任务，不使用复杂的上下文切换 */
    first_task->task_func(first_task->arg);
}

/* 创建新任务 */
task_t* task_create(void (*func)(void*), void* arg, uint32_t priority) {
    RTOS_DEBUG_PRINT(2, "Creating new task: func=%p, arg=%p, priority=%d", func, arg, priority);
    
    if (scheduler.task_count >= MAX_TASKS || priority > MAX_PRIORITY) {
        RTOS_DEBUG_PRINT(1, "ERROR: Task creation failed - task_count=%d, priority=%d", 
                        scheduler.task_count, priority);
        return NULL;  /* 任务数量或优先级超出限制 */
    }
    
    /* 分配任务控制块内存 */
    static task_t task_pool[MAX_TASKS];
    task_t* task = &task_pool[scheduler.task_count];
    
    task->task_func = func;      /* 设置任务函数 */
    task->arg = arg;             /* 设置任务参数 */
    task->priority = priority;   /* 设置任务优先级 */
    task->state = TASK_READY;    /* 设置任务状态为就绪 */
    
    RTOS_DEBUG_PRINT(2, "Task control block allocated at %p", task);
    
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
    
    RTOS_DEBUG_PRINT(3, "Stack initialized: stack_ptr=%p, stack_size=%d", 
                    task->stack_ptr, STACK_SIZE);
    
    scheduler.tasks[scheduler.task_count] = task;
    scheduler.task_count++;
    
    RTOS_DEBUG_PRINT(1, "Task created successfully: task_count=%d", scheduler.task_count);
    RTOS_DEBUG_PRINT_TASK(2, task, "Task created and ready");
    
    return task;
}

/* 挂起指定任务 */
void task_suspend(task_t* task) {
    if (task) {
        RTOS_DEBUG_PRINT_TASK(2, task, "Suspending task");
        task->state = TASK_SUSPENDED;  /* 将任务状态设置为挂起 */
        RTOS_DEBUG_PRINT_TASK(2, task, "Task suspended");
    } else {
        RTOS_DEBUG_PRINT(1, "ERROR: Attempting to suspend NULL task");
    }
}

/* 恢复挂起的任务 */
void task_resume(task_t* task) {
    if (task && task->state == TASK_SUSPENDED) {
        RTOS_DEBUG_PRINT_TASK(2, task, "Resuming task");
        task->state = TASK_READY;  /* 将任务状态恢复为就绪 */
        RTOS_DEBUG_PRINT_TASK(2, task, "Task resumed");
    } else if (task) {
        RTOS_DEBUG_PRINT_TASK(1, task, "WARNING: Attempting to resume non-suspended task");
    } else {
        RTOS_DEBUG_PRINT(1, "ERROR: Attempting to resume NULL task");
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
    
    RTOS_DEBUG_PRINT(3, "Searching for highest priority ready task...");
    
    /* 遍历所有任务，找到优先级最高的就绪任务 */
    for (uint8_t i = 0; i < scheduler.task_count; i++) {
        task_t* task = scheduler.tasks[i];
        RTOS_DEBUG_PRINT(3, "Checking task[%d]: %p, state=%d, priority=%d", 
                        i, task, task->state, task->priority);
        
        if (task->state == TASK_READY && task->priority < highest_priority) {
            highest_priority = task->priority;
            highest_priority_task = task;
            RTOS_DEBUG_PRINT(3, "Found better candidate: priority=%d", task->priority);
        }
    }
    
    if (highest_priority_task) {
        RTOS_DEBUG_PRINT_TASK(2, highest_priority_task, "Selected as highest priority ready task");
    } else {
        RTOS_DEBUG_PRINT(2, "No ready tasks found");
    }
    
    return highest_priority_task;
}

/* 调度器核心函数 - 执行任务切换 */
void rtos_schedule(void) {
    RTOS_DEBUG_PRINT(2, "=== Task Scheduling Requested ===");
    
    task_t* next_task = find_highest_priority_task();  /* 找到最高优先级的就绪任务 */
    
    if (next_task && next_task != scheduler.current_task) {
        RTOS_DEBUG_PRINT(1, "Context switch required");
        
        if (scheduler.current_task) {
            RTOS_DEBUG_PRINT_TASK(2, scheduler.current_task, "Current task -> READY");
            scheduler.current_task->state = TASK_READY;  /* 当前任务状态改为就绪 */
        }
        
        RTOS_DEBUG_PRINT_TASK(2, next_task, "Next task -> RUNNING");
        next_task->state = TASK_RUNNING;      /* 新任务状态改为运行 */
        scheduler.current_task = next_task;   /* 更新当前运行任务 */
        
        RTOS_DEBUG_PRINT(2, "Triggering PendSV for context switch");
        /* 触发PendSV中断进行上下文切换 */
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    } else if (next_task == scheduler.current_task) {
        RTOS_DEBUG_PRINT(2, "No context switch needed - same task");
    } else {
        RTOS_DEBUG_PRINT(1, "WARNING: No ready tasks found for scheduling");
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
        "ldr r2, [r1, #132]\n"          /* 加载current_task指针 (偏移量132) */
        "str r0, [r2, #8]\n"            /* 保存堆栈指针到当前任务的stack_ptr */
        
        /* 注意：此时current_task已经指向下一个要运行的任务 */
        /* 直接使用current_task作为新任务指针 */
        "mov r6, r2\n"                  /* 将current_task指针复制到r6 */
        
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

/* 调试辅助函数实现 */

/**
 * @brief  获取任务状态名称
 * @param  state: 任务状态
 * @retval 状态名称字符串
 */
const char* rtos_debug_get_state_name(uint8_t state) {
    switch (state) {
        case TASK_READY:    return "READY";
        case TASK_RUNNING:  return "RUNNING";
        case TASK_SUSPENDED: return "SUSPENDED";
        default:            return "UNKNOWN";
    }
}

/**
 * @brief  打印调度器信息
 * @param  None
 * @retval None
 */
void rtos_debug_print_scheduler_info(void) {
    RTOS_DEBUG_PRINT(1, "=== Scheduler Information ===");
    RTOS_DEBUG_PRINT(1, "Total tasks: %d", scheduler.task_count);
    RTOS_DEBUG_PRINT(1, "Current task: %p", scheduler.current_task);
    
    if (scheduler.current_task) {
        RTOS_DEBUG_PRINT_TASK(1, scheduler.current_task, "Current running task");
    }
    
    RTOS_DEBUG_PRINT(1, "Task list:");
    for (uint8_t i = 0; i < scheduler.task_count; i++) {
        task_t* task = scheduler.tasks[i];
        if (task) {
            RTOS_DEBUG_PRINT(1, "  [%d] %p: Priority=%d, State=%s", 
                            i, task, task->priority, rtos_debug_get_state_name(task->state));
        }
    }
    RTOS_DEBUG_PRINT(1, "=== End Scheduler Information ===");
}

/**
 * @brief  打印任务详细信息
 * @param  task: 任务指针
 * @retval None
 */
void rtos_debug_print_task_info(task_t* task) {
    if (!task) {
        RTOS_DEBUG_PRINT(1, "Task info: NULL task");
        return;
    }
    
    RTOS_DEBUG_PRINT(1, "=== Task Information ===");
    RTOS_DEBUG_PRINT(1, "Task address: %p", task);
    RTOS_DEBUG_PRINT(1, "Task function: %p", task->task_func);
    RTOS_DEBUG_PRINT(1, "Task argument: %p", task->arg);
    RTOS_DEBUG_PRINT(1, "Priority: %d", task->priority);
    RTOS_DEBUG_PRINT(1, "State: %s", rtos_debug_get_state_name(task->state));
    RTOS_DEBUG_PRINT(1, "Stack pointer: %p", task->stack_ptr);
    RTOS_DEBUG_PRINT(1, "Stack base: %p", task->stack);
    RTOS_DEBUG_PRINT(1, "Stack size: %d bytes", STACK_SIZE * sizeof(uint32_t));
    RTOS_DEBUG_PRINT(1, "=== End Task Information ===");
}

/**
 * @brief  打印堆栈使用情况
 * @param  task: 任务指针
 * @retval None
 */
void rtos_debug_print_stack_usage(task_t* task) {
    if (!task) {
        RTOS_DEBUG_PRINT(1, "Stack usage: NULL task");
        return;
    }
    
    /* 计算堆栈使用情况 */
    uint32_t stack_base = (uint32_t)task->stack;
    uint32_t stack_top = (uint32_t)task->stack_ptr;
    uint32_t stack_size = STACK_SIZE * sizeof(uint32_t);
    uint32_t used_bytes = stack_base + stack_size - stack_top;
    uint32_t used_percent = (used_bytes * 100) / stack_size;
    
    RTOS_DEBUG_PRINT(1, "=== Stack Usage for Task %p ===", task);
    RTOS_DEBUG_PRINT(1, "Stack base: 0x%08X", stack_base);
    RTOS_DEBUG_PRINT(1, "Stack top:  0x%08X", stack_top);
    RTOS_DEBUG_PRINT(1, "Stack size: %d bytes", stack_size);
    RTOS_DEBUG_PRINT(1, "Used:       %d bytes (%d%%)", used_bytes, used_percent);
    RTOS_DEBUG_PRINT(1, "Free:       %d bytes (%d%%)", 
                    stack_size - used_bytes, 100 - used_percent);
    RTOS_DEBUG_PRINT(1, "=== End Stack Usage ===");
}