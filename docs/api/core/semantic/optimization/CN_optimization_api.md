# 高级优化模块 API 文档

## 概述

本文档描述了CN_Language高级优化模块的应用程序编程接口（API）。高级优化模块提供了常量传播、死代码消除、公共子表达式消除、循环优化和函数内联等优化技术的接口。

## 文件结构

```
src/core/semantic/optimization/
├── CN_optimization.h      # 头文件 - 接口定义
├── CN_optimization.c      # 源文件 - 接口实现
└── README.md              # 模块文档
```

## 接口定义

### 常量传播器接口

```c
/**
 * @brief 常量传播器接口结构体
 * 
 * 提供常量传播功能的抽象接口，用于将已知的常量值传播到表达式中。
 */
typedef struct Stru_ConstantPropagationInterface_t
{
    /**
     * @brief 初始化常量传播器
     * 
     * @param propagator 常量传播器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_ConstantPropagationInterface_t* propagator,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行常量传播
     * 
     * @param propagator 常量传播器实例
     * @param ast_node AST节点
     * @param constant_map 常量映射表
     * @return Stru_AstNodeInterface_t* 优化后的AST节点
     */
    Stru_AstNodeInterface_t* (*propagate_constants)(Stru_ConstantPropagationInterface_t* propagator,
                                                   Stru_AstNodeInterface_t* ast_node,
                                                   void* constant_map);
    
    /**
     * @brief 分析并收集常量值
     * 
     * @param propagator 常量传播器实例
     * @param ast_node AST节点
     * @param constant_map 输出的常量映射表
     * @return bool 分析成功返回true，失败返回false
     */
    bool (*analyze_constants)(Stru_ConstantPropagationInterface_t* propagator,
                             Stru_AstNodeInterface_t* ast_node,
                             void** constant_map);
    
    /**
     * @brief 检查表达式是否为常量
     * 
     * @param propagator 常量传播器实例
     * @param ast_node AST节点
     * @return bool 是常量表达式返回true，否则返回false
     */
    bool (*is_constant_expression)(Stru_ConstantPropagationInterface_t* propagator,
                                  const Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 获取表达式的常量值
     * 
     * @param propagator 常量传播器实例
     * @param ast_node AST节点
     * @param value 输出的常量值
     * @param type 输出的常量类型
     * @return bool 获取成功返回true，失败返回false
     */
    bool (*get_constant_value)(Stru_ConstantPropagationInterface_t* propagator,
                              const Stru_AstNodeInterface_t* ast_node,
                              void* value,
                              void** type);
    
    /**
     * @brief 销毁常量传播器
     * 
     * @param propagator 常量传播器实例
     */
    void (*destroy)(Stru_ConstantPropagationInterface_t* propagator);
    
    /** @brief 私有数据指针 */
    void* private_data;
} Stru_ConstantPropagationInterface_t;
```

### 死代码消除器接口

