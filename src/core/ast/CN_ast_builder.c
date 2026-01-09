/**
 * @file CN_ast_builder.c
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

#include "CN_ast_builder.h"
#include "CN_ast_node.h"
#include "../../infrastructure/memory/CN_memory_interface.h"
#include "../../infrastructure/utils/CN_utils_interface.h"
#include <string.h>

// ============================================================================
// 静态函数声明
// ============================================================================

static bool initialize_impl(Stru_AstBuilderInterface_t* builder);
static Stru_AstNodeInterface_t* create_node_impl(Stru_AstBuilderInterface_t* builder,
                                                Eum_AstNodeType type,
                                                const Stru_AstNodeLocation_t* location);
static Stru_AstNodeInterface_t* create_literal_node_impl(Stru_AstBuilderInterface_t* builder,
                                                        Eum_AstNodeType type,
                                                        const Stru_AstNodeLocation_t* location,
                                                        const Uni_AstNodeData_t* data);
static Stru_AstNodeInterface_t* create_identifier_node_impl(Stru_AstBuilderInterface_t* builder,
                                                           const Stru_AstNodeLocation_t* location,
                                                           const char* identifier);
static Stru_AstNodeInterface_t* create_type_node_impl(Stru_AstBuilderInterface_t* builder,
                                                     const Stru_AstNodeLocation_t* location,
                                                     const char* type_name);
static Stru_AstNodeInterface_t* create_operator_node_impl(Stru_AstBuilderInterface_t* builder,
                                                         Eum_AstNodeType type,
                                                         const Stru_AstNodeLocation_t* location,
                                                         int operator_type);
static Stru_AstNodeInterface_t* build_program_node_impl(Stru_AstBuilderInterface_t* builder,
                                                       const Stru_AstNodeLocation_t* location,
                                                       const char* module_name,
                                                       Stru_AstNodeInterface_t** declarations,
                                                       size_t declaration_count);
static Stru_AstNodeInterface_t* build_function_declaration_impl(Stru_AstBuilderInterface_t* builder,
                                                               const Stru_AstNodeLocation_t* location,
                                                               const char* function_name,
                                                               Stru_AstNodeInterface_t* return_type,
                                                               Stru_AstNodeInterface_t** parameters,
                                                               size_t parameter_count,
                                                               Stru_AstNodeInterface_t* body);
static Stru_AstNodeInterface_t* build_variable_declaration_impl(Stru_AstBuilderInterface_t* builder,
                                                               const Stru_AstNodeLocation_t* location,
                                                               const char* variable_name,
                                                               Stru_AstNodeInterface_t* type,
                                                               Stru_AstNodeInterface_t* initializer);
static Stru_AstNodeInterface_t* build_expression_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                               const Stru_AstNodeLocation_t* location,
                                                               Stru_AstNodeInterface_t* expression);
static Stru_AstNodeInterface_t* build_if_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                       const Stru_AstNodeLocation_t* location,
                                                       Stru_AstNodeInterface_t* condition,
                                                       Stru_AstNodeInterface_t* then_branch,
                                                       Stru_AstNodeInterface_t* else_branch);
static Stru_AstNodeInterface_t* build_while_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* condition,
                                                          Stru_AstNodeInterface_t* body);
static Stru_AstNodeInterface_t* build_for_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                        const Stru_AstNodeLocation_t* location,
                                                        Stru_AstNodeInterface_t* init,
                                                        Stru_AstNodeInterface_t* condition,
                                                        Stru_AstNodeInterface_t* update,
                                                        Stru_AstNodeInterface_t* body);
static Stru_AstNodeInterface_t* build_return_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                           const Stru_AstNodeLocation_t* location,
                                                           Stru_AstNodeInterface_t* expression);
static Stru_AstNodeInterface_t* build_binary_expression_impl(Stru_AstBuilderInterface_t* builder,
                                                            const Stru_AstNodeLocation_t* location,
                                                            int operator_type,
                                                            Stru_AstNodeInterface_t* left,
                                                            Stru_AstNodeInterface_t* right);
static Stru_AstNodeInterface_t* build_unary_expression_impl(Stru_AstBuilderInterface_t* builder,
                                                           const Stru_AstNodeLocation_t* location,
                                                           int operator_type,
                                                           Stru_AstNodeInterface_t* operand);
static Stru_AstNodeInterface_t* build_call_expression_impl(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* function,
                                                          Stru_AstNodeInterface_t** arguments,
                                                          size_t argument_count);
static Stru_AstNodeInterface_t* build_try_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                        const Stru_AstNodeLocation_t* location,
                                                        Stru_AstNodeInterface_t* try_block,
                                                        Stru_AstNodeInterface_t** catch_clauses,
                                                        size_t catch_clause_count,
                                                        Stru_AstNodeInterface_t* finally_block);
static Stru_AstNodeInterface_t* build_catch_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* exception_type,
                                                          const char* exception_name,
                                                          Stru_AstNodeInterface_t* catch_block);
static Stru_AstNodeInterface_t* build_throw_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* expression);
static Stru_AstNodeInterface_t* build_finally_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                            const Stru_AstNodeLocation_t* location,
                                                            Stru_AstNodeInterface_t* finally_block);
static Stru_AstNodeInterface_t* build_switch_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                           const Stru_AstNodeLocation_t* location,
                                                           Stru_AstNodeInterface_t* expression,
                                                           Stru_AstNodeInterface_t** cases,
                                                           size_t case_count,
                                                           Stru_AstNodeInterface_t* default_case);
static Stru_AstNodeInterface_t* build_case_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                         const Stru_AstNodeLocation_t* location,
                                                         Stru_AstNodeInterface_t* case_expression,
                                                         Stru_AstNodeInterface_t* case_body);
static Stru_AstNodeInterface_t* build_default_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                            const Stru_AstNodeLocation_t* location,
                                                            Stru_AstNodeInterface_t* default_body);
static Stru_AstNodeInterface_t* build_array_literal_impl(Stru_AstBuilderInterface_t* builder,
                                                        const Stru_AstNodeLocation_t* location,
                                                        Stru_AstNodeInterface_t** elements,
                                                        size_t element_count);
static Stru_AstNodeInterface_t* build_struct_literal_impl(Stru_AstBuilderInterface_t* builder,
                                                         const Stru_AstNodeLocation_t* location,
                                                         const char* struct_type_name,
                                                         Stru_AstNodeInterface_t** members,
                                                         size_t member_count);
static void destroy_impl(Stru_AstBuilderInterface_t* builder);

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

// ============================================================================
// 工厂函数实现
// ============================================================================

Stru_AstBuilderInternalData_t* F_create_ast_builder_internal_data(void)
{
    Stru_AstBuilderInternalData_t* internal_data = 
        (Stru_AstBuilderInternalData_t*)cn_malloc(sizeof(Stru_AstBuilderInternalData_t));
    if (!internal_data) return NULL;
    
    internal_data->current_node = NULL;
    internal_data->root_node = NULL;
    internal_data->node_count = 0;
    internal_data->builder_context = NULL;
    
    return internal_data;
}

void F_destroy_ast_builder_internal_data(Stru_AstBuilderInternalData_t* internal_data)
{
    if (!internal_data) return;
    
    // 注意：不销毁节点，因为节点有自己的生命周期管理
    cn_free(internal_data);
}

Stru_AstBuilderInterface_t* F_create_ast_builder_interface_impl(void)
{
    Stru_AstBuilderInterface_t* interface = 
        (Stru_AstBuilderInterface_t*)cn_malloc(sizeof(Stru_AstBuilderInterface_t));
    if (!interface) return NULL;
    
    // 设置函数指针
    interface->initialize = initialize_impl;
    interface->create_node = create_node_impl;
    interface->create_literal_node = create_literal_node_impl;
    interface->create_identifier_node = create_identifier_node_impl;
    interface->create_type_node = create_type_node_impl;
    interface->create_operator_node = create_operator_node_impl;
    interface->build_program_node = build_program_node_impl;
    interface->build_function_declaration = build_function_declaration_impl;
    interface->build_variable_declaration = build_variable_declaration_impl;
    interface->build_expression_statement = build_expression_statement_impl;
    interface->build_if_statement = build_if_statement_impl;
    interface->build_while_statement = build_while_statement_impl;
    interface->build_for_statement = build_for_statement_impl;
    interface->build_return_statement = build_return_statement_impl;
    interface->build_binary_expression = build_binary_expression_impl;
    interface->build_unary_expression = build_unary_expression_impl;
    interface->build_call_expression = build_call_expression_impl;
    interface->build_try_statement = build_try_statement_impl;
    interface->build_catch_statement = build_catch_statement_impl;
    interface->build_throw_statement = build_throw_statement_impl;
    interface->build_finally_statement = build_finally_statement_impl;
    interface->build_switch_statement = build_switch_statement_impl;
    interface->build_case_statement = build_case_statement_impl;
    interface->build_default_statement = build_default_statement_impl;
    interface->build_array_literal = build_array_literal_impl;
    interface->build_struct_literal = build_struct_literal_impl;
    interface->destroy = destroy_impl;
    
    // 创建内部数据
    interface->private_data = F_create_ast_builder_internal_data();
    if (!interface->private_data) {
        cn_free(interface);
        return NULL;
    }
    
    return interface;
}

Stru_AstBuilderInterface_t* F_get_ast_builder_interface_from_internal_data(
    Stru_AstBuilderInternalData_t* internal_data)
{
    if (!internal_data) return NULL;
    
    Stru_AstBuilderInterface_t* interface = 
        (Stru_AstBuilderInterface_t*)cn_malloc(sizeof(Stru_AstBuilderInterface_t));
    if (!interface) return NULL;
    
    // 设置函数指针
    interface->initialize = initialize_impl;
    interface->create_node = create_node_impl;
    interface->create_literal_node = create_literal_node_impl;
    interface->create_identifier_node = create_identifier_node_impl;
    interface->create_type_node = create_type_node_impl;
    interface->create_operator_node = create_operator_node_impl;
    interface->build_program_node = build_program_node_impl;
    interface->build_function_declaration = build_function_declaration_impl;
    interface->build_variable_declaration = build_variable_declaration_impl;
    interface->build_expression_statement = build_expression_statement_impl;
    interface->build_if_statement = build_if_statement_impl;
    interface->build_while_statement = build_while_statement_impl;
    interface->build_for_statement = build_for_statement_impl;
    interface->build_return_statement = build_return_statement_impl;
    interface->build_binary_expression = build_binary_expression_impl;
    interface->build_unary_expression = build_unary_expression_impl;
    interface->build_call_expression = build_call_expression_impl;
    interface->build_try_statement = build_try_statement_impl;
    interface->build_catch_statement = build_catch_statement_impl;
    interface->build_throw_statement = build_throw_statement_impl;
    interface->build_finally_statement = build_finally_statement_impl;
    interface->build_switch_statement = build_switch_statement_impl;
    interface->build_case_statement = build_case_statement_impl;
    interface->build_default_statement = build_default_statement_impl;
    interface->build_array_literal = build_array_literal_impl;
    interface->build_struct_literal = build_struct_literal_impl;
    interface->destroy = destroy_impl;
    
    interface->private_data = internal_data;
    
    return interface;
}

Stru_AstBuilderInternalData_t* F_get_internal_data_from_ast_builder_interface(
    const Stru_AstBuilderInterface_t* builder_interface)
{
    if (!builder_interface) return NULL;
    return (Stru_AstBuilderInternalData_t*)builder_interface->private_data;
}

// ============================================================================
// 接口函数实现
// ============================================================================

static bool initialize_impl(Stru_AstBuilderInterface_t* builder)
{
    Stru_AstBuilderInternalData_t* internal_data = 
        F_get_internal_data_from_ast_builder_interface(builder);
    if (!internal_data) return false;
    
    // 重置构建器状态
    internal_data->current_node = NULL;
    internal_data->root_node = NULL;
    internal_data->node_count = 0;
    
    return true;
}

static Stru_AstNodeInterface_t* create_node_impl(Stru_AstBuilderInterface_t* builder,
                                                Eum_AstNodeType type,
                                                const Stru_AstNodeLocation_t* location)
{
    Stru_AstBuilderInternalData_t* internal_data = 
        F_get_internal_data_from_ast_builder_interface(builder);
    if (!internal_data) return NULL;
    
    // 创建节点
    Stru_AstNodeInterface_t* node = 
        F_create_ast_node_interface_with_type(type, location);
    if (!node) return NULL;
    
    // 更新构建器状态
    internal_data->node_count++;
    
    if (!internal_data->root_node) {
        internal_data->root_node = node;
    }
    
    internal_data->current_node = node;
    
    return node;
}

static Stru_AstNodeInterface_t* create_literal_node_impl(Stru_AstBuilderInterface_t* builder,
                                                        Eum_AstNodeType type,
                                                        const Stru_AstNodeLocation_t* location,
                                                        const Uni_AstNodeData_t* data)
{
    Stru_AstNodeInterface_t* node = create_node_impl(builder, type, location);
    if (!node) return NULL;
    
    if (data) {
        node->set_data(node, data);
    }
    
    return node;
}

static Stru_AstNodeInterface_t* create_identifier_node_impl(Stru_AstBuilderInterface_t* builder,
                                                           const Stru_AstNodeLocation_t* location,
                                                           const char* identifier)
{
    Stru_AstNodeInterface_t* node = create_node_impl(builder, Eum_AST_IDENTIFIER_EXPR, location);
    if (!node) return NULL;
    
    if (identifier) {
        Uni_AstNodeData_t data;
        memset(&data, 0, sizeof(Uni_AstNodeData_t));
        data.identifier = cn_strdup(identifier);
        node->set_data(node, &data);
    }
    
    return node;
}

static Stru_AstNodeInterface_t* create_type_node_impl(Stru_AstBuilderInterface_t* builder,
                                                     const Stru_AstNodeLocation_t* location,
                                                     const char* type_name)
{
    Stru_AstNodeInterface_t* node = create_node_impl(builder, Eum_AST_TYPE_NAME, location);
    if (!node) return NULL;
    
    if (type_name) {
        Uni_AstNodeData_t data;
        memset(&data, 0, sizeof(Uni_AstNodeData_t));
        data.type_name = cn_strdup(type_name);
        node->set_data(node, &data);
    }
    
    return node;
}

static Stru_AstNodeInterface_t* create_operator_node_impl(Stru_AstBuilderInterface_t* builder,
                                                         Eum_AstNodeType type,
                                                         const Stru_AstNodeLocation_t* location,
                                                         int operator_type)
{
    Stru_AstNodeInterface_t* node = create_node_impl(builder, type, location);
    if (!node) return NULL;
    
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.operator_type = operator_type;
    node->set_data(node, &data);
    
    return node;
}

static Stru_AstNodeInterface_t* build_program_node_impl(Stru_AstBuilderInterface_t* builder,
                                                       const Stru_AstNodeLocation_t* location,
                                                       const char* module_name,
                                                       Stru_AstNodeInterface_t** declarations,
                                                       size_t declaration_count)
{
    Stru_AstNodeInterface_t* program_node = create_node_impl(builder, Eum_AST_PROGRAM, location);
    if (!program_node) return NULL;
    
    // 设置模块名称
    if (module_name) {
        Uni_AstNodeData_t data;
        memset(&data, 0, sizeof(Uni_AstNodeData_t));
        data.identifier = cn_strdup(module_name);
        program_node->set_data(program_node, &data);
    }
    
    // 添加声明节点
    for (size_t i = 0; i < declaration_count; i++) {
        if (declarations[i]) {
            program_node->add_child(program_node, declarations[i]);
        }
    }
    
    return program_node;
}

static Stru_AstNodeInterface_t* build_function_declaration_impl(Stru_AstBuilderInterface_t* builder,
                                                               const Stru_AstNodeLocation_t* location,
                                                               const char* function_name,
                                                               Stru_AstNodeInterface_t* return_type,
                                                               Stru_AstNodeInterface_t** parameters,
                                                               size_t parameter_count,
                                                               Stru_AstNodeInterface_t* body)
{
    Stru_AstNodeInterface_t* func_node = create_node_impl(builder, Eum_AST_FUNCTION_DECL, location);
    if (!func_node) return NULL;
    
    // 设置函数名称
    if (function_name) {
        Uni_AstNodeData_t data;
        memset(&data, 0, sizeof(Uni_AstNodeData_t));
        data.identifier = cn_strdup(function_name);
        func_node->set_data(func_node, &data);
    }
    
    // 添加返回类型
    if (return_type) {
        func_node->add_child(func_node, return_type);
    }
    
    // 添加参数
    for (size_t i = 0; i < parameter_count; i++) {
        if (parameters[i]) {
            func_node->add_child(func_node, parameters[i]);
        }
    }
    
    // 添加函数体
    if (body) {
        func_node->add_child(func_node, body);
    }
    
    return func_node;
}

static Stru_AstNodeInterface_t* build_variable_declaration_impl(Stru_AstBuilderInterface_t* builder,
                                                               const Stru_AstNodeLocation_t* location,
                                                               const char* variable_name,
                                                               Stru_AstNodeInterface_t* type,
                                                               Stru_AstNodeInterface_t* initializer)
{
    Stru_AstNodeInterface_t* var_node = create_node_impl(builder, Eum_AST_VARIABLE_DECL, location);
    if (!var_node) return NULL;
    
    // 设置变量名称
    if (variable_name) {
        Uni_AstNodeData_t data;
        memset(&data, 0, sizeof(Uni_AstNodeData_t));
        data.identifier = cn_strdup(variable_name);
        var_node->set_data(var_node, &data);
    }
    
    // 添加类型
    if (type) {
        var_node->add_child(var_node, type);
    }
    
    // 添加初始化表达式
    if (initializer) {
        var_node->add_child(var_node, initializer);
    }
    
    return var_node;
}

static Stru_AstNodeInterface_t* build_expression_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                               const Stru_AstNodeLocation_t* location,
                                                               Stru_AstNodeInterface_t* expression)
{
    Stru_AstNodeInterface_t* stmt_node = create_node_impl(builder, Eum_AST_EXPRESSION_STMT, location);
    if (!stmt_node) return NULL;
    
    // 添加表达式
    if (expression) {
        stmt_node->add_child(stmt_node, expression);
    }
    
    return stmt_node;
}

static Stru_AstNodeInterface_t* build_if_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                       const Stru_AstNodeLocation_t* location,
                                                       Stru_AstNodeInterface_t* condition,
                                                       Stru_AstNodeInterface_t* then_branch,
                                                       Stru_AstNodeInterface_t* else_branch)
{
    Stru_AstNodeInterface_t* if_node = create_node_impl(builder, Eum_AST_IF_STMT, location);
    if (!if_node) return NULL;
    
    // 添加条件
    if (condition) {
        if_node->add_child(if_node, condition);
    }
    
    // 添加then分支
    if (then_branch) {
        if_node->add_child(if_node, then_branch);
    }
    
    // 添加else分支（可选）
    if (else_branch) {
        if_node->add_child(if_node, else_branch);
    }
    
    return if_node;
}

static Stru_AstNodeInterface_t* build_while_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* condition,
                                                          Stru_AstNodeInterface_t* body)
{
    Stru_AstNodeInterface_t* while_node = create_node_impl(builder, Eum_AST_WHILE_STMT, location);
    if (!while_node) return NULL;
    
    // 添加条件
    if (condition) {
        while_node->add_child(while_node, condition);
    }
    
    // 添加循环体
    if (body) {
        while_node->add_child(while_node, body);
    }
    
    return while_node;
}

static Stru_AstNodeInterface_t* build_for_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                        const Stru_AstNodeLocation_t* location,
                                                        Stru_AstNodeInterface_t* init,
                                                        Stru_AstNodeInterface_t* condition,
                                                        Stru_AstNodeInterface_t* update,
                                                        Stru_AstNodeInterface_t* body)
{
    Stru_AstNodeInterface_t* for_node = create_node_impl(builder, Eum_AST_FOR_STMT, location);
    if (!for_node) return NULL;
    
    // 添加初始化表达式（可选）
    if (init) {
        for_node->add_child(for_node, init);
    }
    
    // 添加条件表达式（可选）
    if (condition) {
        for_node->add_child(for_node, condition);
    }
    
    // 添加更新表达式（可选）
    if (update) {
        for_node->add_child(for_node, update);
    }
    
    // 添加循环体
    if (body) {
        for_node->add_child(for_node, body);
    }
    
    return for_node;
}

static Stru_AstNodeInterface_t* build_return_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                           const Stru_AstNodeLocation_t* location,
                                                           Stru_AstNodeInterface_t* expression)
{
    Stru_AstNodeInterface_t* return_node = create_node_impl(builder, Eum_AST_RETURN_STMT, location);
    if (!return_node) return NULL;
    
    // 添加返回表达式（可选）
    if (expression) {
        return_node->add_child(return_node, expression);
    }
    
    return return_node;
}

static Stru_AstNodeInterface_t* build_binary_expression_impl(Stru_AstBuilderInterface_t* builder,
                                                            const Stru_AstNodeLocation_t* location,
                                                            int operator_type,
                                                            Stru_AstNodeInterface_t* left,
                                                            Stru_AstNodeInterface_t* right)
{
    Stru_AstNodeInterface_t* bin_expr = create_operator_node_impl(builder, Eum_AST_BINARY_EXPR, location, operator_type);
    if (!bin_expr) return NULL;
    
    // 添加左操作数
    if (left) {
        bin_expr->add_child(bin_expr, left);
    }
    
    // 添加右操作数
    if (right) {
        bin_expr->add_child(bin_expr, right);
    }
    
    return bin_expr;
}

static Stru_AstNodeInterface_t* build_unary_expression_impl(Stru_AstBuilderInterface_t* builder,
                                                           const Stru_AstNodeLocation_t* location,
                                                           int operator_type,
                                                           Stru_AstNodeInterface_t* operand)
{
    Stru_AstNodeInterface_t* unary_expr = create_operator_node_impl(builder, Eum_AST_UNARY_EXPR, location, operator_type);
    if (!unary_expr) return NULL;
    
    // 添加操作数
    if (operand) {
        unary_expr->add_child(unary_expr, operand);
    }
    
    return unary_expr;
}

static Stru_AstNodeInterface_t* build_call_expression_impl(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* function,
                                                          Stru_AstNodeInterface_t** arguments,
                                                          size_t argument_count)
{
    Stru_AstNodeInterface_t* call_expr = create_node_impl(builder, Eum_AST_CALL_EXPR, location);
    if (!call_expr) return NULL;
    
    // 添加函数表达式
    if (function) {
        call_expr->add_child(call_expr, function);
    }
    
    // 添加参数
    for (size_t i = 0; i < argument_count; i++) {
        if (arguments[i]) {
            call_expr->add_child(call_expr, arguments[i]);
        }
    }
    
    return call_expr;
}

static void destroy_impl(Stru_AstBuilderInterface_t* builder)
{
    if (!builder) return;
    
    Stru_AstBuilderInternalData_t* internal_data = 
        F_get_internal_data_from_ast_builder_interface(builder);
    if (internal_data) {
        F_destroy_ast_builder_internal_data(internal_data);
    }
    
    cn_free(builder);
}

// ============================================================================
// 异常处理语句构建函数实现
// ============================================================================

static Stru_AstNodeInterface_t* build_try_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                        const Stru_AstNodeLocation_t* location,
                                                        Stru_AstNodeInterface_t* try_block,
                                                        Stru_AstNodeInterface_t** catch_clauses,
                                                        size_t catch_clause_count,
                                                        Stru_AstNodeInterface_t* finally_block)
{
    Stru_AstNodeInterface_t* try_node = create_node_impl(builder, Eum_AST_TRY_STMT, location);
    if (!try_node) return NULL;
    
    // 添加try块
    if (try_block) {
        try_node->add_child(try_node, try_block);
    }
    
    // 添加catch子句
    for (size_t i = 0; i < catch_clause_count; i++) {
        if (catch_clauses[i]) {
            try_node->add_child(try_node, catch_clauses[i]);
        }
    }
    
    // 添加finally块（可选）
    if (finally_block) {
        try_node->add_child(try_node, finally_block);
    }
    
    return try_node;
}

static Stru_AstNodeInterface_t* build_catch_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* exception_type,
                                                          const char* exception_name,
                                                          Stru_AstNodeInterface_t* catch_block)
{
    Stru_AstNodeInterface_t* catch_node = create_node_impl(builder, Eum_AST_CATCH_STMT, location);
    if (!catch_node) return NULL;
    
    // 设置异常名称（如果有）
    if (exception_name) {
        Uni_AstNodeData_t data;
        memset(&data, 0, sizeof(Uni_AstNodeData_t));
        data.identifier = cn_strdup(exception_name);
        catch_node->set_data(catch_node, &data);
    }
    
    // 添加异常类型（可选）
    if (exception_type) {
        catch_node->add_child(catch_node, exception_type);
    }
    
    // 添加catch块
    if (catch_block) {
        catch_node->add_child(catch_node, catch_block);
    }
    
    return catch_node;
}

static Stru_AstNodeInterface_t* build_throw_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* expression)
{
    Stru_AstNodeInterface_t* throw_node = create_node_impl(builder, Eum_AST_THROW_STMT, location);
    if (!throw_node) return NULL;
    
    // 添加抛出表达式（可选）
    if (expression) {
        throw_node->add_child(throw_node, expression);
    }
    
    return throw_node;
}

static Stru_AstNodeInterface_t* build_finally_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                            const Stru_AstNodeLocation_t* location,
                                                            Stru_AstNodeInterface_t* finally_block)
{
    Stru_AstNodeInterface_t* finally_node = create_node_impl(builder, Eum_AST_FINALLY_STMT, location);
    if (!finally_node) return NULL;
    
    // 添加finally块
    if (finally_block) {
        finally_node->add_child(finally_node, finally_block);
    }
    
    return finally_node;
}

// ============================================================================
// switch-case语句构建函数实现
// ============================================================================

/**
 * @brief 构建switch语句节点
 * 
 * @param builder AST构建器接口
 * @param location 节点位置信息
 * @param expression switch表达式
 * @param cases case语句数组
 * @param case_count case语句数量
 * @param default_case 默认case语句（可选）
 * @return Stru_AstNodeInterface_t* switch语句节点
 */
