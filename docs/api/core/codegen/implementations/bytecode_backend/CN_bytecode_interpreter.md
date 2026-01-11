# CN_Bytecode_Interpreter API 文档

## 概述

字节码解释器是CN_Language虚拟机的核心执行引擎，负责加载和执行字节码程序。该模块实现了完整的栈式虚拟机，支持字节码程序的解释执行、调试、性能分析和错误处理等功能。

## 设计原则

1. **模块化设计**：解释器采用模块化设计，各个组件职责清晰
2. **接口抽象**：通过抽象接口实现模块解耦，支持不同的实现
3. **可扩展性**：支持插件式扩展，可添加新的指令和运行时功能
4. **安全性**：内置安全检查，防止恶意代码执行
5. **性能优化**：采用高效的指令分发和内存管理策略

## 数据结构

### Stru_BytecodeInterpreterConfig_t

字节码解释器配置结构体，用于配置解释器的运行时参数。

```c
typedef struct Stru_BytecodeInterpreterConfig_t {
    size_t stack_size;               ///< 栈大小（字节）
    size_t heap_size;                ///< 堆大小（字节）
    size_t max_call_depth;           ///< 最大调用深度
    size_t max_instruction_count;    ///< 最大指令执行数（防止无限循环）
    bool enable_gc;                  ///< 是否启用垃圾回收
    bool enable_profiling;           ///< 是否启用性能分析
    bool enable_debugging;           ///< 是否启用调试支持
    bool enable_tracing;             ///< 是否启用执行跟踪
    const char* log_level;           ///< 日志级别 ("none", "error", "warning", "info", "debug")
} Stru_BytecodeInterpreterConfig_t;
```

**字段说明**：
- `stack_size`：值栈的大小，决定可以同时存储多少个值
- `heap_size`：堆内存的大小，用于动态分配对象
- `max_call_depth`：最大函数调用深度，防止栈溢出
- `max_instruction_count`：最大指令执行数，防止无限循环
- `enable_gc`：是否启用自动垃圾回收
- `enable_profiling`：是否启用性能分析，收集执行统计信息
- `enable_debugging`：是否启用调试支持，支持断点和单步执行
- `enable_tracing`：是否启用执行跟踪，记录指令执行历史
- `log_level`：日志级别，控制日志输出的详细程度

### Eum_BytecodeValueType

字节码值类型枚举，定义解释器中支持的所有值类型。

```c
typedef enum Eum_BytecodeValueType {
    Eum_BCV_NULL = 0,        ///< 空值
    Eum_BCV_BOOL,            ///< 布尔值
    Eum_BCV_INT8,            ///< 8位整数
    Eum_BCV_INT16,           ///< 16位整数
    Eum_BCV_INT32,           ///< 32位整数
    Eum_BCV_INT64,           ///< 64位整数
    Eum_BCV_UINT8,           ///< 8位无符号整数
    Eum_BCV_UINT16,          ///< 16位无符号整数
    Eum_BCV_UINT32,          ///< 32位无符号整数
    Eum_BCV_UINT64,          ///< 64位无符号整数
    Eum_BCV_FLOAT32,         ///< 32位浮点数
    Eum_BCV_FLOAT64,         ///< 64位浮点数
    Eum_BCV_STRING,          ///< 字符串
    Eum_BCV_ARRAY,           ///< 数组
    Eum_BCV_OBJECT,          ///< 对象
    Eum_BCV_FUNCTION,        ///< 函数
    Eum_BCV_NATIVE_FUNCTION, ///< 原生函数
    Eum_BCV_CLOSURE,         ///< 闭包
    Eum_BCV_POINTER          ///< 指针
} Eum_BytecodeValueType;
```

### Stru_BytecodeValue_t

字节码值结构体，表示解释器中的值，使用联合体存储不同类型的值。

```c
typedef struct Stru_BytecodeValue_t {
    Eum_BytecodeValueType type;      ///< 值类型
    union {
        bool bool_value;             ///< 布尔值
        int8_t int8_value;           ///< 8位整数
        int16_t int16_value;         ///< 16位整数
        int32_t int32_value;         ///< 32位整数
        int64_t int64_value;          ///< 64位整数
        uint8_t uint8_value;         ///< 8位无符号整数
        uint16_t uint16_value;       ///< 16位无符号整数
        uint32_t uint32_value;       ///< 32位无符号整数
        uint64_t uint64_value;       ///< 64位无符号整数
        float float32_value;         ///< 32位浮点数
        double float64_value;        ///< 64位浮点数
        char* string_value;          ///< 字符串值
        void* pointer_value;         ///< 指针值
        void* object_value;          ///< 对象值
        void* array_value;           ///< 数组值
        void* function_value;        ///< 函数值
    } data;                          ///< 值数据
    size_t ref_count;                ///< 引用计数（用于垃圾回收）
    void* extra_data;                ///< 额外数据
} Stru_BytecodeValue_t;
```

### Eum_BytecodeInterpreterState

字节码解释器状态枚举，定义解释器的执行状态。

