# STM32F407 RTOS内核 - 重构版

这是一个专为STM32F407微控制器设计的模块化实时操作系统(RTOS)内核，采用面向对象的设计理念，具有以下特点：

## 主要特性

1. **模块化架构** - 采用面向对象设计，各功能模块独立且可扩展
2. **完全基于优先级抢占调度** - 高优先级任务可以抢占低优先级任务
3. **丰富的同步机制** - 信号量、互斥量、消息队列、事件组、内存池
4. **软件定时器** - 支持周期性和单次定时器，带回调函数
5. **内存管理** - 固定大小内存池，避免内存碎片
6. **硬件抽象层** - 平台无关的硬件接口，便于移植
7. **系统钩子** - 可配置的系统事件回调
8. **针对STM32F407优化** - 充分利用Cortex-M4特性和STM32F407硬件资源

## 核心功能

### 任务管理
- 任务创建、删除、挂起、恢复
- 基于优先级的抢占式调度
- 支持最多32个优先级级别(0-31，0为最高优先级)
- 支持最多16个并发任务
- 栈溢出检测
- 时间片轮转调度

### 同步机制
- **信号量(Semaphore)** - 支持计数信号量和二进制信号量
- **互斥量(Mutex)** - 支持递归锁定和优先级继承
- **消息队列(Message Queue)** - 环形缓冲区实现，支持任意大小的消息传递
- **事件标志组(Event Groups)** - 支持多事件等待和组合逻辑
- **内存池(Memory Pools)** - 确定性内存分配，避免碎片

### 时间管理
- 软件定时器系统
- 周期性和单次定时器
- 自动重装载功能
- 回调函数支持
- 定时器状态查询

### 内存管理
- 固定大小块内存池
- 内存使用统计
- 内存完整性检查
- 内存池重置功能

### 硬件抽象层
- 平台检测和配置
- 中断优先级管理
- 时钟频率获取
- 延时函数实现
- 临界区保护
- 系统复位和看门狗
- 内存信息查询

### 系统管理
- 对象系统初始化
- 任务系统初始化
- 同步机制初始化
- 系统钩子支持
- 系统状态管理

## 文件结构

```
├── rtos/                    # RTOS核心模块
│   ├── core/               # 核心对象系统
│   │   ├── object.h        # 对象基类定义
│   │   ├── object.c        # 对象系统实现
│   │   └── types.h         # 通用类型定义
│   ├── task/               # 任务管理模块
│   │   ├── task.h          # 任务管理接口
│   │   └── task.c          # 任务管理实现
│   ├── sync/               # 同步机制模块
│   │   ├── semaphore.h     # 信号量接口
│   │   ├── semaphore.c     # 信号量实现
│   │   ├── mutex.h         # 互斥量接口
│   │   ├── mutex.c         # 互斥量实现
│   │   ├── queue.h         # 消息队列接口
│   │   ├── queue.c         # 消息队列实现
│   │   ├── event.h         # 事件组接口
│   │   └── event.c         # 事件组实现
│   ├── time/               # 时间管理模块
│   │   ├── timer.h         # 软件定时器接口
│   │   └── timer.c         # 软件定时器实现
│   ├── memory/             # 内存管理模块
│   │   ├── mempool.h       # 内存池接口
│   │   └── mempool.c       # 内存池实现
│   ├── hw/                 # 硬件抽象层
│   │   ├── hw_abstraction.h # 硬件抽象接口
│   │   └── hw_abstraction.c # 硬件抽象实现
│   ├── config/             # 配置模块
│   │   └── config.h        # 系统配置
│   └── system.c            # 系统管理
├── rtos.h                  # 主头文件
├── main_refactored.c       # 重构版示例程序
├── main_complete.c         # 完整功能示例程序
├── Makefile_refactored     # 重构版编译配置
├── Makefile_complete       # 完整版编译配置
├── startup_stm32f407xx.s   # STM32F407启动代码
├── STM32F407VGTx_FLASH.ld # 链接脚本
├── system_support.c        # 系统支持函数
└── README.md               # 说明文档
```

## 编译和使用

### 重构版编译
```bash
make -f Makefile_refactored
```

### 完整版编译
```bash
make -f Makefile_complete
```

### 清理编译文件
```bash
make -f Makefile_refactored clean
make -f Makefile_complete clean
```

## 使用示例

### 基本任务创建
```c
#include "rtos.h"

void task1(void *param) {
    while (1) {
        rtos_delay_ms(1000);
        // 任务代码
    }
}

int main() {
    rtos_system_init();
    
    rtos_task_create(task1, "Task1", 1024, NULL, 5);
    
    rtos_system_start();
    return 0;
}
```

### 同步机制使用
```c
// 创建信号量
rtos_semaphore_t sem = rtos_semaphore_create(1, 1);

// 等待信号量
rtos_semaphore_take(sem, RTOS_WAIT_FOREVER);

// 释放信号量
rtos_semaphore_give(sem);
```

### 软件定时器
```c
// 创建定时器
rtos_sw_timer_t timer = rtos_sw_timer_create("Timer1", 1000, RTOS_TIMER_PERIODIC);

// 设置回调函数
rtos_sw_timer_set_callback(timer, timer_callback);

// 启动定时器
rtos_sw_timer_start(timer);
```

## 技术特点

- **模块化设计**：各功能模块独立，便于维护和扩展
- **面向对象**：基于对象的设计理念，代码结构清晰
- **可移植性**：硬件抽象层设计，便于移植到其他平台
- **内存安全**：内存池管理，避免内存碎片和泄漏
- **实时性能**：优化的调度算法，保证实时性要求
- **易于使用**：简洁的API设计，降低学习成本

## 系统要求

- STM32F407微控制器
- ARM Cortex-M4内核
- 至少64KB RAM
- 至少512KB Flash
- GCC ARM工具链

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进这个RTOS内核。

## 联系方式

如有问题或建议，请通过GitHub Issues联系。