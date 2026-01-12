# CN_Language 目标代码生成器模块

## 概述

目标代码生成器模块是CN_Language编译器后端的关键组件，负责将中间表示（IR）或抽象语法树（AST）转换为特定目标平台的机器代码或汇编代码。本模块采用模块化设计，支持多种目标平台和代码格式。

## 模块结构

```
src/core/codegen/target_codegen/
├── README.md                          # 本文件
├── config/                            # 配置管理模块
│   └── CN_target_codegen_config.c     # 目标配置实现
├── factory/                           # 工厂函数模块
│   └── CN_target_codegen_factory.c    # 接口工厂实现
├── result/                            # 结果管理模块
│   └── CN_target_codegen_result.c     # 结果管理实现
└── utils/                             # 工具函数模块
    └── CN_target_codegen_utils.c      # 工具函数实现
```

## 核心接口

### Stru_TargetCodeGeneratorInterface_t

目标代码生成器的主接口，定义在`CN_target_codegen_interface.h`中，提供以下功能：

#### 初始化与配置
- `initialize`: 初始化目标代码生成器
- `configure_target`: 配置目标平台参数
- `set_target_triple`: 设置目标三元组（架构-厂商-操作系统）
- `set_cpu_features`: 设置CPU特性

#### 代码生成
- `generate_from_ir`: 从IR生成目标代码
- `generate_from_ast`: 从AST生成目标代码
- `generate_assembly`: 生成汇编代码
- `generate_object_file`: 生成目标文件

#### 优化与调度
- `apply_target_optimizations`: 应用目标特定优化
- `select_instructions`: 选择指令
- `allocate_registers`: 分配寄存器
- `schedule_instructions`: 调度指令

#### 链接与库管理
- `link_objects`: 链接目标文件
- `create_static_library`: 创建静态库
- `create_dynamic_library`: 创建动态库

#### 平台查询
- `get_supported_platforms`: 获取支持的平台列表
- `get_supported_abis`: 获取支持的ABI列表
- `get_supported_formats`: 获取支持的格式列表
- `is_platform_supported`: 检查平台是否支持

#### 错误处理
- `has_errors`: 检查是否有错误
- `get_errors`: 获取错误列表
- `clear_errors`: 清除错误

#### 资源管理
- `reset`: 重置生成器状态
- `destroy`: 销毁生成器实例

## 数据结构

### 目标配置 (Stru_TargetConfig_t)
```c
typedef struct Stru_TargetConfig_t {
    Eum_TargetPlatform platform;      // 目标平台
    Eum_TargetOS os;                  // 目标操作系统
    Eum_TargetABI abi;                // 目标ABI
    Eum_TargetFormat format;          // 目标格式
    Eum_TargetCodeType code_type;     // 代码类型
    
    bool position_independent;        // 位置无关代码
    bool stack_protector;             // 栈保护
    bool omit_frame_pointer;          // 省略帧指针
    bool optimize_for_size;           // 优化代码大小
    bool optimize_for_speed;          // 优化执行速度
    
    size_t alignment;                 // 对齐要求
    size_t stack_alignment;           // 栈对齐
    size_t red_zone_size;             // 红区大小
    
    char** defines;                   // 预定义宏
    char** includes;                  // 包含路径
    char** libraries;                 // 链接库
    char** lib_paths;                 // 库路径
} Stru_TargetConfig_t;
```

### 目标代码生成结果 (Stru_TargetCodeGenResult_t)
```c
typedef struct Stru_TargetCodeGenResult_t {
    bool success;                     // 是否成功
    const char* assembly_code;        // 汇编代码
    const char* object_code;          // 目标代码
    size_t object_size;               // 目标代码大小
    
    Stru_SymbolInfo_t* symbols;       // 符号表
    size_t symbol_count;              // 符号数量
    
    Stru_RelocationInfo_t* relocations; // 重定位信息
    size_t relocation_count;          // 重定位数量
    
    Stru_SectionInfo_t* sections;     // 节信息
    size_t section_count;             // 节数量
    
    Stru_DebugInfo_t* debug_info;     // 调试信息
    size_t debug_info_count;          // 调试信息数量
    
    Stru_ErrorInfo_t* errors;         // 错误信息
    size_t error_count;               // 错误数量
    
    Stru_WarningInfo_t* warnings;     // 警告信息
    size_t warning_count;             // 警告数量
} Stru_TargetCodeGenResult_t;
```

