# CN_Language 优化器模块 API 文档

## 概述

优化器模块是CN_Language编译器的核心组件之一，负责对抽象语法树（AST）和中间表示（IR）进行各种优化，以提高生成代码的性能和效率。本模块采用插件式架构，支持多种优化算法，包括常量折叠、死代码消除、公共子表达式消除等。

## 模块结构

```
src/core/codegen/optimizers/
├── CN_optimizer_interface.h          # 优化器插件接口定义
├── basic_optimizer/                  # 基础优化器实现
│   ├── CN_basic_optimizer.h          # 基础优化器公共接口
│   ├── CN_basic_optimizer.c          # 基础优化器主实现
│   ├── CN_constant_folding.c         # 常量折叠算法实现
│   ├── CN_constant_folding.h         # 常量折叠算法接口
│   ├── CN_dead_code_elimination.c    # 死代码消除算法实现
│   ├── CN_dead_code_elimination.h    # 死代码消除算法接口
│   └── README.md                     # 基础优化器文档
├── peephole_optimizer/               # 窥孔优化器（待实现）
├── instruction_scheduler/            # 指令调度器（待实现）
└── utils/                            # 优化器工具函数（待实现）
```

## 核心接口

### 1. `Stru_OptimizerPluginInterface_t`

优化器插件接口，提供以下功能：

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

## 基础优化器 API

### 1. `F_create_basic_optimizer_interface`

**功能：** 创建基础优化器插件接口实例

**原型：**
```c
Stru_OptimizerPluginInterface_t* F_create_basic_optimizer_interface(void);
```

**返回值：**
- 成功：返回新创建的基础优化器插件接口实例
- 失败：返回NULL

**说明：**
- 调用者负责在不再使用时调用`destroy()`函数释放资源
- 创建的基础优化器支持常量折叠、死代码消除等基本优化

**示例：**
```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_basic_optimizer.h"

int main(void)
{
    // 创建基础优化器插件
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
    if (!optimizer) {
        fprintf(stderr, "创建优化器插件失败\n");
        return EXIT_FAILURE;
    }
    
    // 使用优化器...
    
    // 清理资源
    optimizer->destroy(optimizer);
    
    return EXIT_SUCCESS;
}
```

### 2. `F_get_basic_optimizer_version`

**功能：** 获取基础优化器版本信息

**原型：**
```c
void F_get_basic_optimizer_version(int* major, int* minor, int* patch);
```

**参数：**
- `major`: 输出参数，主版本号
- `minor`: 输出参数，次版本号
- `patch`: 输出参数，修订号

**说明：**
- 版本号遵循语义化版本规范

**示例：**
```c
int major, minor, patch;
F_get_basic_optimizer_version(&major, &minor, &patch);
printf("基础优化器版本: %d.%d.%d\n", major, minor, patch);
```

### 3. `F_get_basic_optimizer_version_string`

**功能：** 获取基础优化器版本字符串

**原型：**
```c
const char* F_get_basic_optimizer_version_string(void);
```

**返回值：**
- 版本字符串，格式为"主版本号.次版本号.修订号"

**说明：**
- 返回的字符串是静态常量，调用者不应修改或释放

**示例：**
```c
const char* version = F_get_basic_optimizer_version_string();
printf("基础优化器版本: %s\n", version);
```

### 4. `F_basic_optimizer_supports_optimization`

**功能：** 检查是否支持特定优化类型

**原型：**
```c
bool F_basic_optimizer_supports_optimization(Eum_OptimizationType optimization_type);
```

**参数：**
- `optimization_type`: 优化类型枚举值

**返回值：**
- 支持返回`true`，不支持返回`false`

**支持的优化类型：**
- `Eum_OPTIMIZE_CONSTANT_FOLDING`: 常量折叠
- `Eum_OPTIMIZE_DEAD_CODE_ELIM`: 死代码消除
- `Eum_OPTIMIZE_COMMON_SUBEXPR`: 公共子表达式消除
- `Eum_OPTIMIZE_STRENGTH_REDUCTION`: 强度削减
- `Eum_OPTIMIZE_PEEPHOLE`: 窥孔优化

