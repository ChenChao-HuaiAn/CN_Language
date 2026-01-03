# Linux平台中断处理模块

## 概述

本模块实现了CN_Language项目在Linux平台上的中断处理功能。基于Linux信号机制，提供了信号到中断的映射、中断处理程序注册、中断优先级管理等功能。

## 设计原则

1. **模块化设计**：遵循单一职责原则，将中断控制器、中断管理器和中断工具分离
2. **平台抽象**：通过抽象接口隐藏Linux信号处理的具体实现细节
3. **线程安全**：所有关键操作都通过互斥锁保护，确保多线程环境下的安全性
4. **可扩展性**：支持中断共享、中断链、延迟处理等高级功能

## 架构设计

### 1. 中断控制器 (Interrupt Controller)

负责底层中断处理，包括：
- 中断向量表管理
- 中断处理程序注册和注销
- 中断使能/禁用控制
- 中断优先级管理
- 中断屏蔽和解除屏蔽

### 2. 中断管理器 (Interrupt Manager)

提供高级中断管理功能：
- 中断分配和释放
- 中断路由和共享
- 系统范围中断控制
- 中断等待和轮询
- 中断模拟和诊断

### 3. 中断工具 (Interrupt Tools)

提供辅助功能：
- 中断上下文管理
- 中断栈管理
- 延迟中断处理
- 中断过滤和日志
- 性能分析和监控

## 实现细节

### 信号到中断映射

Linux使用信号机制处理异步事件，本模块将Linux信号映射为CN中断号：
- 信号1-64映射为中断号1001-1064
- 实时信号(34-64)支持优先级管理
- 支持信号屏蔽和解除屏蔽

### 中断处理流程

1. **信号接收**：Linux内核发送信号到进程
2. **信号转换**：将Linux信号转换为CN中断号
3. **中断分发**：根据中断号调用注册的处理程序
4. **上下文保存**：保存中断发生时的CPU上下文
5. **处理程序执行**：执行用户注册的中断服务例程
6. **上下文恢复**：恢复中断前的CPU上下文
7. **中断结束**：发送EOI(End of Interrupt)信号

### 线程安全机制

- 使用pthread互斥锁保护共享数据结构
- 支持递归锁，避免死锁
- 信号处理程序使用异步信号安全函数
- 关键区域使用原子操作

## API使用示例

### 基本中断处理

```c
#include "CN_platform_linux_interrupt.h"

// 中断处理函数
void my_interrupt_handler(void* context) {
    printf("中断处理程序被调用\n");
    // 处理中断
}

int main() {
    // 初始化中断处理系统
    CN_interrupt_initialize();
    
    // 获取中断控制器接口
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_platform_linux_get_interrupt_controller();
    
    // 注册中断处理程序
    CN_InterruptNumber_t interrupt_num = CN_LINUX_SIGNAL_TO_INTERRUPT(SIGUSR1);
    controller->register_handler(interrupt_num, my_interrupt_handler, NULL, "用户信号1", 0);
    
    // 启用中断
    controller->enable_interrupt(interrupt_num);
    
    // ... 程序运行 ...
    
    // 清理
    controller->cleanup();
    
    return 0;
}
```

### 高级中断管理

```c
// 使用中断管理器分配中断
Stru_CN_InterruptManagerInterface_t* manager = 
    CN_platform_linux_get_interrupt_manager();

Stru_CN_InterruptDescriptor_t desc = {
    .type = Eum_INTERRUPT_TYPE_EXTERNAL,
    .trigger = Eum_INTERRUPT_TRIGGER_EDGE,
    .priority = 5,
    .description = "自定义中断"
};

CN_InterruptNumber_t irq;
if (manager->allocate_interrupt(&irq, &desc)) {
    printf("分配中断号: %u\n", irq);
}

// 共享中断
manager->share_interrupt(irq, another_handler, another_context);

// 等待中断
bool occurred;
manager->poll_interrupt(irq, &occurred);
```

## 配置选项

### 编译时配置

在`CN_platform_linux_interrupt.h`中定义：

```c
// 最大中断处理程序数量
#define CN_LINUX_MAX_INTERRUPT_HANDLERS 256

// 最大中断名称长度
#define CN_LINUX_MAX_INTERRUPT_NAME 64

// 中断信号范围
#define CN_LINUX_SIGNAL_MIN 1
#define CN_LINUX_SIGNAL_MAX 64
```

### 运行时配置

通过中断管理器接口配置：
- 中断优先级
- CPU亲和性
- 触发模式（边沿/电平）
- 中断极性（高电平/低电平有效）

## 性能考虑

1. **中断延迟**：最小化中断处理时间，避免长时间持有锁
2. **内存使用**：使用固定大小的数组，避免动态内存分配
3. **缓存友好**：优化数据结构布局，提高缓存命中率
4. **可扩展性**：支持大量并发中断处理

## 测试策略

### 单元测试
- 测试中断注册和注销
- 测试中断使能和禁用
- 测试中断优先级设置
- 测试中断共享功能

### 集成测试
- 测试多个中断同时发生
- 测试中断嵌套处理
- 测试中断与线程的交互
- 测试系统负载下的中断处理

### 压力测试
- 测试最大数量中断处理程序
- 测试高频率中断
- 测试长时间运行稳定性

## 兼容性

### 支持的Linux版本
- Linux内核2.6.32及以上
- glibc 2.17及以上
- POSIX线程支持

### 硬件平台
- x86/x86_64架构
- ARM架构（需要相应调整）
- 支持SMP（对称多处理）

## 故障排除

### 常见问题

1. **中断丢失**：检查信号屏蔽设置
2. **处理程序不执行**：确认中断已启用
3. **性能问题**：检查中断处理时间
4. **死锁**：检查锁的使用顺序

### 调试支持

- 启用中断日志记录
- 使用性能分析工具
- 检查系统日志（/var/log/messages）
- 使用strace跟踪信号处理

## 未来扩展

1. **实时信号支持**：增强实时信号处理能力
2. **多核优化**：优化SMP环境下的中断处理
3. **电源管理**：支持中断唤醒功能
4. **虚拟化支持**：在虚拟化环境中运行

## 参考文档

1. Linux信号处理手册：`man 7 signal`
2. POSIX线程编程指南
3. CN_Language架构设计文档
4. 中断处理最佳实践

---

*最后更新：2026年1月3日*
*版本：1.0.0*
