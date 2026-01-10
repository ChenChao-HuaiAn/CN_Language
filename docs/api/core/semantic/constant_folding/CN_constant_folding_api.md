# CN_Language 常量折叠和表达式简化模块 API 文档

## 概述

常量折叠和表达式简化模块是CN_Language编译器语义分析阶段的重要组成部分，负责在编译时优化AST（抽象语法树）。该模块通过计算常量表达式、应用代数恒等式和逻辑简化规则，减少运行时计算开销，提高程序执行效率。

## 接口定义

### Stru_ConstantFoldingInterface_t

常量折叠器接口，负责在编译时计算常量表达式。

```c
typedef struct Stru_ConstantFoldingInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_ConstantFoldingInterface_t* folder,
                      Stru_TypeSystem_t* type_system);
    
    // 常量折叠
    Stru_AstNodeInterface_t* (*fold_constant_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                        Stru_AstNodeInterface_t* ast_node);
    bool (*is_constant_expression)(Stru_ConstantFoldingInterface_t* folder,
                                  const Stru_AstNodeInterface_t* ast_node);
    bool (*try_get_constant_value)(Stru_ConstantFoldingInterface_t* folder,
                                  const Stru_AstNodeInterface_t* ast_node,
                                  void* value,
                                  Stru_TypeDescriptor_t** type);
    
    // 特定表达式折叠
    Stru_AstNodeInterface_t* (*fold_binary_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                      int operator_type,
                                                      Stru_AstNodeInterface_t* left,
                                                      Stru_AstNodeInterface_t* right);
    Stru_AstNodeInterface_t* (*fold_unary_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                     int operator_type,
                                                     Stru_AstNodeInterface_t* operand);
    Stru_AstNodeInterface_t* (*fold_conditional_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                           Stru_AstNodeInterface_t* condition,
                                                           Stru_AstNodeInterface_t* then_expr,
                                                           Stru_AstNodeInterface_t* else_expr);
    Stru_AstNodeInterface_t* (*fold_call_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                    Stru_AstNodeInterface_t* function,
                                                    Stru_AstNodeInterface_t** arguments,
                                                    size_t argument_count);
    
    // 配置
    void (*set_aggressive_folding)(Stru_ConstantFoldingInterface_t* folder,
                                  bool aggressive);
    
    // 资源管理
    void (*reset)(Stru_ConstantFoldingInterface_t* folder);
    void (*destroy)(Stru_ConstantFoldingInterface_t* folder);
    
    // 私有数据
    void* private_data;
} Stru_ConstantFoldingInterface_t;
```

### Stru_ExpressionSimplifierInterface_t

表达式简化器接口，负责简化表达式结构。

```c
typedef struct Stru_ExpressionSimplifierInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_ExpressionSimplifierInterface_t* simplifier,
                      Stru_TypeSystem_t* type_system);
    
    // 表达式简化
    Stru_AstNodeInterface_t* (*simplify_expression)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                   Stru_AstNodeInterface_t* ast_node);
    
    // 特定简化规则
    Stru_AstNodeInterface_t* (*apply_algebraic_identities)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                          Stru_AstNodeInterface_t* ast_node);
    Stru_AstNodeInterface_t* (*apply_logical_simplifications)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                             Stru_AstNodeInterface_t* ast_node);
    Stru_AstNodeInterface_t* (*eliminate_redundant_operations)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                              Stru_AstNodeInterface_t* ast_node);
    Stru_AstNodeInterface_t* (*propagate_constants)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                   Stru_AstNodeInterface_t* ast_node,
                                                   void* constant_values);
    Stru_AstNodeInterface_t* (*eliminate_dead_code)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                   Stru_AstNodeInterface_t* ast_node);
    
    // 配置
    void (*set_simplification_level)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                    int level);
    
    // 资源管理
    void (*reset)(Stru_ExpressionSimplifierInterface_t* simplifier);
    void (*destroy)(Stru_ExpressionSimplifierInterface_t* simplifier);
    
    // 私有数据
    void* private_data;
} Stru_ExpressionSimplifierInterface_t;
```

### Stru_ConstantFoldingManagerInterface_t

