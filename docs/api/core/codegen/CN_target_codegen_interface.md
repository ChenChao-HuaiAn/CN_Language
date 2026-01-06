# CN_Target_Codegen_Interface API 文档

## 概述

`CN_target_codegen_interface.h` 定义了CN_Language目标代码生成器的抽象接口。该接口支持将中间表示转换为特定目标平台的代码，包括汇编代码、目标文件和可执行文件。

## 文件信息

- **文件名**: `CN_target_codegen_interface.h`
- **位置**: `src/core/codegen/`
- **版本**: 1.0.0
- **作者**: CN_Language架构委员会
- **创建日期**: 2026-01-06

## 包含文件

```c
#include "../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../ast/CN_ast.h"
#include "CN_codegen_interface.h"
```

## 枚举类型

### Eum_TargetPlatform

目标平台枚举，定义支持的硬件平台。

```c
typedef enum Eum_TargetPlatform {
    Eum_TARGET_PLATFORM_GENERIC,    ///< 通用平台
    Eum_TARGET_PLATFORM_X86,        ///< x86平台
    Eum_TARGET_PLATFORM_X86_64,     ///< x86-64平台
    Eum_TARGET_PLATFORM_ARM,        ///< ARM平台
    Eum_TARGET_PLATFORM_ARM64,      ///< ARM64平台
    Eum_TARGET_PLATFORM_MIPS,       ///< MIPS平台
    Eum_TARGET_PLATFORM_POWERPC,    ///< PowerPC平台
    Eum_TARGET_PLATFORM_RISCV,      ///< RISC-V平台
    Eum_TARGET_PLATFORM_WASM,       ///< WebAssembly平台
    Eum_TARGET_PLATFORM_JVM,        ///< Java虚拟机平台
    Eum_TARGET_PLATFORM_DOTNET,     ///< .NET平台
    Eum_TARGET_PLATFORM_CUSTOM      ///< 自定义平台
} Eum_TargetPlatform;
```

### Eum_TargetOS

目标操作系统枚举，定义支持的操作系统。

```c
typedef enum Eum_TargetOS {
    Eum_TARGET_OS_GENERIC,          ///< 通用操作系统
    Eum_TARGET_OS_LINUX,            ///< Linux
    Eum_TARGET_OS_WINDOWS,          ///< Windows
    Eum_TARGET_OS_MACOS,            ///< macOS
    Eum_TARGET_OS_ANDROID,          ///< Android
    Eum_TARGET_OS_IOS,              ///< iOS
    Eum_TARGET_OS_FREEBSD,          ///< FreeBSD
    Eum_TARGET_OS_EMBEDDED,         ///< 嵌入式系统
    Eum_TARGET_OS_BARE_METAL,       ///< 裸机系统
    Eum_TARGET_OS_CUSTOM            ///< 自定义操作系统
} Eum_TargetOS;
```

### Eum_TargetABI

目标ABI枚举，定义支持的应用程序二进制接口。

```c
typedef enum Eum_TargetABI {
    Eum_TARGET_ABI_GENERIC,         ///< 通用ABI
    Eum_TARGET_ABI_SYSV,            ///< System V ABI
    Eum_TARGET_ABI_WIN32,           ///< Win32 ABI
    Eum_TARGET_ABI_MACH_O,          ///< Mach-O ABI
    Eum_TARGET_ABI_ELF,             ///< ELF ABI
    Eum_TARGET_ABI_COFF,            ///< COFF ABI
    Eum_TARGET_ABI_WASM,            ///< WebAssembly ABI
    Eum_TARGET_ABI_CUSTOM           ///< 自定义ABI
} Eum_TargetABI;
```

### Eum_TargetFormat

目标代码格式枚举，定义支持的输出格式。

