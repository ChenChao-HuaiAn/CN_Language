# CN_Language 优化模块

## 概述

CN_Language优化模块提供了中间代码表示（IR）的优化功能，包括优化算法实现和优化效果评估。本模块遵循项目架构规范，采用分层设计和接口模式，确保模块间的解耦和可扩展性。

## 模块结构

### 1. 优化器接口 (CN_ir_optimizer.h)
- 定义IR优化器的统一接口
- 支持多种优化类型和级别
- 提供优化配置、统计和结果管理

### 2. 三地址代码优化器 (CN_tac_optimizer)
- 实现三地址代码的优化算法
- 支持常量折叠、死代码消除等基本优化
- 提供优化结果验证和统计

### 3. 优化效果评估器 (CN_optimization_evaluator)
- 评估优化前后的效果
- 计算改进比例和优化质量评分
- 生成优化报告和建议

## 优化类型

### 基本优化
1. **常量折叠 (Constant Folding)**
   - 在编译时计算常量表达式
   - 减少运行时计算开销

2. **死代码消除 (Dead Code Elimination)**
   - 移除不会执行的代码
   - 减少代码大小和内存占用

3. **公共子表达式消除 (Common Subexpression Elimination)**
   - 识别并重用重复的计算结果
   - 减少冗余计算

4. **强度削减 (Strength Reduction)**
   - 用更高效的操作替换昂贵的操作
   - 如用移位代替乘除法

### 高级优化
5. **循环优化 (Loop Optimization)**
   - 循环不变代码外提
   - 循环展开
   - 循环融合

6. **函数内联 (Function Inlining)**
   - 将小函数调用替换为函数体
   - 减少函数调用开销

7. **窥孔优化 (Peephole Optimization)**
   - 局部代码模式匹配和替换
   - 优化特定指令序列

## 优化级别

### O0 - 无优化
- 禁用所有优化
- 保留完整的调试信息

### O1 - 基本优化
- 启用常量折叠和死代码消除
- 编译时间开销小

### O2 - 中级优化
- 包含O1所有优化
- 增加公共子表达式消除和强度削减
- 平衡性能和编译时间

### O3 - 高级优化
- 包含O2所有优化
- 增加循环优化和函数内联
- 最大化性能优化

### Os - 代码大小优化
- 优化以减少代码大小为主
- 适用于嵌入式系统

### Oz - 激进代码大小优化
- 更激进的代码大小优化
- 可能牺牲部分性能

## 优化效果评估

### 评估指标
1. **指令减少比例**
   - 优化后指令数量减少的比例
   - 范围：0.0-1.0

2. **代码大小减少比例**
   - 优化后代码大小减少的比例
   - 范围：0.0-1.0

3. **估计性能改进**
   - 基于优化类型和指标的估计改进
   - 范围：0.0-1.0

4. **内存使用改进**
   - 内存使用减少的比例
   - 范围：0.0-1.0

5. **编译时间开销**
   - 优化增加的编译时间比例
   - 范围：0.0-1.0

6. **优化质量评分**
   - 综合评分（0-100）
   - 基于多个指标的加权计算

### 评估流程
1. **输入验证**
   - 检查原始IR和优化后IR的有效性
   - 验证优化类型和级别

2. **指标计算**
   - 计算各项优化指标
   - 应用优化级别因子

3. **报告生成**
   - 生成详细的评估报告
   - 提供优化建议

4. **结果输出**
   - 返回评估结果结构
   - 支持报告保存到文件

## 使用示例

### 创建优化评估器
```c
#include "CN_optimization_evaluator.h"

// 创建评估器接口
Stru_OptimizationEvaluatorInterface_t* evaluator = 
    F_create_optimization_evaluator_interface();
if (!evaluator) {
    // 错误处理
}
```

### 评估优化效果
```c
// 评估特定优化类型
Stru_OptimizationEvaluationResult_t* result = 
    evaluator->evaluate_optimization(original_ir, optimized_ir, 
                                     Eum_IR_OPT_CONSTANT_FOLDING);

if (result && result->evaluation_successful) {
    // 使用评估结果
    printf("优化质量评分: %.1f\n", result->metrics.optimization_quality_score);
    
    // 生成报告
    char* report = evaluator->generate_evaluation_report(result);
    if (report) {
        printf("%s\n", report);
        cn_free(report);
    }
}

// 销毁结果
if (result) {
    evaluator->destroy_evaluation_result(result);
}
```

### 评估优化级别
```c
// 评估优化级别效果
Stru_OptimizationEvaluationResult_t* level_result = 
    evaluator->evaluate_optimization_level(original_ir, optimized_ir, 
                                           Eum_IR_OPT_LEVEL_O2);
```