static Stru_AstNodeInterface_t* build_switch_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                           const Stru_AstNodeLocation_t* location,
                                                           Stru_AstNodeInterface_t* expression,
                                                           Stru_AstNodeInterface_t** cases,
                                                           size_t case_count,
                                                           Stru_AstNodeInterface_t* default_case)
{
    Stru_AstNodeInterface_t* switch_node = create_node_impl(builder, Eum_AST_SWITCH_STMT, location);
    if (!switch_node) return NULL;
    
    // 添加switch表达式
    if (expression) {
        switch_node->add_child(switch_node, expression);
    }
    
    // 添加case语句
    for (size_t i = 0; i < case_count; i++) {
        if (cases[i]) {
            switch_node->add_child(switch_node, cases[i]);
        }
    }
    
    // 添加默认case语句（可选）
    if (default_case) {
        switch_node->add_child(switch_node, default_case);
    }
    
    return switch_node;
}

/**
 * @brief 构建case语句节点
 * 
 * @param builder AST构建器接口
 * @param location 节点位置信息
 * @param case_expression case表达式
 * @param case_body case语句体
 * @return Stru_AstNodeInterface_t* case语句节点
 */
static Stru_AstNodeInterface_t* build_case_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                         const Stru_AstNodeLocation_t* location,
                                                         Stru_AstNodeInterface_t* case_expression,
                                                         Stru_AstNodeInterface_t* case_body)
{
    Stru_AstNodeInterface_t* case_node = create_node_impl(builder, Eum_AST_CASE_STMT, location);
    if (!case_node) return NULL;
    
    // 添加case表达式
    if (case_expression) {
        case_node->add_child(case_node, case_expression);
    }
    
    // 添加case语句体
    if (case_body) {
        case_node->add_child(case_node, case_body);
    }
    
    return case_node;
}

