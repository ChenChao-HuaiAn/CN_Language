/**
 * @file CN_semantic_interface.c
 * @brief 语义分析接口实现
 
 * 实现语义分析抽象接口的工厂函数和桩实现。
 * 遵循单一职责原则，每个接口独立实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_semantic_interface.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 符号表接口实现
// ============================================================================

/**
 * @brief 符号表接口私有数据
 */
typedef struct Stru_SymbolTableData_t
{
    Stru_SymbolTableInterface_t* parent_scope;  ///< 父作用域符号表
    size_t symbol_count;                        ///< 符号数量
    size_t capacity;                            ///< 符号表容量
    Stru_SymbolInfo_t* symbols;                 ///< 符号数组
} Stru_SymbolTableData_t;

/**
 * @brief 符号表初始化函数
 */
static bool symbol_table_initialize(Stru_SymbolTableInterface_t* symbol_table, 
                                   Stru_SymbolTableInterface_t* parent_scope)
{
    if (symbol_table == NULL || symbol_table->private_data != NULL)
    {
        return false;
    }
    
    Stru_SymbolTableData_t* data = 
        (Stru_SymbolTableData_t*)malloc(sizeof(Stru_SymbolTableData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->parent_scope = parent_scope;
    data->symbol_count = 0;
    data->capacity = 16;  // 初始容量
    data->symbols = (Stru_SymbolInfo_t*)malloc(data->capacity * sizeof(Stru_SymbolInfo_t));
    
    if (data->symbols == NULL)
    {
        free(data);
        return false;
    }
    
    symbol_table->private_data = data;
    return true;
}

/**
 * @brief 插入符号
 */
static bool symbol_table_insert_symbol(Stru_SymbolTableInterface_t* symbol_table, 
                                      const Stru_SymbolInfo_t* symbol)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL || symbol == NULL)
    {
        return false;
    }
    
    Stru_SymbolTableData_t* data = (Stru_SymbolTableData_t*)symbol_table->private_data;
    
    // 检查是否已存在同名符号
    for (size_t i = 0; i < data->symbol_count; i++)
    {
        if (strcmp(data->symbols[i].name, symbol->name) == 0)
        {
            return false;  // 重复定义
        }
    }
    
    // 检查是否需要扩容
    if (data->symbol_count >= data->capacity)
    {
        size_t new_capacity = data->capacity * 2;
        Stru_SymbolInfo_t* new_symbols = 
            (Stru_SymbolInfo_t*)realloc(data->symbols, new_capacity * sizeof(Stru_SymbolInfo_t));
        if (new_symbols == NULL)
        {
            return false;
        }
        data->symbols = new_symbols;
        data->capacity = new_capacity;
    }
    
    // 插入符号
    data->symbols[data->symbol_count] = *symbol;
    data->symbol_count++;
    
    return true;
}

/**
 * @brief 查找符号
 */
static Stru_SymbolInfo_t* symbol_table_lookup_symbol(Stru_SymbolTableInterface_t* symbol_table,
                                                    const char* name, bool search_parent)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL || name == NULL)
    {
        return NULL;
    }
    
    Stru_SymbolTableData_t* data = (Stru_SymbolTableData_t*)symbol_table->private_data;
    
    // 在当前符号表中查找
    for (size_t i = 0; i < data->symbol_count; i++)
    {
        if (strcmp(data->symbols[i].name, name) == 0)
        {
            return &data->symbols[i];
        }
    }
    
    // 如果允许在父作用域中查找且存在父作用域
    if (search_parent && data->parent_scope != NULL)
    {
        return data->parent_scope->lookup_symbol(data->parent_scope, name, true);
    }
    
    return NULL;
}

/**
 * @brief 删除符号
 */
