# CN_Language 运行时系统接口 API 文档

## 概述

运行时系统接口是CN_Language项目的核心组件之一，负责程序的执行、内存管理和调试支持。本接口采用面向接口的编程模式，提供完整的抽象接口定义，支持多种运行时策略（解释执行、JIT编译、垃圾回收等）。

## 接口架构

运行时系统采用分层接口设计：

```
┌─────────────────────────────────────────┐
│      Stru_RuntimeInterface_t            │ (主运行时接口)
├─────────────────────────────────────────┤
│  Stru_VirtualMachineInterface_t         │ (虚拟机接口)
├─────────────────────────────────────────┤
│  Stru_ExecutionEngineInterface_t        │ (执行引擎接口)
│  Stru_RuntimeMemoryInterface_t          │ (内存管理接口)
│  Stru_DebugSupportInterface_t           │ (调试支持接口)
└─────────────────────────────────────────┘
```

## 接口定义

### Stru_RuntimeInterface_t

运行时系统的主接口，提供统一的运行时访问。

```c
typedef struct Stru_RuntimeInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_RuntimeInterface_t* runtime, const char* config);
    
    // 组件访问
    Stru_RuntimeMemoryInterface_t* (*get_memory_interface)(Stru_RuntimeInterface_t* runtime);
    Stru_ExecutionEngineInterface_t* (*get_execution_engine)(Stru_RuntimeInterface_t* runtime);
    Stru_DebugSupportInterface_t* (*get_debug_support)(Stru_RuntimeInterface_t* runtime);
    Stru_VirtualMachineInterface_t* (*get_virtual_machine)(Stru_RuntimeInterface_t* runtime);
    
    // 程序执行
    int (*execute_program)(Stru_RuntimeInterface_t* runtime,
                          const uint8_t* bytecode, size_t bytecode_size,
                          int argc, char** argv);
    
    // 资源管理
    void (*destroy)(Stru_RuntimeInterface_t* runtime);
    
    // 私有数据
    void* private_data;
} Stru_RuntimeInterface_t;
```

### Stru_RuntimeMemoryInterface_t

运行时内存管理接口，专门为运行时系统设计。

```c
typedef struct Stru_RuntimeMemoryInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_RuntimeMemoryInterface_t* memory_interface, 
                      size_t heap_size, bool use_gc);
    
    // 内存分配
    void* (*allocate_object)(Stru_RuntimeMemoryInterface_t* memory_interface, 
                            size_t size, uint32_t type_tag);
    void* (*allocate_array)(Stru_RuntimeMemoryInterface_t* memory_interface,
                           size_t element_size, size_t element_count,
                           uint32_t type_tag);
    
    // 垃圾回收
    void (*mark_object)(Stru_RuntimeMemoryInterface_t* memory_interface, void* object);
    size_t (*collect_garbage)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    // 统计信息
    void (*get_memory_stats)(Stru_RuntimeMemoryInterface_t* memory_interface,
                            size_t* total_heap, size_t* used_heap,
                            size_t* object_count, size_t* gc_count);
    
    // 资源管理
    void (*destroy)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    // 私有数据
    void* private_data;
} Stru_RuntimeMemoryInterface_t;
```

### Stru_ExecutionEngineInterface_t

执行引擎接口，负责执行编译后的字节码。

```c
typedef struct Stru_ExecutionEngineInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_ExecutionEngineInterface_t* engine,
                      Stru_RuntimeMemoryInterface_t* memory_interface,
                      bool use_jit);
    
    // 字节码处理
    void* (*load_module)(Stru_ExecutionEngineInterface_t* engine,
                        const uint8_t* bytecode, size_t bytecode_size,
                        const char* module_name);
    void* (*execute_function)(Stru_ExecutionEngineInterface_t* engine,
                             void* module, const char* function_name,
                             int argc, void** argv);
    void* (*execute_bytecode)(Stru_ExecutionEngineInterface_t* engine,
                             const uint8_t* bytecode, size_t bytecode_size,
                             void* context);
    
    // 执行控制
    void (*set_timeout)(Stru_ExecutionEngineInterface_t* engine, uint64_t timeout_ms);
    void (*reset)(Stru_ExecutionEngineInterface_t* engine);
    
    // 统计信息
    void (*get_execution_stats)(Stru_ExecutionEngineInterface_t* engine,
                               uint64_t* instructions_executed,
                               uint64_t* execution_time_ms,
                               size_t* memory_used);
    
    // 资源管理
    void (*destroy)(Stru_ExecutionEngineInterface_t* engine);
    
    // 私有数据
    void* private_data;
} Stru_ExecutionEngineInterface_t;
```

