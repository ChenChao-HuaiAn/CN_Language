/**
 * @file CN_symbol_table_optimized.h
 * @brief 优化版符号表模块头文件（使用哈希表）
 
 * 本文件定义了优化版符号表模块的接口和函数，使用哈希表提高查找性能，
 * 支持缓存机制和批量操作优化。
 * 遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_SYMBOL_TABLE_OPTIMIZED_H
#define CN_SYMBOL_TABLE_OPTIMIZED_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_SymbolTableInterface_t Stru_SymbolTableInterface_t;
typedef struct Stru_SymbolInfo_t Stru_SymbolInfo_t;

// ============================================================================
// 工厂函数声明（优化版）
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建优化版符号表接口实例
 * 
 * 此函数创建并初始化一个优化版的符号表接口实例，使用哈希表实现提高性能。
 * 优化版符号表特点：
 * 1. 使用FNV-1a哈希算法
 * 2. 开放寻址法解决冲突
 * 3. 动态扩容机制
 * 4. 缓存最近访问的符号
 * 5. 惰性删除策略
 * 
 * @return Stru_SymbolTableInterface_t* 初始化完成的符号表接口，失败返回NULL
 */
Stru_SymbolTableInterface_t* F_create_symbol_table_interface_optimized(void);

/**
 * @brief 销毁优化版符号表接口实例
 * 
 * 此函数销毁优化版符号表接口实例，释放所有相关资源。
 * 
 * @param interface 要销毁的符号表接口实例
 */
void F_destroy_symbol_table_interface_optimized(Stru_SymbolTableInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_SYMBOL_TABLE_OPTIMIZED_H */
