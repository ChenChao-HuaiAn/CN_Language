# CN_Language 代码生成模块 API 文档

## 概述

代码生成模块是CN_Language编译器的后端部分，负责将抽象语法树（AST）转换为目标代码。本模块采用分层架构和插件系统设计，支持多种目标平台和优化策略。

## 模块结构

```
src/core/codegen/
├── CN_codegen_interface.h          # 代码生成器主接口
├── CN_optimizer_interface.h        # 优化器插件接口
├── CN_target_codegen_interface.h   # 目标代码生成器接口
├── CN_codegen_factory.c            # 代码生成器工厂实现
├── implementations/                # 具体实现（按平台/技术划分）
│   ├── c_backend/                  # C语言后端
│   │   ├── CN_c_backend.h          # C后端接口
│   │   └── CN_c_backend.c          # C后端实现
│   ├── llvm_backend/               # LLVM后端（待实现）
│   ├── x86_backend/                # x86汇编后端（待实现）
│   └── wasm_backend/               # WebAssembly后端（待实现）
├── optimizers/                     # 优化器实现
│   ├── basic_optimizer/            # 基础优化器（待实现）
│   ├── peephole_optimizer/         # 窥孔优化器（待实现）
│   └── instruction_scheduler/      # 指令调度器（待实现）
└── utils/                          # 工具函数（待实现）
```

## 核心接口

### 1. Stru_CodeGeneratorInterface_t

代码生成器的主接口，提供以下功能：

#### 初始化配置
- `initialize()`: 初始化代码生成器
- `set_target()`: 设置目标平台和架构

#### 代码生成
- `generate_code()`: 从AST生成目标代码
- `generate_module_code()`: 生成模块级别的代码

#### 优化支持
- `optimize()`: 应用代码优化
- `register_optimizer_plugin()`: 注册优化器插件

#### 目标文件生成
- `generate_target_file()`: 生成目标文件

#### 状态查询
- `get_target_type()`: 获取当前目标类型
- `get_supported_optimization_levels()`: 获取支持的优化级别
- `get_registered_plugins()`: 获取已注册的插件

#### 错误处理
- `has_errors()`: 检查是否有错误
- `get_errors()`: 获取错误信息
- `clear_errors()`: 清除错误

#### 资源管理
- `reset()`: 重置代码生成器
- `destroy()`: 销毁代码生成器

### 2. Stru_OptimizerPluginInterface_t

优化器插件接口，支持：

#### 插件信息
- `get_info()`: 获取插件元数据信息

#### 初始化
- `initialize()`: 初始化优化器插件

#### 分析功能
- `analyze()`: 分析代码以收集优化信息
- `can_optimize()`: 检查优化可行性

#### 优化功能
- `optimize()`: 应用指定的优化
- `optimize_batch()`: 批量应用优化
- `optimize_level()`: 根据优化级别应用优化

#### 转换功能
- `transform_ast()`: 对AST进行转换优化
- `transform_ir()`: 对中间表示进行转换优化

#### 验证功能
- `validate()`: 验证优化正确性
- `is_safe()`: 检查优化安全性

#### 统计和报告
- `collect_statistics()`: 收集统计信息
- `generate_report()`: 生成优化报告

#### 配置功能
- `configure()`: 配置插件参数
- `get_config_options()`: 获取配置选项

#### 资源管理
- `reset()`: 重置优化器插件
- `destroy()`: 销毁优化器插件

### 3. Stru_TargetCodeGeneratorInterface_t

目标代码生成器接口，提供：

#### 配置功能
- `initialize()`: 初始化目标代码生成器
- `configure_target()`: 配置目标平台、操作系统和ABI
- `set_target_triple()`: 设置目标三元组
- `set_cpu_features()`: 设置CPU特性

#### 代码生成功能
- `generate_from_ir()`: 从IR生成目标代码
- `generate_from_ast()`: 从AST生成目标代码
- `generate_assembly()`: 生成汇编代码
- `generate_object_file()`: 生成目标文件

