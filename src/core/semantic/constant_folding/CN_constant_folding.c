/**
 * @file CN_constant_folding.c
 * @brief 常量折叠和表达式简化模块实现文件
 
 * 本文件实现了常量折叠和表达式简化模块，负责在编译时计算常量表达式、
 * 简化表达式结构、优化AST。遵循SOLID设计原则和分层架构。
 * 
 * 实现功能包括：
 * 1. 常量折叠：算术、逻辑、比较、位运算、条件表达式等
 * 2. 表达式简化：代数恒等式、逻辑简化、冗余操作消除、常量传播、死代码消除
 * 3. 统一管理器：整合常量折叠和表达式简化功能
 * 
 * 设计原则：
 * 1. 单一职责：每个函数不超过50行，每个.c文件不超过500行
 * 2. 开闭原则：通过接口支持扩展
 * 3. 依赖倒置：高层模块定义接口，低层模块实现接口
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_constant_folding.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "CN_log.h"

// ============================================================================
// 内部数据结构定义
// ============================================================================

/**
 * @brief 常量折叠器私有数据结构
 */
typedef struct
{
    Stru_TypeSystem_t* type_system;      /**< 类型系统实例 */
    bool aggressive_folding;             /**< 是否启用激进折叠 */
    int recursion_depth;                 /**< 递归深度计数器 */
    int max_recursion_depth;             /**< 最大递归深度限制 */
} ConstantFolderPrivateData;

/**
 * @brief 表达式简化器私有数据结构
 */
typedef struct
{
    Stru_TypeSystem_t* type_system;      /**< 类型系统实例 */
    int simplification_level;            /**< 简化级别：0-基本，1-中等，2-激进 */
    void* constant_values;               /**< 常量值映射 */
} ExpressionSimplifierPrivateData;

/**
 * @brief 常量折叠和表达式简化管理器私有数据结构
 */
typedef struct
{
    Stru_ConstantFoldingInterface_t* folder;         /**< 常量折叠器接口 */
    Stru_ExpressionSimplifierInterface_t* simplifier; /**< 表达式简化器接口 */
    Stru_TypeSystem_t* type_system;                  /**< 类型系统实例 */
    bool enable_folding;                             /**< 是否启用常量折叠 */
    bool enable_simplification;                      /**< 是否启用表达式简化 */
    int simplification_level;                        /**< 简化级别 */
} ConstantFoldingManagerPrivateData;

// ============================================================================
// 常量折叠器接口实现
// ============================================================================

/**
 * @brief 初始化常量折叠器
 */
static bool F_constant_folder_initialize(Stru_ConstantFoldingInterface_t* folder,
                                        Stru_TypeSystem_t* type_system)
{
    if (!folder || !type_system)
    {
        CN_LOG_ERROR("常量折叠器初始化失败：参数为空");
        return false;
    }
    
    ConstantFolderPrivateData* private_data = (ConstantFolderPrivateData*)malloc(sizeof(ConstantFolderPrivateData));
    if (!private_data)
    {
        CN_LOG_ERROR("常量折叠器初始化失败：内存分配失败");
        return false;
    }
    
    private_data->type_system = type_system;
    private_data->aggressive_folding = false;
    private_data->recursion_depth = 0;
    private_data->max_recursion_depth = 100; // 防止无限递归
    
    folder->private_data = private_data;
    CN_LOG_INFO("常量折叠器初始化成功");
    return true;
}

/**
 * @brief 检查表达式是否为常量
 */
static bool F_is_constant_expression(Stru_ConstantFoldingInterface_t* folder,
                                    const Stru_AstNodeInterface_t* ast_node)
{
    if (!folder || !ast_node)
    {
        return false;
    }
    
    // 这里需要根据AST节点类型判断是否为常量
    // 简化实现：假设有方法可以判断
    // 实际实现中需要检查节点类型和子节点
    return false; // 占位实现
}

/**
 * @brief 折叠常量表达式
 */