### Stru_DebugSupportInterface_t

调试支持接口，提供运行时调试功能。

```c
typedef struct Stru_DebugSupportInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_DebugSupportInterface_t* debugger,
                      Stru_ExecutionEngineInterface_t* engine);
    
    // 断点管理
    bool (*set_breakpoint)(Stru_DebugSupportInterface_t* debugger,
                          const Stru_Breakpoint_t* breakpoint);
    bool (*clear_breakpoint)(Stru_DebugSupportInterface_t* debugger, int breakpoint_id);
    
    // 执行控制
    bool (*step)(Stru_DebugSupportInterface_t* debugger, int step_type);
    bool (*continue_execution)(Stru_DebugSupportInterface_t* debugger);
    bool (*pause_execution)(Stru_DebugSupportInterface_t* debugger);
    
    // 状态检查
    int (*get_call_stack)(Stru_DebugSupportInterface_t* debugger,
                         void* stack_buffer, size_t buffer_size);
    bool (*inspect_variable)(Stru_DebugSupportInterface_t* debugger,
                            const char* variable_name,
                            char* value_buffer, size_t buffer_size);
    bool (*set_variable)(Stru_DebugSupportInterface_t* debugger,
                        const char* variable_name, const char* value);
    
    // 事件处理
    bool (*get_next_event)(Stru_DebugSupportInterface_t* debugger,
                          Stru_DebugEvent_t* event, uint64_t timeout_ms);
    
    // 资源管理
    void (*destroy)(Stru_DebugSupportInterface_t* debugger);
    
    // 私有数据
    void* private_data;
} Stru_DebugSupportInterface_t;
```

### Stru_VirtualMachineInterface_t

虚拟机接口，整合内存管理、执行引擎和调试支持。

```c
typedef struct Stru_VirtualMachineInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_VirtualMachineInterface_t* vm,
                      size_t heap_size, bool use_gc, bool use_jit);
    
    // 程序执行
    int (*load_and_execute)(Stru_VirtualMachineInterface_t* vm,
                           const uint8_t* bytecode, size_t bytecode_size,
                           const char* module_name, int argc, void** argv);
    
    // 组件访问
    Stru_RuntimeMemoryInterface_t* (*get_memory_interface)(Stru_VirtualMachineInterface_t* vm);
    Stru_ExecutionEngineInterface_t* (*get_execution_engine)(Stru_VirtualMachineInterface_t* vm);
    Stru_DebugSupportInterface_t* (*get_debug_support)(Stru_VirtualMachineInterface_t* vm);
    
    // 资源管理
    void (*destroy)(Stru_VirtualMachineInterface_t* vm);
    
    // 私有数据
    void* private_data;
} Stru_VirtualMachineInterface_t;
```

## 辅助数据结构

### Stru_Breakpoint_t

断点信息结构体。

```c
typedef struct Stru_Breakpoint_t {
    const char* module_name;      // 模块名称
    const char* function_name;    // 函数名称
    size_t line_number;           // 行号
    size_t instruction_offset;    // 指令偏移
    bool enabled;                 // 是否启用
    void* user_data;              // 用户数据
} Stru_Breakpoint_t;
```

### Eum_DebugEventType

调试事件类型枚举。

```c
typedef enum Eum_DebugEventType {
    Eum_DEBUG_EVENT_BREAKPOINT_HIT,      // 断点命中
    Eum_DEBUG_EVENT_STEP_COMPLETE,       // 单步完成
    Eum_DEBUG_EVENT_EXCEPTION,           // 异常发生
    Eum_DEBUG_EVENT_PROGRAM_START,       // 程序开始
    Eum_DEBUG_EVENT_PROGRAM_END,         // 程序结束
    Eum_DEBUG_EVENT_FUNCTION_CALL,       // 函数调用
    Eum_DEBUG_EVENT_FUNCTION_RETURN,     // 函数返回
    Eum_DEBUG_EVENT_MEMORY_ACCESS        // 内存访问
} Eum_DebugEventType;
```

### Stru_DebugEvent_t

