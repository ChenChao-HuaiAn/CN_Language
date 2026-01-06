# CN_Codegen_Interface API 文档

## 概述

`CN_codegen_interface.h` 定义了CN_Language代码生成器的抽象接口。该接口提供了将抽象语法树（AST）转换为目标代码的完整功能，包括初始化、代码生成、优化和资源管理。

## 文件信息

- **文件名**: `CN_codegen_interface.h`
- **位置**: `src/core/codegen/`
- **版本**: 1.0.0
- **作者**: CN_Language架构委员会
- **创建日期**: 2026-01-06

## 包含文件

```c
#include "../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../ast/CN_ast.h"
```

## 枚举类型

### Eum_TargetCodeType

目标代码类型枚举，定义支持的目标代码格式。

```c
typedef enum Eum_TargetCodeType {
    Eum_TARGET_C,           ///< C语言代码
    Eum_TARGET_LLVM_IR,     ///< LLVM中间表示
    Eum_TARGET_X86_64,      ///< x86-64汇编
    Eum_TARGET_ARM64,       ///< ARM64汇编
    Eum_TARGET_WASM,        ///< WebAssembly
    Eum_TARGET_BYTECODE     ///< 自定义字节码
} Eum_TargetCodeType;
```

## 结构体类型

### Stru_CodeGenOptions_t

代码生成选项结构体，包含代码生成的所有配置参数。

```c
typedef struct Stru_CodeGenOptions_t {
    Eum_TargetCodeType target_type;     ///< 目标代码类型
    bool optimize;                      ///< 是否启用优化
    int optimization_level;             ///< 优化级别 (0-3)
    bool debug_info;                    ///< 是否生成调试信息
    const char* output_file;            ///< 输出文件名（可选）
    bool verbose;                       ///< 是否输出详细信息
} Stru_CodeGenOptions_t;
```

#### 字段说明

- `target_type`: 目标代码类型，从`Eum_TargetCodeType`枚举中选择
- `optimize`: 布尔值，指示是否启用优化
- `optimization_level`: 优化级别，0表示无优化，1-3表示优化级别
- `debug_info`: 布尔值，指示是否生成调试信息
- `output_file`: 输出文件路径，可选（NULL表示不写入文件）
- `verbose`: 布尔值，指示是否输出详细生成信息

### Stru_CodeGenResult_t

代码生成结果结构体，包含代码生成的结果和状态信息。

```c
typedef struct Stru_CodeGenResult_t {
    bool success;                       ///< 是否成功
    const char* code;                   ///< 生成的代码字符串
    size_t code_length;                 ///< 代码长度
    Stru_DynamicArray_t* errors;        ///< 错误信息数组
    Stru_DynamicArray_t* warnings;      ///< 警告信息数组
    size_t instruction_count;           ///< 生成的指令数量
    size_t memory_usage;                ///< 内存使用量（字节）
} Stru_CodeGenResult_t;
```

#### 字段说明

- `success`: 布尔值，指示代码生成是否成功
- `code`: 生成的代码字符串，调用者不应修改
- `code_length`: 生成的代码长度（字节）
- `errors`: 错误信息动态数组，包含`ErrorInfo`结构体
- `warnings`: 警告信息动态数组，包含`WarningInfo`结构体
- `instruction_count`: 生成的指令数量（对于汇编/字节码）
- `memory_usage`: 代码生成过程中使用的内存量

### Stru_CodeGeneratorInterface_t

代码生成器抽象接口结构体，定义代码生成器的完整功能接口。

