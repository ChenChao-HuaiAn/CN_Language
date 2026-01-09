/**
 * @file CN_symbol_table.c
 * @brief 符号表模块实现
 
 * 本文件实现了符号表模块的功能，包括符号的插入、查找、删除和遍历等操作。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_symbol_table.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 符号表接口私有数据结构
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

// ============================================================================
// 符号表接口实现函数
// ============================================================================

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

// ============================================================================
// 工厂函数实现
// ============================================================================

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
