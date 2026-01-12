# CN_target_codegen_interface.h - 目标代码生成器接口API文档

## 概述

`CN_target_codegen_interface.h` 定义了CN_Language编译器的目标代码生成器抽象接口。该接口支持将中间表示（IR）或抽象语法树（AST）转换为特定目标平台的机器代码或汇编代码。接口采用插件式架构设计，支持多种目标平台和代码格式。

## 文件信息

- **文件名**: `CN_target_codegen_interface.h`
- **位置**: `src/core/codegen/CN_target_codegen_interface.h`
- **版本**: 1.0.0
- **作者**: CN_Language架构委员会
- **日期**: 2026-01-06
- **依赖**: 
  - `../../../infrastructure/containers/array/CN_dynamic_array.h`
  - `../../ast/CN_ast.h`
  - `../CN_codegen_interface.h`

## 枚举类型

### Eum_TargetPlatform - 目标平台枚举

定义支持的目标平台。

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

### Eum_TargetOS - 目标操作系统枚举

定义支持的目标操作系统。

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

### Eum_TargetABI - 目标ABI枚举

定义支持的目标ABI（应用程序二进制接口）。

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

### Eum_TargetFormat - 目标代码格式枚举

定义支持的目标代码格式。

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

### Stru_TargetConfig_t - 目标配置结构体

定义目标代码生成的完整配置。

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

#### 字段说明

| 字段 | 类型 | 描述 |
|------|------|------|
| `platform` | `Eum_TargetPlatform` | 目标平台 |
| `os` | `Eum_TargetOS` | 目标操作系统 |
| `abi` | `Eum_TargetABI` | 目标ABI |
| `format` | `Eum_TargetFormat` | 目标格式 |
| `code_type` | `Eum_TargetCodeType` | 目标代码类型 |
| `cpu` | `const char*` | CPU型号（可选） |
| `features` | `const char*` | CPU特性字符串（可选） |
| `triple` | `const char*` | LLVM目标三元组（可选） |
| `position_independent` | `bool` | 是否生成位置无关代码 |
| `stack_protector` | `bool` | 是否启用栈保护 |
| `omit_frame_pointer` | `bool` | 是否省略帧指针 |
| `optimize_for_size` | `bool` | 是否优化代码大小 |
| `optimize_for_speed` | `bool` | 是否优化执行速度 |
| `alignment` | `int` | 对齐要求（字节） |
| `stack_alignment` | `int` | 栈对齐要求（字节） |
| `red_zone_size` | `int` | 红区大小（字节） |
| `defines` | `Stru_DynamicArray_t*` | 预定义宏数组 |
| `includes` | `Stru_DynamicArray_t*` | 包含路径数组 |
| `libraries` | `Stru_DynamicArray_t*` | 链接库数组 |
| `lib_paths` | `Stru_DynamicArray_t*` | 库路径数组 |

### Stru_TargetCodeGenResult_t - 目标代码生成结果结构体

包含目标代码生成的详细结果。

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

#### 字段说明

| 字段 | 类型 | 描述 |
|------|------|------|
| `success` | `bool` | 是否成功 |
| `assembly_code` | `const char*` | 汇编代码字符串 |
| `object_code` | `const char*` | 目标代码二进制数据 |
| `code_size` | `size_t` | 代码大小（字节） |
| `symbols` | `Stru_DynamicArray_t*` | 符号表数组 |
| `relocations` | `Stru_DynamicArray_t*` | 重定位信息数组 |
| `sections` | `Stru_DynamicArray_t*` | 段信息数组 |
| `debug_info` | `Stru_DynamicArray_t*` | 调试信息数组 |
| `instruction_count` | `size_t` | 指令数量 |
| `data_size` | `size_t` | 数据段大小 |
| `bss_size` | `size_t` | BSS段大小 |
| `total_size` | `size_t` | 总大小 |
| `errors` | `Stru_DynamicArray_t*` | 错误信息数组 |
| `warnings` | `Stru_DynamicArray_t*` | 警告信息数组 |
| `generation_time` | `double` | 生成时间（秒） |
| `memory_used` | `size_t` | 内存使用量（字节） |

### Stru_TargetCodeGeneratorInterface_t - 目标代码生成器抽象接口结构体