/**
 * @brief 构建default语句节点
 * 
 * @param builder AST构建器接口
 * @param location 节点位置信息
 * @param default_body default语句体
 * @return Stru_AstNodeInterface_t* default语句节点
 */
static Stru_AstNodeInterface_t* build_default_statement_impl(Stru_AstBuilderInterface_t* builder,
                                                            const Stru_AstNodeLocation_t* location,
                                                            Stru_AstNodeInterface_t* default_body)
{
    Stru_AstNodeInterface_t* default_node = create_node_impl(builder, Eum_AST_DEFAULT_STMT, location);
    if (!default_node) return NULL;
    
    // 添加default语句体
    if (default_body) {
        default_node->add_child(default_node, default_body);
    }
    
    return default_node;
}

// ============================================================================
// 字面量构建函数实现
// ============================================================================

/**
 * @brief 构建数组字面量节点
 * 
 * @param builder AST构建器接口
 * @param location 节点位置信息
 * @param elements 数组元素节点数组
 * @param element_count 数组元素数量
 * @return Stru_AstNodeInterface_t* 数组字面量节点
 */
static Stru_AstNodeInterface_t* build_array_literal_impl(Stru_AstBuilderInterface_t* builder,
                                                        const Stru_AstNodeLocation_t* location,
                                                        Stru_AstNodeInterface_t** elements,
                                                        size_t element_count)
{
    Stru_AstNodeInterface_t* array_node = create_node_impl(builder, Eum_AST_ARRAY_LITERAL, location);
    if (!array_node) return NULL;
    
    // 添加数组元素
    for (size_t i = 0; i < element_count; i++) {
        if (elements[i]) {
            array_node->add_child(array_node, elements[i]);
        }
    }
    
    // 设置元素数量作为属性
    Uni_AstNodeData_t data;
    memset(&data, 0, sizeof(Uni_AstNodeData_t));
    data.array_size = element_count;
    array_node->set_data(array_node, &data);
    
    return array_node;
}