## 枚举类型

### 目标平台 (Eum_TargetPlatform)
```c
enum Eum_TargetPlatform {
    Eum_TARGET_PLATFORM_GENERIC,      // 通用平台
    Eum_TARGET_PLATFORM_X86,          // x86平台
    Eum_TARGET_PLATFORM_X86_64,       // x86-64平台
    Eum_TARGET_PLATFORM_ARM,          // ARM平台
    Eum_TARGET_PLATFORM_ARM64,        // ARM64平台
    Eum_TARGET_PLATFORM_MIPS,         // MIPS平台
    Eum_TARGET_PLATFORM_POWERPC,      // PowerPC平台
    Eum_TARGET_PLATFORM_RISCV,        // RISC-V平台
    Eum_TARGET_PLATFORM_WASM,         // WebAssembly平台
    Eum_TARGET_PLATFORM_JVM,          // Java虚拟机平台
    Eum_TARGET_PLATFORM_DOTNET,       // .NET平台
    Eum_TARGET_PLATFORM_CUSTOM        // 自定义平台
};
```

### 目标操作系统 (Eum_TargetOS)
```c
enum Eum_TargetOS {
    Eum_TARGET_OS_GENERIC,            // 通用操作系统
    Eum_TARGET_OS_LINUX,              // Linux
    Eum_TARGET_OS_WINDOWS,            // Windows
    Eum_TARGET_OS_MACOS,              // macOS
    Eum_TARGET_OS_ANDROID,            // Android
    Eum_TARGET_OS_IOS,                // iOS
    Eum_TARGET_OS_FREEBSD,            // FreeBSD
    Eum_TARGET_OS_EMBEDDED,           // 嵌入式系统
    Eum_TARGET_OS_BARE_METAL,         // 裸机系统
    Eum_TARGET_OS_CUSTOM              // 自定义操作系统
};
```

### 目标格式 (Eum_TargetFormat)
```c
enum Eum_TargetFormat {
    Eum_TARGET_FORMAT_ASSEMBLY,       // 汇编格式
    Eum_TARGET_FORMAT_OBJECT,         // 目标文件格式
    Eum_TARGET_FORMAT_EXECUTABLE,     // 可执行文件格式
    Eum_TARGET_FORMAT_LIBRARY,        // 库文件格式
    Eum_TARGET_FORMAT_SHARED_LIB,     // 共享库格式
    Eum_TARGET_FORMAT_BYTECODE,       // 字节码格式
    Eum_TARGET_FORMAT_IR,             // 中间表示格式
    Eum_TARGET_FORMAT_CUSTOM          // 自定义格式
};
```

## 模块功能

### 1. 配置管理模块 (config/)
负责创建和管理目标配置，包括：
- 创建默认目标配置
- 验证配置有效性
- 序列化和反序列化配置
- 平台特定配置适配

### 2. 工厂函数模块 (factory/)
负责创建目标代码生成器接口实例，包括：
- 根据平台和操作系统创建相应实现
- 初始化接口函数指针
- 管理实现实例的生命周期

### 3. 结果管理模块 (result/)
负责管理代码生成结果，包括：
- 创建和初始化结果结构
- 添加错误和警告信息
- 释放结果占用的资源
- 序列化结果数据

### 4. 工具函数模块 (utils/)
提供辅助功能，包括：
- 枚举类型转字符串
- 平台特性检测
- 格式验证和转换
- 调试信息生成

## 使用示例