常量折叠和表达式简化管理器接口，整合所有优化功能。

```c
typedef struct Stru_ConstantFoldingManagerInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_ConstantFoldingManagerInterface_t* manager,
                      Stru_TypeSystem_t* type_system);
    
    // AST优化
    Stru_AstNodeInterface_t* (*optimize_ast)(Stru_ConstantFoldingManagerInterface_t* manager,
                                            Stru_AstNodeInterface_t* ast_node);
    
    // 组件访问
    Stru_ConstantFoldingInterface_t* (*get_constant_folder)(Stru_ConstantFoldingManagerInterface_t* manager);
    Stru_ExpressionSimplifierInterface_t* (*get_expression_simplifier)(Stru_ConstantFoldingManagerInterface_t* manager);
    
    // 配置
    void (*set_optimization_options)(Stru_ConstantFoldingManagerInterface_t* manager,
                                    bool enable_folding,
                                    bool enable_simplification,
                                    int simplification_level);
    
    // 资源管理
    void (*reset)(Stru_ConstantFoldingManagerInterface_t* manager);
    void (*destroy)(Stru_ConstantFoldingManagerInterface_t* manager);
    
    // 私有数据
    void* private_data;
} Stru_ConstantFoldingManagerInterface_t;
```

## 辅助数据结构

### Eum_ConstantType

常量类型枚举。

```c
typedef enum Eum_ConstantType {
    Eum_CONSTANT_TYPE_INTEGER,      // 整数常量
    Eum_CONSTANT_TYPE_FLOAT,        // 浮点数常量
    Eum_CONSTANT_TYPE_BOOLEAN,      // 布尔常量
    Eum_CONSTANT_TYPE_STRING,       // 字符串常量
    Eum_CONSTANT_TYPE_NULL,         // 空常量
    Eum_CONSTANT_TYPE_COMPLEX,      // 复合常量
    Eum_CONSTANT_TYPE_UNKNOWN       // 未知常量类型
} Eum_ConstantType;
```

### Stru_ConstantValue_t

常量值结构体，存储常量的值和类型信息。

```c
typedef struct Stru_ConstantValue_t {
    Eum_ConstantType type;          // 常量类型
    union {
        long integer_value;         // 整数值
        double float_value;         // 浮点数值
        bool boolean_value;         // 布尔值
        const char* string_value;   // 字符串值
        void* complex_value;        // 复合值
    } value;
    Stru_TypeDescriptor_t* type_descriptor; // 类型描述符
    bool is_valid;                  // 是否有效
} Stru_ConstantValue_t;
```

### Stru_SimplificationRule_t

简化规则结构体。

```c
typedef struct Stru_SimplificationRule_t {
    const char* pattern;            // 模式描述
    const char* replacement;        // 替换描述
    int priority;                   // 优先级（0-100）
    bool safe;                      // 是否安全（不会改变语义）
    bool (*condition)(Stru_AstNodeInterface_t* node); // 应用条件
} Stru_SimplificationRule_t;
```

## API 参考

### F_create_constant_folding_interface

创建常量折叠器接口实例。

**函数签名：**
```c
Stru_ConstantFoldingInterface_t* F_create_constant_folding_interface(void);
```

**返回值：**
- `Stru_ConstantFoldingInterface_t*`：新创建的常量折叠器接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_ConstantFoldingInterface_t* folder = F_create_constant_folding_interface();
if (folder == NULL) {
    fprintf(stderr, "无法创建常量折叠器\n");
    return 1;
}

// 使用常量折叠器...

folder->destroy(folder);
```

### F_create_expression_simplifier_interface

创建表达式简化器接口实例。

**函数签名：**
```c
Stru_ExpressionSimplifierInterface_t* F_create_expression_simplifier_interface(void);
```

**返回值：**
- `Stru_ExpressionSimplifierInterface_t*`：新创建的表达式简化器接口实例
- `NULL`：创建失败（内存不足）

### F_create_constant_folding_manager_interface

创建常量折叠和表达式简化管理器接口实例。

**函数签名：**
```c
Stru_ConstantFoldingManagerInterface_t* F_create_constant_folding_manager_interface(void);
```

**返回值：**
- `Stru_ConstantFoldingManagerInterface_t*`：新创建的管理器接口实例
- `NULL`：创建失败（内存不足）

### folder->initialize

初始化常量折叠器。

**函数签名：**
```c
bool initialize(Stru_ConstantFoldingInterface_t* folder,
                Stru_TypeSystem_t* type_system);