/**
 * @brief 构建结构体字面量节点
 * 
 * @param builder AST构建器接口
 * @param location 节点位置信息
 * @param struct_type_name 结构体类型名称（可选）
 * @param members 结构体成员节点数组
 * @param member_count 结构体成员数量
 * @return Stru_AstNodeInterface_t* 结构体字面量节点
 */
static Stru_AstNodeInterface_t* build_struct_literal_impl(Stru_AstBuilderInterface_t* builder,
                                                         const Stru_AstNodeLocation_t* location,
                                                         const char* struct_type_name,
                                                         Stru_AstNodeInterface_t** members,
                                                         size_t member_count)
{
    Stru_AstNodeInterface_t* struct_node = create_node_impl(builder, Eum_AST_STRUCT_LITERAL, location);
    if (!struct_node) return NULL;
    
    // 设置结构体类型名称（如果有）
    if (struct_type_name) {
        Uni_AstNodeData_t data;
        memset(&data, 0, sizeof(Uni_AstNodeData_t));
        data.type_name = cn_strdup(struct_type_name);
        struct_node->set_data(struct_node, &data);
    }
    
    // 添加结构体成员
    for (size_t i = 0; i < member_count; i++) {
        if (members[i]) {
            struct_node->add_child(struct_node, members[i]);
        }
    }
    
    return struct_node;
}
