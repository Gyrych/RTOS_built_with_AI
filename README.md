# Tickless RTOS (STM32F407)

本项目是一个基于 STM32F407 的自研 Tickless RTOS 演示工程，展示在无 SysTick 的架构下，如何通过 SVC + PendSV 实现协作式任务切换，并预留基于 TIM2 的高精度延时能力。

## 目录结构

```
02_TickLessRTOS/
├─ 00_project/
│  ├─ User/                    # 应用与硬件抽象（main、LED、UART、中断封装）
│  └─ EIDE/                    # EIDE 工程与构建产物
├─ 01_fwlib/                   # ST 外设库与 CMSIS
└─ 02_rtos/                    # RTOS 核心（调度/任务/时间）
```

## 核心设计

- 启动首任务：`rtos_start()` 通过 `svc 1` 进入 `svc_handler`，设置 PSP 后以 EXC_RETURN 返回到线程态运行首任务。
- 任务让出：线程态调用 `rtos_schedule()`，直接进行调度决策并置位 `PendSV` 触发上下文切换（避免 SVC 尾链边界）。
- 上下文切换：`pend_sv_handler` 纯汇编实现，保存/恢复 R4-R11，更新 PSP 指向硬件帧（R0..xPSR），以 `0xFFFFFFFD` 返回到线程态。
- 任务栈布局：每个任务创建时，先构造“硬件自动帧（R0..xPSR）”，再在更低地址放置“R4..R11 保存区”，`stack_ptr` 指向 R4 起始，满足恢复序列 `ldmia r0!,{r4-r11}` → `MSR PSP,r0` → EXC_RETURN。

## 日志与调试

- 默认 `RTOS_DEBUG_LEVEL=2`，提供适度的 RTOS 内部信息；异常上下文（SVC/PendSV）禁止使用 `printf`。
- `task_resume` 对非挂起任务的恢复视为无害操作，仅在更高调试级别提示。

## 构建与运行

- 使用 EIDE 工程进行构建与下载调试；串口：UART1（PA9/PA10），115200 8N1。
- 上电后将看到系统横幅、任务创建信息与两个 LED 任务交替打印。

## 开发计划

- 集成 TIM2 高精度延时至任务延时 API（当前 Demo 使用忙等）。
- 增加任务状态可视化与更完善的调试接口（不在异常内打印）。
- 评估加入简单的优先级抢占与时间片（保持 Tickless 设计理念）。

## 注意事项

- 启动文件需将向量表中的 `SVC_Handler`/`PendSV_Handler` 指向裸跳转桩，不可在异常内做 C 调用或 `printf`。
- 任何涉及异常返回路径的修改需谨慎验证 EXC_RETURN 与 PSP 指向。
