/**
 * @file CN_ast.h
 * @brief CN_Language 抽象语法树（AST）定义 - 兼容层
 * 
 * 提供向后兼容的AST API，内部使用新的接口实现。
 * 遵循项目架构规范，支持语法分析器的AST构建需求。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_AST_H
#define CN_AST_H

#include <stddef.h>
#include <stdbool.h>
#include "CN_ast_interface.h"

// ============================================================================
// 类型定义 - 保持向后兼容
// ============================================================================

/**
 * @brief AST节点类型枚举
 * 
 * 定义所有可能的AST节点类型，涵盖CN_Language的所有语法结构。
 * 注意：此枚举与CN_ast_interface.h中的Eum_AstNodeType相同。
 */
typedef Eum_AstNodeType Eum_AstNodeType;

/**
 * @brief AST节点结构体（兼容层）
 * 
 * 提供向后兼容的AST节点结构体，内部使用接口实现。
 */
typedef struct Stru_AstNode_t {
    Stru_AstNodeInterface_t* interface;  ///< 内部接口实现
    void* private_data;                  ///< 私有数据（用于兼容层）
} Stru_AstNode_t;

// ============================================================================
// AST节点创建和销毁函数（兼容层）
// ============================================================================

/**
 * @brief 创建AST节点
 * 
 * 分配并初始化一个新的AST节点。
 * 
 * @param type 节点类型
 * @param token 关联的令牌（可选）
 * @param line 行号
 * @param column 列号
 * @return Stru_AstNode_t* 新创建的AST节点指针，失败返回NULL
 */
Stru_AstNode_t* F_create_ast_node(Eum_AstNodeType type, void* token, size_t line, size_t column);

/**
 * @brief 销毁AST节点
 * 
 * 释放AST节点及其所有子节点占用的内存。
 * 
 * @param node 要销毁的AST节点指针
 */
void F_destroy_ast_node(Stru_AstNode_t* node);

/**
 * @brief 复制AST节点
 * 
 * 创建AST节点的深拷贝。
 * 
 * @param node 要复制的AST节点指针
 * @return Stru_AstNode_t* 新复制的AST节点指针
 */
Stru_AstNode_t* F_copy_ast_node(const Stru_AstNode_t* node);

// ============================================================================
// AST节点操作函数（兼容层）
// ============================================================================

/**
 * @brief 添加子节点
 * 
 * 向父节点添加一个子节点。
 * 
 * @param parent 父节点
 * @param child 子节点
 * @return true 添加成功
 * @return false 添加失败
 */
bool F_ast_add_child(Stru_AstNode_t* parent, Stru_AstNode_t* child);

/**
 * @brief 获取子节点数量
 * 
 * 获取节点的子节点数量。
 * 
 * @param node 节点
 * @return size_t 子节点数量
 */
size_t F_ast_get_child_count(const Stru_AstNode_t* node);

/**
 * @brief 获取子节点
 * 
 * 获取指定索引的子节点。
 * 
 * @param node 节点
 * @param index 子节点索引
 * @return Stru_AstNode_t* 子节点指针，索引无效返回NULL
 */
Stru_AstNode_t* F_ast_get_child(const Stru_AstNode_t* node, size_t index);

/**
 * @brief 移除子节点
 * 
 * 移除指定索引的子节点（不销毁）。
 * 
 * @param node 节点
 * @param index 子节点索引
 * @return Stru_AstNode_t* 被移除的子节点指针，索引无效返回NULL
 */
Stru_AstNode_t* F_ast_remove_child(Stru_AstNode_t* node, size_t index);

// ============================================================================
// AST节点属性操作函数（兼容层）
// ============================================================================

/**
 * @brief 设置节点属性
 * 
 * 设置节点的属性值。
 * 
 * @param node 节点
 * @param key 属性键
 * @param value 属性值
 * @return true 设置成功
 * @return false 设置失败
 */
bool F_ast_set_attribute(Stru_AstNode_t* node, const char* key, void* value);

/**
 * @brief 获取节点属性
 * 
 * 获取节点的属性值。
 * 
 * @param node 节点
 * @param key 属性键
 * @return void* 属性值，不存在返回NULL
 */
void* F_ast_get_attribute(const Stru_AstNode_t* node, const char* key);

