# 循环优化器模块

## 概述

循环优化器是CN_Language编译器的一个核心优化插件，专门用于分析和优化程序中的循环结构。该模块实现了多种循环优化算法，旨在提升循环代码的执行性能。

## 功能特性

### 支持的优化类型

1. **循环不变代码外提 (Loop Invariant Code Motion)**
   - 将循环中不变的计算移到循环外部
   - 减少重复计算，提升性能

2. **循环展开 (Loop Unrolling)**
   - 展开循环体，减少循环控制开销
   - 支持可配置的展开因子

3. **循环融合 (Loop Fusion)**
   - 合并相邻的相似循环
   - 减少循环开销，改善数据局部性

4. **循环向量化 (Loop Vectorization)**
   - 将标量操作转换为向量操作
   - 利用SIMD指令提升性能

5. **循环并行化 (Loop Parallelization)**
   - 识别可并行执行的循环
   - 支持多线程执行

6. **内存访问优化 (Memory Access Optimization)**
   - 优化循环中的内存访问模式
   - 改善缓存利用率

### 优化级别支持

- **O1级别**: 基本循环展开
- **O2级别**: 循环展开 + 向量化
- **O3级别**: 所有循环优化

## 架构设计

### 模块结构

```
src/core/codegen/optimizers/loop_optimizer/
├── CN_loop_optimizer.h      # 头文件 - 公共接口定义
├── CN_loop_optimizer.c      # 源文件 - 实现逻辑
└── README.md                # 模块文档
```

### 接口设计

循环优化器实现了标准的优化器插件接口 (`Stru_OptimizerPluginInterface_t`)，包括：

1. **工厂函数**: `F_create_loop_optimizer_interface()`
2. **初始化函数**: `initialize()`
3. **分析函数**: `analyze()`
4. **优化函数**: `optimize()`, `optimize_batch()`, `optimize_level()`
5. **转换函数**: `transform_ast()`, `transform_ir()`
6. **验证函数**: `validate()`, `is_safe()`
7. **配置函数**: `configure()`, `get_config_options()`
8. **资源管理**: `reset()`, `destroy()`

### 数据结构

1. **循环分析信息** (`Stru_LoopAnalysisInfo_t`)
   - 循环ID、类型、嵌套深度
   - 迭代次数、循环变量
   - 依赖关系、数据流信息

2. **循环优化配置** (`Stru_LoopOptimizationConfig_t`)
   - 最大展开因子
   - 最大向量宽度
   - 优化策略选项

3. **优化结果** (`Stru_OptimizationResult_t`)
   - 优化成功状态
   - 性能提升百分比
   - 代码大小变化

## 使用示例

### 基本使用

```c
#include "CN_loop_optimizer.h"

// 创建循环优化器接口
Stru_OptimizerPluginInterface_t* optimizer = F_create_loop_optimizer_interface();
if (!optimizer) {
    // 处理错误
}

// 初始化优化器
Stru_OptimizationContext_t context = {0};
context.ast = ast_node;  // AST节点
if (!optimizer->initialize(optimizer, &context)) {
    // 处理错误
}

// 应用循环展开优化
Stru_OptimizationResult_t* result = optimizer->optimize(
    optimizer, 
    &context, 
    Eum_OPTIMIZE_LOOP_UNROLLING
);

if (result && result->success) {
    printf("优化成功，性能提升: %.2f%%\n", result->performance_improvement * 100);
}

// 清理资源
if (result) free(result);
optimizer->destroy(optimizer);
```

### 批量优化

```c
// 批量应用多个优化
Eum_OptimizationType optimizations[] = {
    Eum_OPTIMIZE_LOOP_UNROLLING,
    Eum_OPTIMIZE_VECTORIZATION,
    Eum_OPTIMIZE_MEMORY_ACCESS
};

Stru_OptimizationResult_t* batch_result = optimizer->optimize_batch(
    optimizer,
    &context,
    optimizations,
    3
);
```

### 按级别优化

```c
// 应用O3级别的所有优化
Stru_OptimizationResult_t* level_result = optimizer->optimize_level(
    optimizer,
    &context,
    Eum_OPT_LEVEL_O3
);
```

## 配置选项

循环优化器支持以下配置选项：

| 配置项 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `max_unroll_factor` | 整数 | 4 | 循环展开的最大因子 |
| `max_vector_width` | 整数 | 4 | 向量化的最大宽度 |
| `enable_aggressive_optimizations` | 布尔 | false | 是否启用激进优化 |
| `preserve_semantics` | 布尔 | true | 是否保持程序语义不变 |
| `enable_profiling` | 布尔 | false | 是否启用性能分析 |
| `cost_threshold` | 浮点数 | 1.5 | 优化的成本阈值 |

### 配置示例

```c
// 配置循环优化器
optimizer->configure(optimizer, "max_unroll_factor", "8");
optimizer->configure(optimizer, "enable_aggressive_optimizations", "true");

// 获取所有配置选项
Stru_DynamicArray_t* options = optimizer->get_config_options(optimizer);
```

## 依赖关系

### 内部依赖
- 基础设施层: 内存管理、动态数组
- 核心层: AST节点、优化器接口

### 外部依赖
- C标准库: `stdio.h`, `stdlib.h`, `string.h`

## 性能考虑

1. **分析阶段**: 循环分析可能消耗较多时间，特别是对于深度嵌套的循环
2. **优化阶段**: 循环展开和向量化可能显著增加代码大小
3. **内存使用**: 需要存储循环分析信息和优化配置

## 限制和注意事项

1. **循环类型支持**: 目前主要支持`for`循环，`while`和`do-while`循环支持有限
2. **嵌套深度**: 深度嵌套循环的优化效果可能受限
3. **副作用**: 激进优化可能改变程序语义，需谨慎使用
4. **平台依赖**: 向量化优化依赖于目标平台的SIMD指令集

## 测试策略

### 单元测试
- 测试每个优化算法的正确性
- 验证优化前后的语义等价性

### 集成测试
- 测试优化器与编译器其他组件的集成
- 验证优化配置的正确应用

### 性能测试
- 测量优化前后的性能提升
- 分析代码大小变化

## 扩展性

循环优化器设计为可扩展的插件架构：

1. **添加新优化算法**: 实现新的优化函数并注册到接口
2. **自定义配置**: 通过配置接口添加新的配置选项
3. **平台特定优化**: 针对不同目标平台实现特定的优化策略

## 版本历史

- **v1.0.0** (2026-01-12): 初始版本，实现基本循环优化功能

## 相关文档

- [API文档](../docs/api/core/codegen/optimizers/loop_optimizer/)
- [优化器接口规范](../../../docs/specifications/优化器插件接口规范.md)
- [循环优化算法设计](../../../docs/architecture/循环优化算法设计.md)

## 贡献指南

1. 遵循项目编码标准和架构规范
2. 每个.c文件不超过500行，每个函数不超过50行
3. 为新增功能编写单元测试
4. 更新相关文档和API文档

## 许可证

MIT许可证
