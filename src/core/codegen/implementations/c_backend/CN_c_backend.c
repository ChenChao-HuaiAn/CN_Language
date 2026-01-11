/******************************************************************************
 * 文件名: CN_c_backend.c
 * 功能: CN_Language C语言后端实现
 * 
 * 提供将CN_Language抽象语法树转换为C语言代码的后端实现。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月9日
 * 修改历史:
 *   - 2026年1月9日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_c_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* 内部状态结构体 */
typedef struct {
    int indent_level;          ///< 当前缩进级别
    char* output_buffer;       ///< 输出缓冲区
    size_t buffer_size;        ///< 缓冲区大小
    size_t buffer_pos;         ///< 缓冲区当前位置
    Stru_DynamicArray_t* errors; ///< 错误信息
    Stru_DynamicArray_t* warnings; ///< 警告信息
    bool has_errors;           ///< 是否有错误
    bool has_warnings;         ///< 是否有警告
} CBackendState;

/* 内部函数声明 */
static CBackendState* create_c_backend_state(void);
static void destroy_c_backend_state(CBackendState* state);
static bool ensure_buffer_capacity(CBackendState* state, size_t needed);
static void append_string(CBackendState* state, const char* str);
static void append_indent(CBackendState* state);
static void append_newline(CBackendState* state);
static void add_error(CBackendState* state, const char* message);
static void add_warning(CBackendState* state, const char* message);
static void generate_program(CBackendState* state, Stru_AstNode_t* ast);
static void generate_statement(CBackendState* state, Stru_AstNode_t* stmt);
static void generate_expression(CBackendState* state, Stru_AstNode_t* expr);
static void generate_declaration(CBackendState* state, Stru_AstNode_t* decl);
static void generate_function_declaration(CBackendState* state, Stru_AstNode_t* func_decl);
static void generate_variable_declaration(CBackendState* state, Stru_AstNode_t* var_decl);
static void generate_if_statement(CBackendState* state, Stru_AstNode_t* if_stmt);
static void generate_while_statement(CBackendState* state, Stru_AstNode_t* while_stmt);
static void generate_return_statement(CBackendState* state, Stru_AstNode_t* return_stmt);
static void generate_binary_expression(CBackendState* state, Stru_AstNode_t* bin_expr);
static void generate_unary_expression(CBackendState* state, Stru_AstNode_t* unary_expr);
static void generate_literal_expression(CBackendState* state, Stru_AstNode_t* literal);
static void generate_identifier_expression(CBackendState* state, Stru_AstNode_t* identifier);
static void generate_call_expression(CBackendState* state, Stru_AstNode_t* call_expr);
static const char* get_c_type_from_cn_type(const char* cn_type);
static const char* get_c_operator(int cn_operator);

/**
 * @brief 创建C后端状态
 */
static CBackendState* create_c_backend_state(void)
{
    CBackendState* state = (CBackendState*)malloc(sizeof(CBackendState));
    if (!state) {
        return NULL;
    }
    
    memset(state, 0, sizeof(CBackendState));
    state->indent_level = 0;
    state->buffer_size = 4096; // 初始缓冲区大小
    state->buffer_pos = 0;
    state->output_buffer = (char*)malloc(state->buffer_size);
    state->has_errors = false;
    state->has_warnings = false;
    
    // 创建错误和警告动态数组
    state->errors = F_create_dynamic_array(sizeof(char*));
    state->warnings = F_create_dynamic_array(sizeof(char*));
    
    if (!state->output_buffer || !state->errors || !state->warnings) {
        if (state->output_buffer) free(state->output_buffer);
        if (state->errors) F_destroy_dynamic_array(state->errors);
        if (state->warnings) F_destroy_dynamic_array(state->warnings);
        free(state);
        return NULL;
    }
    
    state->output_buffer[0] = '\0';
    return state;
}

/**
 * @brief 销毁C后端状态
 */