```c
typedef enum Eum_BytecodeInterpreterState {
    Eum_BCI_STATE_CREATED = 0,       ///< 已创建
    Eum_BCI_STATE_INITIALIZED,       ///< 已初始化
    Eum_BCI_STATE_RUNNING,           ///< 运行中
    Eum_BCI_STATE_PAUSED,            ///< 已暂停
    Eum_BCI_STATE_STEPPING,          ///< 单步执行中
    Eum_BCI_STATE_BREAKPOINT,        ///< 断点命中
    Eum_BCI_STATE_ERROR,             ///< 错误状态
    Eum_BCI_STATE_FINISHED,          ///< 执行完成
    Eum_BCI_STATE_TERMINATED         ///< 已终止
} Eum_BytecodeInterpreterState;
```

### Stru_BytecodeInterpreter_t

字节码解释器结构体，表示解释器的实例，包含执行状态和运行时数据。

```c
typedef struct Stru_BytecodeInterpreter_t {
    // 状态信息
    Eum_BytecodeInterpreterState state;          ///< 当前状态
    bool has_errors;                             ///< 是否有错误
    bool has_warnings;                           ///< 是否有警告
    
    // 程序信息
    Stru_BytecodeProgram_t* program;             ///< 当前执行的字节码程序
    uint32_t program_counter;                    ///< 程序计数器（指令索引）
    uint32_t call_stack_depth;                   ///< 调用栈深度
    
    // 运行时数据
    Stru_BytecodeValue_t* stack;                 ///< 值栈
    size_t stack_top;                            ///< 栈顶指针
    size_t stack_capacity;                       ///< 栈容量
    
    Stru_BytecodeValue_t* globals;               ///< 全局变量数组
    size_t global_count;                         ///< 全局变量数量
    
    void** call_stack;                           ///< 调用栈（帧指针数组）
    size_t call_stack_capacity;                  ///< 调用栈容量
    
    // 调试信息
    bool breakpoints_enabled;                    ///< 断点是否启用
    uint32_t* breakpoints;                       ///< 断点数组（指令偏移）
    size_t breakpoint_count;                     ///< 断点数量
    
    // 性能分析
    uint64_t instruction_count;                  ///< 已执行指令计数
    uint64_t start_time;                         ///< 开始时间（毫秒）
    uint64_t total_time;                         ///< 总执行时间（毫秒）
    
    // 错误处理
    char** error_messages;                       ///< 错误消息数组
    size_t error_count;                          ///< 错误数量
    size_t error_capacity;                       ///< 错误数组容量
    
    // 内部状态
    void* internal_state;                        ///< 内部状态
} Stru_BytecodeInterpreter_t;
```

## API 函数

### 解释器生命周期管理

#### F_create_bytecode_interpreter

```c
Stru_BytecodeInterpreter_t* F_create_bytecode_interpreter(
    const Stru_BytecodeInterpreterConfig_t* config);
```

**功能**：创建字节码解释器实例。

**参数**：
- `config`：解释器配置，如果为NULL则使用默认配置

**返回值**：新创建的字节码解释器实例，失败返回NULL。

**示例**：
```c
Stru_BytecodeInterpreterConfig_t config = F_create_default_bytecode_interpreter_config();
config.stack_size = 65536;      // 64KB栈
config.heap_size = 1048576;     // 1MB堆
config.enable_debugging = true; // 启用调试

Stru_BytecodeInterpreter_t* interpreter = F_create_bytecode_interpreter(&config);
if (!interpreter) {
    printf("创建解释器失败\n");
    return;
}
```

#### F_destroy_bytecode_interpreter

```c
void F_destroy_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：销毁字节码解释器实例，释放所有资源。

**参数**：
- `interpreter`：要销毁的解释器实例

**注意**：调用此函数后，解释器指针不再有效。

#### F_reset_bytecode_interpreter

```c
void F_reset_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：重置解释器到初始状态，清除所有运行时数据。

**参数**：
- `interpreter`：要重置的解释器实例

### 程序加载和执行

#### F_load_bytecode_program

```c
bool F_load_bytecode_program(Stru_BytecodeInterpreter_t* interpreter,
                             Stru_BytecodeProgram_t* program);
```

**功能**：将字节码程序加载到解释器中，准备执行。

**参数**：
- `interpreter`：字节码解释器
- `program`：要加载的字节码程序

**返回值**：加载成功返回true，失败返回false。

**注意**：解释器会复制程序数据，调用者可以安全地释放原始程序。

#### F_execute_bytecode_program

```c
bool F_execute_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：执行已加载的字节码程序。

**参数**：
- `interpreter`：字节码解释器

**返回值**：执行成功返回true，失败返回false。

**注意**：此函数会阻塞直到程序执行完成或发生错误。

#### F_step_bytecode_program

```c
bool F_step_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：单步执行字节码程序，执行下一条指令并暂停。

**参数**：
- `interpreter`：字节码解释器

**返回值**：执行成功返回true，失败返回false。

#### F_continue_bytecode_program