**示例：**
```c
if (F_basic_optimizer_supports_optimization(Eum_OPTIMIZE_CONSTANT_FOLDING)) {
    printf("支持常量折叠优化\n");
}
```

### 5. `F_get_basic_optimizer_supported_optimizations`

**功能：** 获取基础优化器支持的所有优化类型

**原型：**
```c
const Eum_OptimizationType* F_get_basic_optimizer_supported_optimizations(size_t* count);
```

**参数：**
- `count`: 输出参数，优化类型数量

**返回值：**
- 优化类型数组，调用者不应修改

**示例：**
```c
size_t count;
const Eum_OptimizationType* optimizations = 
    F_get_basic_optimizer_supported_optimizations(&count);
    
printf("基础优化器支持 %zu 种优化:\n", count);
for (size_t i = 0; i < count; i++) {
    printf("  - %d\n", optimizations[i]);
}
```

## 优化算法 API

### 常量折叠算法

#### `apply_constant_folding`

**功能：** 应用常量折叠优化

**原型：**
```c
bool apply_constant_folding(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
```

**参数：**
- `ast`: 抽象语法树根节点
- `context`: 优化上下文

**返回值：**
- 优化成功应用返回`true`，失败返回`false`

**算法描述：**
常量折叠是一种编译时优化技术，用于在编译阶段计算常量表达式，而不是在运行时计算。例如：
- `2 + 3 * 4` → `14`
- `(2 + 3) * 4` → `20`
- `true && false` → `false`

**实现特性：**
1. 支持整数、浮点数、布尔值和字符串常量
2. 支持类型转换和兼容性检查
3. 支持所有基本运算符的常量计算
4. 提供错误处理和报告机制

**示例：**
```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_constant_folding.h"

int main(void)
{
    // 创建AST和优化上下文
    Stru_AstNode_t* ast = ...;
    Stru_OptimizationContext_t* context = ...;
    
    // 应用常量折叠
    if (apply_constant_folding(ast, context)) {
        printf("常量折叠优化成功应用\n");
    }
    
    return EXIT_SUCCESS;
}
```

#### `test_constant_folding`

**功能：** 测试常量折叠算法

**原型：**
```c
void test_constant_folding(void);
```

**说明：**
- 运行常量折叠算法的测试用例
- 输出测试结果和状态

**示例：**
```c
test_constant_folding();
```

### 死代码消除算法

#### `apply_dead_code_elimination`

**功能：** 应用死代码消除优化

**原型：**
```c
bool apply_dead_code_elimination(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
```

**参数：**
- `ast`: 抽象语法树根节点
- `context`: 优化上下文

**返回值：**
- 优化成功应用返回`true`，失败返回`false`

**算法描述：**
死代码消除是一种优化技术，用于移除不会执行的代码或计算结果不会被使用的代码。包括：
1. **不可达代码**: 永远不会执行的代码块
2. **死赋值**: 赋值后从未使用的变量
3. **未使用变量**: 声明后从未使用的变量

**实现特性：**
1. 构建控制流图（CFG）分析代码可达性
2. 分析变量使用情况识别死赋值
3. 检查副作用确保安全优化
4. 支持增量优化和批量优化

**示例：**
```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_dead_code_elimination.h"

int main(void)
{
    // 创建AST和优化上下文
    Stru_AstNode_t* ast = ...;
    Stru_OptimizationContext_t* context = ...;
    
    // 应用死代码消除
    if (apply_dead_code_elimination(ast, context)) {
        printf("死代码消除优化成功应用\n");
    }
    
    return EXIT_SUCCESS;
}
```

#### `test_dead_code_elimination`

**功能：** 测试死代码消除算法

**原型：**
```c
void test_dead_code_elimination(void);
```

**说明：**
- 运行死代码消除算法的测试用例
- 输出测试结果和状态

**示例：**
```c
test_dead_code_elimination();
```

## 数据类型

### 枚举类型

