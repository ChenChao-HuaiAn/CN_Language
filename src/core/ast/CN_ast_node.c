/**
 * @file CN_ast_node.c
 * @brief CN_Language AST节点实现
 * 
 * 实现AST节点接口的具体功能，提供AST节点的创建、操作和管理。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_ast_node.h"
#include "../../infrastructure/memory/CN_memory_interface.h"
#include "../../infrastructure/utils/CN_utils_interface.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// 辅助函数声明
// ============================================================================

/**
 * @brief 复制字符串（类似strdup）
 */
static char* cn_strdup(const char* str)
{
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* new_str = (char*)cn_malloc(len);
    if (!new_str) return NULL;
    
    memcpy(new_str, str, len);
    return new_str;
}

// ============================================================================
// 静态函数声明
// ============================================================================

static Eum_AstNodeType get_type_impl(const Stru_AstNodeInterface_t* node);
static const Stru_AstNodeLocation_t* get_location_impl(const Stru_AstNodeInterface_t* node);
static const Uni_AstNodeData_t* get_data_impl(const Stru_AstNodeInterface_t* node);
static bool set_data_impl(Stru_AstNodeInterface_t* node, const Uni_AstNodeData_t* data);
static Stru_AstNodeInterface_t* get_parent_impl(const Stru_AstNodeInterface_t* node);
static bool set_parent_impl(Stru_AstNodeInterface_t* node, Stru_AstNodeInterface_t* parent);
static size_t get_child_count_impl(const Stru_AstNodeInterface_t* node);
static Stru_AstNodeInterface_t* get_child_impl(const Stru_AstNodeInterface_t* node, size_t index);
static bool add_child_impl(Stru_AstNodeInterface_t* node, Stru_AstNodeInterface_t* child);
static Stru_AstNodeInterface_t* remove_child_impl(Stru_AstNodeInterface_t* node, size_t index);
static void* get_attribute_impl(const Stru_AstNodeInterface_t* node, const char* key);
static bool set_attribute_impl(Stru_AstNodeInterface_t* node, const char* key, void* value);
static bool remove_attribute_impl(Stru_AstNodeInterface_t* node, const char* key);
static bool has_attribute_impl(const Stru_AstNodeInterface_t* node, const char* key);
static Stru_AstNodeInterface_t* copy_impl(const Stru_AstNodeInterface_t* node);
static void destroy_impl(Stru_AstNodeInterface_t* node);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 创建属性键值对
 */
static Stru_AstAttribute_t* create_attribute(const char* key, void* value)
{
    Stru_AstAttribute_t* attr = (Stru_AstAttribute_t*)cn_malloc(sizeof(Stru_AstAttribute_t));
    if (!attr) return NULL;
    
    attr->key = cn_strdup(key);
    if (!attr->key) {
        cn_free(attr);
        return NULL;
    }
    
    attr->value = value;
    return attr;
}

/**
 * @brief 销毁属性键值对
 */
static void destroy_attribute(Stru_AstAttribute_t* attr)
{
    if (!attr) return;
    
    if (attr->key) {
        cn_free(attr->key);
    }
    
    cn_free(attr);
}

/**
 * @brief 在属性数组中查找属性
 */
static Stru_AstAttribute_t* find_attribute(Stru_DynamicArray_t* attributes, const char* key)
{
    if (!attributes || !key) return NULL;
    
    size_t count = F_dynamic_array_get_size(attributes);
    for (size_t i = 0; i < count; i++) {
        Stru_AstAttribute_t* attr = (Stru_AstAttribute_t*)F_dynamic_array_get(attributes, i);
        if (attr && attr->key && strcmp(attr->key, key) == 0) {
            return attr;
        }
    }
    
    return NULL;
}

// ============================================================================
// 工厂函数实现
// ============================================================================