### 保存报告到文件
```c
// 保存评估报告
bool saved = evaluator->save_report_to_file(result, "optimization_report.txt");
if (saved) {
    printf("报告已保存到文件\n");
}
```

## 接口设计

### 优化评估器接口
```c
typedef struct Stru_OptimizationEvaluatorInterface_t {
    // 基本信息
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    
    // 效果评估
    Stru_OptimizationEvaluationResult_t* (*evaluate_optimization)(
        void* original_ir,
        void* optimized_ir,
        Eum_IrOptimizationType opt_type);
    
    Stru_OptimizationEvaluationResult_t* (*evaluate_optimization_level)(
        void* original_ir,
        void* optimized_ir,
        Eum_IrOptimizationLevel level);
    
    // 基准测试
    bool (*run_benchmark)(Stru_OptimizationBenchmark_t* benchmark);
    Stru_OptimizationBenchmark_t* (*create_benchmark)(const char* name,
                                                     void* original_ir,
                                                     void* optimized_ir);
    void (*destroy_benchmark)(Stru_OptimizationBenchmark_t* benchmark);
    
    // 报告生成
    char* (*generate_evaluation_report)(const Stru_OptimizationEvaluationResult_t* result);
    bool (*save_report_to_file)(const Stru_OptimizationEvaluationResult_t* result,
                               const char* filename);
    
    // 统计分析
    bool (*analyze_optimization_trends)(Stru_OptimizationEvaluationResult_t** results,
                                       size_t count,
                                       Stru_OptimizationMetrics_t* trends);
    
    // 资源管理
    void (*destroy_evaluation_result)(Stru_OptimizationEvaluationResult_t* result);
    void (*destroy)(void);
    
    // 内部状态
    void* internal_state;
} Stru_OptimizationEvaluatorInterface_t;
```

## 设计原则

### 1. 单一职责原则
- 每个.c文件不超过500行
- 每个函数不超过50行
- 每个模块只负责一个功能领域

### 2. 开闭原则
- 通过抽象接口支持扩展
- 模块对修改封闭，对扩展开放
- 支持新的优化算法和评估指标

### 3. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入管理依赖关系

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活的功能集

## 性能考虑

### 编译时性能
- 优化算法的时间复杂度可控
- 支持渐进式优化
- 可配置的优化深度

### 运行时性能
- 优化后的代码执行效率更高
- 内存使用更合理
- 支持性能基准测试

### 内存使用
- 优化过程内存占用可控
- 支持大代码文件的优化
- 内存泄漏检测和防护

## 测试策略

### 单元测试
- 测试每个优化算法的正确性
- 验证优化效果评估的准确性
- 覆盖边界条件和错误情况

### 集成测试
- 测试优化器与IR系统的集成
- 验证优化流程的完整性
- 测试多模块协作

### 端到端测试
- 完整编译流程测试
- 优化效果验证
- 性能基准测试

## 扩展性

### 添加新的优化算法
1. 在CN_ir_optimizer.h中定义新的优化类型
2. 实现优化算法
3. 注册到优化器接口
4. 更新优化配置和统计

### 添加新的评估指标
1. 扩展Stru_OptimizationMetrics_t结构
2. 实现指标计算函数
3. 更新评估报告生成
4. 修改质量评分计算

### 支持新的IR类型
1. 实现IR特定的优化器
2. 注册到优化器工厂
3. 更新优化效果评估
4. 添加相应的测试

## 相关文档

- [IR优化器接口文档](../docs/api/core/ir/CN_ir_optimizer.md)
- [优化效果评估器API文档](../docs/api/core/ir/optimizations/CN_optimization_evaluator.md)
- [三地址代码优化器文档](../docs/api/core/ir/optimizations/CN_tac_optimizer.md)
- [优化模块设计文档](../../../docs/architecture/优化模块设计.md)

## 版本历史

### v1.0.0 (2026-01-11)
- 初始版本发布
- 支持基本优化算法
- 实现优化效果评估
- 提供完整的API接口

## 许可证

本项目采用MIT许可证。详见LICENSE文件。

## 贡献指南

1. 遵循项目编码规范
2. 添加相应的单元测试
3. 更新相关文档
4. 通过代码审查

## 联系方式

- 项目主页: https://github.com/ChenChao-HuaiAn/CN_Language
- 问题跟踪: https://github.com/ChenChao-HuaiAn/CN_Language/issues
- 讨论区: https://github.com/ChenChao-HuaiAn/CN_Language/discussions