static void destroy_c_backend_state(CBackendState* state)
{
    if (!state) {
        return;
    }
    
    if (state->output_buffer) {
        free(state->output_buffer);
    }
    
    // 销毁动态数组及其内容
    if (state->errors) {
        // 释放所有错误消息字符串
        for (size_t i = 0; i < F_dynamic_array_length(state->errors); i++) {
            char** error_ptr = (char**)F_dynamic_array_get(state->errors, i);
            if (error_ptr && *error_ptr) {
                free(*error_ptr);
            }
        }
        F_destroy_dynamic_array(state->errors);
    }
    
    if (state->warnings) {
        // 释放所有警告消息字符串
        for (size_t i = 0; i < F_dynamic_array_length(state->warnings); i++) {
            char** warning_ptr = (char**)F_dynamic_array_get(state->warnings, i);
            if (warning_ptr && *warning_ptr) {
                free(*warning_ptr);
            }
        }
        F_destroy_dynamic_array(state->warnings);
    }
    
    free(state);
}

/**
 * @brief 确保缓冲区容量
 */
static bool ensure_buffer_capacity(CBackendState* state, size_t needed)
{
    if (!state || !state->output_buffer) {
        return false;
    }
    
    if (state->buffer_pos + needed + 1 <= state->buffer_size) {
        return true;
    }
    
    // 计算新大小
    size_t new_size = state->buffer_size * 2;
    while (new_size < state->buffer_pos + needed + 1) {
        new_size *= 2;
    }
    
    // 重新分配缓冲区
    char* new_buffer = (char*)realloc(state->output_buffer, new_size);
    if (!new_buffer) {
        return false;
    }
    
    state->output_buffer = new_buffer;
    state->buffer_size = new_size;
    return true;
}

/**
 * @brief 追加字符串到缓冲区
 */
static void append_string(CBackendState* state, const char* str)
{
    if (!state || !str) {
        return;
    }
    
    size_t len = strlen(str);
    if (!ensure_buffer_capacity(state, len)) {
        return;
    }
    
    memcpy(state->output_buffer + state->buffer_pos, str, len);
    state->buffer_pos += len;
    state->output_buffer[state->buffer_pos] = '\0';
}

/**
 * @brief 追加新行
 */
static void append_newline(CBackendState* state)
{
    append_string(state, "\n");
}

/**
 * @brief 添加错误信息
 */
static void add_error(CBackendState* state, const char* message)
{
    if (!state || !state->errors || !message) {
        return;
    }
    
    char* error_copy = strdup(message);
    if (error_copy) {
        F_dynamic_array_push(state->errors, &error_copy);
        state->has_errors = true;
    }
}

/**
 * @brief 添加警告信息
 */
static void add_warning(CBackendState* state, const char* message)
{
    if (!state || !state->warnings || !message) {
        return;
    }
    
    char* warning_copy = strdup(message);
    if (warning_copy) {
        F_dynamic_array_push(state->warnings, &warning_copy);
        state->has_warnings = true;
    }
}

/**
 * @brief 追加缩进
 */
static void append_indent(CBackendState* state)
{
    if (!state) {
        return;
    }
    
    for (int i = 0; i < state->indent_level; i++) {
        append_string(state, "    "); // 4个空格缩进
    }
}

/**
 * @brief 获取C类型对应的C类型
 */
static const char* get_c_type_from_cn_type(const char* cn_type)
{
    if (!cn_type) return "void";
    
    // 简单的类型映射
    if (strcmp(cn_type, "整数") == 0 || strcmp(cn_type, "int") == 0) return "int";
    if (strcmp(cn_type, "浮点数") == 0 || strcmp(cn_type, "float") == 0) return "float";
    if (strcmp(cn_type, "双精度") == 0 || strcmp(cn_type, "double") == 0) return "double";
    if (strcmp(cn_type, "布尔") == 0 || strcmp(cn_type, "bool") == 0) return "bool";
    if (strcmp(cn_type, "字符") == 0 || strcmp(cn_type, "char") == 0) return "char";
    if (strcmp(cn_type, "字符串") == 0 || strcmp(cn_type, "string") == 0) return "char*";
    if (strcmp(cn_type, "空") == 0 || strcmp(cn_type, "void") == 0) return "void";
    
    // 默认返回原类型名
    return cn_type;
}

/**
 * @brief 获取C运算符
 */