调试事件结构体。

```c
typedef struct Stru_DebugEvent_t {
    Eum_DebugEventType type;      // 事件类型
    void* location;               // 事件位置
    const char* message;          // 事件消息
    void* context;                // 事件上下文
    uint64_t timestamp;           // 时间戳
} Stru_DebugEvent_t;
```

## API 参考

### F_create_runtime_interface

创建主运行时接口实例。

**函数签名：**
```c
Stru_RuntimeInterface_t* F_create_runtime_interface(void);
```

**返回值：**
- `Stru_RuntimeInterface_t*`：新创建的运行时接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
if (runtime == NULL) {
    fprintf(stderr, "无法创建运行时系统\n");
    return 1;
}

// 使用运行时系统...

runtime->destroy(runtime);
```

### runtime->initialize

初始化运行时系统。

**函数签名：**
```c
bool initialize(Stru_RuntimeInterface_t* runtime, const char* config);
```

**参数：**
- `runtime`：运行时接口指针
- `config`：配置字符串（JSON格式），可为NULL使用默认配置

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

**配置示例：**
```json
{
    "heap_size": 1048576,
    "use_gc": true,
    "use_jit": false,
    "debug_enabled": true,
    "timeout_ms": 0,
    "log_level": "info"
}
```

### runtime->execute_program

执行CN语言程序。

**函数签名：**
```c
int execute_program(Stru_RuntimeInterface_t* runtime,
                   const uint8_t* bytecode, size_t bytecode_size,
                   int argc, char** argv);
```

**参数：**
- `runtime`：运行时接口指针
- `bytecode`：字节码数据
- `bytecode_size`：字节码大小
- `argc`：参数数量
- `argv`：参数数组

**返回值：**
- `int`：程序退出码（0表示成功，非0表示错误）

### F_create_runtime_memory_interface

创建运行时内存管理接口实例。

**函数签名：**
```c
Stru_RuntimeMemoryInterface_t* F_create_runtime_memory_interface(void);
```

### F_create_execution_engine_interface

创建执行引擎接口实例。

**函数签名：**
```c
Stru_ExecutionEngineInterface_t* F_create_execution_engine_interface(void);
```

### F_create_debug_support_interface

创建调试支持接口实例。

**函数签名：**
```c
Stru_DebugSupportInterface_t* F_create_debug_support_interface(void);
```

### F_create_virtual_machine_interface

创建虚拟机接口实例。

**函数签名：**
```c
Stru_VirtualMachineInterface_t* F_create_virtual_machine_interface(void);
```

## 使用模式

### 基本使用模式

```c
#include "CN_runtime_interface.h"

