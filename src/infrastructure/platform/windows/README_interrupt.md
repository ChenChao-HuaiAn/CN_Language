# Windows平台中断处理模块

## 概述

Windows平台中断处理模块是CN_Language项目在Windows操作系统上的中断处理实现。该模块基于Windows事件机制提供中断模拟功能，支持基于事件、APC（异步过程调用）和I/O完成端口的中断处理。

## 架构设计

### 模块结构

```
src/infrastructure/platform/windows/
├── CN_platform_windows_interrupt.h      # Windows中断处理接口定义
├── CN_platform_windows_interrupt.c      # Windows中断控制器实现
└── README_interrupt.md                  # 本说明文档
```

### 设计原则

1. **平台适配性**：充分利用Windows特有的事件、APC和I/O完成端口机制
2. **线程安全**：使用临界区（CRITICAL_SECTION）确保多线程安全
3. **资源管理**：自动管理Windows句柄和内存资源
4. **接口一致性**：与通用中断接口完全兼容

## 核心功能

### 1. 基于事件的中断处理

Windows平台使用事件对象（Event Objects）模拟硬件中断：

```c
// 创建事件
HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);

// 等待事件（模拟中断等待）
WaitForSingleObject(event, INFINITE);

// 触发事件（模拟中断触发）
SetEvent(event);
```

### 2. APC（异步过程调用）支持

支持Windows APC机制，允许在特定线程上下文中执行中断处理：

```c
// 队列用户模式APC
QueueUserAPC(apc_func, thread, data);

// 进入可警告等待状态
SleepEx(0, TRUE);
```

### 3. I/O完成端口集成

支持I/O完成端口机制，适用于高性能I/O中断处理：

```c
// 创建完成端口
HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

// 关联设备句柄
CreateIoCompletionPort(device_handle, iocp, completion_key, 0);

// 获取完成状态
GetQueuedCompletionStatus(iocp, &bytes_transferred, &completion_key, &overlapped, INFINITE);
```

## 接口说明

### 中断控制器接口

Windows中断控制器接口实现了`Stru_CN_InterruptControllerInterface_t`定义的所有功能：

- **初始化/清理**：`initialize()` / `cleanup()`
- **中断注册**：`register_handler()` / `unregister_handler()`
- **中断控制**：`enable_interrupt()` / `disable_interrupt()`
- **状态查询**：`is_interrupt_enabled()` / `is_pending()`

### Windows特有接口

除了标准接口外，还提供Windows特有接口：

1. **事件接口** (`Stru_CN_WindowsEventInterface_t`)：Windows事件管理
2. **APC接口** (`Stru_CN_WindowsAPCInterface_t`)：异步过程调用管理
3. **I/O完成端口接口** (`Stru_CN_WindowsIOCompletionInterface_t`)：高性能I/O中断处理
4. **中断线程接口** (`Stru_CN_WindowsInterruptThreadInterface_t`)：中断线程管理

## 使用示例

### 基本使用

```c
#include "CN_platform_windows_interrupt.h"

// 中断处理函数
void my_interrupt_handler(void* context)
{
    int* counter = (int*)context;
    (*counter)++;
    printf("Windows中断发生，计数器: %d\n", *counter);
}

int main()
{
    // 初始化Windows中断处理系统
    if (!CN_platform_windows_interrupt_initialize()) {
        fprintf(stderr, "无法初始化Windows中断处理系统\n");
        return -1;
    }
    
    // 获取Windows中断控制器
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_platform_windows_get_interrupt_controller();
    
    // 注册中断处理函数
    int counter = 0;
    CN_InterruptNumber_t irq = 2001; // Windows事件1映射的中断号
    
    Stru_CN_InterruptDescriptor_t desc = {
        .number = irq,
        .type = Eum_INTERRUPT_TYPE_EXTERNAL,
        .trigger = Eum_INTERRUPT_TRIGGER_EDGE,
        .polarity = Eum_INTERRUPT_POLARITY_RISING_EDGE,
        .priority = 5,
        .cpu_affinity = 0x1,
        .enabled = true,
        .shared = false,
        .description = "Windows测试中断"
    };
    
    if (!controller->register_handler(irq, my_interrupt_handler, &counter, &desc)) {
        fprintf(stderr, "无法注册中断处理函数\n");
        CN_platform_windows_interrupt_cleanup();
        return -1;
    }
    
    // 启用中断
    controller->enable_interrupt(irq);
    
    printf("Windows中断处理已设置，等待中断...\n");
    
    // 模拟中断触发（在实际应用中可能由外部事件触发）
    // 这里简单等待用户输入
    printf("按Enter键模拟中断触发...\n");
    getchar();
    
    // 在实际应用中，这里会有事件触发机制
    // 例如：SetEvent(handler_event);
    
    // 清理
    controller->unregister_handler(irq);
    CN_platform_windows_interrupt_cleanup();
    
    return 0;
}
```

### 使用Windows事件接口

```c
#include "CN_platform_windows_interrupt.h"

void use_windows_event_interface(void)
{
    Stru_CN_WindowsEventInterface_t* event_if = 
        CN_platform_windows_get_event_interface();
    
    if (!event_if) {
        return;
    }
    
    // 创建事件
    HANDLE event = event_if->event_create(FALSE, FALSE, "TestEvent");
    if (!event) {
        return;
    }
    
    // 触发事件
    event_if->event_set(event);
    
    // 等待事件
    DWORD result = event_if->event_wait_single(event, 1000);
    if (result == WAIT_OBJECT_0) {
        printf("事件已触发\n");
    }
    
    // 清理
    event_if->event_destroy(event);
}
```

### 使用APC接口

