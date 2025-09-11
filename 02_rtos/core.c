#include "core.h"
#include <string.h>
#include "stm32f4xx.h"

scheduler_t scheduler;  /* 全局调度器实例 */

/* 在C侧暴露给汇编使用的全局指针（便于汇编读取，不要用硬编码偏移） */
volatile task_t * volatile pxCurrentTCB = NULL;
volatile task_t * volatile pxNextTCB = NULL;
/* 提供给汇编用的调度器 current_task 指针，避免在异常内调用 C 函数 */
volatile task_t * volatile * const pxSchedulerCurrentTaskPtr = &scheduler.current_task;

/* 注：为保持异常路径最小化，异常现场不进行繁重打印 */

/* 注意：异常上下文中禁止使用 printf。为避免破坏异常现场，调试请在线程态进行。*/

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

    /* 首任务将通过 SVC 返回到线程态运行。*/

    RTOS_DEBUG_PRINT(1, "=== Starting first task execution ===");

    /* 通过 SVC 1 启动首任务：在 SVC 中设置 PSP 并异常返回至线程态 */
    __asm volatile("svc %0" :: "I"(SVC_START_FIRST_TASK));

    /* 验证性打印：正常情况下不会执行到这里；若出现则表明 SVC 未通过 EXC_RETURN 进入首任务 */
    printf("[FATAL] Returned from SVC start — should never happen!\r\n");
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

    /* 初始化任务堆栈 - 确保与 svc_start_first/PendSV 恢复序列对齐 */
    /* 确保堆栈8字节对齐 */
    uint32_t *sp = &(task->stack[STACK_SIZE]);  /* 指向栈数组"末端"（one-past-end） */
    sp = (uint32_t *)((uint32_t)sp & ~0x7);     /* 8字节对齐 */

    /* 先为硬件自动保存区（异常返回时弹出）分配空间并初始化：
       内存从低到高依次为 R0,R1,R2,R3,R12,LR,PC,xPSR */
    sp -= 8;                    /* 预留 8 words 硬件帧空间 */
    uint32_t *hw_frame = sp;    /* 指向硬件帧起始（R0 位置） */
    hw_frame[0] = (uint32_t)arg;                   /* R0 - 任务参数 */
    hw_frame[1] = 0;                               /* R1 */
    hw_frame[2] = 0;                               /* R2 */
    hw_frame[3] = 0;                               /* R3 */
    hw_frame[4] = 0;                               /* R12 */
    hw_frame[5] = (uint32_t)prvTaskExitError;      /* LR: 任务返回时兜底 */
    hw_frame[6] = ((uint32_t)func) | 0x1;          /* PC (确保Thumb位=1) */
    hw_frame[7] = 0x01000000;                       /* xPSR (T 位=1) */

    /* 再为R4-R11（手动保存区）分配空间并清零；恢复时将先弹出R4-R11，然后 PSP 指向硬件帧 */
    sp -= 8;                    /* 预留 8 words 给 R4..R11 */
    for (int i = 0; i < 8; i++) {
        sp[i] = 0;  /* R4..R11 初值 */
    }

    /* 此时 sp 指向 R4 起始位置；与 ldmia r0!, {r4-r11} 的恢复序列匹配 */
    task->stack_ptr = sp;

    RTOS_DEBUG_PRINT(3, "Stack initialized: stack_ptr=%p, stack_size=%d",
                    task->stack_ptr, STACK_SIZE);

    scheduler.tasks[scheduler.task_count] = task;
    scheduler.task_count++;

    RTOS_DEBUG_PRINT(1, "Task created successfully: task_count=%d", scheduler.task_count);
    RTOS_DEBUG_PRINT_TASK(2, task, "Task created and ready");

    /* 任务已创建为 READY 态。*/

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
    /* 协作式模型下，对非 SUSPENDED 任务恢复并非错误，这里仅在高调试级别提示。*/
    if (task && task->state == TASK_SUSPENDED) {
        RTOS_DEBUG_PRINT_TASK(2, task, "Resuming task");
        task->state = TASK_READY;  /* 将任务状态恢复为就绪 */
        RTOS_DEBUG_PRINT_TASK(2, task, "Task resumed");
    } else if (task) {
        RTOS_DEBUG_PRINT_TASK(3, task, "Attempting to resume non-suspended task (no-op)");
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
    /* 线程态直接进行一次调度决策，必要时置位 PendSV 触发上下文切换 */
    int need = rtos_schedule_decide_next();
    if (need) {
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    }
}