```c
/**
 * @brief 死代码消除器接口结构体
 * 
 * 提供死代码消除功能的抽象接口，用于移除不可达的代码分支和未使用的变量。
 */
typedef struct Stru_DeadCodeEliminationInterface_t
{
    /**
     * @brief 初始化死代码消除器
     * 
     * @param eliminator 死代码消除器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_DeadCodeEliminationInterface_t* eliminator,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行死代码消除
     * 
     * @param eliminator 死代码消除器实例
     * @param ast_node AST节点
     * @return Stru_AstNodeInterface_t* 优化后的AST节点
     */
    Stru_AstNodeInterface_t* (*eliminate_dead_code)(Stru_DeadCodeEliminationInterface_t* eliminator,
                                                   Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 分析代码可达性
     * 
     * @param eliminator 死代码消除器实例
     * @param ast_node AST节点
     * @param reachable_map 输出的可达性映射表
     * @return bool 分析成功返回true，失败返回false
     */
    bool (*analyze_reachability)(Stru_DeadCodeEliminationInterface_t* eliminator,
                                Stru_AstNodeInterface_t* ast_node,
                                void** reachable_map);
    
    /**
     * @brief 分析变量使用情况
     * 
     * @param eliminator 死代码消除器实例
     * @param ast_node AST节点
     * @param used_vars 输出的已使用变量集合
     * @param defined_vars 输出的已定义变量集合
     * @return bool 分析成功返回true，失败返回false
     */
    bool (*analyze_variable_usage)(Stru_DeadCodeEliminationInterface_t* eliminator,
                                  Stru_AstNodeInterface_t* ast_node,
                                  void** used_vars,
                                  void** defined_vars);
    
    /**
     * @brief 检查代码是否可达
     * 
     * @param eliminator 死代码消除器实例
     * @param ast_node AST节点
     * @return bool 可达返回true，否则返回false
     */
    bool (*is_reachable)(Stru_DeadCodeEliminationInterface_t* eliminator,
                        const Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 检查变量是否被使用
     * 
     * @param eliminator 死代码消除器实例
     * @param var_name 变量名
     * @param ast_node AST节点
     * @return bool 被使用返回true，否则返回false
     */
    bool (*is_variable_used)(Stru_DeadCodeEliminationInterface_t* eliminator,
                            const char* var_name,
                            const Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 销毁死代码消除器
     * 
     * @param eliminator 死代码消除器实例
     */
    void (*destroy)(Stru_DeadCodeEliminationInterface_t* eliminator);
    
    /** @brief 私有数据指针 */
    void* private_data;
} Stru_DeadCodeEliminationInterface_t;
```

### 公共子表达式消除器接口

```c
/**
 * @brief 公共子表达式消除器接口结构体
 * 
 * 提供公共子表达式消除功能的抽象接口，用于识别并重用重复的计算结果。
 */
typedef struct Stru_CommonSubexpressionEliminationInterface_t
{
    /**
     * @brief 初始化公共子表达式消除器
     * 
     * @param eliminator 公共子表达式消除器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_CommonSubexpressionEliminationInterface_t* eliminator,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行公共子表达式消除
     * 
     * @param eliminator 公共子表达式消除器实例
     * @param ast_node AST节点
     * @return Stru_AstNodeInterface_t* 优化后的AST节点
     */
    Stru_AstNodeInterface_t* (*eliminate_common_subexpressions)(Stru_CommonSubexpressionEliminationInterface_t* eliminator,
                                                               Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 分析表达式等价性
     * 
     * @param eliminator 公共子表达式消除器实例
     * @param expr1 第一个表达式
     * @param expr2 第二个表达式
     * @return bool 等价返回true，否则返回false
     */
    bool (*are_expressions_equivalent)(Stru_CommonSubexpressionEliminationInterface_t* eliminator,
                                      const Stru_AstNodeInterface_t* expr1,
                                      const Stru_AstNodeInterface_t* expr2);
    
    /**
     * @brief 计算表达式哈希值
     * 
     * @param eliminator 公共子表达式消除器实例
     * @param expr 表达式
     * @return uint64_t 表达式的哈希值
     */
    uint64_t (*hash_expression)(Stru_CommonSubexpressionEliminationInterface_t* eliminator,
                               const Stru_AstNodeInterface_t* expr);
    
    /**
     * @brief 销毁公共子表达式消除器
     * 
     * @param eliminator 公共子表达式消除器实例
     */
    void (*destroy)(Stru_CommonSubexpressionEliminationInterface_t* eliminator);
    
    /** @brief 私有数据指针 */
    void* private_data;
} Stru_CommonSubexpressionEliminationInterface_t;
```

### 循环优化器接口

