/**
 * @file CN_ast_traversal.c
 * @brief CN_Language AST遍历器实现
 * 
 * 实现AST遍历接口的具体功能，提供深度优先、广度优先等遍历功能。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_ast_traversal.h"
#include "../../infrastructure/utils/CN_utils_interface.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// 内部数据结构定义
// ============================================================================

/**
 * @brief AST遍历器内部数据结构
 */
struct Stru_AstTraversalData_t
{
    Stru_AstTraversalInterface_t interface;  ///< 遍历器接口
    size_t match_count;                      ///< 匹配节点数量
    Stru_AstNodeInterface_t** matches;       ///< 匹配节点数组
    size_t matches_capacity;                 ///< 匹配节点数组容量
};

// ============================================================================
// 静态函数声明
// ============================================================================

static bool initialize_impl(Stru_AstTraversalInterface_t* traversal);
static void depth_first_traversal_impl(Stru_AstTraversalInterface_t* traversal,
                                      Stru_AstNodeInterface_t* root,
                                      void (*pre_order)(Stru_AstNodeInterface_t*, void*),
                                      void (*in_order)(Stru_AstNodeInterface_t*, void*),
                                      void (*post_order)(Stru_AstNodeInterface_t*, void*),
                                      void* context);
static void breadth_first_traversal_impl(Stru_AstTraversalInterface_t* traversal,
                                        Stru_AstNodeInterface_t* root,
                                        void (*visitor)(Stru_AstNodeInterface_t*, void*),
                                        void* context);
static Stru_AstNodeInterface_t** find_nodes_impl(Stru_AstTraversalInterface_t* traversal,
                                                Stru_AstNodeInterface_t* root,
                                                bool (*predicate)(Stru_AstNodeInterface_t*, void*),
                                                void* context);
static size_t get_match_count_impl(Stru_AstTraversalInterface_t* traversal);
static void destroy_impl(Stru_AstTraversalInterface_t* traversal);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 扩展匹配节点数组
 */
static bool expand_matches_array(Stru_AstTraversalData_t* data)
{
    if (!data) return false;
    
    size_t new_capacity = data->matches_capacity == 0 ? 16 : data->matches_capacity * 2;
    Stru_AstNodeInterface_t** new_matches = 
        (Stru_AstNodeInterface_t**)cn_realloc(data->matches, 
                                             new_capacity * sizeof(Stru_AstNodeInterface_t*));
    if (!new_matches) return false;
    
    data->matches = new_matches;
    data->matches_capacity = new_capacity;
    return true;
}

/**
 * @brief 添加匹配节点
 */
static bool add_match(Stru_AstTraversalData_t* data, Stru_AstNodeInterface_t* node)
{
    if (!data || !node) return false;
    
    // 检查是否需要扩展数组
    if (data->match_count >= data->matches_capacity) {
        if (!expand_matches_array(data)) {
            return false;
        }
    }
    
    data->matches[data->match_count] = node;
    data->match_count++;
    return true;
}

/**
 * @brief 清空匹配节点数组
 */
static void clear_matches(Stru_AstTraversalData_t* data)
{
    if (!data) return;
    
    data->match_count = 0;
}

// ============================================================================
// 工厂函数实现
// ============================================================================

Stru_AstTraversalData_t* F_create_ast_traversal_data(void)
{
    Stru_AstTraversalData_t* data = 
        (Stru_AstTraversalData_t*)cn_malloc(sizeof(Stru_AstTraversalData_t));
    if (!data) return NULL;
    
    // 初始化字段
    memset(data, 0, sizeof(Stru_AstTraversalData_t));
    
    // 设置接口函数指针
    data->interface.initialize = initialize_impl;
    data->interface.depth_first_traversal = depth_first_traversal_impl;
    data->interface.breadth_first_traversal = breadth_first_traversal_impl;
    data->interface.find_nodes = find_nodes_impl;
    data->interface.get_match_count = get_match_count_impl;
    data->interface.destroy = destroy_impl;
    data->interface.private_data = data;
    
    return data;
}

void F_destroy_ast_traversal_data(Stru_AstTraversalData_t* data)
{
    if (!data) return;
    
    // 释放匹配节点数组
    if (data->matches) {
        cn_free(data->matches);
    }
    
    cn_free(data);
}

Stru_AstTraversalInterface_t* F_get_ast_traversal_interface(Stru_AstTraversalData_t* data)
{
    if (!data) return NULL;
    return &data->interface;
}

// ============================================================================
// 接口函数实现
// ============================================================================

static bool initialize_impl(Stru_AstTraversalInterface_t* traversal)
{
    if (!traversal) return false;
    
    Stru_AstTraversalData_t* data = 
        (Stru_AstTraversalData_t*)traversal->private_data;
    if (!data) return false;
    
    // 清空匹配节点数组
    clear_matches(data);
    
    return true;
}