/* 进行一次调度决策，返回是否需要上下文切换（1=需要，0=不需要） */
int rtos_schedule_decide_next(void) {
    RTOS_DEBUG_PRINT(2, "=== Task Scheduling Requested ===");

    task_t* next_task = find_highest_priority_task();  /* 找到最高优先级的就绪任务 */

    if (next_task && next_task != pxCurrentTCB) {
        RTOS_DEBUG_PRINT(1, "Context switch required");

        /* 仅当当前任务仍处于 RUNNING 时，才将其置回 READY。
           若任务已在上层逻辑（如 Delay/阻塞）中改为 SUSPENDED，则保持不变。 */
        if (pxCurrentTCB && pxCurrentTCB->state == TASK_RUNNING) {
            RTOS_DEBUG_PRINT_TASK(2, pxCurrentTCB, "Current task -> READY");
            pxCurrentTCB->state = TASK_READY;
        }

        RTOS_DEBUG_PRINT_TASK(2, next_task, "Next task -> RUNNING");
        next_task->state = TASK_RUNNING;      /* 新任务状态改为运行 */

        /* 设置下一个任务，由 PendSV 完成实际切换与 pxCurrentTCB 更新 */
        pxNextTCB = next_task;
        return 1;
    } else if (next_task == pxCurrentTCB) {
        RTOS_DEBUG_PRINT(2, "No context switch needed - same task");
        return 0;
    } else {
        RTOS_DEBUG_PRINT(1, "WARNING: No ready tasks found for scheduling");
        return 0;
    }
}

/* 在中断环境中请求上下文切换：仅做决策并置位 PendSV */
void rtos_request_context_switch_from_isr(void) {
    rtos_enter_critical();
    int need = rtos_schedule_decide_next();
    if (need) {
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    }
    rtos_exit_critical();
}

/* PendSV中断处理函数 - 执行实际的上下文切换 */
void __attribute__((naked)) pend_sv_handler(void) {
    __asm volatile(
        "MRS r0, PSP \n"               /* r0 = current PSP */
        "ISB \n"                       /* 内存屏障 */

        /* 保存r4-r11到当前任务堆栈 */
        "STMDB r0!, {r4-r11} \n"       /* r0更新为新的PSP */

        /* 将更新后的PSP存入pxCurrentTCB->stack_ptr（偏移0） */
        "LDR r3, =pxCurrentTCB \n"
        "LDR r2, [r3] \n"              /* r2 = pxCurrentTCB */
        "STR r0, [r2, #0] \n"

        /* 读取pxNextTCB并加载其stack_ptr */
        "LDR r3, =pxNextTCB \n"
        "LDR r2, [r3] \n"              /* r2 = pxNextTCB */
        "LDR r0, [r2, #0] \n"          /* r0 = pxNextTCB->stack_ptr */
        "MOV r12, r2 \n"               /* 保存 next_tcb 指针到 r12，避免后续调试覆写 */

        /* 加载下一个任务栈指针 */

        /* 恢复r4-r11并更新PSP */
        "LDMIA r0!, {r4-r11} \n"
        "MSR PSP, r0 \n"
        "ISB \n"                       /* 内存屏障 */

        /* 更新 PSP 后直接返回 */

        /* 更新pxCurrentTCB = pxNextTCB (使用 r12) */
        "LDR r3, =pxCurrentTCB \n"
        "STR r12, [r3] \n"

        /* 同步 scheduler.current_task = pxCurrentTCB (即 r12) */
        "LDR r3, =pxSchedulerCurrentTaskPtr \n"
        "LDR r3, [r3] \n"              /* r3 = &scheduler.current_task */
        "STR r12, [r3] \n"

        /* 固定使用 EXC_RETURN=0xFFFFFFFD 返回到线程态并使用 PSP */
        "LDR r0, =0xFFFFFFFD \n"
        "BX r0 \n"
    );
}

/* SVC中断处理函数 - 处理系统调用 */
void __attribute__((naked)) svc_handler(void) {
    __asm volatile(
        /* 解码SVC号 */
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "ldr r1, [r0, #24]\n"         /* 堆栈中的返回PC */
        "ldrb r1, [r1, #-2]\n"        /* SVC立即数 */

        /* 分派到SVC 0/1 */
        "cmp r1, #0\n"
        "beq svc_yield\n"
        "cmp r1, #1\n"
        "beq svc_start_first\n"
        "bx lr\n"

        "svc_yield:\n"
        "bl rtos_schedule_decide_next\n"  /* r0=是否需要切换 */
        "cmp r0, #0\n"
        "beq svc_exit\n"
        "ldr r0, =0xE000ED04\n"
        "ldr r1, =0x10000000\n"
        "str r1, [r0]\n"               /* 触发PendSV */
        "b svc_exit\n"

        "svc_start_first:\n"
        /* 从pxCurrentTCB->stack_ptr恢复R4-R11，设置PSP并异常返回到线程态 */
        "ldr r3, =pxCurrentTCB\n"
        "ldr r3, [r3]\n"               /* r3 = pxCurrentTCB */
        "ldr r0, [r3, #0]\n"           /* r0 = stack_ptr */
        "ldmia r0!, {r4-r11}\n"
        "msr psp, r0\n"
        "isb\n"
        "ldr r0, =0xFFFFFFFD\n"        /* 返回到线程模式，使用PSP */
        "bx r0\n"

        "svc_exit:\n"
        "bx lr\n"
        :
        :
        : "r0", "r1", "r3"
    );
}

/* 在上下文切换完成后由PendSV调用，用于同步C层面的状态（如scheduler.current_task） */
void rtos_on_context_switch_completed(void) {
    scheduler.current_task = (task_t*)pxCurrentTCB;
}

/* 调试辅助函数实现 */

/* 线程态调试：如需追加打印，请保持在高调试级别并避免异常上下文。*/

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