```c
#include "CN_platform_windows_interrupt.h"

VOID CALLBACK my_apc_func(ULONG_PTR param)
{
    printf("APC回调执行，参数: %llu\n", param);
}

void use_windows_apc_interface(void)
{
    Stru_CN_WindowsAPCInterface_t* apc_if = 
        CN_platform_windows_get_apc_interface();
    
    if (!apc_if) {
        return;
    }
    
    // 注册APC回调
    if (apc_if->apc_register_callback(my_apc_func, 12345)) {
        // 进入可警告等待状态以执行APC
        apc_if->apc_enter_alertable_wait();
        
        // 注销APC回调
        apc_if->apc_unregister_callback(my_apc_func);
    }
}
```

## 配置选项

### 编译配置

在Windows平台编译时，需要定义`_WIN32`宏：

```makefile
# Makefile配置
CFLAGS += -D_WIN32 -DWIN32_LEAN_AND_MEAN
LIBS += -lkernel32 -luser32 -ladvapi32
```

### 运行时配置

可以通过环境变量配置模块行为：

```bash
# 设置最大中断处理程序数量
set CN_WINDOWS_MAX_INTERRUPT_HANDLERS=512

# 启用调试日志
set CN_WINDOWS_INTERRUPT_DEBUG=1
```

## 性能考虑

### 1. 事件性能
- 事件对象是轻量级的，适合高频中断模拟
- 自动重置事件比手动重置事件性能更好
- 建议使用`WaitForMultipleObjects`处理多个事件

### 2. APC性能
- APC在目标线程上下文中执行，减少上下文切换
- 适合延迟敏感的中断处理
- 注意APC队列长度，避免队列溢出

### 3. I/O完成端口性能
- 适合高并发I/O操作
- 支持线程池，自动负载均衡
- 减少系统调用次数，提高吞吐量

## 错误处理

### 常见错误

1. **句柄创建失败**：检查系统资源限制
2. **内存分配失败**：检查内存使用情况
3. **线程同步错误**：检查临界区使用
4. **APC队列满**：减少APC频率或增加队列大小

### 错误恢复策略

```c
bool handle_interrupt_error(CN_InterruptNumber_t irq)
{
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_platform_windows_get_interrupt_controller();
    
    if (!controller) {
        return false;
    }
    
    // 尝试禁用中断
    controller->disable_interrupt(irq);
    
    // 重新初始化控制器
    controller->cleanup();
    if (!controller->initialize()) {
        return false;
    }
    
    // 重新注册处理程序
    // ... 重新注册代码 ...
    
    return true;
}
```

## 平台限制

### 已知限制

1. **硬件中断模拟**：Windows用户模式无法直接处理硬件中断
2. **实时性**：Windows不是实时操作系统，中断响应时间有波动
3. **优先级反转**：Windows线程调度可能导致优先级反转
4. **DLL注入**：某些安全软件可能阻止APC注入

### 解决方案

1. **使用内核驱动**：对于真正的硬件中断，需要开发内核驱动
2. **提高线程优先级**：使用实时优先级类提高响应性
3. **优先级继承**：使用优先级继承协议避免优先级反转
4. **白名单**：将应用程序添加到安全软件白名单

## 测试验证

### 单元测试

```c
// 测试中断注册
void test_interrupt_registration(void)
{
    CN_platform_windows_interrupt_initialize();
    
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_platform_windows_get_interrupt_controller();
    
    // 测试有效中断号
    CN_InterruptNumber_t valid_irq = 2001;
    bool result = controller->register_handler(valid_irq, dummy_handler, NULL, NULL);
    assert(result == true);
    
    // 测试无效中断号
    CN_InterruptNumber_t invalid_irq = 1000; // 低于最小值
    result = controller->register_handler(invalid_irq, dummy_handler, NULL, NULL);
    assert(result == false);
    
    CN_platform_windows_interrupt_cleanup();
}
```

### 集成测试

```c
// 测试中断触发和响应
void test_interrupt_trigger(void)
{
    CN_platform_windows_interrupt_initialize();
    
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_platform_windows_get_interrupt_controller();
    
    int interrupt_count = 0;
    CN_InterruptNumber_t irq = 2002;
    
    // 注册处理程序
    controller->register_handler(irq, counting_handler, &interrupt_count, NULL);
    controller->enable_interrupt(irq);
    
    // 模拟中断触发（实际应用中由外部事件触发）
    // 这里使用Windows事件模拟
    HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
    SetEvent(event); // 模拟中断触发
    
    // 等待处理
    Sleep(100); // 给处理程序时间执行
    
    assert(interrupt_count > 0);
    
    CloseHandle(event);
    CN_platform_windows_interrupt_cleanup();
}
```

## 维护指南

### 代码维护

1. **保持接口兼容性**：修改接口时保持向后兼容
2. **资源泄漏检查**：使用工具检查句柄和内存泄漏
3. **线程安全验证**：多线程环境下测试所有功能
4. **性能监控**：监控中断处理延迟和吞吐量

### 版本升级

1. **接口版本**：修改接口时更新版本号
2. **迁移指南**：提供旧版本到新版本的迁移指南
3. **兼容性测试**：测试与旧版本客户端的兼容性
4. **文档更新**：及时更新API文档和示例

## 相关文档

- [CN_interrupt API文档](../../../docs/api/infrastructure/platform/CN_interrupt_API.md)
- [Windows平台接口文档](../CN_platform_windows.h)
- [架构设计原则](../../../../docs/architecture/架构设计原则.md)
- [编码标准](../../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language开发团队

## 更新历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-03 | 初始版本，实现基本中断处理功能 |
| 1.1.0 | 2026-01-03 | 添加APC和I/O完成端口支持 |
| 1.2.0 | 2026-01-03 | 优化性能，添加错误恢复机制 |

---
*最后更新: 2026年1月3日*
