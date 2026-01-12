# CN_x86_backend API 文档

## 概述

`CN_x86_backend` 模块是 CN_Language 编译器的 x86/x86-64 汇编后端实现，负责将抽象语法树（AST）、中间表示（IR）或 LLVM IR 转换为 x86/x86-64 汇编代码。该模块提供完整的汇编代码生成、优化和汇编功能。

## 版本信息

- **当前版本**: 1.0.0
- **架构支持**: x86 32位, x86-64 64位
- **汇编语法**: AT&T, Intel
- **最后更新**: 2026年1月12日

## 头文件

```c
#include "CN_x86_backend.h"
```

## 类型定义

### 枚举类型

#### Eum_X86Architecture

x86架构枚举，定义支持的x86架构变体。

```c
typedef enum Eum_X86Architecture {
    Eum_X86_ARCH_32BIT,          ///< x86 32位架构
    Eum_X86_ARCH_64BIT,          ///< x86-64 64位架构
    Eum_X86_ARCH_X32,            ///< x32 ABI（64位寄存器，32位指针）
    Eum_X86_ARCH_IA32,           ///< Intel IA-32架构
    Eum_X86_ARCH_AMD64           ///< AMD64架构
} Eum_X86Architecture;
```

#### Eum_X86Extension

x86指令集扩展枚举，定义支持的x86指令集扩展。

```c
typedef enum Eum_X86Extension {
    Eum_X86_EXT_MMX,             ///< MMX扩展
    Eum_X86_EXT_SSE,             ///< SSE扩展
    Eum_X86_EXT_SSE2,            ///< SSE2扩展
    Eum_X86_EXT_SSE3,            ///< SSE3扩展
    Eum_X86_EXT_SSSE3,           ///< SSSE3扩展
    Eum_X86_EXT_SSE41,           ///< SSE4.1扩展
    Eum_X86_EXT_SSE42,           ///< SSE4.2扩展
    Eum_X86_EXT_AVX,             ///< AVX扩展
    Eum_X86_EXT_AVX2,            ///< AVX2扩展
    Eum_X86_EXT_AVX512,          ///< AVX-512扩展
    Eum_X86_EXT_FMA,             ///< FMA扩展
    Eum_X86_EXT_AES,             ///< AES指令集
    Eum_X86_EXT_SHA,             ///< SHA指令集
    Eum_X86_EXT_RDRAND,          ///< RDRAND指令
    Eum_X86_EXT_RDSEED           ///< RDSEED指令
} Eum_X86Extension;
```

### 结构体类型

#### Stru_X86BackendConfig_t

x86后端配置结构体，定义x86汇编后端的配置选项。

```c
typedef struct Stru_X86BackendConfig_t {
    Eum_X86Architecture architecture;   ///< x86架构变体
    bool use_64bit;                     ///< 是否使用64位模式
    const char* cpu_model;              ///< CPU型号（如 "corei7", "athlon64"）
    const char* features;               ///< CPU特性字符串
    int optimization_level;             ///< 优化级别（0-3）
    bool debug_info;                    ///< 是否生成调试信息
    bool position_independent;          ///< 是否生成位置无关代码
    bool stack_protector;               ///< 是否启用栈保护
    bool omit_frame_pointer;            ///< 是否省略帧指针
    bool red_zone;                      ///< 是否使用红区（x86-64）
    bool pic_base_register;             ///< 是否使用PIC基址寄存器
    const char* assembler_syntax;       ///< 汇编语法（"att", "intel"）
    const char* output_format;          ///< 输出格式（"asm", "obj", "exe"）
} Stru_X86BackendConfig_t;
```

#### Stru_X86BackendState_t

x86后端状态结构体，存储x86后端的内部状态。

```c
typedef struct Stru_X86BackendState_t {
    Stru_X86BackendConfig_t config;     ///< 当前配置
    Stru_DynamicArray_t* errors;        ///< 错误信息数组
    Stru_DynamicArray_t* warnings;      ///< 警告信息数组
    void* code_buffer;                  ///< 代码缓冲区
    size_t code_buffer_size;            ///< 代码缓冲区大小
    size_t code_buffer_capacity;        ///< 代码缓冲区容量
    void* symbol_table;                 ///< 符号表
    void* relocation_table;             ///< 重定位表
    bool initialized;                   ///< 是否已初始化
} Stru_X86BackendState_t;
```