int main(int argc, char** argv) {
    // 创建运行时系统
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    if (runtime == NULL) {
        return 1;
    }
    
    // 初始化运行时系统
    const char* config = "{\"heap_size\": 1048576, \"use_gc\": true}";
    if (!runtime->initialize(runtime, config)) {
        runtime->destroy(runtime);
        return 1;
    }
    
    // 加载字节码文件
    FILE* file = fopen("program.cnb", "rb");
    if (file == NULL) {
        runtime->destroy(runtime);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t* bytecode = (uint8_t*)malloc(size);
    fread(bytecode, 1, size, file);
    fclose(file);
    
    // 执行程序
    int exit_code = runtime->execute_program(runtime, bytecode, size, argc, argv);
    
    // 清理
    free(bytecode);
    runtime->destroy(runtime);
    
    return exit_code;
}
```

### 内存管理使用模式

```c
void memory_management_example(Stru_RuntimeInterface_t* runtime) {
    // 获取内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = runtime->get_memory_interface(runtime);
    
    // 初始化内存管理器（1MB堆，启用垃圾回收）
    if (!memory->initialize(memory, 1024 * 1024, true)) {
        return;
    }
    
    // 分配对象
    void* object = memory->allocate_object(memory, 100, 1); // 类型标签为1
    if (object == NULL) {
        return;
    }
    
    // 分配数组
    void* array = memory->allocate_array(memory, sizeof(int), 10, 2); // 10个int，类型标签为2
    if (array == NULL) {
        memory->destroy(memory);
        return;
    }
    
    // 获取内存统计
    size_t total_heap, used_heap, object_count, gc_count;
    memory->get_memory_stats(memory, &total_heap, &used_heap, &object_count, &gc_count);
    
    printf("堆大小: %zu bytes\n", total_heap);
    printf("已使用: %zu bytes\n", used_heap);
    printf("对象数量: %zu\n", object_count);
    printf("垃圾回收次数: %zu\n", gc_count);
    
    // 执行垃圾回收
    size_t freed = memory->collect_garbage(memory);
    printf("回收了 %zu bytes\n", freed);
    
    // 注意：实际项目中需要正确管理对象生命周期
    // 这里简化示例，不实际释放对象
}
```

### 调试支持使用模式

```c
void debugging_example(Stru_RuntimeInterface_t* runtime) {
    // 获取调试支持接口
    Stru_DebugSupportInterface_t* debugger = runtime->get_debug_support(runtime);
    
    // 初始化调试器
    Stru_ExecutionEngineInterface_t* engine = runtime->get_execution_engine(runtime);
    if (!debugger->initialize(debugger, engine)) {
        return;
    }
    
    // 设置断点
    Stru_Breakpoint_t breakpoint = {
        .module_name = "main.cn",
        .function_name = "calculate",
        .line_number = 42,
        .instruction_offset = 0,
        .enabled = true,
        .user_data = NULL
    };
    
    if (!debugger->set_breakpoint(debugger, &breakpoint)) {
        printf("设置断点失败\n");
        return;
    }
    
    // 单步执行
    printf("开始单步执行...\n");
    debugger->step(debugger, 0); // 步入
    
    // 检查变量
    char value[256];
    if (debugger->inspect_variable(debugger, "result", value, sizeof(value))) {
        printf("变量值: %s\n", value);
    }
    
    // 获取调用栈
    void* stack_buffer[100];
    int frame_count = debugger->get_call_stack(debugger, stack_buffer, 100);
    printf("调用栈深度: %d\n", frame_count);
    
    // 等待调试事件
    Stru_DebugEvent_t event;
    if (debugger->get_next_event(debugger, &event, 1000)) { // 1秒超时
        printf("调试事件: %s\n", event.message);
    }
    
    // 继续执行
    debugger->continue_execution(debugger);
}
```

### 虚拟机使用模式

```c
void virtual_machine_example(void) {
    // 创建虚拟机
    Stru_VirtualMachineInterface_t* vm = F_create_virtual_machine_interface();
    if (vm == NULL) {
        return;
    }
    
    // 初始化虚拟机（1MB堆，启用垃圾回收，禁用JIT）
    if (!vm->initialize(vm, 1024 * 1024, true, false)) {
        vm->destroy(vm);
        return;
    }
    
    // 获取组件接口
    Stru_RuntimeMemoryInterface_t* memory = vm->get_memory_interface(vm);
    Stru_ExecutionEngineInterface_t* engine = vm->get_execution_engine(vm);
    Stru_DebugSupportInterface_t* debugger = vm->get_debug_support(vm);
    
    // 使用组件...
    
    // 加载并执行模块
    uint8_t bytecode[] = {0x01, 0x02, 0x03}; // 示例字节码
    int exit_code = vm->load_and_execute(vm, bytecode, sizeof(bytecode), 
                                        "test.cn", 0, NULL);
    
    printf("程序退出码: %d\n", exit_code);
    
    // 销毁虚拟机
    vm->destroy(vm);
}
```

## 错误处理

### 错误类型

运行时系统可能遇到以下类型的错误：

1. **初始化错误**：无效的配置参数或资源不足
2. **内存错误**：内存分配失败、堆溢出、内存泄漏
3. **执行错误**：无效字节码、执行超时、运行时异常
4. **调试错误**：断点设置失败、变量访问失败
5. **配置错误**：无效的JSON配置、不支持的选项
6. **状态错误**：在无效状态下调用方法

### 错误信息格式

错误信息包含以下内容：
- 错误类型描述
- 错误位置（模块、函数、行号）
- 相关上下文信息
- 建议的修复方法

**示例错误信息：**
```
内存分配失败：请求 1048576 字节，可用 524288 字节
位置：main.cn:calculate:42
建议：增加堆大小或启用垃圾回收
```

### 错误恢复策略

运行时系统实现以下错误恢复策略：

1. **优雅降级**：当高级功能失败时，回退到基本功能
2. **资源回收**：错误发生时自动回收已分配资源
3. **状态保存**：保存错误发生前的状态，便于调试
4. **错误传播**：错误通过返回值传递，不崩溃程序

## 性能考虑

### 内存使用

1. **接口实例**：每个接口约64-128字节
2. **私有数据**：取决于具体实现，通常256-1024字节
3. **堆管理**：垃圾回收器有额外的内存开销
4. **调试支持**：调试器需要存储断点和状态信息

### 时间复杂性

1. **初始化**：O(1)
2. **内存分配**：平均O(1)，最坏O(n)
3. **程序执行**：O(n)，其中n为指令数量
4. **垃圾回收**：O(m)，其中m为对象数量
5. **调试操作**：O(1) 到 O(k)，其中k为断点数量

### 优化建议

1. **批量操作**：对于大量对象，使用专用接口批量处理
2. **内存池**：对于频繁分配的小对象，使用内存池
3. **延迟初始化**：组件按需初始化，减少启动时间
4. **缓存策略**：缓存频繁访问的数据和计算结果
5. **异步操作**：长时间操作使用异步接口

## 线程安全性

### 线程安全级别

运行时系统接口是**非线程安全**的。多个线程不能同时访问同一个接口实例。

### 线程安全使用模式

```c
// 每个线程创建自己的运行时实例
void* thread_function(void* arg) {
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    runtime->initialize(runtime, NULL);
    
    // ... 使用运行时系统 ...
    
    runtime->destroy(runtime);
    return NULL;
}

// 或者使用互斥锁保护共享实例
pthread_mutex_t runtime_mutex = PTHREAD_MUTEX_INITIALIZER;
Stru_RuntimeInterface_t* shared_runtime = NULL;

void safe_runtime_operation(const uint8_t* bytecode, size_t size) {
    pthread_mutex_lock(&runtime_mutex);
    
    if (shared_runtime == NULL) {
        shared_runtime = F_create_runtime_interface();
        shared_runtime->initialize(shared_runtime, NULL);
    }
    
    int result = shared_runtime->execute_program(shared_runtime, 
                                                bytecode, size, 0, NULL);
    
    pthread_mutex_unlock(&runtime_mutex);
}
```

## 版本兼容性

### API 稳定性

运行时系统接口遵循以下版本兼容性规则：

1. **主版本号变更**：不兼容的API修改
2. **次版本号变更**：向下兼容的功能性新增
3. **修订号变更**：向下兼容的问题修正

### 向后兼容性保证

1. 现有函数签名不会改变
2. 现有枚举值不会删除
3. 结构体布局保持稳定
4. 错误代码含义不变

### 废弃策略

1. 废弃的API会标记为`DEPRECATED`
2. 废弃的API至少保留两个主版本
3. 提供迁移指南和替代方案

## 扩展指南

### 添加新的内存管理策略

1. 创建新的内存管理接口实现
2. 实现所有接口函数
3. 提供工厂函数
4. 更新配置解析支持新策略

**示例：添加分代垃圾回收器**
```c
// 分代垃圾回收器实现
typedef struct Stru_GenerationalGCData_t {
    // 实现细节...
} Stru_GenerationalGCData_t;

Stru_RuntimeMemoryInterface_t* F_create_generational_gc_interface(void) {
    // 创建并初始化分代垃圾回收器
}
```

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

测试每个接口函数的正确性：
- 正常情况测试
- 边界条件测试
- 错误处理测试
- 内存泄漏测试

### 集成测试

测试接口组合使用：
- 内存管理 + 执行引擎集成
- 执行引擎 + 调试支持集成
- 完整虚拟机测试

### 性能测试

建立性能基准：
- 内存分配性能
- 程序执行性能
- 垃圾回收性能
- 调试操作性能

### 兼容性测试

测试不同配置和环境的兼容性：
- 不同堆大小配置
- 启用/禁用垃圾回收
- 启用/禁用JIT编译
- 启用/禁用调试支持

## 相关文档

- [运行时系统模块 README](../../../src/core/runtime/README.md)
- [架构设计文档](../../../architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../specifications/CN_Language项目 技术规范和编码标准.md)
- [内存管理接口文档](../../infrastructure/memory/CN_memory.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本发布 |
| 1.0.1 | 2026-01-06 | 修复内存统计错误 |
| 1.1.0 | 2026-01-06 | 添加虚拟机接口 |
| 1.2.0 | 2026-01-06 | 添加调试事件支持 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
