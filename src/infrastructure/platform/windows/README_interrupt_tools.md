# Windows平台中断工具模块

## 概述

Windows平台中断工具模块是CN_Language项目在Windows操作系统上的中断处理工具实现。该模块提供中断上下文管理、栈管理、延迟处理、中断过滤器、中断日志和性能分析等功能，支持跨编译器（MSVC/GCC）兼容性。

## 架构设计

### 模块结构

```
src/infrastructure/platform/windows/
├── CN_platform_windows_interrupt.h          # Windows中断处理接口定义
├── CN_platform_windows_interrupt_tools.c    # Windows中断工具实现
├── README_interrupt.md                      # 中断处理模块说明文档
└── README_interrupt_tools.md                # 本说明文档
```

### 设计原则

1. **跨编译器兼容**：支持MSVC和GCC/MinGW编译器
2. **线程安全**：使用临界区（CRITICAL_SECTION）确保多线程安全
3. **资源管理**：自动管理Windows句柄和内存资源
4. **性能优化**：提供性能分析工具，优化关键路径
5. **错误恢复**：完善的错误处理和恢复机制

## 核心功能

### 1. 中断上下文管理

提供中断上下文的保存、恢复和复制功能：

```c
// 保存当前中断上下文
Stru_CN_InterruptContext_t context;
windows_interrupt_tools_save_context(&context);

// 恢复中断上下文
windows_interrupt_tools_restore_context(&context);

// 复制中断上下文
Stru_CN_InterruptContext_t dest;
windows_interrupt_tools_copy_context(&dest, &context);
```

### 2. 栈管理

提供中断栈的分配、释放和溢出检查：

```c
// 分配中断栈
void* stack = windows_interrupt_tools_allocate_interrupt_stack(4096);

// 检查栈溢出
bool overflow = windows_interrupt_tools_check_stack_overflow(stack, 4096);

// 释放中断栈
windows_interrupt_tools_free_interrupt_stack(stack);
```

### 3. 延迟处理

支持中断处理的延迟执行：

```c
// 调度延迟处理
windows_interrupt_tools_schedule_deferred(handler, context, 100); // 100ms后执行

// 取消延迟处理
windows_interrupt_tools_cancel_deferred(handler);

// 处理延迟任务
windows_interrupt_tools_process_deferred();
```

### 4. 中断过滤器

提供中断过滤机制，允许选择性处理中断：

```c
// 安装中断过滤器
bool filter_func(CN_InterruptNumber_t irq, void* context) {
    return (irq != 0); // 过滤掉中断号0
}
windows_interrupt_tools_install_filter(irq, filter_func, NULL);

// 移除中断过滤器
windows_interrupt_tools_remove_filter(irq);
```

### 5. 中断日志

提供中断日志记录功能：

```c
// 启用中断日志
windows_interrupt_tools_enable_logging(irq, 1); // 日志级别1

// 禁用中断日志
windows_interrupt_tools_disable_logging(irq);

// 获取日志条目
Stru_CN_WindowsInterruptLogEntry_t log_buffer[100];
size_t entry_count;
windows_interrupt_tools_get_log_entries(irq, log_buffer, sizeof(log_buffer), &entry_count);
```

### 6. 性能分析

提供中断处理性能分析：

```c
// 开始性能分析
windows_interrupt_tools_start_profiling(irq);

// 停止性能分析
windows_interrupt_tools_stop_profiling(irq);

// 获取性能分析数据
Stru_CN_WindowsInterruptProfileData_t profile_data;
windows_interrupt_tools_get_profile_data(irq, &profile_data, sizeof(profile_data));
```

## 跨编译器支持

### MSVC支持

使用MSVC内部函数和内联汇编：

```c
#ifdef _MSC_VER
    ip = (uintptr_t)_ReturnAddress();
    sp = (uintptr_t)_AddressOfReturnAddress();
    __asm {
        mov sp, esp
        mov bp, ebp
    }
#endif
```

### GCC/MinGW支持

使用GCC内置函数和内联汇编：

```c
#ifndef _MSC_VER
    ip = (uintptr_t)__builtin_return_address(0);
    sp = (uintptr_t)__builtin_frame_address(0);
    __asm__ __volatile__ (
        "movl %%esp, %0\n"
        "movl %%ebp, %1\n"
        : "=r"(sp), "=r"(bp)
        :
        : "memory"
    );
#endif
```

## 接口说明

### 中断工具接口

Windows中断工具接口实现了`Stru_CN_InterruptToolsInterface_t`定义的所有功能：

- **上下文管理**：`save_context()` / `restore_context()` / `copy_context()`
- **栈管理**：`allocate_interrupt_stack()` / `free_interrupt_stack()` / `check_stack_overflow()`
- **延迟处理**：`schedule_deferred()` / `cancel_deferred()` / `process_deferred()`
- **中断过滤器**：`install_filter()` / `remove_filter()`
- **中断日志**：`enable_logging()` / `disable_logging()` / `get_log_entries()`
- **性能分析**：`start_profiling()` / `stop_profiling()` / `get_profile_data()`