#### 优化功能
- `apply_target_optimizations()`: 应用目标特定优化
- `select_instructions()`: 执行指令选择
- `allocate_registers()`: 执行寄存器分配
- `schedule_instructions()`: 执行指令调度

#### 链接功能
- `link_objects()`: 链接目标文件
- `create_static_library()`: 创建静态库
- `create_dynamic_library()`: 创建动态库

#### 查询功能
- `get_supported_platforms()`: 获取支持的平台
- `get_supported_abis()`: 获取支持的ABI
- `get_supported_formats()`: 获取支持的格式
- `is_platform_supported()`: 检查平台支持

#### 错误处理
- `has_errors()`: 检查是否有错误
- `get_errors()`: 获取错误信息
- `clear_errors()`: 清除错误

#### 资源管理
- `reset()`: 重置目标代码生成器
- `destroy()`: 销毁目标代码生成器

## 数据类型

### 枚举类型

#### Eum_TargetCodeType
目标代码类型枚举：
- `Eum_TARGET_C`: C语言代码
- `Eum_TARGET_LLVM_IR`: LLVM中间表示
- `Eum_TARGET_X86_64`: x86-64汇编
- `Eum_TARGET_ARM64`: ARM64汇编
- `Eum_TARGET_WASM`: WebAssembly
- `Eum_TARGET_BYTECODE`: 自定义字节码

#### Eum_OptimizationType
优化类型枚举：
- `Eum_OPTIMIZE_NONE`: 无优化
- `Eum_OPTIMIZE_CONSTANT_FOLDING`: 常量折叠
- `Eum_OPTIMIZE_DEAD_CODE_ELIM`: 死代码消除
- `Eum_OPTIMIZE_INLINING`: 函数内联
- `Eum_OPTIMIZE_LOOP_UNROLLING`: 循环展开
- `Eum_OPTIMIZE_COMMON_SUBEXPR`: 公共子表达式消除
- `Eum_OPTIMIZE_STRENGTH_REDUCTION`: 强度削减
- `Eum_OPTIMIZE_PEEPHOLE`: 窥孔优化
- `Eum_OPTIMIZE_REGISTER_ALLOC`: 寄存器分配
- `Eum_OPTIMIZE_SCHEDULING`: 指令调度
- `Eum_OPTIMIZE_VECTORIZATION`: 向量化
- `Eum_OPTIMIZE_PARALLELIZATION`: 并行化
- `Eum_OPTIMIZE_MEMORY_ACCESS`: 内存访问优化
- `Eum_OPTIMIZE_SIZE`: 代码大小优化
- `Eum_OPTIMIZE_SPEED`: 执行速度优化
- `Eum_OPTIMIZE_CUSTOM`: 自定义优化

#### Eum_OptimizationLevel
优化级别枚举：
- `Eum_OPT_LEVEL_NONE`: 无优化
- `Eum_OPT_LEVEL_O1`: 基本优化
- `Eum_OPT_LEVEL_O2`: 标准优化
- `Eum_OPT_LEVEL_O3`: 激进优化
- `Eum_OPT_LEVEL_OS`: 代码大小优化
- `Eum_OPT_LEVEL_OZ`: 最大代码大小优化
- `Eum_OPT_LEVEL_FAST`: 快速执行优化

### 结构体类型

#### Stru_CodeGenOptions_t
代码生成选项结构体：
- `target_type`: 目标代码类型
- `optimize`: 是否启用优化
- `optimization_level`: 优化级别 (0-3)
- `debug_info`: 是否生成调试信息
- `output_file`: 输出文件名（可选）
- `verbose`: 是否输出详细信息

#### Stru_CodeGenResult_t
代码生成结果结构体：
- `success`: 是否成功
- `code`: 生成的代码字符串
- `code_length`: 代码长度
- `errors`: 错误信息数组
- `warnings`: 警告信息数组
- `instruction_count`: 生成的指令数量
- `memory_usage`: 内存使用量（字节）

