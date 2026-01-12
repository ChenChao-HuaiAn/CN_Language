# CN_llvm_backend.h API 文档

## 概述

`CN_llvm_backend.h` 定义了 CN_Language 的 LLVM IR 后端接口，提供将抽象语法树或中间表示转换为 LLVM IR 的功能。

## 文件信息

- **文件名**: CN_llvm_backend.h
- **位置**: src/core/codegen/implementations/llvm_backend/
- **版本**: 1.0.0
- **最后更新**: 2026年1月12日

## 包含文件

```c
#include "../../CN_codegen_interface.h"
#include "../../CN_target_codegen_interface.h"
```

## 类型定义

### Eum_TargetPlatform

目标平台枚举，定义支持的目标平台。

```c
typedef enum Eum_TargetPlatform {
    Eum_TARGET_PLATFORM_GENERIC,    // 通用平台
    Eum_TARGET_PLATFORM_X86,        // x86平台
    Eum_TARGET_PLATFORM_X86_64,     // x86-64平台
    Eum_TARGET_PLATFORM_ARM,        // ARM平台
    Eum_TARGET_PLATFORM_ARM64,      // ARM64平台
    Eum_TARGET_PLATFORM_MIPS,       // MIPS平台
    Eum_TARGET_PLATFORM_POWERPC,    // PowerPC平台
    Eum_TARGET_PLATFORM_RISCV,      // RISC-V平台
    Eum_TARGET_PLATFORM_WASM,       // WebAssembly平台
    Eum_TARGET_PLATFORM_JVM,        // Java虚拟机平台
    Eum_TARGET_PLATFORM_DOTNET,     // .NET平台
    Eum_TARGET_PLATFORM_CUSTOM      // 自定义平台
} Eum_TargetPlatform;
```

### Eum_TargetOS

目标操作系统枚举，定义支持的目标操作系统。

```c
typedef enum Eum_TargetOS {
    Eum_TARGET_OS_GENERIC,          // 通用操作系统
    Eum_TARGET_OS_LINUX,            // Linux
    Eum_TARGET_OS_WINDOWS,          // Windows
    Eum_TARGET_OS_MACOS,            // macOS
    Eum_TARGET_OS_ANDROID,          // Android
    Eum_TARGET_OS_IOS,              // iOS
    Eum_TARGET_OS_FREEBSD,          // FreeBSD
    Eum_TARGET_OS_EMBEDDED,         // 嵌入式系统
    Eum_TARGET_OS_BARE_METAL,       // 裸机系统
    Eum_TARGET_OS_CUSTOM            // 自定义操作系统
} Eum_TargetOS;
```

### Eum_TargetABI

目标ABI枚举，定义支持的目标ABI（应用程序二进制接口）。

```c
typedef enum Eum_TargetABI {
    Eum_TARGET_ABI_GENERIC,         // 通用ABI
    Eum_TARGET_ABI_SYSV,            // System V ABI
    Eum_TARGET_ABI_WIN32,           // Win32 ABI
    Eum_TARGET_ABI_MACH_O,          // Mach-O ABI
    Eum_TARGET_ABI_ELF,             // ELF ABI
    Eum_TARGET_ABI_COFF,            // COFF ABI
    Eum_TARGET_ABI_WASM,            // WebAssembly ABI
    Eum_TARGET_ABI_CUSTOM           // 自定义ABI
} Eum_TargetABI;
```

### Eum_TargetFormat

目标代码格式枚举，定义支持的目标代码格式。

```c
typedef enum Eum_TargetFormat {
    Eum_TARGET_FORMAT_ASSEMBLY,     // 汇编代码
    Eum_TARGET_FORMAT_OBJECT,       // 目标文件
    Eum_TARGET_FORMAT_EXECUTABLE,   // 可执行文件
    Eum_TARGET_FORMAT_LIBRARY,      // 库文件
    Eum_TARGET_FORMAT_SHARED_LIB,   // 共享库
    Eum_TARGET_FORMAT_BYTECODE,     // 字节码
    Eum_TARGET_FORMAT_IR,           // 中间表示
    Eum_TARGET_FORMAT_CUSTOM        // 自定义格式
} Eum_TargetFormat;
```

### Stru_LLVMBackendConfig_t

LLVM后端配置结构体，定义LLVM IR后端的配置选项。

