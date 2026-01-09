# CN_Language 运行时系统接口 API 文档

## 概述

运行时系统接口是CN_Language项目的核心组件之一，负责程序的执行、内存管理和调试支持。本接口采用面向接口的编程模式，提供完整的抽象接口定义，支持多种运行时策略（解释执行、JIT编译、垃圾回收等）。

## 模块化架构

运行时系统采用模块化分层接口设计：

```
src/core/runtime/
├── factory/                         # 工厂模块
│   ├── CN_runtime_factory.h               # 主运行时接口
│   └── CN_runtime_factory.c               # 主运行时实现
├── memory/                          # 内存管理模块
│   ├── CN_runtime_memory_interface.h      # 内存管理接口
│   └── CN_runtime_memory_interface.c      # 内存管理实现
├── execution/                       # 执行引擎模块
│   ├── CN_execution_engine_interface.h    # 执行引擎接口
│   └── CN_execution_engine_interface.c    # 执行引擎实现
├── debug/                           # 调试支持模块
│   ├── CN_debug_support_interface.h       # 调试支持接口
│   └── CN_debug_support_interface.c       # 调试支持实现
└── vm/                              # 虚拟机模块
    ├── CN_virtual_machine_interface.h     # 虚拟机接口
    └── CN_virtual_machine_interface.c     # 虚拟机实现
```

## 接口定义

### Stru_RuntimeInterface_t (主运行时接口)

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
    
    // 系统控制
    void (*reset)(Stru_RuntimeInterface_t* runtime);
    
    // 资源管理
    void (*destroy)(Stru_RuntimeInterface_t* runtime);
    
    // 私有数据
    void* private_data;
} Stru_RuntimeInterface_t;
```

### Stru_RuntimeMemoryInterface_t (内存管理接口)

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
    
    // 内存释放
    void (*free_object)(Stru_RuntimeMemoryInterface_t* memory_interface,
                       void* object);
    
    // 垃圾回收
    void (*mark_object)(Stru_RuntimeMemoryInterface_t* memory_interface, void* object);
    size_t (*collect_garbage)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    // 统计信息
    void (*get_memory_stats)(Stru_RuntimeMemoryInterface_t* memory_interface,
                            size_t* total_heap, size_t* used_heap,
                            size_t* object_count, size_t* gc_count);
    
    // 系统控制
    void (*reset)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    // 资源管理
    void (*destroy)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    // 私有数据
    void* private_data;
} Stru_RuntimeMemoryInterface_t;
```