static bool symbol_table_remove_symbol(Stru_SymbolTableInterface_t* symbol_table,
                                      const char* name)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL || name == NULL)
    {
        return false;
    }
    
    Stru_SymbolTableData_t* data = (Stru_SymbolTableData_t*)symbol_table->private_data;
    
    for (size_t i = 0; i < data->symbol_count; i++)
    {
        if (strcmp(data->symbols[i].name, name) == 0)
        {
            // 将最后一个符号移动到当前位置
            if (i < data->symbol_count - 1)
            {
                data->symbols[i] = data->symbols[data->symbol_count - 1];
            }
            data->symbol_count--;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取符号数量
 */
static size_t symbol_table_get_symbol_count(Stru_SymbolTableInterface_t* symbol_table)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL)
    {
        return 0;
    }
    
    Stru_SymbolTableData_t* data = (Stru_SymbolTableData_t*)symbol_table->private_data;
    return data->symbol_count;
}

/**
 * @brief 获取所有符号
 */
static size_t symbol_table_get_all_symbols(Stru_SymbolTableInterface_t* symbol_table,
                                          Stru_SymbolInfo_t* symbols, size_t max_symbols)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL || symbols == NULL)
    {
        return 0;
    }
    
    Stru_SymbolTableData_t* data = (Stru_SymbolTableData_t*)symbol_table->private_data;
    size_t count = (data->symbol_count < max_symbols) ? data->symbol_count : max_symbols;
    
    for (size_t i = 0; i < count; i++)
    {
        symbols[i] = data->symbols[i];
    }
    
    return count;
}

/**
 * @brief 清空符号表
 */
static void symbol_table_clear(Stru_SymbolTableInterface_t* symbol_table)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL)
    {
        return;
    }
    
    Stru_SymbolTableData_t* data = (Stru_SymbolTableData_t*)symbol_table->private_data;
    data->symbol_count = 0;
}

/**
 * @brief 销毁符号表
 */
static void symbol_table_destroy(Stru_SymbolTableInterface_t* symbol_table)
{
    if (symbol_table == NULL)
    {
        return;
    }
    
    if (symbol_table->private_data != NULL)
    {
        Stru_SymbolTableData_t* data = (Stru_SymbolTableData_t*)symbol_table->private_data;
        
        if (data->symbols != NULL)
        {
            free(data->symbols);
        }
        
        free(data);
        symbol_table->private_data = NULL;
    }
}

