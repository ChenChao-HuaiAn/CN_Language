/**
 * @file CN_ast_query.c
 * @brief CN_Language AST查询器实现
 * 
 * 实现AST查询接口的具体功能，提供节点类型统计、属性查询等功能。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_ast_query.h"
#include "../../infrastructure/utils/CN_utils_interface.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// 内部数据结构定义
// ============================================================================

/**
 * @brief AST查询器内部数据结构
 */
struct Stru_AstQueryData_t
{
    Stru_AstQueryInterface_t interface;      ///< 查询器接口
    size_t match_count;                      ///< 匹配节点数量
    Stru_AstNodeInterface_t** matches;       ///< 匹配节点数组
    size_t matches_capacity;                 ///< 匹配节点数组容量
};

// ============================================================================
// 静态函数声明
// ============================================================================

static bool initialize_impl(Stru_AstQueryInterface_t* query);
static size_t count_node_types_impl(Stru_AstQueryInterface_t* query,
                                   Stru_AstNodeInterface_t* root,
                                   size_t* type_counts,
                                   size_t max_types);
static Stru_AstNodeInterface_t** find_nodes_with_attribute_impl(Stru_AstQueryInterface_t* query,
                                                               Stru_AstNodeInterface_t* root,
                                                               const char* key,
                                                               void* value);
static size_t get_node_depth_impl(Stru_AstQueryInterface_t* query,
                                 Stru_AstNodeInterface_t* node);
static size_t get_subtree_size_impl(Stru_AstQueryInterface_t* query,
                                   Stru_AstNodeInterface_t* node);
static bool validate_ast_structure_impl(Stru_AstQueryInterface_t* query,
                                       Stru_AstNodeInterface_t* root);
static void destroy_impl(Stru_AstQueryInterface_t* query);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 扩展匹配节点数组
 */
static bool expand_matches_array(Stru_AstQueryData_t* data)
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
static bool add_match(Stru_AstQueryData_t* data, Stru_AstNodeInterface_t* node)
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
static void clear_matches(Stru_AstQueryData_t* data)
{
    if (!data) return;
    
    data->match_count = 0;
}

// ============================================================================
// 工厂函数实现
// ============================================================================

Stru_AstQueryData_t* F_create_ast_query_data(void)
{
    Stru_AstQueryData_t* data = 
        (Stru_AstQueryData_t*)cn_malloc(sizeof(Stru_AstQueryData_t));
    if (!data) return NULL;
    
    // 初始化字段
    memset(data, 0, sizeof(Stru_AstQueryData_t));
    
    // 设置接口函数指针
    data->interface.initialize = initialize_impl;
    data->interface.count_node_types = count_node_types_impl;
    data->interface.find_nodes_with_attribute = find_nodes_with_attribute_impl;
    data->interface.get_node_depth = get_node_depth_impl;
    data->interface.get_subtree_size = get_subtree_size_impl;
    data->interface.validate_ast_structure = validate_ast_structure_impl;
    data->interface.destroy = destroy_impl;
    data->interface.private_data = data;
    
    return data;
}

void F_destroy_ast_query_data(Stru_AstQueryData_t* data)
{
    if (!data) return;
    
    // 释放匹配节点数组
    if (data->matches) {
        cn_free(data->matches);
    }
    
    cn_free(data);
}

Stru_AstQueryInterface_t* F_get_ast_query_interface(Stru_AstQueryData_t* data)
{
    if (!data) return NULL;
    return &data->interface;
}

// ============================================================================
// 接口函数实现
// ============================================================================

static bool initialize_impl(Stru_AstQueryInterface_t* query)
{
    if (!query) return false;
    
    Stru_AstQueryData_t* data = 
        (Stru_AstQueryData_t*)query->private_data;
    if (!data) return false;
    
    // 清空匹配节点数组
    clear_matches(data);
    
    return true;
}

static size_t count_node_types_impl(Stru_AstQueryInterface_t* query,
                                   Stru_AstNodeInterface_t* root,
                                   size_t* type_counts,
                                   size_t max_types)
{
    if (!query || !root || !type_counts || max_types == 0) return 0;
    
    // 清零统计数组
    memset(type_counts, 0, max_types * sizeof(size_t));
    
    // 递归统计节点类型
    F_count_node_types_recursive(root, type_counts, max_types);
    
    // 计算实际统计的类型数量
    size_t actual_count = 0;
    for (size_t i = 0; i < max_types && i < Eum_AST_COUNT; i++) {
        if (type_counts[i] > 0) {
            actual_count++;
        }
    }
    
    return actual_count;
}