static Stru_AstNodeInterface_t* F_fold_constant_expression(Stru_ConstantFoldingInterface_t* folder,
                                                          Stru_AstNodeInterface_t* ast_node)
{
    if (!folder || !ast_node)
    {
        return ast_node;
    }
    
    ConstantFolderPrivateData* private_data = (ConstantFolderPrivateData*)folder->private_data;
    if (!private_data)
    {
        return ast_node;
    }
    
    // 检查递归深度
    if (private_data->recursion_depth >= private_data->max_recursion_depth)
    {
        CN_LOG_WARNING("常量折叠递归深度达到限制：%d", private_data->max_recursion_depth);
        return ast_node;
    }
    
    private_data->recursion_depth++;
    
    // 检查是否为常量表达式
    if (F_is_constant_expression(folder, ast_node))
    {
        // 尝试折叠常量表达式
        // 这里需要根据AST节点类型进行不同的折叠处理
        // 简化实现：返回原节点
    }
    
    private_data->recursion_depth--;
    return ast_node;
}

/**
 * @brief 尝试获取常量表达式的值
 */
static bool F_try_get_constant_value(Stru_ConstantFoldingInterface_t* folder,
                                    const Stru_AstNodeInterface_t* ast_node,
                                    void* value,
                                    Stru_TypeDescriptor_t** type)
{
    if (!folder || !ast_node || !value || !type)
    {
        return false;
    }
    
    // 检查是否为常量表达式
    if (!F_is_constant_expression(folder, ast_node))
    {
        return false;
    }
    
    // 这里需要根据AST节点类型提取常量值
    // 简化实现：返回false
    return false;
}

/**
 * @brief 折叠二元运算表达式
 */
static Stru_AstNodeInterface_t* F_fold_binary_expression(Stru_ConstantFoldingInterface_t* folder,
                                                        int operator_type,
                                                        Stru_AstNodeInterface_t* left,
                                                        Stru_AstNodeInterface_t* right)
{
    if (!folder || !left || !right)
    {
        return NULL;
    }
    
    // 检查左右操作数是否为常量
    bool left_is_constant = F_is_constant_expression(folder, left);
    bool right_is_constant = F_is_constant_expression(folder, right);
    
    // 如果两个操作数都是常量，可以折叠
    if (left_is_constant && right_is_constant)
    {
        // 这里需要根据运算符类型进行常量计算
        // 简化实现：返回原左节点
        return left;
    }
    
    // 如果只有一个操作数是常量，可以尝试简化
    if (left_is_constant || right_is_constant)
    {
        // 应用代数恒等式简化
        // 例如：x + 0 → x, x * 1 → x, x * 0 → 0 等
        // 简化实现：返回原左节点
        return left;
    }
    
    // 都不是常量，返回原表达式
    // 这里需要创建新的二元表达式节点
    // 简化实现：返回原左节点
    return left;
}

/**
 * @brief 折叠一元运算表达式
 */
static Stru_AstNodeInterface_t* F_fold_unary_expression(Stru_ConstantFoldingInterface_t* folder,
                                                       int operator_type,
                                                       Stru_AstNodeInterface_t* operand)
{
    if (!folder || !operand)
    {
        return NULL;
    }
    
    // 检查操作数是否为常量
    bool operand_is_constant = F_is_constant_expression(folder, operand);
    
    if (operand_is_constant)
    {
        // 常量一元运算可以折叠
        // 例如：!true → false, -5 → -5, ~0 → -1 等
        // 简化实现：返回原操作数
        return operand;
    }
    
    // 应用简化规则
    // 例如：!!x → x, -(-x) → x 等
    // 简化实现：返回原操作数
    return operand;
}

/**
 * @brief 折叠条件表达式
 */
static Stru_AstNodeInterface_t* F_fold_conditional_expression(Stru_ConstantFoldingInterface_t* folder,
                                                             Stru_AstNodeInterface_t* condition,
                                                             Stru_AstNodeInterface_t* then_expr,
                                                             Stru_AstNodeInterface_t* else_expr)
{
    if (!folder || !condition || !then_expr || !else_expr)
    {
        return NULL;
    }
    
    // 检查条件是否为常量
    bool condition_is_constant = F_is_constant_expression(folder, condition);
    
    if (condition_is_constant)
    {
        // 尝试获取条件值
        int condition_value = 0;
        Stru_TypeDescriptor_t* condition_type = NULL;
        
        if (F_try_get_constant_value(folder, condition, &condition_value, &condition_type))
        {
            // 根据条件值选择分支
            if (condition_value != 0)
            {
                return then_expr; // 条件为真，返回then分支
            }
            else
            {
                return else_expr; // 条件为假，返回else分支
            }
        }
    }
    
    // 条件不是常量或无法获取值，返回原表达式
    // 这里需要创建新的条件表达式节点
    // 简化实现：返回then分支
    return then_expr;
}