```c
typedef enum Eum_TargetFormat {
    Eum_TARGET_FORMAT_ASSEMBLY,     ///< 汇编代码
    Eum_TARGET_FORMAT_OBJECT,       ///< 目标文件
    Eum_TARGET_FORMAT_EXECUTABLE,   ///< 可执行文件
    Eum_TARGET_FORMAT_LIBRARY,      ///< 库文件
    Eum_TARGET_FORMAT_SHARED_LIB,   ///< 共享库
    Eum_TARGET_FORMAT_BYTECODE,     ///< 字节码
    Eum_TARGET_FORMAT_IR,           ///< 中间表示
    Eum_TARGET_FORMAT_CUSTOM        ///< 自定义格式
} Eum_TargetFormat;
```

## 结构体类型

### Stru_TargetConfig_t

目标配置结构体，包含目标代码生成的完整配置。

```c
typedef struct Stru_TargetConfig_t {
    Eum_TargetPlatform platform;    ///< 目标平台
    Eum_TargetOS os;                ///< 目标操作系统
    Eum_TargetABI abi;              ///< 目标ABI
    Eum_TargetFormat format;        ///< 目标格式
    Eum_TargetCodeType code_type;   ///< 目标代码类型（来自CN_codegen_interface.h）
    
    const char* cpu;                ///< CPU型号（可选）
    const char* features;           ///< CPU特性（可选）
    const char* triple;             ///< LLVM目标三元组（可选）
    
    bool position_independent;      ///< 是否生成位置无关代码
    bool stack_protector;           ///< 是否启用栈保护
    bool omit_frame_pointer;        ///< 是否省略帧指针
    bool optimize_for_size;         ///< 是否优化代码大小
    bool optimize_for_speed;        ///< 是否优化执行速度
    
    int alignment;                  ///< 对齐要求（字节）
    int stack_alignment;            ///< 栈对齐要求（字节）
    int red_zone_size;              ///< 红区大小（字节）
    
    Stru_DynamicArray_t* defines;   ///< 预定义宏
    Stru_DynamicArray_t* includes;  ///< 包含路径
    Stru_DynamicArray_t* libraries; ///< 链接库
    Stru_DynamicArray_t* lib_paths; ///< 库路径
} Stru_TargetConfig_t;
```

### Stru_TargetCodeGenResult_t

目标代码生成结果结构体，包含生成的代码和元数据。

```c
typedef struct Stru_TargetCodeGenResult_t {
    bool success;                   ///< 是否成功
    
    // 生成的代码
    const char* assembly_code;      ///< 汇编代码（如果生成汇编）
    const char* object_code;        ///< 目标代码（二进制）
    size_t code_size;               ///< 代码大小（字节）
    
    // 元数据
    Stru_DynamicArray_t* symbols;   ///< 符号表
    Stru_DynamicArray_t* relocations; ///< 重定位信息
    Stru_DynamicArray_t* sections;  ///< 段信息
    Stru_DynamicArray_t* debug_info; ///< 调试信息
    
    // 统计信息
    size_t instruction_count;       ///< 指令数量
    size_t data_size;               ///< 数据段大小
    size_t bss_size;                ///< BSS段大小
    size_t total_size;              ///< 总大小
    
    // 错误和警告
    Stru_DynamicArray_t* errors;    ///< 错误信息
    Stru_DynamicArray_t* warnings;  ///< 警告信息
    
    // 性能信息
    double generation_time;         ///< 生成时间（秒）
    size_t memory_used;             ///< 内存使用量（字节）
} Stru_TargetCodeGenResult_t;
```

### Stru_TargetCodeGeneratorInterface_t

目标代码生成器抽象接口结构体，定义目标代码生成的完整功能。

