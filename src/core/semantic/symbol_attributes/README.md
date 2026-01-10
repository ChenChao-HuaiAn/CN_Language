# 符号属性扩展模块

## 概述

符号属性扩展模块为CN_Language项目提供了增强的符号属性管理功能，包括符号可见性、生命周期、访问权限、存储类别等扩展属性，以及符号依赖关系跟踪和重命名支持。

## 功能特性

### 1. 符号属性扩展
- **可见性级别**：私有、保护、公有、内部、外部
- **生命周期**：静态、自动、动态、线程、临时
- **访问权限**：只读、只写、读写、只执行、无访问
- **存储类别**：自动、静态、寄存器、外部、线程局部

### 2. 依赖关系跟踪
- **依赖类型**：使用、定义、引用、别名、继承、实现
- **依赖图分析**：构建和分析符号间的依赖关系
- **循环依赖检测**：自动检测和报告循环依赖
- **依赖链查询**：查询符号的完整依赖链

### 3. 重命名支持
- **安全重命名**：检查重命名操作的安全性
- **影响分析**：分析重命名对其他符号的影响
- **批量重命名**：支持批量符号重命名
- **重命名建议**：提供智能重命名建议

### 4. 符号信息管理
- **扩展符号信息**：包含所有扩展属性的符号信息结构
- **引用计数**：跟踪符号的引用次数
- **别名管理**：支持符号别名
- **时间戳**：记录符号创建和修改时间

## 架构设计

### 模块结构
```
symbol_attributes/
├── CN_symbol_attributes.h      # 头文件（接口定义）
├── CN_symbol_attributes.c      # 实现文件
├── README.md                   # 本文档
└── (未来可能添加测试文件)
```

### 接口设计
模块提供三个主要接口：

1. **符号属性接口** (`Stru_SymbolAttributesInterface_t`)
   - 符号信息创建和销毁
   - 属性设置和获取
   - 依赖关系管理
   - 重命名支持

2. **符号依赖关系接口** (`Stru_SymbolDependencyInterface_t`)
   - 依赖关系图构建
   - 依赖分析
   - 循环依赖检测
   - 重命名影响分析

3. **符号重命名接口** (`Stru_SymbolRenameInterface_t`)
   - 符号重命名
   - 批量重命名
   - 重命名验证
   - 重命名建议

### 数据结构
- `Stru_ExtendedSymbolInfo_t`：扩展的符号信息结构
- `Stru_SymbolDependency_t`：符号依赖关系结构
- `Stru_SymbolReference_t`：符号引用结构

## 使用示例

### 基本使用
```c
#include "CN_symbol_attributes.h"

// 创建接口实例
Stru_SymbolAttributesInterface_t* attr_interface = F_create_symbol_attributes_interface();
Stru_SymbolDependencyInterface_t* dep_interface = F_create_symbol_dependency_interface();
Stru_SymbolRenameInterface_t* rename_interface = F_create_symbol_rename_interface();

// 初始化模块
attr_interface->initialize();
dep_interface->initialize();
rename_interface->initialize();

// 创建符号信息
Stru_ExtendedSymbolInfo_t* symbol = attr_interface->create_symbol_info("my_var", "int");

// 设置属性
attr_interface->set_visibility(symbol, Eum_VISIBILITY_PUBLIC);
attr_interface->set_lifetime(symbol, Eum_LIFETIME_STATIC);
attr_interface->set_access(symbol, Eum_ACCESS_READ_WRITE);

// 添加依赖关系
attr_interface->add_dependency(symbol, "other_var", Eum_DEPENDENCY_USES, 10, 5);

// 重命名符号
rename_interface->rename_symbol("my_var", "new_var", &symbol, 1);

// 获取符号信息字符串
char* info_str = attr_interface->to_string(symbol);
printf("%s\n", info_str);
free(info_str);

// 清理
attr_interface->destroy_symbol_info(symbol);
attr_interface->destroy();
dep_interface->destroy();
rename_interface->destroy();

F_destroy_symbol_attributes_interface(attr_interface);
F_destroy_symbol_dependency_interface(dep_interface);
F_destroy_symbol_rename_interface(rename_interface);
```

### 依赖关系分析
```c
// 构建依赖关系图
Stru_ExtendedSymbolInfo_t* symbols[] = {symbol1, symbol2, symbol3};
dep_interface->create_dependency_graph(symbols, 3);

// 分析依赖关系
char* analysis = dep_interface->analyze_dependencies("my_var");
printf("%s\n", analysis);
free(analysis);

// 检测循环依赖
char* cycles = dep_interface->detect_cycles();
printf("%s\n", cycles);
free(cycles);
```

### 重命名操作
```c
// 验证重命名
bool valid = rename_interface->validate_rename("old_name", "new_name", symbols, 3);

// 获取重命名建议
char* suggestions = rename_interface->get_rename_suggestions("var", "context");
printf("%s\n", suggestions);
free(suggestions);

// 执行重命名并获取报告
char* report = rename_interface->rename_with_report("old_name", "new_name", symbols, 3);
printf("%s\n", report);
free(report);
```

## API文档

详细的API文档请参考：
- `docs/api/core/semantic/symbol_attributes/CN_symbol_attributes_api.md`

## 设计原则

### 单一职责原则
- 每个接口专注于单一功能领域
- 符号属性接口负责属性管理
- 依赖关系接口负责依赖分析
- 重命名接口负责重命名操作

### 开闭原则
- 通过抽象接口支持扩展
- 可以添加新的属性类型而不修改现有代码
- 可以添加新的依赖分析算法

### 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活的功能集

### 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过工厂函数创建接口实例

## 性能考虑

### 内存管理
- 使用动态内存分配存储符号信息
- 提供完整的内存释放接口
- 避免内存泄漏

### 时间复杂度
- 符号查找：O(n)（链表实现）
- 依赖关系分析：O(n²)（简化实现）
- 重命名操作：O(n)

### 空间复杂度
- 符号信息：O(n)
- 依赖关系图：O(n²)（最坏情况）

## 测试策略

### 单元测试
- 测试每个接口函数的基本功能
- 测试错误处理
- 测试边界条件

### 集成测试
- 测试模块间的协作
- 测试完整的符号属性管理流程
- 测试依赖关系分析

### 端到端测试
- 测试在实际编译场景中的应用
- 测试性能表现
- 测试内存使用

## 未来扩展

### 计划功能
1. **符号版本控制**：支持符号版本管理
2. **符号迁移**：支持符号在不同作用域间的迁移
3. **符号合并**：支持符号合并操作
4. **符号冲突检测**：检测符号命名冲突

### 性能优化
1. **哈希表实现**：使用哈希表提高符号查找效率
2. **依赖关系缓存**：缓存依赖关系分析结果
3. **增量更新**：支持增量式依赖关系更新

### 工具集成
1. **IDE插件**：提供符号属性可视化工具
2. **调试器集成**：在调试器中显示符号属性
3. **文档生成**：自动生成符号文档

## 贡献指南

### 代码规范
- 遵循项目编码标准
- 使用统一的命名约定
- 添加充分的注释

### 测试要求
- 为新功能添加单元测试
- 确保测试覆盖率达标
- 更新集成测试

### 文档要求
- 更新API文档
- 更新设计文档
- 更新使用示例

## 许可证

本项目采用MIT许可证。详见LICENSE文件。

## 维护者

- CN_Language架构委员会
- 邮箱：architecture@cn-language.org
- 问题反馈：GitHub Issues
