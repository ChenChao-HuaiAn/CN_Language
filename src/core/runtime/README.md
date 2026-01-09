# CN_Language 运行时系统模块

## 概述

运行时系统模块是CN_Language项目的核心组件之一，负责程序的执行、内存管理和调试支持。本模块采用分层架构和SOLID设计原则，提供可扩展、可替换的运行时实现。

## 模块结构

```
src/core/runtime/
├── memory/                          # 内存管理模块
│   ├── CN_runtime_memory_interface.h      # 内存管理接口定义
│   └── CN_runtime_memory_interface.c      # 内存管理接口实现
├── execution/                       # 执行引擎模块
│   ├── CN_execution_engine_interface.h    # 执行引擎接口定义
│   └── CN_execution_engine_interface.c    # 执行引擎接口实现
├── debug/                           # 调试支持模块
│   ├── CN_debug_support_interface.h       # 调试支持接口定义
│   └── CN_debug_support_interface.c       # 调试支持接口实现
├── vm/                              # 虚拟机模块
│   ├── CN_virtual_machine_interface.h     # 虚拟机接口定义
│   └── CN_virtual_machine_interface.c     # 虚拟机接口实现
├── factory/                         # 工厂模块
│   ├── CN_runtime_factory.h               # 运行时工厂接口定义
│   └── CN_runtime_factory.c               # 运行时工厂接口实现
└── README.md                        # 本文件
```

## 接口设计

运行时系统模块包含以下主要接口：

### 1. Stru_RuntimeMemoryInterface_t
运行时内存管理接口，提供：
- 对象和数组内存分配
- 内存释放
- 垃圾回收支持
- 内存使用统计
- 对象生命周期管理
- 内存管理器重置

### 2. Stru_ExecutionEngineInterface_t
执行引擎接口，提供：
- 字节码模块加载和卸载
- 函数执行
- 字节码片段执行
- 执行统计和超时控制
- 执行引擎重置

### 3. Stru_DebugSupportInterface_t
调试支持接口，提供：
- 断点设置和管理
- 单步执行控制
- 变量检查和修改
- 调用栈跟踪
- 调试事件处理

### 4. Stru_VirtualMachineInterface_t
虚拟机接口，整合：
- 内存管理
- 执行引擎
- 调试支持
- 提供完整的虚拟机功能
- 虚拟机状态重置

### 5. Stru_RuntimeInterface_t
主运行时接口，提供：
- 统一的运行时系统访问
- 组件生命周期管理
- 程序执行入口
- 运行时系统重置

## 使用示例

### 基本使用

```c
#include "src/core/runtime/factory/CN_runtime_factory.h"

int main() {
    // 创建运行时系统
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    
    // 初始化运行时系统
    if (!runtime->initialize(runtime, "{\"heap_size\": 1048576, \"use_gc\": true}")) {
        // 处理初始化失败
        return 1;
    }
    
    // 获取内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = runtime->get_memory_interface(runtime);
    
    // 分配对象
    void* object = memory->allocate_object(memory, 100, 1);
    
    // 使用对象...
    
    // 释放对象
    memory->free_object(memory, object);
    
    // 销毁运行时系统
    runtime->destroy(runtime);
    
    return 0;
}
```

### 执行程序

```c
#include "src/core/runtime/factory/CN_runtime_factory.h"
#include <stdio.h>

int main(int argc, char** argv) {
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    runtime->initialize(runtime, NULL);
    
    // 加载字节码文件（示例）
    FILE* file = fopen("program.cnb", "rb");
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t* bytecode = (uint8_t*)malloc(size);
    fread(bytecode, 1, size, file);
    fclose(file);
    
    // 执行程序
    int exit_code = runtime->execute_program(runtime, bytecode, size, argc, argv);
    
    free(bytecode);
    runtime->destroy(runtime);
    
    return exit_code;
}
```

### 调试支持

```c
#include "src/core/runtime/factory/CN_runtime_factory.h"

void debug_example(Stru_RuntimeInterface_t* runtime) {
    // 获取调试支持接口
    Stru_DebugSupportInterface_t* debugger = runtime->get_debug_support(runtime);
    
    // 设置断点
    Stru_Breakpoint_t breakpoint = {
        .module_name = "main.cn",
        .function_name = "calculate",
        .line_number = 42,
        .instruction_offset = 0,
        .enabled = true,
        .user_data = NULL
    };
    
    debugger->set_breakpoint(debugger, &breakpoint);
    
    // 单步执行
    debugger->step(debugger, 0); // 步入
    
    // 检查变量
    char value[256];
    debugger->inspect_variable(debugger, "result", value, sizeof(value));
    printf("变量值: %s\n", value);
}
```

## 设计原则

### 1. 单一职责原则
- 每个接口只负责一个功能领域
- 实现文件不超过500行
- 函数不超过50行

### 2. 开闭原则
- 通过抽象接口支持扩展
- 新的内存管理策略、执行引擎或调试器可以通过实现相应接口来添加

### 3. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过工厂函数创建实例

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活的功能集

## 配置选项

运行时系统支持以下配置选项（JSON格式）：

```json
{
    "heap_size": 1048576,           // 堆大小（字节）
    "use_gc": true,                 // 是否启用垃圾回收
    "use_jit": false,               // 是否启用JIT编译
    "debug_enabled": true,          // 是否启用调试支持
    "timeout_ms": 0,                // 执行超时（0表示无超时）
    "log_level": "info"             // 日志级别：debug, info, warn, error
}
```

## 性能考虑

### 内存使用
- 每个接口实例约64-128字节
- 私有数据大小取决于具体实现
- 垃圾回收器有额外的内存开销

### 执行性能
- 解释执行：中等性能
- JIT编译：高性能，但有启动开销
- 调试支持：有性能开销，建议生产环境禁用

### 线程安全
- 接口实例不是线程安全的
- 多线程使用需要外部同步
- 建议每个线程创建独立的运行时实例

## 扩展指南

### 添加新的内存管理策略

1. 创建新的内存管理接口实现
2. 实现所有接口函数
3. 提供工厂函数
4. 更新配置解析支持新策略

### 添加新的执行引擎

1. 创建新的执行引擎接口实现
2. 实现字节码加载和执行函数
3. 支持JIT编译（可选）
4. 提供工厂函数

### 添加新的调试功能

1. 扩展调试支持接口（如果需要新功能）
2. 实现新的调试功能
3. 保持向后兼容性

## 测试策略

### 单元测试
- 测试每个接口函数
- 测试错误处理
- 测试边界条件

### 集成测试
- 测试接口组合使用
- 测试配置选项
- 测试性能基准

### 端到端测试
- 测试完整程序执行
- 测试调试功能
- 测试内存管理

## 相关文档

- [API文档](../../../docs/api/core/runtime/CN_runtime_interface.md)
- [架构设计文档](../../../docs/architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本发布 |
| 2.0.0 | 2026-01-09 | 模块化重构，拆分为子模块 |
| 2.1.0 | 2026-01-09 | 添加内存释放和重置功能 |

## 维护信息

- **最后更新**：2026年1月9日
- **维护者**：CN_Language运行时系统团队
- **联系方式**：runtime-team@cn-language.org

## 许可证

本模块遵循MIT许可证。详细信息请参阅项目根目录下的LICENSE文件。
