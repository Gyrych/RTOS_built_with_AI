# RTOS调试功能说明

## 概述

本文档说明了为RTOS系统添加的详细调试打印信息功能，用于排查任务调度过程是否成功，包括Hardfault中断的详细错误信息。

## 调试功能特性

### 1. 调试级别控制
- **调试级别 0**: 关闭所有调试输出
- **调试级别 1**: 基本调试信息（RTOS初始化、任务创建、调度等）
- **调试级别 2**: 详细调试信息（任务状态变化、中断处理等）
- **调试级别 3**: 完整调试信息（堆栈操作、寄存器状态等）

### 2. 调试信息分类

#### RTOS核心调试信息
- RTOS初始化过程
- 任务创建、挂起、恢复操作
- 任务调度过程
- 上下文切换跟踪
- 调度器状态信息

#### 中断处理调试信息
- SVC系统调用中断
- PendSV上下文切换中断
- TIM2定时器中断
- Hardfault硬件错误中断

#### 任务执行调试信息
- 任务函数开始和结束
- 任务状态变化
- 堆栈使用情况
- 任务优先级信息

## 调试输出示例

### 系统启动时的调试输出
```
=== Debug Configuration ===
RTOS Debug Enabled: YES
RTOS Debug Level: 2
Debug Levels: 0=Off, 1=Basic, 2=Detailed, 3=Complete
=============================

[RTOS-DEBUG] === RTOS Initialization Started ===
[RTOS-DEBUG] Scheduler structure cleared
[RTOS-DEBUG] Creating new task: func=0x08001234, arg=0x00000000, priority=31
[RTOS-DEBUG] Task control block allocated at 0x20000000
[RTOS-DEBUG] Task created successfully: task_count=1
[RTOS-DEBUG] Task@0x20000000(P31,S0) Task created and ready
[RTOS-DEBUG] === RTOS Initialization Completed ===
[RTOS-DEBUG] Total tasks: 1
```

### 任务调度时的调试输出
```
[RTOS-DEBUG] === Task Scheduling Requested ===
[RTOS-DEBUG] Searching for highest priority ready task...
[RTOS-DEBUG] Checking task[0]: 0x20000000, state=0, priority=1
[RTOS-DEBUG] Found better candidate: priority=1
[RTOS-DEBUG] Task@0x20000000(P1,S0) Selected as highest priority ready task
[RTOS-DEBUG] Context switch required
[RTOS-DEBUG] Task@0x20000000(P31,S0) Current task -> READY
[RTOS-DEBUG] Task@0x20000000(P1,S1) Next task -> RUNNING
[RTOS-DEBUG] Triggering PendSV for context switch
[PendSV] Context switch interrupt triggered
[PendSV] Context switch interrupt completed
```

### 任务执行时的调试输出
```
[GREEN-TASK] Green LED task started
[GREEN-TASK] Cycle 0: Turning ON green LED
[GREEN-TASK] Cycle 0: Turning OFF green LED
[GREEN-TASK] Cycle 0: Resuming red task and suspending self
[RTOS-DEBUG] Task@0x20000000(P1,S0) Resuming task
[RTOS-DEBUG] Task@0x20000000(P1,S0) Task resumed
[RTOS-DEBUG] Task@0x20000000(P1,S2) Suspending task
[RTOS-DEBUG] Task@0x20000000(P1,S2) Task suspended
```

### Hardfault错误时的调试输出
```
========================================
           HARD FAULT DETECTED!
========================================
Fault Status Register (CFSR): 0x00000000
Memory Management Fault Address: 0x00000000
Stack Pointer (MSP): 0x20001000
Program Counter (PC): 0x08001234
Link Register (LR): 0x08001235
xPSR: 0x01000000

--- Fault Analysis ---
Memory Management Fault:
  - Instruction access violation

--- RTOS Information ---
Current task: 0x20000000
Total tasks: 3
Current task priority: 1
Current task state: 1
Current task stack pointer: 0x20000F00
========================================
System halted - check debug output above
========================================
```

## 调试配置

### 修改调试级别
在 `02_rtos/core.h` 文件中修改以下宏定义：

```c
#define RTOS_DEBUG_ENABLE 1    /* 启用RTOS调试功能 */
#define RTOS_DEBUG_LEVEL  2    /* 调试级别: 0=关闭, 1=基本, 2=详细, 3=完整 */
```

### 调试级别说明
- **级别 0**: 完全关闭调试输出，系统运行最安静
- **级别 1**: 显示关键事件，适合生产环境监控
- **级别 2**: 显示详细过程，适合开发调试
- **级别 3**: 显示所有信息，适合深度调试

## 调试功能验证

### 1. 编译和烧录
确保所有修改的文件都已正确编译，没有错误。

### 2. 串口监控
- 连接UART1 (PA9-TX, PA10-RX)
- 波特率: 115200
- 数据位: 8
- 停止位: 1
- 校验位: 无

### 3. 观察调试输出
系统启动后，应该能看到：
1. 系统启动横幅
2. 调试配置信息
3. RTOS初始化过程
4. 任务创建过程
5. 任务调度过程
6. 任务执行过程

### 4. 验证任务调度
观察两个LED任务的交替执行，应该能看到：
- 绿色LED任务执行时输出 `[GREEN-TASK]` 信息
- 红色LED任务执行时输出 `[RED-TASK]` 信息
- 任务切换时的调度器调试信息

### 5. 验证中断处理
观察中断处理调试信息：
- `[SVC]` - 系统调用中断
- `[PendSV]` - 上下文切换中断
- `[TIM2]` - 定时器中断（如果使用延时函数）

## 故障排查

### 1. 没有调试输出
- 检查UART1配置是否正确
- 检查RTOS_DEBUG_ENABLE是否为1
- 检查RTOS_DEBUG_LEVEL是否大于0

### 2. 调试输出不完整
- 检查RTOS_DEBUG_LEVEL设置
- 检查printf重定向是否正确
- 检查堆栈是否足够

### 3. 系统崩溃
- 查看Hardfault调试输出
- 检查任务堆栈使用情况
- 检查中断优先级配置

## 性能影响

### 调试输出对性能的影响
- **级别 0**: 无性能影响
- **级别 1**: 轻微影响（约1-2%）
- **级别 2**: 中等影响（约5-10%）
- **级别 3**: 较大影响（约15-20%）

### 建议
- 开发阶段使用级别2或3
- 测试阶段使用级别1
- 生产环境使用级别0

## 总结

通过添加详细的调试打印信息，现在可以：
1. 实时监控RTOS系统的运行状态
2. 快速定位任务调度问题
3. 详细分析Hardfault错误原因
4. 跟踪中断处理过程
5. 监控堆栈使用情况

这些调试功能将大大提高RTOS系统的可维护性和问题排查效率。