### Stru_ExecutionEngineInterface_t (执行引擎接口)

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
    void (*unload_module)(Stru_ExecutionEngineInterface_t* engine,
                         void* module);
    void* (*execute_function)(Stru_ExecutionEngineInterface_t* engine,
                             void* module, const char* function_name,
                             int argc, void** argv);
    void* (*execute_bytecode)(Stru_ExecutionEngineInterface_t* engine,
                             const uint8_t* bytecode, size_t bytecode_size,
                             void* context);
    
    // 执行控制
    void (*set_timeout)(Stru_ExecutionEngineInterface_t* engine, uint64_t timeout_ms);
    
    // 统计信息
    void (*get_execution_stats)(Stru_ExecutionEngineInterface_t* engine,
                               uint64_t* instructions_executed,
                               uint64_t* execution_time_ms,
                               size_t* memory_used);
    
    // 系统控制
    void (*reset)(Stru_ExecutionEngineInterface_t* engine);
    
    // 资源管理
    void (*destroy)(Stru_ExecutionEngineInterface_t* engine);
    
    // 私有数据
    void* private_data;
} Stru_ExecutionEngineInterface_t;
```

### Stru_DebugSupportInterface_t (调试支持接口)

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

### Stru_VirtualMachineInterface_t (虚拟机接口)

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
    
    // 系统控制
    void (*reset)(Stru_VirtualMachineInterface_t* vm);
    
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

**头文件：** `src/core/runtime/factory/CN_runtime_factory.h`

**返回值：**
- `Stru_RuntimeInterface_t*`：新创建的运行时接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
#include "src/core/runtime/factory/CN_runtime_factory.h"

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

**头文件：** `src/core/runtime/memory/CN_runtime_memory_interface.h`

### F_create_execution_engine_interface

创建执行引擎接口实例。

**函数签名：**
```c
Stru_ExecutionEngineInterface_t* F_create_execution_engine_interface(void);
```

**头文件：** `src/core/runtime/execution/CN_execution_engine_interface.h`

### F_create_debug_support_interface

创建调试支持接口实例。

**函数签名：**
```c
Stru_DebugSupportInterface_t* F_create_debug_support_interface(void);
```

**头文件：** `src/core/runtime/debug/CN_debug_support_interface.h`

### F_create_virtual_machine_interface

创建虚拟机接口实例。

**函数签名：**
```c
Stru_VirtualMachineInterface_t* F_create_virtual_machine_interface(void);
```

**头文件：** `src/core/runtime/vm/CN_virtual_machine_interface.h`

## 使用模式

### 基本使用模式

```c
#include "src/core/runtime/factory/CN_runtime_factory.h"

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
#include "src/core/runtime/memory/CN_runtime_memory_interface.h"

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
    
    // 释放对象
    memory->free_object(memory, object);
    memory->free_object(memory, array);
    
    // 重置内存管理器
    memory->reset(memory);
}
```

### 调试支持使用模式

```c
#include "src/core/runtime/debug/CN_debug_support_interface.h"

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
        printf("变量 result 的值: %s\n", value);
    }
    
    // 设置变量
    if (debugger->set_variable(debugger, "debug_mode", "true")) {
        printf("变量 debug_mode 已设置为 true\n");
    }
    
    // 获取调用栈
    void* call_stack[32];
    int stack_depth = debugger->get_call_stack(debugger, call_stack, 32);
    printf("调用栈深度: %d\n", stack_depth);
    
    // 等待调试事件
    Stru_DebugEvent_t event;
    if (debugger->get_next_event(debugger, &event, 1000)) {
        printf("收到调试事件: 类型=%d, 消息=%s\n", event.type, event.message);
    }
    
    // 继续执行
    debugger->continue_execution(debugger);
    
    // 清理
    debugger->destroy(debugger);
}
```

### 虚拟机使用模式

```c
#include "src/core/runtime/vm/CN_virtual_machine_interface.h"

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
    
    // 加载并执行字节码
    uint8_t bytecode[] = { /* 示例字节码 */ };
    size_t bytecode_size = sizeof(bytecode);
    
    int argc = 2;
    void* argv[] = { "arg1", "arg2" };
    
    int exit_code = vm->load_and_execute(vm, bytecode, bytecode_size, "example.cn", argc, argv);
    printf("程序退出码: %d\n", exit_code);
    
    // 获取组件接口
    Stru_RuntimeMemoryInterface_t* memory = vm->get_memory_interface(vm);
    Stru_ExecutionEngineInterface_t* engine = vm->get_execution_engine(vm);
    Stru_DebugSupportInterface_t* debugger = vm->get_debug_support(vm);
    
    if (memory != NULL && engine != NULL && debugger != NULL) {
        printf("虚拟机组件加载成功\n");
    }
    
    // 重置虚拟机状态
    vm->reset(vm);
    
    // 清理
    vm->destroy(vm);
}
```

## 错误处理

运行时系统使用统一的错误处理机制：

### 错误码定义

```c
typedef enum Eum_RuntimeError {
    Eum_RUNTIME_SUCCESS = 0,           // 成功
    Eum_RUNTIME_MEMORY_ERROR,          // 内存错误
    Eum_RUNTIME_EXECUTION_ERROR,       // 执行错误
    Eum_RUNTIME_DEBUGGER_ERROR,        // 调试器错误
    Eum_RUNTIME_VM_ERROR,              // 虚拟机错误
    Eum_RUNTIME_CONFIG_ERROR,          // 配置错误
    Eum_RUNTIME_TIMEOUT_ERROR,         // 超时错误
    Eum_RUNTIME_INVALID_ARGUMENT,      // 参数错误
    Eum_RUNTIME_NOT_INITIALIZED,       // 未初始化
    Eum_RUNTIME_ALREADY_INITIALIZED    // 已初始化
} Eum_RuntimeError;
```

### 错误处理示例

```c
#include "src/core/runtime/factory/CN_runtime_factory.h"

int handle_runtime_errors(void) {
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    if (runtime == NULL) {
        fprintf(stderr, "错误: 无法创建运行时系统 (内存不足)\n");
        return Eum_RUNTIME_MEMORY_ERROR;
    }
    
    // 初始化
    if (!runtime->initialize(runtime, NULL)) {
        fprintf(stderr, "错误: 运行时系统初始化失败\n");
        runtime->destroy(runtime);
        return Eum_RUNTIME_NOT_INITIALIZED;
    }
    
    // 执行程序
    uint8_t bytecode[] = { /* 字节码 */ };
    int result = runtime->execute_program(runtime, bytecode, sizeof(bytecode), 0, NULL);
    
    if (result != 0) {
        fprintf(stderr, "错误: 程序执行失败，退出码: %d\n", result);
    }
    
    runtime->destroy(runtime);
    return result == 0 ? Eum_RUNTIME_SUCCESS : Eum_RUNTIME_EXECUTION_ERROR;
}
```

## 性能优化建议

1. **内存管理优化**：
   - 对于频繁分配的小对象，使用对象池
   - 调整垃圾回收触发阈值以减少停顿时间
   - 使用区域分配器管理临时对象

2. **执行引擎优化**：
   - 启用JIT编译以提升热点代码性能
   - 使用内联缓存优化方法调用
   - 实现字节码优化器

3. **调试支持优化**：
   - 仅在需要时启用调试功能
   - 使用条件断点减少性能影响
   - 批量处理调试事件

## 兼容性说明

运行时系统接口设计为跨平台兼容：

- **操作系统**：支持Linux、Windows、macOS
- **编译器**：支持GCC、Clang、MSVC
- **架构**：支持x86、x86_64、ARM
- **标准**：符合C99标准

## 版本历史

### 版本 1.0.0 (2026-01-09)
- 初始版本发布
- 模块化接口设计
- 支持内存管理、执行引擎、调试支持、虚拟机
- 完整的API文档

## 相关文档

- [CN_Language 架构设计](../architecture/001-中文编程语言CN_Language开发规划.md)
- [CN_Language 语法规范](../../specifications/CN_Language%20语法规范.md)
- [CN_Language 编码标准](../../specifications/CN_Language项目%20技术规范和编码标准.md)

## 许可证

本接口遵循MIT许可证。详见项目根目录的LICENSE文件。

---
*文档最后更新: 2026-01-09*