```c
typedef struct Stru_CodeGeneratorInterface_t {
    // 初始化函数
    bool (*initialize)(struct Stru_CodeGeneratorInterface_t* interface, 
                      const Stru_CodeGenOptions_t* options);
    
    // 核心功能
    Stru_CodeGenResult_t* (*generate_code)(struct Stru_CodeGeneratorInterface_t* interface, 
                                          Stru_AstNode_t* ast);
    Stru_CodeGenResult_t* (*generate_module_code)(struct Stru_CodeGeneratorInterface_t* interface, 
                                                 Stru_AstNode_t** modules, size_t module_count);
    
    // 优化功能
    Stru_CodeGenResult_t* (*optimize)(struct Stru_CodeGeneratorInterface_t* interface, 
                                     const Stru_CodeGenResult_t* code, int optimization_level);
    bool (*register_optimizer_plugin)(struct Stru_CodeGeneratorInterface_t* interface,
                                     const char* plugin_name, void* plugin_func);
    
    // 目标代码生成
    bool (*set_target)(struct Stru_CodeGeneratorInterface_t* interface,
                      Eum_TargetCodeType target_type,
                      const char* platform, const char* architecture);
    bool (*generate_target_file)(struct Stru_CodeGeneratorInterface_t* interface,
                                const Stru_CodeGenResult_t* code, const char* output_file);
    
    // 状态查询
    Eum_TargetCodeType (*get_target_type)(struct Stru_CodeGeneratorInterface_t* interface);
    void (*get_supported_optimization_levels)(struct Stru_CodeGeneratorInterface_t* interface,
                                             int* min_level, int* max_level);
    Stru_DynamicArray_t* (*get_registered_plugins)(struct Stru_CodeGeneratorInterface_t* interface);
    
    // 错误处理
    bool (*has_errors)(struct Stru_CodeGeneratorInterface_t* interface);
    Stru_DynamicArray_t* (*get_errors)(struct Stru_CodeGeneratorInterface_t* interface);
    void (*clear_errors)(struct Stru_CodeGeneratorInterface_t* interface);
    
    // 资源管理
    void (*reset)(struct Stru_CodeGeneratorInterface_t* interface);
    void (*destroy)(struct Stru_CodeGeneratorInterface_t* interface);
    
    // 内部状态
    void* internal_state;
} Stru_CodeGeneratorInterface_t;
```

## 函数指针详细说明

### 初始化函数

#### `initialize`

初始化代码生成器，设置代码生成选项。

**参数**:
- `interface`: 代码生成器接口指针
- `options`: 代码生成选项指针

**返回值**:
- `true`: 初始化成功
- `false`: 初始化失败

**前置条件**:
- `interface` 必须有效
- `options` 必须包含有效的配置

**后置条件**:
- 代码生成器准备好进行代码生成
- 内部状态根据选项初始化

### 核心功能

#### `generate_code`

从抽象语法树生成目标代码。

**参数**:
- `interface`: 代码生成器接口指针
- `ast`: 抽象语法树根节点

**返回值**:
- `Stru_CodeGenResult_t*`: 代码生成结果，调用者负责销毁

**注意事项**:
- 调用者负责销毁返回的结果结构体
- AST在生成过程中不会被修改
- 错误信息包含在结果结构体中

#### `generate_module_code`

从多个AST节点生成模块级别的代码。

**参数**:
- `interface`: 代码生成器接口指针
- `modules`: AST节点数组
- `module_count`: 模块数量

**返回值**:
- `Stru_CodeGenResult_t*`: 代码生成结果，调用者负责销毁

**使用场景**:
- 多模块项目编译
- 库文件生成
- 增量编译

### 优化功能

#### `optimize`

对生成的代码应用优化。

**参数**:
- `interface`: 代码生成器接口指针
- `code`: 要优化的代码结果
- `optimization_level`: 优化级别

**返回值**:
- `Stru_CodeGenResult_t*`: 优化后的代码结果

**优化级别**:
- `0`: 无优化
- `1`: 基本优化（快速）
- `2`: 标准优化（平衡）
- `3`: 激进优化（最大优化）

#### `register_optimizer_plugin`

注册自定义优化器插件。

**参数**:
- `interface`: 代码生成器接口指针
- `plugin_name`: 插件名称
- `plugin_func`: 插件函数指针

**返回值**:
- `true`: 注册成功
- `false`: 注册失败

**插件要求**:
- 插件函数必须符合`OptimizerPluginFunc`签名
- 插件名称必须唯一
- 插件必须实现必要的接口

### 目标代码生成

#### `set_target`

设置目标代码生成的平台和架构。

**参数**:
- `interface`: 代码生成器接口指针
- `target_type`: 目标代码类型
- `platform`: 平台名称（如"linux", "windows"）
- `architecture`: 架构名称（如"x86_64", "arm64"）

