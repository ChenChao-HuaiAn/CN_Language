# 基础优化器模块

## 概述

基础优化器模块是CN_Language编译器的核心优化组件，提供了一系列基本的代码优化算法。该模块遵循项目的分层架构和SOLID设计原则，实现了可扩展的优化器插件接口。

## 功能特性

### 支持的优化类型

1. **常量折叠** - 在编译时计算常量表达式
2. **死代码消除** - 移除不会执行的代码
3. **公共子表达式消除** - 消除重复的表达式计算
4. **强度削减** - 用更高效的运算替换昂贵的运算
5. **窥孔优化** - 对生成的代码进行局部优化

### 优化级别

- **O1** - 基本优化（常量折叠、死代码消除）
- **O2** - 标准优化（包含O1，加上公共子表达式消除、强度削减）
- **O3** - 激进优化（包含O2，加上窥孔优化）

## 架构设计

### 模块结构

```
basic_optimizer/
├── CN_basic_optimizer.c      # 基础优化器主实现
├── CN_basic_optimizer.h      # 基础优化器头文件
├── CN_constant_folding.c     # 常量折叠算法实现
├── CN_constant_folding.h     # 常量折叠算法头文件
├── CN_dead_code_elimination.c # 死代码消除算法实现
├── CN_dead_code_elimination.h # 死代码消除算法头文件
├── CN_common_subexpr.c       # 公共子表达式消除算法实现
├── CN_common_subexpr.h       # 公共子表达式消除算法头文件
├── CN_strength_reduction.c   # 强度削减算法实现
├── CN_strength_reduction.h   # 强度削减算法头文件
├── CN_peephole_optimization.c # 窥孔优化算法实现
├── CN_peephole_optimization.h # 窥孔优化算法头文件
└── README.md                 # 本文件
```

### 接口设计

基础优化器实现了`Stru_OptimizerPluginInterface_t`接口，提供以下核心功能：

1. **插件管理** - 初始化、配置、销毁
2. **代码分析** - 分析代码以确定优化机会
3. **优化应用** - 应用具体的优化算法
4. **结果验证** - 验证优化的正确性
5. **统计报告** - 生成优化统计信息

## 使用示例

### 创建和使用基础优化器

```c
#include "CN_optimizer_interface.h"
#include "CN_basic_optimizer.h"

// 创建基础优化器实例
Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();

// 创建优化上下文
Stru_OptimizationContext_t* context = F_create_optimization_context();
context->ast = ast;  // 设置AST
context->level = Eum_OPT_LEVEL_O2;  // 设置优化级别

// 初始化优化器
if (optimizer->initialize(optimizer, context)) {
    // 应用优化
    Stru_OptimizationResult_t* result = optimizer->optimize_level(optimizer, context, Eum_OPT_LEVEL_O2);
    
    if (result && result->success) {
        printf("优化成功，改进比例: %.1f%%\n", result->improvement_ratio * 100);
    }
    
    // 清理结果
    if (result) {
        F_destroy_optimization_result(result);
    }
}

// 清理资源
F_destroy_optimization_context(context);
optimizer->destroy(optimizer);
```

### 应用特定优化

```c
// 应用常量折叠
Stru_OptimizationResult_t* result = optimizer->optimize(
    optimizer, 
    context, 
    Eum_OPTIMIZE_CONSTANT_FOLDING
);

// 批量应用优化
Eum_OptimizationType optimizations[] = {
    Eum_OPTIMIZE_CONSTANT_FOLDING,
    Eum_OPTIMIZE_DEAD_CODE_ELIM
};
Stru_OptimizationResult_t* batch_result = optimizer->optimize_batch(
    optimizer,
    context,
    optimizations,
    2
);
```

## 算法实现

### 常量折叠算法

常量折叠算法在编译时计算常量表达式，例如：
- `3 + 5` → `8`
- `2 * (3 + 4)` → `14`
- `true && false` → `false`

**实现要点**：
1. 递归遍历AST，识别常量表达式
2. 评估常量表达式的值
3. 用常量值替换表达式节点
4. 处理类型转换和溢出检查

### 死代码消除算法

死代码消除算法移除不会执行的代码，包括：
- 不可达的条件分支
- 死赋值（赋值后从未使用）
- 未使用的变量声明

**实现要点**：
1. 构建控制流图（CFG）
2. 分析代码可达性
3. 分析活跃变量
4. 安全地移除死代码

### 公共子表达式消除