/**
 * @brief 创建符号表接口实例
 */
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void)
{
    Stru_SymbolTableInterface_t* interface = 
        (Stru_SymbolTableInterface_t*)malloc(sizeof(Stru_SymbolTableInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = symbol_table_initialize;
    interface->insert_symbol = symbol_table_insert_symbol;
    interface->lookup_symbol = symbol_table_lookup_symbol;
    interface->remove_symbol = symbol_table_remove_symbol;
    interface->get_symbol_count = symbol_table_get_symbol_count;
    interface->get_all_symbols = symbol_table_get_all_symbols;
    interface->clear = symbol_table_clear;
    interface->destroy = symbol_table_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁符号表接口实例
 */
void F_destroy_symbol_table_interface(Stru_SymbolTableInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}

// ============================================================================
// 作用域管理器接口实现
// ============================================================================

/**
 * @brief 作用域管理器接口私有数据
 */
typedef struct Stru_ScopeManagerData_t
{
    Stru_SymbolTableInterface_t* global_scope;  ///< 全局作用域符号表
    size_t scope_depth;                         ///< 当前作用域深度
    size_t scope_capacity;                      ///< 作用域栈容量
    Stru_SymbolTableInterface_t** scope_stack;  ///< 作用域栈
} Stru_ScopeManagerData_t;

/**
 * @brief 作用域管理器初始化函数
 */
static bool scope_manager_initialize(Stru_ScopeManagerInterface_t* scope_manager,
                                    Stru_SymbolTableInterface_t* global_symbol_table)
{
    if (scope_manager == NULL || scope_manager->private_data != NULL)
    {
        return false;
    }
    
    Stru_ScopeManagerData_t* data = 
        (Stru_ScopeManagerData_t*)malloc(sizeof(Stru_ScopeManagerData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->global_scope = global_symbol_table;
    data->scope_depth = 0;
    data->scope_capacity = 16;
    data->scope_stack = (Stru_SymbolTableInterface_t**)malloc(
        data->scope_capacity * sizeof(Stru_SymbolTableInterface_t*));
    
    if (data->scope_stack == NULL)
    {
        free(data);
        return false;
    }
    
    // 将全局作用域压入栈
    if (data->global_scope != NULL)
    {
        data->scope_stack[0] = data->global_scope;
        data->scope_depth = 1;
    }
    
    scope_manager->private_data = data;
    return true;
}

/**
 * @brief 创建新作用域
 */
static Stru_SymbolTableInterface_t* scope_manager_create_scope(
    Stru_ScopeManagerInterface_t* scope_manager, Eum_ScopeType type, const char* name)
{
    (void)type;
    (void)name;
    
    if (scope_manager == NULL || scope_manager->private_data == NULL)
    {
        return NULL;
    }
    
    // 创建新的符号表
    Stru_SymbolTableInterface_t* new_scope = F_create_symbol_table_interface();
    if (new_scope == NULL)
    {
        return NULL;
    }
    
    // 获取当前作用域作为父作用域
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    Stru_SymbolTableInterface_t* parent_scope = NULL;
    
    if (data->scope_depth > 0)
    {
        parent_scope = data->scope_stack[data->scope_depth - 1];
    }
    
    // 初始化新作用域
    if (!new_scope->initialize(new_scope, parent_scope))
    {
        F_destroy_symbol_table_interface(new_scope);
        return NULL;
    }
    
    return new_scope;
}

/**
 * @brief 进入作用域
 */
static bool scope_manager_enter_scope(Stru_ScopeManagerInterface_t* scope_manager,
                                     Stru_SymbolTableInterface_t* symbol_table)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL || symbol_table == NULL)
    {
        return false;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    
    // 检查是否需要扩容
    if (data->scope_depth >= data->scope_capacity)
    {
        size_t new_capacity = data->scope_capacity * 2;
        Stru_SymbolTableInterface_t** new_stack = 
            (Stru_SymbolTableInterface_t**)realloc(
                data->scope_stack, new_capacity * sizeof(Stru_SymbolTableInterface_t*));
        if (new_stack == NULL)
        {
            return false;
        }
        data->scope_stack = new_stack;
        data->scope_capacity = new_capacity;
    }
    
    // 压入作用域栈
    data->scope_stack[data->scope_depth] = symbol_table;
    data->scope_depth++;
    
    return true;
}

/**
 * @brief 退出当前作用域
 */
static Stru_SymbolTableInterface_t* scope_manager_exit_scope(
    Stru_ScopeManagerInterface_t* scope_manager)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    
    if (data->scope_depth == 0)
    {
        return NULL;
    }
    
    data->scope_depth--;
    
    // 返回新的当前作用域
    if (data->scope_depth > 0)
    {
        return data->scope_stack[data->scope_depth - 1];
    }
    
    return NULL;
}

/**
 * @brief 获取当前作用域
 */
static Stru_SymbolTableInterface_t* scope_manager_get_current_scope(
    Stru_ScopeManagerInterface_t* scope_manager)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    
    if (data->scope_depth == 0)
    {
        return NULL;
    }
    
    return data->scope_stack[data->scope_depth - 1];
}

/**
 * @brief 获取全局作用域
 */
static Stru_SymbolTableInterface_t* scope_manager_get_global_scope(
    Stru_ScopeManagerInterface_t* scope_manager)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    return data->global_scope;
}

/**
 * @brief 获取作用域深度
 */
static size_t scope_manager_get_scope_depth(Stru_ScopeManagerInterface_t* scope_manager)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL)
    {
        return 0;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    return data->scope_depth;
}

/**
 * @brief 查找符号（在当前作用域链中）
 */