```c
typedef struct Stru_LLVMBackendConfig_t {
    const char* target_triple;          // 目标三元组（如 "x86_64-pc-linux-gnu"）
    const char* cpu;                    // CPU型号（如 "x86-64"）
    const char* features;               // CPU特性字符串
    int optimization_level;             // 优化级别（0-3）
    bool debug_info;                    // 是否生成调试信息
    bool position_independent;          // 是否生成位置无关代码
    bool stack_protector;               // 是否启用栈保护
    bool omit_frame_pointer;            // 是否省略帧指针
    bool verify_module;                 // 是否验证LLVM模块
    bool print_module;                  // 是否打印LLVM模块
    const char* output_format;          // 输出格式（"ir", "bc", "asm", "obj"）
} Stru_LLVMBackendConfig_t;
```

### Stru_LLVMBackendState_t

LLVM后端状态结构体，存储LLVM后端的内部状态。

```c
typedef struct Stru_LLVMBackendState_t {
    void* llvm_context;                 // LLVM上下文
    void* llvm_module;                  // LLVM模块
    void* llvm_builder;                 // LLVM IR构建器
    void* llvm_pass_manager;            // LLVM优化管理器
    Stru_LLVMBackendConfig_t config;    // 当前配置
    Stru_DynamicArray_t* errors;        // 错误信息数组
    Stru_DynamicArray_t* warnings;      // 警告信息数组
    bool initialized;                   // 是否已初始化
} Stru_LLVMBackendState_t;
```

## 函数文档

### F_generate_llvm_ir_from_ast

从AST生成LLVM IR。

```c
Stru_CodeGenResult_t* F_generate_llvm_ir_from_ast(
    Stru_AstNode_t* ast, 
    const Stru_LLVMBackendConfig_t* config
);
```

**参数**:
- `ast`: 抽象语法树根节点
- `config`: LLVM后端配置（可为NULL，使用默认配置）

**返回值**:
- 成功: 返回代码生成结果指针
- 失败: 返回NULL

**说明**:
此函数是LLVM后端的主要入口点，负责协调整个LLVM IR生成过程。生成的LLVM IR可以直接用LLVM工具链优化和编译。

### F_generate_llvm_ir_from_ir

从IR生成LLVM IR。

```c
Stru_CodeGenResult_t* F_generate_llvm_ir_from_ir(
    const void* ir, 
    size_t ir_size, 
    const Stru_LLVMBackendConfig_t* config
);
```

**参数**:
- `ir`: 中间表示数据
- `ir_size`: 中间表示大小
- `config`: LLVM后端配置（可为NULL，使用默认配置）

**返回值**:
- 成功: 返回代码生成结果指针
- 失败: 返回NULL

### F_optimize_llvm_ir

优化LLVM IR。

```c
char* F_optimize_llvm_ir(
    const char* llvm_ir, 
    int optimization_level
);
```

**参数**:
- `llvm_ir`: LLVM IR字符串
- `optimization_level`: 优化级别（0-3）

**返回值**:
- 成功: 返回优化后的LLVM IR字符串
- 失败: 返回NULL

**注意**: 调用者负责释放返回的字符串。

### F_compile_llvm_ir_to_target

将LLVM IR编译为目标代码。

```c
Stru_TargetCodeGenResult_t* F_compile_llvm_ir_to_target(
    const char* llvm_ir, 
    const Stru_TargetConfig_t* target_config
);
```

**参数**:
- `llvm_ir`: LLVM IR字符串
- `target_config`: 目标配置

**返回值**:
- 成功: 返回目标代码生成结果指针
- 失败: 返回NULL

### F_get_llvm_backend_version

获取LLVM后端版本信息。

```c
void F_get_llvm_backend_version(
    int* major, 
    int* minor, 
    int* patch
);
```

**参数**:
- `major`: 输出参数，主版本号
- `minor`: 输出参数，次版本号
- `patch`: 输出参数，修订号

### F_get_llvm_backend_version_string

获取LLVM后端版本字符串。

```c
const char* F_get_llvm_backend_version_string(void);
```

**返回值**:
- 版本字符串，格式为"主版本号.次版本号.修订号"

**注意**: 返回的字符串是静态常量，调用者不应修改或释放。

### F_llvm_backend_supports_feature

检查LLVM后端是否支持特定功能。