## 函数文档

### 代码生成函数

#### F_generate_x86_asm_from_ast

从AST生成x86汇编代码。

```c
Stru_CodeGenResult_t* F_generate_x86_asm_from_ast(
    Stru_AstNode_t* ast, 
    const Stru_X86BackendConfig_t* config
);
```

**参数**:
- `ast`: 抽象语法树根节点
- `config`: x86后端配置（可为NULL，使用默认配置）

**返回值**:
- `Stru_CodeGenResult_t*`: 代码生成结果，调用者负责销毁

**说明**:
此函数是x86后端的主要入口点，负责协调整个x86汇编代码生成过程。生成的汇编代码可以直接用汇编器（如NASM、GAS）汇编。

**示例**:
```c
Stru_AstNode_t* ast = parse_source_code(source);
Stru_X86BackendConfig_t config = F_create_default_x86_backend_config();
config.assembler_syntax = "intel";
config.optimization_level = 2;

Stru_CodeGenResult_t* result = F_generate_x86_asm_from_ast(ast, &config);
if (result->success) {
    printf("生成的x86汇编代码:\n%s\n", result->code);
}
F_destroy_codegen_result(result);
```

#### F_generate_x86_asm_from_ir

从IR生成x86汇编代码。

```c
Stru_CodeGenResult_t* F_generate_x86_asm_from_ir(
    const void* ir, 
    size_t ir_size, 
    const Stru_X86BackendConfig_t* config
);
```

**参数**:
- `ir`: 中间表示数据
- `ir_size`: 中间表示大小
- `config`: x86后端配置（可为NULL，使用默认配置）

**返回值**:
- `Stru_CodeGenResult_t*`: 代码生成结果，调用者负责销毁

**说明**:
将CN_Language的中间表示转换为x86汇编代码。

#### F_generate_x86_asm_from_llvm_ir

从LLVM IR生成x86汇编代码。

```c
Stru_CodeGenResult_t* F_generate_x86_asm_from_llvm_ir(
    const char* llvm_ir, 
    const Stru_X86BackendConfig_t* config
);
```

**参数**:
- `llvm_ir`: LLVM IR字符串
- `config`: x86后端配置（可为NULL，使用默认配置）

**返回值**:
- `Stru_CodeGenResult_t*`: 代码生成结果，调用者负责销毁

**说明**:
将LLVM IR转换为x86汇编代码。这允许使用LLVM的优化管道，然后生成x86汇编代码。

**示例**:
```c
// 首先生成LLVM IR
Stru_LLVMBackendConfig_t llvm_config = F_create_default_llvm_backend_config();
Stru_CodeGenResult_t* llvm_result = F_generate_llvm_ir_from_ast(ast, &llvm_config);

if (llvm_result->success) {
    // 从LLVM IR生成x86汇编代码
    Stru_X86BackendConfig_t x86_config = F_create_default_x86_backend_config();
    Stru_CodeGenResult_t* asm_result = 
        F_generate_x86_asm_from_llvm_ir(llvm_result->code, &x86_config);
    
    if (asm_result->success) {
        printf("从LLVM IR生成的汇编代码:\n%s\n", asm_result->code);
    }
    
    F_destroy_codegen_result(asm_result);
}

F_destroy_codegen_result(llvm_result);
```

### 优化函数

#### F_optimize_x86_asm

优化x86汇编代码。

```c
char* F_optimize_x86_asm(
    const char* asm_code, 
    int optimization_level
);
```

**参数**:
- `asm_code`: x86汇编代码字符串
- `optimization_level`: 优化级别（0-3）

**返回值**:
- `char*`: 优化后的x86汇编代码字符串，调用者负责释放

**说明**:
对x86汇编代码应用优化。优化级别说明：
- O0: 无优化
- O1: 基本优化
- O2: 标准优化
- O3: 激进优化