static const char* get_c_operator(int cn_operator)
{
    // 简单的运算符映射
    switch (cn_operator) {
        case 1: return "+";   // 加法
        case 2: return "-";   // 减法
        case 3: return "*";   // 乘法
        case 4: return "/";   // 除法
        case 5: return "%";   // 取模
        case 6: return "==";  // 等于
        case 7: return "!=";  // 不等于
        case 8: return "<";   // 小于
        case 9: return "<=";  // 小于等于
        case 10: return ">";  // 大于
        case 11: return ">="; // 大于等于
        case 12: return "&&"; // 逻辑与
        case 13: return "||"; // 逻辑或
        case 14: return "!";  // 逻辑非
        case 15: return "=";  // 赋值
        case 16: return "+="; // 加法赋值
        case 17: return "-="; // 减法赋值
        case 18: return "*="; // 乘法赋值
        case 19: return "/="; // 除法赋值
        default: return "?";  // 未知运算符
    }
}

/**
 * @brief 生成程序
 */
static void generate_program(CBackendState* state, Stru_AstNode_t* ast)
{
    if (!state || !ast) {
        add_error(state, "无效的AST节点");
        return;
    }
    
    // 调试：检查AST节点接口
    Stru_AstNodeInterface_t* ast_interface = F_ast_get_internal_interface(ast);
    if (!ast_interface) {
        add_error(state, "AST节点接口为NULL");
        return;
    }
    
    // 添加文件头注释
    append_string(state, "/******************************************************************************\n");
    append_string(state, " * 文件名: generated.c\n");
    append_string(state, " * 功能: 由CN_Language编译器生成的C代码\n");
    append_string(state, " * 作者: CN_Language编译器\n");
    append_string(state, " * 生成日期: 2026-01-10\n");
    append_string(state, " * 警告: 这是自动生成的代码，请勿手动修改\n");
    append_string(state, " ******************************************************************************/\n\n");
    
    // 添加包含头文件
    append_string(state, "#include <stdio.h>\n");
    append_string(state, "#include <stdlib.h>\n");
    append_string(state, "#include <stdbool.h>\n");
    append_string(state, "#include <stdint.h>\n");
    append_string(state, "#include <string.h>\n\n");
    
    // 生成全局声明
    append_string(state, "/* 全局声明 */\n");
    
    // 遍历AST子节点，生成声明
    size_t child_count = F_ast_get_child_count(ast);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNode_t* child = F_ast_get_child(ast, i);
        if (child) {
            generate_declaration(state, child);
        }
    }
    
    append_newline(state);
    
    // 生成main函数
    append_string(state, "int main(void)\n");
    append_string(state, "{\n");
    
    state->indent_level++;
    
    // 生成main函数体
    append_indent(state);
    append_string(state, "printf(\"CN_Language程序开始执行...\\n\");\n");
    
    // 生成语句
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNode_t* child = F_ast_get_child(ast, i);
        if (child) {
            // 检查节点类型，如果是函数声明则调用
            // 这里简化处理，实际应该根据AST结构生成
            generate_statement(state, child);
        }
    }
    
    append_indent(state);
    append_string(state, "printf(\"CN_Language程序执行完成。\\n\");\n");
    append_indent(state);
    append_string(state, "return 0;\n");
    
    state->indent_level--;
    append_string(state, "}\n");
}

/**
 * @brief 生成语句
 */
static void generate_statement(CBackendState* state, Stru_AstNode_t* stmt)
{
    if (!state || !stmt) {
        add_error(state, "无效的语句节点");
        return;
    }
    
    // 获取节点类型
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(stmt);
    if (!interface) {
        add_error(state, "无法获取AST节点接口");
        return;
    }
    
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    // 根据节点类型调用相应的生成函数
    switch (node_type) {
        case Eum_AST_EXPRESSION_STMT:
            // 表达式语句
            if (interface->get_child_count(interface) > 0) {
                Stru_AstNodeInterface_t* expr = interface->get_child(interface, 0);
                if (expr) {
                    append_indent(state);
                    generate_expression(state, (Stru_AstNode_t*)expr);
                    append_string(state, ";\n");
                }
            }
            break;
            
        case Eum_AST_IF_STMT:
            generate_if_statement(state, stmt);
            break;
            
        case Eum_AST_WHILE_STMT:
            generate_while_statement(state, stmt);
            break;
            
        case Eum_AST_RETURN_STMT:
            generate_return_statement(state, stmt);
            break;
            
        case Eum_AST_VARIABLE_DECL:
            generate_variable_declaration(state, stmt);
            break;
            
        case Eum_AST_FUNCTION_DECL:
            generate_function_declaration(state, stmt);
            break;
            
        case Eum_AST_BLOCK_STMT:
            // 代码块语句
            append_indent(state);
            append_string(state, "{\n");
            state->indent_level++;
            
            for (size_t i = 0; i < interface->get_child_count(interface); i++) {
                Stru_AstNodeInterface_t* child = interface->get_child(interface, i);
                if (child) {
                    generate_statement(state, (Stru_AstNode_t*)child);
                }
            }
            
            state->indent_level--;
            append_indent(state);
            append_string(state, "}\n");
            break;
            
        default:
            add_warning(state, "未支持的语句类型");
            append_indent(state);
            append_string(state, "// 未支持的语句类型\n");
            break;
    }
}

