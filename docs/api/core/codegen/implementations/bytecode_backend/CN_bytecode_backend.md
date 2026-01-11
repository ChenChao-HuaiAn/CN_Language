# CN_Bytecode_Backend API 文档

## 概述

字节码后端是CN_Language编译器的核心组件之一，负责将抽象语法树（AST）或中间代码表示（IR）转换为字节码程序。字节码是一种平台无关的中间表示形式，可以直接由CN_Language虚拟机解释执行。

## 设计原则

1. **模块化设计**：字节码后端采用模块化设计，各个组件职责清晰
2. **接口抽象**：通过抽象接口实现模块解耦
3. **可扩展性**：支持多种输入格式（AST、IR）和输出格式
4. **性能优化**：内置字节码优化功能

## 数据结构

### Stru_BytecodeBackendConfig_t

字节码后端配置结构体，用于配置字节码生成参数。

```c
typedef struct Stru_BytecodeBackendConfig_t {
    bool optimize_bytecode;          // 是否优化字节码
    int optimization_level;          // 优化级别 (0-3)
    bool include_debug_info;         // 是否包含调试信息
    size_t stack_size;               // 虚拟机栈大小（字节）
    size_t heap_size;                // 虚拟机堆大小（字节）
    bool enable_gc;                  // 是否启用垃圾回收
    bool enable_profiling;           // 是否启用性能分析
    const char* output_format;       // 输出格式 ("binary", "text", "hex")
} Stru_BytecodeBackendConfig_t;
```

### Stru_BytecodeProgram_t

字节码程序结构体，包含完整的字节码程序信息。

```c
typedef struct Stru_BytecodeProgram_t {
    Stru_BytecodeInstruction_t* instructions;  // 指令数组
    size_t instruction_count;                  // 指令数量
    size_t instruction_capacity;               // 指令数组容量
    
    char** constant_pool;                      // 常量池（字符串常量）
    size_t constant_count;                     // 常量数量
    size_t constant_capacity;                  // 常量池容量
    
    uint32_t* line_table;                      // 行号表（指令索引->源代码行号）
    size_t line_table_size;                    // 行号表大小
    
    Stru_DebugSymbol_t* debug_symbols;         // 调试符号表
    size_t debug_symbol_count;                 // 调试符号数量
    
    size_t stack_size;                         // 所需栈大小
    size_t heap_size;                          // 所需堆大小
    size_t global_count;                       // 全局变量数量
    
    const char* entry_point;                   // 入口点函数名
    uint32_t entry_point_offset;               // 入口点指令偏移
    
    void* internal_data;                       // 内部数据
} Stru_BytecodeProgram_t;
```

## API 函数

### F_generate_bytecode_from_ast

```c
Stru_BytecodeProgram_t* F_generate_bytecode_from_ast(
    Stru_AstNode_t* ast, 
    const Stru_BytecodeBackendConfig_t* config
);
```

**功能**：从抽象语法树生成字节码程序。

**参数**：
- `ast`：抽象语法树根节点
- `config`：字节码生成配置

**返回值**：生成的字节码程序指针，调用者负责销毁。

**示例**：
```c
Stru_BytecodeBackendConfig_t config = F_create_default_bytecode_backend_config();
config.optimize_bytecode = true;
config.optimization_level = 2;
config.include_debug_info = true;

Stru_BytecodeProgram_t* program = F_generate_bytecode_from_ast(ast, &config);
if (program) {
    // 使用生成的字节码程序
    F_destroy_bytecode_program(program);
}
```

### F_generate_bytecode_from_ir

```c
Stru_BytecodeProgram_t* F_generate_bytecode_from_ir(
    void* ir, 
    const Stru_BytecodeBackendConfig_t* config
);
```

**功能**：从中间代码表示生成字节码程序。

**参数**：
- `ir`：中间代码表示
- `config`：字节码生成配置

**返回值**：生成的字节码程序指针，调用者负责销毁。

### F_create_default_bytecode_backend_config

