# 详细的错误报告和恢复机制模块

## 概述

详细的错误报告和恢复机制模块是CN_Language编译器语义分析层的重要组成部分，负责提供详细的错误信息、错误上下文跟踪、错误恢复策略和错误报告格式化功能。本模块遵循SOLID设计原则和分层架构，确保代码的可维护性、可扩展性和可测试性。

## 功能特性

### 1. 详细的错误信息
- **错误类型分类**：支持多种错误类型（语法错误、语义错误、类型错误等）
- **位置信息**：精确的错误位置（文件、行号、列号）
- **上下文信息**：错误发生时的源代码片段、周围上下文
- **修复建议**：提供具体的修复建议和相关文档链接
- **调用栈跟踪**：可选的支持调用栈跟踪，帮助调试复杂错误

### 2. 错误上下文跟踪
- **编译状态记录**：记录错误发生时的编译阶段和处理过程
- **符号表状态快照**：捕获错误发生时的符号表状态
- **类型系统状态**：记录类型系统的当前状态
- **性能指标**：记录处理时间、内存使用等性能指标
- **环境信息**：记录平台、编译器版本、语言版本等信息

### 3. 错误恢复策略
- **跳过策略**：跳过错误继续处理后续代码
- **修复策略**：尝试自动修复错误
- **回滚策略**：回滚到安全状态后继续处理
- **中止策略**：中止编译过程
- **重试策略**：重试失败的操作

### 4. 错误报告格式化
- **文本格式**：人类可读的纯文本格式
- **JSON格式**：机器可读的JSON格式，便于工具集成
- **XML格式**：结构化的XML格式
- **HTML格式**：美观的HTML格式，适合网页展示
- **Markdown格式**：适合文档和报告的Markdown格式

### 5. 错误统计和分析
- **错误频率统计**：统计各类错误的出现频率
- **错误分布分析**：分析错误在文件、模块、严重性等方面的分布
- **错误趋势分析**：分析错误随时间的变化趋势
- **性能影响评估**：评估错误对编译性能的影响

## 架构设计

### 模块结构
```
src/core/semantic/error_recovery/
├── CN_error_recovery.h          # 头文件：接口定义和数据结构
├── CN_error_recovery.c          # 实现文件：接口实现
├── README.md                    # 本文档
└── (未来扩展)
    ├── detailed_reporter/       # 详细的错误报告器实现
    ├── recovery_strategies/     # 错误恢复策略实现
    ├── context_tracking/        # 错误上下文跟踪实现
    └── statistics/              # 错误统计和分析实现
```

### 接口设计
本模块采用抽象接口模式，定义了4个核心接口：

1. **详细的错误报告器接口** (`Stru_DetailedErrorReporterInterface_t`)
   - 负责收集、管理和报告详细的错误信息
   - 支持错误格式化、导出和查询

2. **错误恢复接口** (`Stru_ErrorRecoveryInterface_t`)
   - 负责实现错误恢复策略
   - 支持策略注册、匹配和执行

3. **错误上下文接口** (`Stru_ErrorContextInterface_t`)
   - 负责捕获和管理错误发生时的上下文信息
   - 支持状态快照和上下文恢复

4. **错误统计接口** (`Stru_ErrorStatisticsInterface_t`)
   - 负责统计和分析错误信息
   - 支持频率统计、分布分析和趋势分析

### 依赖关系
- **基础设施层**：依赖标准C库（stdlib.h, string.h, stdio.h, time.h）
- **核心层内部**：无其他模块依赖，保持模块独立性
- **应用层**：通过接口提供服务，不直接依赖具体实现

## 使用示例

### 基本使用流程

```c
#include "CN_error_recovery.h"

// 1. 创建接口实例
Stru_DetailedErrorReporterInterface_t* reporter = 
    F_create_detailed_error_reporter_interface();
Stru_ErrorRecoveryInterface_t* recovery = 
    F_create_error_recovery_interface();
Stru_ErrorContextInterface_t* context = 
    F_create_error_context_interface();
Stru_ErrorStatisticsInterface_t* statistics = 
    F_create_error_statistics_interface();

// 2. 初始化接口
reporter->initialize(reporter, 100, true);  // 最多100个错误，启用调用栈跟踪
recovery->initialize(recovery, Eum_RECOVERY_STRATEGY_SKIP);
context->initialize(context, true);         // 启用状态快照
statistics->initialize(statistics);

// 3. 捕获错误上下文
Stru_ErrorContext_t error_context;
context->capture_context(context, "语义分析", "类型检查", &error_context);

// 4. 创建错误信息
Stru_DetailedError_t error = {
    .error_code = 1001,
    .error_type = "类型错误",
    .error_message = "变量类型不匹配",
    .severity = Eum_ERROR_SEVERITY_ERROR,
    .line = 42,
    .column = 15,
    .file_name = "test.cn",
    .function_name = "main",
    .module_name = "测试模块",
    .suggested_fix = "检查变量类型声明",
    .timestamp = time(NULL)
};

// 5. 报告错误
reporter->report_detailed_error(reporter, &error, &error_context);

// 6. 执行错误恢复
void* recovery_result = NULL;
bool recovery_success = recovery->perform_recovery(recovery, &error, &error_context, &recovery_result);

// 7. 统计错误
statistics->add_error(statistics, &error, &error_context);

// 8. 导出错误报告
reporter->export_all_errors(reporter, Eum_ERROR_FORMAT_TEXT, "errors.txt");

// 9. 清理资源
F_destroy_detailed_error_reporter_interface(reporter);
F_destroy_error_recovery_interface(recovery);
F_destroy_error_context_interface(context);
F_destroy_error_statistics_interface(statistics);
```

### 错误恢复策略配置