Stru_AstNodeInternalData_t* F_create_ast_node_internal_data(
    Eum_AstNodeType type,
    const Stru_AstNodeLocation_t* location)
{
    Stru_AstNodeInternalData_t* internal_data = 
        (Stru_AstNodeInternalData_t*)cn_malloc(sizeof(Stru_AstNodeInternalData_t));
    if (!internal_data) return NULL;
    
    // 初始化基本字段
    internal_data->type = type;
    
    if (location) {
        internal_data->location = *location;
    } else {
        memset(&internal_data->location, 0, sizeof(Stru_AstNodeLocation_t));
    }
    
    memset(&internal_data->data, 0, sizeof(Uni_AstNodeData_t));
    internal_data->parent = NULL;
    
    // 创建子节点数组
    internal_data->children = F_create_dynamic_array(sizeof(Stru_AstNodeInterface_t*), 8);
    if (!internal_data->children) {
        cn_free(internal_data);
        return NULL;
    }
    
    // 创建属性数组
    internal_data->attributes = F_create_dynamic_array(sizeof(Stru_AstAttribute_t*), 4);
    if (!internal_data->attributes) {
        F_destroy_dynamic_array(internal_data->children);
        cn_free(internal_data);
        return NULL;
    }
    
    return internal_data;
}

void F_destroy_ast_node_internal_data(Stru_AstNodeInternalData_t* internal_data)
{
    if (!internal_data) return;
    
    // 释放字符串数据
    if (internal_data->data.string_value) {
        cn_free(internal_data->data.string_value);
    }
    
    if (internal_data->data.type_name) {
        cn_free(internal_data->data.type_name);
    }
    
    if (internal_data->data.identifier) {
        cn_free(internal_data->data.identifier);
    }
    
    if (internal_data->location.file_name) {
        cn_free((void*)internal_data->location.file_name);
    }
    
    // 释放所有属性
    if (internal_data->attributes) {
        size_t attr_count = F_dynamic_array_get_size(internal_data->attributes);
        for (size_t i = 0; i < attr_count; i++) {
            Stru_AstAttribute_t* attr = (Stru_AstAttribute_t*)F_dynamic_array_get(internal_data->attributes, i);
            destroy_attribute(attr);
        }
        F_destroy_dynamic_array(internal_data->attributes);
    }
    
    // 注意：不释放子节点，因为子节点有自己的生命周期管理
    if (internal_data->children) {
        F_destroy_dynamic_array(internal_data->children);
    }
    
    cn_free(internal_data);
}

Stru_AstNodeInterface_t* F_create_ast_node_interface_impl(void)
{
    Stru_AstNodeInterface_t* interface = 
        (Stru_AstNodeInterface_t*)cn_malloc(sizeof(Stru_AstNodeInterface_t));
    if (!interface) return NULL;
    
    // 设置函数指针
    interface->get_type = get_type_impl;
    interface->get_location = get_location_impl;
    interface->get_data = get_data_impl;
    interface->set_data = set_data_impl;
    interface->get_parent = get_parent_impl;
    interface->set_parent = set_parent_impl;
    interface->get_child_count = get_child_count_impl;
    interface->get_child = get_child_impl;
    interface->add_child = add_child_impl;
    interface->remove_child = remove_child_impl;
    interface->get_attribute = get_attribute_impl;
    interface->set_attribute = set_attribute_impl;
    interface->remove_attribute = remove_attribute_impl;
    interface->has_attribute = has_attribute_impl;
    interface->copy = copy_impl;
    interface->destroy = destroy_impl;
    
    // 创建内部数据
    interface->private_data = F_create_ast_node_internal_data(Eum_AST_PROGRAM, NULL);
    if (!interface->private_data) {
        cn_free(interface);
        return NULL;
    }
    
    return interface;
}

Stru_AstNodeInterface_t* F_create_ast_node_interface_with_type(
    Eum_AstNodeType type,
    const Stru_AstNodeLocation_t* location)
{
    Stru_AstNodeInterface_t* interface = F_create_ast_node_interface_impl();
    if (!interface) return NULL;
    
    Stru_AstNodeInternalData_t* internal_data = 
        (Stru_AstNodeInternalData_t*)interface->private_data;
    
    if (internal_data) {
        internal_data->type = type;
        if (location) {
            internal_data->location = *location;
            if (location->file_name) {
                internal_data->location.file_name = cn_strdup(location->file_name);
            }
        }
    }
    
    return interface;
}