#### Stru_OptimizerPluginInfo_t
优化器插件信息结构体：
- `name`: 插件名称
- `version`: 插件版本
- `author`: 作者
- `description`: 插件描述
- `supported_optimizations`: 支持的优化类型数组
- `optimization_count`: 支持的优化类型数量
- `min_level`: 最小支持优化级别
- `max_level`: 最大支持优化级别
- `requires_analysis`: 是否需要分析阶段
- `modifies_ast`: 是否修改AST
- `modifies_ir`: 是否修改中间表示
- `modifies_code`: 是否修改目标代码

#### Stru_OptimizationContext_t
优化上下文结构体：
- `ast`: 抽象语法树（可选）
- `intermediate_representation`: 中间表示（可选）
- `source_code`: 源代码（可选）
- `source_length`: 源代码长度
- `level`: 优化级别
- `enabled_optimizations`: 启用的优化类型
- `user_data`: 用户数据
- `statistics`: 优化统计信息
- `warnings`: 警告信息
- `errors`: 错误信息

#### Stru_OptimizationResult_t
优化结果结构体：
- `success`: 是否成功
- `optimized_ast`: 优化后的AST（如果修改了AST）
- `optimized_ir`: 优化后的中间表示（如果修改了IR）
- `optimized_code`: 优化后的代码（如果修改了代码）
- `code_length`: 优化后代码长度
- `original_size`: 原始大小（字节/指令数）
- `optimized_size`: 优化后大小（字节/指令数）
- `improvement_ratio`: 改进比例（0.0-1.0）
- `statistics`: 详细统计信息
- `warnings`: 警告信息
- `errors`: 错误信息

## 工厂函数

### 代码生成器工厂函数

#### `F_create_codegen_interface()`
创建代码生成器接口实例。

**原型：**
```c
Stru_CodeGeneratorInterface_t* F_create_codegen_interface(void);
```

**参数：**
- 无

**返回值：**
- 成功：返回新创建的代码生成器接口实例
- 失败：返回NULL

**说明：**
调用者负责在不再使用时调用`destroy()`函数释放资源。

#### `F_create_default_codegen_options()`
创建默认的代码生成选项。

**原型：**
```c
Stru_CodeGenOptions_t F_create_default_codegen_options(void);
```

**参数：**
- 无

**返回值：**
- 返回默认的代码生成选项

**说明：**
默认选项包括：
- `target_type`: `Eum_TARGET_C`
- `optimize`: `true`
- `optimization_level`: `2`
- `debug_info`: `false`
- `output_file`: `NULL`
- `verbose`: `false`

#### `F_destroy_codegen_result()`
销毁代码生成结果。

**原型：**
```c
void F_destroy_codegen_result(Stru_CodeGenResult_t* result);
```

**参数：**
- `result`: 要销毁的代码生成结果

**返回值：**
- 无

**说明：**
释放代码生成结果占用的所有资源，包括代码字符串、错误信息和警告信息。

### 优化器插件工厂函数

#### `F_create_optimizer_plugin_interface()`
创建优化器插件接口实例。

**原型：**
```c
Stru_OptimizerPluginInterface_t* F_create_optimizer_plugin_interface(const char* plugin_name);
```

**参数：**
- `plugin_name`: 插件名称

**返回值：**
- 成功：返回新创建的优化器插件接口实例
- 失败：返回NULL

#### `F_create_optimization_context()`
创建优化上下文。

**原型：**
```c
Stru_OptimizationContext_t* F_create_optimization_context(void);
```

**参数：**
- 无

**返回值：**
- 成功：返回新创建的优化上下文
- 失败：返回NULL

#### `F_destroy_optimization_context()`
销毁优化上下文。

**原型：**
```c
void F_destroy_optimization_context(Stru_OptimizationContext_t* context);
```

**参数：**
- `context`: 要销毁的优化上下文

**返回值：**
- 无

#### `F_destroy_optimization_result()`
销毁优化结果。

**原型：**
```c
void F_destroy_optimization_result(Stru_OptimizationResult_t* result);
```

**参数：**
- `result`: 要销毁的优化结果