```c
// 注册自定义恢复策略
Stru_ErrorRecoveryStrategy_t custom_strategy = {
    .strategy = Eum_RECOVERY_STRATEGY_REPAIR,
    .strategy_name = "类型修复策略",
    .description = "自动修复类型不匹配错误",
    .max_retries = 3,
    .retry_delay_ms = 100,
    .allow_partial_recovery = true,
    .recovery_action = custom_type_repair_action,
    .recovery_context = NULL
};

recovery->register_recovery_strategy(recovery, "类型错误", &custom_strategy);
```

## 配置选项

### 错误报告器配置
- **最大错误数量**：限制收集的错误数量，防止内存溢出
- **调用栈跟踪**：启用或禁用调用栈跟踪功能
- **输出格式**：设置默认的错误报告格式

### 错误恢复器配置
- **默认恢复策略**：设置未匹配错误类型的默认恢复策略
- **策略匹配规则**：配置错误类型与恢复策略的匹配规则
- **恢复参数**：设置重试次数、延迟时间等参数

### 上下文管理器配置
- **状态快照**：启用或禁用状态快照功能
- **快照频率**：设置状态快照的频率和粒度
- **内存限制**：限制状态快照的内存使用

### 统计器配置
- **统计周期**：设置统计数据的收集周期
- **数据保留**：配置历史数据的保留策略
- **输出格式**：设置统计报告的格式

## 实现细节

### 内存管理
- 使用统一的内存分配接口（未来将集成到基础设施层）
- 实现引用计数和自动清理机制
- 提供内存使用统计和泄漏检测

### 错误处理
- 所有接口函数都返回布尔值表示成功/失败
- 提供详细的错误码和错误信息
- 支持错误链和嵌套错误

### 性能优化
- 使用对象池减少内存分配开销
- 实现延迟加载和缓存机制
- 提供性能监控和调优接口

### 线程安全
- 所有接口都是线程安全的
- 使用读写锁保护共享数据
- 提供原子操作和无锁数据结构

## 测试策略

### 单元测试
- 测试每个接口函数的正确性
- 测试错误情况的处理
- 测试内存管理和资源清理

### 集成测试
- 测试接口之间的协作
- 测试错误恢复流程
- 测试性能指标收集

### 端到端测试
- 测试完整的错误处理流程
- 测试大规模错误场景
- 测试性能和稳定性

## 扩展指南

### 添加新的错误类型
1. 在错误枚举中添加新的错误代码
2. 实现相应的错误处理逻辑
3. 更新错误类型映射表

### 添加新的恢复策略
1. 定义新的恢复策略枚举值
2. 实现恢复策略函数
3. 注册策略到恢复器

### 添加新的输出格式
1. 定义新的输出格式枚举值
2. 实现格式化函数
3. 集成到错误报告器

### 自定义上下文信息
1. 扩展错误上下文结构体
2. 实现上下文捕获逻辑
3. 更新上下文管理器

## 性能考虑

### 内存使用
- 错误信息结构体设计为紧凑格式
- 使用字符串池减少重复字符串的内存占用
- 实现内存使用限制和清理策略

### 处理速度
- 使用哈希表加速错误类型查找
- 实现批量处理和并行处理
- 优化格式化函数的性能

### 可扩展性
- 接口设计支持插件式扩展
- 配置系统支持运行时调整
- 提供性能监控和调优接口

## 兼容性

### 版本兼容性
- 保持接口的向后兼容性
- 使用版本号管理接口变更
- 提供迁移工具和文档

### 平台兼容性
- 支持Windows、Linux、macOS等主流平台
- 使用平台无关的API和数据类型
- 提供平台特定的优化

### 编译器兼容性
- 支持GCC、Clang、MSVC等主流编译器
- 使用标准的C语言特性
- 提供编译器特定的工作区

## 维护指南

### 代码规范
- 遵循项目的编码标准和命名规范
- 使用Doxygen格式的注释
- 保持函数不超过50行，文件不超过500行

### 文档更新
- 更新接口文档和API文档
- 维护使用示例和测试用例
- 记录架构决策和设计理由

### 问题排查
- 提供详细的日志和调试信息
- 实现错误重现和诊断工具
- 建立问题跟踪和解决流程

## 参考资源

### 相关文档
- [CN_Language架构设计文档](../docs/architecture/)
- [语义分析器API文档](../docs/api/core/semantic/)
- [错误处理规范](../docs/specifications/错误处理规范.md)

### 测试文件
- [单元测试](../tests/core/semantic/error_recovery/)
- [集成测试](../tests/integration/error_recovery/)
- [性能测试](../tests/performance/error_recovery/)

### 示例代码
- [基本使用示例](../examples/error_recovery/basic_usage.cn)
- [高级配置示例](../examples/error_recovery/advanced_config.cn)
- [自定义扩展示例](../examples/error_recovery/custom_extension.cn)

## 版本历史

### v1.0.0 (2026-01-10)
- 初始版本发布
- 实现基本的错误报告和恢复功能
- 提供4个核心接口和工厂函数
- 支持多种错误输出格式
- 实现错误统计和分析功能

### 未来计划
- 集成到基础设施层的内存管理
- 添加更多的错误恢复策略
- 实现分布式错误跟踪
- 提供可视化错误报告工具
- 支持机器学习驱动的错误预测

## 贡献指南

欢迎贡献代码、报告问题或提出改进建议。请遵循以下步骤：

1. 阅读项目贡献指南
2. 创建功能分支
3. 编写测试用例
4. 提交代码变更
5. 创建拉取请求

## 许可证

本项目采用MIT许可证。详见LICENSE文件。

## 联系方式

如有问题或建议，请联系：
- 项目维护者：CN_Language架构委员会
- 问题跟踪：GitHub Issues
- 讨论论坛：项目Wiki