**示例**:
```c
char* asm_code = generate_some_assembly();
char* optimized_asm = F_optimize_x86_asm(asm_code, 2);
if (optimized_asm) {
    printf("优化后的汇编代码:\n%s\n", optimized_asm);
    free(optimized_asm);
}
free(asm_code);
```

### 汇编函数

#### F_assemble_x86_asm_to_object

将x86汇编代码汇编为目标文件。

```c
Stru_TargetCodeGenResult_t* F_assemble_x86_asm_to_object(
    const char* asm_code, 
    const Stru_TargetConfig_t* target_config
);
```

**参数**:
- `asm_code`: x86汇编代码字符串
- `target_config`: 目标配置

**返回值**:
- `Stru_TargetCodeGenResult_t*`: 目标代码生成结果，调用者负责销毁

**说明**:
将x86汇编代码汇编为特定目标文件格式。支持的目标格式包括ELF、COFF、Mach-O等。

**示例**:
```c
Stru_TargetConfig_t target_config = F_create_default_target_config();
target_config.platform = Eum_TARGET_PLATFORM_X86_64;
target_config.os = Eum_TARGET_OS_LINUX;
target_config.format = Eum_TARGET_FORMAT_OBJECT;

Stru_TargetCodeGenResult_t* result = 
    F_assemble_x86_asm_to_object(asm_code, &target_config);

if (result->success) {
    printf("目标文件生成成功，大小: %zu 字节\n", result->code_size);
    // 可以将result->code写入文件
}

F_destroy_target_codegen_result(result);
```

### 版本信息函数

#### F_get_x86_backend_version

获取x86后端版本信息。

```c
void F_get_x86_backend_version(
    int* major, 
    int* minor, 
    int* patch
);
```

**参数**:
- `major`: 输出参数，主版本号
- `minor`: 输出参数，次版本号
- `patch`: 输出参数，修订号

**说明**:
版本号遵循语义化版本规范。

**示例**:
```c
int major, minor, patch;
F_get_x86_backend_version(&major, &minor, &patch);
printf("x86后端版本: %d.%d.%d\n", major, minor, patch);
```

#### F_get_x86_backend_version_string

获取x86后端版本字符串。

```c
const char* F_get_x86_backend_version_string(void);
```

**返回值**:
- `const char*`: 版本字符串，格式为"主版本号.次版本号.修订号"

**说明**:
返回的字符串是静态常量，调用者不应修改或释放。

**示例**:
```c
const char* version = F_get_x86_backend_version_string();
printf("x86后端版本: %s\n", version);
```

### 功能检查函数

#### F_x86_backend_supports_feature

检查x86后端是否支持特定功能。

```c
bool F_x86_backend_supports_feature(const char* feature);
```

**参数**:
- `feature`: 功能标识符

**返回值**:
- `bool`: 支持返回true，不支持返回false

**说明**:
功能标识符可以是以下值：
- "x86-32": 支持x86 32位架构
- "x86-64": 支持x86-64 64位架构
- "sse": 支持SSE指令集
- "avx": 支持AVX指令集
- "avx512": 支持AVX-512指令集
- "att-syntax": 支持AT&T汇编语法
- "intel-syntax": 支持Intel汇编语法

**示例**:
```c
if (F_x86_backend_supports_feature("x86-64")) {
    printf("支持x86-64架构\n");
}

if (F_x86_backend_supports_feature("avx")) {
    printf("支持AVX指令集\n");
}
```

#### F_x86_backend_supports_extension

检查是否支持特定指令集扩展。

```c
bool F_x86_backend_supports_extension(Eum_X86Extension extension);
```

**参数**:
- `extension`: 指令集扩展

**返回值**:
- `bool`: 支持返回true，不支持返回false

**示例**:
```c
if (F_x86_backend_supports_extension(Eum_X86_EXT_AVX2)) {
    printf("支持AVX2指令集扩展\n");
}
```

### 配置函数

#### F_create_default_x86_backend_config

创建默认x86后端配置。

```c
Stru_X86BackendConfig_t F_create_default_x86_backend_config(void);
```

**返回值**:
- `Stru_X86BackendConfig_t`: 默认x86后端配置

**说明**:
返回的默认配置为：
- 架构: x86-64 64位
- CPU型号: "generic"
- 优化级别: 2
- 汇编语法: AT&T
- 输出格式: asm