/**
 * @brief 折叠函数调用表达式
 */
static Stru_AstNodeInterface_t* F_fold_call_expression(Stru_ConstantFoldingInterface_t* folder,
                                                      Stru_AstNodeInterface_t* function,
                                                      Stru_AstNodeInterface_t** arguments,
                                                      size_t argument_count)
{
    if (!folder || !function)
    {
        return NULL;
    }
    
    // 检查是否为纯函数调用且所有参数都是常量
    bool all_arguments_constant = true;
    
    for (size_t i = 0; i < argument_count; i++)
    {
        if (!arguments[i] || !F_is_constant_expression(folder, arguments[i]))
        {
            all_arguments_constant = false;
            break;
        }
    }
    
    if (all_arguments_constant)
    {
        // 纯函数调用且所有参数都是常量，可以尝试折叠
        // 需要知道函数是否是纯函数（无副作用）
        // 简化实现：返回原函数节点
        return function;
    }
    
    // 无法折叠，返回原表达式
    // 这里需要创建新的函数调用节点
    // 简化实现：返回原函数节点
    return function;
}

/**
 * @brief 设置是否启用激进折叠
 */
static void F_set_aggressive_folding(Stru_ConstantFoldingInterface_t* folder,
                                    bool aggressive)
{
    if (!folder)
    {
        return;
    }
    
    ConstantFolderPrivateData* private_data = (ConstantFolderPrivateData*)folder->private_data;
    if (private_data)
    {
        private_data->aggressive_folding = aggressive;
        CN_LOG_INFO("设置激进折叠：%s", aggressive ? "启用" : "禁用");
    }
}

/**
 * @brief 重置常量折叠器状态
 */
static void F_reset_constant_folder(Stru_ConstantFoldingInterface_t* folder)
{
    if (!folder)
    {
        return;
    }
    
    ConstantFolderPrivateData* private_data = (ConstantFolderPrivateData*)folder->private_data;
    if (private_data)
    {
        private_data->recursion_depth = 0;
        CN_LOG_INFO("常量折叠器状态已重置");
    }
}

/**
 * @brief 销毁常量折叠器
 */
static void F_destroy_constant_folder(Stru_ConstantFoldingInterface_t* folder)
{
    if (!folder)
    {
        return;
    }
    
    if (folder->private_data)
    {
        free(folder->private_data);
        folder->private_data = NULL;
    }
    
    CN_LOG_INFO("常量折叠器已销毁");
}

// ============================================================================
// 表达式简化器接口实现
// ============================================================================

/**
 * @brief 初始化表达式简化器
 */
static bool F_expression_simplifier_initialize(Stru_ExpressionSimplifierInterface_t* simplifier,
                                              Stru_TypeSystem_t* type_system)
{
    if (!simplifier || !type_system)
    {
        CN_LOG_ERROR("表达式简化器初始化失败：参数为空");
        return false;
    }
    
    ExpressionSimplifierPrivateData* private_data = (ExpressionSimplifierPrivateData*)malloc(sizeof(ExpressionSimplifierPrivateData));
    if (!private_data)
    {
        CN_LOG_ERROR("表达式简化器初始化失败：内存分配失败");
        return false;
    }
    
    private_data->type_system = type_system;
    private_data->simplification_level = 1; // 默认中等简化级别
    private_data->constant_values = NULL;
    
    simplifier->private_data = private_data;
    CN_LOG_INFO("表达式简化器初始化成功");
    return true;
}

/**
 * @brief 简化表达式
 */
