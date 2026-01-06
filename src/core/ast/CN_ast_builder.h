/**
 * @file CN_ast_builder.h
 * @brief CN_Language AST构建器实现
 * 
 * 实现AST构建器接口的具体功能，提供AST节点的创建和构建。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_AST_BUILDER_H
#define CN_AST_BUILDER_H

#include "CN_ast_interface.h"

// ============================================================================
// 内部数据结构定义
// ============================================================================

/**
 * @brief AST构建器内部数据结构
 * 
 * 存储AST构建器的内部状态和配置信息。
 */
typedef struct Stru_AstBuilderInternalData_t
{
    Stru_AstNodeInterface_t* current_node;    ///< 当前构建的节点
    Stru_AstNodeInterface_t* root_node;       ///< 根节点
    size_t node_count;                        ///< 创建的节点数量
    void* builder_context;                    ///< 构建器上下文
} Stru_AstBuilderInternalData_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建AST构建器内部数据
 * 
 * 分配并初始化AST构建器的内部数据结构。
 * 
 * @return Stru_AstBuilderInternalData_t* 新创建的内部数据指针，失败返回NULL
 */
Stru_AstBuilderInternalData_t* F_create_ast_builder_internal_data(void);

/**
 * @brief 销毁AST构建器内部数据
 * 
 * 释放AST构建器内部数据结构占用的内存。
 * 
 * @param internal_data 要销毁的内部数据指针
 */
void F_destroy_ast_builder_internal_data(Stru_AstBuilderInternalData_t* internal_data);

/**
 * @brief 创建AST构建器接口实例
 * 
 * 创建并初始化AST构建器接口实例。
 * 
 * @return Stru_AstBuilderInterface_t* AST构建器接口实例，失败返回NULL
 */
Stru_AstBuilderInterface_t* F_create_ast_builder_interface_impl(void);

/**
 * @brief 从内部数据获取AST构建器接口
 * 
 * 根据内部数据创建AST构建器接口实例。
 * 
 * @param internal_data 内部数据指针
 * @return Stru_AstBuilderInterface_t* AST构建器接口实例，失败返回NULL
 */
Stru_AstBuilderInterface_t* F_get_ast_builder_interface_from_internal_data(
    Stru_AstBuilderInternalData_t* internal_data);

/**
 * @brief 从AST构建器接口获取内部数据
 * 
 * 从AST构建器接口实例获取内部数据指针。
 * 
 * @param builder_interface AST构建器接口实例
 * @return Stru_AstBuilderInternalData_t* 内部数据指针，失败返回NULL
 */
Stru_AstBuilderInternalData_t* F_get_internal_data_from_ast_builder_interface(
    const Stru_AstBuilderInterface_t* builder_interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_AST_BUILDER_H */