**返回值：**
- 无

### 目标代码生成器工厂函数

#### `F_create_target_codegen_interface()`
创建目标代码生成器接口实例。

**原型：**
```c
Stru_TargetCodeGeneratorInterface_t* F_create_target_codegen_interface(Eum_TargetPlatform platform, Eum_TargetOS os);
```

**参数：**
- `platform`: 目标平台
- `os`: 目标操作系统

**返回值：**
- 成功：返回新创建的目标代码生成器接口实例
- 失败：返回NULL

#### `F_create_default_target_config()`
创建默认的目标配置。

**原型：**
```c
Stru_TargetConfig_t F_create_default_target_config(void);
```

**参数：**
- 无

**返回值：**
- 返回默认的目标配置

## 使用示例

### 基本代码生成

```c
#include "src/core/codegen/CN_codegen_interface.h"

int main(void)
{
    // 创建代码生成器接口
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    if (!codegen) {
        fprintf(stderr, "创建代码生成器失败\n");
        return EXIT_FAILURE;
    }
    
    // 配置生成选项
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    options.target_type = Eum_TARGET_C;
    options.optimize = true;
    options.optimization_level = 2;
    
    // 初始化生成器
    if (!codegen->initialize(codegen, &options)) {
        fprintf(stderr, "初始化代码生成器失败\n");
        codegen->destroy(codegen);
        return EXIT_FAILURE;
    }
    
    // 生成代码（假设ast_root是有效的AST根节点）
    Stru_CodeGenResult_t* result = codegen->generate_code(codegen, ast_root);
    
    // 检查结果
    if (result->success) {
        printf("生成的代码:\n%s\n", result->code);
    } else {
        fprintf(stderr, "代码生成失败\n");
    }
    
    // 清理资源
    F_destroy_codegen_result(result);
    codegen->destroy(codegen);
    
    return EXIT_SUCCESS;
}
```

### 使用优化器插件

```c
#include "src/core/codegen/CN_optimizer_interface.h"

int main(void)
{
    // 创建优化器插件
    Stru_OptimizerPluginInterface_t* optimizer = F_create_optimizer_plugin_interface("constant_folding");
    if (!optimizer) {
        fprintf(stderr, "创建优化器插件失败\n");
        return EXIT_FAILURE;
    }
    
    // 创建优化上下文
    Stru_OptimizationContext_t* context = F_create_optimization_context();
    if (!context) {
        fprintf(stderr, "创建优化上下文失败\n");
        optimizer->destroy(optimizer);
        return EXIT_FAILURE;
    }
    
    context->ast = ast_root;
    context->level = Eum_OPT_LEVEL_O2;
    
    // 初始化优化器
    if (!optimizer->initialize(optimizer, context)) {
        fprintf(stderr, "初始化优化器失败\n");
        F_destroy_optimization_context(context);
        optimizer->destroy(optimizer);
        return EXIT_FAILURE;
    }
    
    // 应用优化
    Stru_OptimizationResult_t* opt_result = optimizer->optimize_level(optimizer, context, Eum_OPT_LEVEL_O2);
    
    // 检查优化结果
    if (opt_result->success) {
        printf("优化改进比例: %.2f%%\n", opt_result->improvement_ratio * 100);
    }
    
    // 清理资源
    F_destroy_optimization_result(opt_result);
    F_destroy_optimization_context(context);
    optimizer->destroy(optimizer);
    
    return EXIT_SUCCESS;
}
```

## 错误处理

代码生成模块使用统一的错误处理机制：

1. **错误收集**：所有错误被收集到错误数组中
2. **错误分类**：错误按严重程度分类（错误、警告、信息）
3. **错误恢复**：尽可能从错误中恢复并继续
4. **错误报告**：提供详细的错误信息和位置

## 版本信息

- **当前版本**: 1.0.0
- **发布日期**: 2026年1月9日
- **兼容性**: 向后兼容

## 许可证

MIT许可证

## 维护者

CN_Language架构委员会
代码生成模块工作组