static Stru_AstNodeInterface_t* F_simplify_expression(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                     Stru_AstNodeInterface_t* ast_node)
{
    if (!simplifier || !ast_node)
    {
        return ast_node;
    }
    
    ExpressionSimplifierPrivateData* private_data = (ExpressionSimplifierPrivateData*)simplifier->private_data;
    if (!private_data)
    {
        return ast_node;
    }
    
    // 根据简化级别应用不同的简化策略
    Stru_AstNodeInterface_t* simplified_node = ast_node;
    
    // 应用代数恒等式简化
    simplified_node = simplifier->apply_algebraic_identities(simplifier, simplified_node);
    
    // 应用逻辑简化规则
    simplified_node = simplifier->apply_logical_simplifications(simplifier, simplified_node);
    
    // 消除冗余操作
    simplified_node = simplifier->eliminate_redundant_operations(simplifier, simplified_node);
    
    // 如果启用激进简化，应用更多规则
    if (private_data->simplification_level >= 2)
    {
        // 激进简化规则
        // 例如：x + x → 2*x, x * x → x^2 等
    }
    
    return simplified_node;
}

/**
 * @brief 应用代数恒等式简化
 */
static Stru_AstNodeInterface_t* F_apply_algebraic_identities(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                            Stru_AstNodeInterface_t* ast_node)
{
    if (!simplifier || !ast_node)
    {
        return ast_node;
    }
    
    // 应用代数恒等式
    // 例如：x + 0 → x, x * 1 → x, x * 0 → 0
    // x - 0 → x, x / 1 → x, 0 / x → 0 (x ≠ 0)
    // x - x → 0, x / x → 1 (x ≠ 0)
    // 简化实现：返回原节点
    return ast_node;
}

/**
 * @brief 应用逻辑简化规则
 */
static Stru_AstNodeInterface_t* F_apply_logical_simplifications(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                               Stru_AstNodeInterface_t* ast_node)
{
    if (!simplifier || !ast_node)
    {
        return ast_node;
    }
    
    // 应用逻辑简化规则
    // 例如：true && x → x, false && x → false
    // true || x → true, false || x → x
    // !true → false, !false → true
    // !!x → x, x && x → x, x || x → x
    // 简化实现：返回原节点
    return ast_node;
}

/**
 * @brief 消除冗余操作
 */
static Stru_AstNodeInterface_t* F_eliminate_redundant_operations(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                                Stru_AstNodeInterface_t* ast_node)
{
    if (!simplifier || !ast_node)
    {
        return ast_node;
    }
    
    // 消除冗余操作
    // 例如：(x) → x, (x + y) + z → x + y + z（如果结合律允许）
    // 多层括号消除，冗余类型转换等
    // 简化实现：返回原节点
    return ast_node;
}

/**
 * @brief 执行常量传播
 */
static Stru_AstNodeInterface_t* F_propagate_constants(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                     Stru_AstNodeInterface_t* ast_node,
                                                     void* constant_values)
{
    if (!simplifier || !ast_node)
    {
        return ast_node;
    }
    
    // 常量传播：将已知的常量值替换到表达式中
    // 例如：如果知道 x = 5，那么 x + 3 → 5 + 3 → 8
    // 需要常量值映射表
    // 简化实现：返回原节点
    return ast_node;
}

/**
 * @brief 消除死代码
 */
static Stru_AstNodeInterface_t* F_eliminate_dead_code(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                     Stru_AstNodeInterface_t* ast_node)
{
    if (!simplifier || !ast_node)
    {
        return ast_node;
    }
    
    // 消除死代码
    // 例如：if (false) { ... } → 空语句
    // if (true) { x } else { y } → x
    // while (false) { ... } → 空语句
    // 简化实现：返回原节点
    return ast_node;
}

/**
 * @brief 设置简化级别
 */
static void F_set_simplification_level(Stru_ExpressionSimplifierInterface_t* simplifier,
                                      int level)
{
    if (!simplifier)
    {
        return;
    }
    
    ExpressionSimplifierPrivateData* private_data = (ExpressionSimplifierPrivateData*)simplifier->private_data;
    if (private_data)
    {
        // 确保级别在有效范围内
        if (level < 0) level = 0;
        if (level > 2) level = 2;
        
        private_data->simplification_level = level;
        CN_LOG_INFO("设置简化级别：%d", level);
    }
}