```

**参数：**
- `folder`：常量折叠器接口指针
- `type_system`：类型系统实例

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

### folder->fold_constant_expression

折叠常量表达式。

**函数签名：**
```c
Stru_AstNodeInterface_t* fold_constant_expression(Stru_ConstantFoldingInterface_t* folder,
                                                  Stru_AstNodeInterface_t* ast_node);
```

**参数：**
- `folder`：常量折叠器接口指针
- `ast_node`：AST表达式节点

**返回值：**
- `Stru_AstNodeInterface_t*`：折叠后的AST节点，如果无法折叠则返回原节点

### folder->is_constant_expression

检查表达式是否为常量。

**函数签名：**
```c
bool is_constant_expression(Stru_ConstantFoldingInterface_t* folder,
                           const Stru_AstNodeInterface_t* ast_node);
```

**参数：**
- `folder`：常量折叠器接口指针
- `ast_node`：AST表达式节点

**返回值：**
- `true`：是常量表达式
- `false`：不是常量表达式

### simplifier->simplify_expression

简化表达式。

**函数签名：**
```c
Stru_AstNodeInterface_t* simplify_expression(Stru_ExpressionSimplifierInterface_t* simplifier,
                                            Stru_AstNodeInterface_t* ast_node);
```

**参数：**
- `simplifier`：表达式简化器接口指针
- `ast_node`：AST表达式节点

**返回值：**
- `Stru_AstNodeInterface_t*`：简化后的AST节点

### simplifier->apply_algebraic_identities

应用代数恒等式简化。

**函数签名：**
```c
Stru_AstNodeInterface_t* apply_algebraic_identities(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                   Stru_AstNodeInterface_t* ast_node);
```

**参数：**
- `simplifier`：表达式简化器接口指针
- `ast_node`：AST表达式节点

**返回值：**
- `Stru_AstNodeInterface_t*`：应用代数恒等式后的AST节点

### manager->optimize_ast

优化AST节点。

**函数签名：**
```c
Stru_AstNodeInterface_t* optimize_ast(Stru_ConstantFoldingManagerInterface_t* manager,
                                     Stru_AstNodeInterface_t* ast_node);
```

**参数：**
- `manager`：管理器接口指针
- `ast_node`：AST节点

**返回值：**
- `Stru_AstNodeInterface_t*`：优化后的AST节点

### manager->set_optimization_options

设置优化选项。

**函数签名：**
```c
void set_optimization_options(Stru_ConstantFoldingManagerInterface_t* manager,
                             bool enable_folding,
                             bool enable_simplification,
                             int simplification_level);
```

**参数：**
- `manager`：管理器接口指针
- `enable_folding`：是否启用常量折叠
- `enable_simplification`：是否启用表达式简化
- `simplification_level`：简化级别（0-基本，1-中等，2-激进）

## 使用示例

### 基本使用模式

```c
#include "CN_semantic_interface.h"

int main() {
    // 创建类型系统
    Stru_TypeSystem_t* type_system = F_create_type_system();
    if (type_system == NULL) {
        return 1;
    }
    
    // 创建常量折叠管理器
    Stru_ConstantFoldingManagerInterface_t* manager = 
        F_create_constant_folding_manager_interface();
    if (manager == NULL) {
        F_destroy_type_system(type_system);
        return 1;
    }
    
    // 初始化管理器
    if (!manager->initialize(manager, type_system)) {
        F_destroy_constant_folding_manager_interface(manager);
        F_destroy_type_system(type_system);
        return 1;
    }
    
    // 设置优化选项
    manager->set_optimization_options(manager, true, true, 1);
    
    // 创建示例AST节点（假设有相关函数）
    Stru_AstNodeInterface_t* original_ast = create_sample_ast();
    
    // 优化AST
    Stru_AstNodeInterface_t* optimized_ast = manager->optimize_ast(manager, original_ast);
    
    // 使用优化后的AST...
    
    // 清理资源
    destroy_ast_node(original_ast);
    destroy_ast_node(optimized_ast);
    F_destroy_constant_folding_manager_interface(manager);
    F_destroy_type_system(type_system);
    
    return 0;
}
```

### 单独使用常量折叠器

```c
#include "CN_semantic/constant_folding/CN_constant_folding.h"