Stru_AstNodeInterface_t* F_get_ast_node_interface_from_internal_data(
    Stru_AstNodeInternalData_t* internal_data)
{
    if (!internal_data) return NULL;
    
    Stru_AstNodeInterface_t* interface = 
        (Stru_AstNodeInterface_t*)cn_malloc(sizeof(Stru_AstNodeInterface_t));
    if (!interface) return NULL;
    
    // 设置函数指针
    interface->get_type = get_type_impl;
    interface->get_location = get_location_impl;
    interface->get_data = get_data_impl;
    interface->set_data = set_data_impl;
    interface->get_parent = get_parent_impl;
    interface->set_parent = set_parent_impl;
    interface->get_child_count = get_child_count_impl;
    interface->get_child = get_child_impl;
    interface->add_child = add_child_impl;
    interface->remove_child = remove_child_impl;
    interface->get_attribute = get_attribute_impl;
    interface->set_attribute = set_attribute_impl;
    interface->remove_attribute = remove_attribute_impl;
    interface->has_attribute = has_attribute_impl;
    interface->copy = copy_impl;
    interface->destroy = destroy_impl;
    
    interface->private_data = internal_data;
    
    return interface;
}

Stru_AstNodeInternalData_t* F_get_internal_data_from_ast_node_interface(
    const Stru_AstNodeInterface_t* node_interface)
{
    if (!node_interface) return NULL;
    return (Stru_AstNodeInternalData_t*)node_interface->private_data;
}

// ============================================================================
// 接口函数实现
// ============================================================================

static Eum_AstNodeType get_type_impl(const Stru_AstNodeInterface_t* node)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    return internal_data ? internal_data->type : Eum_AST_ERROR_NODE;
}

static const Stru_AstNodeLocation_t* get_location_impl(const Stru_AstNodeInterface_t* node)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    return internal_data ? &internal_data->location : NULL;
}

static const Uni_AstNodeData_t* get_data_impl(const Stru_AstNodeInterface_t* node)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    return internal_data ? &internal_data->data : NULL;
}

static bool set_data_impl(Stru_AstNodeInterface_t* node, const Uni_AstNodeData_t* data)
{
    if (!node || !data) return false;
    
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data) return false;
    
    // 释放旧的字符串数据
    if (internal_data->data.string_value) {
        cn_free(internal_data->data.string_value);
        internal_data->data.string_value = NULL;
    }
    
    if (internal_data->data.type_name) {
        cn_free(internal_data->data.type_name);
        internal_data->data.type_name = NULL;
    }
    
    if (internal_data->data.identifier) {
        cn_free(internal_data->data.identifier);
        internal_data->data.identifier = NULL;
    }
    
    // 复制数据
    internal_data->data = *data;
    
    // 复制字符串数据
    if (data->string_value) {
        internal_data->data.string_value = cn_strdup(data->string_value);
    }
    
    if (data->type_name) {
        internal_data->data.type_name = cn_strdup(data->type_name);
    }
    
    if (data->identifier) {
        internal_data->data.identifier = cn_strdup(data->identifier);
    }
    
    return true;
}

static Stru_AstNodeInterface_t* get_parent_impl(const Stru_AstNodeInterface_t* node)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    return internal_data ? internal_data->parent : NULL;
}

static bool set_parent_impl(Stru_AstNodeInterface_t* node, Stru_AstNodeInterface_t* parent)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data) return false;
    
    internal_data->parent = parent;
    return true;
}

static size_t get_child_count_impl(const Stru_AstNodeInterface_t* node)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data || !internal_data->children) return 0;
    
    return F_dynamic_array_get_size(internal_data->children);
}

static Stru_AstNodeInterface_t* get_child_impl(const Stru_AstNodeInterface_t* node, size_t index)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data || !internal_data->children) return NULL;
    
    size_t count = F_dynamic_array_get_size(internal_data->children);
    if (index >= count) return NULL;
    
    return *(Stru_AstNodeInterface_t**)F_dynamic_array_get(internal_data->children, index);
}

static bool add_child_impl(Stru_AstNodeInterface_t* node, Stru_AstNodeInterface_t* child)
{
    if (!node || !child) return false;
    
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data || !internal_data->children) return false;
    
    // 设置子节点的父节点
    Stru_AstNodeInternalData_t* child_internal_data = 
        F_get_internal_data_from_ast_node_interface(child);
    if (child_internal_data) {
        child_internal_data->parent = node;
    }
    
    // 添加子节点到数组
    return F_dynamic_array_push_back(internal_data->children, &child);
}

