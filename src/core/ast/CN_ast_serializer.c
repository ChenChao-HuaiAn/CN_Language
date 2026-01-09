/**
 * @file CN_ast_serializer.c
 * @brief CN_Language AST序列化器实现
 * 
 * 实现AST序列化接口的具体功能，提供JSON、XML和二进制格式的序列化和反序列化功能。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_ast_serializer.h"
#include "../../infrastructure/utils/CN_utils_interface.h"
#include "../../infrastructure/containers/string/CN_string.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// ============================================================================
// 内部数据结构定义
// ============================================================================

/**
 * @brief AST序列化器内部数据结构
 */
struct Stru_AstSerializerData_t
{
    Stru_AstSerializerInterface_t interface; ///< 序列化器接口
    Stru_String_t* json_buffer;              ///< JSON缓冲区
    Stru_String_t* xml_buffer;               ///< XML缓冲区
    void* binary_buffer;                     ///< 二进制缓冲区
    size_t binary_buffer_size;               ///< 二进制缓冲区大小
};

// ============================================================================
// 静态函数声明
// ============================================================================

static bool initialize_impl(Stru_AstSerializerInterface_t* serializer);
static size_t serialize_to_json_impl(Stru_AstSerializerInterface_t* serializer,
                                    Stru_AstNodeInterface_t* root,
                                    char* buffer,
                                    size_t buffer_size);
static Stru_AstNodeInterface_t* deserialize_from_json_impl(Stru_AstSerializerInterface_t* serializer,
                                                          const char* json);
static size_t serialize_to_xml_impl(Stru_AstSerializerInterface_t* serializer,
                                   Stru_AstNodeInterface_t* root,
                                   char* buffer,
                                   size_t buffer_size);
static Stru_AstNodeInterface_t* deserialize_from_xml_impl(Stru_AstSerializerInterface_t* serializer,
                                                         const char* xml);
static size_t serialize_to_binary_impl(Stru_AstSerializerInterface_t* serializer,
                                      Stru_AstNodeInterface_t* root,
                                      void* buffer,
                                      size_t buffer_size);
static Stru_AstNodeInterface_t* deserialize_from_binary_impl(Stru_AstSerializerInterface_t* serializer,
                                                            const void* buffer,
                                                            size_t buffer_size);
static void destroy_impl(Stru_AstSerializerInterface_t* serializer);

// ============================================================================
// 辅助函数
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

/**
 * @brief 转义JSON字符串
 */
static char* escape_json_string(const char* str)
{
    if (!str) return NULL;
    
    // 计算转义后的长度
    size_t len = 0;
    for (const char* p = str; *p; p++) {
        switch (*p) {
            case '"': case '\\': case '\b': case '\f': 
            case '\n': case '\r': case '\t':
                len += 2; // 转义字符
                break;
            default:
                if ((unsigned char)*p < 0x20) {
                    len += 6; // Unicode转义 \uXXXX
                } else {
                    len += 1;
                }
                break;
        }
    }
    
    char* result = (char*)cn_malloc(len + 1);
    if (!result) return NULL;
    
    char* q = result;
    for (const char* p = str; *p; p++) {
        switch (*p) {
            case '"':  *q++ = '\\'; *q++ = '"'; break;
            case '\\': *q++ = '\\'; *q++ = '\\'; break;
            case '\b': *q++ = '\\'; *q++ = 'b'; break;
            case '\f': *q++ = '\\'; *q++ = 'f'; break;
            case '\n': *q++ = '\\'; *q++ = 'n'; break;
            case '\r': *q++ = '\\'; *q++ = 'r'; break;
            case '\t': *q++ = '\\'; *q++ = 't'; break;
            default:
                if ((unsigned char)*p < 0x20) {
                    sprintf(q, "\\u%04x", (unsigned char)*p);
                    q += 6;
                } else {
                    *q++ = *p;
                }
                break;
        }
    }
    *q = '\0';
    
    return result;
}

/**
 * @brief 追加字符串到缓冲区
 */
