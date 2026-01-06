# CN_Optimizer_Interface API 文档

## 概述

`CN_optimizer_interface.h` 定义了CN_Language优化器插件的抽象接口。该接口支持代码优化和转换功能，采用插件架构实现可扩展的优化系统。

## 文件信息

- **文件名**: `CN_optimizer_interface.h`
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

### Eum_OptimizationType

优化类型枚举，定义支持的优化策略。

```c
typedef enum Eum_OptimizationType {
    Eum_OPTIMIZE_NONE = 0,          ///< 无优化
    Eum_OPTIMIZE_CONSTANT_FOLDING,  ///< 常量折叠
    Eum_OPTIMIZE_DEAD_CODE_ELIM,    ///< 死代码消除
    Eum_OPTIMIZE_INLINING,          ///< 函数内联
    Eum_OPTIMIZE_LOOP_UNROLLING,    ///< 循环展开
    Eum_OPTIMIZE_COMMON_SUBEXPR,    ///< 公共子表达式消除
    Eum_OPTIMIZE_STRENGTH_REDUCTION,///< 强度削减
    Eum_OPTIMIZE_PEEPHOLE,          ///< 窥孔优化
    Eum_OPTIMIZE_REGISTER_ALLOC,    ///< 寄存器分配
    Eum_OPTIMIZE_SCHEDULING,        ///< 指令调度
    Eum_OPTIMIZE_VECTORIZATION,     ///< 向量化
    Eum_OPTIMIZE_PARALLELIZATION,   ///< 并行化
    Eum_OPTIMIZE_MEMORY_ACCESS,     ///< 内存访问优化
    Eum_OPTIMIZE_SIZE,              ///< 代码大小优化
    Eum_OPTIMIZE_SPEED,             ///< 执行速度优化
    Eum_OPTIMIZE_CUSTOM             ///< 自定义优化
} Eum_OptimizationType;
```

### Eum_OptimizationLevel

优化级别枚举，定义优化强度级别。

```c
typedef enum Eum_OptimizationLevel {
    Eum_OPT_LEVEL_NONE = 0,         ///< 无优化
    Eum_OPT_LEVEL_O1,               ///< 基本优化
    Eum_OPT_LEVEL_O2,               ///< 标准优化
    Eum_OPT_LEVEL_O3,               ///< 激进优化
    Eum_OPT_LEVEL_OS,               ///< 代码大小优化
    Eum_OPT_LEVEL_OZ,               ///< 最大代码大小优化
    Eum_OPT_LEVEL_FAST              ///< 快速执行优化
} Eum_OptimizationLevel;
```

## 结构体类型

### Stru_OptimizerPluginInfo_t

优化器插件信息结构体，包含插件的元数据。

```c
typedef struct Stru_OptimizerPluginInfo_t {
    const char* name;               ///< 插件名称
    const char* version;            ///< 插件版本
    const char* author;             ///< 作者
    const char* description;        ///< 插件描述
    Eum_OptimizationType* supported_optimizations; ///< 支持的优化类型数组
    size_t optimization_count;      ///< 支持的优化类型数量
    Eum_OptimizationLevel min_level;///< 最小支持优化级别
    Eum_OptimizationLevel max_level;///< 最大支持优化级别
    bool requires_analysis;         ///< 是否需要分析阶段
    bool modifies_ast;              ///< 是否修改AST
    bool modifies_ir;               ///< 是否修改中间表示
    bool modifies_code;             ///< 是否修改目标代码
} Stru_OptimizerPluginInfo_t;
```

### Stru_OptimizationContext_t

优化上下文结构体，包含优化过程的上下文信息。

```c
typedef struct Stru_OptimizationContext_t {
    Stru_AstNode_t* ast;            ///< 抽象语法树（可选）
    void* intermediate_representation; ///< 中间表示（可选）
    const char* source_code;        ///< 源代码（可选）
    size_t source_length;           ///< 源代码长度
    Eum_OptimizationLevel level;    ///< 优化级别
    Stru_DynamicArray_t* enabled_optimizations; ///< 启用的优化类型
    void* user_data;                ///< 用户数据
    Stru_DynamicArray_t* statistics;///< 优化统计信息
    Stru_DynamicArray_t* warnings;  ///< 警告信息
    Stru_DynamicArray_t* errors;    ///< 错误信息
} Stru_OptimizationContext_t;
```