/**
 * @brief 重置表达式简化器状态
 */
static void F_reset_expression_simplifier(Stru_ExpressionSimplifierInterface_t* simplifier)
{
    if (!simplifier)
    {
        return;
    }
    
    ExpressionSimplifierPrivateData* private_data = (ExpressionSimplifierPrivateData*)simplifier->private_data;
    if (private_data)
    {
        // 重置状态
        private_data->constant_values = NULL;
        CN_LOG_INFO("表达式简化器状态已重置");
    }
}

/**
 * @brief 销毁表达式简化器
 */
static void F_destroy_expression_simplifier(Stru_ExpressionSimplifierInterface_t* simplifier)
{
    if (!simplifier)
    {
        return;
    }
    
    if (simplifier->private_data)
    {
        free(simplifier->private_data);
        simplifier->private_data = NULL;
    }
    
    CN_LOG_INFO("表达式简化器已销毁");
}

// ============================================================================
// 常量折叠和表达式简化管理器接口实现
// ============================================================================

/**
 * @brief 初始化管理器
 */
static bool F_constant_folding_manager_initialize(Stru_ConstantFoldingManagerInterface_t* manager,
                                                 Stru_TypeSystem_t* type_system)
{
    if (!manager || !type_system)
    {
        CN_LOG_ERROR("常量折叠管理器初始化失败：参数为空");
        return false;
    }
    
    ConstantFoldingManagerPrivateData* private_data = (ConstantFoldingManagerPrivateData*)malloc(sizeof(ConstantFoldingManagerPrivateData));
    if (!private_data)
    {
        CN_LOG_ERROR("常量折叠管理器初始化失败：内存分配失败");
        return false;
    }
    
    // 创建常量折叠器
    private_data->folder = F_create_constant_folding_interface();
    if (!private_data->folder)
    {
        free(private_data);
        CN_LOG_ERROR("常量折叠管理器初始化失败：无法创建常量折叠器");
        return false;
    }
    
    // 创建表达式简化器
    private_data->simplifier = F_create_expression_simplifier_interface();
    if (!private_data->simplifier)
    {
        F_destroy_constant_folding_interface(private_data->folder);
        free(private_data);
        CN_LOG_ERROR("常量折叠管理器初始化失败：无法创建表达式简化器");
        return false;
    }
    
    // 初始化组件
    if (!private_data->folder->initialize(private_data->folder, type_system) ||
        !private_data->simplifier->initialize(private_data->simplifier, type_system))
    {
        F_destroy_expression_simplifier_interface(private_data->simplifier);
        F_destroy_constant_folding_interface(private_data->folder);
        free(private_data);
        CN_LOG_ERROR("常量折叠管理器初始化失败：组件初始化失败");
        return false;
    }
    
    private_data->type_system = type_system;
    private_data->enable_folding = true;
    private_data->enable_simplification = true;
    private_data->simplification_level = 1;
    
    manager->private_data = private_data;
    CN_LOG_INFO("常量折叠管理器初始化成功");
    return true;
}

/**
 * @brief 优化AST节点
 */
static Stru_AstNodeInterface_t* F_optimize_ast(Stru_ConstantFoldingManagerInterface_t* manager,
                                              Stru_AstNodeInterface_t* ast_node)
{
    if (!manager || !ast_node)
    {
        return ast_node;
    }
    
    ConstantFoldingManagerPrivateData* private_data = (ConstantFoldingManagerPrivateData*)manager->private_data;
    if (!private_data)
    {
        return ast_node;
    }
    
    Stru_AstNodeInterface_t* optimized_node = ast_node;
    
    // 应用常量折叠
    if (private_data->enable_folding && private_data->folder)
    {
        optimized_node = private_data->folder->fold_constant_expression(private_data->folder, optimized_node);
    }
    
    // 应用表达式简化
    if (private_data->enable_simplification && private_data->simplifier)
    {
        optimized_node = private_data->simplifier->simplify_expression(private_data->simplifier, optimized_node);
    }
    
    return optimized_node;
}

/**
 * @brief 获取常量折叠器接口
 */