### 创建目标代码生成器
```c
#include "CN_target_codegen_interface.h"

// 创建目标代码生成器接口
Stru_TargetCodeGeneratorInterface_t* target_codegen = 
    F_create_target_codegen_interface(Eum_TARGET_PLATFORM_X86_64, Eum_TARGET_OS_LINUX);

if (!target_codegen) {
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

// 初始化生成器
if (target_codegen->initialize) {
    target_codegen->initialize(target_codegen, &config);
}

// 生成代码
Stru_TargetCodeGenResult_t* result = NULL;
if (target_codegen->generate_from_ir) {
    result = target_codegen->generate_from_ir(target_codegen, ir_module);
}

// 检查结果
if (result && result->success) {
    printf("生成的汇编代码:\n%s\n", result->assembly_code);
} else {
    fprintf(stderr, "代码生成失败\n");
    if (result && result->errors) {
        for (size_t i = 0; i < result->error_count; i++) {
            fprintf(stderr, "错误: %s\n", result->errors[i].message);
        }
    }
}

// 清理资源
if (result) {
    F_destroy_target_codegen_result(result);
}
if (target_codegen->destroy) {
    target_codegen->destroy(target_codegen);
}
```

### 枚举类型转换
```c
#include "CN_target_codegen_interface.h"

// 将枚举转换为可读字符串
const char* platform_str = F_target_platform_to_string(Eum_TARGET_PLATFORM_X86_64);
const char* os_str = F_target_os_to_string(Eum_TARGET_OS_LINUX);
const char* abi_str = F_target_abi_to_string(Eum_TARGET_ABI_SYSV);
const char* format_str = F_target_format_to_string(Eum_TARGET_FORMAT_ASSEMBLY);

printf("目标平台: %s\n", platform_str);
printf("操作系统: %s\n", os_str);
printf("ABI: %s\n", abi_str);
printf("输出格式: %s\n", format_str);
```

## 设计原则

### 1. 模块化设计
- 每个子模块专注于单一职责
- 清晰的模块边界和接口
- 可独立测试和替换

### 2. 平台无关性
- 抽象接口隐藏平台细节
- 通过配置适配不同平台
- 统一的错误处理机制

### 3. 可扩展性
- 支持添加新的目标平台
- 插件式架构设计
- 配置驱动的行为定制

### 4. 资源安全
- 明确的所有权管理
- 自动资源清理
- 内存泄漏检测

## 错误处理

目标代码生成器使用分层错误处理策略：

1. **配置错误**：在初始化阶段检测和报告
2. **生成错误**：在代码生成过程中收集
3. **链接错误**：在链接阶段处理
4. **验证错误**：在结果验证阶段检查

所有错误都通过`Stru_TargetCodeGenResult_t`结构返回，包含详细的错误信息和位置。

## 性能考虑

### 编译时性能
- 懒加载目标后端实现
- 缓存常用配置
- 并行代码生成（如果支持）

### 运行时性能
- 优化的指令选择算法
- 智能寄存器分配
- 高效的指令调度

### 内存使用
- 流式代码生成减少内存占用
- 重用中间数据结构
- 及时释放临时资源

## 测试策略

### 单元测试
- 测试每个子模块的独立功能
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

## 扩展指南

### 添加新目标平台
1. 实现`Stru_TargetCodeGeneratorInterface_t`接口
2. 提供平台特定的优化实现
3. 在工厂函数中注册新平台
4. 编写测试用例验证功能

### 添加新输出格式
1. 扩展`Eum_TargetFormat`枚举
2. 实现格式特定的序列化逻辑
3. 更新配置验证逻辑
4. 添加格式转换工具

## 维护说明

### 版本兼容性
- 保持公共API向后兼容
- 使用版本号管理接口变更
- 提供迁移指南

### 文档更新
- 更新API文档反映接口变更
- 维护示例代码
- 记录已知问题和限制

### 依赖管理
- 明确声明平台依赖
- 管理第三方库版本
- 处理平台特定差异

## 相关文档

- [CN_target_codegen_interface.h](../CN_target_codegen_interface.h) - 接口定义
- [docs/api/core/codegen/target_codegen/](../docs/api/core/codegen/target_codegen/) - API文档
- [examples/target_codegen/](../../../../examples/target_codegen/) - 示例代码

## 版本历史

- **v1.0.0** (2026-01-12)：初始版本，定义核心接口和模块结构
- **v1.1.0** (计划)：添加x86/x86-64后端实现
- **v1.2.0** (计划)：添加ARM/ARM64后端支持
- **v1.3.0** (计划)：添加WebAssembly后端支持

## 维护者

- CN_Language架构委员会
- 目标代码生成器工作组

## 许可证

MIT License
