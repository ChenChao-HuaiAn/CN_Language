# 常量折叠和表达式简化模块

## 概述

常量折叠和表达式简化模块是CN_Language编译器语义分析阶段的重要组成部分，负责在编译时优化AST（抽象语法树）。该模块通过计算常量表达式、应用代数恒等式和逻辑简化规则，减少运行时计算开销，提高程序执行效率。

## 功能特性

### 1. 常量折叠
- **算术运算折叠**：计算常量算术表达式（+、-、*、/、%等）
- **逻辑运算折叠**：计算常量逻辑表达式（&&、||、!等）
- **比较运算折叠**：计算常量比较表达式（==、!=、<、>、<=、>=等）
- **位运算折叠**：计算常量位运算表达式（&、|、^、~、<<、>>等）
- **条件表达式折叠**：计算常量条件表达式（?:）

### 2. 表达式简化
- **代数恒等式简化**：应用代数恒等式（x+0 → x, x*1 → x, x*0 → 0等）
- **逻辑表达式简化**：应用逻辑简化规则（true && x → x, false || x → x等）
- **冗余操作消除**：消除冗余操作（!!x → x, (x) → x等）
- **常量传播**：将常量值传播到使用处
- **死代码消除**：移除不可达的代码分支

## 架构设计

### 模块结构
```
constant_folding/
├── CN_constant_folding.h      # 主头文件，定义所有接口
├── CN_constant_folding.c      # 主实现文件
├── README.md                  # 本文档
└── (未来扩展子模块)
```

### 接口设计

#### 1. 常量折叠器接口 (`Stru_ConstantFoldingInterface_t`)
- `initialize()`: 初始化常量折叠器
- `fold_constant_expression()`: 折叠常量表达式
- `is_constant_expression()`: 检查表达式是否为常量
- `try_get_constant_value()`: 尝试获取常量表达式的值
- `fold_binary_expression()`: 折叠二元运算表达式
- `fold_unary_expression()`: 折叠一元运算表达式
- `fold_conditional_expression()`: 折叠条件表达式
- `fold_call_expression()`: 折叠函数调用表达式
- `set_aggressive_folding()`: 设置是否启用激进折叠
- `reset()`: 重置常量折叠器状态
- `destroy()`: 销毁常量折叠器

#### 2. 表达式简化器接口 (`Stru_ExpressionSimplifierInterface_t`)
- `initialize()`: 初始化表达式简化器
- `simplify_expression()`: 简化表达式
- `apply_algebraic_identities()`: 应用代数恒等式简化
- `apply_logical_simplifications()`: 应用逻辑简化规则
- `eliminate_redundant_operations()`: 消除冗余操作
- `propagate_constants()`: 执行常量传播
- `eliminate_dead_code()`: 消除死代码
- `set_simplification_level()`: 设置简化级别
- `reset()`: 重置表达式简化器状态
- `destroy()`: 销毁表达式简化器

#### 3. 常量折叠管理器接口 (`Stru_ConstantFoldingManagerInterface_t`)
- `initialize()`: 初始化管理器
- `optimize_ast()`: 优化AST节点
- `get_constant_folder()`: 获取常量折叠器接口
- `get_expression_simplifier()`: 获取表达式简化器接口
- `set_optimization_options()`: 设置优化选项
- `reset()`: 重置管理器状态
- `destroy()`: 销毁管理器

## 使用示例

### 基本使用
```c
#include "CN_semantic_interface.h"

// 创建类型系统
Stru_TypeSystem_t* type_system = F_create_type_system();

// 创建常量折叠管理器
Stru_ConstantFoldingManagerInterface_t* manager = 
    F_create_constant_folding_manager_interface();

// 初始化管理器
if (manager->initialize(manager, type_system)) {
    // 设置优化选项
    manager->set_optimization_options(manager, true, true, 1);
    
    // 优化AST节点
    Stru_AstNodeInterface_t* optimized_ast = 
        manager->optimize_ast(manager, original_ast);
    
    // 使用优化后的AST...
}

// 清理资源
F_destroy_constant_folding_manager_interface(manager);
F_destroy_type_system(type_system);
```