## 使用示例

### 基本使用

```c
#include "CN_platform_windows_interrupt.h"

// 中断处理函数
void my_interrupt_handler(void* context)
{
    printf("中断处理函数执行\n");
}

int main()
{
    // 初始化Windows中断工具
    CN_platform_windows_interrupt_initialize();
    
    // 获取中断工具接口
    Stru_CN_InterruptToolsInterface_t* tools = 
        CN_platform_windows_get_interrupt_tools();
    
    // 分配中断栈
    void* stack = tools->allocate_interrupt_stack(4096);
    if (!stack) {
        fprintf(stderr, "无法分配中断栈\n");
        return -1;
    }
    
    // 检查栈溢出
    bool overflow = tools->check_stack_overflow(stack, 4096);
    if (overflow) {
        fprintf(stderr, "栈溢出检测\n");
    }
    
    // 调度延迟处理
    tools->schedule_deferred(my_interrupt_handler, NULL, 100);
    
    // 启用中断日志
    tools->enable_logging(2001, 1);
    
    // 开始性能分析
    tools->start_profiling(2001);
    
    // 处理延迟任务
    tools->process_deferred();
    
    // 停止性能分析
    tools->stop_profiling(2001);
    
    // 获取性能数据
    Stru_CN_WindowsInterruptProfileData_t profile_data;
    tools->get_profile_data(2001, &profile_data, sizeof(profile_data));
    
    printf("中断调用次数: %llu\n", profile_data.call_count);
    printf("平均处理时间: %llu ns\n", profile_data.avg_time);
    
    // 清理
    tools->free_interrupt_stack(stack);
    CN_platform_windows_interrupt_cleanup();
    
    return 0;
}
```

### 中断过滤器使用

```c
#include "CN_platform_windows_interrupt.h"

// 中断过滤器函数
bool my_interrupt_filter(CN_InterruptNumber_t irq, void* context)
{
    // 只允许中断号2000-2010
    return (irq >= 2000 && irq <= 2010);
}

void setup_interrupt_filters(void)
{
    Stru_CN_InterruptToolsInterface_t* tools = 
        CN_platform_windows_get_interrupt_tools();
    
    if (!tools) {
        return;
    }
    
    // 安装中断过滤器
    for (CN_InterruptNumber_t irq = 2000; irq <= 2010; irq++) {
        tools->install_filter(irq, my_interrupt_filter, NULL);
    }
    
    printf("中断过滤器已安装\n");
}
```

### 中断日志使用

```c
#include "CN_platform_windows_interrupt.h"

void analyze_interrupt_logs(void)
{
    Stru_CN_InterruptToolsInterface_t* tools = 
        CN_platform_windows_get_interrupt_tools();
    
    if (!tools) {
        return;
    }
    
    // 启用中断日志
    tools->enable_logging(2001, 2); // 日志级别2
    
    // 获取日志条目
    Stru_CN_WindowsInterruptLogEntry_t log_buffer[100];
    size_t entry_count;
    
    if (tools->get_log_entries(2001, log_buffer, sizeof(log_buffer), &entry_count)) {
        printf("找到 %zu 个日志条目:\n", entry_count);
        
        for (size_t i = 0; i < entry_count; i++) {
            printf("条目 %zu: 中断号=%u, 时间戳=%llu, 消息=%s\n",
                   i, log_buffer[i].interrupt, log_buffer[i].timestamp,
                   log_buffer[i].message);
        }
    }
    
    // 禁用中断日志
    tools->disable_logging(2001);
}
```

## 配置选项

### 编译配置

支持MSVC和GCC/MinGW编译器：

```makefile
# MSVC编译配置
CLFLAGS = /D_WIN32 /D_MSC_VER /DWIN32_LEAN_AND_MEAN

# GCC/MinGW编译配置
CFLAGS = -D_WIN32 -DWIN32_LEAN_AND_MEAN

# 链接库
LIBS = -lkernel32 -luser32
```

### 运行时配置

可以通过宏定义配置模块行为：

```c
// 最大中断处理程序数量
#define CN_WINDOWS_MAX_INTERRUPT_HANDLERS 256

// 中断日志缓冲区大小
#define CN_WINDOWS_INTERRUPT_LOG_BUFFER_SIZE 1024
```

## 性能优化

### 1. 上下文保存优化

- 使用编译器内置函数获取返回地址和栈帧
- 避免不必要的寄存器保存
- 使用条件编译优化不同架构

### 2. 栈管理优化

- 使用VirtualAlloc分配可执行栈内存
- 设置栈保护页检测溢出
- 智能栈使用率检查

### 3. 延迟处理优化

- 按计划时间排序的延迟任务列表
- 高效的插入和删除算法
- 批量处理延迟任务

### 4. 性能分析优化

- 低开销的时间戳获取
- 增量式性能数据更新
- 线程安全的性能统计

## 错误处理