### Stru_OptimizationResult_t

优化结果结构体，包含优化的结果信息。

```c
typedef struct Stru_OptimizationResult_t {
    bool success;                   ///< 是否成功
    Stru_AstNode_t* optimized_ast;  ///< 优化后的AST（如果修改了AST）
    void* optimized_ir;             ///< 优化后的中间表示（如果修改了IR）
    const char* optimized_code;     ///< 优化后的代码（如果修改了代码）
    size_t code_length;             ///< 优化后代码长度
    size_t original_size;           ///< 原始大小（字节/指令数）
    size_t optimized_size;          ///< 优化后大小（字节/指令数）
    double improvement_ratio;       ///< 改进比例（0.0-1.0）
    Stru_DynamicArray_t* statistics;///< 详细统计信息
    Stru_DynamicArray_t* warnings;  ///< 警告信息
    Stru_DynamicArray_t* errors;    ///< 错误信息
} Stru_OptimizationResult_t;
```

### Stru_OptimizerPluginInterface_t

优化器插件抽象接口结构体，定义优化器插件的完整功能接口。

```c
typedef struct Stru_OptimizerPluginInterface_t {
    // 插件信息
    const Stru_OptimizerPluginInfo_t* (*get_info)(struct Stru_OptimizerPluginInterface_t* interface);
    
    // 初始化函数
    bool (*initialize)(struct Stru_OptimizerPluginInterface_t* interface, Stru_OptimizationContext_t* context);
    
    // 分析功能
    Stru_DynamicArray_t* (*analyze)(struct Stru_OptimizerPluginInterface_t* interface, Stru_OptimizationContext_t* context);
    bool (*can_optimize)(struct Stru_OptimizerPluginInterface_t* interface, 
                         Stru_OptimizationContext_t* context, Eum_OptimizationType optimization_type);
    
    // 优化功能
    Stru_OptimizationResult_t* (*optimize)(struct Stru_OptimizerPluginInterface_t* interface,
                                           Stru_OptimizationContext_t* context,
                                           Eum_OptimizationType optimization_type);
    Stru_OptimizationResult_t* (*optimize_batch)(struct Stru_OptimizerPluginInterface_t* interface,
                                                 Stru_OptimizationContext_t* context,
                                                 Eum_OptimizationType* optimizations, size_t count);
    Stru_OptimizationResult_t* (*optimize_level)(struct Stru_OptimizerPluginInterface_t* interface,
                                                 Stru_OptimizationContext_t* context,
                                                 Eum_OptimizationLevel level);
    
    // 转换功能
    Stru_AstNode_t* (*transform_ast)(struct Stru_OptimizerPluginInterface_t* interface,
                                     Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
    void* (*transform_ir)(struct Stru_OptimizerPluginInterface_t* interface,
                          void* ir, Stru_OptimizationContext_t* context);
    
    // 验证功能
    bool (*validate)(struct Stru_OptimizerPluginInterface_t* interface,
                     void* original, void* optimized, Stru_OptimizationContext_t* context);
    bool (*is_safe)(struct Stru_OptimizerPluginInterface_t* interface,
                    Stru_OptimizationContext_t* context, Eum_OptimizationType optimization_type);
    
    // 统计和报告
    Stru_DynamicArray_t* (*collect_statistics)(struct Stru_OptimizerPluginInterface_t* interface,
                                               Stru_OptimizationContext_t* context);
    const char* (*generate_report)(struct Stru_OptimizerPluginInterface_t* interface,
                                   const Stru_OptimizationResult_t* result);
    
    // 配置功能
    bool (*configure)(struct Stru_OptimizerPluginInterface_t* interface,
                      const char* key, const char* value);
    Stru_DynamicArray_t* (*get_config_options)(struct Stru_OptimizerPluginInterface_t* interface);
    
    // 资源管理
    void (*reset)(struct Stru_OptimizerPluginInterface_t* interface);
    void (*destroy)(struct Stru_OptimizerPluginInterface_t* interface);
    
    // 内部状态
    void* internal_state;
} Stru_OptimizerPluginInterface_t;
```