/**
 * @brief 移除节点属性
 * 
 * 移除节点的属性。
 * 
 * @param node 节点
 * @param key 属性键
 * @return true 移除成功
 * @return false 移除失败（属性不存在）
 */
bool F_ast_remove_attribute(Stru_AstNode_t* node, const char* key);

/**
 * @brief 检查节点是否有属性
 * 
 * 检查节点是否具有指定属性。
 * 
 * @param node 节点
 * @param key 属性键
 * @return true 有属性
 * @return false 无属性
 */
bool F_ast_has_attribute(const Stru_AstNode_t* node, const char* key);

// ============================================================================
// AST节点数据操作函数（兼容层）
// ============================================================================

/**
 * @brief 设置整数值
 * 
 * 为节点设置整数值。
 * 
 * @param node 节点
 * @param value 整数值
 */
void F_ast_set_int_value(Stru_AstNode_t* node, long value);

/**
 * @brief 设置浮点数值
 * 
 * 为节点设置浮点数值。
 * 
 * @param node 节点
 * @param value 浮点数值
 */
void F_ast_set_float_value(Stru_AstNode_t* node, double value);

/**
 * @brief 设置字符串值
 * 
 * 为节点设置字符串值。
 * 
 * @param node 节点
 * @param value 字符串值（会被复制）
 */
void F_ast_set_string_value(Stru_AstNode_t* node, const char* value);

/**
 * @brief 设置布尔值
 * 
 * 为节点设置布尔值。
 * 
 * @param node 节点
 * @param value 布尔值
 */
void F_ast_set_bool_value(Stru_AstNode_t* node, bool value);

/**
 * @brief 设置标识符名称
 * 
 * 为节点设置标识符名称。
 * 
 * @param node 节点
 * @param name 标识符名称（会被复制）
 */
void F_ast_set_identifier(Stru_AstNode_t* node, const char* name);

/**
 * @brief 设置类型名称
 * 
 * 为节点设置类型名称。
 * 
 * @param node 节点
 * @param type_name 类型名称（会被复制）
 */
void F_ast_set_type_name(Stru_AstNode_t* node, const char* type_name);

/**
 * @brief 设置数组大小
 * 
 * 为节点设置数组大小。
 * 
 * @param node 节点
 * @param size 数组大小
 */
void F_ast_set_array_size(Stru_AstNode_t* node, size_t size);

/**
 * @brief 设置运算符类型
 * 
 * 为节点设置运算符类型。
 * 
 * @param node 节点
 * @param operator_type 运算符类型
 */
void F_ast_set_operator_type(Stru_AstNode_t* node, int operator_type);

/**
 * @brief 获取整数值
 * 
 * 获取节点的整数值。
 * 
 * @param node 节点
 * @return long 整数值
 */
long F_ast_get_int_value(const Stru_AstNode_t* node);

/**
 * @brief 获取浮点数值
 * 
 * 获取节点的浮点数值。
 * 
 * @param node 节点
 * @return double 浮点数值
 */
double F_ast_get_float_value(const Stru_AstNode_t* node);

/**
 * @brief 获取字符串值
 * 
 * 获取节点的字符串值。
 * 
 * @param node 节点
 * @return const char* 字符串值
 */
const char* F_ast_get_string_value(const Stru_AstNode_t* node);

/**
 * @brief 获取布尔值
 * 
 * 获取节点的布尔值。
 * 
 * @param node 节点
 * @return bool 布尔值
 */
bool F_ast_get_bool_value(const Stru_AstNode_t* node);

/**
 * @brief 获取标识符名称
 * 
 * 获取节点的标识符名称。
 * 
 * @param node 节点
 * @return const char* 标识符名称
 */
const char* F_ast_get_identifier(const Stru_AstNode_t* node);

/**
 * @brief 获取类型名称
 * 
 * 获取节点的类型名称。
 * 
 * @param node 节点
 * @return const char* 类型名称
 */
const char* F_ast_get_type_name(const Stru_AstNode_t* node);

/**
 * @brief 获取数组大小
 * 
 * 获取节点的数组大小。
 * 
 * @param node 节点
 * @return size_t 数组大小
 */
size_t F_ast_get_array_size(const Stru_AstNode_t* node);