static Stru_AstNodeInterface_t* remove_child_impl(Stru_AstNodeInterface_t* node, size_t index)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data || !internal_data->children) return NULL;
    
    size_t count = F_dynamic_array_get_size(internal_data->children);
    if (index >= count) return NULL;
    
    Stru_AstNodeInterface_t* child = 
        *(Stru_AstNodeInterface_t**)F_dynamic_array_get(internal_data->children, index);
    
    // 从数组中移除
    F_dynamic_array_remove(internal_data->children, index);
    
    // 清除子节点的父节点引用
    if (child) {
        Stru_AstNodeInternalData_t* child_internal_data = 
            F_get_internal_data_from_ast_node_interface(child);
        if (child_internal_data && child_internal_data->parent == node) {
            child_internal_data->parent = NULL;
        }
    }
    
    return child;
}

static void* get_attribute_impl(const Stru_AstNodeInterface_t* node, const char* key)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data || !internal_data->attributes || !key) return NULL;
    
    Stru_AstAttribute_t* attr = find_attribute(internal_data->attributes, key);
    return attr ? attr->value : NULL;
}

static bool set_attribute_impl(Stru_AstNodeInterface_t* node, const char* key, void* value)
{
    if (!node || !key) return false;
    
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data || !internal_data->attributes) return false;
    
    // 查找现有属性
    Stru_AstAttribute_t* attr = find_attribute(internal_data->attributes, key);
    
    if (attr) {
        // 更新现有属性
        attr->value = value;
        return true;
    } else {
        // 创建新属性
        attr = create_attribute(key, value);
        if (!attr) return false;
        
        return F_dynamic_array_push_back(internal_data->attributes, &attr);
    }
}

static bool remove_attribute_impl(Stru_AstNodeInterface_t* node, const char* key)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data || !internal_data->attributes || !key) return false;
    
    size_t count = F_dynamic_array_get_size(internal_data->attributes);
    for (size_t i = 0; i < count; i++) {
        Stru_AstAttribute_t* attr = (Stru_AstAttribute_t*)F_dynamic_array_get(internal_data->attributes, i);
        if (attr && attr->key && strcmp(attr->key, key) == 0) {
            destroy_attribute(attr);
            F_dynamic_array_remove(internal_data->attributes, i);
            return true;
        }
    }
    
    return false;
}

static bool has_attribute_impl(const Stru_AstNodeInterface_t* node, const char* key)
{
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data || !internal_data->attributes || !key) return false;
    
    Stru_AstAttribute_t* attr = find_attribute(internal_data->attributes, key);
    return attr != NULL;
}

static Stru_AstNodeInterface_t* copy_impl(const Stru_AstNodeInterface_t* node)
{
    if (!node) return NULL;
    
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (!internal_data) return NULL;
    
    // 创建新节点
    Stru_AstNodeInterface_t* new_node = 
        F_create_ast_node_interface_with_type(internal_data->type, &internal_data->location);
    if (!new_node) return NULL;
    
    Stru_AstNodeInternalData_t* new_internal_data = 
        F_get_internal_data_from_ast_node_interface(new_node);
    if (!new_internal_data) {
        new_node->destroy(new_node);
        return NULL;
    }
    
    // 复制数据
    if (!new_node->set_data(new_node, &internal_data->data)) {
        new_node->destroy(new_node);
        return NULL;
    }
    
    // 复制子节点
    size_t child_count = node->get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        if (child) {
            Stru_AstNodeInterface_t* child_copy = child->copy(child);
            if (child_copy) {
                new_node->add_child(new_node, child_copy);
            }
        }
    }
    
    // 复制属性
    size_t attr_count = F_dynamic_array_get_size(internal_data->attributes);
    for (size_t i = 0; i < attr_count; i++) {
        Stru_AstAttribute_t* attr = (Stru_AstAttribute_t*)F_dynamic_array_get(internal_data->attributes, i);
        if (attr && attr->key) {
            new_node->set_attribute(new_node, attr->key, attr->value);
        }
    }
    
    return new_node;
}

static void destroy_impl(Stru_AstNodeInterface_t* node)
{
    if (!node) return;
    
    Stru_AstNodeInternalData_t* internal_data = 
        F_get_internal_data_from_ast_node_interface(node);
    if (internal_data) {
        F_destroy_ast_node_internal_data(internal_data);
    }
    
    cn_free(node);
}