### 常见错误

1. **内存分配失败**：检查系统内存使用情况
2. **栈溢出**：增加栈大小或优化栈使用
3. **编译器不兼容**：检查编译器版本和配置
4. **线程同步错误**：检查临界区使用

### 错误恢复策略

```c
bool recover_from_tool_error(CN_InterruptNumber_t irq)
{
    Stru_CN_InterruptToolsInterface_t* tools = 
        CN_platform_windows_get_interrupt_tools();
    
    if (!tools) {
        return false;
    }
    
    // 禁用相关功能
    tools->disable_logging(irq);
    tools->stop_profiling(irq);
    
    // 清理延迟任务
    // 注意：这里需要遍历所有延迟任务
    // 实际实现中可能需要更复杂的清理逻辑
    
    // 重新初始化工具
    CN_platform_windows_interrupt_cleanup();
    if (!CN_platform_windows_interrupt_initialize()) {
        return false;
    }
    
    return true;
}
```

## 平台适配

### 架构适配

支持x86和x64架构：

```c
#ifdef _M_IX86
    // x86架构实现
    #ifdef _MSC_VER
        __asm { mov sp, esp }
    #else
        __asm__ __volatile__ ("movl %%esp, %0" : "=r"(sp));
    #endif
#else
    // x64架构实现
    #ifdef _MSC_VER
        sp = (uintptr_t)_AddressOfReturnAddress();
    #else
        sp = (uintptr_t)__builtin_frame_address(0);
    #endif
#endif
```

### 编译器适配

支持MSVC和GCC/MinGW编译器：

```c
#ifdef _MSC_VER
    #include <intrin.h>
    #define CN_INLINE_ASM __asm
#else
    #define CN_INLINE_ASM __asm__ __volatile__
#endif
```

## 测试验证

### 单元测试

```c
// 测试栈管理功能
void test_stack_management(void)
{
    CN_platform_windows_interrupt_initialize();
    
    Stru_CN_InterruptToolsInterface_t* tools = 
        CN_platform_windows_get_interrupt_tools();
    
    // 测试栈分配
    void* stack = tools->allocate_interrupt_stack(4096);
    assert(stack != NULL);
    
    // 测试栈溢出检查
    bool overflow = tools->check_stack_overflow(stack, 4096);
    assert(overflow == false); // 新分配的栈不应溢出
    
    // 测试栈释放
    bool freed = tools->free_interrupt_stack(stack);
    assert(freed == true);
    
    CN_platform_windows_interrupt_cleanup();
}
```

### 性能测试

```c
// 测试性能分析功能
void test_performance_profiling(void)
{
    CN_platform_windows_interrupt_initialize();
    
    Stru_CN_InterruptToolsInterface_t* tools = 
        CN_platform_windows_get_interrupt_tools();
    
    CN_InterruptNumber_t irq = 2001;
    
    // 开始性能分析
    tools->start_profiling(irq);
    
    // 模拟中断处理
    for (int i = 0; i < 1000; i++) {
        // 模拟中断处理工作
        Sleep(1);
    }
    
    // 停止性能分析
    tools->stop_profiling(irq);
    
    // 获取性能数据
    Stru_CN_WindowsInterruptProfileData_t profile_data;
    tools->get_profile_data(irq, &profile_data, sizeof(profile_data));
    
    printf("性能测试结果:\n");
    printf("调用次数: %llu\n", profile_data.call_count);
    printf("总时间: %llu ns\n", profile_data.total_time);
    printf("平均时间: %llu ns\n", profile_data.avg_time);
    printf("最大时间: %llu ns\n", profile_data.max_time);
    printf("最小时间: %llu ns\n", profile_data.min_time);
    
    CN_platform_windows_interrupt_cleanup();
}
```

## 维护指南

### 代码维护

1. **保持跨编译器兼容性**：修改代码时测试MSVC和GCC
2. **资源泄漏检查**：使用工具检查内存和句柄泄漏
3. **性能监控**：定期进行性能测试和优化
4. **错误处理完善**：添加适当的错误检查和恢复

### 版本升级

1. **接口兼容性**：修改接口时保持向后兼容
2. **编译器支持**：测试新版本编译器的兼容性
3. **平台适配**：测试新Windows版本的兼容性
4. **文档更新**：及时更新文档和示例

## 相关文档

- [Windows中断处理模块文档](./README_interrupt.md)
- [CN_interrupt API文档](../../../docs/api/infrastructure/platform/CN_interrupt_API.md)
- [架构设计原则](../../../../docs/architecture/架构设计原则.md)
- [编码标准](../../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language开发团队

## 更新历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-03 | 初始版本，实现基本中断工具功能 |
| 1.1.0 | 2026-01-03 | 添加跨编译器支持（MSVC/GCC） |
| 1.2.0 | 2026-01-03 | 优化性能，添加错误恢复机制 |
| 1.3.0 | 2026-01-03 | 完善文档，添加使用示例 |

---

*最后更新: 2026年1月3日*