#### `Eum_OptimizationType`
优化类型枚举：
```c
enum Eum_OptimizationType {
    Eum_OPTIMIZE_NONE = 0,                ///< 无优化
    Eum_OPTIMIZE_CONSTANT_FOLDING,        ///< 常量折叠
    Eum_OPTIMIZE_DEAD_CODE_ELIM,          ///< 死代码消除
    Eum_OPTIMIZE_INLINING,                ///< 函数内联
    Eum_OPTIMIZE_LOOP_UNROLLING,          ///< 循环展开
    Eum_OPTIMIZE_COMMON_SUBEXPR,          ///< 公共子表达式消除
    Eum_OPTIMIZE_STRENGTH_REDUCTION,      ///< 强度削减
    Eum_OPTIMIZE_PEEPHOLE,                ///< 窥孔优化
    Eum_OPTIMIZE_REGISTER_ALLOC,          ///< 寄存器分配
    Eum_OPTIMIZE_SCHEDULING,              ///< 指令调度
    Eum_OPTIMIZE_VECTORIZATION,           ///< 向量化
    Eum_OPTIMIZE_PARALLELIZATION,         ///< 并行化
    Eum_OPTIMIZE_MEMORY_ACCESS,           ///< 内存访问优化
    Eum_OPTIMIZE_SIZE,                    ///< 代码大小优化
    Eum_OPTIMIZE_SPEED,                   ///< 执行速度优化
    Eum_OPTIMIZE_CUSTOM                   ///< 自定义优化
};
```

#### `Eum_OptimizationLevel`
优化级别枚举：
```c
enum Eum_OptimizationLevel {
    Eum_OPT_LEVEL_NONE = 0,               ///< 无优化
    Eum_OPT_LEVEL_O1,                     ///< 基本优化
    Eum_OPT_LEVEL_O2,                     ///< 标准优化
    Eum_OPT_LEVEL_O3,                     ///< 激进优化
    Eum_OPT_LEVEL_OS,                     ///< 代码大小优化
    Eum_OPT_LEVEL_OZ,                     ///< 最大代码大小优化
    Eum_OPT_LEVEL_FAST                    ///< 快速执行优化
};
```

### 结构体类型

#### `Stru_OptimizerPluginInfo_t`
优化器插件信息结构体：
```c
typedef struct {
    const char* name;                     ///< 插件名称
    const char* version;                  ///< 插件版本
    const char* author;                   ///< 作者
    const char* description;              ///< 插件描述
    const Eum_OptimizationType* supported_optimizations; ///< 支持的优化类型数组
    size_t optimization_count;            ///< 支持的优化类型数量
    Eum_OptimizationLevel min_level;      ///< 最小支持优化级别
    Eum_OptimizationLevel max_level;      ///< 最大支持优化级别
    bool requires_analysis;               ///< 是否需要分析阶段
    bool modifies_ast;                    ///< 是否修改AST
    bool modifies_ir;                     ///< 是否修改中间表示
    bool modifies_code;                   ///< 是否修改目标代码
} Stru_OptimizerPluginInfo_t;
```

#### `Stru_OptimizationContext_t`
优化上下文结构体：
```c
typedef struct {
    Stru_AstNode_t* ast;                  ///< 抽象语法树（可选）
    void* intermediate_representation;    ///< 中间表示（可选）
    const char* source_code;              ///< 源代码（可选）
    size_t source_length;                 ///< 源代码长度
    Eum_OptimizationLevel level;          ///< 优化级别
    Eum_OptimizationType* enabled_optimizations; ///< 启用的优化类型
    size_t enabled_count;                 ///< 启用的优化类型数量
    void* user_data;                      ///< 用户数据
    Stru_DynamicArray_t* statistics;      ///< 优化统计信息
    Stru_DynamicArray_t* warnings;        ///< 警告信息
    Stru_DynamicArray_t* errors;          ///< 错误信息
} Stru_OptimizationContext_t;
```