```c
/**
 * @brief 循环优化器接口结构体
 * 
 * 提供循环优化功能的抽象接口，包括循环展开和循环不变代码外提。
 */
typedef struct Stru_LoopOptimizationInterface_t
{
    /**
     * @brief 初始化循环优化器
     * 
     * @param optimizer 循环优化器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_LoopOptimizationInterface_t* optimizer,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行循环展开
     * 
     * @param optimizer 循环优化器实例
     * @param loop_node 循环节点
     * @param unroll_factor 展开因子
     * @return Stru_AstNodeInterface_t* 优化后的循环节点
     */
    Stru_AstNodeInterface_t* (*unroll_loop)(Stru_LoopOptimizationInterface_t* optimizer,
                                           Stru_AstNodeInterface_t* loop_node,
                                           int unroll_factor);
    
    /**
     * @brief 执行循环不变代码外提
     * 
     * @param optimizer 循环优化器实例
     * @param loop_node 循环节点
     * @return Stru_AstNodeInterface_t* 优化后的循环节点
     */
    Stru_AstNodeInterface_t* (*hoist_invariant_code)(Stru_LoopOptimizationInterface_t* optimizer,
                                                    Stru_AstNodeInterface_t* loop_node);
    
    /**
     * @brief 分析循环不变表达式
     * 
     * @param optimizer 循环优化器实例
     * @param loop_node 循环节点
     * @param invariant_exprs 输出的不变表达式集合
     * @return bool 分析成功返回true，失败返回false
     */
    bool (*analyze_invariant_expressions)(Stru_LoopOptimizationInterface_t* optimizer,
                                         Stru_AstNodeInterface_t* loop_node,
                                         void** invariant_exprs);
    
    /**
     * @brief 检查表达式是否为循环不变
     * 
     * @param optimizer 循环优化器实例
     * @param expr 表达式
     * @param loop_node 循环节点
     * @return bool 是循环不变表达式返回true，否则返回false
     */
    bool (*is_invariant_expression)(Stru_LoopOptimizationInterface_t* optimizer,
                                   const Stru_AstNodeInterface_t* expr,
                                   const Stru_AstNodeInterface_t* loop_node);
    
    /**
     * @brief 销毁循环优化器
     * 
     * @param optimizer 循环优化器实例
     */
    void (*destroy)(Stru_LoopOptimizationInterface_t* optimizer);
    
    /** @brief 私有数据指针 */
    void* private_data;
} Stru_LoopOptimizationInterface_t;
```

### 函数内联器接口

```c
/**
 * @brief 函数内联器接口结构体
 * 
 * 提供函数内联功能的抽象接口，用于将小函数内联到调用处。
 */
typedef struct Stru_FunctionInliningInterface_t
{
    /**
     * @brief 初始化函数内联器
     * 
     * @param inliner 函数内联器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_FunctionInliningInterface_t* inliner,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行函数内联
     * 
     * @param inliner 函数内联器实例
     * @param call_node 函数调用节点
     * @param function_node 函数定义节点
     * @return Stru_AstNodeInterface_t* 内联后的调用节点
     */
    Stru_AstNodeInterface_t* (*inline_function)(Stru_FunctionInliningInterface_t* inliner,
                                               Stru_AstNodeInterface_t* call_node,
                                               Stru_AstNodeInterface_t* function_node);
    
    /**
     * @brief 检查函数是否适合内联
     * 
     * @param inliner 函数内联器实例
     * @param function_node 函数定义节点
     * @return bool 适合内联返回true，否则返回false
     */
    bool (*is_function_inlinable)(Stru_FunctionInliningInterface_t* inliner,
                                 const Stru_AstNodeInterface_t* function_node);
    
    /**
     * @brief 计算函数内联成本
     * 
     * @param inliner 函数内联器实例
     * @param function_node 函数定义节点
     * @return int 内联成本
     */
    int (*calculate_inlining_cost)(Stru_FunctionInliningInterface_t* inliner,
                                  const Stru_AstNodeInterface_t* function_node);
    
    /**
     * @brief 销毁函数内联器
     * 
     * @param inliner 函数内联器实例
     */
    void (*destroy)(Stru_FunctionInliningInterface_t* inliner);
    
    /** @brief 私有数据指针 */
    void* private_data;
} Stru_FunctionInliningInterface_t;
```

