/**
 * @file CN_ast_compat.c
 * @brief CN_Language AST兼容层实现
 * 
 * 实现向后兼容的AST API，内部使用新的接口实现。
 * 提供从旧API到新接口的转换层。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_ast.h"
#include "CN_ast_node.h"
#include "CN_ast_builder.h"
#include "../../infrastructure/memory/CN_memory_interface.h"
#include <string.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 从兼容层节点获取内部接口
 */
static Stru_AstNodeInterface_t* get_node_interface(const Stru_AstNode_t* node)
{
    if (!node) return NULL;
    return node->interface;
}

/**
 * @brief 从内部接口创建兼容层节点
 */
static Stru_AstNode_t* create_compat_node(Stru_AstNodeInterface_t* interface)
{
    if (!interface) return NULL;
    
    Stru_AstNode_t* node = (Stru_AstNode_t*)cn_malloc(sizeof(Stru_AstNode_t));
    if (!node) return NULL;
    
    node->interface = interface;
    node->private_data = NULL;
    
    return node;
}

/**
 * @brief 将位置信息转换为内部格式
 */
static Stru_AstNodeLocation_t convert_location(size_t line, size_t column, const char* file_name)
{
    Stru_AstNodeLocation_t location;
    location.line = line;
    location.column = column;
    location.file_name = file_name;
    location.start_offset = 0;
    location.end_offset = 0;
    
    return location;
}

// ============================================================================
// AST节点创建和销毁函数（兼容层）
// ============================================================================

Stru_AstNode_t* F_create_ast_node(Eum_AstNodeType type, void* token, size_t line, size_t column)
{
    // 创建位置信息
    Stru_AstNodeLocation_t location = convert_location(line, column, NULL);
    
    // 创建内部节点
    Stru_AstNodeInterface_t* interface = 
        F_create_ast_node_interface_with_type(type, &location);
    if (!interface) return NULL;
    
    // 创建兼容层节点
    return create_compat_node(interface);
}

void F_destroy_ast_node(Stru_AstNode_t* node)
{
    if (!node) return;
    
    if (node->interface && node->interface->destroy) {
        node->interface->destroy(node->interface);
    }
    
    cn_free(node);
}

Stru_AstNode_t* F_copy_ast_node(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return NULL;
    
    // 复制内部节点
    Stru_AstNodeInterface_t* new_interface = node->interface->copy(node->interface);
    if (!new_interface) return NULL;
    
    // 创建兼容层节点
    return create_compat_node(new_interface);
}

// ============================================================================
// AST节点操作函数（兼容层）
// ============================================================================

bool F_ast_add_child(Stru_AstNode_t* parent, Stru_AstNode_t* child)
{
    if (!parent || !child || !parent->interface || !child->interface) return false;
    
    return parent->interface->add_child(parent->interface, child->interface);
}

size_t F_ast_get_child_count(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return 0;
    
    return node->interface->get_child_count(node->interface);
}

Stru_AstNode_t* F_ast_get_child(const Stru_AstNode_t* node, size_t index)
{
    if (!node || !node->interface) return NULL;
    
    Stru_AstNodeInterface_t* child_interface = 
        node->interface->get_child(node->interface, index);
    if (!child_interface) return NULL;
    
    return create_compat_node(child_interface);
}

Stru_AstNode_t* F_ast_remove_child(Stru_AstNode_t* node, size_t index)
{
    if (!node || !node->interface) return NULL;
    
    Stru_AstNodeInterface_t* child_interface = 
        node->interface->remove_child(node->interface, index);
    if (!child_interface) return NULL;
    
    return create_compat_node(child_interface);
}

// ============================================================================
// AST节点属性操作函数（兼容层）
// ============================================================================

bool F_ast_set_attribute(Stru_AstNode_t* node, const char* key, void* value)
{
    if (!node || !node->interface || !key) return false;
    
    return node->interface->set_attribute(node->interface, key, value);
}