#### `Stru_OptimizationResult_t`
优化结果结构体：
```c
typedef struct {
    bool success;                         ///< 是否成功
    Stru_AstNode_t* optimized_ast;        ///< 优化后的AST（如果修改了AST）
    void* optimized_ir;                   ///< 优化后的中间表示（如果修改了IR）
    char* optimized_code;                 ///< 优化后的代码（如果修改了代码）
    size_t code_length;                   ///< 优化后代码长度
    size_t original_size;                 ///< 原始大小（字节/指令数）
    size_t optimized_size;                ///< 优化后大小（字节/指令数）
    double improvement_ratio;             ///< 改进比例（0.0-1.0）
    Stru_DynamicArray_t* statistics;      ///< 详细统计信息
    Stru_DynamicArray_t* warnings;        ///< 警告信息
    Stru_DynamicArray_t* errors;          ///< 错误信息
} Stru_OptimizationResult_t;
```

## 使用示例

### 基本使用

```c
#include <stdio.h>
#include <stdlib.h>
#include "src/core/codegen/optimizers/basic_optimizer/CN_basic_optimizer.h"

int main(void)
{
    // 创建基础优化器插件
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
    if (!optimizer) {
        fprintf(stderr, "创建优化器插件失败\n");
        return EXIT_FAILURE;
    }
    
    // 获取插件信息
    const Stru_OptimizerPluginInfo_t* info = optimizer->get_info(optimizer);
    if (info) {
        printf("优化器插件: %s\n", info->name);
        printf("版本: %s\n", info->version);
        printf("描述: %s\n", info->description);
    }
    
    // 创建优化上下文
    Stru_OptimizationContext_t* context = F_create_optimization_context();
    if (!context) {
        fprintf(stderr, "创建优化上下文失败\n");
        optimizer->destroy(optimizer);
        return EXIT_FAILURE;
    }
    
    context->ast = ast_root; // 假设ast_root是有效的AST根节点
    context->level = Eum_OPT_LEVEL_O2;
    
    // 初始化优化器
    if (!optimizer->initialize(optimizer, context)) {
        fprintf(stderr, "初始化优化器失败\n");
        F_destroy_optimization_context(context);
        optimizer->destroy(optimizer);
        return EXIT_FAILURE;
    }
    
    // 应用常量折叠优化
    Stru_OptimizationResult_t* result = optimizer->optimize(
        optimizer, context, Eum_OPTIMIZE_CONSTANT_FOLDING);
    
    // 检查优化结果
    if (result && result->success) {
        printf("常量折叠优化成功\n");
        printf("改进比例: %.2f%%\n", result->improvement_ratio * 100);
        
        // 生成优化报告
        const char* report = optimizer->generate_report(optimizer, result);
        if (report) {
            printf("优化报告:\n%s\n", report);
        }
    } else {
        fprintf(stderr, "常量折叠优化失败\n");
    }
    
    if (result) {
        F_destroy_optimization_result(result);
    }
    F_destroy_optimization_context(context);
    optimizer->destroy(optimizer);
    
    return EXIT_SUCCESS;
}
```

### 批量优化示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "src/core/codegen/optimizers/basic_optimizer/CN_basic_optimizer.h"