**返回值**:
- `true`: 设置成功
- `false`: 设置失败

**支持平台**:
- Linux, Windows, macOS, Android, iOS
- 裸机系统，嵌入式系统

#### `generate_target_file`

将生成的代码写入目标文件。

**参数**:
- `interface`: 代码生成器接口指针
- `code`: 生成的代码结果
- `output_file`: 输出文件路径

**返回值**:
- `true`: 生成成功
- `false`: 生成失败

**支持格式**:
- 汇编文件（.s, .asm）
- 目标文件（.o, .obj）
- 可执行文件
- 库文件

### 状态查询

#### `get_target_type`

获取当前设置的目标代码类型。

**参数**:
- `interface`: 代码生成器接口指针

**返回值**:
- `Eum_TargetCodeType`: 当前目标代码类型

#### `get_supported_optimization_levels`

获取代码生成器支持的优化级别范围。

**参数**:
- `interface`: 代码生成器接口指针
- `min_level`: 输出参数，最小优化级别
- `max_level`: 输出参数，最大优化级别

**输出参数**:
- `min_level`: 设置为支持的最小优化级别（通常为0）
- `max_level`: 设置为支持的最大优化级别（通常为3）

#### `get_registered_plugins`

获取所有已注册的优化器插件名称。

**参数**:
- `interface`: 代码生成器接口指针

**返回值**:
- `Stru_DynamicArray_t*`: 插件名称数组，调用者负责销毁

### 错误处理

#### `has_errors`

检查代码生成过程中是否发生了错误。

**参数**:
- `interface`: 代码生成器接口指针

**返回值**:
- `true`: 有错误
- `false`: 没有错误

#### `get_errors`

获取所有错误信息的数组。

**参数**:
- `interface`: 代码生成器接口指针

**返回值**:
- `Stru_DynamicArray_t*`: 错误信息数组，调用者负责销毁

**错误信息格式**:
```c
typedef struct {
    int error_code;         // 错误代码
    const char* message;    // 错误描述
    size_t line;            // 错误行号（如果适用）
    size_t column;          // 错误列号（如果适用）
    const char* file;       // 文件名（如果适用）
} ErrorInfo;
```

#### `clear_errors`

清除所有错误信息。

**参数**:
- `interface`: 代码生成器接口指针

### 资源管理

#### `reset`

重置代码生成器到初始状态。

**参数**:
- `interface`: 代码生成器接口指针

**效果**:
- 清除所有内部状态
- 释放临时资源
- 保持配置不变

#### `destroy`

销毁代码生成器，释放所有资源。

**参数**:
- `interface`: 代码生成器接口指针

**重要**:
- 调用此函数后，接口指针不再有效
- 必须调用此函数避免内存泄漏

## 工厂函数

### `F_create_codegen_interface`

创建代码生成器接口实例。

```c
Stru_CodeGeneratorInterface_t* F_create_codegen_interface(void);
```

**返回值**:
- `Stru_CodeGeneratorInterface_t*`: 新创建的代码生成器接口实例

**注意事项**:
- 调用者负责在不再使用时调用`destroy`函数
- 返回的接口已分配内存但未初始化
- 必须调用`initialize`函数进行初始化

### `F_create_default_codegen_options`

创建默认代码生成选项。

```c
Stru_CodeGenOptions_t F_create_default_codegen_options(void);
```

**返回值**:
- `Stru_CodeGenOptions_t`: 默认代码生成选项

**默认值**:
- `target_type`: `Eum_TARGET_C`
- `optimize`: `true`
- `optimization_level`: `2`
- `debug_info`: `false`
- `output_file`: `NULL`
- `verbose`: `false`

### `F_destroy_codegen_result`

销毁代码生成结果。

```c
void F_destroy_codegen_result(Stru_CodeGenResult_t* result);
```

**参数**:
- `result`: 要销毁的代码生成结果

**效果**:
- 释放代码字符串内存
- 销毁错误和警告数组
- 释放结果结构体内存

## 使用示例

### 基本使用

