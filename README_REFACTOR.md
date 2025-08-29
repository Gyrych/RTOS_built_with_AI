# RTOS 内核对象化改造说明

## 改造概述

本项目参考RT-Thread的内核对象概念和实现，对原有RTOS代码进行了全面的模块化改造，实现了高内聚、低耦合的设计。

## 改造特点

### 1. 统一的内核对象基类
- 设计了统一的`rtos_object_t`基类，所有内核对象都继承自此基类
- 实现了`rtos_ipc_object_t`基类，用于线程间通信对象
- 支持对象的统一管理、查找、遍历和信息获取

### 2. 模块化设计
原来的单一文件被拆分为多个专门的模块：

#### 核心模块
- `rtos_object.h/c` - 内核对象管理系统
- `rtos_task.h/c` - 任务管理模块
- `rtos_system.c` - 系统核心实现

#### IPC模块
- `rtos_semaphore.h/c` - 信号量模块
- `rtos_mutex.h/c` - 互斥量模块
- `rtos_queue.h/c` - 消息队列模块
- `rtos_event.h` - 事件组模块(框架)

#### 其他模块
- `rtos_timer.h` - 定时器模块(框架)
- `rtos_mempool.h` - 内存池模块(框架)

#### 配置和头文件
- `rtos.h` - 主头文件，整合所有模块
- `rtos_config.h` - 系统配置文件

### 3. 面向对象的继承体系

```
rtos_object_t (基类)
├── rtos_task_t (任务对象)
├── rtos_timer_t (定时器对象)
└── rtos_ipc_object_t (IPC基类)
    ├── rtos_semaphore_t (信号量)
    ├── rtos_mutex_t (互斥量)
    ├── rtos_messagequeue_t (消息队列)
    ├── rtos_event_t (事件组)
    └── rtos_mempool_t (内存池)
```

### 4. 对象容器管理
- 每类对象都有独立的容器管理
- 支持对象链表组织和快速查找
- 提供对象统计和信息获取功能

### 5. 静态和动态对象支持
- 支持静态对象初始化(`xxx_init`)
- 支持动态对象创建(`xxx_create`)
- 统一的对象生命周期管理

## API变化对比

### 任务管理
```c
// 旧API
rtos_task_create(&task, name, func, param, priority, stack, size);

// 新API
rtos_task_init(&task, name, func, param, stack, size, priority, tick);
rtos_task_startup(&task);
```

### 信号量
```c
// 旧API
rtos_semaphore_create(&sem, initial, max);
rtos_semaphore_take(&sem, timeout_us);
rtos_semaphore_give(&sem);

// 新API
rtos_sem_init(&sem, name, initial, flag);
rtos_sem_take(&sem, timeout_ns);
rtos_sem_release(&sem);
```

### 互斥量
```c
// 旧API
rtos_mutex_create(&mutex);
rtos_mutex_lock(&mutex, timeout_us);
rtos_mutex_unlock(&mutex);

// 新API
rtos_mutex_init(&mutex, name, flag);
rtos_mutex_take(&mutex, timeout_ns);
rtos_mutex_release(&mutex);
```

### 消息队列
```c
// 旧API
rtos_queue_create(&queue, buffer, item_size, max_items);
rtos_queue_send(&queue, item, timeout_us);
rtos_queue_receive(&queue, item, timeout_us);

// 新API
rtos_mq_init(&mq, name, msgpool, msg_size, pool_size, flag);
rtos_mq_send(&mq, buffer, size, timeout_ns);
rtos_mq_recv(&mq, buffer, size, timeout_ns);
```

## 兼容性
- 提供了兼容性宏定义，保持与旧版本API的兼容
- 主程序经过相应更新以使用新的API

## 文件结构

```
.
├── rtos.h                  # 主头文件
├── rtos_config.h          # 系统配置
├── rtos_object.h/c        # 对象管理系统
├── rtos_task.h/c          # 任务管理
├── rtos_semaphore.h/c     # 信号量
├── rtos_mutex.h/c         # 互斥量
├── rtos_queue.h/c         # 消息队列
├── rtos_timer.h           # 定时器(框架)
├── rtos_event.h           # 事件组(框架)
├── rtos_mempool.h         # 内存池(框架)
├── rtos_system.c          # 系统实现
├── main.c                 # 示例程序(已更新)
├── Makefile               # 构建文件(已更新)
└── README_REFACTOR.md     # 本说明文件
```

## 设计优势

### 1. 高内聚
- 每个模块职责单一，专注于特定功能
- 模块内部数据和函数紧密相关

### 2. 低耦合
- 模块间依赖关系清晰
- 通过统一的对象基类实现接口标准化

### 3. 可扩展性
- 新的内核对象类型可以轻松添加
- 遵循统一的设计模式

### 4. 可维护性
- 代码结构清晰，易于理解和维护
- 问题定位更加容易

### 5. 重用性
- 对象管理系统可以被其他模块重用
- 统一的接口设计便于代码复用

## 后续工作
1. 完善定时器、事件组、内存池模块的具体实现
2. 添加更多的调试和诊断功能
3. 优化内存管理算法
4. 添加更完善的错误处理机制
5. 实现更多的系统钩子函数

## 总结
通过本次改造，RTOS系统实现了真正的模块化和面向对象设计，代码结构更加清晰，维护性和扩展性大大提升，为后续的功能扩展奠定了良好的基础。