**示例**:
```c
Stru_X86BackendConfig_t config = F_create_default_x86_backend_config();
config.assembler_syntax = "intel";  // 修改为Intel语法
config.optimization_level = 3;      // 最高优化级别
```

#### F_configure_x86_backend

配置x86后端选项。

```c
bool F_configure_x86_backend(
    const char* option, 
    const char* value
);
```

**参数**:
- `option`: 选项名称
- `value`: 选项值

**返回值**:
- `bool`: 配置成功返回true，失败返回false

**说明**:
支持的选项：
- "architecture": 架构 ("x86", "x86-64")
- "optimization_level": 优化级别 (0-3)
- "debug_info": 是否生成调试信息 (true/false)
- "assembler_syntax": 汇编语法 ("att", "intel")

**示例**:
```c
F_configure_x86_backend("architecture", "x86-64");
F_configure_x86_backend("optimization_level", "3");
F_configure_x86_backend("assembler_syntax", "intel");
```

### 接口创建函数

#### F_create_x86_backend_interface

创建x86后端代码生成器接口。

```c
Stru_CodeGeneratorInterface_t* F_create_x86_backend_interface(void);
```

**返回值**:
- `Stru_CodeGeneratorInterface_t*`: x86后端代码生成器接口实例

**说明**:
此函数创建专门用于x86汇编代码生成的接口实例，与通用的`F_create_codegen_interface()`函数不同，此函数创建的接口已经预配置为x86后端。

**示例**:
```c
Stru_CodeGeneratorInterface_t* x86_interface = F_create_x86_backend_interface();

Stru_CodeGenOptions_t options = F_create_default_codegen_options();
options.target_type = Eum_TARGET_X86_64;
options.optimization_level = 3;

x86_interface->initialize(x86_interface, &options);
Stru_CodeGenResult_t* result = x86_interface->generate_code(x86_interface, ast);

x86_interface->destroy(x86_interface);
```

#### F_create_x86_target_codegen_interface

创建x86后端目标代码生成器接口。

```c
Stru_TargetCodeGeneratorInterface_t* F_create_x86_target_codegen_interface(void);
```

**返回值**:
- `Stru_TargetCodeGeneratorInterface_t*`: x86后端目标代码生成器接口实例

**说明**:
此函数创建专门用于从x86汇编代码生成目标代码的接口实例。

**示例**:
```c
Stru_TargetCodeGeneratorInterface_t* target_interface = 
    F_create_x86_target_codegen_interface();

Stru_TargetConfig_t target_config = F_create_default_target_config();
target_config.platform = Eum_TARGET_PLATFORM_X86_64;
target_config.os = Eum_TARGET_OS_LINUX;

Stru_TargetCodeGenResult_t* result = 
    target_interface->generate_target_code(target_interface, asm_code, &target_config);

target_interface->destroy(target_interface);
```

## 使用示例

### 完整示例：从AST生成x86汇编并优化