定义目标代码生成器的完整接口，包含配置、代码生成、链接和资源管理功能。所有函数指针必须由具体实现提供。

```c
typedef struct Stru_TargetCodeGeneratorInterface_t {
    // 配置函数
    bool (*initialize)(struct Stru_TargetCodeGeneratorInterface_t* interface, const Stru_TargetConfig_t* config);
    bool (*configure_target)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                             Eum_TargetPlatform platform, Eum_TargetOS os, Eum_TargetABI abi);
    bool (*set_target_triple)(struct Stru_TargetCodeGeneratorInterface_t* interface, const char* triple);
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

## 函数指针详细说明

### 配置函数

#### `initialize`
```c
bool (*initialize)(struct Stru_TargetCodeGeneratorInterface_t* interface, const Stru_TargetConfig_t* config);
```
使用目标配置初始化代码生成器。

**参数**:
- `interface`: 目标代码生成器接口指针
- `config`: 目标配置

**返回值**: 
- `true`: 初始化成功
- `false`: 初始化失败

#### `configure_target`
```c
bool (*configure_target)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                         Eum_TargetPlatform platform, Eum_TargetOS os, Eum_TargetABI abi);
```
配置目标平台、操作系统和ABI。

**参数**:
- `interface`: 目标代码生成器接口指针
- `platform`: 目标平台
- `os`: 目标操作系统
- `abi`: 目标ABI

**返回值**:
- `true`: 配置成功
- `false`: 配置失败

#### `set_target_triple`
```c
bool (*set_target_triple)(struct Stru_TargetCodeGeneratorInterface_t* interface, const char* triple);
```
设置LLVM风格的目标三元组。

**参数**:
- `interface`: 目标代码生成器接口指针
- `triple`: 目标三元组字符串

**返回值**:
- `true`: 设置成功
- `false`: 设置失败

#### `set_cpu_features`
```c
bool (*set_cpu_features)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                         const char* cpu, const char* features);
```
设置目标CPU型号和特性。

**参数**:
- `interface`: 目标代码生成器接口指针
- `cpu`: CPU型号字符串
- `features`: CPU特性字符串

**返回值**:
- `true`: 设置成功
- `false`: 设置失败

### 代码生成功能

#### `generate_from_ir`
```c
Stru_TargetCodeGenResult_t* (*generate_from_ir)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                                const void* ir, size_t ir_size);
```
从中间表示生成目标代码。

**参数**:
- `interface`: 目标代码生成器接口指针
- `ir`: 中间表示数据指针
- `ir_size`: 中间表示数据大小

**返回值**:
- `Stru_TargetCodeGenResult_t*`: 生成结果，调用者负责使用`F_destroy_target_codegen_result`销毁

#### `generate_from_ast`
```c
Stru_TargetCodeGenResult_t* (*generate_from_ast)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                                 Stru_AstNode_t* ast);
```
直接从AST生成目标代码。

**参数**:
- `interface`: 目标代码生成器接口指针
- `ast`: AST根节点指针

**返回值**:
- `Stru_TargetCodeGenResult_t*`: 生成结果，调用者负责使用`F_destroy_target_codegen_result`销毁

#### `generate_assembly`
```c
const char* (*generate_assembly)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                 const void* ir, size_t ir_size);
```
生成人类可读的汇编代码。

**参数**:
- `interface`: 目标代码生成器接口指针
- `ir`: 中间表示数据指针
- `ir_size`: 中间表示数据大小

**返回值**:
- `const char*`: 汇编代码字符串，调用者不应修改

#### `generate_object_file`
```c
bool (*generate_object_file)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                             const void* ir, size_t ir_size, const char* output_file);
```
生成目标文件（.o, .obj等）。

**参数**:
- `interface`: 目标代码生成器接口指针
- `ir`: 中间表示数据指针
- `ir_size`: 中间表示数据大小
- `output_file`: 输出文件路径

**返回值**:
- `true`: 生成成功
- `false`: 生成失败

### 优化功能

#### `apply_target_optimizations`
```c
void* (*apply_target_optimizations)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                    const void* ir, size_t ir_size, int optimization_level);
```
应用目标平台特定的优化。

**参数**:
- `interface`: 目标代码生成器接口指针
- `ir`: 中间表示数据指针
- `ir_size`: 中间表示数据大小
- `optimization_level`: 优化级别

**返回值**:
- `void*`: 优化后的中间表示，调用者负责销毁

#### `select_instructions`
```c
void* (*select_instructions)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                             const void* ir, size_t ir_size);
```
执行指令选择，将通用IR转换为目标特定指令。

**参数**:
- `interface`: 目标代码生成器接口指针
- `ir`: 中间表示数据指针
- `ir_size`: 中间表示数据大小

**返回值**:
- `void*`: 目标特定IR，调用者负责销毁

#### `allocate_registers`
```c
void* (*allocate_registers)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                            const void* ir, size_t ir_size);
```
执行寄存器分配。

**参数**:
- `interface`: 目标代码生成器接口指针
- `ir`: 中间表示数据指针
- `ir_size`: 中间表示数据大小

**返回值**:
- `void*`: 寄存器分配后的IR，调用者负责销毁

#### `schedule_instructions`
```c
void* (*schedule_instructions)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                               const void* ir, size_t ir_size);
```
执行指令调度以优化性能。

**参数**:
- `interface`: 目标代码生成器接口指针
- `ir`: 中间表示数据指针
- `ir_size`: 中间表示数据大小

**返回值**:
- `void*`: 调度后的IR，调用者负责销毁

### 链接功能

#### `link_objects`
```c
bool (*link_objects)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                     const char** object_files, size_t file_count, const char* output_file);
```
将多个目标文件链接为可执行文件或库。

**参数**:
- `interface`: 目标代码生成器接口指针
- `object_files`: 目标文件路径数组
- `file_count`: 文件数量
- `output_file`: 输出文件路径

**返回值**:
- `true`: 链接成功
- `false`: 链接失败

#### `create_static_library`
```c
bool (*create_static_library)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                              const char** object_files, size_t file_count, const char* output_file);
```
创建静态库文件。

**参数**:
- `interface`: 目标代码生成器接口指针
- `object_files`: 目标文件路径数组
- `file_count`: 文件数量
- `output_file`: 输出库文件路径

**返回值**:
- `true`: 创建成功
- `false`: 创建失败

#### `create_dynamic_library`
```c
bool (*create_dynamic_library)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                               const char** object_files, size_t file_count, const char* output_file);
```
创建动态库文件。

**参数**:
- `interface`: 目标代码生成器接口指针
- `object_files`: 目标文件路径数组
- `file_count`: 文件数量
- `output_file`: 输出库文件路径

**返回值**:
- `true`: 创建成功
- `false`: 创建失败

### 查询功能

#### `get_supported_platforms`
```c
Stru_DynamicArray_t* (*get_supported_platforms)(struct Stru_TargetCodeGeneratorInterface_t* interface);
```
获取生成器支持的平台列表。

**参数**:
- `interface`: 目标代码生成器接口指针

**返回值**:
- `Stru_DynamicArray_t*`: 支持的平台数组

#### `get_supported_abis`
```c
Stru_DynamicArray_t* (*get_supported_abis)(struct Stru_TargetCodeGeneratorInterface_t* interface);
```
获取生成器支持的ABI列表。

**参数**:
- `interface`: 目标代码生成器接口指针

**返回值**:
- `Stru_DynamicArray_t*`: 支持的ABI数组

#### `get_supported_formats`
```c
Stru_DynamicArray_t* (*get_supported_formats)(struct Stru_TargetCodeGeneratorInterface_t* interface);
```
获取生成器支持的输出格式列表。

**参数**:
- `interface`: 目标代码生成器接口指针

**返回值**:
- `Stru_DynamicArray_t*`: 支持的格式数组

#### `is_platform_supported`
```c
bool (*is_platform_supported)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                              Eum_TargetPlatform platform, Eum_TargetOS os, Eum_TargetABI abi);
```
检查是否支持特定的平台、操作系统和ABI组合。

**参数**:
- `interface`: 目标代码生成器接口指针
- `platform`: 平台
- `os`: 操作系统
- `abi`: ABI

**返回值**:
- `true`: 支持
- `false`: 不支持

### 错误处理

#### `has_errors`
```c
bool (*has_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
```
检查代码生成过程中是否发生了错误。

**参数**:
- `interface`: 目标代码生成器接口指针

**返回值**:
- `true`: 有错误
- `false`: 没有错误

#### `get_errors`
```c
Stru_DynamicArray_t* (*get_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
```
获取所有错误信息的数组。

**参数**:
- `interface`: 目标代码生成器接口指针

**返回值**:
- `Stru_DynamicArray_t*`: 错误信息数组

#### `clear_errors`
```c
void (*clear_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
```
清除所有错误信息。

**参数**:
- `interface`: 目标代码生成器接口指针

### 资源管理

#### `reset`
```c
void (*reset)(struct Stru_TargetCodeGeneratorInterface_t* interface);
```
重置目标代码生成器到初始状态。

**参数**:
- `interface`: 目标代码生成器接口指针

#### `destroy`
```c
void (*destroy)(struct Stru_TargetCodeGeneratorInterface_t* interface);
```
释放目标代码生成器占用的所有资源。

**参数**:
- `interface`: 目标代码生成器接口指针

## 工厂函数

### `F_create_target_codegen_interface`
```c
Stru_TargetCodeGeneratorInterface_t* F_create_target_codegen_interface(Eum_TargetPlatform platform, Eum_TargetOS os);
```
创建并返回一个新的目标代码生成器接口实例。

**参数**:
- `platform`: 目标平台
- `os`: 目标操作系统

**返回值**:
- `Stru_TargetCodeGeneratorInterface_t*`: 新创建的目标代码生成器接口实例，调用者负责在不再使用时调用`destroy`函数

### `F_create_default_target_config`
```c
Stru_TargetConfig_t F_create_default_target_config(void);
```
创建并返回默认的目标配置。

**返回值**:
- `Stru_TargetConfig_t`: 默认目标配置

### `F_destroy_target_codegen_result`
```c
void F_destroy_target_codegen_result(Stru_TargetCodeGenResult_t* result);
```
释放目标代码生成结果占用的所有资源。

**参数**:
- `result`: 要销毁的目标代码生成结果

### `F_target_platform_to_string`
```c
const char* F_target_platform_to_string(Eum_TargetPlatform platform);
```
将目标平台转换为可读的字符串表示。

**参数**:
- `platform`: 目标平台

**返回值**:
- `const char*`: 平台字符串表示

### `F_target_os_to_string`
```c
const char* F_target_os_to_string(Eum_TargetOS os);
```
将目标操作系统转换为可读的字符串表示。

**参数**:
- `os`: 目标操作系统

**返回值**:
- `const char*`: 操作系统字符串表示

### `F_target_abi_to_string`
```c
const char* F_target_abi_to_string(Eum_TargetABI abi);
```
将目标ABI转换为可读的字符串表示。

**参数**:
- `abi`: 目标ABI

**返回值**:
- `const char*`: ABI字符串表示

### `F_target_format_to_string`
```c
const char* F_target_format_to_string(Eum_TargetFormat format);
```
将目标格式转换为可读的字符串表示。

**参数**:
- `format`: 目标格式

**返回值**:
- `const char*`: 格式字符串表示

## 使用示例

### 基本使用
```c
#include "CN_target_codegen_interface.h"

int main() {
    // 创建目标代码生成器
    Stru_TargetCodeGeneratorInterface_t* codegen = 
        F_create_target_codegen_interface(Eum_TARGET_PLATFORM_X86_64, Eum_TARGET_OS_LINUX);
    
    if (!codegen) {
        fprintf(stderr, "无法创建目标代码生成器\n");
        return 1;
    }
    
    // 配置目标
    Stru_TargetConfig_t config = F_create_default_target_config();
    config.platform = Eum_TARGET_PLATFORM_X86_64;
    config.os = Eum_TARGET_OS_LINUX;
    config.abi = Eum_TARGET_ABI_SYSV;
    config.format = Eum_TARGET_FORMAT_ASSEMBLY;
    config.optimize_for_speed = true;
    
    // 初始化
    if (codegen->initialize) {
        if (!codegen->initialize(codegen, &config)) {
            fprintf(stderr, "初始化失败\n");
            codegen->destroy(codegen);
            return 1;
        }
    }
    
    // 生成代码（示例）
    // Stru_TargetCodeGenResult_t* result = codegen->generate_from_ir(codegen, ir_data, ir_size);
    
    // 清理资源
    if (codegen->destroy) {
        codegen->destroy(codegen);
    }
    
    return 0;
}
```

### 枚举类型转换
```c
#include "CN_target_codegen_interface.h"

void print_target_info(Eum_TargetPlatform platform, Eum_TargetOS os, 
                       Eum_TargetABI abi, Eum_TargetFormat format) {
    printf("目标平台: %s\n", F_target_platform_to_string(platform));
    printf("操作系统: %s\n", F_target_os_to_string(os));
    printf("ABI: %s\n", F_target_abi_to_string(abi));
    printf("输出格式: %s\n", F_target_format_to_string(format));
}
```

## 设计原则

### 1. 接口隔离原则
- 每个函数指针专注于单一功能
- 客户端只依赖它们需要的接口
- 避免"胖接口"问题

### 2. 依赖倒置原则
- 高层模块定义抽象接口
- 低层模块实现具体功能
- 通过工厂函数创建实例

### 3. 开闭原则
- 接口对扩展开放，对修改封闭
- 新的目标平台可以通过实现接口添加
- 现有代码不需要修改

### 4. 单一职责原则
- 每个函数有明确的职责
- 配置、生成、优化、链接功能分离
- 错误处理和资源管理独立

## 错误处理策略

### 1. 配置阶段错误
- 在`initialize`和配置函数中返回`false`
- 提供详细的错误信息

### 2. 生成阶段错误
- 通过`Stru_TargetCodeGenResult_t`结构返回错误
- 包含错误位置和描述信息
- 支持多个错误同时报告

### 3. 运行时错误
- 使用返回值指示成功/失败
- 通过错误查询函数获取详细信息
- 支持错误清除和恢复

## 内存管理

### 所有权规则
1. 工厂函数创建的对象由调用者负责销毁
2. 生成函数返回的结果由调用者负责销毁
3. 动态数组由接口内部管理
4. 字符串常量不应被修改或释放

### 资源清理
- 使用`destroy`函数释放接口实例
- 使用`F_destroy_target_codegen_result`释放生成结果
- 及时释放临时资源

## 线程安全性

### 线程安全级别
- **非线程安全**: 接口实例不应在多个线程间共享
- **可重入**: 同一线程可多次调用接口函数
- **线程局部**: 每个线程应创建自己的接口实例

### 最佳实践
1. 每个线程创建独立的目标代码生成器
2. 避免在生成过程中修改配置
3. 使用线程局部存储管理状态

## 性能考虑

### 编译时性能
- 懒加载目标后端实现
- 缓存常用配置
- 重用中间数据结构

### 运行时性能
- 优化的指令选择算法
- 智能寄存器分配
- 高效的指令调度

### 内存使用
- 流式代码生成减少内存占用
- 及时释放临时资源
- 使用内存池分配频繁创建的对象

## 扩展指南

### 添加新目标平台
1. 实现`Stru_TargetCodeGeneratorInterface_t`接口的所有函数
2. 提供平台特定的优化实现
3. 在工厂函数中注册新平台
4. 编写测试用例验证功能

### 添加新功能
1. 在接口结构体中添加新的函数指针
2. 提供默认实现（可选）
3. 更新工厂函数和文档
4. 保持向后兼容性

## 测试策略

### 单元测试
- 测试每个接口函数的独立功能
- 验证错误处理路径
- 检查边界条件

### 集成测试
- 测试完整的目标代码生成流程
- 验证不同平台的兼容性
- 检查生成代码的正确性

### 性能测试
- 测量代码生成时间
- 分析内存使用情况
- 评估生成代码质量

## 相关文档

- [src/core/codegen/target_codegen/README.md](../../../../src/core/codegen/target_codegen/README.md) - 模块概述
- [src/core/codegen/CN_codegen_interface.h](../../../../src/core/codegen/CN_codegen_interface.h) - 代码生成器主接口
- [examples/target_codegen/](../../../../examples/target_codegen/) - 示例代码

## 版本历史

- **v1.0.0** (2026-01-06): 初始版本，定义核心接口
- **v1.0.1** (2026-01-12): 修复包含路径，添加API文档

## 维护者

- CN_Language架构委员会
- 目标代码生成器工作组

## 许可证

MIT License