static void depth_first_traversal_impl(Stru_AstTraversalInterface_t* traversal,
                                      Stru_AstNodeInterface_t* root,
                                      void (*pre_order)(Stru_AstNodeInterface_t*, void*),
                                      void (*in_order)(Stru_AstNodeInterface_t*, void*),
                                      void (*post_order)(Stru_AstNodeInterface_t*, void*),
                                      void* context)
{
    if (!traversal || !root) return;
    
    F_depth_first_traversal_recursive(root, pre_order, in_order, post_order, context);
}

static void breadth_first_traversal_impl(Stru_AstTraversalInterface_t* traversal,
                                        Stru_AstNodeInterface_t* root,
                                        void (*visitor)(Stru_AstNodeInterface_t*, void*),
                                        void* context)
{
    if (!traversal || !root || !visitor) return;
    
    // 创建队列用于广度优先遍历
    Stru_Queue_t* queue = F_create_queue(sizeof(Stru_AstNodeInterface_t*));
    if (!queue) return;
    
    F_breadth_first_traversal_impl(root, visitor, context, queue);
    
    F_destroy_queue(queue);
}

static Stru_AstNodeInterface_t** find_nodes_impl(Stru_AstTraversalInterface_t* traversal,
                                                Stru_AstNodeInterface_t* root,
                                                bool (*predicate)(Stru_AstNodeInterface_t*, void*),
                                                void* context)
{
    if (!traversal || !root || !predicate) return NULL;
    
    Stru_AstTraversalData_t* data = 
        (Stru_AstTraversalData_t*)traversal->private_data;
    if (!data) return NULL;
    
    // 清空之前的匹配结果
    clear_matches(data);
    
    // 递归查找节点
    F_find_nodes_recursive(root, predicate, context, data->matches, 
                          &data->match_count, data->matches_capacity);
    
    // 确保数组以NULL结尾
    if (data->match_count < data->matches_capacity) {
        data->matches[data->match_count] = NULL;
    }
    
    return data->matches;
}

static size_t get_match_count_impl(Stru_AstTraversalInterface_t* traversal)
{
    if (!traversal) return 0;
    
    Stru_AstTraversalData_t* data = 
        (Stru_AstTraversalData_t*)traversal->private_data;
    return data ? data->match_count : 0;
}

static void destroy_impl(Stru_AstTraversalInterface_t* traversal)
{
    if (!traversal) return;
    
    Stru_AstTraversalData_t* data = 
        (Stru_AstTraversalData_t*)traversal->private_data;
    if (data) {
        F_destroy_ast_traversal_data(data);
    }
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

void F_depth_first_traversal_recursive(Stru_AstNodeInterface_t* node,
                                      void (*pre_order)(Stru_AstNodeInterface_t*, void*),
                                      void (*in_order)(Stru_AstNodeInterface_t*, void*),
                                      void (*post_order)(Stru_AstNodeInterface_t*, void*),
                                      void* context)
{
    if (!node) return;
    
    // 前序遍历
    if (pre_order) {
        pre_order(node, context);
    }
    
    // 中序遍历（对于二叉树才有意义，这里我们只处理前序和后序）
    if (in_order) {
        // 对于AST树，中序遍历通常不适用
        // 可以在这里添加特定逻辑
    }
    
    // 遍历子节点
    size_t child_count = node->get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        F_depth_first_traversal_recursive(child, pre_order, in_order, post_order, context);
    }
    
    // 后序遍历
    if (post_order) {
        post_order(node, context);
    }
}

void F_breadth_first_traversal_impl(Stru_AstNodeInterface_t* root,
                                   void (*visitor)(Stru_AstNodeInterface_t*, void*),
                                   void* context,
                                   Stru_Queue_t* queue)
{
    if (!root || !visitor || !queue) return;
    
    // 将根节点入队
    F_queue_enqueue(queue, &root);
    
    // 遍历队列
    while (!F_queue_is_empty(queue)) {
        Stru_AstNodeInterface_t* current_node = NULL;
        F_queue_dequeue(queue, &current_node);
        
        if (!current_node) continue;
        
        // 访问当前节点
        visitor(current_node, context);
        
        // 将子节点入队
        size_t child_count = current_node->get_child_count(current_node);
        for (size_t i = 0; i < child_count; i++) {
            Stru_AstNodeInterface_t* child = current_node->get_child(current_node, i);
            if (child) {
                F_queue_enqueue(queue, &child);
            }
        }
    }
}

void F_find_nodes_recursive(Stru_AstNodeInterface_t* node,
                           bool (*predicate)(Stru_AstNodeInterface_t*, void*),
                           void* context,
                           Stru_AstNodeInterface_t** matches,
                           size_t* match_count,
                           size_t max_matches)
{
    if (!node || !predicate || !matches || !match_count) return;
    
    // 检查当前节点是否匹配
    if (predicate(node, context)) {
        if (*match_count < max_matches) {
            matches[*match_count] = node;
            (*match_count)++;
        }
    }
    
    // 递归检查子节点
    size_t child_count = node->get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        F_find_nodes_recursive(child, predicate, context, matches, match_count, max_matches);
    }
}