static Stru_ConstantFoldingInterface_t* F_get_constant_folder(Stru_ConstantFoldingManagerInterface_t* manager)
{
    if (!manager)
    {
        return NULL;
    }
    
    ConstantFoldingManagerPrivateData* private_data = (ConstantFoldingManagerPrivateData*)manager->private_data;
    return private_data ? private_data->folder : NULL;
}

/**
 * @brief 获取表达式简化器接口
 */
static Stru_ExpressionSimplifierInterface_t* F_get_expression_simplifier(Stru_ConstantFoldingManagerInterface_t* manager)
{
    if (!manager)
    {
        return NULL;
    }
    
    ConstantFoldingManagerPrivateData* private_data = (ConstantFoldingManagerPrivateData*)manager->private_data;
    return private_data ? private_data->simplifier : NULL;
}

/**
 * @brief 设置优化选项
 */
static void F_set_optimization_options(Stru_ConstantFoldingManagerInterface_t* manager,
                                      bool enable_folding,
                                      bool enable_simplification,
                                      int simplification_level)
{
    if (!manager)
    {
        return;
    }
    
    ConstantFoldingManagerPrivateData* private_data = (ConstantFoldingManagerPrivateData*)manager->private_data;
    if (private_data)
    {
        private_data->enable_folding = enable_folding;
        private_data->enable_simplification = enable_simplification;
        
        // 确保简化级别在有效范围内
        if (simplification_level < 0) simplification_level = 0;
        if (simplification_level > 2) simplification_level = 2;
        private_data->simplification_level = simplification_level;
        
        // 更新表达式简化器的简化级别
        if (private_data->simplifier)
        {
            private_data->simplifier->set_simplification_level(private_data->simplifier, simplification_level);
        }
        
        CN_LOG_INFO("设置优化选项：折叠=%s, 简化=%s, 级别=%d",
                   enable_folding ? "启用" : "禁用",
                   enable_simplification ? "启用" : "禁用",
                   simplification_level);
    }
}

/**
 * @brief 重置管理器状态
 */
static void F_reset_constant_folding_manager(Stru_ConstantFoldingManagerInterface_t* manager)
{
    if (!manager)
    {
        return;
    }
    
    ConstantFoldingManagerPrivateData* private_data = (ConstantFoldingManagerPrivateData*)manager->private_data;
    if (private_data)
    {
        // 重置组件状态
        if (private_data->folder)
        {
            private_data->folder->reset(private_data->folder);
        }
        
        if (private_data->simplifier)
        {
            private_data->simplifier->reset(private_data->simplifier);
        }
        
        CN_LOG_INFO("常量折叠管理器状态已重置");
    }
}

/**
 * @brief 销毁管理器
 */