/**
 * @brief 生成表达式
 */
static void generate_expression(CBackendState* state, Stru_AstNode_t* expr)
{
    if (!state || !expr) {
        add_error(state, "无效的表达式节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(expr);
    if (!interface) {
        add_error(state, "无法获取AST节点接口");
        return;
    }
    
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    switch (node_type) {
        case Eum_AST_BINARY_EXPR:
            generate_binary_expression(state, expr);
            break;
            
        case Eum_AST_UNARY_EXPR:
            generate_unary_expression(state, expr);
            break;
            
        case Eum_AST_LITERAL_EXPR:
        case Eum_AST_INT_LITERAL:
        case Eum_AST_FLOAT_LITERAL:
        case Eum_AST_STRING_LITERAL:
        case Eum_AST_BOOL_LITERAL:
            generate_literal_expression(state, expr);
            break;
            
        case Eum_AST_IDENTIFIER_EXPR:
            generate_identifier_expression(state, expr);
            break;
            
        case Eum_AST_CALL_EXPR:
            generate_call_expression(state, expr);
            break;
            
        default:
            add_warning(state, "未支持的表达式类型");
            append_string(state, "/* 未支持的表达式类型 */");
            break;
    }
}

/**
 * @brief 生成声明
 */
static void generate_declaration(CBackendState* state, Stru_AstNode_t* decl)
{
    if (!state || !decl) {
        add_error(state, "无效的声明节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(decl);
    if (!interface) {
        add_error(state, "无法获取AST节点接口");
        return;
    }
    
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    switch (node_type) {
        case Eum_AST_VARIABLE_DECL:
            generate_variable_declaration(state, decl);
            break;
            
        case Eum_AST_FUNCTION_DECL:
            generate_function_declaration(state, decl);
            break;
            
        default:
            add_warning(state, "未支持的声明类型");
            append_indent(state);
            append_string(state, "// 未支持的声明类型\n");
            break;
    }
}

/**
 * @brief 生成函数声明
 */
static void generate_function_declaration(CBackendState* state, Stru_AstNode_t* func_decl)
{
    if (!state || !func_decl) {
        add_error(state, "无效的函数声明节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(func_decl);
    if (!interface) {
        add_error(state, "无法获取AST节点接口");
        return;
    }
    
    // 获取函数名
    const char* func_name = F_ast_get_identifier(func_decl);
    if (!func_name) {
        func_name = "unknown_function";
    }
    
    // 获取返回类型
    const char* return_type_name = F_ast_get_type_name(func_decl);
    const char* c_return_type = get_c_type_from_cn_type(return_type_name);
    
    // 生成函数声明
    append_string(state, c_return_type);
    append_string(state, " ");
    append_string(state, func_name);
    append_string(state, "(");
    
    // 生成参数列表
    size_t param_count = interface->get_child_count(interface);
    bool has_params = false;
    
    for (size_t i = 0; i < param_count; i++) {
        Stru_AstNodeInterface_t* param = interface->get_child(interface, i);
        if (param && param->get_type(param) == Eum_AST_PARAMETER_DECL) {
            if (has_params) {
                append_string(state, ", ");
            }
            
            // 这里简化处理，实际应该获取参数类型和名称
            append_string(state, "int param");
            char index_str[16];
            snprintf(index_str, sizeof(index_str), "%zu", i);
            append_string(state, index_str);
            
            has_params = true;
        }
    }
    
    if (!has_params) {
        append_string(state, "void");
    }
    
    append_string(state, ");\n");
}

/**
 * @brief 生成变量声明
 */
static void generate_variable_declaration(CBackendState* state, Stru_AstNode_t* var_decl)
{
    if (!state || !var_decl) {
        add_error(state, "无效的变量声明节点");
        return;
    }
    
    // 获取变量名
    const char* var_name = F_ast_get_identifier(var_decl);
    if (!var_name) {
        var_name = "unknown_var";
    }
    
    // 获取类型
    const char* type_name = F_ast_get_type_name(var_decl);
    const char* c_type = get_c_type_from_cn_type(type_name);
    
    // 生成变量声明
    append_indent(state);
    append_string(state, c_type);
    append_string(state, " ");
    append_string(state, var_name);
    
    // 检查是否有初始化表达式
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(var_decl);
    if (interface && interface->get_child_count(interface) > 0) {
        // 这里简化处理，实际应该生成初始化表达式
        append_string(state, " = 0");
    }
    
    append_string(state, ";\n");
}

/**
 * @brief 生成if语句
 */
static void generate_if_statement(CBackendState* state, Stru_AstNode_t* if_stmt)
{
    if (!state || !if_stmt) {
        add_error(state, "无效的if语句节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(if_stmt);
    if (!interface || interface->get_child_count(interface) < 1) {
        add_error(state, "if语句缺少条件表达式");
        return;
    }
    
    append_indent(state);
    append_string(state, "if (");
    
    // 生成条件表达式
    Stru_AstNodeInterface_t* condition = interface->get_child(interface, 0);
    if (condition) {
        generate_expression(state, (Stru_AstNode_t*)condition);
    } else {
        append_string(state, "1");
    }
    
    append_string(state, ") {\n");
    
    state->indent_level++;
    
    // 生成then分支
    if (interface->get_child_count(interface) > 1) {
        Stru_AstNodeInterface_t* then_branch = interface->get_child(interface, 1);
        if (then_branch) {
            generate_statement(state, (Stru_AstNode_t*)then_branch);
        }
    }
    
    state->indent_level--;
    append_indent(state);
    append_string(state, "}\n");
    
    // 生成else分支（如果有）
    if (interface->get_child_count(interface) > 2) {
        append_indent(state);
        append_string(state, "else {\n");
        
        state->indent_level++;
        Stru_AstNodeInterface_t* else_branch = interface->get_child(interface, 2);
        if (else_branch) {
            generate_statement(state, (Stru_AstNode_t*)else_branch);
        }
        
        state->indent_level--;
        append_indent(state);
        append_string(state, "}\n");
    }
}

/**
 * @brief 生成while语句
 */
static void generate_while_statement(CBackendState* state, Stru_AstNode_t* while_stmt)
{
    if (!state || !while_stmt) {
        add_error(state, "无效的while语句节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(while_stmt);
    if (!interface || interface->get_child_count(interface) < 1) {
        add_error(state, "while语句缺少条件表达式");
        return;
    }
    
    append_indent(state);
    append_string(state, "while (");
    
    // 生成条件表达式
    Stru_AstNodeInterface_t* condition = interface->get_child(interface, 0);
    if (condition) {
        generate_expression(state, (Stru_AstNode_t*)condition);
    } else {
        append_string(state, "1");
    }
    
    append_string(state, ") {\n");
    
    state->indent_level++;
    
    // 生成循环体
    if (interface->get_child_count(interface) > 1) {
        Stru_AstNodeInterface_t* body = interface->get_child(interface, 1);
        if (body) {
            generate_statement(state, (Stru_AstNode_t*)body);
        }
    }
    
    state->indent_level--;
    append_indent(state);
    append_string(state, "}\n");
}

/**
 * @brief 生成return语句
 */
static void generate_return_statement(CBackendState* state, Stru_AstNode_t* return_stmt)
{
    if (!state || !return_stmt) {
        add_error(state, "无效的return语句节点");
        return;
    }
    
    append_indent(state);
    append_string(state, "return");
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(return_stmt);
    if (interface && interface->get_child_count(interface) > 0) {
        // 生成返回值表达式
        Stru_AstNodeInterface_t* expr = interface->get_child(interface, 0);
        if (expr) {
            append_string(state, " ");
            generate_expression(state, (Stru_AstNode_t*)expr);
        }
    }
    
    append_string(state, ";\n");
}

/**
 * @brief 生成二元表达式
 */
static void generate_binary_expression(CBackendState* state, Stru_AstNode_t* bin_expr)
{
    if (!state || !bin_expr) {
        add_error(state, "无效的二元表达式节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(bin_expr);
    if (!interface || interface->get_child_count(interface) < 2) {
        add_error(state, "二元表达式缺少操作数");
        return;
    }
    
    // 获取运算符
    int operator_type = F_ast_get_operator_type(bin_expr);
    const char* c_operator = get_c_operator(operator_type);
    
    // 生成左操作数
    Stru_AstNodeInterface_t* left = interface->get_child(interface, 0);
    if (left) {
        generate_expression(state, (Stru_AstNode_t*)left);
    }
    
    append_string(state, " ");
    append_string(state, c_operator);
    append_string(state, " ");
    
    // 生成右操作数
    Stru_AstNodeInterface_t* right = interface->get_child(interface, 1);
    if (right) {
        generate_expression(state, (Stru_AstNode_t*)right);
    }
}

/**
 * @brief 生成一元表达式
 */
static void generate_unary_expression(CBackendState* state, Stru_AstNode_t* unary_expr)
{
    if (!state || !unary_expr) {
        add_error(state, "无效的一元表达式节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(unary_expr);
    if (!interface || interface->get_child_count(interface) < 1) {
        add_error(state, "一元表达式缺少操作数");
        return;
    }
    
    // 获取运算符
    int operator_type = F_ast_get_operator_type(unary_expr);
    const char* c_operator = get_c_operator(operator_type);
    
    // 生成运算符
    append_string(state, c_operator);
    
    // 生成操作数
    Stru_AstNodeInterface_t* operand = interface->get_child(interface, 0);
    if (operand) {
        generate_expression(state, (Stru_AstNode_t*)operand);
    }
}

/**
 * @brief 生成字面量表达式
 */
static void generate_literal_expression(CBackendState* state, Stru_AstNode_t* literal)
{
    if (!state || !literal) {
        add_error(state, "无效的字面量节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(literal);
    if (!interface) {
        add_error(state, "无法获取AST节点接口");
        return;
    }
    
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    switch (node_type) {
        case Eum_AST_INT_LITERAL:
            {
                long int_value = F_ast_get_int_value(literal);
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%ld", int_value);
                append_string(state, buffer);
            }
            break;
            
        case Eum_AST_FLOAT_LITERAL:
            {
                double float_value = F_ast_get_float_value(literal);
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%f", float_value);
                append_string(state, buffer);
            }
            break;
            
        case Eum_AST_STRING_LITERAL:
            {
                const char* string_value = F_ast_get_string_value(literal);
                if (string_value) {
                    append_string(state, "\"");
                    append_string(state, string_value);
                    append_string(state, "\"");
                } else {
                    append_string(state, "\"\"");
                }
            }
            break;
            
        case Eum_AST_BOOL_LITERAL:
            {
                bool bool_value = F_ast_get_bool_value(literal);
                append_string(state, bool_value ? "true" : "false");
            }
            break;
            
        default:
            add_warning(state, "未支持的字面量类型");
            append_string(state, "0");
            break;
    }
}

/**
 * @brief 生成标识符表达式
 */
static void generate_identifier_expression(CBackendState* state, Stru_AstNode_t* identifier)
{
    if (!state || !identifier) {
        add_error(state, "无效的标识符节点");
        return;
    }
    
    const char* id_name = F_ast_get_identifier(identifier);
    if (id_name) {
        append_string(state, id_name);
    } else {
        append_string(state, "unknown_identifier");
    }
}

/**
 * @brief 生成函数调用表达式
 */
static void generate_call_expression(CBackendState* state, Stru_AstNode_t* call_expr)
{
    if (!state || !call_expr) {
        add_error(state, "无效的函数调用节点");
        return;
    }
    
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(call_expr);
    if (!interface || interface->get_child_count(interface) < 1) {
        add_error(state, "函数调用缺少函数名");
        return;
    }
    
    // 生成函数名
    Stru_AstNodeInterface_t* func_node = interface->get_child(interface, 0);
    if (func_node) {
        generate_expression(state, (Stru_AstNode_t*)func_node);
    } else {
        append_string(state, "unknown_function");
    }
    
    append_string(state, "(");
    
    // 生成参数列表
    for (size_t i = 1; i < interface->get_child_count(interface); i++) {
        if (i > 1) {
            append_string(state, ", ");
        }
        
        Stru_AstNodeInterface_t* arg = interface->get_child(interface, i);
        if (arg) {
            generate_expression(state, (Stru_AstNode_t*)arg);
        }
    }
    
    append_string(state, ")");
}

/**
 * @brief 从AST生成C代码
 * 
 * 将CN_Language的抽象语法树转换为C语言代码。
 * 
 * @param ast 抽象语法树根节点
 * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
 */
Stru_CodeGenResult_t* F_generate_c_code(Stru_AstNode_t* ast)
{
    // 创建结果
    Stru_CodeGenResult_t* result = (Stru_CodeGenResult_t*)malloc(sizeof(Stru_CodeGenResult_t));
    if (!result) {
        return NULL;
    }
    
    memset(result, 0, sizeof(Stru_CodeGenResult_t));
    
    // 创建后端状态
    CBackendState* state = create_c_backend_state();
    if (!state) {
        result->success = false;
        // 创建错误数组
        result->errors = F_create_dynamic_array(sizeof(char*));
        if (result->errors) {
            char* error_msg = strdup("无法创建C后端状态");
            if (error_msg) {
                F_dynamic_array_push(result->errors, &error_msg);
            }
        }
        return result;
    }
    
    // 生成代码
    generate_program(state, ast);
    
    // 设置结果
    result->success = !state->has_errors;
    result->code = state->output_buffer;
    result->code_length = state->buffer_pos;
    
    // 转移错误和警告信息
    result->errors = state->errors;
    result->warnings = state->warnings;
    
    // 转移缓冲区所有权
    state->output_buffer = NULL;
    state->errors = NULL;
    state->warnings = NULL;
    
    // 清理状态（不释放已转移的资源）
    free(state);
    
    return result;
}

/**
 * @brief 检查C后端是否支持特定功能
 * 
 * 检查C后端是否支持特定的代码生成功能。
 * 
 * @param feature 功能标识符
 * @return 支持返回true，不支持返回false
 */
bool F_c_backend_supports_feature(const char* feature)
{
    if (!feature) return false;
    
    // 支持的功能列表
    if (strcmp(feature, "c99") == 0) return true;
    if (strcmp(feature, "c11") == 0) return true;
    if (strcmp(feature, "c17") == 0) return true;
    if (strcmp(feature, "posix") == 0) return true;
    if (strcmp(feature, "threads") == 0) return false; // 暂不支持
    if (strcmp(feature, "exceptions") == 0) return false; // 暂不支持
    
    return false;
}

/**
 * @brief 配置C后端选项
 * 
 * 配置C后端的代码生成选项。
 * 
 * @param option 选项名称
 * @param value 选项值
 * @return 配置成功返回true，失败返回false
 */
bool F_configure_c_backend(const char* option, const char* value)
{
    // 目前只记录配置，实际实现中应该影响代码生成
    if (!option || !value) return false;
    
    // 这里可以添加配置逻辑
    // 例如：设置C标准、优化级别等
    
    return true;
}

/**
 * @brief 创建C后端代码生成器接口
 * 
 * 创建并返回一个C后端特定的代码生成器接口实例。
 * 
 * @return Stru_CodeGeneratorInterface_t* C后端代码生成器接口实例
 */
Stru_CodeGeneratorInterface_t* F_create_c_backend_interface(void)
{
    // 这里应该创建一个完整的代码生成器接口实例
    // 由于时间关系，这里返回NULL，表示功能待实现
    return NULL;
}

/**
 * @brief 获取C后端版本信息
 * 
 * 返回C后端的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_c_backend_version(int* major, int* minor, int* patch)
{
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (patch) *patch = 0;
}

/**
 * @brief 获取C后端版本字符串
 * 
 * 返回C后端的版本字符串。
 * 
 * @return 版本字符串
 */
const char* F_get_c_backend_version_string(void)
{
    return "1.0.0";
}
