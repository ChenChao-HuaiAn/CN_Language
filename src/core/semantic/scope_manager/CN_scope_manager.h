/**
 * @file CN_scope_manager.h
 * @brief 作用域管理器模块头文件
 
 * 本文件定义了作用域管理器模块的接口和数据结构，负责管理程序中的作用域层次结构，
 * 包括作用域的创建、进入、退出和查询等功能。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_SCOPE_MANAGER_H
#define CN_SCOPE_MANAGER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_ScopeManagerInterface_t Stru_ScopeManagerInterface_t;
typedef struct Stru_SymbolTableInterface_t Stru_SymbolTableInterface_t;
typedef struct Stru_ScopeInfo_t Stru_ScopeInfo_t;
typedef struct Stru_SymbolInfo_t Stru_SymbolInfo_t;

// ============================================================================
// 作用域类型枚举
// ============================================================================

/**
 * @brief 作用域类型枚举
 */
typedef enum Eum_ScopeType
{
    Eum_SCOPE_GLOBAL,           ///< 全局作用域
    Eum_SCOPE_FUNCTION,         ///< 函数作用域
    Eum_SCOPE_BLOCK,            ///< 块作用域
    Eum_SCOPE_STRUCT,           ///< 结构体作用域
    Eum_SCOPE_LOOP,             ///< 循环作用域
    Eum_SCOPE_CONDITIONAL,      ///< 条件作用域
    Eum_SCOPE_TRY_CATCH         ///< try-catch作用域
} Eum_ScopeType;

// ============================================================================
// 作用域信息结构体
// ============================================================================

/**
 * @brief 作用域信息结构体
 */
struct Stru_ScopeInfo_t
{
    Eum_ScopeType type;         ///< 作用域类型
    size_t depth;               ///< 作用域深度（0为全局作用域）
    const char* name;           ///< 作用域名称（如函数名）
    Stru_SymbolTableInterface_t* symbol_table;  ///< 关联的符号表
    void* extra_data;           ///< 额外数据（实现特定）
};

// ============================================================================
// 作用域管理器接口
// ============================================================================

/**
 * @brief 作用域管理器接口
 
 * 管理程序中的作用域层次结构，支持作用域的创建、进入、退出等操作。
 */
struct Stru_ScopeManagerInterface_t
{
    /**
     * @brief 初始化作用域管理器
     * 
     * @param scope_manager 作用域管理器实例
     * @param global_symbol_table 全局符号表
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ScopeManagerInterface_t* scope_manager,
                      Stru_SymbolTableInterface_t* global_symbol_table);
    
    /**
     * @brief 创建新作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @param type 作用域类型
     * @param name 作用域名称（可选）
     * @return Stru_SymbolTableInterface_t* 新作用域的符号表，失败返回NULL
     */
    Stru_SymbolTableInterface_t* (*create_scope)(Stru_ScopeManagerInterface_t* scope_manager,
                                                Eum_ScopeType type, const char* name);
    
    /**
     * @brief 进入作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @param symbol_table 要进入的作用域符号表
     * @return bool 进入成功返回true，否则返回false
     */
    bool (*enter_scope)(Stru_ScopeManagerInterface_t* scope_manager,
                       Stru_SymbolTableInterface_t* symbol_table);
    
    /**
     * @brief 退出当前作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @return Stru_SymbolTableInterface_t* 退出后的当前作用域符号表
     */
    Stru_SymbolTableInterface_t* (*exit_scope)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 获取当前作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @return Stru_SymbolTableInterface_t* 当前作用域符号表
     */
    Stru_SymbolTableInterface_t* (*get_current_scope)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 获取全局作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @return Stru_SymbolTableInterface_t* 全局作用域符号表
     */
    Stru_SymbolTableInterface_t* (*get_global_scope)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 获取作用域深度
     * 
     * @param scope_manager 作用域管理器实例
     * @return size_t 当前作用域深度
     */
    size_t (*get_scope_depth)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 查找符号（在当前作用域链中）
     * 
     * @param scope_manager 作用域管理器实例
     * @param name 符号名称
     * @return Stru_SymbolInfo_t* 符号信息指针，未找到返回NULL
     */
    Stru_SymbolInfo_t* (*lookup_symbol_in_scope_chain)(Stru_ScopeManagerInterface_t* scope_manager,
                                                      const char* name);
    
    /**
     * @brief 销毁作用域管理器
     * 
     * @param scope_manager 作用域管理器实例
     */
    void (*destroy)(Stru_ScopeManagerInterface_t* scope_manager);
    
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
 * @brief 创建作用域管理器接口实例
 * 
 * @return Stru_ScopeManagerInterface_t* 作用域管理器接口实例，失败返回NULL
 */
Stru_ScopeManagerInterface_t* F_create_scope_manager_interface(void);

/**
 * @brief 销毁作用域管理器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_scope_manager_interface(Stru_ScopeManagerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_SCOPE_MANAGER_H */