```c
#include "CN_x86_backend.h"
#include "CN_parser.h"
#include "CN_ast.h"

int main() {
    // 解析源代码
    const char* source_code = "变量 数字 = 42; 打印(数字);";
    Stru_Parser_t* parser = F_create_parser(source_code);
    Stru_AstNode_t* ast = F_parse_program(parser);
    
    if (!ast) {
        printf("解析失败\n");
        F_destroy_parser(parser);
        return 1;
    }
    
    // 创建x86后端配置
    Stru_X86BackendConfig_t config = F_create_default_x86_backend_config();
    config.assembler_syntax = "intel";  // 使用Intel语法
    config.optimization_level = 2;      // 标准优化
    config.debug_info = true;           // 生成调试信息
    
    // 生成x86汇编代码
    Stru_CodeGenResult_t* result = F_generate_x86_asm_from_ast(ast, &config);
    
    if (result->success) {
        printf("生成的x86汇编代码:\n%s\n", result->code);
        
        // 应用额外优化
        char* optimized_asm = F_optimize_x86_asm(result->code, 3);
        if (optimized_asm) {
            printf("\n优化后的汇编代码:\n%s\n", optimized_asm);
            free(optimized_asm);
        }
        
        // 汇编为目标文件
        Stru_TargetConfig_t target_config = F_create_default_target_config();
        target_config.platform = Eum_TARGET_PLATFORM_X86_64;
        target_config.os = Eum_TARGET_OS_LINUX;
        target_config.format = Eum_TARGET_FORMAT_OBJECT;
        target_config.output_file = "output.o";
        
        Stru_TargetCodeGenResult_t* target_result = 
            F_assemble_x86_asm_to_object(result->code, &target_config);
        
        if (target_result->success) {
            printf("目标文件生成成功: %s\n", target_config.output_file);
        } else {
            printf("目标文件生成失败\n");
            // 输出错误信息
            for (size_t i = 0; i < target_result->errors->length; i++) {
                char* error = *(char**)F_dynamic_array_get(target_result->errors, i);
                printf("错误: %s\n", error);
            }
        }
        
        F_destroy_target_codegen_result(target_result);
    } else {
        printf("x86汇编代码生成失败\n");
        // 输出错误信息
        for (size_t i = 0; i < result->errors->length; i++) {
            char* error = *(char**)F_dynamic_array_get(result->errors, i);
            printf("错误: %s\n", error);
        }
    }
    
    // 清理资源
    F_destroy_codegen_result(result);
    F_destroy_ast_node(ast);
    F_destroy_parser(parser);
    
    return 0;
}
```

### 示例：使用x86后端接口

```c
#include "CN_x86_backend.h"

int main() {
    // 创建x86后端接口
    Stru_CodeGeneratorInterface_t* x86_interface = F_create_x86_backend_interface();
    
    if (!x86_interface) {
        printf("无法创建x86后端接口\n");
        return 1;
    }
    
    // 配置接口
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    options.target_type = Eum_TARGET_X86_64;
    options.optimization_level = 3;
    options.debug_info = true;
    options.output_format = Eum_OUTPUT_FORMAT_ASSEMBLY;
    
    if (!x86_interface->initialize(x86_interface, &options)) {
        printf("接口初始化失败\n");
        x86_interface->destroy(x86_interface);
        return 1;
    }
    
    // 生成代码（假设已有AST）
    Stru_CodeGenResult_t* result = x86_interface->generate_code(x86_interface, ast);
    
    if (result->success) {
        printf("使用接口生成的汇编代码:\n%s\n", result->code);
        
        // 保存到文件
        FILE* file = fopen("output.asm", "w");
        if (file) {
            fwrite(result->code, 1, result->code_length, file);
            fclose(file);
            printf("汇编代码已保存到 output.asm\n");
        }
    }
    
    // 清理资源
    F_destroy_codegen_result(result);
    x86_interface->destroy(x86_interface);
    
    return 0;
}
```

### 示例：检查功能支持

```c
#include "CN_x86_backend.h"
#include <stdio.h>

int main() {
    // 检查版本
    int major, minor, patch;
    F_get_x86_backend_version(&major, &minor, &patch);
    printf("x86后端版本: %d.%d.%d\n", major, minor, patch);
    
    // 检查功能支持
    const char* features[] = {
        "x86-32", "x86-64", "sse", "avx", "avx512", 
        "att-syntax", "intel-syntax", NULL
    };
    
    printf("\n功能支持检查:\n");
    for (int i = 0; features[i] != NULL; i++) {
        bool supported = F_x86_backend_supports_feature(features[i]);
        printf("  %-15s: %s\n", features[i], supported ? "支持" : "不支持");
    }
    
    // 检查指令集扩展支持
    printf("\n指令集扩展支持检查:\n");
    Eum_X86Extension extensions[] = {
        Eum_X86_EXT_SSE, Eum_X86_EXT_SSE2, Eum_X86_EXT_AVX,
        Eum_X86_EXT_AVX2, Eum_X86_EXT_AVX512
    };
    
    const char* extension_names[] = {
        "SSE", "SSE2", "AVX", "AVX2", "AVX-512"
    };
    
    for (int i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
        bool supported = F_x86_backend_supports_extension(extensions[i]);
        printf("  %-10s: %s\n", extension_names[i], supported ? "支持" : "不支持");
    }
    
    return 0;
}
```

