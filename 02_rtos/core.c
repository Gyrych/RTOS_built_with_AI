#include "core.h"
#include <string.h>
#include "stm32f4xx.h"

scheduler_t scheduler;  /* 全局调度器实例 */

/* 在C侧暴露给汇编使用的全局指针（便于汇编读取，不要用硬编码偏移） */
volatile task_t * volatile pxCurrentTCB = NULL;
volatile task_t * volatile pxNextTCB = NULL;

/* 空闲任务 - 当没有其他任务运行时执行 */
static void idle_task(void* arg) {
    while (1) {
        __asm("wfi");  /* 等待中断指令，降低功耗 */
    }
}

/* 任务退出处理函数 - 当任务函数返回时调用 */
void prvTaskExitError(void) {
    RTOS_DEBUG_PRINT(1, "ERROR: Task function returned unexpectedly!");
    RTOS_DEBUG_PRINT(1, "This should never happen in a properly designed RTOS task.");

    /* 进入死循环，防止系统崩溃 */
    while (1) {
        __asm("wfi");
    }
}

/* 进入临界区 - 使用BASEPRI屏蔽低优先级中断 */
void rtos_enter_critical(void) {
    __asm volatile(
        "mov r0, %0 \n"
        "msr basepri, r0 \n"
        "isb \n"
        :
        : "i" (configMAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS))
        : "r0"
    );
}

/* 退出临界区 - 清除BASEPRI */
void rtos_exit_critical(void) {
    __asm volatile(
        "mov r0, #0 \n"
        "msr basepri, r0 \n"
        "isb \n"
        :
        :
        : "r0"
    );
}

/* RTOS初始化函数 */
void rtos_init(void) {
    RTOS_DEBUG_PRINT(1, "=== RTOS Initialization Started ===");

    memset(&scheduler, 0, sizeof(scheduler_t));  /* 清空调度器结构体 */
    RTOS_DEBUG_PRINT(2, "Scheduler structure cleared");

    /* 初始化全局指针 */
    pxCurrentTCB = NULL;
    pxNextTCB = NULL;
    RTOS_DEBUG_PRINT(2, "Global pointers initialized");

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
    pxCurrentTCB = first_task;  /* 同步更新全局指针 */
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

    /* 初始化任务堆栈 - 按照FreeRTOS风格初始化 */
    /* 确保堆栈8字节对齐 */
    uint32_t *sp = &(task->stack[STACK_SIZE]);  /* 指向栈数组"末端"（one-past-end） */
    sp = (uint32_t *)((uint32_t)sp & ~0x7);     /* 8字节对齐 */

    /* 为R4-R11（手动保存区）分配空间 */
    sp -= 8;
    for (int i = 0; i < 8; i++) {
        sp[i] = 0;  /* R4..R11初值 */
    }

    /* 为硬件自动保存区分配空间并初始化（xPSR..R0） */
    sp -= 8;
    sp[0] = 0x01000000;                           /* xPSR */
    sp[1] = ((uint32_t)func) | 0x1;              /* PC (确保Thumb位=1) */
    sp[2] = (uint32_t)prvTaskExitError;          /* LR: 指向任务退出处理（安全） */
    sp[3] = 0;                                   /* R12 */
    sp[4] = 0;                                   /* R3 */
    sp[5] = 0;                                   /* R2 */
    sp[6] = 0;                                   /* R1 */
    sp[7] = (uint32_t)arg;                       /* R0 - 任务参数 */

    /* 现在sp指向xPSR；但PendSV的保存会在运行时再把R4..R11 push到栈上，
       因为我们提前为R4..R11分配了空间，所以pxStackPointer应该指向R4的位置 */
    /* R4的位置在当前sp + 8 (因为我们先放了8 words给xPSR..R0，然后更早前再放了R4..R11) */
    uint32_t *stack_ptr_for_restore = sp + 8;    /* 指向R4的位置 */
    task->stack_ptr = stack_ptr_for_restore;

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

    if (next_task && next_task != pxCurrentTCB) {
        RTOS_DEBUG_PRINT(1, "Context switch required");

        if (pxCurrentTCB) {
            RTOS_DEBUG_PRINT_TASK(2, pxCurrentTCB, "Current task -> READY");
            pxCurrentTCB->state = TASK_READY;  /* 当前任务状态改为就绪 */
        }

        RTOS_DEBUG_PRINT_TASK(2, next_task, "Next task -> RUNNING");
        next_task->state = TASK_RUNNING;      /* 新任务状态改为运行 */

        /* 不在这里改变pxCurrentTCB，改为设置pxNextTCB并触发PendSV */
        pxNextTCB = next_task;
        scheduler.current_task = next_task;   /* 保持scheduler同步 */

        RTOS_DEBUG_PRINT(2, "Triggering PendSV for context switch");
        /* 触发PendSV中断进行上下文切换 */
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    } else if (next_task == pxCurrentTCB) {
        RTOS_DEBUG_PRINT(2, "No context switch needed - same task");
    } else {
        RTOS_DEBUG_PRINT(1, "WARNING: No ready tasks found for scheduling");
    }
}

/* PendSV中断处理函数 - 执行实际的上下文切换 */
void __attribute__((naked)) pend_sv_handler(void) {
    __asm volatile(
        "MRS r0, PSP \n"               /* r0 = current PSP */
        "ISB \n"                       /* 内存屏障 */

        /* 保存r4-r11到当前任务堆栈 */
        "STMDB r0!, {r4-r11} \n"       /* r0更新为新的PSP */

        /* 将更新后的PSP存入pxCurrentTCB->stack_ptr */
        "LDR r3, =pxCurrentTCB \n"
        "LDR r2, [r3] \n"              /* r2 = pxCurrentTCB */
        "STR r0, [r2] \n"              /* pxCurrentTCB->stack_ptr = r0 */

        /* 用pxNextTCB作为下一个要运行的任务 */
        "LDR r3, =pxNextTCB \n"
        "LDR r2, [r3] \n"              /* r2 = pxNextTCB */

        /* 从新任务的TCB中加载其stack_ptr */
        "LDR r0, [r2] \n"              /* r0 = pxNextTCB->stack_ptr */

        /* 恢复r4-r11 */
        "LDMIA r0!, {r4-r11} \n"
        "MSR PSP, r0 \n"
        "ISB \n"                       /* 内存屏障 */

        /* 更新pxCurrentTCB = pxNextTCB（在全局C变量中） */
        "LDR r3, =pxCurrentTCB \n"
        "LDR r1, =pxNextTCB \n"
        "LDR r0, [r1] \n"
        "STR r0, [r3] \n"

        "BX lr \n"                     /* 返回，自动恢复剩余的寄存器 */
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