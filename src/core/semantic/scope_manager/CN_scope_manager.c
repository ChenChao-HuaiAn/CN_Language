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
 * @brief 作用域管理器接口私有数据
 */
typedef struct Stru_ScopeManagerData_t
{
    Stru_SymbolTableInterface_t* global_scope;  ///< 全局作用域符号表
    size_t scope_depth;                         ///< 当前作用域深度
    size_t scope_capacity;                      ///< 作用域栈容量
    Stru_SymbolTableInterface_t** scope_stack;  ///< 作用域栈
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
