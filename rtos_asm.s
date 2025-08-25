/**
 * @file rtos_asm.s
 * @brief RTOS汇编代码实现(ARM Cortex-M4)
 */

.syntax unified
.cpu cortex-m4
.thumb

.extern rtos_system
.extern rtos_schedule

.section .text

/**
 * @brief 启动第一个任务
 */
.global rtos_hw_start_first_task
.type rtos_hw_start_first_task, %function
rtos_hw_start_first_task:
    /* 获取当前任务的栈指针 */
    ldr r0, =rtos_system
    ldr r0, [r0]                    /* r0 = rtos_system.current_task */
    ldr r0, [r0]                    /* r0 = current_task->stack_ptr */
    
    /* 恢复寄存器 */
    ldmia r0!, {r4-r11}             /* 恢复R4-R11 */
    msr psp, r0                     /* 设置进程栈指针 */
    isb
    
    /* 切换到进程栈 */
    mov r0, #0
    msr control, r0
    isb
    
    /* 恢复剩余寄存器并跳转到任务 */
    pop {r0-r3, r12, lr}
    pop {r1}                        /* 弹出PC到r1 */
    pop {r2}                        /* 弹出xPSR到r2 */
    msr psr_nzcvq, r2              /* 恢复状态寄存器 */
    bx r1                          /* 跳转到任务 */

/**
 * @brief PendSV中断服务程序(上下文切换)
 */
.global PendSV_Handler
.type PendSV_Handler, %function
PendSV_Handler:
    /* 关闭中断 */
    cpsid i
    
    /* 获取当前PSP */
    mrs r0, psp
    isb
    
    /* 保存当前任务寄存器 */
    stmdb r0!, {r4-r11}             /* 保存R4-R11到栈 */
    
    /* 保存栈指针到当前任务控制块 */
    ldr r1, =rtos_system
    ldr r2, [r1]                    /* r2 = rtos_system.current_task */
    str r0, [r2]                    /* current_task->stack_ptr = r0 */
    
    /* 调用调度器 */
    push {lr}
    bl rtos_schedule
    pop {lr}
    
    /* 获取新任务的栈指针 */
    ldr r1, =rtos_system
    ldr r2, [r1]                    /* r2 = rtos_system.current_task */
    ldr r0, [r2]                    /* r0 = current_task->stack_ptr */
    
    /* 恢复新任务寄存器 */
    ldmia r0!, {r4-r11}             /* 从栈恢复R4-R11 */
    
    /* 更新PSP */
    msr psp, r0
    isb
    
    /* 开启中断 */
    cpsie i
    
    /* 返回到新任务 */
    bx lr

/**
 * @brief SVC中断服务程序
 */
.global SVC_Handler
.type SVC_Handler, %function
SVC_Handler:
    /* 简单返回，暂不实现 */
    bx lr

.end