### 单独使用常量折叠器
```c
#include "CN_semantic/constant_folding/CN_constant_folding.h"

// 创建常量折叠器
Stru_ConstantFoldingInterface_t* folder = F_create_constant_folding_interface();

// 初始化
if (folder->initialize(folder, type_system)) {
    // 折叠常量表达式
    Stru_AstNodeInterface_t* folded_ast = 
        folder->fold_constant_expression(folder, original_ast);
    
    // 检查表达式是否为常量
    bool is_constant = folder->is_constant_expression(folder, folded_ast);
}

// 清理资源
F_destroy_constant_folding_interface(folder);
```

## 实现细节

### 常量折叠算法
1. **递归遍历AST**：深度优先遍历AST节点
2. **常量识别**：识别常量表达式（字面量、常量变量等）
3. **表达式计算**：根据运算符类型计算常量表达式
4. **结果替换**：用计算结果替换原表达式节点

### 表达式简化规则
1. **代数恒等式**：
   - `x + 0 → x`, `0 + x → x`
   - `x * 1 → x`, `1 * x → x`
   - `x * 0 → 0`, `0 * x → 0`
   - `x - 0 → x`, `x - x → 0`
   - `x / 1 → x`, `x / x → 1` (x ≠ 0)

2. **逻辑简化**：
   - `true && x → x`, `x && true → x`
   - `false && x → false`, `x && false → false`
   - `true || x → true`, `x || true → true`
   - `false || x → x`, `x || false → x`
   - `!true → false`, `!false → true`
   - `!!x → x`

3. **冗余操作消除**：
   - `(x) → x`（多余的括号）
   - `x == true → x`, `x == false → !x`
   - `x != true → !x`, `x != false → x`

## 配置选项

### 折叠选项
- **激进折叠**：启用可能影响浮点数精度的折叠
- **递归深度限制**：防止无限递归（默认100层）

### 简化级别
- **级别0（基本）**：只应用最安全的简化规则
- **级别1（中等）**：应用大多数简化规则（默认）
- **级别2（激进）**：应用所有简化规则，包括可能改变语义的规则

## 依赖关系

### 内部依赖
- `CN_type_system.h`：类型系统定义
- `CN_ast_interface.h`：AST接口定义

### 外部依赖
- 标准C库：`stdlib.h`, `string.h`, `math.h`

## 测试策略

### 单元测试
- 测试每个常量折叠函数
- 测试每个表达式简化规则
- 测试边界条件和错误情况

### 集成测试
- 测试与语义分析器的集成
- 测试完整的AST优化流程
- 测试性能影响

### 回归测试
- 确保优化不会改变程序语义
- 测试复杂表达式的正确性

## 性能考虑

### 时间复杂度
- 常量折叠：O(n)，其中n是AST节点数
- 表达式简化：O(n)，最坏情况O(n²)（需要多次遍历）

### 空间复杂度
- 常量缓存：O(k)，其中k是常量表达式数
- 递归栈：O(d)，其中d是AST深度

### 优化建议
1. 启用常量缓存减少重复计算
2. 设置适当的递归深度限制
3. 根据需求选择简化级别

## 扩展性

### 添加新的折叠规则
1. 在`CN_constant_folding.c`中实现新的折叠函数
2. 更新接口函数指针
3. 添加相应的测试用例

### 添加新的简化规则
1. 在相应的简化函数中添加新规则
2. 确保规则不会改变程序语义
3. 添加测试验证规则正确性

## 注意事项

1. **浮点数精度**：激进折叠可能影响浮点数计算精度
2. **副作用**：避免折叠有副作用的表达式
3. **类型安全**：确保折叠操作保持类型安全
4. **错误处理**：正确处理除零等运行时错误

## 版本历史

### v1.0.0 (2026-01-10)
- 初始版本
- 实现基本常量折叠框架
- 实现基本表达式简化框架
- 提供完整的接口定义

## 相关文档

- [语义分析器架构设计](../docs/architecture/语义分析器实现状态检查清单.md)
- [类型系统文档](../type_checker/README.md)
- [AST接口文档](../../ast/README.md)

## 维护者

CN_Language架构委员会