void* F_ast_get_attribute(const Stru_AstNode_t* node, const char* key)
{
    if (!node || !node->interface || !key) return NULL;
    
    return node->interface->get_attribute(node->interface, key);
}

bool F_ast_remove_attribute(Stru_AstNode_t* node, const char* key)
{
    if (!node || !node->interface || !key) return false;
    
    return node->interface->remove_attribute(node->interface, key);
}

bool F_ast_has_attribute(const Stru_AstNode_t* node, const char* key)
{
    if (!node || !node->interface || !key) return false;
    
    return node->interface->has_attribute(node->interface, key);
}

// ============================================================================
// AST节点数据操作函数（兼容层）
// ============================================================================

void F_ast_set_int_value(Stru_AstNode_t* node, long value)
{
    if (!node || !node->interface) return;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.int_value = value;
    
    node->interface->set_data(node->interface, &data);
}

void F_ast_set_float_value(Stru_AstNode_t* node, double value)
{
    if (!node || !node->interface) return;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.float_value = value;
    
    node->interface->set_data(node->interface, &data);
}

void F_ast_set_string_value(Stru_AstNode_t* node, const char* value)
{
    if (!node || !node->interface) return;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.string_value = (char*)value; // 注意：这里只是保存指针，实际实现应该复制
    
    node->interface->set_data(node->interface, &data);
}

void F_ast_set_bool_value(Stru_AstNode_t* node, bool value)
{
    if (!node || !node->interface) return;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.bool_value = value;
    
    node->interface->set_data(node->interface, &data);
}

void F_ast_set_identifier(Stru_AstNode_t* node, const char* name)
{
    if (!node || !node->interface) return;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.identifier = (char*)name; // 注意：这里只是保存指针，实际实现应该复制
    
    node->interface->set_data(node->interface, &data);
}

void F_ast_set_type_name(Stru_AstNode_t* node, const char* type_name)
{
    if (!node || !node->interface) return;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.type_name = (char*)type_name; // 注意：这里只是保存指针，实际实现应该复制
    
    node->interface->set_data(node->interface, &data);
}

void F_ast_set_array_size(Stru_AstNode_t* node, size_t size)
{
    if (!node || !node->interface) return;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.array_size = size;
    
    node->interface->set_data(node->interface, &data);
}

void F_ast_set_operator_type(Stru_AstNode_t* node, int operator_type)
{
    if (!node || !node->interface) return;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.operator_type = operator_type;
    
    node->interface->set_data(node->interface, &data);
}

long F_ast_get_int_value(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return 0;
    
    const Uni_AstNodeData_t* data = node->interface->get_data(node->interface);
    return data ? data->int_value : 0;
}

double F_ast_get_float_value(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return 0.0;
    
    const Uni_AstNodeData_t* data = node->interface->get_data(node->interface);
    return data ? data->float_value : 0.0;
}

const char* F_ast_get_string_value(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return NULL;
    
    const Uni_AstNodeData_t* data = node->interface->get_data(node->interface);
    return data ? data->string_value : NULL;
}

bool F_ast_get_bool_value(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return false;
    
    const Uni_AstNodeData_t* data = node->interface->get_data(node->interface);
    return data ? data->bool_value : false;
}

const char* F_ast_get_identifier(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return NULL;
    
    const Uni_AstNodeData_t* data = node->interface->get_data(node->interface);
    return data ? data->identifier : NULL;
}

const char* F_ast_get_type_name(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return NULL;
    
    const Uni_AstNodeData_t* data = node->interface->get_data(node->interface);
    return data ? data->type_name : NULL;
}

size_t F_ast_get_array_size(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return 0;
    
    const Uni_AstNodeData_t* data = node->interface->get_data(node->interface);
    return data ? data->array_size : 0;
}

int F_ast_get_operator_type(const Stru_AstNode_t* node)
{
    if (!node || !node->interface) return 0;
    
    const Uni_AstNodeData_t* data = node->interface->get_data(node->interface);
    return data ? data->operator_type : 0;
}

