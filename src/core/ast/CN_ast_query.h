/**
 * @file CN_ast_query.h
 * @brief CN_Language AST查询器实现头文件
 * 
 * 实现AST查询接口，提供节点类型统计、属性查询等功能。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_AST_QUERY_H
#define CN_AST_QUERY_H

#include "CN_ast_interface.h"
#include "../../infrastructure/memory/CN_memory_interface.h"
#include "../../infrastructure/containers/array/CN_dynamic_array.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 前向声明
// ============================================================================

/**
 * @brief AST查询器内部数据结构
 */
typedef struct Stru_AstQueryData_t Stru_AstQueryData_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建AST查询器内部实例
 * 
 * @return Stru_AstQueryData_t* AST查询器内部实例，失败返回NULL
 */
Stru_AstQueryData_t* F_create_ast_query_data(void);

/**
 * @brief 销毁AST查询器内部实例
 * 
 * @param data 要销毁的内部实例
 */
void F_destroy_ast_query_data(Stru_AstQueryData_t* data);

/**
 * @brief 获取AST查询器接口
 * 
 * @param data AST查询器内部实例
 * @return Stru_AstQueryInterface_t* AST查询器接口，失败返回NULL
 */
Stru_AstQueryInterface_t* F_get_ast_query_interface(Stru_AstQueryData_t* data);

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 统计节点类型递归实现
 * 
 * @param node 当前节点
 * @param type_counts 类型统计数组
 * @param max_types 最大类型数量
 */
void F_count_node_types_recursive(Stru_AstNodeInterface_t* node,
                                 size_t* type_counts,
                                 size_t max_types);

/**
 * @brief 查找具有特定属性的节点递归实现
 * 
 * @param node 当前节点
 * @param key 属性键
 * @param value 属性值
 * @param matches 匹配节点数组
 * @param match_count 匹配节点数量指针
 * @param max_matches 最大匹配数量
 */
void F_find_nodes_with_attribute_recursive(Stru_AstNodeInterface_t* node,
                                          const char* key,
                                          void* value,
                                          Stru_AstNodeInterface_t** matches,
                                          size_t* match_count,
                                          size_t max_matches);

/**
 * @brief 获取节点深度递归实现
 * 
 * @param node 节点
 * @return size_t 节点深度（根节点为0）
 */
size_t F_get_node_depth_recursive(Stru_AstNodeInterface_t* node);

/**
 * @brief 获取子树大小递归实现
 * 
 * @param node 节点
 * @return size_t 子树节点数量（包括自身）
 */
size_t F_get_subtree_size_recursive(Stru_AstNodeInterface_t* node);

/**
 * @brief 验证AST结构递归实现
 * 
 * @param node 当前节点
 * @return bool AST结构有效返回true，否则返回false
 */
bool F_validate_ast_structure_recursive(Stru_AstNodeInterface_t* node);

#ifdef __cplusplus
}
#endif

#endif /* CN_AST_QUERY_H */