static Stru_SymbolInfo_t* scope_manager_lookup_symbol_in_scope_chain(
    Stru_ScopeManagerInterface_t* scope_manager, const char* name)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL || name == NULL)
    {
        return NULL;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    
    // 从当前作用域开始向上查找
    for (int i = (int)data->scope_depth - 1; i >= 0; i--)
    {
        Stru_SymbolTableInterface_t* current_scope = data->scope_stack[i];
        Stru_SymbolInfo_t* symbol = current_scope->lookup_symbol(current_scope, name, false);
        if (symbol != NULL)
        {
            return symbol;
        }
    }
    
    return NULL;
}

/**
 * @brief 销毁作用域管理器
 */
static void scope_manager_destroy(Stru_ScopeManagerInterface_t* scope_manager)
{
    if (scope_manager == NULL)
    {
        return;
    }
    
    if (scope_manager->private_data != NULL)
    {
        Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
        
        if (data->scope_stack != NULL)
        {
            free(data->scope_stack);
        }
        
        free(data);
        scope_manager->private_data = NULL;
    }
}

/**
 * @brief 创建作用域管理器接口实例
 */
Stru_ScopeManagerInterface_t* F_create_scope_manager_interface(void)
{
    Stru_ScopeManagerInterface_t* interface = 
        (Stru_ScopeManagerInterface_t*)malloc(sizeof(Stru_ScopeManagerInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = scope_manager_initialize;
    interface->create_scope = scope_manager_create_scope;
    interface->enter_scope = scope_manager_enter_scope;
    interface->exit_scope = scope_manager_exit_scope;
    interface->get_current_scope = scope_manager_get_current_scope;
    interface->get_global_scope = scope_manager_get_global_scope;
    interface->get_scope_depth = scope_manager_get_scope_depth;
    interface->lookup_symbol_in_scope_chain = scope_manager_lookup_symbol_in_scope_chain;
    interface->destroy = scope_manager_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁作用域管理器接口实例
 */
void F_destroy_scope_manager_interface(Stru_ScopeManagerInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}

// ============================================================================
// 类型检查器接口实现
// ============================================================================

/**
 * @brief 类型检查器接口私有数据
 */
typedef struct Stru_TypeCheckerData_t
{
    Stru_ScopeManagerInterface_t* scope_manager;  ///< 作用域管理器
} Stru_TypeCheckerData_t;

/**
 * @brief 类型检查器初始化函数
 */
static bool type_checker_initialize(Stru_TypeCheckerInterface_t* type_checker,
                                   Stru_ScopeManagerInterface_t* scope_manager)
{
    if (type_checker == NULL || type_checker->private_data != NULL)
    {
        return false;
    }
    
    Stru_TypeCheckerData_t* data = 
        (Stru_TypeCheckerData_t*)malloc(sizeof(Stru_TypeCheckerData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->scope_manager = scope_manager;
    
    type_checker->private_data = data;
    return true;
}

/**
 * @brief 检查二元表达式类型
 */
static Stru_TypeCheckResult_t type_checker_check_binary_expression(
    Stru_TypeCheckerInterface_t* type_checker, void* left_type, void* right_type,
    int operator_type)
{
    (void)type_checker;
    (void)left_type;
    (void)right_type;
    (void)operator_type;
    
    // 桩实现：总是返回类型兼容
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_COMPATIBLE;
    result.error_message = NULL;
    result.converted_type = NULL;
    result.requires_explicit_cast = false;
    
    return result;
}

/**
 * @brief 检查一元表达式类型
 */
static Stru_TypeCheckResult_t type_checker_check_unary_expression(
    Stru_TypeCheckerInterface_t* type_checker, void* operand_type, int operator_type)
{
    (void)type_checker;
    (void)operand_type;
    (void)operator_type;
    
    // 桩实现：总是返回类型兼容
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_COMPATIBLE;
    result.error_message = NULL;
    result.converted_type = NULL;
    result.requires_explicit_cast = false;
    
    return result;
}

/**
 * @brief 检查赋值类型
 */
static Stru_TypeCheckResult_t type_checker_check_assignment(
    Stru_TypeCheckerInterface_t* type_checker, void* target_type, void* source_type,
    bool is_compound)
{
    (void)type_checker;
    (void)target_type;
    (void)source_type;
    (void)is_compound;
    
    // 桩实现：总是返回类型兼容
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_COMPATIBLE;
    result.error_message = NULL;
    result.converted_type = NULL;
    result.requires_explicit_cast = false;
    
    return result;
}

/**
 * @brief 检查函数调用类型
 */
static Stru_TypeCheckResult_t type_checker_check_function_call(
    Stru_TypeCheckerInterface_t* type_checker, void* function_type,
    void** arg_types, size_t arg_count)
{
    (void)type_checker;
    (void)function_type;
    (void)arg_types;
    (void)arg_count;
    
    // 桩实现：总是返回类型兼容
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_COMPATIBLE;
    result.error_message = NULL;
    result.converted_type = NULL;
    result.requires_explicit_cast = false;
    
    return result;
}

/**
 * @brief 检查类型转换
 */
static Stru_TypeCheckResult_t type_checker_check_type_cast(
    Stru_TypeCheckerInterface_t* type_checker, void* from_type, void* to_type,
    bool is_explicit)
{
    (void)type_checker;
    (void)from_type;
    (void)to_type;
    (void)is_explicit;
    
    // 桩实现：总是返回类型可转换
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_CONVERTIBLE;
    result.error_message = NULL;
    result.converted_type = to_type;
    result.requires_explicit_cast = true;
    
    return result;
}

/**
 * @brief 获取表达式的推断类型
 */
static void* type_checker_infer_expression_type(Stru_TypeCheckerInterface_t* type_checker,
                                               void* ast_node)
{
    (void)type_checker;
    (void)ast_node;
    
    // 桩实现：返回NULL
    return NULL;
}

/**
 * @brief 检查变量声明类型
 */
static Stru_TypeCheckResult_t type_checker_check_variable_declaration(
    Stru_TypeCheckerInterface_t* type_checker, void* declared_type,
    void* initializer_type)
{
    (void)type_checker;
    (void)declared_type;
    (void)initializer_type;
    
    // 桩实现：总是返回类型兼容
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_COMPATIBLE;
    result.error_message = NULL;
    result.converted_type = NULL;
    result.requires_explicit_cast = false;
    
    return result;
}

/**
 * @brief 销毁类型检查器
 */
static void type_checker_destroy(Stru_TypeCheckerInterface_t* type_checker)
{
    if (type_checker == NULL)
    {
        return;
    }
    
    if (type_checker->private_data != NULL)
    {
        free(type_checker->private_data);
        type_checker->private_data = NULL;
    }
}

/**
 * @brief 创建类型检查器接口实例
 */
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void)
{
    Stru_TypeCheckerInterface_t* interface = 
        (Stru_TypeCheckerInterface_t*)malloc(sizeof(Stru_TypeCheckerInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = type_checker_initialize;
    interface->check_binary_expression = type_checker_check_binary_expression;
    interface->check_unary_expression = type_checker_check_unary_expression;
    interface->check_assignment = type_checker_check_assignment;
    interface->check_function_call = type_checker_check_function_call;
    interface->check_type_cast = type_checker_check_type_cast;
    interface->infer_expression_type = type_checker_infer_expression_type;
    interface->check_variable_declaration = type_checker_check_variable_declaration;
    interface->destroy = type_checker_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁类型检查器接口实例
 */
void F_destroy_type_checker_interface(Stru_TypeCheckerInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}

// ============================================================================
// 语义错误报告接口实现
// ============================================================================

/**
 * @brief 语义错误报告接口私有数据
 */
typedef struct Stru_SemanticErrorReporterData_t
{
    size_t max_errors;                         ///< 最大错误数量限制
    size_t error_count;                        ///< 错误数量
    size_t warning_count;                      ///< 警告数量
    size_t error_capacity;                     ///< 错误数组容量
    Stru_SemanticError_t* errors;              ///< 错误数组
    void (*error_callback)(const Stru_SemanticError_t* error, void* user_data);  ///< 错误回调
    void* callback_user_data;                  ///< 回调用户数据
} Stru_SemanticErrorReporterData_t;

/**
 * @brief 语义错误报告器初始化函数
 */
static bool semantic_error_reporter_initialize(
    Stru_SemanticErrorReporterInterface_t* reporter, size_t max_errors)
{
    if (reporter == NULL || reporter->private_data != NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)malloc(sizeof(Stru_SemanticErrorReporterData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->max_errors = max_errors;
    data->error_count = 0;
    data->warning_count = 0;
    data->error_capacity = (max_errors > 0 && max_errors < 64) ? max_errors : 64;
    data->errors = (Stru_SemanticError_t*)malloc(data->error_capacity * sizeof(Stru_SemanticError_t));
    data->error_callback = NULL;
    data->callback_user_data = NULL;
    
    if (data->errors == NULL)
    {
        free(data);
        return false;
    }
    
    reporter->private_data = data;
    return true;
}

/**
 * @brief 报告语义错误
 */
static bool semantic_error_reporter_report_error(
    Stru_SemanticErrorReporterInterface_t* reporter, const Stru_SemanticError_t* error)
{
    if (reporter == NULL || reporter->private_data == NULL || error == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    
    // 检查错误数量限制
    if (data->max_errors > 0 && data->error_count >= data->max_errors)
    {
        return false;
    }
    
    // 检查是否需要扩容
    if (data->error_count >= data->error_capacity)
    {
        size_t new_capacity = data->error_capacity * 2;
        Stru_SemanticError_t* new_errors = 
            (Stru_SemanticError_t*)realloc(data->errors, new_capacity * sizeof(Stru_SemanticError_t));
        if (new_errors == NULL)
        {
            return false;
        }
        data->errors = new_errors;
        data->error_capacity = new_capacity;
    }
    
    // 添加错误
    data->errors[data->error_count] = *error;
    data->error_count++;
    
    // 调用回调函数
    if (data->error_callback != NULL)
    {
        data->error_callback(error, data->callback_user_data);
    }
    
    return true;
}

/**
 * @brief 报告语义警告
 */
static bool semantic_error_reporter_report_warning(
    Stru_SemanticErrorReporterInterface_t* reporter, const Stru_SemanticError_t* error)
{
    if (reporter == NULL || reporter->private_data == NULL || error == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    
    data->warning_count++;
    
    // 调用回调函数
    if (data->error_callback != NULL)
    {
        data->error_callback(error, data->callback_user_data);
    }
    
    return true;
}

/**
 * @brief 获取错误数量
 */
static size_t semantic_error_reporter_get_error_count(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    return data->error_count;
}

/**
 * @brief 获取警告数量
 */
static size_t semantic_error_reporter_get_warning_count(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    return data->warning_count;
}

/**
 * @brief 获取所有错误
 */
static size_t semantic_error_reporter_get_all_errors(
    Stru_SemanticErrorReporterInterface_t* reporter, Stru_SemanticError_t* errors,
    size_t max_errors)
{
    if (reporter == NULL || reporter->private_data == NULL || errors == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    size_t count = (data->error_count < max_errors) ? data->error_count : max_errors;
    
    for (size_t i = 0; i < count; i++)
    {
        errors[i] = data->errors[i];
    }
    
    return count;
}

/**
 * @brief 获取所有警告
 */
static size_t semantic_error_reporter_get_all_warnings(
    Stru_SemanticErrorReporterInterface_t* reporter, Stru_SemanticError_t* warnings,
    size_t max_warnings)
{
    (void)reporter;
    (void)warnings;
    (void)max_warnings;
    
    // 桩实现：警告不单独存储
    return 0;
}

/**
 * @brief 清空所有错误和警告
 */
static void semantic_error_reporter_clear_all(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    data->error_count = 0;
    data->warning_count = 0;
}

/**
 * @brief 检查是否有错误
 */
static bool semantic_error_reporter_has_errors(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    return data->error_count > 0;
}

/**
 * @brief 检查是否有警告
 */
static bool semantic_error_reporter_has_warnings(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    return data->warning_count > 0;
}

/**
 * @brief 设置错误处理回调
 */
static void semantic_error_reporter_set_error_callback(
    Stru_SemanticErrorReporterInterface_t* reporter,
    void (*callback)(const Stru_SemanticError_t* error, void* user_data),
    void* user_data)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    data->error_callback = callback;
    data->callback_user_data = user_data;
}

/**
 * @brief 销毁错误报告器
 */
static void semantic_error_reporter_destroy(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL)
    {
        return;
    }
    
    if (reporter->private_data != NULL)
    {
        Stru_SemanticErrorReporterData_t* data = 
            (Stru_SemanticErrorReporterData_t*)reporter->private_data;
        
        if (data->errors != NULL)
        {
            free(data->errors);
        }
        
        free(data);
        reporter->private_data = NULL;
    }
}

/**
 * @brief 创建语义错误报告器接口实例
 */
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void)
{
    Stru_SemanticErrorReporterInterface_t* interface = 
        (Stru_SemanticErrorReporterInterface_t*)malloc(
            sizeof(Stru_SemanticErrorReporterInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = semantic_error_reporter_initialize;
    interface->report_error = semantic_error_reporter_report_error;
    interface->report_warning = semantic_error_reporter_report_warning;
    interface->get_error_count = semantic_error_reporter_get_error_count;
    interface->get_warning_count = semantic_error_reporter_get_warning_count;
    interface->get_all_errors = semantic_error_reporter_get_all_errors;
    interface->get_all_warnings = semantic_error_reporter_get_all_warnings;
    interface->clear_all = semantic_error_reporter_clear_all;
    interface->has_errors = semantic_error_reporter_has_errors;
    interface->has_warnings = semantic_error_reporter_has_warnings;
    interface->set_error_callback = semantic_error_reporter_set_error_callback;
    interface->destroy = semantic_error_reporter_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁语义错误报告器接口实例
 */
void F_destroy_semantic_error_reporter_interface(
    Stru_SemanticErrorReporterInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}

// ============================================================================
// 主语义分析器接口实现
// ============================================================================

/**
 * @brief 语义分析器接口私有数据
 */
typedef struct Stru_SemanticAnalyzerData_t
{
    Stru_SymbolTableInterface_t* symbol_table;          ///< 符号表接口
    Stru_ScopeManagerInterface_t* scope_manager;        ///< 作用域管理器接口
    Stru_TypeCheckerInterface_t* type_checker;          ///< 类型检查器接口
    Stru_SemanticErrorReporterInterface_t* error_reporter;  ///< 错误报告器接口
} Stru_SemanticAnalyzerData_t;

/**
 * @brief 语义分析器初始化函数
 */
static bool semantic_analyzer_initialize(Stru_SemanticAnalyzerInterface_t* analyzer,
                                        Stru_SymbolTableInterface_t* symbol_table,
                                        Stru_ScopeManagerInterface_t* scope_manager,
                                        Stru_TypeCheckerInterface_t* type_checker,
                                        Stru_SemanticErrorReporterInterface_t* error_reporter)
{
    if (analyzer == NULL || analyzer->private_data != NULL)
    {
        return false;
    }
    
    Stru_SemanticAnalyzerData_t* data = 
        (Stru_SemanticAnalyzerData_t*)malloc(sizeof(Stru_SemanticAnalyzerData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->symbol_table = symbol_table;
    data->scope_manager = scope_manager;
    data->type_checker = type_checker;
    data->error_reporter = error_reporter;
    
    analyzer->private_data = data;
    return true;
}

/**
 * @brief 分析AST节点
 */
static bool semantic_analyzer_analyze_ast(Stru_SemanticAnalyzerInterface_t* analyzer,
                                         void* ast_node)
{
    (void)analyzer;
    (void)ast_node;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 分析单个声明
 */
static bool semantic_analyzer_analyze_declaration(Stru_SemanticAnalyzerInterface_t* analyzer,
                                                 void* declaration_node)
{
    (void)analyzer;
    (void)declaration_node;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 分析单个语句
 */
static bool semantic_analyzer_analyze_statement(Stru_SemanticAnalyzerInterface_t* analyzer,
                                               void* statement_node)
{
    (void)analyzer;
    (void)statement_node;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 分析单个表达式
 */
static void* semantic_analyzer_analyze_expression(Stru_SemanticAnalyzerInterface_t* analyzer,
                                                 void* expression_node)
{
    (void)analyzer;
    (void)expression_node;
    
    // 桩实现：返回NULL
    return NULL;
}

/**
 * @brief 获取符号表接口
 */
static Stru_SymbolTableInterface_t* semantic_analyzer_get_symbol_table(
    Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    return data->symbol_table;
}

/**
 * @brief 获取作用域管理器接口
 */
static Stru_ScopeManagerInterface_t* semantic_analyzer_get_scope_manager(
    Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    return data->scope_manager;
}

/**
 * @brief 获取类型检查器接口
 */
static Stru_TypeCheckerInterface_t* semantic_analyzer_get_type_checker(
    Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    return data->type_checker;
}

/**
 * @brief 获取错误报告器接口
 */
static Stru_SemanticErrorReporterInterface_t* semantic_analyzer_get_error_reporter(
    Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    return data->error_reporter;
}

/**
 * @brief 重置语义分析器状态
 */
static void semantic_analyzer_reset(Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    
    // 重置所有组件
    if (data->symbol_table != NULL)
    {
        data->symbol_table->clear(data->symbol_table);
    }
    
    if (data->scope_manager != NULL)
    {
        // 重置作用域管理器到全局作用域
        while (data->scope_manager->get_scope_depth(data->scope_manager) > 1)
        {
            data->scope_manager->exit_scope(data->scope_manager);
        }
    }
    
    if (data->error_reporter != NULL)
    {
        data->error_reporter->clear_all(data->error_reporter);
    }
}

/**
 * @brief 销毁语义分析器
 */
static void semantic_analyzer_destroy(Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL)
    {
        return;
    }
    
    if (analyzer->private_data != NULL)
    {
        Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
        
        // 注意：这里不销毁组件接口，因为它们可能被其他部分共享
        // 组件的销毁由创建者负责
        
        free(data);
        analyzer->private_data = NULL;
    }
}

/**
 * @brief 创建语义分析器接口实例
 */
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(void)
{
    Stru_SemanticAnalyzerInterface_t* interface = 
        (Stru_SemanticAnalyzerInterface_t*)malloc(sizeof(Stru_SemanticAnalyzerInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = semantic_analyzer_initialize;
    interface->analyze_ast = semantic_analyzer_analyze_ast;
    interface->analyze_declaration = semantic_analyzer_analyze_declaration;
    interface->analyze_statement = semantic_analyzer_analyze_statement;
    interface->analyze_expression = semantic_analyzer_analyze_expression;
    interface->get_symbol_table = semantic_analyzer_get_symbol_table;
    interface->get_scope_manager = semantic_analyzer_get_scope_manager;
    interface->get_type_checker = semantic_analyzer_get_type_checker;
    interface->get_error_reporter = semantic_analyzer_get_error_reporter;
    interface->reset = semantic_analyzer_reset;
    interface->destroy = semantic_analyzer_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁语义分析器接口实例
 */
void F_destroy_semantic_analyzer_interface(Stru_SemanticAnalyzerInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}