## 错误处理

x86后端模块提供详细的错误处理机制。所有生成函数都返回包含错误信息的`Stru_CodeGenResult_t`或`Stru_TargetCodeGenResult_t`结构体。

### 错误代码

模块定义以下错误代码：

```c
typedef enum Eum_X86BackendError {
    Eum_X86_BACKEND_SUCCESS = 0,        ///< 操作成功
    Eum_X86_BACKEND_INVALID_INPUT,      ///< 无效输入
    Eum_X86_BACKEND_MEMORY_ERROR,       ///< 内存分配错误
    Eum_X86_BACKEND_ARCHITECTURE_ERROR, ///< 架构不支持
    Eum_X86_BACKEND_INSTRUCTION_ERROR,  ///< 指令生成错误
    Eum_X86_BACKEND_OPTIMIZATION_ERROR, ///< 优化错误
    Eum_X86_BACKEND_ASSEMBLY_ERROR,     ///< 汇编错误
    Eum_X86_BACKEND_LINKING_ERROR       ///< 链接错误
} Eum_X86BackendError;
```

### 错误处理示例

```c
Stru_CodeGenResult_t* result = F_generate_x86_asm_from_ast(ast, config);

if (!result->success) {
    printf("代码生成失败，错误代码: %d\n", result->error_code);
    
    // 输出所有错误信息
    if (result->errors && result->errors->length > 0) {
        printf("错误信息:\n");
        for (size_t i = 0; i < result->errors->length; i++) {
            char* error = *(char**)F_dynamic_array_get(result->errors, i);
            printf("  [%zu] %s\n", i + 1, error);
        }
    }
    
    // 输出所有警告信息
    if (result->warnings && result->warnings->length > 0) {
        printf("警告信息:\n");
        for (size_t i = 0; i < result->warnings->length; i++) {
            char* warning = *(char**)F_dynamic_array_get(result->warnings, i);
            printf("  [%zu] %s\n", i + 1, warning);
        }
    }
}
```

## 性能考虑

### 内存管理

x86后端模块使用项目统一的内存管理接口，确保内存分配和释放的一致性。

### 优化建议

1. **批量处理**: 对于大量代码生成，考虑批量处理以减少上下文切换开销。
2. **缓存配置**: 重复使用相同的配置对象，避免重复初始化。
3. **预分配缓冲区**: 对于已知大小的代码生成，预分配缓冲区可以提高性能。
4. **异步处理**: 对于长时间运行的代码生成任务，考虑使用异步接口。

### 线程安全

x86后端模块不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 平台兼容性

### 支持的操作系统

- Linux (完全支持)
- Windows (部分支持，需要MinGW或Cygwin)
- macOS (部分支持)

### 支持的汇编器

- GNU Assembler (GAS)
- Netwide Assembler (NASM)
- YASM Assembler
- Microsoft Macro Assembler (MASM) - Windows only

### 编译器要求

- C99兼容编译器
- 支持标准库
- 支持POSIX API（Linux/macOS）
- 支持Windows API（Windows）

## 已知问题和限制

1. **AVX-512支持**: 目前对AVX-512指令集的支持有限。
2. **Windows兼容性**: Windows平台的支持仍在完善中。
3. **调试信息**: 调试信息生成功能相对基础。
4. **性能优化**: 高级优化功能仍在开发中。

## 更新日志

### 版本 1.0.0 (2026-01-12)
- 初始版本发布
- 支持x86 32位和x86-64 64位架构
- 支持AT&T和Intel汇编语法
- 基本代码生成功能
- 基础优化支持
- 目标文件生成框架

## 相关文档

- [x86后端模块README](../src/core/codegen/implementations/x86_backend/README.md)
- [代码生成器接口文档](../../CN_codegen_interface.md)
- [目标代码生成器接口文档](../../CN_target_codegen_interface.md)
- [CN_Language架构文档](../../../../architecture/README.md)

## 联系方式

如有问题或建议，请联系：
- 项目维护者: CN_Language开发团队
- 问题跟踪: GitHub Issues
- 文档更新: 提交Pull Request

---
*文档最后更新: 2026年1月12日*