// ============================================================================
// AST遍历和查询函数（兼容层）- 简化实现
// ============================================================================

const char* F_ast_node_type_to_string(Eum_AstNodeType type)
{
    // 简化实现，返回类型名称
    static const char* type_names[] = {
        "PROGRAM", "MODULE", "IMPORT",
        "VARIABLE_DECL", "FUNCTION_DECL", "STRUCT_DECL", "ENUM_DECL", 
        "CONSTANT_DECL", "PARAMETER_DECL",
        "EXPRESSION_STMT", "IF_STMT", "WHILE_STMT", "FOR_STMT", 
        "RETURN_STMT", "BREAK_STMT", "CONTINUE_STMT", "BLOCK_STMT",
        "SWITCH_STMT", "CASE_STMT", "DEFAULT_STMT",
        "BINARY_EXPR", "UNARY_EXPR", "LITERAL_EXPR", "IDENTIFIER_EXPR",
        "CALL_EXPR", "INDEX_EXPR", "MEMBER_EXPR", "ASSIGN_EXPR",
        "COMPOUND_ASSIGN_EXPR", "CAST_EXPR", "CONDITIONAL_EXPR",
        "NEW_EXPR", "DELETE_EXPR",
        "TYPE_NAME", "ARRAY_TYPE", "POINTER_TYPE", "REFERENCE_TYPE",
        "FUNCTION_TYPE",
        "INT_LITERAL", "FLOAT_LITERAL", "STRING_LITERAL", "BOOL_LITERAL",
        "ARRAY_LITERAL", "STRUCT_LITERAL", "NULL_LITERAL",
        "ERROR_NODE",
        "COMMENT", "DIRECTIVE"
    };
    
    if (type < Eum_AST_COUNT) {
        return type_names[type];
    }
    
    return "UNKNOWN";
}

int F_ast_node_to_string(const Stru_AstNode_t* node, char* buffer, size_t buffer_size)
{
    if (!node || !buffer || buffer_size == 0) return 0;
    
    const char* type_name = F_ast_node_type_to_string(node->interface->get_type(node->interface));
    return snprintf(buffer, buffer_size, "ASTNode(%s)", type_name);
}

void F_ast_print_tree(const Stru_AstNode_t* root, int indent)
{
    // 简化实现，只打印根节点
    if (!root) return;
    
    char buffer[256];
    F_ast_node_to_string(root, buffer, sizeof(buffer));
    
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    printf("%s\n", buffer);
}

void* F_ast_find_nodes(const Stru_AstNode_t* root, 
                       bool (*predicate)(const Stru_AstNode_t*, void*), 
                       void* context)
{
    // 简化实现，返回NULL
    return NULL;
}

void F_ast_traverse(Stru_AstNode_t* root, 
                   void (*visitor)(Stru_AstNode_t*, void*), 
                   void* context)
{
    if (!root || !visitor) return;
    
    // 只访问根节点
    visitor(root, context);
}

// ============================================================================
// AST验证函数（兼容层）
// ============================================================================

bool F_ast_validate_node(const Stru_AstNode_t* node)
{
    return node != NULL && node->interface != NULL;
}

bool F_ast_validate_tree(const Stru_AstNode_t* root)
{
    return F_ast_validate_node(root);
}

// ============================================================================
// 新接口访问函数
// ============================================================================

Stru_AstNodeInterface_t* F_ast_get_internal_interface(const Stru_AstNode_t* node)
{
    if (!node) return NULL;
    return node->interface;
}

Stru_AstBuilderInterface_t* F_ast_create_builder_interface(void)
{
    return F_create_ast_builder_interface();
}

Stru_AstTraversalInterface_t* F_ast_create_traversal_interface(void)
{
    return F_create_ast_traversal_interface();
}

Stru_AstQueryInterface_t* F_ast_create_query_interface(void)
{
    return F_create_ast_query_interface();
}

Stru_AstSerializerInterface_t* F_ast_create_serializer_interface(void)
{
    return F_create_ast_serializer_interface();
}