/**
 * @brief 获取运算符类型
 * 
 * 获取节点的运算符类型。
 * 
 * @param node 节点
 * @return int 运算符类型
 */
int F_ast_get_operator_type(const Stru_AstNode_t* node);

// ============================================================================
// AST遍历和查询函数（兼容层）
// ============================================================================

/**
 * @brief AST节点类型转字符串
 * 
 * 将AST节点类型转换为可读的字符串表示。
 * 
 * @param type AST节点类型
 * @return const char* 类型字符串表示
 */
const char* F_ast_node_type_to_string(Eum_AstNodeType type);

/**
 * @brief 打印AST节点
 * 
 * 将AST节点信息格式化为字符串输出。
 * 
 * @param node AST节点
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 写入的字符数（不包括终止空字符）
 */
int F_ast_node_to_string(const Stru_AstNode_t* node, char* buffer, size_t buffer_size);

/**
 * @brief 打印整个AST树
 * 
 * 以树形结构打印整个AST。
 * 
 * @param root AST根节点
 * @param indent 初始缩进级别
 */
void F_ast_print_tree(const Stru_AstNode_t* root, int indent);

/**
 * @brief 查找节点
 * 
 * 在AST树中查找满足条件的节点。
 * 
 * @param root 根节点
 * @param predicate 谓词函数，返回true表示匹配
 * @param context 上下文参数（传递给谓词函数）
 * @return void* 匹配的节点数组，NULL表示无匹配
 */
void* F_ast_find_nodes(const Stru_AstNode_t* root, 
                       bool (*predicate)(const Stru_AstNode_t*, void*), 
                       void* context);

/**
 * @brief 遍历AST树
 * 
 * 遍历AST树并对每个节点执行操作。
 * 
 * @param root 根节点
 * @param visitor 访问者函数，对每个节点执行操作
 * @param context 上下文参数（传递给访问者函数）
 */
void F_ast_traverse(Stru_AstNode_t* root, 
                   void (*visitor)(Stru_AstNode_t*, void*), 
                   void* context);

// ============================================================================
// AST验证函数（兼容层）
// ============================================================================

/**
 * @brief 验证AST节点
 * 
 * 验证AST节点的结构是否有效。
 * 
 * @param node 要验证的节点
 * @return true 节点有效
 * @return false 节点无效
 */
bool F_ast_validate_node(const Stru_AstNode_t* node);

/**
 * @brief 验证整个AST树
 * 
 * 验证整个AST树的结构是否有效。
 * 
 * @param root 根节点
 * @return true AST树有效
 * @return false AST树无效
 */
bool F_ast_validate_tree(const Stru_AstNode_t* root);

// ============================================================================
// 新接口访问函数
// ============================================================================

/**
 * @brief 获取内部AST节点接口
 * 
 * 获取AST节点的内部接口实现。
 * 
 * @param node AST节点
 * @return Stru_AstNodeInterface_t* 内部接口指针
 */
Stru_AstNodeInterface_t* F_ast_get_internal_interface(const Stru_AstNode_t* node);

/**
 * @brief 创建AST构建器接口实例
 * 
 * 创建新的AST构建器接口实例。
 * 
 * @return Stru_AstBuilderInterface_t* AST构建器接口实例，失败返回NULL
 */
Stru_AstBuilderInterface_t* F_ast_create_builder_interface(void);

/**
 * @brief 创建AST遍历接口实例
 * 
 * 创建新的AST遍历接口实例。
 * 
 * @return Stru_AstTraversalInterface_t* AST遍历接口实例，失败返回NULL
 */
Stru_AstTraversalInterface_t* F_ast_create_traversal_interface(void);

/**
 * @brief 创建AST查询接口实例
 * 
 * 创建新的AST查询接口实例。
 * 
 * @return Stru_AstQueryInterface_t* AST查询接口实例，失败返回NULL
 */
Stru_AstQueryInterface_t* F_ast_create_query_interface(void);

/**
 * @brief 创建AST序列化接口实例
 * 
 * 创建新的AST序列化接口实例。
 * 
 * @return Stru_AstSerializerInterface_t* AST序列化接口实例，失败返回NULL
 */
Stru_AstSerializerInterface_t* F_ast_create_serializer_interface(void);

#endif /* CN_AST_H */