```c
typedef struct Stru_TargetCodeGeneratorInterface_t {
    // 配置函数
    bool (*initialize)(struct Stru_TargetCodeGeneratorInterface_t* interface, 
                      const Stru_TargetConfig_t* config);
    bool (*configure_target)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                            Eum_TargetPlatform platform, Eum_TargetOS os, Eum_TargetABI abi);
    bool (*set_target_triple)(struct Stru_TargetCodeGeneratorInterface_t* interface, 
                             const char* triple);
    bool (*set_cpu_features)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                            const char* cpu, const char* features);
    
    // 代码生成功能
    Stru_TargetCodeGenResult_t* (*generate_from_ir)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                                   const void* ir, size_t ir_size);
    Stru_TargetCodeGenResult_t* (*generate_from_ast)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                                    Stru_AstNode_t* ast);
    const char* (*generate_assembly)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                    const void* ir, size_t ir_size);
    bool (*generate_object_file)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                const void* ir, size_t ir_size, const char* output_file);
    
    // 优化功能
    void* (*apply_target_optimizations)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                       const void* ir, size_t ir_size, int optimization_level);
    void* (*select_instructions)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                const void* ir, size_t ir_size);
    void* (*allocate_registers)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                               const void* ir, size_t ir_size);
    void* (*schedule_instructions)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                  const void* ir, size_t ir_size);
    
    // 链接功能
    bool (*link_objects)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                        const char** object_files, size_t file_count, const char* output_file);
    bool (*create_static_library)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                 const char** object_files, size_t file_count, const char* output_file);
    bool (*create_dynamic_library)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                  const char** object_files, size_t file_count, const char* output_file);
    
    // 查询功能
    Stru_DynamicArray_t* (*get_supported_platforms)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    Stru_DynamicArray_t* (*get_supported_abis)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    Stru_DynamicArray_t* (*get_supported_formats)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    bool (*is_platform_supported)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                 Eum_TargetPlatform platform, Eum_TargetOS os, Eum_TargetABI abi);
    
    // 错误处理
    bool (*has_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    Stru_DynamicArray_t* (*get_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    void (*clear_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    // 资源管理
    void (*reset)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    void (*destroy)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    // 内部状态
    void* internal_state;
} Stru_TargetCodeGeneratorInterface_t;
```

## 工厂函数

### `F_create_target_codegen_interface`

创建目标代码生成器接口实例。

```c
Stru_TargetCodeGeneratorInterface_t* F_create_target_codegen_interface(Eum_TargetPlatform platform, Eum_TargetOS os);
```

### `F_create_default_target_config`

创建默认目标配置。

```c
Stru_TargetConfig_t F_create_default_target_config(void);
```

### `F_destroy_target_codegen_result`

销毁目标代码生成结果。

```c
void F_destroy_target_codegen_result(Stru_TargetCodeGenResult_t* result);
```

### `F_target_platform_to_string`

目标平台转字符串。

```c
const char* F_target_platform_to_string(Eum_TargetPlatform platform);
```

### `F_target_os_to_string`

目标操作系统转字符串。

```c
const char* F_target_os_to_string(Eum_TargetOS os);
```

### `F_target_abi_to_string`

目标ABI转字符串。

```c
const char* F_target_abi_to_string(Eum_TargetABI abi);
```

### `F_target_format_to_string`

目标格式转字符串。

```c
const char* F_target_format_to_string(Eum_TargetFormat format);
```

## 使用示例

### 生成x86-64 Linux汇编代码

```c
// 创建x86-64 Linux目标代码生成器
Stru_TargetCodeGeneratorInterface_t* target_gen = 
    F_create_target_codegen_interface(Eum_TARGET_PLATFORM_X86_64, Eum_TARGET_OS_LINUX);

// 配置目标
Stru_TargetConfig_t config = F_create_default_target_config();
config.platform = Eum_TARGET_PLATFORM_X86_64;
config.os = Eum_TARGET_OS_LINUX;
config.abi = Eum_TARGET_ABI_SYSV;
config.format = Eum_TARGET_FORMAT_ASSEMBLY;
config.cpu = "x86-64";
config.features = "+avx2,+fma";

// 初始化生成器
if (!target_gen->initialize(target_gen, &config)) {
    printf("目标代码生成器初始化失败\n");
    return;
}

// 从IR生成汇编代码
const char* assembly = target_gen->generate_assembly(target_gen, ir_data, ir_size);
if (assembly) {
    printf("生成的汇编代码:\n%s\n", assembly);
}

// 生成目标文件
if (target_gen->generate_object_file(target_gen, ir_data, ir_size, "output.o")) {
    printf("目标文件生成成功\n");
}

// 清理资源
target_gen->destroy(target_gen);
```