void constant_folding_example(Stru_TypeSystem_t* type_system) {
    // 创建常量折叠器
    Stru_ConstantFoldingInterface_t* folder = F_create_constant_folding_interface();
    if (folder == NULL) {
        return;
    }
    
    // 初始化
    if (!folder->initialize(folder, type_system)) {
        F_destroy_constant_folding_interface(folder);
        return;
    }
    
    // 创建二元表达式：10 + 20
    Stru_AstNodeInterface_t* left = create_int_literal_node(10);
    Stru_AstNodeInterface_t* right = create_int_literal_node(20);
    Stru_AstNodeInterface_t* binary_expr = create_binary_expression("+", left, right);
    
    // 检查是否为常量表达式
    bool is_constant = folder->is_constant_expression(folder, binary_expr);
    printf("表达式是否为常量: %s\n", is_constant ? "是" : "否");
    
    // 折叠常量表达式
    Stru_AstNodeInterface_t* folded_expr = 
        folder->fold_constant_expression(folder, binary_expr);
    
    // 检查折叠结果
    if (folded_expr != binary_expr) {
        printf("表达式已折叠\n");
        
        // 尝试获取常量值
        Stru_ConstantValue_t constant_value;
        Stru_TypeDescriptor_t* type = NULL;
        if (folder->try_get_constant_value(folder, folded_expr, &constant_value, &type)) {
            printf("常量值: %ld\n", constant_value.value.integer_value);
        }
    }
    
    // 清理
    destroy_ast_node(binary_expr);
    destroy_ast_node(folded_expr);
    F_destroy_constant_folding_interface(folder);
}
```

### 表达式简化示例

```c
#include "CN_semantic/constant_folding/CN_constant_folding.h"

void expression_simplification_example(Stru_TypeSystem_t* type_system) {
    // 创建表达式简化器
    Stru_ExpressionSimplifierInterface_t* simplifier = F_create_expression_simplifier_interface();
    if (simplifier == NULL) {
        return;
    }
    
    // 初始化
    if (!simplifier->initialize(simplifier, type_system)) {
        F_destroy_expression_simplifier_interface(simplifier);
        return;
    }
    
    // 设置简化级别
    simplifier->set_simplification_level(simplifier, 1);
    
    // 创建需要简化的表达式：x + 0
    Stru_AstNodeInterface_t* variable = create_identifier_node("x");
    Stru_AstNodeInterface_t* zero = create_int_literal_node(0);
    Stru_AstNodeInterface_t* expression = create_binary_expression("+", variable, zero);
    
    printf("原始表达式: x + 0\n");
    
    // 简化表达式
    Stru_AstNodeInterface_t* simplified = 
        simplifier->simplify_expression(simplifier, expression);
    
    // 检查简化结果
    if (simplified != expression) {
        printf("简化后的表达式: x\n");
    }
    
    // 应用代数恒等式
    Stru_AstNodeInterface_t* after_algebraic = 
        simplifier->apply_algebraic_identities(simplifier, expression);
    
    // 应用逻辑简化
    Stru_AstNodeInterface_t* after_logical = 
        simplifier->apply_logical_simplifications(simplifier, expression);
    
    // 清理
    destroy_ast_node(expression);
    destroy_ast_node(simplified);
    destroy_ast_node(after_algebraic);
    destroy_ast_node(after_logical);
    F_destroy_expression_simplifier_interface(simplifier);
}
```

### 常量传播示例

```c
#include "CN_semantic/constant_folding/CN_constant_folding.h"

void constant_propagation_example(Stru_TypeSystem_t* type_system) {
    // 创建表达式简化器
    Stru_ExpressionSimplifierInterface_t* simplifier = F_create_expression_simplifier_interface();
    if (simplifier == NULL) {