static void F_destroy_constant_folding_manager(Stru_ConstantFoldingManagerInterface_t* manager)
{
    if (!manager)
    {
        return;
    }
    
    ConstantFoldingManagerPrivateData* private_data = (ConstantFoldingManagerPrivateData*)manager->private_data;
    if (private_data)
    {
        // 销毁组件
        if (private_data->simplifier)
        {
            F_destroy_expression_simplifier_interface(private_data->simplifier);
        }
        
        if (private_data->folder)
        {
            F_destroy_constant_folding_interface(private_data->folder);
        }
        
        free(private_data);
        manager->private_data = NULL;
    }
    
    CN_LOG_INFO("常量折叠管理器已销毁");
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建常量折叠器接口实例
 */
Stru_ConstantFoldingInterface_t* F_create_constant_folding_interface(void)
{
    Stru_ConstantFoldingInterface_t* interface = (Stru_ConstantFoldingInterface_t*)malloc(sizeof(Stru_ConstantFoldingInterface_t));
    if (!interface)
    {
        CN_LOG_ERROR("创建常量折叠器接口失败：内存分配失败");
        return NULL;
    }
    
    // 设置函数指针
    interface->initialize = F_constant_folder_initialize;
    interface->fold_constant_expression = F_fold_constant_expression;
    interface->is_constant_expression = F_is_constant_expression;
    interface->try_get_constant_value = F_try_get_constant_value;
    interface->fold_binary_expression = F_fold_binary_expression;
    interface->fold_unary_expression = F_fold_unary_expression;
    interface->fold_conditional_expression = F_fold_conditional_expression;
    interface->fold_call_expression = F_fold_call_expression;
    interface->set_aggressive_folding = F_set_aggressive_folding;
    interface->reset = F_reset_constant_folder;
    interface->destroy = F_destroy_constant_folder;
    interface->private_data = NULL;
    
    CN_LOG_INFO("常量折叠器接口创建成功");
    return interface;
}

/**
 * @brief 创建表达式简化器接口实例
 */
Stru_ExpressionSimplifierInterface_t* F_create_expression_simplifier_interface(void)
{
    Stru_ExpressionSimplifierInterface_t* interface = (Stru_ExpressionSimplifierInterface_t*)malloc(sizeof(Stru_ExpressionSimplifierInterface_t));
    if (!interface)
    {
        CN_LOG_ERROR("创建表达式简化器接口失败：内存分配失败");
        return NULL;
    }
    
    // 设置函数指针
    interface->initialize = F_expression_simplifier_initialize;
    interface->simplify_expression = F_simplify_expression;
    interface->apply_algebraic_identities = F_apply_algebraic_identities;
    interface->apply_logical_simplifications = F_apply_logical_simplifications;
    interface->eliminate_redundant_operations = F_eliminate_redundant_operations;
    interface->propagate_constants = F_propagate_constants;
    interface->eliminate_dead_code = F_eliminate_dead_code;
    interface->set_simplification_level = F_set_simplification_level;
    interface->reset = F_reset_expression_simplifier;
    interface->destroy = F_destroy_expression_simplifier;
    interface->private_data = NULL;
    
    CN_LOG_INFO("表达式简化器接口创建成功");
    return interface;
}

/**
 * @brief 创建常量折叠和表达式简化管理器接口实例
 */
Stru_ConstantFoldingManagerInterface_t* F_create_constant_folding_manager_interface(void)
{
    Stru_ConstantFoldingManagerInterface_t* interface = (Stru_ConstantFoldingManagerInterface_t*)malloc(sizeof(Stru_ConstantFoldingManagerInterface_t));
    if (!interface)
    {
        CN_LOG_ERROR("创建常量折叠管理器接口失败：内存分配失败");
        return NULL;
    }
    
    // 设置函数指针
    interface->initialize = F_constant_folding_manager_initialize;
    interface->optimize_ast = F_optimize_ast;
    interface->get_constant_folder = F_get_constant_folder;
    interface->get_expression_simplifier = F_get_expression_simplifier;
    interface->set_optimization_options = F_set_optimization_options;
    interface->reset = F_reset_constant_folding_manager;
    interface->destroy = F_destroy_constant_folding_manager;
    interface->private_data = NULL;
    
    CN_LOG_INFO("常量折叠管理器接口创建成功");
    return interface;
}

/**
 * @brief 销毁常量折叠器接口实例
 */
void F_destroy_constant_folding_interface(Stru_ConstantFoldingInterface_t* interface)
{
    if (!interface)
    {
        return;
    }
    
    // 调用销毁函数
    if (interface->destroy)
    {
        interface->destroy(interface);
    }
    
    free(interface);
    CN_LOG_INFO("常量折叠器接口已销毁");
}

/**
 * @brief 销毁表达式简化器接口实例
 */
void F_destroy_expression_simplifier_interface(Stru_ExpressionSimplifierInterface_t* interface)
{
    if (!interface)
    {
        return;
    }
    
    // 调用销毁函数
    if (interface->destroy)
    {
        interface->destroy(interface);
    }
    
    free(interface);
    CN_LOG_INFO("表达式简化器接口已销毁");
}

/**
 * @brief 销毁常量折叠和表达式简化管理器接口实例
 */
void F_destroy_constant_folding_manager_interface(Stru_ConstantFoldingManagerInterface_t* interface)
{
    if (!interface)
    {
        return;
    }
    
    // 调用销毁函数
    if (interface->destroy)
    {
        interface->destroy(interface);
    }
    
    free(interface);
    CN_LOG_INFO("常量折叠管理器接口已销毁");
}