```c
#include "src/core/codegen/CN_codegen_interface.h"

int main() {
    // 创建代码生成器
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    
    // 配置选项
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    options.target_type = Eum_TARGET_C;
    options.optimize = true;
    
    // 初始化
    if (!codegen->initialize(codegen, &options)) {
        printf("初始化失败\n");
        return 1;
    }
    
    // 设置目标平台
    if (!codegen->set_target(codegen, Eum_TARGET_C, "linux", "x86_64")) {
        printf("设置目标失败\n");
        return 1;
    }
    
    // 生成代码（假设ast已创建）
    Stru_CodeGenResult_t* result = codegen->generate_code(codegen, ast);
    
    // 检查结果
    if (result->success) {
        printf("生成成功，代码长度: %zu\n", result->code_length);
        
        // 写入文件
        if (!codegen->generate_target_file(codegen, result, "output.c")) {
            printf("写入文件失败\n");
        }
    } else {
        printf("生成失败，错误数: %zu\n", 
               result->errors ? F_dynamic_array_length(result->errors) : 0);
    }
    
    // 清理资源
    F_destroy_codegen_result(result);
    codegen->destroy(codegen);
    
    return 0;
}
```

### 错误处理示例

```c
// 检查错误
if (codegen->has_errors(codegen)) {
    Stru_DynamicArray_t* errors = codegen->get_errors(codegen);
    size_t error_count = F_dynamic_array_length(errors);
    
    for (size_t i = 0; i < error_count; i++) {
        ErrorInfo* error = F_dynamic_array_get(errors, i);
        printf("错误 %zu: [%d] %s at %s:%zu:%zu\n",
               i + 1, error->error_code, error->message,
               error->file ? error->file : "<unknown>",
               error->line, error->column);
    }
    
    F_destroy_dynamic_array(errors);
    codegen->clear_errors(codegen);
}
```

## 错误代码

### 通用错误 (1000-1999)

| 代码 | 描述 | 可能原因 |
|------|------|----------|
| 1000 | 内存分配失败 | 系统内存不足 |
| 1001 | 无效参数 | 传入NULL指针或无效值 |
| 1002 | 未初始化 | 接口未正确初始化 |
| 1003 | 内部错误 | 代码生成器内部逻辑错误 |

### 配置错误 (2000-2999)

| 代码 | 描述 | 可能原因 |
|------|------|----------|
| 2000 | 无效目标类型 | 不支持的目标代码类型 |
| 2001 | 无效优化级别 | 优化级别超出范围 |
| 2002 | 文件访问错误 | 无法访问输出文件 |

### 生成错误 (3000-3999)

| 代码 | 描述 | 可能原因 |
|------|------|----------|
| 3000 | AST无效 | 抽象语法树结构错误 |
| 3001 | 类型不匹配 | 表达式类型不兼容 |
| 3002 | 未定义标识符 | 使用了未定义的变量或函数 |
| 3003 | 语法错误 | 无法生成有效的目标代码 |

### 优化错误 (4000-4999)

| 代码 | 描述 | 可能原因 |
|------|------|----------|
| 4000 | 优化失败 | 优化过程出错 |
| 4001 | 插件加载失败 | 无法加载优化器插件 |
| 4002 | 插件不兼容 | 插件接口版本不匹配 |

## 性能考虑

### 内存管理

- 代码生成器使用统一的内存管理接口
- 大块代码使用流式生成减少内存占用
- 中间结果及时释放避免内存泄漏

### 线程安全

- 接口函数不是线程安全的
- 多线程使用需要外部同步
- 每个线程应使用独立的接口实例

### 性能优化

- 使用缓存避免重复计算
- 增量代码生成支持
- 并行优化处理（如果支持）

## 兼容性说明

### 版本兼容性

- 主版本号变更表示不兼容的API修改
- 次版本号变更表示向下兼容的功能新增
- 修订号变更表示向下兼容的问题修正

### 平台兼容性

- 接口设计为平台无关
- 具体实现可能依赖平台特性
- 跨平台代码需要测试所有目标平台

## 扩展指南

### 添加新目标类型

1. 在`Eum_TargetCodeType`枚举中添加新类型
2. 实现对应的目标代码生成器
3. 更新工厂函数支持新类型
4. 编写测试用例验证功能

### 添加优化插件

1. 实现`OptimizerPluginFunc`接口
2. 注册插件到代码生成