```c
Stru_BytecodeBackendConfig_t F_create_default_bytecode_backend_config(void);
```

**功能**：创建默认的字节码后端配置。

**返回值**：默认配置结构体。

### F_destroy_bytecode_program

```c
void F_destroy_bytecode_program(Stru_BytecodeProgram_t* program);
```

**功能**：销毁字节码程序，释放所有资源。

**参数**：
- `program`：要销毁的字节码程序

### F_serialize_bytecode_program

```c
bool F_serialize_bytecode_program(
    const Stru_BytecodeProgram_t* program, 
    uint8_t** data, 
    size_t* size
);
```

**功能**：将字节码程序序列化为二进制格式。

**参数**：
- `program`：字节码程序
- `data`：输出参数，序列化数据指针
- `size`：输出参数，数据大小

**返回值**：成功返回true，失败返回false。

### F_deserialize_bytecode_program

```c
Stru_BytecodeProgram_t* F_deserialize_bytecode_program(
    const uint8_t* data, 
    size_t size
);
```

**功能**：从二进制数据反序列化字节码程序。

**参数**：
- `data`：序列化数据
- `size`：数据大小

**返回值**：反序列化的字节码程序指针，失败返回NULL。

### F_optimize_bytecode_program

```c
Stru_BytecodeProgram_t* F_optimize_bytecode_program(
    Stru_BytecodeProgram_t* program, 
    int optimization_level
);
```

**功能**：对字节码程序应用优化。

**参数**：
- `program`：要优化的字节码程序
- `optimization_level`：优化级别 (0-3)

**返回值**：优化后的字节码程序指针。

### F_validate_bytecode_program

```c
bool F_validate_bytecode_program(
    const Stru_BytecodeProgram_t* program, 
    char*** errors, 
    size_t* error_count
);
```

**功能**：验证字节码程序的正确性和安全性。

**参数**：
- `program`：要验证的字节码程序
- `errors`：输出参数，错误信息数组
- `error_count`：输出参数，错误数量

**返回值**：验证通过返回true，失败返回false。

### F_format_bytecode_program

```c
char* F_format_bytecode_program(const Stru_BytecodeProgram_t* program);
```

**功能**：将字节码程序格式化为可读的文本格式。

**参数**：
- `program`：字节码程序

**返回值**：格式化后的文本字符串，调用者负责释放。

## 辅助函数

### F_get_bytecode_backend_version

```c
void F_get_bytecode_backend_version(int* major, int* minor, int* patch);
```

**功能**：获取字节码后端版本信息。

**参数**：
- `major`：输出参数，主版本号
- `minor`：输出参数，次版本号
- `patch`：输出参数，修订号

### F_get_bytecode_backend_version_string

```c
const char* F_get_bytecode_backend_version_string(void);
```

**功能**：获取字节码后端版本字符串。

**返回值**：版本字符串，格式为"主版本号.次版本号.修订号"。

### F_bytecode_backend_supports_feature

```c
bool F_bytecode_backend_supports_feature(const char* feature);
```

**功能**：检查字节码后端是否支持特定功能。

**参数**：
- `feature`：功能标识符

**返回值**：支持返回true，不支持返回false。

支持的功能标识符：
- "ast_to_bytecode"：支持从AST生成字节码
- "ir_to_bytecode"：支持从IR生成字节码
- "optimization"：支持字节码优化
- "debug_info"：支持调试信息生成
- "profiling"：支持性能分析
- "gc"：支持垃圾回收

### F_configure_bytecode_backend

```c
bool F_configure_bytecode_backend(const char* option, const char* value);
```

**功能**：配置字节码后端选项。

**参数**：
- `option`：选项名称
- `value`：选项值

**返回值**：配置成功返回true，失败返回false。

支持的选项：
- "optimization_level"：优化级别 (0-3)
- "debug_info"：是否生成调试信息 (true/false)
- "stack_size"：栈大小（字节）
- "heap_size"：堆大小（字节）
- "enable_gc"：是否启用垃圾回收 (true/false)
- "output_format"：输出格式 ("binary", "text", "hex")

