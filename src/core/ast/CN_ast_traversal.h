/**
 * @file CN_ast_traversal.h
 * @brief CN_Language AST遍历器实现头文件
 * 
 * 实现AST遍历接口，提供深度优先、广度优先等遍历功能。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_AST_TRAVERSAL_H
#define CN_AST_TRAVERSAL_H

#include "CN_ast_interface.h"
#include "../../infrastructure/memory/CN_memory_interface.h"
#include "../../infrastructure/containers/queue/CN_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 前向声明
// ============================================================================

/**
 * @brief AST遍历器内部数据结构
 */
typedef struct Stru_AstTraversalData_t Stru_AstTraversalData_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建AST遍历器内部实例
 * 
 * @return Stru_AstTraversalData_t* AST遍历器内部实例，失败返回NULL
 */
Stru_AstTraversalData_t* F_create_ast_traversal_data(void);

/**
 * @brief 销毁AST遍历器内部实例
 * 
 * @param data 要销毁的内部实例
 */
void F_destroy_ast_traversal_data(Stru_AstTraversalData_t* data);

/**
 * @brief 获取AST遍历器接口
 * 
 * @param data AST遍历器内部实例
 * @return Stru_AstTraversalInterface_t* AST遍历器接口，失败返回NULL
 */
Stru_AstTraversalInterface_t* F_get_ast_traversal_interface(Stru_AstTraversalData_t* data);

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 深度优先遍历递归实现
 * 
 * @param node 当前节点
 * @param pre_order 前序遍历回调函数
 * @param in_order 中序遍历回调函数
 * @param post_order 后序遍历回调函数
 * @param context 上下文参数
 */
void F_depth_first_traversal_recursive(Stru_AstNodeInterface_t* node,
                                      void (*pre_order)(Stru_AstNodeInterface_t*, void*),
                                      void (*in_order)(Stru_AstNodeInterface_t*, void*),
                                      void (*post_order)(Stru_AstNodeInterface_t*, void*),
                                      void* context);

/**
 * @brief 广度优先遍历实现
 * 
 * @param root 根节点
 * @param visitor 访问者回调函数
 * @param context 上下文参数
 * @param queue 队列（用于遍历）
 */
void F_breadth_first_traversal_impl(Stru_AstNodeInterface_t* root,
                                   void (*visitor)(Stru_AstNodeInterface_t*, void*),
                                   void* context,
                                   Stru_Queue_t* queue);

/**
 * @brief 查找节点递归实现
 * 
 * @param node 当前节点
 * @param predicate 谓词函数
 * @param context 上下文参数
 * @param matches 匹配节点数组
 * @param match_count 匹配节点数量指针
 * @param max_matches 最大匹配数量
 */
void F_find_nodes_recursive(Stru_AstNodeInterface_t* node,
                           bool (*predicate)(Stru_AstNodeInterface_t*, void*),
                           void* context,
                           Stru_AstNodeInterface_t** matches,
                           size_t* match_count,
                           size_t max_matches);

#ifdef __cplusplus
}
#endif

#endif /* CN_AST_TRAVERSAL_H */