```c
bool F_llvm_backend_supports_feature(const char* feature);
```

**参数**:
- `feature`: 功能标识符

**返回值**:
- 支持: true
- 不支持: false

**支持的功能标识符**:
- "llvm-16": 支持LLVM 16
- "llvm-17": 支持LLVM 17
- "llvm-18": 支持LLVM 18
- "jit": 支持即时编译
- "link-time-opt": 支持链接时优化
- "profile-guided-opt": 支持基于性能分析的优化

### F_create_default_llvm_backend_config

创建默认LLVM后端配置。

```c
Stru_LLVMBackendConfig_t F_create_default_llvm_backend_config(void);
```

**返回值**:
- 默认LLVM后端配置

### F_configure_llvm_backend

配置LLVM后端选项。

```c
bool F_configure_llvm_backend(
    const char* option, 
    const char* value
);
```

**参数**:
- `option`: 选项名称
- `value`: 选项值

**返回值**:
- 配置成功: true
- 配置失败: false

**支持的选项**:
- "target_triple": 目标三元组
- "optimization_level": 优化级别 (0-3)
- "debug_info": 是否生成调试信息 (true/false)
- "position_independent": 是否生成位置无关代码 (true/false)

### F_create_llvm_backend_interface

创建LLVM后端代码生成器接口。

```c
Stru_CodeGeneratorInterface_t* F_create_llvm_backend_interface(void);
```

**返回值**:
- LLVM后端代码生成器接口实例

**注意**: 此函数创建专门用于LLVM IR代码生成的接口实例，与通用的`F_create_codegen_interface()`函数不同，此函数创建的接口已经预配置为LLVM后端。

### F_create_llvm_target_codegen_interface

创建LLVM后端目标代码生成器接口。

```c
Stru_TargetCodeGeneratorInterface_t* F_create_llvm_target_codegen_interface(void);
```

**返回值**:
- LLVM后端目标代码生成器接口实例

**注意**: 此函数创建专门用于从LLVM IR生成目标代码的接口实例。

## 使用示例

### 示例1: 基本使用

```c
#include "CN_llvm_backend.h"

int main() {
    // 创建默认配置
    Stru_LLVMBackendConfig_t config = F_create_default_llvm_backend_config();
    
    // 设置目标平台
    config.target_triple = "x86_64-pc-linux-gnu";
    config.optimization_level = 2;
    config.debug_info = true;
    
    // 生成LLVM IR
    Stru_CodeGenResult_t* result = F_generate_llvm_ir_from_ast(ast, &config);
    
    if (result->success) {
        printf("LLVM IR生成成功:\n%s\n", result->code);
        
        // 优化LLVM IR
        char* optimized = F_optimize_llvm_ir(result->code, 3);
        if (optimized) {
            printf("优化后的LLVM IR:\n%s\n", optimized);
            free(optimized);
        }
    }
    
    F_destroy_codegen_result(result);
    return 0;
}
```

### 示例2: 使用接口

```c
#include "CN_llvm_backend.h"

int main() {
    // 创建LLVM后端接口
    Stru_CodeGeneratorInterface_t* llvm_interface = F_create_llvm_backend_interface();
    
    // 配置接口
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    options.target_type = Eum_TARGET_LLVM_IR;
    options.optimization_level = 3;
    options.debug_info = true;
    
    if (llvm_interface->initialize(llvm_interface, &options)) {
        // 生成代码
        Stru_CodeGenResult_t* result = llvm_interface->generate_code(llvm_interface, ast);
        
        if (result->success) {
            printf("代码生成成功\n");
        }
        
        F_destroy_codegen_result(result);
    }
    
    // 清理资源
    llvm_interface->destroy(llvm_interface);
    return 0;
}
```

## 错误处理

所有函数都通过返回值和错误信息数组提供错误处理。调用者应检查函数的返回值，并在失败时检查错误信息。

## 内存管理

- 所有返回指针的函数都需要调用者负责释放内存
- 配置结构体中的字符串字段应由调用者管理生命周期
- 接口实例使用后必须调用相应的destroy函数

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-12 | 初始版本 |

## 相关文档

- [CN_codegen_interface.h](../CN_codegen_interface.md)
- [CN_target_codegen_interface.h](../CN_target_codegen_interface.md)
- [LLVM后端README](../../../../src/core/codegen/implementations/llvm_backend/README.md)