### F_create_bytecode_backend_interface

```c
Stru_CodeGeneratorInterface_t* F_create_bytecode_backend_interface(void);
```

**功能**：创建字节码后端代码生成器接口。

**返回值**：字节码后端代码生成器接口实例。

## 使用示例

### 基本使用

```c
#include "src/core/codegen/implementations/bytecode_backend/CN_bytecode_backend.h"

int main() {
    // 创建默认配置
    Stru_BytecodeBackendConfig_t config = F_create_default_bytecode_backend_config();
    config.optimize_bytecode = true;
    config.optimization_level = 2;
    config.include_debug_info = true;
    
    // 从AST生成字节码
    Stru_BytecodeProgram_t* program = F_generate_bytecode_from_ast(ast, &config);
    if (!program) {
        printf("字节码生成失败\n");
        return 1;
    }
    
    // 验证字节码
    char** errors = NULL;
    size_t error_count = 0;
    if (!F_validate_bytecode_program(program, &errors, &error_count)) {
        printf("字节码验证失败，错误数量：%zu\n", error_count);
        for (size_t i = 0; i < error_count; i++) {
            printf("错误 %zu: %s\n", i + 1, errors[i]);
        }
        // 释放错误信息
        for (size_t i = 0; i < error_count; i++) {
            free(errors[i]);
        }
        free(errors);
        F_destroy_bytecode_program(program);
        return 1;
    }
    
    // 序列化字节码
    uint8_t* serialized_data = NULL;
    size_t data_size = 0;
    if (F_serialize_bytecode_program(program, &serialized_data, &data_size)) {
        printf("字节码序列化成功，大小：%zu 字节\n", data_size);
        // 保存到文件或传输
        free(serialized_data);
    }
    
    // 格式化输出
    char* formatted = F_format_bytecode_program(program);
    if (formatted) {
        printf("格式化字节码：\n%s\n", formatted);
        free(formatted);
    }
    
    // 清理
    F_destroy_bytecode_program(program);
    return 0;
}
```

### 高级使用：优化和验证

```c
// 创建字节码程序
Stru_BytecodeProgram_t* program = F_generate_bytecode_from_ast(ast, &config);

// 应用优化
Stru_BytecodeProgram_t* optimized = F_optimize_bytecode_program(program, 3);
if (optimized) {
    printf("优化成功，指令数量从 %zu 减少到 %zu\n", 
           program->instruction_count, 
           optimized->instruction_count);
    
    // 验证优化后的程序
    char** errors = NULL;
    size_t error_count = 0;
    if (F_validate_bytecode_program(optimized, &errors, &error_count)) {
        printf("优化后的字节码验证通过\n");
    }
    
    // 清理
    F_destroy_bytecode_program(optimized);
}

F_destroy_bytecode_program(program);
```

## 错误处理

字节码后端使用以下错误处理机制：

1. **返回值检查**：所有可能失败的函数都通过返回值指示成功或失败
2. **错误信息**：验证函数返回详细的错误信息
3. **资源管理**：所有分配的资源都有对应的释放函数

## 性能考虑

1. **内存使用**：字节码程序可能占用较多内存，特别是包含调试信息时
2. **优化级别**：高级别优化可能增加生成时间，但减少执行时间
3. **序列化开销**：序列化和反序列化操作有性能开销

## 兼容性

字节码后端与以下组件兼容：

1. **CN_Language AST**：支持所有AST节点类型
2. **中间代码表示**：支持TAC、SSA等IR格式
3. **字节码解释器**：生成的字节码可以直接由CN_bytecode_interpreter执行

## 版本历史

- 1.0.0 (2026-01-11)：初始版本，支持基本字节码生成功能
- 1.1.0 (计划)：支持更多优化和调试功能

## 相关文档

- [字节码指令集](CN_bytecode_instructions.md)
- [字节码解释器API](CN_bytecode_interpreter.md)
- [字节码优化器API](../optimizers/README.md)

---

*最后更新：2026年1月11日*
*版本：1.0.0*