### 优化管理器接口

```c
/**
 * @brief 优化管理器接口结构体
 * 
 * 提供优化管理功能的抽象接口，用于协调和管理各个优化器。
 */
typedef struct Stru_OptimizationManagerInterface_t
{
    /**
     * @brief 初始化优化管理器
     * 
     * @param manager 优化管理器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_OptimizationManagerInterface_t* manager,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 优化AST节点
     * 
     * @param manager 优化管理器实例
     * @param ast_node AST节点
     * @param optimization_level 优化级别
     * @return Stru_AstNodeInterface_t* 优化后的AST节点
     */
    Stru_AstNodeInterface_t* (*optimize_ast)(Stru_OptimizationManagerInterface_t* manager,
                                            Stru_AstNodeInterface_t* ast_node,
                                            int optimization_level);
    
    /**
     * @brief 获取常量传播器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_ConstantPropagationInterface_t* 常量传播器接口
     */
    Stru_ConstantPropagationInterface_t* (*get_constant_propagator)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 获取死代码消除器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_DeadCodeEliminationInterface_t* 死代码消除器接口
     */
    Stru_DeadCodeEliminationInterface_t* (*get_dead_code_eliminator)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 获取公共子表达式消除器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_CommonSubexpressionEliminationInterface_t* 公共子表达式消除器接口
     */
    Stru_CommonSubexpressionEliminationInterface_t* (*get_common_subexpression_eliminator)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 获取循环优化器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_LoopOptimizationInterface_t* 循环优化器接口
     */
    Stru_LoopOptimizationInterface_t* (*get_loop_optimizer)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 获取函数内联器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_FunctionInliningInterface_t* 函数内联器接口
     */
    Stru_FunctionInliningInterface_t* (*get_function_inliner)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 设置优化选项
     * 
     * @param manager 优化管理器实例
     * @param enable_constant_propagation 是否启用常量传播
     * @param enable_dead_code_elimination 是否启用死代码消除
     * @param enable_cse 是否启用公共子表达式消除
     * @param enable_loop_optimization 是否启用循环优化
     * @param enable_function_inlining 是否启用函数内联
     * @param optimization_level 优化级别
     */
    void (*set_optimization_options)(Stru_OptimizationManagerInterface_t* manager,
                                    bool enable_constant_propagation,
                                    bool enable_dead_code_elimination,
                                    bool enable_cse,
                                    bool enable_loop_optimization,
                                    bool enable_function_inlining,
                                    int optimization_level);
    
    /**
     * @brief 重置优化管理器状态
     * 
     * @param manager 优化管理器实例
     */
    void (*reset)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 销毁优化管理器
     * 
     * @param manager 优化管理器实例
     */
    void (*destroy)(Stru_OptimizationManagerInterface_t* manager);
    
    /** @brief 私有数据指针 */
    void* private_data;
} Stru_OptimizationManagerInterface_t;
```

## 工厂函数

### 常量传播器工厂函数

```c
/**
 * @brief 创建常量传播器接口
 * 
 * @return Stru_ConstantPropagationInterface_t* 新创建的常量传播器接口，失败返回NULL
 */
Stru_ConstantPropagationInterface_t* F_create_constant_propagation_interface(void);

/**
 * @brief 销毁常量传播器接口
 * 
 * @param propagator 要销毁的常量传播器接口
 */
void F_destroy_constant_propagation_interface(Stru_ConstantPropagationInterface_t* propagator);
```

### 死代码消除器工厂函数

```c
/**
 * @brief 创建死代码消除器接口
 * 
 * @return Stru_DeadCodeEliminationInterface_t* 新创建的死代码消除器接口，失败返回NULL
 */
Stru_DeadCodeEliminationInterface_t* F_create_dead_code_elimination_interface(void);

/**
 * @brief 销毁死代码消除器接口
 * 
 * @param eliminator 要销毁的死代码消除器接口
 */
void F_destroy_dead_code_elimination_interface(Stru_DeadCodeEliminationInterface_t* eliminator);
```

