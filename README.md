# Tickless RTOS on STM32F407（中文说明）

## 项目简介
本项目在 STM32F407 上实现一个极简 Tickless 实时操作系统（RTOS）。特点：
- 无滴答（不使用 SysTick），事件驱动
- 基于 TIM2@84MHz 的高精度延时（ns/us/ms），并与调度协作
- USART1 + DMA（RX 环形 + IDLE 分段；TX DMA 队列化）
- 异步日志（线程格式化入队 + 低优先级后台冲刷）
- 双 LED 任务演示调度与延时

## 目录结构
- `02_rtos/`: RTOS 内核与驱动
  - `core.{h,c}`: 任务/调度/上下文切换/临界区/调试
  - `time.{h,c}`: TIM2 高精度延时与并发队列
  - `uart.{h,c}`: USART1 DMA RX(IDLE)+TX 队列
  - `log.{h,c}`: 异步日志
- `00_project/User/`: 用户入口与中断
  - `main.c`: 双 LED 任务演示
  - `config/stm32f4/core/*`: `system_stm32f4xx.*`、`stm32f4xx_it.*`、`main.h`
- `01_fwlib/`: ST 标准外设库
- `03_doc/`: 文档（计划书、阶段性评价、开发记录）

## 硬件与工具
- 开发板：星火一号 STM32F407VGTx（PF11 绿色 LED，PF12 红色 LED）
- 串口：USART1（PA9/PA10），115200 8N1
- 开发环境：EIDE/Make/GCC，OpenOCD + ST-Link

## 快速上手
1. 连接串口至 PC（115200 8N1）
2. 构建并下载固件（EIDE 已配置）
3. 复位运行，串口可见简洁启动信息；LED 按设定周期闪烁

## 运行演示
- 任务：
  - `task_led_r_blink`: PF12 红灯，周期 100 ms
  - `task_led_g_blink`: PF11 绿灯，周期 500 ms
- 延时：使用 `Delay_ms()`，任务在延时期间挂起；到期由 TIM2 中断恢复并触发调度
- 日志：`rtos_log_task` 低优先级后台冲刷 UART（无阻塞主任务）

## 架构要点
- 调度：使用 `pxCurrentTCB/pxNextTCB` 指针与 PendSV 裸汇编切换，SVC 首任务进入线程态（PSP）
- 时间：TIM2 32 位计数，支持回绕；并发延时队列选择最近比较点
- 串口：DMA RX 环形 + IDLE 分段回调，DMA TX 队列线性切片续传

## 中断优先级约定
- SVC: 0（最高）
- TIM2: 3（建议）
- USART1/DMA: 6（建议）
- PendSV: 15（最低）
- SysTick: 未使用

## 常见问题（FAQ）
- 无串口输出：检查 GND/PA9/PA10 连接与波特率；确认 `rtos_uart_init()` 被调用
- LED 不闪烁：确认 GPIOF 时钟开启与 PF11/PF12 接线；检查 TIM2 中断是否触发
- HardFault：典型原因是栈溢出或任务返回；请核对任务栈大小与 `prvTaskExitError`

## 版本记录
详见 `03_doc/开发过程记录.md`

## 许可证
- 本项目代码（除 `01_fwlib`）以 MIT License 发布；
- `01_fwlib`（ST 标准外设库）遵循 STMicroelectronics 原版权与许可；

MIT License 文本：

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

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