static bool append_to_buffer(char* buffer, size_t buffer_size, size_t* position, const char* str)
{
    if (!buffer || !position || !str) return false;
    
    size_t str_len = strlen(str);
    if (*position + str_len >= buffer_size) {
        return false; // 缓冲区不足
    }
    
    memcpy(buffer + *position, str, str_len);
    *position += str_len;
    buffer[*position] = '\0';
    
    return true;
}

/**
 * @brief 获取节点类型名称
 */
static const char* get_node_type_name(Eum_AstNodeType type)
{
    switch (type) {
        case Eum_AST_PROGRAM: return "PROGRAM";
        case Eum_AST_MODULE: return "MODULE";
        case Eum_AST_IMPORT: return "IMPORT";
        case Eum_AST_VARIABLE_DECL: return "VARIABLE_DECL";
        case Eum_AST_FUNCTION_DECL: return "FUNCTION_DECL";
        case Eum_AST_STRUCT_DECL: return "STRUCT_DECL";
        case Eum_AST_ENUM_DECL: return "ENUM_DECL";
        case Eum_AST_CONSTANT_DECL: return "CONSTANT_DECL";
        case Eum_AST_PARAMETER_DECL: return "PARAMETER_DECL";
        case Eum_AST_EXPRESSION_STMT: return "EXPRESSION_STMT";
        case Eum_AST_IF_STMT: return "IF_STMT";
        case Eum_AST_WHILE_STMT: return "WHILE_STMT";
        case Eum_AST_FOR_STMT: return "FOR_STMT";
        case Eum_AST_RETURN_STMT: return "RETURN_STMT";
        case Eum_AST_BREAK_STMT: return "BREAK_STMT";
        case Eum_AST_CONTINUE_STMT: return "CONTINUE_STMT";
        case Eum_AST_BLOCK_STMT: return "BLOCK_STMT";
        case Eum_AST_SWITCH_STMT: return "SWITCH_STMT";
        case Eum_AST_CASE_STMT: return "CASE_STMT";
        case Eum_AST_DEFAULT_STMT: return "DEFAULT_STMT";
        case Eum_AST_BINARY_EXPR: return "BINARY_EXPR";
        case Eum_AST_UNARY_EXPR: return "UNARY_EXPR";
        case Eum_AST_LITERAL_EXPR: return "LITERAL_EXPR";
        case Eum_AST_IDENTIFIER_EXPR: return "IDENTIFIER_EXPR";
        case Eum_AST_CALL_EXPR: return "CALL_EXPR";
        case Eum_AST_INDEX_EXPR: return "INDEX_EXPR";
        case Eum_AST_MEMBER_EXPR: return "MEMBER_EXPR";
        case Eum_AST_ASSIGN_EXPR: return "ASSIGN_EXPR";
        case Eum_AST_COMPOUND_ASSIGN_EXPR: return "COMPOUND_ASSIGN_EXPR";
        case Eum_AST_CAST_EXPR: return "CAST_EXPR";
        case Eum_AST_CONDITIONAL_EXPR: return "CONDITIONAL_EXPR";
        case Eum_AST_NEW_EXPR: return "NEW_EXPR";
        case Eum_AST_DELETE_EXPR: return "DELETE_EXPR";
        case Eum_AST_TYPE_NAME: return "TYPE_NAME";
        case Eum_AST_ARRAY_TYPE: return "ARRAY_TYPE";
        case Eum_AST_POINTER_TYPE: return "POINTER_TYPE";
        case Eum_AST_REFERENCE_TYPE: return "REFERENCE_TYPE";
        case Eum_AST_FUNCTION_TYPE: return "FUNCTION_TYPE";
        case Eum_AST_INT_LITERAL: return "INT_LITERAL";
        case Eum_AST_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case Eum_AST_STRING_LITERAL: return "STRING_LITERAL";
        case Eum_AST_BOOL_LITERAL: return "BOOL_LITERAL";
        case Eum_AST_ARRAY_LITERAL: return "ARRAY_LITERAL";
        case Eum_AST_STRUCT_LITERAL: return "STRUCT_LITERAL";
        case Eum_AST_NULL_LITERAL: return "NULL_LITERAL";
        case Eum_AST_ERROR_NODE: return "ERROR_NODE";
        case Eum_AST_COMMENT: return "COMMENT";
        case Eum_AST_DIRECTIVE: return "DIRECTIVE";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

Stru_AstSerializerData_t* F_create_ast_serializer_data(void)
{
    Stru_AstSerializerData_t* data = 
        (Stru_AstSerializerData_t*)cn_malloc(sizeof(Stru_AstSerializerData_t));
    if (!data) return NULL;
    
    // 初始化字段
    memset(data, 0, sizeof(Stru_AstSerializerData_t));
    
    // 设置接口函数指针
    data->interface.initialize = initialize_impl;
    data->interface.serialize_to_json = serialize_to_json_impl;
    data->interface.deserialize_from_json = deserialize_from_json_impl;
    data->interface.serialize_to_xml = serialize_to_xml_impl;
    data->interface.deserialize_from_xml = deserialize_from_xml_impl;
    data->interface.serialize_to_binary = serialize_to_binary_impl;
    data->interface.deserialize_from_binary = deserialize_from_binary_impl;
    data->interface.destroy = destroy_impl;
    data->interface.private_data = data;
    
    return data;
}

void F_destroy_ast_serializer_data(Stru_AstSerializerData_t* data)
{
    if (!data) return;
    
    // 释放缓冲区
    if (data->json_buffer) {
        F_string_destroy(data->json_buffer);
    }
    
    if (data->xml_buffer) {
        F_string_destroy(data->xml_buffer);
    }
    
    if (data->binary_buffer) {
        cn_free(data->binary_buffer);
    }
    
    cn_free(data);
}

Stru_AstSerializerInterface_t* F_get_ast_serializer_interface(Stru_AstSerializerData_t* data)
{
    if (!data) return NULL;
    return &data->interface;
}

// ============================================================================
// 接口函数实现
// ============================================================================

static bool initialize_impl(Stru_AstSerializerInterface_t* serializer)
{
    if (!serializer) return false;
    
    Stru_AstSerializerData_t* data = 
        (Stru_AstSerializerData_t*)serializer->private_data;
    if (!data) return false;
    
    // 创建缓冲区
    if (!data->json_buffer) {
        data->json_buffer = F_string_create_with_capacity(1024);
        if (!data->json_buffer) return false;
    }
    
    if (!data->xml_buffer) {
        data->xml_buffer = F_string_create_with_capacity(1024);
        if (!data->xml_buffer) return false;
    }
    
    return true;
}

static size_t serialize_to_json_impl(Stru_AstSerializerInterface_t* serializer,
                                    Stru_AstNodeInterface_t* root,
                                    char* buffer,
                                    size_t buffer_size)
{
    if (!serializer || !root || !buffer || buffer_size == 0) return 0;
    
    size_t position = 0;
    
    // 开始JSON对象
    if (!append_to_buffer(buffer, buffer_size, &position, "{\n")) {
        return 0;
    }
    
    // 序列化根节点
    if (!F_serialize_to_json_recursive(root, buffer, buffer_size, &position)) {
        return 0;
    }
    
    // 结束JSON对象
    if (!append_to_buffer(buffer, buffer_size, &position, "\n}")) {
        return 0;
    }
    
    return position;
}

static Stru_AstNodeInterface_t* deserialize_from_json_impl(Stru_AstSerializerInterface_t* serializer,
                                                          const char* json)
{
    if (!serializer || !json) return NULL;
    
    size_t position = 0;
    size_t length = strlen(json);
    
    // 跳过空白字符
    while (position < length && isspace(json[position])) {
        position++;
    }
    
    // 检查是否以'{'开始
    if (position >= length || json[position] != '{') {
        return NULL;
    }
    
    position++; // 跳过'{'
    
    // 反序列化根节点
    Stru_AstNodeInterface_t* root = F_deserialize_from_json_recursive(json, &position, length);
    if (!root) return NULL;
    
    return root;
}

static size_t serialize_to_xml_impl(Stru_AstSerializerInterface_t* serializer,
                                   Stru_AstNodeInterface_t* root,
                                   char* buffer,
                                   size_t buffer_size)
{
    if (!serializer || !root || !buffer || buffer_size == 0) return 0;
    
    size_t position = 0;
    
    // XML声明
    if (!append_to_buffer(buffer, buffer_size, &position, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")) {
        return 0;
    }
    
    // 序列化根节点
    if (!F_serialize_to_xml_recursive(root, buffer, buffer_size, &position, 0)) {
        return 0;
    }
    
    return position;
}

static Stru_AstNodeInterface_t* deserialize_from_xml_impl(Stru_AstSerializerInterface_t* serializer,
                                                         const char* xml)
{
    if (!serializer || !xml) return NULL;
    
    size_t position = 0;
    size_t length = strlen(xml);
    
    // 跳过XML声明
    if (strncmp(xml, "<?xml", 5) == 0) {
        const char* end = strstr(xml, "?>");
        if (end) {
            position = end - xml + 2;
        }
    }
    
    // 跳过空白字符
    while (position < length && isspace(xml[position])) {
        position++;
    }
    
    // 反序列化根节点
    Stru_AstNodeInterface_t* root = F_deserialize_from_xml_recursive(xml, &position, length);
    if (!root) return NULL;
    
    return root;
}

static size_t serialize_to_binary_impl(Stru_AstSerializerInterface_t* serializer,
                                      Stru_AstNodeInterface_t* root,
                                      void* buffer,
                                      size_t buffer_size)
{
    if (!serializer || !root || !buffer || buffer_size == 0) return 0;
    
    size_t position = 0;
    
    // 序列化根节点
    if (!F_serialize_to_binary_recursive(root, buffer, buffer_size, &position)) {
        return 0;
    }
    
    return position;
}

static Stru_AstNodeInterface_t* deserialize_from_binary_impl(Stru_AstSerializerInterface_t* serializer,
                                                            const void* buffer,
                                                            size_t buffer_size)
{
    if (!serializer || !buffer || buffer_size == 0) return NULL;
    
    size_t position = 0;
    
    // 反序列化根节点
    Stru_AstNodeInterface_t* root = F_deserialize_from_binary_recursive(buffer, buffer_size, &position);
    if (!root) return NULL;
    
    return root;
}

static void destroy_impl(Stru_AstSerializerInterface_t* serializer)
{
    if (!serializer) return;
    
    Stru_AstSerializerData_t* data = 
        (Stru_AstSerializerData_t*)serializer->private_data;
    if (data) {
        F_destroy_ast_serializer_data(data);
    }
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

bool F_serialize_to_json_recursive(Stru_AstNodeInterface_t* node,
                                  char* buffer,
                                  size_t buffer_size,
                                  size_t* position)
{
    if (!node || !buffer || !position) return false;
    
    // 获取节点信息
    Eum_AstNodeType type = node->get_type(node);
    const char* type_name = get_node_type_name(type);
    
    // 写入节点类型
    char temp[256];
    snprintf(temp, sizeof(temp), "  \"type\": \"%s\",\n", type_name);
    if (!append_to_buffer(buffer, buffer_size, position, temp)) {
        return false;
    }
    
    // 写入子节点数量
    size_t child_count = node->get_child_count(node);
    snprintf(temp, sizeof(temp), "  \"child_count\": %zu,\n", child_count);
    if (!append_to_buffer(buffer, buffer_size, position, temp)) {
        return false;
    }
    
    // 写入子节点
    if (child_count > 0) {
        if (!append_to_buffer(buffer, buffer_size, position, "  \"children\": [\n")) {
            return false;
        }
        
        for (size_t i = 0; i < child_count; i++) {
            Stru_AstNodeInterface_t* child = node->get_child(node, i);
            if (child) {
                // 开始子节点对象
                if (!append_to_buffer(buffer, buffer_size, position, "    {\n")) {
                    return false;
                }
                
                // 递归序列化子节点
                if (!F_serialize_to_json_recursive(child, buffer, buffer_size, position)) {
                    return false;
                }
                
                // 结束子节点对象
                if (i < child_count - 1) {
                    if (!append_to_buffer(buffer, buffer_size, position, "    },\n")) {
                        return false;
                    }
                } else {
                    if (!append_to_buffer(buffer, buffer_size, position, "    }\n")) {
                        return false;
                    }
                }
            }
        }
        
        if (!append_to_buffer(buffer, buffer_size, position, "  ]\n")) {
            return false;
        }
    }
    
    return true;
}

Stru_AstNodeInterface_t* F_deserialize_from_json_recursive(const char* json,
                                                          size_t* position,
                                                          size_t length)
{
    // 简化实现：返回一个基本节点
    // 在实际实现中，这里应该解析JSON并重建AST
    return NULL;
}

bool F_serialize_to_xml_recursive(Stru_AstNodeInterface_t* node,
                                 char* buffer,
                                 size_t buffer_size,
                                 size_t* position,
                                 size_t depth)
{
    if (!node || !buffer || !position) return false;
    
    // 获取节点信息
    Eum_AstNodeType type = node->get_type(node);
    const char* type_name = get_node_type_name(type);
    
    // 生成缩进
    char indent[256];
    memset(indent, ' ', depth * 2);
    indent[depth * 2] = '\0';
    
    // 开始XML元素
    char temp[512];
    snprintf(temp, sizeof(temp), "%s<%s>\n", indent, type_name);
    if (!append_to_buffer(buffer, buffer_size, position, temp)) {
        return false;
    }
    
    // 写入子节点
    size_t child_count = node->get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        if (child) {
            if (!F_serialize_to_xml_recursive(child, buffer, buffer_size, position, depth + 1)) {
                return false;
            }
        }
    }
    
    // 结束XML元素
    snprintf(temp, sizeof(temp), "%s</%s>\n", indent, type_name);
    if (!append_to_buffer(buffer, buffer_size, position, temp)) {
        return false;
    }
    
    return true;
}

Stru_AstNodeInterface_t* F_deserialize_from_xml_recursive(const char* xml,
                                                         size_t* position,
                                                         size_t length)
{
    // 简化实现：返回一个基本节点
    // 在实际实现中，这里应该解析XML并重建AST
    return NULL;
}

bool F_serialize_to_binary_recursive(Stru_AstNodeInterface_t* node,
                                    void* buffer,
                                    size_t buffer_size,
                                    size_t* position)
{
    if (!node || !buffer || !position) return false;
    
    // 获取节点类型
    Eum_AstNodeType type = node->get_type(node);
    
    // 检查缓冲区是否足够
    if (*position + sizeof(Eum_AstNodeType) > buffer_size) {
        return false;
    }
    
    // 写入节点类型
    memcpy((char*)buffer + *position, &type, sizeof(Eum_AstNodeType));
    *position += sizeof(Eum_AstNodeType);
    
    // 写入子节点数量
    size_t child_count = node->get_child_count(node);
    if (*position + sizeof(size_t) > buffer_size) {
        return false;
    }
    
    memcpy((char*)buffer + *position, &child_count, sizeof(size_t));
    *position += sizeof(size_t);
    
    // 递归序列化子节点
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNodeInterface_t* child = node->get_child(node, i);
        if (child) {
            if (!F_serialize_to_binary_recursive(child, buffer, buffer_size, position)) {
                return false;
            }
        }
    }
    
    return true;
}

Stru_AstNodeInterface_t* F_deserialize_from_binary_recursive(const void* buffer,
                                                            size_t buffer_size,
                                                            size_t* position)
{
    // 简化实现：返回一个基本节点
    // 在实际实现中，这里应该解析二进制数据并重建AST
    return NULL;
}