### 公共子表达式消除器工厂函数

```c
/**
 * @brief 创建公共子表达式消除器接口
 * 
 * @return Stru_CommonSubexpressionEliminationInterface_t* 新创建的公共子表达式消除器接口，失败返回NULL
 */
Stru_CommonSubexpressionEliminationInterface_t* F_create_common_subexpression_elimination_interface(void);

/**
 * @brief 销毁公共子表达式消除器接口
 * 
 * @param eliminator 要销毁的公共子表达式消除器接口
 */
void F_destroy_common_subexpression_elimination_interface(Stru_CommonSubexpressionEliminationInterface_t* eliminator);
```

### 循环优化器工厂函数

```c
/**
 * @brief 创建循环优化器接口
 * 
 * @return Stru_LoopOptimizationInterface_t* 新创建的循环优化器接口，失败返回NULL
 */
Stru_LoopOptimizationInterface_t* F_create_loop_optimization_interface(void);

/**
 * @brief 销毁循环优化器接口
 * 
 * @param optimizer 要销毁的循环优化器接口
 */
void F_destroy_loop_optimization_interface(Stru_LoopOptimizationInterface_t* optimizer);
```

### 函数内联器工厂函数

```c
/**
 * @brief 创建函数内联器接口
 * 
 * @return Stru_FunctionInliningInterface_t* 新创建的函数内联器接口，失败返回NULL
 */
Stru_FunctionInliningInterface_t* F_create_function_inlining_interface(void);

/**
 * @brief 销毁函数内联器接口
 * 
 * @param inliner 要销毁的函数内联器接口
 */
void F_destroy_function_inlining_interface(Stru_FunctionInliningInterface_t* inliner);
```

### 优化管理器工厂函数

```c
/**
 * @brief 创建优化管理器接口
 * 
 * @return Stru_OptimizationManagerInterface_t* 新创建的优化管理器接口，失败返回NULL
 */
Stru_OptimizationManagerInterface_t* F_create_optimization_manager_interface(void);

/**
 * @brief 销毁优化管理器接口
 * 
 * @param manager 要销毁的优化管理器接口
 */
void F_destroy_optimization_manager_interface(Stru_OptimizationManagerInterface_t* manager);
```

## 使用示例

### 基本使用示例

```c
#include "src/core/semantic/optimization/CN_optimization.h"

// 创建优化管理器
Stru_OptimizationManagerInterface_t* manager = F_create_optimization_manager_interface();
if (!manager) {
    // 处理错误
    return;
}

// 初始化优化管理器
if (!manager->initialize(manager, type_system)) {
    // 处理初始化失败
    F_destroy_optimization_manager_interface(manager);
    return;
}

// 设置优化选项
manager->set_optimization_options(manager,
    true,   // 启用常量传播
    true,   // 启用死代码消除
    false,  // 禁用公共子表达式消除
    true,   // 启用循环优化
    false,  // 禁用函数内联
    1       // 优化级别1
);

// 优化AST节点
Stru_AstNodeInterface_t* optimized_ast = manager->optimize_ast(manager, ast_node, 1);

// 获取特定优化器
Stru_ConstantPropagationInterface_t* propagator = manager->get_constant_propagator(manager);
if (propagator) {
    // 使用常量传播器
    void* constant_map = NULL;
    propagator->analyze_constants(propagator, ast_node, &constant_map);
    Stru_AstNodeInterface_t* propagated_ast = propagator->propagate_constants(propagator, ast_node, constant_map);
}

// 销毁优化管理器
F_destroy_optimization_manager_interface(manager);
```

### 单独使用优化器示例