int main(void)
{
    // 创建基础优化器插件
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
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
    
    // 定义要应用的优化类型
    Eum_OptimizationType optimizations[] = {
        Eum_OPTIMIZE_CONSTANT_FOLDING,
        Eum_OPTIMIZE_DEAD_CODE_ELIM,
        Eum_OPTIMIZE_COMMON_SUBEXPR
    };
    size_t optimization_count = sizeof(optimizations) / sizeof(optimizations[0]);
    
    // 应用批量优化
    Stru_OptimizationResult_t* result = optimizer->optimize_batch(
        optimizer, context, optimizations, optimization_count);
    
    // 检查优化结果
    if (result && result->success) {
        printf("批量优化成功\n");
        printf("改进比例: %.2f%%\n", result->improvement_ratio * 100);
        
        // 生成优化报告
        const char* report = optimizer->generate_report(optimizer, result);
        if (report) {
            printf("优化报告:\n%s\n", report);
        }
    } else {
        fprintf(stderr, "批量优化失败\n");
    }
    
    // 清理资源
    if (result) {
        F_destroy_optimization_result(result);
    }
    F_destroy_optimization_context(context);
    optimizer->destroy(optimizer);
    
    return EXIT_SUCCESS;
}
```

### 优化级别示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "src/core/codegen/optimizers/basic_optimizer/CN_basic_optimizer.h"

int main(void)
{
    // 创建基础优化器插件
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
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
    
    // 初始化优化器
    if (!optimizer->initialize(optimizer, context)) {
        fprintf(stderr, "初始化优化器失败\n");
        F_destroy_optimization_context(context);
        optimizer->destroy(optimizer);
        return EXIT_FAILURE;
    }
    
    // 应用O2级别的优化
    Stru_OptimizationResult_t* result = optimizer->optimize_level(
        optimizer, context, Eum_OPT_LEVEL_O2);
    
    // 检查优化结果
    if (result && result->success) {
        printf("O2级别优化成功\n");
        printf("改进比例: %.2f%%\n", result->improvement_ratio * 100);
        
        // 生成优化报告
        const char* report = optimizer->generate_report(optimizer, result);
        if (report) {
            printf("优化报告:\n%s\n", report);
        }
    } else {
        fprintf(stderr, "O2级别优化失败\n");
    }
    
    // 清理资源
    if (result) {
        F_destroy_optimization_result(result);
    }
    F_destroy_optimization_context(context);
    optimizer->destroy(optimizer);
    
    return EXIT_SUCCESS;
}
```

## 错误处理

优化器模块使用统一的错误处理机制：

### 1. 错误收集
- 所有错误被收集到错误数组中
- 错误按严重程度分类（错误、警告、信息）
- 尽可能从错误中恢复并继续

### 2. 错误类型
- **初始化错误**: 优化器初始化失败
- **分析错误**: 代码分析过程中出错
- **优化错误**: 优化算法执行失败
- **验证错误**: 优化结果验证失败
- **资源错误**: 内存分配失败等

### 3. 错误报告
- 提供详细的错误信息和位置
- 错误信息包含上下文信息
- 支持多语言错误消息

## 性能考虑

### 1. 优化效率
- 常量折叠: O(n)时间复杂度，n为AST节点数
- 死代码消除: O(n + e)时间复杂度，n为节点数，e为控制流边数
- 公共子表达式消除: O(n²)最坏情况，通常为O(n log n)

### 2. 内存使用
- 控制流图构建需要额外内存
- 优化上下文保存中间结果
- 统计信息收集增加内存开销

### 3. 优化顺序
建议的优化顺序：
1. 常量折叠（简化表达式）
2. 死代码消除（移除无用代码）
3. 公共子表达式消除（减少重复计算）
4. 强度削减（优化运算）
5. 窥孔优化（局部优化）

## 限制和已知问题

### 1. 当前限制
- 不支持循环优化（如循环展开、循环不变代码外提）
- 不支持函数内联优化
- 不支持向量化和并行化优化
- 某些高级优化算法尚未实现

### 2. 已知问题
- 复杂控制流图的死代码消除可能不够精确
- 常量折叠的类型转换可能不够完善
- 优化验证机制需要加强

## 版本历史

### 版本 1.0.0 (2026-01-10)
- 初始版本发布
- 实现常量折叠算法
- 实现死代码消除算法
- 提供基础优化器插件接口
- 支持批量优化和级别优化

## 未来计划

### 短期计划
- 实现公共子表达式消除算法
- 实现强度削减算法
- 实现窥孔优化算法
- 改进错误处理和报告机制

### 中期计划
- 添加循环优化支持
- 实现函数内联优化
- 添加优化验证工具
- 支持增量优化

### 长期计划
- 实现向量化优化
- 实现并行化优化
- 添加机器学习驱动的优化
- 支持跨平台优化

## 许可证

MIT许可证

## 维护者

CN_Language架构委员会
优化器模块开发工作组

## 联系方式

- 项目主页: https://github.com/ChenChao-HuaiAn/CN_Language
- 问题跟踪: https://github.com/ChenChao-HuaiAn/CN_Language/issues
- 文档: https://github.com/ChenChao-HuaiAn/CN_Language/docs
