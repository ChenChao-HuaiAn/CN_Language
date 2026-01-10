/**
 * @file CN_scope_manager.c
 * @brief 作用域管理器模块实现
 
 * 本文件实现了作用域管理器模块的功能，包括作用域的创建、进入、退出和查询等操作。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_scope_manager.h"
#include "../symbol_table/CN_symbol_table.h"
#include <stdlib.h>

// ============================================================================
// 作用域管理器接口私有数据结构
// ============================================================================

/**
 * @brief 作用域生命周期信息
 */
typedef struct Stru_ScopeLifecycleInfo_t
{
    size_t start_line;          ///< 作用域开始行号
    size_t start_column;        ///< 作用域开始列号
    size_t end_line;            ///< 作用域结束行号
    size_t end_column;          ///< 作用域结束列号
    bool is_active;             ///< 作用域是否活跃
} Stru_ScopeLifecycleInfo_t;

/**
 * @brief 作用域管理器接口私有数据
 */
typedef struct Stru_ScopeManagerData_t
{
    Stru_SymbolTableInterface_t* global_scope;  ///< 全局作用域符号表
    size_t scope_depth;                         ///< 当前作用域深度
    size_t scope_capacity;                      ///< 作用域栈容量
    Stru_SymbolTableInterface_t** scope_stack;  ///< 作用域栈
    Eum_ScopeType* scope_types;                 ///< 作用域类型数组
    Stru_ScopeLifecycleInfo_t* lifecycle_info;  ///< 作用域生命周期信息数组
} Stru_ScopeManagerData_t;

// ============================================================================
// 作用域管理器接口实现函数
// ============================================================================

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
 * @brief 验证作用域嵌套规则
 */
static bool scope_manager_validate_scope_nesting(
    Stru_ScopeManagerInterface_t* scope_manager, Eum_ScopeType new_scope_type)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL)
    {
        return false;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    
    // 如果当前没有作用域，任何类型的作用域都可以创建
    if (data->scope_depth == 0)
    {
        return true;
    }
    
    // 获取当前作用域类型（这里简化处理，实际应该从scope_types数组中获取）
    // 这里我们假设当前作用域类型为函数作用域（实际实现中应该存储作用域类型）
    Eum_ScopeType current_scope_type = Eum_SCOPE_FUNCTION;
    
    // 定义作用域嵌套规则
    switch (current_scope_type)
    {
        case Eum_SCOPE_GLOBAL:
            // 全局作用域可以包含函数、结构体等
            return (new_scope_type == Eum_SCOPE_FUNCTION || 
                    new_scope_type == Eum_SCOPE_STRUCT ||
                    new_scope_type == Eum_SCOPE_BLOCK);
            
        case Eum_SCOPE_FUNCTION:
            // 函数作用域可以包含块、循环、条件、try-catch作用域
            return (new_scope_type == Eum_SCOPE_BLOCK ||
                    new_scope_type == Eum_SCOPE_LOOP ||
                    new_scope_type == Eum_SCOPE_CONDITIONAL ||
                    new_scope_type == Eum_SCOPE_TRY_CATCH);
            
        case Eum_SCOPE_BLOCK:
            // 块作用域可以包含嵌套块、循环、条件作用域
            return (new_scope_type == Eum_SCOPE_BLOCK ||
                    new_scope_type == Eum_SCOPE_LOOP ||
                    new_scope_type == Eum_SCOPE_CONDITIONAL);
            
        case Eum_SCOPE_LOOP:
            // 循环作用域可以包含块、条件作用域
            return (new_scope_type == Eum_SCOPE_BLOCK ||
                    new_scope_type == Eum_SCOPE_CONDITIONAL);
            
        case Eum_SCOPE_CONDITIONAL:
            // 条件作用域可以包含块作用域
            return (new_scope_type == Eum_SCOPE_BLOCK);
            
        case Eum_SCOPE_STRUCT:
            // 结构体作用域不能包含其他作用域（只能包含成员声明）
            return false;
            
        case Eum_SCOPE_TRY_CATCH:
            // try-catch作用域可以包含块作用域
            return (new_scope_type == Eum_SCOPE_BLOCK);
            
        default:
            return false;
    }
}

/**
 * @brief 获取作用域生命周期信息
 */
static bool scope_manager_get_scope_lifecycle_info(
    Stru_ScopeManagerInterface_t* scope_manager, Stru_ScopeInfo_t* scope_info)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL || scope_info == NULL)
    {
        return false;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    
    // 如果当前没有作用域，返回false
    if (data->scope_depth == 0)
    {
        return false;
    }
    
    // 这里简化实现，实际应该从存储的信息中获取
    scope_info->type = Eum_SCOPE_FUNCTION;  // 假设当前作用域类型
    scope_info->depth = data->scope_depth;
    scope_info->name = "current_scope";
    scope_info->symbol_table = data->scope_stack[data->scope_depth - 1];
    scope_info->extra_data = NULL;
    
    return true;
}

/**
 * @brief 检查变量生命周期
 */
static bool scope_manager_check_variable_lifetime(
    Stru_ScopeManagerInterface_t* scope_manager,
    const char* variable_name,
    size_t usage_line, size_t usage_column)
{
    if (scope_manager == NULL || scope_manager->private_data == NULL || variable_name == NULL)
    {
        return false;
    }
    
    Stru_ScopeManagerData_t* data = (Stru_ScopeManagerData_t*)scope_manager->private_data;
    
    // 查找变量
    Stru_SymbolInfo_t* symbol = scope_manager_lookup_symbol_in_scope_chain(
        scope_manager, variable_name);
    
    if (symbol == NULL)
    {
        // 变量未声明
        return false;
    }
    
    // 这里简化实现，实际应该检查：
    // 1. 变量是否在使用前已声明（通过声明位置和引用位置比较）
    // 2. 变量是否在作用域外被引用
    // 3. 变量是否在声明前被使用
    
    // 假设变量生命周期有效
    (void)usage_line;
    (void)usage_column;
    
    return true;
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
        
        if (data->scope_types != NULL)
        {
            free(data->scope_types);
        }
        
        if (data->lifecycle_info != NULL)
        {
            free(data->lifecycle_info);
        }
        
        free(data);
        scope_manager->private_data = NULL;
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

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
    interface->validate_scope_nesting = scope_manager_validate_scope_nesting;
    interface->get_scope_lifecycle_info = scope_manager_get_scope_lifecycle_info;
    interface->check_variable_lifetime = scope_manager_check_variable_lifetime;
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