### 链接多个目标文件

```c
// 假设已创建目标代码生成器
const char* object_files[] = {"module1.o", "module2.o", "module3.o"};
size_t file_count = 3;

// 链接为可执行文件
if (target_gen->link_objects(target_gen, object_files, file_count, "program")) {
    printf("链接成功，生成可执行文件: program\n");
}

// 创建静态库
if (target_gen->create_static_library(target_gen, object_files, file_count, "libmylib.a")) {
    printf("静态库创建成功: libmylib.a\n");
}

// 创建动态库
if (target_gen->create_dynamic_library(target_gen, object_files, file_count, "libmylib.so")) {
    printf("动态库创建成功: libmylib.so\n");
}
```

### 查询支持的平台

```c
// 查询支持的平台
Stru_DynamicArray_t* platforms = target_gen->get_supported_platforms(target_gen);
if (platforms) {
    size_t count = F_dynamic_array_length(platforms);
    printf("支持的平台 (%zu 个):\n", count);
    
    for (size_t i = 0; i < count; i++) {
        Eum_TargetPlatform* platform = F_dynamic_array_get(platforms, i);
        printf("  - %s\n", F_target_platform_to_string(*platform));
    }
    
    F_destroy_dynamic_array(platforms);
}

// 检查特定平台支持
bool supported = target_gen->is_platform_supported(target_gen, 
                                                   Eum_TARGET_PLATFORM_ARM64, 
                                                   Eum_TARGET_OS_LINUX, 
                                                   Eum_TARGET_ABI_ELF);
printf("ARM64 Linux ELF 支持: %s\n", supported ? "是" : "否");
```

## 目标平台特性

### x86/x86-64平台
- 支持多种指令集扩展（SSE, AVX, AVX-512）
- 支持多种调用约定（cdecl, stdcall, fastcall）
- 支持位置无关代码（PIC）
- 支持栈保护和安全特性

### ARM/ARM64平台
- 支持ARMv7, ARMv8架构
- 支持Thumb指令集
- 支持NEON SIMD扩展
- 支持AAPCS调用约定

### WebAssembly平台
- 支持WASI系统接口
- 支持多线程和SIMD
- 支持异常处理
- 支持动态链接

### 嵌入式平台
- 最小化代码大小
- 无操作系统支持
- 直接硬件访问
- 实时性要求

## 优化策略

### 目标特定优化
1. **指令选择**：选择最合适的机器指令
2. **寄存器分配**：有效利用寄存器资源
3. **指令调度**：优化指令执行顺序
4. **窥孔优化**：局部指令模式优化

### 代码生成阶段
1. **指令选择阶段**：将IR转换为机器指令
2. **寄存器分配阶段**：分配物理寄存器
3. **指令调度阶段**：重排指令优化流水线
4. **代码发射阶段**：生成最终目标代码

## 链接和重定位

### 静态链接
- 合并所有目标文件
- 解析符号引用
- 分配最终地址
- 生成可执行文件

### 动态链接
- 生成位置无关代码
- 创建共享库
- 延迟绑定支持
- 符号版本控制

### 重定位类型
- **绝对重定位**：直接地址引用
- **相对重定位**：相对地址引用
- **PC相对重定位**：程序计数器相对引用
- **符号重定位**：符号地址引用

## 调试信息

### DWARF格式
- 支持DWARF 2/3/4/5
- 行号信息
- 变量位置信息
- 类型信息
- 调用栈信息

### 其他格式
- STABS（简单调试格式）
- CodeView（Windows调试格式）
- PDB（程序数据库）

## 性能考虑

### 代码生成性能
- 增量代码生成
- 并行指令选择
- 缓存优化结果