## 工厂函数

### `F_create_optimizer_plugin_interface`

创建优化器插件接口实例。

```c
Stru_OptimizerPluginInterface_t* F_create_optimizer_plugin_interface(const char* plugin_name);
```

### `F_create_optimization_context`

创建优化上下文。

```c
Stru_OptimizationContext_t* F_create_optimization_context(void);
```

### `F_destroy_optimization_context`

销毁优化上下文。

```c
void F_destroy_optimization_context(Stru_OptimizationContext_t* context);
```

### `F_destroy_optimization_result`

销毁优化结果。

```c
void F_destroy_optimization_result(Stru_OptimizationResult_t* result);
```

### `F_optimization_type_to_string`

优化类型转字符串。

```c
const char* F_optimization_type_to_string(Eum_OptimizationType type);
```

### `F_optimization_level_to_string`

优化级别转字符串。

```c
const char* F_optimization_level_to_string(Eum_OptimizationLevel level);
```

## 使用示例

```c
// 创建常量折叠优化器插件
Stru_OptimizerPluginInterface_t* constant_folder = 
    F_create_optimizer_plugin_interface("constant_folding");

// 获取插件信息
const Stru_OptimizerPluginInfo_t* info = constant_folder->get_info(constant_folder);
printf("插件: %s v%s\n", info->name, info->version);
printf("描述: %s\n", info->description);

// 创建优化上下文
Stru_OptimizationContext_t* context = F_create_optimization_context();
context->ast = ast_root;
context->level = Eum_OPT_LEVEL_O2;

// 初始化优化器
if (!constant_folder->initialize(constant_folder, context)) {
    printf("优化器初始化失败\n");
    return;
}

// 分析代码
Stru_DynamicArray_t* analysis = constant_folder->analyze(constant_folder, context);
printf("分析完成，找到 %zu 个优化机会\n", 
       F_dynamic_array_length(analysis));
F_destroy_dynamic_array(analysis);

// 应用优化
Stru_OptimizationResult_t* result = 
    constant_folder->optimize(constant_folder, context, Eum_OPTIMIZE_CONSTANT_FOLDING);

if (result->success) {
    printf("优化成功，改进比例: %.2f%%\n", result->improvement_ratio * 100);
    printf("原始大小: %zu, 优化后大小: %zu\n", 
           result->original_size, result->optimized_size);
}

// 清理资源
F_destroy_optimization_result(result);
F_destroy_optimization_context(context);
constant_folder->destroy(constant_folder);
```

## 优化类型说明

### 常量折叠 (Constant Folding)
在编译时计算常量表达式，用计算结果替换表达式。

### 死代码消除 (Dead Code Elimination)
移除不会执行的代码，如不可达分支、未使用的变量。

### 函数内联 (Function Inlining)
将小函数调用替换为函数体，减少调用开销。

### 循环展开 (Loop Unrolling)
展开循环体，减少循环控制开销。

### 公共子表达式消除 (Common Subexpression Elimination)
识别并重用重复计算的表达式。

### 强度削减 (Strength Reduction)
用廉价操作替换昂贵操作，如用移位代替乘法。

## 性能考虑

### 优化顺序
优化器应按特定顺序应用优化以获得最佳效果：
1. 常量传播和折叠
2. 死代码消除
3. 公共子表达式消除
4. 循环优化
5. 函数内联
6. 寄存器分配
7. 指令调度

### 分析开销
复杂优化可能需要昂贵的分析阶段，应考虑：
- 分析结果缓存
- 增量分析
- 选择性分析（基于代码特征）

## 安全考虑

### 优化安全性
优化必须保持程序语义不变：
- 不改变可见的副作用
- 不引入新的异常
- 不改变并发行为

### 验证机制
所有优化都应包含验证阶段：
- 优化前后语义等价性检查
- 边界条件验证
- 性能回归检测

## 扩展指南

### 实现新优化器
1. 定义优化器信息结构
2. 实现`Stru_OptimizerPluginInterface_t`接口
3. 注册到优化器工厂
4. 编写测试用例

### 优化器配置
优化器应支持运行时配置：
- 启用/禁用特定优化
- 调整优化参数
- 设置优化阈值