static Stru_AstNodeInterface_t** find_nodes_with_attribute_impl(Stru_AstQueryInterface_t* query,
                                                               Stru_AstNodeInterface_t* root,
                                                               const char* key,
                                                               void* value)
{
    if (!query || !root || !key) return NULL;
    
    Stru_AstQueryData_t* data = 
        (Stru_AstQueryData_t*)query->private_data;
    if (!data) return NULL;
    
    // 清空之前的匹配结果
    clear_matches(data);
    
    // 递归查找具有特定属性的节点
    F_find_nodes_with_attribute_recursive(root, key, value, data->matches, 
                                         &data->match_count, data->matches_capacity);
    
    // 确保数组以NULL结尾
    if (data->match_count < data->matches_capacity) {
        data->matches[data->match_count] = NULL;
    }
    
    return data->matches;
}

static size_t get_node_depth_impl(Stru_AstQueryInterface_t* query,
                                 Stru_AstNodeInterface_t* node)
{
    if (!query || !node) return 0;
    
    return F_get_node_depth_recursive(node);
}

static size_t get_subtree_size_impl(Stru_AstQueryInterface_t* query,
                                   Stru_AstNodeInterface_t* node)
{
    if (!query || !node) return 0;
    
    return F_get_subtree_size_recursive(node);
}

static bool validate_ast_structure_impl(Stru_AstQueryInterface_t* query,
                                       Stru_AstNodeInterface_t* root)
{
    if (!query || !root) return false;
    
    return F_validate_ast_structure_recursive(root);
}

static void destroy_impl(Stru_AstQueryInterface_t* query)
{
    if (!query) return;
    
    Stru_AstQueryData_t* data = 
        (Stru_AstQueryData_t*)query->private_data;
    if (data) {
        F_destroy_ast_query_data(data);
    }
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

void F_count_node_types_recursive(Stru_AstNodeInterface_t* node,
                                 size_t* type_counts,
                                 size_t max_types)
{
    if (!node || !type_counts) return;
    
    // 获取节点类型
    Eum_AstNodeType type = node->get_type(node);
    
    // 统计节点类型
    if (type < max_types) {
        type_counts[type]++;
    }
    
    // 递归统计子节点
    size_t child_count = node->get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        F_count_node_types_recursive(child, type_counts, max_types);
    }
}

void F_find_nodes_with_attribute_recursive(Stru_AstNodeInterface_t* node,
                                          const char* key,
                                          void* value,
                                          Stru_AstNodeInterface_t** matches,
                                          size_t* match_count,
                                          size_t max_matches)
{
    if (!node || !key || !matches || !match_count) return;
    
    // 检查当前节点是否具有指定属性
    void* attr_value = node->get_attribute(node, key);
    
    // 如果value为NULL，检查是否有该属性（无论值是什么）
    // 如果value不为NULL，检查属性值是否匹配
    bool matches_criteria = false;
    if (value == NULL) {
        matches_criteria = (attr_value != NULL);
    } else {
        matches_criteria = (attr_value == value);
    }
    
    if (matches_criteria) {
        if (*match_count < max_matches) {
            matches[*match_count] = node;
            (*match_count)++;
        }
    }
    
    // 递归检查子节点
    size_t child_count = node->get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        F_find_nodes_with_attribute_recursive(child, key, value, matches, match_count, max_matches);
    }
}

size_t F_get_node_depth_recursive(Stru_AstNodeInterface_t* node)
{
    if (!node) return 0;
    
    Stru_AstNodeInterface_t* parent = node->get_parent(node);
    if (!parent) {
        return 0; // 根节点深度为0
    }
    
    return F_get_node_depth_recursive(parent) + 1;
}

size_t F_get_subtree_size_recursive(Stru_AstNodeInterface_t* node)
{
    if (!node) return 0;
    
    size_t size = 1; // 包括自身
    
    // 递归计算子节点数量
    size_t child_count = node->get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        size += F_get_subtree_size_recursive(child);
    }
    
    return size;
}

bool F_validate_ast_structure_recursive(Stru_AstNodeInterface_t* node)
{
    if (!node) return true;
    
    // 检查节点类型是否有效
    Eum_AstNodeType type = node->get_type(node);
    if (type >= Eum_AST_COUNT) {
        return false; // 无效的节点类型
    }
    
    // 检查子节点
    size_t child_count = node->get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        
        // 检查子节点是否有效
        if (!child) {
            return false; // 子节点为NULL
        }
        
        // 递归检查子节点
        if (!F_validate_ast_structure_recursive(child)) {
            return false;
        }
        
        // 检查子节点的父节点是否指向当前节点
        Stru_AstNodeInterface_t* child_parent = child->get_parent(child);
        if (child_parent != node) {
            return false; // 父子关系不一致
        }
    }
    
    return true;
}
