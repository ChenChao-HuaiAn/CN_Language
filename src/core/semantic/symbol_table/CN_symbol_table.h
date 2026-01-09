/**
 * @file CN_symbol_table.h
 * @brief 符号表模块头文件
 
 * 本文件定义了符号表模块的接口和数据结构，负责管理程序中的符号信息，
 * 包括符号的插入、查找、删除和遍历等功能。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_SYMBOL_TABLE_H
#define CN_SYMBOL_TABLE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_SymbolTableInterface_t Stru_SymbolTableInterface_t;
typedef struct Stru_SymbolInfo_t Stru_SymbolInfo_t;

// ============================================================================
// 符号类型枚举
// ============================================================================

/**
 * @brief 符号类型枚举
 */
typedef enum Eum_SymbolType
{
    Eum_SYMBOL_VARIABLE,        ///< 变量符号
    Eum_SYMBOL_FUNCTION,        ///< 函数符号
    Eum_SYMBOL_STRUCT,          ///< 结构体符号
    Eum_SYMBOL_ENUM,            ///< 枚举符号
    Eum_SYMBOL_CONSTANT,        ///< 常量符号
    Eum_SYMBOL_PARAMETER,       ///< 参数符号
    Eum_SYMBOL_TYPE_ALIAS,      ///< 类型别名符号
    Eum_SYMBOL_MODULE,          ///< 模块符号
    Eum_SYMBOL_IMPORT,          ///< 导入符号
    Eum_SYMBOL_TEMPLATE         ///< 模板符号
} Eum_SymbolType;

// ============================================================================
// 符号信息结构体
// ============================================================================

/**
 * @brief 符号信息结构体
 */
struct Stru_SymbolInfo_t
{
    const char* name;           ///< 符号名称
    Eum_SymbolType type;        ///< 符号类型
    void* type_info;            ///< 类型信息（具体类型由实现决定）
    size_t line;                ///< 定义行号
    size_t column;              ///< 定义列号
    bool is_exported;           ///< 是否导出（对外可见）
    bool is_initialized;        ///< 是否已初始化
    bool is_constant;           ///< 是否为常量
    void* extra_data;           ///< 额外数据（实现特定）
};

// ============================================================================
// 符号表接口
// ============================================================================

/**
 * @brief 符号表接口
 
 * 管理程序中的符号信息，支持符号的插入、查找、作用域管理等操作。
 */
struct Stru_SymbolTableInterface_t
{
    /**
     * @brief 初始化符号表
     * 
     * @param symbol_table 符号表实例
     * @param parent_scope 父作用域符号表（可为NULL）
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_SymbolTableInterface_t* symbol_table, 
                      Stru_SymbolTableInterface_t* parent_scope);
    
    /**
     * @brief 插入符号
     * 
     * @param symbol_table 符号表实例
     * @param symbol 符号信息
     * @return bool 插入成功返回true，否则返回false（如重复定义）
     */
    bool (*insert_symbol)(Stru_SymbolTableInterface_t* symbol_table, 
                         const Stru_SymbolInfo_t* symbol);
    
    /**
     * @brief 查找符号
     * 
     * @param symbol_table 符号表实例
     * @param name 符号名称
     * @param search_parent 是否在父作用域中查找
     * @return Stru_SymbolInfo_t* 符号信息指针，未找到返回NULL
     */
    Stru_SymbolInfo_t* (*lookup_symbol)(Stru_SymbolTableInterface_t* symbol_table,
                                       const char* name, bool search_parent);
    
    /**
     * @brief 删除符号
     * 
     * @param symbol_table 符号表实例
     * @param name 符号名称
     * @return bool 删除成功返回true，否则返回false
     */
    bool (*remove_symbol)(Stru_SymbolTableInterface_t* symbol_table,
                         const char* name);
    
    /**
     * @brief 获取符号数量
     * 
     * @param symbol_table 符号表实例
     * @return size_t 符号数量
     */
    size_t (*get_symbol_count)(Stru_SymbolTableInterface_t* symbol_table);
    
    /**
     * @brief 获取所有符号
     * 
     * @param symbol_table 符号表实例
     * @param symbols 输出参数：符号数组
     * @param max_symbols 最大符号数量
     * @return size_t 实际获取的符号数量
     */
    size_t (*get_all_symbols)(Stru_SymbolTableInterface_t* symbol_table,
                             Stru_SymbolInfo_t* symbols, size_t max_symbols);
    
    /**
     * @brief 清空符号表
     * 
     * @param symbol_table 符号表实例
     */
    void (*clear)(Stru_SymbolTableInterface_t* symbol_table);
    
    /**
     * @brief 销毁符号表
     * 
     * @param symbol_table 符号表实例
     */
    void (*destroy)(Stru_SymbolTableInterface_t* symbol_table);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建符号表接口实例
 * 
 * @return Stru_SymbolTableInterface_t* 符号表接口实例，失败返回NULL
 */
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void);

/**
 * @brief 销毁符号表接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_symbol_table_interface(Stru_SymbolTableInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_SYMBOL_TABLE_H */