```c
bool F_continue_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：从当前暂停状态继续执行字节码程序。

**参数**：
- `interpreter`：字节码解释器

**返回值**：继续执行成功返回true，失败返回false。

#### F_pause_bytecode_program

```c
bool F_pause_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：暂停当前正在执行的字节码程序。

**参数**：
- `interpreter`：字节码解释器

**返回值**：暂停成功返回true，失败返回false。

#### F_stop_bytecode_program

```c
bool F_stop_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：停止当前正在执行的字节码程序。

**参数**：
- `interpreter`：字节码解释器

**返回值**：停止成功返回true，失败返回false。

### 状态查询和错误处理

#### F_get_interpreter_state

```c
Eum_BytecodeInterpreterState F_get_interpreter_state(
    const Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：获取字节码解释器的当前状态。

**参数**：
- `interpreter`：字节码解释器

**返回值**：解释器状态。

#### F_interpreter_has_errors

```c
bool F_interpreter_has_errors(const Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：检查字节码解释器是否有错误。

**参数**：
- `interpreter`：字节码解释器

**返回值**：有错误返回true，否则返回false。

#### F_get_interpreter_errors

```c
void F_get_interpreter_errors(const Stru_BytecodeInterpreter_t* interpreter,
                              char*** errors, size_t* error_count);
```

**功能**：获取字节码解释器的所有错误信息。

**参数**：
- `interpreter`：字节码解释器
- `errors`：输出参数，错误信息数组
- `error_count`：输出参数，错误数量

**注意**：调用者负责释放错误信息数组。

#### F_clear_interpreter_errors

```c
void F_clear_interpreter_errors(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：清除字节码解释器的所有错误信息。

**参数**：
- `interpreter`：字节码解释器

### 调试功能

#### F_set_breakpoint

```c
bool F_set_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                      uint32_t instruction_offset);
```

**功能**：在指定指令偏移处设置断点。

**参数**：
- `interpreter`：字节码解释器
- `instruction_offset`：指令偏移（从程序开始）

**返回值**：设置成功返回true，失败返回false。

#### F_clear_breakpoint

```c
bool F_clear_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                        uint32_t instruction_offset);
```

**功能**：清除指定指令偏移处的断点。

**参数**：
- `interpreter`：字节码解释器
- `instruction_offset`：指令偏移

**返回值**：清除成功返回true，失败返回false。

#### F_clear_all_breakpoints

```c
void F_clear_all_breakpoints(Stru_BytecodeInterpreter_t* interpreter);
```

**功能**：清除所有断点。

**参数**：
- `interpreter`：字节码解释器

#### F_get_current_stack_frame

```c
bool F_get_current_stack_frame(const Stru_BytecodeInterpreter_t* interpreter,
                               size_t frame_index,
                               const char** function_name,
                               uint32_t* line_number,
                               uint32_t* instruction_offset);
```

**功能**：获取当前调用栈帧的信息。

**参数**：
- `interpreter`：字节码解释器
- `frame_index`：栈帧索引（0表示当前帧，1表示调用者，依此类推）
- `function_name`：输出参数，函数名
- `line_number`：输出参数，行号
- `instruction_offset`：输出参数，指令偏移

**返回值**：获取成功返回true，失败返回false。

#### F_get_stack_trace

```c
bool F_get_stack_trace(const Stru_BytecodeInterpreter_t* interpreter,
                       void*** frames, size_t* frame_count);
```

**功能**：获取当前的调用栈跟踪。

**参数**：
- `interpreter`：字节码解释器
- `frames`：输出参数，栈帧数组
- `frame_count`：输出参数，栈帧数量

**返回值**：获取成功返回true，失败返回false。

**注意**：调用者负责释放栈帧数组。

### 性能分析

#### F_get_profiling_data

```c
void F_get_profiling_data(const Stru_BytecodeInterpreter_t* interpreter,
                          uint64_t* instruction_count,
                          uint64_t* execution_time,
                          size_t* memory_usage);
```

**功能**：获取字节码解释器的性能分析数据。

**参数**：
- `interpreter`：字节码解释器
- `instruction_count`：输出参数，已执行指令计数
- `execution_time`：输出参数，执行时间（毫秒）
- `memory_usage`：输出参数，内存使用量（字节）

### 辅助函数

#### F_create_default_bytecode_interpreter_config

```c
Stru_BytecodeInterpreterConfig_t F_create_default_bytecode_interpreter_config(void);
```

**功能**：创建并返回默认的字节码解释器配置。

**返回值**：默认字节码解释器配置。

**默认配置**：
- `stack_size`：65536（64KB）
- `heap_size`：1048576（1MB）
- `max_call_depth`：256
- `max_instruction_count`：1000000
- `enable_gc`：true
- `enable_profiling`：false
- `enable_debugging`：false
- `enable_tracing`：false
- `log_level`："warning"

#### F_get_bytecode_interpreter_version

```c
void F_get_bytecode_interpreter_version(int* major, int* minor, int* patch);
```

**功能**：返回字节码解释器的版本信息。

**参数**：
- `major`：输出参数，主版本号
- `minor`：输出参数，次版本号
- `patch`：输出参数，修订号

#### F_get_bytecode_interpreter_version_string