```c
#include "src/core/semantic/optimization/CN_optimization.h"

// 创建常量传播器
Stru_ConstantPropagationInterface_t* propagator = F_create_constant_propagation_interface();
if (!propagator) {
    // 处理错误
    return;
}

// 初始化常量传播器
if (!propagator->initialize(propagator, type_system)) {
    // 处理初始化失败
    F_destroy_constant_propagation_interface(propagator);
    return;
}

// 分析常量
void* constant_map = NULL;
if (propagator->analyze_constants(propagator, ast_node, &constant_map)) {
    // 执行常量传播
    Stru_AstNodeInterface_t* optimized_ast = propagator->propagate_constants(propagator, ast_node, constant_map);
    
    // 检查表达式是否为常量
    bool is_constant = propagator->is_constant_expression(propagator, ast_node);
    
    // 获取常量值
    void* value = NULL;
    void* type = NULL;
    if (propagator->get_constant_value(propagator, ast_node, &value, &type)) {
        // 使用常量值
    }
}

// 销毁常量传播器
F_destroy_constant_propagation_interface(propagator);
```

## 错误处理

### 返回值约定

1. **布尔返回值**：
   - `true`：操作成功
   - `false`：操作失败

2. **指针返回值**：
   - 非NULL：操作成功，返回有效指针
   - NULL：操作失败

3. **整数返回值**：
   - 非负值：操作成功，返回有意义的值
   - 负值或特殊值（如INT_MAX）：操作失败

### 错误类型

1. **内存分配错误**：当malloc失败时返回NULL
2. **参数错误**：当输入参数无效时返回false或NULL
3. **初始化错误**：当依赖组件初始化失败时返回false
4. **状态错误**：当优化器处于无效状态时返回false

### 错误恢复

1. **资源清理**：所有创建的资源都必须通过对应的销毁函数释放
2. **状态重置**：优化管理器提供reset函数重置状态
3. **错误传播**：错误通过返回值传递，不依赖全局状态

## 性能考虑

### 内存管理

1. **接口创建**：每个工厂函数都会分配内存，必须通过对应的销毁函数释放
2. **私有数据**：每个接口包含私有数据指针，由接口实现管理生命周期
3. **缓存策略**：优化器可以使用缓存提高性能，但需要注意内存使用

### 时间复杂度

1. **常量传播**：O(n)，n为AST节点数
2. **死代码消除**：O(n + e)，n为节点数，e为边数
3. **公共子表达式消除**：O(n log n)，使用哈希表
4. **循环优化**：O(m × k)，m为循环数，k为循环体大小
5. **函数内联**：O(f × s)，f为函数数，s为函数大小

### 线程安全

1. **非线程安全**：当前实现不支持多线程并发访问
2. **独立实例**：每个线程应创建独立的优化器实例
3. **共享资源**：类型系统等共享资源需要外部同步

## 扩展指南

### 添加新的优化器

1. **定义接口**：在CN_optimization.h中定义新的接口结构体
2. **实现接口**：在CN_optimization.c中实现接口函数
3. **添加工厂函数**：添加创建和销毁新优化器的工厂函数
4. **集成到管理器**：在优化管理器中添加对新优化器的支持
5. **更新文档**：更新API文档和README

### 自定义优化策略

1. **继承接口**：创建新的结构体继承现有接口
2. **重写函数**：重写特定的优化函数
3. **依赖注入**：通过构造函数参数配置优化策略
4. **组合模式**：组合多个优化器实现复杂策略

## 版本兼容性

### API版本

- **v1.0.0**：初始版本，提供五种基本优化技术
- **向后兼容**：后续版本保持API向后兼容
- **扩展接口**：通过添加新函数扩展接口，不影响现有代码

### 二进制兼容性

- **结构体布局**：保持结构体布局不变
- **函数指针顺序**：保持函数指针顺序不变
- **私有数据**：私有数据指针位置不变

## 相关文档

- [模块README](../../../src/core/semantic/optimization/README.md)
- [架构设计文档](../../../../docs/architecture/)
- [编码规范](../../../../docs/specifications/)

## 维护者

- CN_Language架构委员会

## 许可证

MIT License
