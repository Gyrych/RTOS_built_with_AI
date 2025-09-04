# STM32F407 RTOS 开发指南

## 目录
1. [开发环境搭建](#开发环境搭建)
2. [项目构建](#项目构建)
3. [代码规范](#代码规范)
4. [调试技巧](#调试技巧)
5. [扩展开发](#扩展开发)
6. [测试指南](#测试指南)
7. [常见问题](#常见问题)

## 开发环境搭建

### 必需软件

#### 1. 开发工具
- **VSCode**: 代码编辑器
- **EIDE扩展**: 嵌入式开发环境
- **ARM GCC**: 交叉编译器
- **Git**: 版本控制

#### 2. 调试工具
- **ST-Link Utility**: ST-Link调试器工具
- **OpenOCD**: 开源调试服务器 (可选)
- **J-Link**: 第三方调试器 (可选)

### 安装步骤

#### 1. 安装VSCode和EIDE
```bash
# 下载并安装VSCode
# 在VSCode中安装EIDE扩展

# 或者使用命令行安装
code --install-extension marus25.cortex-debug
code --install-extension gcc-arm-none-eabi
```

#### 2. 安装ARM GCC工具链
```bash
# Windows (使用Chocolatey)
choco install gcc-arm-embedded

# Linux (Ubuntu/Debian)
sudo apt-get install gcc-arm-none-eabi

# macOS (使用Homebrew)
brew install gcc-arm-none-eabi
```

#### 3. 验证安装
```bash
# 检查编译器版本
arm-none-eabi-gcc --version

# 检查调试器连接
st-info --probe
```

### 项目配置

#### 1. 克隆项目
```bash
git clone <repository-url>
cd RTOS_built_with_AI/00_project/EIDE
```

#### 2. 配置EIDE
```json
// .vscode/settings.json
{
    "eide.projectRoot": ".",
    "eide.buildPath": "./build",
    "eide.outputPath": "./build/output",
    "eide.toolchain": "gcc",
    "eide.debugger": "stlink"
}
```

#### 3. 配置编译选项
```json
// .eide/eide.json
{
    "name": "template_stm32f4_rt-thread-nano_c",
    "type": "executable",
    "target": "stm32f407vgtx",
    "toolchain": "gcc",
    "compiler": {
        "flags": [
            "-mcpu=cortex-m4",
            "-mthumb",
            "-mfloat-abi=hard",
            "-mfpu=fpv4-sp-d16",
            "-DSTM32F40_41xxx",
            "-DUSE_STDPERIPH_DRIVER"
        ]
    }
}
```

## 项目构建

### 构建命令

#### 1. 完整构建
```bash
# 清理并重新构建
eide clean
eide build

# 或者使用命令行
make clean
make all
```

#### 2. 增量构建
```bash
# 只构建修改的文件
eide build
```

#### 3. 调试构建
```bash
# 构建调试版本
eide build --debug

# 构建发布版本
eide build --release
```

### 构建输出

#### 1. 输出文件
```
build/
├── template_stm32f4_rt-thread_c.elf    # ELF可执行文件
├── template_stm32f4_rt-thread_c.bin    # 二进制文件
├── template_stm32f4_rt-thread_c.hex    # Intel HEX文件
├── template_stm32f4_rt-thread_c.map    # 内存映射文件
└── compile_commands.json               # 编译命令数据库
```

#### 2. 内存使用报告
```
Memory Usage:
Flash: 12345 bytes (1.2% of 1MB)
RAM:   6789 bytes (3.5% of 192KB)
```

### 烧录和调试

#### 1. 烧录到设备
```bash
# 使用ST-Link烧录
eide upload

# 或者使用命令行
st-flash write build/output.bin 0x08000000
```

#### 2. 启动调试
```bash
# 启动调试会话
eide debug

# 或者使用GDB
arm-none-eabi-gdb build/output.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) continue
```

## 代码规范

### 命名规范

#### 1. 函数命名
```c
// 使用驼峰命名法
void taskCreate(void (*func)(void*), void* arg, uint32_t priority);
void delayMs(uint32_t ms);
void ledInit(void);

// RTOS系统函数使用下划线
void rtos_init(void);
void rtos_start(void);
void rtos_schedule(void);
```

#### 2. 变量命名
```c
// 全局变量使用下划线前缀
static uint32_t g_task_count = 0;
static task_t* g_current_task = NULL;

// 局部变量使用驼峰命名法
uint32_t taskPriority = 1;
uint32_t stackSize = 256;
```

#### 3. 宏定义
```c
// 使用大写字母和下划线
#define MAX_TASKS 32
#define STACK_SIZE 256
#define TASK_READY 0
#define TASK_RUNNING 1
```

### 代码格式

#### 1. 缩进和空格
```c
// 使用4个空格缩进
void functionName(void) {
    if (condition) {
        // 代码块
        doSomething();
    }
}

// 操作符周围使用空格
uint32_t result = a + b * c;
if (value > threshold) {
    // 处理逻辑
}
```

#### 2. 注释规范
```c
/**
  * @brief  函数功能描述
  * @param  param1: 参数1描述
  * @param  param2: 参数2描述
  * @retval 返回值描述
  */
void functionName(uint32_t param1, uint32_t param2) {
    // 单行注释说明关键逻辑
    uint32_t result = param1 + param2;
    
    /* 多行注释说明复杂逻辑
     * 这里实现了复杂的算法
     * 需要详细说明实现原理
     */
    return result;
}
```

#### 3. 头文件格式
```c
/**
  ******************************************************************************
  * @file    filename.h
  * @author  Author Name
  * @version V1.0.0
  * @date    2025-01-14
  * @brief   文件功能描述
  ******************************************************************************
  * @attention
  *
  * 详细说明和注意事项
  *
  ******************************************************************************
  */

#ifndef __FILENAME_H
#define __FILENAME_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __FILENAME_H */
```

### 错误处理

#### 1. 返回值检查
```c
// 检查函数返回值
task_t* task = task_create(task_func, NULL, 1);
if (task == NULL) {
    // 错误处理
    printf("Error: Failed to create task\n");
    return -1;
}
```

#### 2. 参数验证
```c
// 验证输入参数
void delay_ms(uint32_t ms) {
    if (ms == 0) {
        return;  // 无效参数，直接返回
    }
    
    if (ms > MAX_DELAY_MS) {
        ms = MAX_DELAY_MS;  // 限制最大值
    }
    
    // 执行延时
    tim2_start_delay(ms);
}
```

## 调试技巧

### 调试工具使用

#### 1. 断点调试
```c
// 在关键位置设置断点
void task_led_blink(void* arg) {
    while(1) {
        LED_ON();
        // 在这里设置断点
        Delay_ms(200);
        
        LED_OFF();
        Delay_ms(800);
    }
}
```

#### 2. 变量监视
```c
// 监视关键变量
// 在调试器中添加监视：
// - scheduler.task_count
// - scheduler.current_task
// - delay_ctrl.state
```

#### 3. 内存查看
```c
// 查看任务堆栈使用情况
void debug_print_stack_usage(void) {
    for (uint8_t i = 0; i < scheduler.task_count; i++) {
        task_t* task = scheduler.tasks[i];
        uint32_t stack_used = STACK_SIZE - 
            ((uint32_t)task->stack_ptr - (uint32_t)task->stack) / 4;
        printf("Task %d stack usage: %d/%d bytes\n", 
               i, stack_used * 4, STACK_SIZE * 4);
    }
}
```

### 日志调试

#### 1. 串口输出
```c
// 添加串口调试输出
void debug_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // 通过串口输出
    uart_send_string(buffer);
    
    va_end(args);
}
```

#### 2. LED指示
```c
// 使用LED指示系统状态
void debug_led_indicate(uint32_t pattern) {
    for (uint8_t i = 0; i < 8; i++) {
        if (pattern & (1 << i)) {
            LED_ON();
        } else {
            LED_OFF();
        }
        Delay_ms(100);
    }
}
```

### 性能分析

#### 1. 执行时间测量
```c
// 测量函数执行时间
uint32_t measure_execution_time(void (*func)(void)) {
    uint32_t start_time = TIM_GetCounter(TIM2);
    func();
    uint32_t end_time = TIM_GetCounter(TIM2);
    
    return end_time - start_time;
}
```

#### 2. 任务切换统计
```c
// 统计任务切换次数
static uint32_t context_switch_count = 0;

void pend_sv_handler(void) {
    context_switch_count++;
    
    // 原有的上下文切换代码
    // ...
}
```

## 扩展开发

### 添加新功能

#### 1. 信号量实现
```c
// 信号量结构体
typedef struct {
    uint32_t count;
    uint32_t max_count;
    task_t* waiting_tasks[MAX_TASKS];
    uint8_t waiting_count;
} semaphore_t;

// 信号量操作函数
void semaphore_init(semaphore_t* sem, uint32_t initial_count);
void semaphore_wait(semaphore_t* sem);
void semaphore_signal(semaphore_t* sem);
```

#### 2. 消息队列实现
```c
// 消息队列结构体
typedef struct {
    void* buffer;
    uint32_t buffer_size;
    uint32_t message_size;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} message_queue_t;

// 消息队列操作函数
void queue_init(message_queue_t* queue, void* buffer, 
                uint32_t buffer_size, uint32_t message_size);
bool queue_send(message_queue_t* queue, const void* message);
bool queue_receive(message_queue_t* queue, void* message);
```

#### 3. 软件定时器
```c
// 定时器结构体
typedef struct {
    uint32_t period;
    uint32_t remaining;
    void (*callback)(void*);
    void* arg;
    bool active;
} software_timer_t;

// 定时器操作函数
void timer_init(software_timer_t* timer, uint32_t period, 
                void (*callback)(void*), void* arg);
void timer_start(software_timer_t* timer);
void timer_stop(software_timer_t* timer);
void timer_tick(void);  // 在SysTick中断中调用
```

### 模块化设计

#### 1. 创建新模块
```c
// 新模块头文件 (new_module.h)
#ifndef __NEW_MODULE_H
#define __NEW_MODULE_H

#include "stm32f4xx.h"

// 模块配置
#define NEW_MODULE_MAX_ITEMS 16

// 模块接口
void new_module_init(void);
void new_module_process(void);
uint32_t new_module_get_count(void);

#endif /* __NEW_MODULE_H */
```

#### 2. 模块实现
```c
// 新模块实现 (new_module.c)
#include "new_module.h"

// 私有变量
static uint32_t module_count = 0;

// 模块初始化
void new_module_init(void) {
    module_count = 0;
    // 其他初始化代码
}

// 模块处理函数
void new_module_process(void) {
    // 模块处理逻辑
    module_count++;
}

// 获取模块计数
uint32_t new_module_get_count(void) {
    return module_count;
}
```

## 测试指南

### 单元测试

#### 1. 测试框架
```c
// 简单的测试框架
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("TEST FAILED: %s:%d\n", __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define TEST_RUN(test_func) \
    do { \
        printf("Running test: %s\n", #test_func); \
        if (test_func()) { \
            printf("PASSED\n"); \
        } else { \
            printf("FAILED\n"); \
        } \
    } while(0)
```

#### 2. 测试用例
```c
// 测试任务创建
bool test_task_create(void) {
    task_t* task = task_create(test_task_func, NULL, 1);
    TEST_ASSERT(task != NULL);
    TEST_ASSERT(task->priority == 1);
    TEST_ASSERT(task->state == TASK_READY);
    return true;
}

// 测试延时精度
bool test_delay_accuracy(void) {
    uint32_t start_time = TIM_GetCounter(TIM2);
    Delay_ms(100);
    uint32_t end_time = TIM_GetCounter(TIM2);
    
    uint32_t actual_delay = (end_time - start_time) / 84000; // 转换为毫秒
    TEST_ASSERT(actual_delay >= 99 && actual_delay <= 101);
    return true;
}
```

### 集成测试

#### 1. 多任务测试
```c
// 测试多任务调度
void test_multitask_scheduling(void) {
    // 创建多个任务
    task_create(task_high_priority, NULL, 1);
    task_create(task_medium_priority, NULL, 2);
    task_create(task_low_priority, NULL, 3);
    
    // 启动RTOS
    rtos_start();
    
    // 验证任务调度
    // ...
}
```

#### 2. 压力测试
```c
// 压力测试：创建大量任务
void stress_test_task_creation(void) {
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        task_t* task = task_create(stress_test_task, NULL, i);
        TEST_ASSERT(task != NULL);
    }
    
    // 验证系统稳定性
    // ...
}
```

## 常见问题

### 编译问题

#### 1. 链接错误
```
错误: undefined reference to `function_name'
解决: 检查函数声明和实现是否匹配
```

#### 2. 内存不足
```
错误: region 'FLASH' overflowed
解决: 优化代码大小，移除未使用的函数
```

### 运行时问题

#### 1. 系统崩溃
```
问题: 系统进入HardFault
解决: 检查堆栈溢出，验证内存访问
```

#### 2. 任务不切换
```
问题: 只有一个任务在运行
解决: 检查中断配置，验证调度器
```

#### 3. 延时不准确
```
问题: 延时时间偏差较大
解决: 检查TIM2配置，验证中断优先级
```

### 调试问题

#### 1. 断点不生效
```
问题: 断点设置后程序不停止
解决: 检查调试器连接，验证程序加载
```

#### 2. 变量值不正确
```
问题: 调试器中变量值异常
解决: 检查优化级别，验证内存布局
```

### 性能问题

#### 1. 任务切换慢
```
问题: 任务切换时间过长
解决: 优化上下文切换代码，减少中断处理时间
```

#### 2. 内存使用过多
```
问题: RAM使用量超出预期
解决: 优化数据结构，减少任务堆栈大小
```

---

**注意**: 本指南会随着项目发展持续更新，请关注最新版本。如有问题，请参考技术文档或提交Issue。