公共子表达式消除算法识别并消除重复的表达式计算。

**实现要点**：
1. 识别具有相同操作数和操作符的表达式
2. 计算表达式的哈希值用于快速比较
3. 用临时变量替换重复的表达式
4. 确保语义等价性

### 强度削减

强度削减算法用更高效的运算替换昂贵的运算，例如：
- `x * 2` → `x << 1`
- `x / 2` → `x >> 1`
- `x * 0` → `0`

**实现要点**：
1. 识别可优化的运算模式
2. 验证优化安全性
3. 应用强度削减转换

### 窥孔优化

窥孔优化算法对生成的代码进行局部优化，例如：
- 移除冗余的加载/存储
- 简化控制流
- 优化跳转指令

**实现要点**：
1. 定义优化模式（peephole patterns）
2. 扫描代码窗口
3. 应用匹配的优化
4. 迭代优化直到收敛

## 性能考虑

### 时间复杂度

- **常量折叠**: O(n)，其中n是AST节点数
- **死代码消除**: O(n + e)，其中n是节点数，e是控制流边数
- **公共子表达式消除**: O(n log n)，需要表达式哈希和比较
- **强度削减**: O(n)，线性扫描
- **窥孔优化**: O(kn)，其中k是迭代次数

### 空间复杂度

- **常量折叠**: O(h)，其中h是AST深度（递归栈）
- **死代码消除**: O(n + e)，需要存储控制流图
- **公共子表达式消除**: O(n)，需要表达式哈希表
- **强度削减**: O(1)，常量空间
- **窥孔优化**: O(w)，其中w是窥孔窗口大小

## 测试策略

### 单元测试

每个优化算法都有对应的单元测试，验证：
1. 基本功能正确性
2. 边界条件处理
3. 错误情况处理
4. 性能基准

### 集成测试

测试优化器与其他组件的集成：
1. 与词法分析器/语法分析器的集成
2. 与代码生成器的集成
3. 多轮优化的正确性

### 端到端测试

完整的编译流程测试：
1. 源代码 → 优化 → 目标代码
2. 优化前后的语义等价性
3. 性能改进验证

## 配置选项

基础优化器支持以下配置选项：

```c
// 启用详细输出
optimizer->configure(optimizer, "verbose", "true");

// 设置最大迭代次数
optimizer->configure(optimizer, "max_iterations", "10");

// 启用激进优化
optimizer->configure(optimizer, "aggressive", "true");
```

## 错误处理

优化器实现了完善的错误处理机制：

1. **输入验证** - 验证AST和上下文的有效性
2. **安全优化** - 确保优化不会改变程序语义
3. **错误恢复** - 在错误情况下优雅恢复
4. **详细报告** - 提供详细的错误和警告信息

## 扩展性

基础优化器设计为可扩展的：

1. **插件架构** - 支持添加新的优化算法
2. **配置驱动** - 通过配置启用/禁用特定优化
3. **接口抽象** - 统一的优化器接口
4. **依赖注入** - 可替换的算法实现

## 维护指南

### 添加新的优化算法

1. 在头文件中定义算法接口
2. 实现算法逻辑
3. 添加单元测试
4. 集成到基础优化器中
5. 更新文档

### 修改现有算法

1. 确保向后兼容性
2. 更新单元测试
3. 验证性能影响
4. 更新文档

## 相关文档

- [优化器接口文档](../../../docs/api/codegen/optimizer_interface.md)
- [常量折叠算法文档](../../../docs/api/codegen/constant_folding.md)
- [死代码消除算法文档](../../../docs/api/codegen/dead_code_elimination.md)
- [公共子表达式消除算法文档](../../../docs/api/codegen/common_subexpression.md)
- [强度削减算法文档](../../../docs/api/codegen/strength_reduction.md)
- [窥孔优化算法文档](../../../docs/api/codegen/peephole_optimization.md)

## 版本历史

- **1.0.0** (2026-01-10): 初始版本，包含常量折叠和死代码消除
- **1.1.0** (2026-01-11): 添加公共子表达式消除、强度削减、窥孔优化
- **1.2.0** (2026-01-12): 性能优化和错误处理改进

## 许可证

本项目采用 MIT 许可证。详见项目根目录的 LICENSE 文件。

## 贡献指南

欢迎贡献代码、报告问题或提出改进建议。请遵循项目的编码标准和开发流程。

---

*最后更新: 2026年1月11日*  
*维护者: CN_Language优化器开发团队*
