#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/frontend/ast/class_node.h"  // 类和接口AST节点

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct CnParser {
    CnLexer *lexer;
    CnToken current;
    int has_current;
    int error_count;
    CnDiagnostics *diagnostics;
    CnVisibility current_visibility;  // 当前可见性（用于文件级块声明）
    int in_function_body;             // 是否在函数体内（用于静态变量作用域检查）
} CnParser;

static void parser_advance(CnParser *parser);
static int parser_match(CnParser *parser, CnTokenKind kind);
static int parser_expect(CnParser *parser, CnTokenKind kind);
static void check_reserved_keyword(CnParser *parser);
static int is_reserved_keyword(CnTokenKind kind);

static CnAstProgram *parse_program_internal(CnParser *parser);
static CnAstFunctionDecl *parse_function_decl(CnParser *parser);
static CnAstFunctionDecl *parse_interrupt_handler(CnParser *parser);
static CnAstStmt *parse_struct_decl(CnParser *parser);
static CnAstStmt *parse_enum_decl(CnParser *parser);
static CnAstStmt *parse_import_stmt(CnParser *parser);
static CnAstStmt *parse_from_import_stmt(CnParser *parser);  // 从...导入 语法
static CnAstModulePath *parse_module_path(CnParser *parser);  // 解析点分模块路径
static CnAstBlockStmt *parse_block(CnParser *parser);
static CnAstStmt *parse_statement(CnParser *parser);

/* 类和接口解析函数（阶段11 - 面向对象编程支持） */
static CnAstStmt *parse_class_decl(CnParser *parser);
static CnAstStmt *parse_interface_decl(CnParser *parser);
static void parse_base_class_list(CnParser *parser, CnAstClassDecl *class_decl);
static CnClassMember *parse_class_member(CnParser *parser, CnAccessLevel default_access,
                                          const char *class_name, size_t class_name_length);
static CnAccessLevel parse_access_label(CnParser *parser);
static CnType *parse_type(CnParser *parser);
static CnAstExpr *parse_expression(CnParser *parser);
static CnAstExpr *parse_assignment(CnParser *parser);
static CnAstExpr *parse_ternary(CnParser *parser);
static CnAstExpr *parse_logical_or(CnParser *parser);
static CnAstExpr *parse_logical_and(CnParser *parser);
static CnAstExpr *parse_bitwise_or(CnParser *parser);
static CnAstExpr *parse_bitwise_xor(CnParser *parser);
static CnAstExpr *parse_bitwise_and(CnParser *parser);
static CnAstExpr *parse_comparison(CnParser *parser);
static CnAstExpr *parse_shift(CnParser *parser);
static CnAstExpr *parse_additive(CnParser *parser);
static CnAstExpr *parse_term(CnParser *parser);
static CnAstExpr *parse_unary(CnParser *parser);
static CnAstExpr *parse_postfix(CnParser *parser);
static CnAstExpr *parse_factor(CnParser *parser);
static CnAstExpr *parse_struct_literal_with_name(CnParser *parser, const char *struct_name, size_t struct_name_length);

static CnAstExpr *make_integer_literal(long value);
static CnAstExpr *make_float_literal(double value);
static char* process_string_escapes(const char *raw_string, size_t raw_length, size_t *out_length);
static CnAstExpr *make_string_literal(const char *value, size_t length);
static CnAstExpr *make_bool_literal(int value);
static CnAstExpr *make_identifier(const char *name, size_t length);
static CnAstExpr *make_binary(CnAstBinaryOp op, CnAstExpr *left, CnAstExpr *right);
static CnAstExpr *make_logical(CnAstLogicalOp op, CnAstExpr *left, CnAstExpr *right);
static CnAstExpr *make_unary(CnAstUnaryOp op, CnAstExpr *operand);
static CnAstExpr *make_assign(CnAstExpr *target, CnAstExpr *value);
static CnAstExpr *make_call(CnAstExpr *callee, CnAstExpr **arguments, size_t argument_count);
static CnAstExpr *make_array_literal(CnAstExpr **elements, size_t element_count);
static CnAstExpr *make_index(CnAstExpr *array, CnAstExpr *index);
static CnAstExpr *make_member_access(CnAstExpr *object, const char *member_name, size_t member_name_length, int is_arrow);
static CnAstExpr *make_struct_literal(const char *struct_name, size_t struct_name_length, CnAstStructFieldInit *fields, size_t field_count);
static CnAstExpr *make_memory_read(CnAstExpr *address);
static CnAstExpr *make_memory_write(CnAstExpr *address, CnAstExpr *value);
static CnAstExpr *make_memory_copy(CnAstExpr *dest, CnAstExpr *src, CnAstExpr *size);
static CnAstExpr *make_memory_set(CnAstExpr *address, CnAstExpr *value, CnAstExpr *size);
static CnAstExpr *make_memory_map(CnAstExpr *address, CnAstExpr *size, CnAstExpr *prot, CnAstExpr *flags);
static CnAstExpr *make_memory_unmap(CnAstExpr *address, CnAstExpr *size);
static CnAstExpr *make_inline_asm(CnAstExpr *asm_code, CnAstExpr **outputs, size_t output_count, 
                                   CnAstExpr **inputs, size_t input_count, CnAstExpr *clobbers);
static CnAstStmt *make_expr_stmt(CnAstExpr *expr);
static CnAstStmt *make_return_stmt(CnAstExpr *expr);
static CnAstStmt *make_if_stmt(CnAstExpr *condition, CnAstBlockStmt *then_block, CnAstBlockStmt *else_block);
static CnAstStmt *make_while_stmt(CnAstExpr *condition, CnAstBlockStmt *body);
static CnAstStmt *make_for_stmt(CnAstStmt *init, CnAstExpr *condition, CnAstExpr *update, CnAstBlockStmt *body);
static CnAstStmt *make_switch_stmt(CnAstExpr *expr, CnAstSwitchCase *cases, size_t case_count);
static CnAstStmt *make_break_stmt(void);
static CnAstStmt *make_continue_stmt(void);
static CnAstStmt *make_var_decl_stmt(const char *name, size_t name_length, CnType *declared_type, CnAstExpr *initializer, CnVisibility visibility);
static CnAstStmt *make_struct_decl_stmt(const char *name, size_t name_length, CnAstStructField *fields, size_t field_count);
static CnAstStmt *make_enum_decl_stmt(const char *name, size_t name_length, CnAstEnumMember *members, size_t member_count);
static CnAstBlockStmt *make_block(void);
static void block_add_stmt(CnAstBlockStmt *block, CnAstStmt *stmt);
static CnAstProgram *make_program(void);
static void program_add_function(CnAstProgram *program, CnAstFunctionDecl *function_decl);
static void program_add_struct(CnAstProgram *program, CnAstStmt *struct_decl);
static void program_add_enum(CnAstProgram *program, CnAstStmt *enum_decl);
static void program_add_import(CnAstProgram *program, CnAstStmt *import_stmt);
static void program_add_global_var(CnAstProgram *program, CnAstStmt *var_decl);
/* 类和接口添加函数（阶段11 - 面向对象编程支持） */
static void program_add_class(CnAstProgram *program, CnAstStmt *class_decl);
static void program_add_interface(CnAstProgram *program, CnAstStmt *interface_decl);
/* 模板添加函数（阶段13 - 泛型编程支持） */
static void program_add_template_func(CnAstProgram *program, CnAstStmt *template_func_decl);
static void program_add_template_struct(CnAstProgram *program, CnAstStmt *template_struct_decl);
/* 模板解析函数（阶段13 - 泛型编程支持） */
static CnAstTemplateParams *parse_template_params(CnParser *parser);
static CnAstStmt *parse_template_declaration(CnParser *parser);
static CnAstStmt *parse_template_function_decl_with_params(CnParser *parser, CnAstTemplateParams *params);
static CnAstStmt *parse_template_struct_decl_with_params(CnParser *parser, CnAstTemplateParams *params);
static CnAstExpr *parse_template_instantiation(CnParser *parser, const char *name, size_t name_len);

/**
 * @brief 判断当前 token 是否可能是类型名的开始
 *
 * 用于区分模板实例化和比较表达式。
 * 当遇到 "标识符 <" 时，需要判断 < 后面是否是类型名：
 * - 如果是类型名，则可能是模板实例化
 * - 如果不是类型名，则一定是比较表达式
 *
 * 注意：在解析阶段无法区分标识符是类型名还是变量名，
 * 因此只判断明确的类型关键字。用户定义的类型名作为模板参数
 * 的情况较少见，可以在语义分析阶段处理。
 *
 * @param parser 解析器实例
 * @return true 如果当前 token 是明确的类型关键字，false 否则
 */
static bool is_type_start(CnParser *parser)
{
    if (!parser || !parser->has_current) {
        return false;
    }

    switch (parser->current.kind) {
        case CN_TOKEN_KEYWORD_INT:      // 整数
        case CN_TOKEN_KEYWORD_FLOAT:    // 浮点
        case CN_TOKEN_KEYWORD_STRING:   // 字符串
        case CN_TOKEN_KEYWORD_BOOL:     // 布尔
        case CN_TOKEN_KEYWORD_VOID:     // 空类型
        case CN_TOKEN_KEYWORD_STRUCT:   // 结构体
        case CN_TOKEN_KEYWORD_ENUM:     // 枚举
            // 注意：不包含 CN_TOKEN_IDENT，因为在解析阶段
            // 无法区分标识符是类型名还是变量名
            return true;
        default:
            return false;
    }
}

CnParser *cn_frontend_parser_new(CnLexer *lexer)
{
    CnParser *parser;

    if (!lexer) {
        return NULL;
    }

    parser = (CnParser *)malloc(sizeof(CnParser));
    if (!parser) {
        return NULL;
    }

    parser->lexer = lexer;
    parser->has_current = 0;
    parser->error_count = 0;
    parser->diagnostics = NULL;
    parser->current_visibility = CN_VISIBILITY_PRIVATE;  // 默认私有
    parser->in_function_body = 0;  // 初始不在函数体内

    return parser;
}

void cn_frontend_parser_free(CnParser *parser)
{
    free(parser);
}

void cn_frontend_parser_set_diagnostics(CnParser *parser, CnDiagnostics *diagnostics)
{
    if (!parser) {
        return;
    }

    parser->diagnostics = diagnostics;
}

bool cn_frontend_parse_program(CnParser *parser, CnAstProgram **out_program)
{
    CnAstProgram *program;

    if (!parser || !out_program) {
        return false;
    }

    program = parse_program_internal(parser);
    if (!program) {
        return false;
    }

    *out_program = program;
    return parser->error_count == 0;
}

static void parser_advance(CnParser *parser)
{
    if (!parser) {
        return;
    }

    if (!cn_frontend_lexer_next_token(parser->lexer, &parser->current)) {
        parser->current.kind = CN_TOKEN_EOF;
    }

    parser->has_current = 1;
}

// 向前看下一个token（不消耗当前token）
static CnTokenKind parser_peek(CnParser *parser)
{
    if (!parser || !parser->lexer) {
        return CN_TOKEN_EOF;
    }
    
    // 保存当前lexer状态
    const char *saved_source = parser->lexer->source;
    const char *saved_filename = parser->lexer->filename;
    size_t saved_length = parser->lexer->length;
    size_t saved_offset = parser->lexer->offset;
    int saved_line = parser->lexer->line;
    int saved_column = parser->lexer->column;
    
    // 读取下一个token
    CnToken next_token;
    if (!cn_frontend_lexer_next_token(parser->lexer, &next_token)) {
        next_token.kind = CN_TOKEN_EOF;
    }
    
    // 恢复lexer状态
    parser->lexer->source = saved_source;
    parser->lexer->filename = saved_filename;
    parser->lexer->length = saved_length;
    parser->lexer->offset = saved_offset;
    parser->lexer->line = saved_line;
    parser->lexer->column = saved_column;
    
    return next_token.kind;
}

static int parser_match(CnParser *parser, CnTokenKind kind)
{
    if (!parser->has_current) {
        parser_advance(parser);
    }

    if (parser->current.kind == kind) {
        parser_advance(parser);
        return 1;
    }

    return 0;
}

static int parser_expect(CnParser *parser, CnTokenKind kind)
{
    if (parser_match(parser, kind)) {
        return 1;
    }

    parser->error_count++;
    if (parser->diagnostics) {
        cn_support_diagnostics_report(parser->diagnostics,
                                      CN_DIAG_SEVERITY_ERROR,
                                      CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                      parser->lexer ? parser->lexer->filename : NULL,
                                      parser->current.line,
                                      parser->current.column,
                                      "语法错误：缺少预期的标记");
    }
    return 0;
}

// 检查是否为预留关键字
static int is_reserved_keyword(CnTokenKind kind)
{
    return kind == CN_TOKEN_KEYWORD_TEMPLATE ||
           kind == CN_TOKEN_KEYWORD_NAMESPACE;
           /* 注意："常量" 关键字已在当前版本实现，不再视为预留关键字 */
           /* 注意："静态" 关键字已在当前版本实现，用于函数内静态局部变量 */
           /* 注意："公开"、"私有" 关键字已在模块系统中实现，不再视为预留关键字 */
           /* 注意："类"、"接口" 关键字已在阶段11实现，不再视为预留关键字 */
           /* 注意："重写" 关键字已在阶段11实现，用于标记重写基类方法 */
           /* 注意："保护" 关键字已在阶段11实现，用于类成员访问控制 */
           /* 注意："虚拟" 关键字已在阶段11实现，用于虚函数声明 */
           /* 注意："抽象" 关键字已在阶段11实现，用于抽象类和纯虚函数 */
}

// 获取预留关键字的错误消息（静态字符串）
static const char *get_reserved_keyword_error_msg(CnTokenKind kind)
{
    switch (kind) {
    case CN_TOKEN_KEYWORD_TEMPLATE:
        return "语法错误：关键字 '模板' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_NAMESPACE:
        return "语法错误：关键字 '命名空间' 为预留特性，当前版本暂不支持";
    /* 注意："保护"、"虚拟"、"抽象" 关键字已在阶段11实现，不再视为预留关键字 */
    default:
        return "语法错误：遇到未知的预留关键字";
    }
}

// 检测并报告预留关键字错误
static void check_reserved_keyword(CnParser *parser)
{
    if (!parser || !parser->has_current) {
        return;
    }

    if (is_reserved_keyword(parser->current.kind)) {
        parser->error_count++;
        if (parser->diagnostics) {
            const char *error_msg = get_reserved_keyword_error_msg(parser->current.kind);
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_RESERVED_FEATURE,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          error_msg);
        }
        // 跳过该关键字，继续解析
        parser_advance(parser);
    }
}

static CnAstProgram *parse_program_internal(CnParser *parser)
{
    CnAstProgram *program = make_program();
    // 当前可见性已在 parser 初始化时设置（默认为私有）

    parser_advance(parser);

    while (parser->current.kind != CN_TOKEN_EOF) {
        // 检查是否为预留关键字
        check_reserved_keyword(parser);
        
        // 检查文件级公开/私有块声明
        // 语法：公开: 或 私有:
        if (parser->current.kind == CN_TOKEN_KEYWORD_PUBLIC) {
            parser_advance(parser);
            if (parser->current.kind == CN_TOKEN_COLON) {
                // 进入公开块
                parser->current_visibility = CN_VISIBILITY_PUBLIC;
                parser_advance(parser);
                continue;
            } else {
                // 错误：期望 ':'
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：'公开' 后期望 ':'");
                }
                continue;
            }
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_PRIVATE) {
            parser_advance(parser);
            if (parser->current.kind == CN_TOKEN_COLON) {
                // 进入私有块
                parser->current_visibility = CN_VISIBILITY_PRIVATE;
                parser_advance(parser);
                continue;
            } else {
                // 错误：期望 ':'
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：'私有' 后期望 ':'");
                }
                continue;
            }
        }

        if (parser->current.kind == CN_TOKEN_KEYWORD_IMPORT) {
            // 解析导入语句
            CnAstStmt *import_stmt = parse_import_stmt(parser);
            if (!import_stmt) {
                break;
            }
            program_add_import(program, import_stmt);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_FROM) {
            // 解析 "从...导入" 语句
            CnAstStmt *import_stmt = parse_from_import_stmt(parser);
            if (!import_stmt) {
                break;
            }
            program_add_import(program, import_stmt);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRUCT) {
            // 解析结构体声明或结构体类型的全局变量声明
            CnAstStmt *stmt = parse_struct_decl(parser);
            if (!stmt) {
                break;
            }
            if (stmt->kind == CN_AST_STMT_STRUCT_DECL) {
                program_add_struct(program, stmt);
            } else if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                program_add_global_var(program, stmt);
            } else {
                // 其他情况视为错误，终止解析
                break;
            }
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_ENUM) {
            // 解析枚举声明
            CnAstStmt *enum_decl = parse_enum_decl(parser);
            if (!enum_decl) {
                break;
            }
            program_add_enum(program, enum_decl);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_CLASS) {
            // 解析类声明（阶段11 - 面向对象编程支持）
            CnAstStmt *class_decl = parse_class_decl(parser);
            if (!class_decl) {
                break;
            }
            program_add_class(program, class_decl);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_INTERFACE) {
            // 解析接口声明（阶段11 - 面向对象编程支持）
            CnAstStmt *interface_decl = parse_interface_decl(parser);
            if (!interface_decl) {
                break;
            }
            program_add_interface(program, interface_decl);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_TEMPLATE) {
            // 解析模板声明（阶段13 - 泛型编程支持）
            CnAstStmt *template_decl = parse_template_declaration(parser);
            if (!template_decl) {
                break;
            }
            // 根据类型添加到对应列表
            if (template_decl->kind == CN_AST_STMT_TEMPLATE_FUNCTION_DECL) {
                program_add_template_func(program, template_decl);
            } else if (template_decl->kind == CN_AST_STMT_TEMPLATE_STRUCT_DECL) {
                program_add_template_struct(program, template_decl);
            }
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_FN) {
            // 解析函数声明
            CnAstFunctionDecl *fn = parse_function_decl(parser);
            if (!fn) {
                break;
            }
            program_add_function(program, fn);
        /* 注意:CN_TOKEN_KEYWORD_INTERRUPT_HANDLER 已删除
         * 中断处理功能将通过运行时库函数提供
         * 原语法: 中断处理 向量号 () { ... }
         * 新方式: 使用运行时API注册中断处理函数
         */
        /* 已禁用的中断处理解析
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_INTERRUPT_HANDLER) {
            // 解析中断处理函数
            CnAstFunctionDecl *isr = parse_interrupt_handler(parser);
            if (!isr) {
                break;
            }
            program_add_function(program, isr);
        */
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_VAR || 
                   parser->current.kind == CN_TOKEN_KEYWORD_CONST) {
            // 解析全局变量声明
            CnAstStmt *var_decl = parse_statement(parser);
            if (!var_decl) {
                break;
            }
            // 应用当前块的可见性
            if (var_decl->kind == CN_AST_STMT_VAR_DECL) {
                var_decl->as.var_decl.visibility = parser->current_visibility;
            }
            program_add_global_var(program, var_decl);
        } else if (parser->current.kind == CN_TOKEN_SEMICOLON) {
            // 跳过结构体/枚举声明后的可选分号
            // 例如: 结构体 Foo { ... };
            parser_advance(parser);
        } else {
            // 遇到无法识别的token，跳过
            parser->error_count++;
            parser_advance(parser);
        }
    }

    return program;
}

static CnAstFunctionDecl *parse_function_decl(CnParser *parser)
{
    CnAstFunctionDecl *fn;
    CnAstBlockStmt *body;
    CnAstParameter *params = NULL;
    size_t param_count = 0;
    size_t param_capacity = 0;
    CnType *return_type = NULL;

    if (!parser_expect(parser, CN_TOKEN_KEYWORD_FN)) {
        return NULL;
    }

    if (!parser->has_current) {
        parser_advance(parser);
    }

    // 函数名必须是标识符（"主程序"现在作为普通标识符而非关键字）
    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：函数名无效，期望标识符");
        }
        return NULL;
    }

    fn = (CnAstFunctionDecl *)malloc(sizeof(CnAstFunctionDecl));
    if (!fn) {
        return NULL;
    }

    fn->name = parser->current.lexeme_begin;
    fn->name_length = parser->current.lexeme_length;
    fn->parameters = NULL;
    fn->parameter_count = 0;
    fn->return_type = NULL;
    fn->body = NULL;
    fn->visibility = parser->current_visibility;  // 使用当前块的可见性
    fn->is_interrupt_handler = 0;  // 普通函数
    fn->interrupt_vector = 0;

    parser_advance(parser);

    parser_expect(parser, CN_TOKEN_LPAREN);

    // 解析参数列表
    if (parser->current.kind != CN_TOKEN_RPAREN) {
        param_capacity = 4;
        params = (CnAstParameter *)malloc(sizeof(CnAstParameter) * param_capacity);
        if (!params) {
            free(fn);
            return NULL;
        }

        do {
            int param_is_const = 0;
            
            // 处理 '常量' 关键字：常量参数声明
            if (parser->current.kind == CN_TOKEN_KEYWORD_CONST) {
                param_is_const = 1;
                parser_advance(parser);
            }
            
            // 使用统一的 parse_type 解析参数类型
            CnType *param_type = parse_type(parser);
            if (!param_type) {
                // 类型解析失败，尝试容错：允许 '变量' 关键字
                if (parser->current.kind == CN_TOKEN_KEYWORD_VAR) {
                    param_type = NULL; // 无显式类型，后续通过类型推断
                    parser_advance(parser);
                } else {
                    // 类型解析失败且不是 '变量' 关键字
                    free(params);
                    free(fn);
                    return NULL;
                }
            }

            // 检查是否是函数指针参数语法：返回类型(*)(参数列表) 后跟参数名
            // 或者是普通参数语法：类型 参数名
            const char *param_name = NULL;
            size_t param_name_length = 0;
            
            // 检查是否是函数指针参数语法：类型 后跟 '(' 且后面有 '*'
            // 例如：整数(*)(整数, 整数) 回调 或 整数(*回调)(整数, 整数)
            if (parser->current.kind == CN_TOKEN_LPAREN) {
                // 可能是函数指针参数：返回类型(*参数名)(参数列表)
                // 保存状态以便回退
                CnToken saved_token = parser->current;
                int saved_has_current = parser->has_current;
                
                parser_advance(parser); // 跳过 '('
                
                if (parser->current.kind == CN_TOKEN_STAR) {
                    parser_advance(parser); // 跳过 '*'
                    
                    // 检查下一个 token：可能是参数名或 ')'
                    if (parser->current.kind == CN_TOKEN_IDENT) {
                        // 函数指针参数名嵌入在类型中：返回类型(*参数名)(参数列表)
                        param_name = parser->current.lexeme_begin;
                        param_name_length = parser->current.lexeme_length;
                        parser_advance(parser);
                    }
                    
                    // 期望 ')'
                    if (parser_expect(parser, CN_TOKEN_RPAREN)) {
                        // 解析参数列表：'(' 类型1, 类型2, ... ')'
                        if (parser_expect(parser, CN_TOKEN_LPAREN)) {
                            size_t fp_param_capacity = 4;
                            size_t fp_param_count = 0;
                            CnType **fp_param_types = (CnType **)malloc(sizeof(CnType*) * fp_param_capacity);
                            if (!fp_param_types) {
                                free(params);
                                free(fn);
                                return NULL;
                            }
                            
                            // 解析参数类型，允许空参数列表
                            if (parser->current.kind != CN_TOKEN_RPAREN) {
                                do {
                                    CnType *fp_param_type = parse_type(parser);
                                    if (!fp_param_type) {
                                        free(fp_param_types);
                                        free(params);
                                        free(fn);
                                        return NULL;
                                    }
                                    
                                    // 扩容
                                    if (fp_param_count >= fp_param_capacity) {
                                        fp_param_capacity *= 2;
                                        CnType **new_fp_param_types = (CnType **)realloc(
                                            fp_param_types, sizeof(CnType*) * fp_param_capacity);
                                        if (!new_fp_param_types) {
                                            free(fp_param_types);
                                            free(params);
                                            free(fn);
                                            return NULL;
                                        }
                                        fp_param_types = new_fp_param_types;
                                    }
                                    
                                    fp_param_types[fp_param_count++] = fp_param_type;
                                    
                                    if (parser->current.kind == CN_TOKEN_COMMA) {
                                        parser_advance(parser);
                                    } else {
                                        break;
                                    }
                                } while (parser->current.kind != CN_TOKEN_RPAREN &&
                                         parser->current.kind != CN_TOKEN_EOF);
                            }
                            
                            if (parser_expect(parser, CN_TOKEN_RPAREN)) {
                                // 创建函数类型和函数指针类型
                                CnType *func_type = cn_type_new_function(param_type, fp_param_types, fp_param_count);
                                param_type = cn_type_new_pointer(func_type);
                                // 参数名可能已经在上面解析，如果没有则在后面解析
                            } else {
                                free(fp_param_types);
                                free(params);
                                free(fn);
                                return NULL;
                            }
                        } else {
                            // 解析失败，回退
                            parser->current = saved_token;
                            parser->has_current = saved_has_current;
                        }
                    } else {
                        // 解析失败，回退
                        parser->current = saved_token;
                        parser->has_current = saved_has_current;
                    }
                } else {
                    // 不是函数指针，回退
                    parser->current = saved_token;
                    parser->has_current = saved_has_current;
                }
            }
            
            // 如果参数名还没有解析，尝试解析参数名
            if (param_name == NULL && parser->current.kind == CN_TOKEN_IDENT) {
                param_name = parser->current.lexeme_begin;
                param_name_length = parser->current.lexeme_length;
                parser_advance(parser);
            }
            
            // 对于函数指针参数，参数名可以是可选的（匿名参数）
            // 但对于普通参数，必须有参数名
            if (param_name == NULL && param_type != NULL &&
                param_type->kind != CN_TYPE_POINTER) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少参数名");
                }
                free(params);
                free(fn);
                return NULL;
            }

            if (param_count >= param_capacity) {
                param_capacity *= 2;
                CnAstParameter *new_params = (CnAstParameter *)realloc(
                    params, sizeof(CnAstParameter) * param_capacity);
                if (!new_params) {
                    free(params);
                    free(fn);
                    return NULL;
                }
                params = new_params;
            }

            params[param_count].name = param_name;
            params[param_count].name_length = param_name_length;
            params[param_count].is_const = param_is_const;

            // 检查参数名后是否有 [] 语法（如：整数 arr[] 或 整数 arr[10]）
            // 这表示该参数是数组参数，在 C 中实际上会退化为指针
            if (parser->current.kind == CN_TOKEN_LBRACKET) {
                parser_advance(parser);  // 跳过 '['
                
                size_t array_size = 0;
                
                // 检查是否指定了数组大小
                if (parser->current.kind != CN_TOKEN_RBRACKET) {
                    // 解析数组大小（简化为只接受整数字面量）
                    if (parser->current.kind == CN_TOKEN_INTEGER) {
                        array_size = (size_t)strtol(parser->current.lexeme_begin, NULL, 10);
                        parser_advance(parser);
                    } else {
                        parser->error_count++;
                        if (parser->diagnostics) {
                            cn_support_diagnostics_report(parser->diagnostics,
                                                          CN_DIAG_SEVERITY_ERROR,
                                                          CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                                          parser->lexer ? parser->lexer->filename : NULL,
                                                          parser->current.line,
                                                          parser->current.column,
                                                          "语法错误：数组参数大小必须是整数字面量或省略");
                        }
                        free(params);
                        free(fn);
                        return NULL;
                    }
                }
                
                if (!parser_expect(parser, CN_TOKEN_RBRACKET)) {
                    free(params);
                    free(fn);
                    return NULL;
                }
                
                // 将参数类型转换为指针类型（C 语言中数组参数会退化为指针）
                // 如果指定了大小，也记录为数组类型（虽然在 C 中仍是指针，但保留语义信息）
                if (param_type) {
                    if (array_size > 0) {
                        // 保留数组语义信息
                        param_type = cn_type_new_array(param_type, array_size);
                    } else {
                        // 无大小的数组参数，转换为指针类型
                        param_type = cn_type_new_pointer(param_type);
                    }
                }
            }
            
            params[param_count].declared_type = param_type;
            param_count++;

            if (parser->current.kind == CN_TOKEN_COMMA) {
                parser_advance(parser);
            } else {
                break;
            }
        } while (parser->current.kind != CN_TOKEN_RPAREN &&
                 parser->current.kind != CN_TOKEN_EOF);
    }

    parser_expect(parser, CN_TOKEN_RPAREN);

    fn->parameters = params;
    fn->parameter_count = param_count;

    // 解析返回类型（可选）：-> 类型
    if (parser->current.kind == CN_TOKEN_ARROW) {
        parser_advance(parser); // 跳过 '->'
        
        return_type = parse_type(parser);
        if (!return_type) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：'->' 后必须指定返回类型");
            }
            free(params);
            free(fn);
            return NULL;
        }
    }

    fn->return_type = return_type;

    // 设置函数体上下文标志，用于静态变量作用域检查
    parser->in_function_body = 1;
    body = parse_block(parser);
    parser->in_function_body = 0;  // 解析完成后恢复
    fn->body = body;

    return fn;
}

// 解析中断处理函数声明
// 语法：中断处理 向量号 () { ... }
// 注意：parse_interrupt_handler 函数已废弃
// 中断处理功能已迁移到运行时库函数
/* 已禁用的中断处理解析函数
static CnAstFunctionDecl *parse_interrupt_handler(CnParser *parser)
{
    CnAstFunctionDecl *isr;
    CnAstBlockStmt *body;
    uint32_t vector_num = 0;

    // CN_TOKEN_KEYWORD_INTERRUPT_HANDLER 已删除
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_INTERRUPT_HANDLER)) {
        return NULL;
    }

    if (!parser->has_current) {
        parser_advance(parser);
    }

    // 中断向量号必须是整数字面量
    if (parser->current.kind != CN_TOKEN_INTEGER) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：中断向量号必须是整数字面量");
        }
        return NULL;
    }

    // 获取中断向量号
    vector_num = (uint32_t)strtol(parser->current.lexeme_begin, NULL, 10);

    // 验证向量号范围（基本检查）
    if (vector_num >= 256) {  // 大多数架构中断向量号 < 256
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：中断向量号超出范围 (0-255)");
        }
        return NULL;
    }

    isr = (CnAstFunctionDecl *)malloc(sizeof(CnAstFunctionDecl));
    if (!isr) {
        return NULL;
    }

    // 中断处理函数的名称为 "__isr_<向量号>"
    char *isr_name = (char *)malloc(32);
    if (!isr_name) {
        free(isr);
        return NULL;
    }
    snprintf(isr_name, 32, "__isr_%u", vector_num);

    isr->name = isr_name;
    isr->name_length = strlen(isr_name);
    isr->parameters = NULL;      // 中断处理函数不允许有参数
    isr->parameter_count = 0;
    isr->body = NULL;
    isr->is_interrupt_handler = 1;  // 标记为中断处理函数
    isr->interrupt_vector = vector_num;

    parser_advance(parser);

    // 中断处理函数不允许有参数，必须是 ()
    parser_expect(parser, CN_TOKEN_LPAREN);
    
    if (parser->current.kind != CN_TOKEN_RPAREN) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：中断处理函数不允许有参数");
        }
        free((void*)isr->name);
        free(isr);
        return NULL;
    }

    parser_expect(parser, CN_TOKEN_RPAREN);

    // 解析函数体
    body = parse_block(parser);
    isr->body = body;

    return isr;
}
*/

static CnAstBlockStmt *parse_block(CnParser *parser)
{
    CnAstBlockStmt *block;

    if (!parser_expect(parser, CN_TOKEN_LBRACE)) {
        return NULL;
    }

    block = make_block();

    while (parser->current.kind != CN_TOKEN_RBRACE &&
           parser->current.kind != CN_TOKEN_EOF) {
        CnAstStmt *stmt = parse_statement(parser);
        if (!stmt) {
            break;
        }
        block_add_stmt(block, stmt);
    }

    parser_expect(parser, CN_TOKEN_RBRACE);

    return block;
}

static CnAstStmt *parse_statement(CnParser *parser)
{
    CnAstExpr *expr;

    if (!parser->has_current) {
        parser_advance(parser);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_RETURN) {
        CnAstExpr *value = NULL;

        parser_advance(parser);

        if (parser->current.kind != CN_TOKEN_SEMICOLON) {
            value = parse_expression(parser);
        }

        parser_expect(parser, CN_TOKEN_SEMICOLON);

        return make_return_stmt(value);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_IF) {
        CnAstExpr *condition;
        CnAstBlockStmt *then_block;
        CnAstBlockStmt *else_block = NULL;

        parser_advance(parser);

        parser_expect(parser, CN_TOKEN_LPAREN);
        condition = parse_expression(parser);
        parser_expect(parser, CN_TOKEN_RPAREN);

        then_block = parse_block(parser);

        if (parser->current.kind == CN_TOKEN_KEYWORD_ELSE) {
            parser_advance(parser);
            
            // 支持 "否则 如果" (else if) 语法
            if (parser->current.kind == CN_TOKEN_KEYWORD_IF) {
                // 将 "否则 如果" 处理为 else { if ... }
                // 递归解析 if 语句
                CnAstStmt *nested_if = parse_statement(parser);
                if (nested_if && nested_if->kind == CN_AST_STMT_IF) {
                    // 创建一个只包含 if 语句的 else 块
                    else_block = make_block();
                    block_add_stmt(else_block, nested_if);
                } else {
                    // 解析失败，返回 NULL
                    if (nested_if) {
                        cn_frontend_ast_stmt_free(nested_if);
                    }
                    cn_frontend_ast_expr_free(condition);
                    cn_frontend_ast_block_free(then_block);
                    return NULL;
                }
            } else {
                // 普通的 else 块
                else_block = parse_block(parser);
            }
        }

        return make_if_stmt(condition, then_block, else_block);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_WHILE) {
        CnAstExpr *condition;
        CnAstBlockStmt *body;

        parser_advance(parser);

        parser_expect(parser, CN_TOKEN_LPAREN);
        condition = parse_expression(parser);
        parser_expect(parser, CN_TOKEN_RPAREN);

        body = parse_block(parser);

        return make_while_stmt(condition, body);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_FOR) {
        CnAstStmt *init = NULL;
        CnAstExpr *condition = NULL;
        CnAstExpr *update = NULL;
        CnAstBlockStmt *body;

        parser_advance(parser);

        parser_expect(parser, CN_TOKEN_LPAREN);

        if (parser->current.kind != CN_TOKEN_SEMICOLON) {
            init = parse_statement(parser);
        } else {
            parser_advance(parser);
        }

        if (parser->current.kind != CN_TOKEN_SEMICOLON) {
            condition = parse_expression(parser);
        }
        parser_expect(parser, CN_TOKEN_SEMICOLON);

        if (parser->current.kind != CN_TOKEN_RPAREN) {
            update = parse_expression(parser);
        }
        parser_expect(parser, CN_TOKEN_RPAREN);

        body = parse_block(parser);

        return make_for_stmt(init, condition, update, body);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_BREAK) {
        parser_advance(parser);
        parser_expect(parser, CN_TOKEN_SEMICOLON);
        return make_break_stmt();
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_CONTINUE) {
        parser_advance(parser);
        parser_expect(parser, CN_TOKEN_SEMICOLON);
        return make_continue_stmt();
    }

    // 解析局部结构体定义（支持在函数内部定义结构体）
    if (parser->current.kind == CN_TOKEN_KEYWORD_STRUCT) {
        return parse_struct_decl(parser);
    }

    // 解析局部枚举定义（支持在函数内部定义枚举）
    if (parser->current.kind == CN_TOKEN_KEYWORD_ENUM) {
        return parse_enum_decl(parser);
    }

    // 解析 switch 语句：选择 (表达式) { [情况 值: 语句...]* [默认: 语句...]? }
    if (parser->current.kind == CN_TOKEN_KEYWORD_SWITCH) {
        CnAstExpr *switch_expr;
        CnAstSwitchCase *cases = NULL;
        size_t case_count = 0;
        size_t case_capacity = 4;

        parser_advance(parser); // 跳过 '选择'

        // 期望 '('
        if (!parser_expect(parser, CN_TOKEN_LPAREN)) {
            return NULL;
        }

        // 解析 switch 表达式
        switch_expr = parse_expression(parser);
        if (!switch_expr) {
            return NULL;
        }

        // 期望 ')'
        if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
            cn_frontend_ast_expr_free(switch_expr);
            return NULL;
        }

        // 期望 '{'
        if (!parser_expect(parser, CN_TOKEN_LBRACE)) {
            cn_frontend_ast_expr_free(switch_expr);
            return NULL;
        }

        // 分配 case 数组
        cases = (CnAstSwitchCase *)malloc(sizeof(CnAstSwitchCase) * case_capacity);
        if (!cases) {
            cn_frontend_ast_expr_free(switch_expr);
            return NULL;
        }

        // 解析 case 和 default 分支
        while (parser->current.kind != CN_TOKEN_RBRACE && parser->current.kind != CN_TOKEN_EOF) {
            CnAstExpr *case_value = NULL;
            CnAstBlockStmt *case_body = NULL;

            if (parser->current.kind == CN_TOKEN_KEYWORD_CASE) {
                // 解析 case 分支
                parser_advance(parser); // 跳过 '情况'

                // 解析 case 值表达式
                case_value = parse_expression(parser);
                if (!case_value) {
                    // 清理已分配的资源
                    for (size_t i = 0; i < case_count; i++) {
                        cn_frontend_ast_expr_free(cases[i].value);
                        cn_frontend_ast_block_free(cases[i].body);
                    }
                    free(cases);
                    cn_frontend_ast_expr_free(switch_expr);
                    return NULL;
                }

                // 期望 ':'
                if (!parser_expect(parser, CN_TOKEN_COLON)) {
                    cn_frontend_ast_expr_free(case_value);
                    for (size_t i = 0; i < case_count; i++) {
                        cn_frontend_ast_expr_free(cases[i].value);
                        cn_frontend_ast_block_free(cases[i].body);
                    }
                    free(cases);
                    cn_frontend_ast_expr_free(switch_expr);
                    return NULL;
                }
            } else if (parser->current.kind == CN_TOKEN_KEYWORD_DEFAULT) {
                // 解析 default 分支
                parser_advance(parser); // 跳过 '默认'

                // default 不需要值表达式
                case_value = NULL;

                // 期望 ':'
                if (!parser_expect(parser, CN_TOKEN_COLON)) {
                    for (size_t i = 0; i < case_count; i++) {
                        cn_frontend_ast_expr_free(cases[i].value);
                        cn_frontend_ast_block_free(cases[i].body);
                    }
                    free(cases);
                    cn_frontend_ast_expr_free(switch_expr);
                    return NULL;
                }
            } else {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：switch 语句中期望 'case' 或 'default'");
                }
                for (size_t i = 0; i < case_count; i++) {
                    cn_frontend_ast_expr_free(cases[i].value);
                    cn_frontend_ast_block_free(cases[i].body);
                }
                free(cases);
                cn_frontend_ast_expr_free(switch_expr);
                return NULL;
            }

            // 解析 case/default 的语句序列（不需要大括号）
            case_body = make_block();
            if (!case_body) {
                cn_frontend_ast_expr_free(case_value);
                for (size_t i = 0; i < case_count; i++) {
                    cn_frontend_ast_expr_free(cases[i].value);
                    cn_frontend_ast_block_free(cases[i].body);
                }
                free(cases);
                cn_frontend_ast_expr_free(switch_expr);
                return NULL;
            }

            // 解析语句，直到遇到下一个 case/default 或 }
            while (parser->current.kind != CN_TOKEN_KEYWORD_CASE &&
                   parser->current.kind != CN_TOKEN_KEYWORD_DEFAULT &&
                   parser->current.kind != CN_TOKEN_RBRACE &&
                   parser->current.kind != CN_TOKEN_EOF) {
                CnAstStmt *stmt = parse_statement(parser);
                if (!stmt) {
                    cn_frontend_ast_expr_free(case_value);
                    cn_frontend_ast_block_free(case_body);
                    for (size_t i = 0; i < case_count; i++) {
                        cn_frontend_ast_expr_free(cases[i].value);
                        cn_frontend_ast_block_free(cases[i].body);
                    }
                    free(cases);
                    cn_frontend_ast_expr_free(switch_expr);
                    return NULL;
                }
                block_add_stmt(case_body, stmt);
            }

            // 扩容 case 数组
            if (case_count >= case_capacity) {
                case_capacity *= 2;
                CnAstSwitchCase *new_cases = (CnAstSwitchCase *)realloc(cases, sizeof(CnAstSwitchCase) * case_capacity);
                if (!new_cases) {
                    cn_frontend_ast_expr_free(case_value);
                    cn_frontend_ast_block_free(case_body);
                    for (size_t i = 0; i < case_count; i++) {
                        cn_frontend_ast_expr_free(cases[i].value);
                        cn_frontend_ast_block_free(cases[i].body);
                    }
                    free(cases);
                    cn_frontend_ast_expr_free(switch_expr);
                    return NULL;
                }
                cases = new_cases;
            }

            // 添加当前 case/default
            cases[case_count].value = case_value;
            cases[case_count].body = case_body;
            case_count++;
        }

        // 期望 '}'
        if (!parser_expect(parser, CN_TOKEN_RBRACE)) {
            for (size_t i = 0; i < case_count; i++) {
                cn_frontend_ast_expr_free(cases[i].value);
                cn_frontend_ast_block_free(cases[i].body);
            }
            free(cases);
            cn_frontend_ast_expr_free(switch_expr);
            return NULL;
        }

        return make_switch_stmt(switch_expr, cases, case_count);
    }

    // 解析 try-catch-finally 语句：尝试 { ... } 捕获 (类型 变量) { ... } [最终 { ... }]
    if (parser->current.kind == CN_TOKEN_KEYWORD_TRY) {
        CnAstBlockStmt *try_block = NULL;
        CnAstCatchClause *catches = NULL;
        size_t catch_count = 0;
        size_t catch_capacity = 2;
        CnAstBlockStmt *finally_block = NULL;

        parser_advance(parser); // 跳过 '尝试'

        // 解析 try 块
        try_block = parse_block(parser);
        if (!try_block) {
            return NULL;
        }

        // 分配 catch 数组
        catches = (CnAstCatchClause *)malloc(sizeof(CnAstCatchClause) * catch_capacity);
        if (!catches) {
            cn_frontend_ast_block_free(try_block);
            return NULL;
        }

        // 解析一个或多个 catch 子句
        while (parser->current.kind == CN_TOKEN_KEYWORD_CATCH) {
            const char *exception_type = NULL;
            size_t exception_type_length = 0;
            const char *var_name = NULL;
            size_t var_name_length = 0;
            CnAstBlockStmt *catch_body = NULL;

            parser_advance(parser); // 跳过 '捕获'

            // 检查是否有异常类型声明
            if (parser->current.kind == CN_TOKEN_LPAREN) {
                parser_advance(parser); // 跳过 '('

                // 解析异常类型（可选）
                if (parser->current.kind == CN_TOKEN_IDENT) {
                    exception_type = parser->current.lexeme_begin;
                    exception_type_length = parser->current.lexeme_length;
                    parser_advance(parser);

                    // 检查是否有异常变量
                    if (parser->current.kind == CN_TOKEN_IDENT) {
                        var_name = parser->current.lexeme_begin;
                        var_name_length = parser->current.lexeme_length;
                        parser_advance(parser);
                    }
                }
                // 如果是 ')' 则表示捕获所有异常

                if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                    // 清理资源
                    for (size_t i = 0; i < catch_count; i++) {
                        cn_frontend_ast_block_free(catches[i].body);
                    }
                    free(catches);
                    cn_frontend_ast_block_free(try_block);
                    return NULL;
                }
            }

            // 解析 catch 块
            catch_body = parse_block(parser);
            if (!catch_body) {
                for (size_t i = 0; i < catch_count; i++) {
                    cn_frontend_ast_block_free(catches[i].body);
                }
                free(catches);
                cn_frontend_ast_block_free(try_block);
                return NULL;
            }

            // 扩容 catch 数组
            if (catch_count >= catch_capacity) {
                catch_capacity *= 2;
                CnAstCatchClause *new_catches = (CnAstCatchClause *)realloc(catches,
                    sizeof(CnAstCatchClause) * catch_capacity);
                if (!new_catches) {
                    cn_frontend_ast_block_free(catch_body);
                    for (size_t i = 0; i < catch_count; i++) {
                        cn_frontend_ast_block_free(catches[i].body);
                    }
                    free(catches);
                    cn_frontend_ast_block_free(try_block);
                    return NULL;
                }
                catches = new_catches;
            }

            // 添加当前 catch 子句
            catches[catch_count].exception_type = exception_type;
            catches[catch_count].exception_type_length = exception_type_length;
            catches[catch_count].var_name = var_name;
            catches[catch_count].var_name_length = var_name_length;
            catches[catch_count].body = catch_body;
            catch_count++;
        }

        // 解析 finally 子句（可选）
        if (parser->current.kind == CN_TOKEN_KEYWORD_FINALLY) {
            parser_advance(parser); // 跳过 '最终'
            finally_block = parse_block(parser);
            if (!finally_block) {
                for (size_t i = 0; i < catch_count; i++) {
                    cn_frontend_ast_block_free(catches[i].body);
                }
                free(catches);
                cn_frontend_ast_block_free(try_block);
                return NULL;
            }
        }

        // 创建 try 语句节点
        CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
        if (!stmt) {
            if (finally_block) cn_frontend_ast_block_free(finally_block);
            for (size_t i = 0; i < catch_count; i++) {
                cn_frontend_ast_block_free(catches[i].body);
            }
            free(catches);
            cn_frontend_ast_block_free(try_block);
            return NULL;
        }

        stmt->kind = CN_AST_STMT_TRY;
        stmt->loc.filename = parser->lexer ? parser->lexer->filename : NULL;
        stmt->loc.line = parser->current.line;
        stmt->loc.column = parser->current.column;

        CnAstTryStmt *try_stmt = (CnAstTryStmt *)malloc(sizeof(CnAstTryStmt));
        if (!try_stmt) {
            free(stmt);
            if (finally_block) cn_frontend_ast_block_free(finally_block);
            for (size_t i = 0; i < catch_count; i++) {
                cn_frontend_ast_block_free(catches[i].body);
            }
            free(catches);
            cn_frontend_ast_block_free(try_block);
            return NULL;
        }

        try_stmt->try_block = try_block;
        try_stmt->catches = catches;
        try_stmt->catch_count = catch_count;
        try_stmt->finally_block = finally_block;

        stmt->as.try_stmt = try_stmt;
        return stmt;
    }

    // 解析 throw 语句：抛出 表达式; 或 抛出 "类型名" "消息";
    if (parser->current.kind == CN_TOKEN_KEYWORD_THROW) {
        CnAstExpr *exception_expr = NULL;
        const char *exception_type = NULL;
        size_t exception_type_length = 0;
        const char *message = NULL;
        size_t message_length = 0;

        parser_advance(parser); // 跳过 '抛出'

        // 检查是否是字符串字面量（简单异常抛出）
        if (parser->current.kind == CN_TOKEN_STRING_LITERAL) {
            // 简单形式：抛出 "异常类型" "消息";
            exception_type = parser->current.lexeme_begin;
            exception_type_length = parser->current.lexeme_length;
            parser_advance(parser);

            // 可选的消息
            if (parser->current.kind == CN_TOKEN_STRING_LITERAL) {
                message = parser->current.lexeme_begin;
                message_length = parser->current.lexeme_length;
                parser_advance(parser);
            }
        } else {
            // 表达式形式：抛出 表达式;
            exception_expr = parse_expression(parser);
            if (!exception_expr) {
                return NULL;
            }
        }

        parser_expect(parser, CN_TOKEN_SEMICOLON);

        // 创建 throw 语句节点
        CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
        if (!stmt) {
            if (exception_expr) cn_frontend_ast_expr_free(exception_expr);
            return NULL;
        }

        stmt->kind = CN_AST_STMT_THROW;
        stmt->loc.filename = parser->lexer ? parser->lexer->filename : NULL;
        stmt->loc.line = parser->current.line;
        stmt->loc.column = parser->current.column;

        stmt->as.throw_stmt.exception_expr = exception_expr;
        stmt->as.throw_stmt.exception_type = exception_type;
        stmt->as.throw_stmt.exception_type_length = exception_type_length;
        stmt->as.throw_stmt.message = message;
        stmt->as.throw_stmt.message_length = message_length;

        return stmt;
    }

    // 支持变量声明：静态/常量/变量/基础类型/自定义类型(枚举/结构体)
    // 注意：当遇到标识符时，需要向前看判断是变量声明还是赋值语句
    // 变量声明格式：类型名 变量名 = 初始值; （标识符后跟另一个标识符）
    // 赋值语句格式：变量名 = 表达式; （标识符后跟 = 或其他操作符）
    int is_var_decl = 0;
    if (parser->current.kind == CN_TOKEN_KEYWORD_STATIC ||
        parser->current.kind == CN_TOKEN_KEYWORD_CONST ||
        parser->current.kind == CN_TOKEN_KEYWORD_VAR ||
        parser->current.kind == CN_TOKEN_KEYWORD_INT ||
        parser->current.kind == CN_TOKEN_KEYWORD_FLOAT ||
        parser->current.kind == CN_TOKEN_KEYWORD_STRING ||
        parser->current.kind == CN_TOKEN_KEYWORD_BOOL ||
        parser->current.kind == CN_TOKEN_KEYWORD_VOID) {
        is_var_decl = 1;
    } else if (parser->current.kind == CN_TOKEN_IDENT) {
        // 向前看判断是变量声明还是赋值语句
        // 变量声明：标识符(类型名) 标识符(变量名) ...
        // 赋值语句：标识符(变量名) = ...
        CnTokenKind next_kind = parser_peek(parser);
        if (next_kind == CN_TOKEN_IDENT) {
            // 类型名后跟标识符，是变量声明
            is_var_decl = 1;
        }
        // 否则不是变量声明，可能是赋值语句
    }
    
    if (is_var_decl) {
        const char *var_name;
        size_t var_name_length;
        CnAstExpr *initializer = NULL;
        CnType *declared_type = NULL;
        int is_const = 0;
        int is_static = 0;  // 静态变量标记
        
        // 保存类型名，用于构造函数调用
        const char *type_name = NULL;
        size_t type_name_length = 0;

        // 处理 '静态' 关键字：静态局部变量声明
        if (parser->current.kind == CN_TOKEN_KEYWORD_STATIC) {
            // 检查是否在函数内部
            if (!parser->in_function_body) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：静态关键字只能在函数内部使用");
                }
                // 跳过该声明，继续解析
                parser_advance(parser);
                while (parser->current.kind != CN_TOKEN_SEMICOLON &&
                       parser->current.kind != CN_TOKEN_EOF) {
                    parser_advance(parser);
                }
                if (parser->current.kind == CN_TOKEN_SEMICOLON) {
                    parser_advance(parser);
                }
                return NULL;
            }
            
            is_static = 1;
            parser_advance(parser);
            
            // 静态变量后可以跟类型关键字或 '变量' 关键字
            // 支持：静态 整数 x = 0; 或 静态 变量 x = 0;
            if (parser->current.kind == CN_TOKEN_KEYWORD_VAR) {
                parser_advance(parser);
                declared_type = NULL;  // 后续通过类型推断
            } else if (parser->current.kind == CN_TOKEN_KEYWORD_INT ||
                       parser->current.kind == CN_TOKEN_KEYWORD_FLOAT ||
                       parser->current.kind == CN_TOKEN_KEYWORD_STRING ||
                       parser->current.kind == CN_TOKEN_KEYWORD_BOOL ||
                       parser->current.kind == CN_TOKEN_KEYWORD_VOID) {
                declared_type = parse_type(parser);
                if (!declared_type) {
                    return NULL;
                }
            } else {
                // 静态关键字后必须有类型或变量关键字
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：静态关键字后必须有类型或变量关键字");
                }
                return NULL;
            }
        }
        // 处理 '常量' 关键字：常量变量声明
        else if (parser->current.kind == CN_TOKEN_KEYWORD_CONST) {
            is_const = 1;
            parser_advance(parser);

            // 支持：常量 变量 a = 1; 或 常量 整数 a = 1;
            if (parser->current.kind == CN_TOKEN_KEYWORD_VAR) {
                parser_advance(parser);
                declared_type = NULL; // 后续通过类型推断
            } else {
                // 使用统一的 parse_type 解析类型
                declared_type = parse_type(parser);
                if (!declared_type) {
                    return NULL;
                }
            }
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_VAR) {
            // 处理 '变量' 关键字：无显式类型
            parser_advance(parser);
            declared_type = NULL; // 后续通过类型推断
        } else {
            // 保存类型名（用于构造函数调用）
            // 如果当前是标识符，保存其名称
            if (parser->current.kind == CN_TOKEN_IDENT) {
                type_name = parser->current.lexeme_begin;
                type_name_length = parser->current.lexeme_length;
            }
            // 使用统一的 parse_type 解析类型
            declared_type = parse_type(parser);
            if (!declared_type) {
                return NULL;
            }
            // 如果类型是结构体类型，更新类型名
            if (declared_type && declared_type->kind == CN_TYPE_STRUCT) {
                type_name = declared_type->as.struct_type.name;
                type_name_length = declared_type->as.struct_type.name_length;
            }
        }

        // 检查函数指针声明：形如 整数(*回调)(整数, 整数)
        // 此时 declared_type 是返回类型, parser->current 应该是 LPAREN
        if (declared_type && parser->current.kind == CN_TOKEN_LPAREN) {
            parser_advance(parser); // 跳过 '('
            
            // 期望函数指针名前有 '*'
            if (parser->current.kind == CN_TOKEN_STAR) {
                parser_advance(parser); // 跳过 '*'
                
                // 读取函数指针变量名
                if (parser->current.kind != CN_TOKEN_IDENT) {
                    parser->error_count++;
                    if (parser->diagnostics) {
                        cn_support_diagnostics_report(parser->diagnostics,
                                                      CN_DIAG_SEVERITY_ERROR,
                                                      CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                                      parser->lexer ? parser->lexer->filename : NULL,
                                                      parser->current.line,
                                                      parser->current.column,
                                                      "语法错误：函数指针名称无效");
                    }
                    return NULL;
                }
                
                var_name = parser->current.lexeme_begin;
                var_name_length = parser->current.lexeme_length;
                parser_advance(parser);
                
                // 期望 ')'
                if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                    return NULL;
                }
                
                // 解析参数列表类型：'(' 类型1, 类型2, ... ')'
                if (!parser_expect(parser, CN_TOKEN_LPAREN)) {
                    return NULL;
                }
                
                // 解析函数参数类型列表
                size_t param_capacity = 4;
                size_t param_count = 0;
                CnType **param_types = (CnType **)malloc(sizeof(CnType*) * param_capacity);
                if (!param_types) {
                    return NULL;
                }
                
                // 解析参数类型，允许空参数列表
                if (parser->current.kind != CN_TOKEN_RPAREN) {
                    do {
                        // 使用统一的 parse_type 解析参数类型
                        CnType *param_type = parse_type(parser);
                        if (!param_type) {
                            parser->error_count++;
                            if (parser->diagnostics) {
                                cn_support_diagnostics_report(parser->diagnostics,
                                                              CN_DIAG_SEVERITY_ERROR,
                                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                              parser->lexer ? parser->lexer->filename : NULL,
                                                              parser->current.line,
                                                              parser->current.column,
                                                              "语法错误：函数指针参数类型无效");
                            }
                            free(param_types);
                            return NULL;
                        }
                        
                        // 参数名称是可选的，跳过如果存在
                        if (parser->current.kind == CN_TOKEN_IDENT) {
                            parser_advance(parser);
                        }
                        
                        // 扩容
                        if (param_count >= param_capacity) {
                            param_capacity *= 2;
                            CnType **new_param_types = (CnType **)realloc(
                                param_types, sizeof(CnType*) * param_capacity);
                            if (!new_param_types) {
                                free(param_types);
                                return NULL;
                            }
                            param_types = new_param_types;
                        }
                        
                        param_types[param_count++] = param_type;
                        
                        if (parser->current.kind == CN_TOKEN_COMMA) {
                            parser_advance(parser);
                        } else {
                            break;
                        }
                    } while (parser->current.kind != CN_TOKEN_RPAREN &&
                             parser->current.kind != CN_TOKEN_EOF);
                }
                
                if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                    free(param_types);
                    return NULL;
                }
                
                // 创建函数类型
                CnType *func_type = cn_type_new_function(declared_type, param_types, param_count);
                
                // 函数指针是指向函数类型的指针
                declared_type = cn_type_new_pointer(func_type);
                
                // 解析可选的初始化表达式
                if (parser->current.kind == CN_TOKEN_EQUAL) {
                    parser_advance(parser);
                    initializer = parse_expression(parser);
                }
                
                parser_expect(parser, CN_TOKEN_SEMICOLON);
                
                CnAstStmt *stmt = make_var_decl_stmt(var_name, var_name_length, declared_type, initializer, CN_VISIBILITY_DEFAULT);
                if (stmt) {
                    if (is_const) {
                        stmt->as.var_decl.is_const = 1;
                    }
                    if (is_static) {
                        stmt->as.var_decl.is_static = 1;
                    }
                }
                return stmt;
            }
            
            // 如果不是函数指针，恢复解析普通变量（这里简化处理，实际可能需要回溯）
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：期望函数指针声明或变量名");
            }
            return NULL;
        }

        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：变量名无效");
            }
            return NULL;
        }

       var_name = parser->current.lexeme_begin;
        var_name_length = parser->current.lexeme_length;
        parser_advance(parser);

        // C风格数组声明支持（包括多维数组）：类型 名称[大小1][大小2]... = {...}
        // 首先收集所有维度信息
        size_t dimension_capacity = 4;
        size_t dimension_count = 0;
        size_t *dimensions = NULL;
                
        if (parser->current.kind == CN_TOKEN_LBRACKET) {
            dimensions = (size_t *)malloc(sizeof(size_t) * dimension_capacity);
            if (!dimensions) {
                return NULL;
            }
                    
            while (parser->current.kind == CN_TOKEN_LBRACKET) {
                parser_advance(parser);  // 跳过 '['
                        
                size_t array_size = 0;
                        
                // 检查是否指定了数组大小
                if (parser->current.kind != CN_TOKEN_RBRACKET) {
                    // 解析数组大小表达式（这里简化为只接受整数字面量）
                    if (parser->current.kind == CN_TOKEN_INTEGER) {
                        array_size = (size_t)strtol(parser->current.lexeme_begin, NULL, 10);
                        parser_advance(parser);
                    } else {
                        parser->error_count++;
                        if (parser->diagnostics) {
                            cn_support_diagnostics_report(parser->diagnostics,
                                                          CN_DIAG_SEVERITY_ERROR,
                                                          CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                                          parser->lexer ? parser->lexer->filename : NULL,
                                                          parser->current.line,
                                                          parser->current.column,
                                                          "语法错误：数组大小必须是整数字面量");
                        }
                        free(dimensions);
                        return NULL;
                    }
                }
                        
                if (!parser_expect(parser, CN_TOKEN_RBRACKET)) {
                    free(dimensions);
                    return NULL;
                }
                        
                // 扩容检查
                if (dimension_count >= dimension_capacity) {
                    dimension_capacity *= 2;
                    size_t *new_dimensions = (size_t *)realloc(dimensions, sizeof(size_t) * dimension_capacity);
                    if (!new_dimensions) {
                        free(dimensions);
                        return NULL;
                    }
                    dimensions = new_dimensions;
                }
                        
                dimensions[dimension_count++] = array_size;
            }
                    
            // 从右向左构建数组类型
            // 例如：整数 arr[3][4] -> array(3, array(4, int))
            // dimensions = [3, 4]，从右向左：先array(4, int)，再array(3, ...)
            if (declared_type) {
                // 从最右边的维度开始
                for (int i = (int)dimension_count - 1; i >= 0; i--) {
                    declared_type = cn_type_new_array(declared_type, dimensions[i]);
                }
            } else {
                // 如果使用"变量"关键字，先创建默认整数类型
                declared_type = cn_type_new_primitive(CN_TYPE_INT);
                for (int i = (int)dimension_count - 1; i >= 0; i--) {
                    declared_type = cn_type_new_array(declared_type, dimensions[i]);
                }
            }
                    
            free(dimensions);
        }

        // 支持带参数构造函数调用语法：类型 变量名(参数列表);
        // 例如：学生 学生2("张三", 20, 85.5);
        if (parser->current.kind == CN_TOKEN_LPAREN) {
            parser_advance(parser);  // 跳过 '('

            // 解析参数列表
            CnAstExpr **args = NULL;
            size_t arg_count = 0;
            size_t arg_capacity = 0;

            if (parser->current.kind != CN_TOKEN_RPAREN) {
                arg_capacity = 4;
                args = (CnAstExpr **)malloc(sizeof(CnAstExpr *) * arg_capacity);
                if (!args) {
                    return NULL;
                }

                do {
                    if (arg_count >= arg_capacity) {
                        arg_capacity *= 2;
                        CnAstExpr **new_args = (CnAstExpr **)realloc(
                            args, sizeof(CnAstExpr *) * arg_capacity);
                        if (!new_args) {
                            for (size_t i = 0; i < arg_count; i++) {
                                cn_frontend_ast_expr_free(args[i]);
                            }
                            free(args);
                            return NULL;
                        }
                        args = new_args;
                    }

                    args[arg_count] = parse_expression(parser);
                    if (!args[arg_count]) {
                        for (size_t i = 0; i < arg_count; i++) {
                            cn_frontend_ast_expr_free(args[i]);
                        }
                        free(args);
                        return NULL;
                    }
                    arg_count++;

                    if (parser->current.kind == CN_TOKEN_COMMA) {
                        parser_advance(parser);
                    } else {
                        break;
                    }
                } while (parser->current.kind != CN_TOKEN_RPAREN &&
                         parser->current.kind != CN_TOKEN_EOF);
            }

            parser_expect(parser, CN_TOKEN_RPAREN);

            // 创建构造函数调用表达式作为初始化器
            // 构造函数名为类型名，例如：学生("张三", 20, 85.5)
            CnAstExpr *type_name_expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
            if (!type_name_expr) {
                for (size_t i = 0; i < arg_count; i++) {
                    cn_frontend_ast_expr_free(args[i]);
                }
                free(args);
                return NULL;
            }
            type_name_expr->kind = CN_AST_EXPR_IDENTIFIER;
            type_name_expr->type = NULL;
            type_name_expr->loc.filename = parser->lexer ? parser->lexer->filename : NULL;
            type_name_expr->loc.line = parser->current.line;
            type_name_expr->loc.column = parser->current.column;
            // 使用保存的类型名
            type_name_expr->as.identifier.name = type_name;
            type_name_expr->as.identifier.name_length = type_name_length;

            // 创建函数调用表达式
            initializer = make_call(type_name_expr, args, arg_count);
        }

        if (parser->current.kind == CN_TOKEN_EQUAL) {
            parser_advance(parser);
            initializer = parse_expression(parser);
        }

        parser_expect(parser, CN_TOKEN_SEMICOLON);

        CnAstStmt *stmt = make_var_decl_stmt(var_name, var_name_length, declared_type, initializer, CN_VISIBILITY_DEFAULT);
        if (stmt) {
            if (is_const) {
                stmt->as.var_decl.is_const = 1;
            }
            if (is_static) {
                stmt->as.var_decl.is_static = 1;
            }
        }
        return stmt;
    }

    expr = parse_expression(parser);
    if (!expr) {
        return NULL;
    }

    parser_expect(parser, CN_TOKEN_SEMICOLON);

    return make_expr_stmt(expr);
}

static CnAstExpr *parse_expression(CnParser *parser)
{
    return parse_assignment(parser);
}

static CnAstExpr *parse_assignment(CnParser *parser)
{
    CnAstExpr *expr = parse_ternary(parser);  // 三元运算符优先级低于赋值

    if (parser->current.kind == CN_TOKEN_EQUAL) {
        // 保存赋值符号的位置信息
        CnToken assign_token = parser->current;
        parser_advance(parser);
        CnAstExpr *value = parse_assignment(parser);  // 右结合
        CnAstExpr *assign_expr = make_assign(expr, value);
        if (assign_expr) {
            // 设置赋值表达式的位置信息为赋值符号的位置
            assign_expr->loc.filename = parser->lexer ? parser->lexer->filename : NULL;
            assign_expr->loc.line = assign_token.line;
            assign_expr->loc.column = assign_token.column;
        }
        return assign_expr;
    }

    return expr;
}

// 解析三元运算符表达式 (condition ? true_expr : false_expr)
// 三元运算符优先级介于赋值和逻辑或之间，且是右结合
static CnAstExpr *parse_ternary(CnParser *parser)
{
    CnAstExpr *condition = parse_logical_or(parser);

    if (parser->current.kind == CN_TOKEN_QUESTION) {
        CnToken question_token = parser->current;
        parser_advance(parser);
        
        CnAstExpr *true_expr = parse_expression(parser);  // 递归调用 parse_expression 以支持嵌套
        
        if (parser->current.kind != CN_TOKEN_COLON) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：三元运算符需要 \":\" 分隔真假分支");
            }
            return condition;
        }
        
        parser_advance(parser);  // 跳过 ':'
        
        CnAstExpr *false_expr = parse_ternary(parser);  // 右结合，递归调用 parse_ternary
        
        // 创建三元表达式节点
        CnAstExpr *ternary_expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
        if (!ternary_expr) {
            return condition;
        }
        
        ternary_expr->kind = CN_AST_EXPR_TERNARY;
        ternary_expr->type = NULL;
        ternary_expr->loc.filename = parser->lexer ? parser->lexer->filename : NULL;
        ternary_expr->loc.line = question_token.line;
        ternary_expr->loc.column = question_token.column;
        ternary_expr->as.ternary.condition = condition;
        ternary_expr->as.ternary.true_expr = true_expr;
        ternary_expr->as.ternary.false_expr = false_expr;
        
        return ternary_expr;
    }

    return condition;
}

static CnAstExpr *parse_logical_or(CnParser *parser)
{
    CnAstExpr *left = parse_logical_and(parser);

    while (parser->current.kind == CN_TOKEN_LOGICAL_OR) {
        parser_advance(parser);
        CnAstExpr *right = parse_logical_and(parser);
        left = make_logical(CN_AST_LOGICAL_OP_OR, left, right);
    }

    return left;
}

static CnAstExpr *parse_logical_and(CnParser *parser)
{
    CnAstExpr *left = parse_bitwise_or(parser);

    while (parser->current.kind == CN_TOKEN_LOGICAL_AND) {
        parser_advance(parser);
        CnAstExpr *right = parse_bitwise_or(parser);
        left = make_logical(CN_AST_LOGICAL_OP_AND, left, right);
    }

    return left;
}

// 按位或运算：优先级低于按位异或
static CnAstExpr *parse_bitwise_or(CnParser *parser)
{
    CnAstExpr *left = parse_bitwise_xor(parser);

    while (parser->current.kind == CN_TOKEN_BITWISE_OR) {
        parser_advance(parser);
        CnAstExpr *right = parse_bitwise_xor(parser);
        left = make_binary(CN_AST_BINARY_OP_BITWISE_OR, left, right);
    }

    return left;
}

// 按位异或运算：优先级低于按位与
static CnAstExpr *parse_bitwise_xor(CnParser *parser)
{
    CnAstExpr *left = parse_bitwise_and(parser);

    while (parser->current.kind == CN_TOKEN_BITWISE_XOR) {
        parser_advance(parser);
        CnAstExpr *right = parse_bitwise_and(parser);
        left = make_binary(CN_AST_BINARY_OP_BITWISE_XOR, left, right);
    }

    return left;
}

// 按位与运算：优先级低于比较运算
static CnAstExpr *parse_bitwise_and(CnParser *parser)
{
    CnAstExpr *left = parse_comparison(parser);

    while (parser->current.kind == CN_TOKEN_AMPERSAND ||
           parser->current.kind == CN_TOKEN_BITWISE_AND) {
        parser_advance(parser);
        CnAstExpr *right = parse_comparison(parser);
        left = make_binary(CN_AST_BINARY_OP_BITWISE_AND, left, right);
    }

    return left;
}

static CnAstExpr *parse_shift(CnParser *parser)
{
    CnAstExpr *left = parse_additive(parser);

    while (parser->current.kind == CN_TOKEN_LEFT_SHIFT ||
           parser->current.kind == CN_TOKEN_RIGHT_SHIFT) {
        CnAstBinaryOp op = (parser->current.kind == CN_TOKEN_LEFT_SHIFT)
                               ? CN_AST_BINARY_OP_LEFT_SHIFT
                               : CN_AST_BINARY_OP_RIGHT_SHIFT;
        parser_advance(parser);
        left = make_binary(op, left, parse_additive(parser));
    }

    return left;
}

static CnAstExpr *parse_comparison(CnParser *parser)
{
    CnAstExpr *left = parse_shift(parser);

    while (parser->current.kind == CN_TOKEN_EQUAL_EQUAL ||
           parser->current.kind == CN_TOKEN_BANG_EQUAL ||
           parser->current.kind == CN_TOKEN_LESS ||
           parser->current.kind == CN_TOKEN_GREATER ||
           parser->current.kind == CN_TOKEN_LESS_EQUAL ||
           parser->current.kind == CN_TOKEN_GREATER_EQUAL) {
        CnAstBinaryOp op;

        switch (parser->current.kind) {
        case CN_TOKEN_EQUAL_EQUAL:
            op = CN_AST_BINARY_OP_EQ;
            break;
        case CN_TOKEN_BANG_EQUAL:
            op = CN_AST_BINARY_OP_NE;
            break;
        case CN_TOKEN_LESS:
            op = CN_AST_BINARY_OP_LT;
            break;
        case CN_TOKEN_GREATER:
            op = CN_AST_BINARY_OP_GT;
            break;
        case CN_TOKEN_LESS_EQUAL:
            op = CN_AST_BINARY_OP_LE;
            break;
        case CN_TOKEN_GREATER_EQUAL:
            op = CN_AST_BINARY_OP_GE;
            break;
        default:
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_COMPARISON_OP,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：比较运算符无效");
            }
            return left;
        }

        parser_advance(parser);
        left = make_binary(op, left, parse_additive(parser));
    }

    return left;
}

static CnAstExpr *parse_additive(CnParser *parser)
{
    CnAstExpr *left = parse_term(parser);

    while (parser->current.kind == CN_TOKEN_PLUS ||
           parser->current.kind == CN_TOKEN_MINUS) {
        CnAstBinaryOp op = (parser->current.kind == CN_TOKEN_PLUS)
                               ? CN_AST_BINARY_OP_ADD
                               : CN_AST_BINARY_OP_SUB;
        parser_advance(parser);
        left = make_binary(op, left, parse_term(parser));
    }

    return left;
}

static CnAstExpr *parse_term(CnParser *parser)
{
    CnAstExpr *left = parse_unary(parser);

    while (parser->current.kind == CN_TOKEN_STAR ||
           parser->current.kind == CN_TOKEN_SLASH ||
           parser->current.kind == CN_TOKEN_PERCENT) {
        CnAstBinaryOp op;
        if (parser->current.kind == CN_TOKEN_STAR) {
            op = CN_AST_BINARY_OP_MUL;
        } else if (parser->current.kind == CN_TOKEN_SLASH) {
            op = CN_AST_BINARY_OP_DIV;
        } else {
            op = CN_AST_BINARY_OP_MOD;
        }
        parser_advance(parser);
        left = make_binary(op, left, parse_unary(parser));
    }

    return left;
}

static CnAstExpr *parse_unary(CnParser *parser)
{
    // 处理前置自增运算符 ++
    if (parser->current.kind == CN_TOKEN_PLUS_PLUS) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);
        return make_unary(CN_AST_UNARY_OP_PRE_INC, operand);
    }

    // 处理前置自减运算符 --
    if (parser->current.kind == CN_TOKEN_MINUS_MINUS) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);
        return make_unary(CN_AST_UNARY_OP_PRE_DEC, operand);
    }

    // 处理取地址运算符 &
    if (parser->current.kind == CN_TOKEN_AMPERSAND) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);  // 递归支持连续 &
        return make_unary(CN_AST_UNARY_OP_ADDRESS_OF, operand);
    }

    // 处理解引用运算符 *
    if (parser->current.kind == CN_TOKEN_STAR) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);  // 递归支持连续 *
        return make_unary(CN_AST_UNARY_OP_DEREFERENCE, operand);
    }

    // 处理逻辑非运算符 !
    if (parser->current.kind == CN_TOKEN_BANG) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);  // 递归处理多个 !
        return make_unary(CN_AST_UNARY_OP_NOT, operand);
    }

    // 处理按位取反运算符 ~
    if (parser->current.kind == CN_TOKEN_BITWISE_NOT) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);  // 递归处理多个 ~
        return make_unary(CN_AST_UNARY_OP_BITWISE_NOT, operand);
    }

    // 处理一元负号 -
    if (parser->current.kind == CN_TOKEN_MINUS) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);  // 递归处理多个 -
        return make_unary(CN_AST_UNARY_OP_MINUS, operand);
    }

    return parse_postfix(parser);  // 支持后缀表达式（如函数调用、数组索引）
}

// 解析后缀表达式（如函数调用、数组索引、成员访问、后置自增/自减）
static CnAstExpr *parse_postfix(CnParser *parser)
{
    CnAstExpr *expr = parse_factor(parser);

    while (parser->current.kind == CN_TOKEN_LPAREN || 
           parser->current.kind == CN_TOKEN_LBRACKET ||
           parser->current.kind == CN_TOKEN_DOT ||
           parser->current.kind == CN_TOKEN_ARROW ||
           parser->current.kind == CN_TOKEN_PLUS_PLUS ||
           parser->current.kind == CN_TOKEN_MINUS_MINUS) {
        if (parser->current.kind == CN_TOKEN_LPAREN) {
            // 函数调用
            parser_advance(parser);

            CnAstExpr **args = NULL;
            size_t arg_count = 0;
            size_t arg_capacity = 0;

            if (parser->current.kind != CN_TOKEN_RPAREN) {
                arg_capacity = 4;
                args = (CnAstExpr **)malloc(sizeof(CnAstExpr *) * arg_capacity);
                if (!args) {
                    cn_frontend_ast_expr_free(expr);
                    return NULL;
                }

                do {
                    if (arg_count >= arg_capacity) {
                        arg_capacity *= 2;
                        CnAstExpr **new_args = (CnAstExpr **)realloc(
                            args, sizeof(CnAstExpr *) * arg_capacity);
                        if (!new_args) {
                            for (size_t i = 0; i < arg_count; i++) {
                                cn_frontend_ast_expr_free(args[i]);
                            }
                            free(args);
                            cn_frontend_ast_expr_free(expr);
                            return NULL;
                        }
                        args = new_args;
                    }

                    args[arg_count] = parse_expression(parser);
                    if (!args[arg_count]) {
                        for (size_t i = 0; i < arg_count; i++) {
                            cn_frontend_ast_expr_free(args[i]);
                        }
                        free(args);
                        cn_frontend_ast_expr_free(expr);
                        return NULL;
                    }
                    arg_count++;

                    if (parser->current.kind == CN_TOKEN_COMMA) {
                        parser_advance(parser);
                    } else {
                        break;
                    }
                } while (parser->current.kind != CN_TOKEN_RPAREN &&
                         parser->current.kind != CN_TOKEN_EOF);
            }

            parser_expect(parser, CN_TOKEN_RPAREN);

            expr = make_call(expr, args, arg_count);
        } else if (parser->current.kind == CN_TOKEN_LBRACKET) {
            // 数组索引访问 arr[index]
            parser_advance(parser);  // 跳过 [
            
            CnAstExpr *index_expr = parse_expression(parser);
            if (!index_expr) {
                cn_frontend_ast_expr_free(expr);
                return NULL;
            }
            
            parser_expect(parser, CN_TOKEN_RBRACKET);  // 期望 ]
            
            expr = make_index(expr, index_expr);
        } else if (parser->current.kind == CN_TOKEN_DOT) {
            // 结构体成员访问 obj.member
            parser_advance(parser);  // 跳过 .
            
            if (parser->current.kind != CN_TOKEN_IDENT) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少成员名称");
                }
                cn_frontend_ast_expr_free(expr);
                return NULL;
            }
            
            const char *member_name = parser->current.lexeme_begin;
            size_t member_name_length = parser->current.lexeme_length;
            parser_advance(parser);
            
            expr = make_member_access(expr, member_name, member_name_length, 0);
        } else if (parser->current.kind == CN_TOKEN_ARROW) {
            // 结构体指针成员访问 ptr->member
            parser_advance(parser);  // 跳过 ->
            
            if (parser->current.kind != CN_TOKEN_IDENT) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少成员名称");
                }
                cn_frontend_ast_expr_free(expr);
                return NULL;
            }
            
            const char *member_name = parser->current.lexeme_begin;
            size_t member_name_length = parser->current.lexeme_length;
            parser_advance(parser);
            
            expr = make_member_access(expr, member_name, member_name_length, 1);
        } else if (parser->current.kind == CN_TOKEN_PLUS_PLUS) {
            // 后置自增 i++
            parser_advance(parser);
            expr = make_unary(CN_AST_UNARY_OP_POST_INC, expr);
        } else if (parser->current.kind == CN_TOKEN_MINUS_MINUS) {
            // 后置自减 i--
            parser_advance(parser);
            expr = make_unary(CN_AST_UNARY_OP_POST_DEC, expr);
        }
    }

    return expr;
}

// 统一的类型解析函数，支持所有类型组合
// 解析：整数/小数/布尔/字符串/数组/结构体/枚举/指针等类型
static CnType *parse_type(CnParser *parser)
{
    CnType *type = NULL;

    if (!parser->has_current) {
        parser_advance(parser);
    }

    // 解析基础类型
    if (parser->current.kind == CN_TOKEN_KEYWORD_INT) {
        type = cn_type_new_primitive(CN_TYPE_INT);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_FLOAT) {
        type = cn_type_new_primitive(CN_TYPE_FLOAT);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_BOOL) {
        type = cn_type_new_primitive(CN_TYPE_BOOL);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRING) {
        type = cn_type_new_primitive(CN_TYPE_STRING);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_VOID) {
        type = cn_type_new_primitive(CN_TYPE_VOID);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_SELF_TYPE) {
        // 自身类型：表示当前类/接口的类型
        type = cn_type_new_primitive(CN_TYPE_SELF);
        parser_advance(parser);
    /* 已禁用：CN_TOKEN_KEYWORD_MEMORY_ADDRESS 已删除
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_MEMORY_ADDRESS) {
        // 注意：CN_TOKEN_KEYWORD_MEMORY_ADDRESS 已删除
        type = cn_type_new_memory_address();
        parser_advance(parser);
    */
    // 注意：CN_TOKEN_KEYWORD_ARRAY 已删除
    // 数组类型现在使用 "类型[大小]" 或 "类型[]" 语法，在后续指针解析后处理
    } else if (parser->current.kind == CN_TOKEN_IDENT) {
        // 可能是结构体类型或枚举类型（自定义类型）
        // 目前简化处理：将标识符视为结构体类型名
        // TODO: 未来可以通过符号表区分结构体和枚举
        const char *type_name = parser->current.lexeme_begin;
        size_t type_name_length = parser->current.lexeme_length;
        parser_advance(parser);

        // 创建结构体类型（字段列表留空，后续语义分析填充）
        // 注意：解析阶段还不知道作用域，传入NULL
        type = cn_type_new_struct(type_name, type_name_length, NULL, 0, NULL, NULL, 0);
    } else {
        // 无法识别的类型
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：期望类型名称");
        }
        return NULL;
    }

    // 解析指针层级：支持多级指针，例如 "整数*" 或 "整数**"
    while (parser->current.kind == CN_TOKEN_STAR) {
        type = cn_type_new_pointer(type);
        parser_advance(parser);
    }

    // 注意：函数指针类型的解析由变量声明逻辑、结构体字段解析逻辑和函数参数解析逻辑处理
    // 这里不解析函数指针类型，因为回退 lexer 状态太复杂

    return type;
}

static CnAstExpr *parse_struct_literal_with_name(CnParser *parser, const char *struct_name, size_t struct_name_length)
{
    // 当前 token 应该是 '{'
    if (parser->current.kind != CN_TOKEN_LBRACE) {
        return NULL;
    }

    parser_advance(parser);  // 跳过 '{'

    size_t field_capacity = 4;
    size_t field_count = 0;
    CnAstStructFieldInit *fields = (CnAstStructFieldInit *)malloc(
        sizeof(CnAstStructFieldInit) * field_capacity);
    if (!fields) {
        return NULL;
    }

    // 处理空结构体 {}
    if (parser->current.kind != CN_TOKEN_RBRACE) {
        do {
            // 容量检查与扩容
            if (field_count >= field_capacity) {
                field_capacity *= 2;
                CnAstStructFieldInit *new_fields = (CnAstStructFieldInit *)realloc(
                    fields, sizeof(CnAstStructFieldInit) * field_capacity);
                if (!new_fields) {
                    free(fields);
                    return NULL;
                }
                fields = new_fields;
            }

            // 初始化当前字段条目
            fields[field_count].field_name = NULL;
            fields[field_count].field_name_length = 0;
            fields[field_count].value = NULL;

            if (parser->current.kind == CN_TOKEN_IDENT) {
                // 可能是“指定成员初始化”或“位置初始化的标识符”
                const char *name = parser->current.lexeme_begin;
                size_t name_length = parser->current.lexeme_length;
                parser_advance(parser);

                if (parser->current.kind == CN_TOKEN_COLON ||
                    parser->current.kind == CN_TOKEN_EQUAL) {
                    // 指定成员初始化：字段名[:|=] 值
                    parser_advance(parser); // 跳过 ':' 或 '='
                    CnAstExpr *value = parse_expression(parser);
                    if (!value) {
                        free(fields);
                        return NULL;
                    }
                    fields[field_count].field_name = name;
                    fields[field_count].field_name_length = name_length;
                    fields[field_count].value = value;
                } else {
                    // 位置初始化：标识符本身作为表达式
                    fields[field_count].value = make_identifier(name, name_length);
                }
            } else {
                // 其他情况统一按“表达式”解析，作为位置初始化
                CnAstExpr *value = parse_expression(parser);
                if (!value) {
                    free(fields);
                    return NULL;
                }
                fields[field_count].value = value;
            }

            field_count++;

            // 处理逗号分隔符
            if (parser->current.kind == CN_TOKEN_COMMA) {
                parser_advance(parser);
            } else {
                break;
            }
        } while (parser->current.kind != CN_TOKEN_RBRACE &&
                 parser->current.kind != CN_TOKEN_EOF);
    }

    // 期望右花括号
    if (!parser_expect(parser, CN_TOKEN_RBRACE)) {
        for (size_t i = 0; i < field_count; i++) {
            if (fields[i].value) {
                cn_frontend_ast_expr_free(fields[i].value);
            }
        }
        free(fields);
        return NULL;
    }

    return make_struct_literal(struct_name, struct_name_length, fields, field_count);
}

static CnAstExpr *parse_factor(CnParser *parser)
{
    CnAstExpr *expr = NULL;

    if (!parser->has_current) {
        parser_advance(parser);
    }

    if (parser->current.kind == CN_TOKEN_INTEGER) {
        // 自动检测进制：十六进制(0x/0X)、二进制(0b/0B)、八进制(0o/0O)、十进制
        long value;
        const char *str = parser->current.lexeme_begin;
        
        if (str[0] == '0' && parser->current.lexeme_length > 2) {
            char prefix = str[1];
            if (prefix == 'x' || prefix == 'X') {
                // 十六进制
                value = strtol(str, NULL, 16);
            } else if (prefix == 'b' || prefix == 'B') {
                // 二进制：需要手动解析（strtol不支持二进制）
                value = 0;
                for (size_t i = 2; i < parser->current.lexeme_length; i++) {
                    value = value * 2 + (str[i] - '0');
                }
            } else if (prefix == 'o' || prefix == 'O') {
                // 八进制
                value = strtol(str + 2, NULL, 8);
            } else {
                // 其他以0开头的数字，使用自动检测
                value = strtol(str, NULL, 0);
            }
        } else {
            // 普通十进制数字
            value = strtol(str, NULL, 10);
        }
        
        expr = make_integer_literal(value);
        
        // 根据后缀设置类型
        if (parser->current.number_suffix == 0) {
            // 无后缀，默认为 CN_TYPE_INT
            expr->type = cn_type_new_primitive(CN_TYPE_INT);
        } else if (parser->current.number_suffix == 2) {
            // L 后缀 -> int32
            expr->type = cn_type_new_primitive(CN_TYPE_INT32);
        } else if (parser->current.number_suffix == 3) {
            // LL 后缀 -> int64
            expr->type = cn_type_new_primitive(CN_TYPE_INT64);
        } else if (parser->current.number_suffix == 4) {
            // U 后缀 -> uint32
            expr->type = cn_type_new_primitive(CN_TYPE_UINT32);
        } else if (parser->current.number_suffix == 5) {
            // UL 后缀 -> uint64
            expr->type = cn_type_new_primitive(CN_TYPE_UINT64);
        } else if (parser->current.number_suffix == 6) {
            // ULL 后缀 -> uint64
            expr->type = cn_type_new_primitive(CN_TYPE_UINT64_LL);
        }
        
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_FLOAT_LITERAL) {
        double value = strtod(parser->current.lexeme_begin, NULL);
        expr = make_float_literal(value);
        
        // 根据后缀设置类型
        if (parser->current.number_suffix == 1) {
            // f/F 后缀 -> float32
            expr->type = cn_type_new_primitive(CN_TYPE_FLOAT32);
        } else {
            // 无后缀或其他，默认为 CN_TYPE_FLOAT (double)
            expr->type = cn_type_new_primitive(CN_TYPE_FLOAT);
        }
        
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_STRING_LITERAL) {
        // 处理字符串转义序列
        size_t processed_length = 0;
        char *processed_string = process_string_escapes(parser->current.lexeme_begin, 
                                                         parser->current.lexeme_length,
                                                         &processed_length);
        if (processed_string) {
            expr = make_string_literal(processed_string, processed_length);
        } else {
            // 处理失败，使用原始字符串
            expr = make_string_literal(parser->current.lexeme_begin, parser->current.lexeme_length);
        }
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_TRUE) {
        expr = make_bool_literal(1);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_FALSE) {
        expr = make_bool_literal(0);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_NULL) {
        // NULL 关键字：生成空指针字面量
        expr = make_integer_literal(0);  // 简化处理：将NULL表示为0
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_THIS) {
        // 自身 关键字：生成标识符表达式 "self"
        // 在代码生成阶段会转换为 C 代码中的 self 参数
        fprintf(stderr, "[DEBUG PARSER] Found CN_TOKEN_KEYWORD_THIS, creating self identifier\n");
        expr = make_identifier("self", 4);
        expr->is_this_pointer = 1;  // 标记为自身指针，用于语义检查
        fprintf(stderr, "[DEBUG PARSER] expr=%p, is_this_pointer=%d\n", (void*)expr, expr->is_this_pointer);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_BASE) {
        // 基类 关键字：生成标识符表达式 "base"
        // 在代码生成阶段会转换为对父类方法的调用
        expr = make_identifier("base", 4);
        expr->is_base_pointer = 1;  // 标记为基类指针，用于语义检查和代码生成
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_IDENT) {
        // 保存标识符信息
        const char *ident_name = parser->current.lexeme_begin;
        size_t ident_name_length = parser->current.lexeme_length;
        parser_advance(parser);
        
        // 检查是否是模板实例化：标识符 < ... >
        // 注意：需要区分模板实例化和比较表达式
        if (parser->current.kind == CN_TOKEN_LESS) {
            // 前瞻判断：检查 < 后面是否是类型名
            // 如果不是类型名，则一定是比较表达式，不需要尝试模板解析
            // 需要保存词法分析器的完整状态，因为 parser_advance 会修改它
            CnToken saved_token = parser->current;
            int saved_has_current = parser->has_current;
            CnLexer saved_lexer_state = *parser->lexer;  // 保存词法分析器状态

            parser_advance(parser);  // 临时消耗 '<' 进行前瞻

            bool is_template = is_type_start(parser);

            // 恢复状态（包括词法分析器状态）
            parser->current = saved_token;
            parser->has_current = saved_has_current;
            *parser->lexer = saved_lexer_state;  // 恢复词法分析器状态

            if (is_template) {
                // 可能是模板实例化，尝试解析
                expr = parse_template_instantiation(parser, ident_name, ident_name_length);
                if (!expr) {
                    // 模板实例化解析失败，回退并创建普通标识符
                    parser->current = saved_token;
                    parser->has_current = saved_has_current;
                    *parser->lexer = saved_lexer_state;
                    expr = make_identifier(ident_name, ident_name_length);
                }
            } else {
                // 不是模板实例化（< 后面不是类型名），创建普通标识符
                // 让后续的比较表达式解析器处理 '<' 运算符
                expr = make_identifier(ident_name, ident_name_length);
            }
        } else if (parser->current.kind == CN_TOKEN_LBRACE) {
            // 检查是否是结构体字面量：标识符 { ... }
            expr = parse_struct_literal_with_name(parser, ident_name, ident_name_length);
        } else {
            // 普通标识符
            expr = make_identifier(ident_name, ident_name_length);
        }
    /* 注意:CN_TOKEN_KEYWORD_READ/WRITE/COPY/SET/MAP/UNMAP_MEMORY 已删除
     * 内存操作功能将通过运行时库函数提供
     * 原语法: 读取内存(地址)  写入内存(地址,值) 等
     * 新方式: 使用运行时API,如 cn_rt_mem_read(), cn_rt_mem_write() 等
     */
    /* 已禁用的内存操作解析
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_READ_MEMORY ||
               parser->current.kind == CN_TOKEN_KEYWORD_WRITE_MEMORY ||
               parser->current.kind == CN_TOKEN_KEYWORD_MEMORY_COPY ||
               parser->current.kind == CN_TOKEN_KEYWORD_MEMORY_SET ||
               parser->current.kind == CN_TOKEN_KEYWORD_MAP_MEMORY ||
               parser->current.kind == CN_TOKEN_KEYWORD_UNMAP_MEMORY) {
        // 解析内存访问内置函数
        CnTokenKind kind = parser->current.kind;
        parser_advance(parser);
        
        if (!parser_expect(parser, CN_TOKEN_LPAREN)) {
            return NULL;
        }
        
        CnAstExpr *result = NULL;
        
        if (kind == CN_TOKEN_KEYWORD_READ_MEMORY) {
            // 读取内存: 读取内存(地址)
            CnAstExpr *address = parse_expression(parser);
            if (!address) {
                return NULL;
            }
            result = make_memory_read(address);
        } else if (kind == CN_TOKEN_KEYWORD_WRITE_MEMORY) {
            // 写入内存: 写入内存(地址, 值)
            CnAstExpr *address = parse_expression(parser);
            if (!address) {
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(address);
                return NULL;
            }
            CnAstExpr *value = parse_expression(parser);
            if (!value) {
                cn_frontend_ast_expr_free(address);
                return NULL;
            }
            result = make_memory_write(address, value);
        } else if (kind == CN_TOKEN_KEYWORD_MEMORY_COPY) {
            // 内存复制: 内存复制(目标, 源, 大小)
            CnAstExpr *dest = parse_expression(parser);
            if (!dest) {
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(dest);
                return NULL;
            }
            CnAstExpr *src = parse_expression(parser);
            if (!src) {
                cn_frontend_ast_expr_free(dest);
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(dest);
                cn_frontend_ast_expr_free(src);
                return NULL;
            }
            CnAstExpr *size = parse_expression(parser);
            if (!size) {
                cn_frontend_ast_expr_free(dest);
                cn_frontend_ast_expr_free(src);
                return NULL;
            }
            result = make_memory_copy(dest, src, size);
        } else if (kind == CN_TOKEN_KEYWORD_MEMORY_SET) {
            // 内存设置: 内存设置(地址, 值, 大小)
            CnAstExpr *address = parse_expression(parser);
            if (!address) {
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(address);
                return NULL;
            }
            CnAstExpr *value = parse_expression(parser);
            if (!value) {
                cn_frontend_ast_expr_free(address);
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(address);
                cn_frontend_ast_expr_free(value);
                return NULL;
            }
            CnAstExpr *size = parse_expression(parser);
            if (!size) {
                cn_frontend_ast_expr_free(address);
                cn_frontend_ast_expr_free(value);
                return NULL;
            }
            result = make_memory_set(address, value, size);
        } else if (kind == CN_TOKEN_KEYWORD_MAP_MEMORY) {
            // 内存映射: 映射内存(地址, 大小, 保护, 标志)
            CnAstExpr *address = parse_expression(parser);
            if (!address) {
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(address);
                return NULL;
            }
            CnAstExpr *size = parse_expression(parser);
            if (!size) {
                cn_frontend_ast_expr_free(address);
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(address);
                cn_frontend_ast_expr_free(size);
                return NULL;
            }
            CnAstExpr *prot = parse_expression(parser);
            if (!prot) {
                cn_frontend_ast_expr_free(address);
                cn_frontend_ast_expr_free(size);
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(address);
                cn_frontend_ast_expr_free(size);
                cn_frontend_ast_expr_free(prot);
                return NULL;
            }
            CnAstExpr *flags = parse_expression(parser);
            if (!flags) {
                cn_frontend_ast_expr_free(address);
                cn_frontend_ast_expr_free(size);
                cn_frontend_ast_expr_free(prot);
                return NULL;
            }
            result = make_memory_map(address, size, prot, flags);
        } else if (kind == CN_TOKEN_KEYWORD_UNMAP_MEMORY) {
            // 解除内存映射: 解除映射(地址, 大小)
            CnAstExpr *address = parse_expression(parser);
            if (!address) {
                return NULL;
            }
            if (!parser_expect(parser, CN_TOKEN_COMMA)) {
                cn_frontend_ast_expr_free(address);
                return NULL;
            }
            CnAstExpr *size = parse_expression(parser);
            if (!size) {
                cn_frontend_ast_expr_free(address);
                return NULL;
            }
            result = make_memory_unmap(address, size);
        }
        
        if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
            if (result) {
                cn_frontend_ast_expr_free(result);
            }
            return NULL;
        }
        
        expr = result;
    */  // 结束已禁用的内存操作解析
    /* 注意:CN_TOKEN_KEYWORD_INLINE_ASM 已删除
     * 内联汇编功能将通过运行时库函数提供
     * 原语法: 内联汇编("asm code", outputs, inputs, clobbers)
     * 新方式: 使用运行时API,如 cn_rt_inline_asm()
     */
    /* 已禁用的内联汇编解析
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_INLINE_ASM) {
        // 解析内联汇编: 内联汇编("asm code", outputs, inputs, clobbers)
        parser_advance(parser);
        
        if (!parser_expect(parser, CN_TOKEN_LPAREN)) {
            return NULL;
        }
        
        // 解析汇编代码字符串
        CnAstExpr *asm_code = parse_expression(parser);
        if (!asm_code) {
            return NULL;
        }
        
        // 解析输出列表（可选）
        CnAstExpr **outputs = NULL;
        size_t output_count = 0;
        
        if (parser->current.kind == CN_TOKEN_COMMA) {
            parser_advance(parser);
            
            // 解析输出变量列表
            size_t out_capacity = 4;
            outputs = (CnAstExpr **)malloc(sizeof(CnAstExpr *) * out_capacity);
            if (!outputs) {
                cn_frontend_ast_expr_free(asm_code);
                return NULL;
            }
            
            if (parser->current.kind != CN_TOKEN_RPAREN) {
                do {
                    if (output_count >= out_capacity) {
                        out_capacity *= 2;
                        CnAstExpr **new_outputs = (CnAstExpr **)realloc(
                            outputs, sizeof(CnAstExpr *) * out_capacity);
                        if (!new_outputs) {
                            free(outputs);
                            cn_frontend_ast_expr_free(asm_code);
                            return NULL;
                        }
                        outputs = new_outputs;
                    }
                    
                    outputs[output_count++] = parse_expression(parser);
                    
                    if (parser->current.kind == CN_TOKEN_COMMA) {
                        parser_advance(parser);
                    } else {
                        break;
                    }
                } while (parser->current.kind != CN_TOKEN_RPAREN &&
                         parser->current.kind != CN_TOKEN_EOF);
            }
        }
        
        // 解析输入列表（可选）
        CnAstExpr **inputs = NULL;
        size_t input_count = 0;
        
        if (parser->current.kind == CN_TOKEN_COMMA) {
            parser_advance(parser);
            
            // 解析输入变量列表
            size_t in_capacity = 4;
            inputs = (CnAstExpr **)malloc(sizeof(CnAstExpr *) * in_capacity);
            if (!inputs) {
                for (size_t i = 0; i < output_count; i++) {
                    cn_frontend_ast_expr_free(outputs[i]);
                }
                free(outputs);
                cn_frontend_ast_expr_free(asm_code);
                return NULL;
            }
            
            if (parser->current.kind != CN_TOKEN_RPAREN) {
                do {
                    if (input_count >= in_capacity) {
                        in_capacity *= 2;
                        CnAstExpr **new_inputs = (CnAstExpr **)realloc(
                            inputs, sizeof(CnAstExpr *) * in_capacity);
                        if (!new_inputs) {
                            free(inputs);
                            for (size_t i = 0; i < output_count; i++) {
                                cn_frontend_ast_expr_free(outputs[i]);
                            }
                            free(outputs);
                            cn_frontend_ast_expr_free(asm_code);
                            return NULL;
                        }
                        inputs = new_inputs;
                    }
                    
                    inputs[input_count++] = parse_expression(parser);
                    
                    if (parser->current.kind == CN_TOKEN_COMMA) {
                        parser_advance(parser);
                    } else {
                        break;
                    }
                } while (parser->current.kind != CN_TOKEN_RPAREN &&
                         parser->current.kind != CN_TOKEN_EOF);
            }
        }
        
        // 解析破坏列表（可选）
        CnAstExpr *clobbers = NULL;
        
        if (parser->current.kind == CN_TOKEN_COMMA) {
            parser_advance(parser);
            clobbers = parse_expression(parser);
        }
        
        if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
            for (size_t i = 0; i < input_count; i++) {
                cn_frontend_ast_expr_free(inputs[i]);
            }
            free(inputs);
            for (size_t i = 0; i < output_count; i++) {
                cn_frontend_ast_expr_free(outputs[i]);
            }
            free(outputs);
            cn_frontend_ast_expr_free(asm_code);
            if (clobbers) {
                cn_frontend_ast_expr_free(clobbers);
            }
            return NULL;
        }
        
        expr = make_inline_asm(asm_code, outputs, output_count, inputs, input_count, clobbers);
    */  // 结束已禁用的内联汇编解析
    } else if (parser->current.kind == CN_TOKEN_LPAREN) {
        parser_advance(parser);
        expr = parse_expression(parser);
        parser_expect(parser, CN_TOKEN_RPAREN);
    } else if (parser->current.kind == CN_TOKEN_LBRACKET) {
        // 解析数组字面量 [1, 2, 3]
        parser_advance(parser);  // 跳过 [
        
        // 动态分配元素数组
        size_t elem_capacity = 8;
        size_t elem_count = 0;
        CnAstExpr **elements = (CnAstExpr **)malloc(sizeof(CnAstExpr *) * elem_capacity);
        if (!elements) {
            return NULL;
        }
        
        // 处理空数组 []
        if (parser->current.kind != CN_TOKEN_RBRACKET) {
            do {
                // 如果容量不够，扩容
                if (elem_count >= elem_capacity) {
                    elem_capacity *= 2;
                    CnAstExpr **new_elements = (CnAstExpr **)realloc(
                        elements, sizeof(CnAstExpr *) * elem_capacity);
                    if (!new_elements) {
                        free(elements);
                        return NULL;
                    }
                    elements = new_elements;
                }
                
                // 解析元素表达式
                elements[elem_count++] = parse_expression(parser);
                
                // 如果下一个是逗号，跳过它
                if (parser->current.kind == CN_TOKEN_COMMA) {
                    parser_advance(parser);
                } else {
                    break;
                }
            } while (parser->current.kind != CN_TOKEN_RBRACKET &&
                     parser->current.kind != CN_TOKEN_EOF);
        }
        
        parser_expect(parser, CN_TOKEN_RBRACKET);
        expr = make_array_literal(elements, elem_count);
    } else if (parser->current.kind == CN_TOKEN_LBRACE) {
        // C风格数组初始化列表 {1, 2, 3}
        parser_advance(parser);  // 跳过 {
        
        // 动态分配元素数组
        size_t elem_capacity = 8;
        size_t elem_count = 0;
        CnAstExpr **elements = (CnAstExpr **)malloc(sizeof(CnAstExpr *) * elem_capacity);
        if (!elements) {
            return NULL;
        }
        
        // 处理空数组 {}
        if (parser->current.kind != CN_TOKEN_RBRACE) {
            do {
                // 如果容量不够，扩容
                if (elem_count >= elem_capacity) {
                    elem_capacity *= 2;
                    CnAstExpr **new_elements = (CnAstExpr **)realloc(
                        elements, sizeof(CnAstExpr *) * elem_capacity);
                    if (!new_elements) {
                        free(elements);
                        return NULL;
                    }
                    elements = new_elements;
                }
                
                // 解析元素表达式
                elements[elem_count++] = parse_expression(parser);
                
                // 如果下一个是逗号，跳过它
                if (parser->current.kind == CN_TOKEN_COMMA) {
                    parser_advance(parser);
                } else {
                    break;
                }
            } while (parser->current.kind != CN_TOKEN_RBRACE &&
                     parser->current.kind != CN_TOKEN_EOF);
        }
        
        parser_expect(parser, CN_TOKEN_RBRACE);
        expr = make_array_literal(elements, elem_count);
    } else {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_EXPR,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：无法解析表达式");
        }
        parser_advance(parser);
    }

    return expr;
}

static CnAstExpr *make_integer_literal(long value)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_INTEGER_LITERAL;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.integer_literal.value = value;
    return expr;
}

static CnAstExpr *make_float_literal(double value)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_FLOAT_LITERAL;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.float_literal.value = value;
    return expr;
}

// 处理字符串转义序列
static char* process_string_escapes(const char *raw_string, size_t raw_length, size_t *out_length)
{
    // 跳过开头和结尾的引号
    if (raw_length < 2 || raw_string[0] != '"' || raw_string[raw_length - 1] != '"') {
        *out_length = 0;
        return NULL;
    }
    
    // 分配足够的空间（最坏情况：没有转义，长度相同）
    char *result = (char *)malloc(raw_length);
    if (!result) {
        *out_length = 0;
        return NULL;
    }
    
    size_t write_pos = 0;
    // 从第1个字符开始（跳过开头的"），到倒数第2个字符结束（跳过结尾的"）
    for (size_t i = 1; i < raw_length - 1; i++) {
        if (raw_string[i] == '\\' && i + 1 < raw_length - 1) {
            // 处理转义序列
            char next = raw_string[i + 1];
            switch (next) {
                case 'n':  result[write_pos++] = '\n'; i++; break;
                case 'r':  result[write_pos++] = '\r'; i++; break;
                case 't':  result[write_pos++] = '\t'; i++; break;
                case '\\': result[write_pos++] = '\\'; i++; break;
                case '"':  result[write_pos++] = '"'; i++; break;
                case '0':  result[write_pos++] = '\0'; i++; break;
                default:
                    // 未知转义序列，保留反斜杠
                    result[write_pos++] = '\\';
                    break;
            }
        } else {
            result[write_pos++] = raw_string[i];
        }
    }
    
    result[write_pos] = '\0';
    *out_length = write_pos;
    return result;
}

static CnAstExpr *make_string_literal(const char *value, size_t length)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_STRING_LITERAL;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.string_literal.value = value;
    expr->as.string_literal.length = length;
    return expr;
}

static CnAstExpr *make_bool_literal(int value)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_BOOL_LITERAL;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.bool_literal.value = value;
    return expr;
}

static CnAstExpr *make_identifier(const char *name, size_t length)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_IDENTIFIER;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->is_this_pointer = 0;  // 【修复】初始化为非自身指针
    expr->is_base_pointer = 0;  // 【修复】初始化为非基类指针
    expr->as.identifier.name = name;
    expr->as.identifier.name_length = length;
    return expr;
}

static CnAstExpr *make_binary(CnAstBinaryOp op, CnAstExpr *left, CnAstExpr *right)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_BINARY;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.binary.op = op;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    return expr;
}

static CnAstExpr *make_assign(CnAstExpr *target, CnAstExpr *value)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_ASSIGN;
    expr->type = NULL;
    // 初始化位置信息为未知
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.assign.target = target;
    expr->as.assign.value = value;
    return expr;
}

static CnAstExpr *make_logical(CnAstLogicalOp op, CnAstExpr *left, CnAstExpr *right)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_LOGICAL;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.logical.op = op;
    expr->as.logical.left = left;
    expr->as.logical.right = right;
    return expr;
}

static CnAstExpr *make_unary(CnAstUnaryOp op, CnAstExpr *operand)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_UNARY;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.unary.op = op;
    expr->as.unary.operand = operand;
    return expr;
}

static CnAstExpr *make_call(CnAstExpr *callee, CnAstExpr **arguments, size_t argument_count)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_CALL;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.call.callee = callee;
    expr->as.call.arguments = arguments;
    expr->as.call.argument_count = argument_count;
    return expr;
}

static CnAstExpr *make_array_literal(CnAstExpr **elements, size_t element_count)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_ARRAY_LITERAL;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.array_literal.elements = elements;
    expr->as.array_literal.element_count = element_count;
    return expr;
}

static CnAstExpr *make_index(CnAstExpr *array, CnAstExpr *index)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_INDEX;
    expr->type = NULL;
    expr->loc.filename = NULL;
    expr->loc.line = 0;
    expr->loc.column = 0;
    expr->as.index.array = array;
    expr->as.index.index = index;
    return expr;
}

static CnAstStmt *make_expr_stmt(CnAstExpr *expr)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_EXPR;
    stmt->loc.filename = NULL;
    stmt->loc.line = 0;
    stmt->loc.column = 0;
    stmt->as.expr.expr = expr;
    return stmt;
}

static CnAstStmt *make_return_stmt(CnAstExpr *expr)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_RETURN;
    stmt->as.return_stmt.expr = expr;
    return stmt;
}

static CnAstStmt *make_if_stmt(CnAstExpr *condition,
                               CnAstBlockStmt *then_block,
                               CnAstBlockStmt *else_block)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_IF;
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_block = then_block;
    stmt->as.if_stmt.else_block = else_block;
    return stmt;
}

static CnAstStmt *make_while_stmt(CnAstExpr *condition, CnAstBlockStmt *body)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_WHILE;
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    return stmt;
}

static CnAstStmt *make_for_stmt(CnAstStmt *init,
                                CnAstExpr *condition,
                                CnAstExpr *update,
                                CnAstBlockStmt *body)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_FOR;
    stmt->as.for_stmt.init = init;
    stmt->as.for_stmt.condition = condition;
    stmt->as.for_stmt.update = update;
    stmt->as.for_stmt.body = body;
    return stmt;
}

static CnAstStmt *make_break_stmt(void)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_BREAK;
    return stmt;
}

static CnAstStmt *make_continue_stmt(void)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_CONTINUE;
    return stmt;
}

// 创建 switch 语句节点
static CnAstStmt *make_switch_stmt(CnAstExpr *expr, CnAstSwitchCase *cases, size_t case_count)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_SWITCH;
    stmt->as.switch_stmt.expr = expr;
    stmt->as.switch_stmt.cases = cases;
    stmt->as.switch_stmt.case_count = case_count;
    return stmt;
}

static CnAstStmt *make_var_decl_stmt(const char *name, size_t name_length, CnType *declared_type, CnAstExpr *initializer, CnVisibility visibility)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_VAR_DECL;
    stmt->as.var_decl.name = name;
    stmt->as.var_decl.name_length = name_length;
    stmt->as.var_decl.declared_type = declared_type;
    stmt->as.var_decl.initializer = initializer;
    stmt->as.var_decl.visibility = visibility;
    stmt->as.var_decl.is_const = 0;   // 默认非常量，具体由解析器在需要时设置
    stmt->as.var_decl.is_static = 0;  // 默认非静态，具体由解析器在需要时设置
    return stmt;
}

static CnAstBlockStmt *make_block(void)
{
    CnAstBlockStmt *block = (CnAstBlockStmt *)malloc(sizeof(CnAstBlockStmt));
    if (!block) {
        return NULL;
    }

    block->stmt_count = 0;
    block->stmts = NULL;
    return block;
}

static void block_add_stmt(CnAstBlockStmt *block, CnAstStmt *stmt)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!block || !stmt) {
        return;
    }

    new_count = block->stmt_count + 1;
    new_array = (CnAstStmt **)realloc(block->stmts, new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    block->stmts = new_array;
    block->stmts[block->stmt_count] = stmt;
    block->stmt_count = new_count;
}

static CnAstProgram *make_program(void)
{
    CnAstProgram *program = (CnAstProgram *)malloc(sizeof(CnAstProgram));
    if (!program) {
        return NULL;
    }

    program->function_count = 0;
    program->functions = NULL;
    program->struct_count = 0;
    program->structs = NULL;
    program->enum_count = 0;
    program->enums = NULL;
    program->import_count = 0;
    program->imports = NULL;
    program->global_var_count = 0;
    program->global_vars = NULL;
    /* OOP支持（阶段11 - 面向对象编程支持） */
    program->class_count = 0;
    program->classes = NULL;
    program->interface_count = 0;
    program->interfaces = NULL;
    /* 泛型编程支持（阶段13 - 模板支持） */
    program->template_func_count = 0;
    program->template_funcs = NULL;
    program->template_struct_count = 0;
    program->template_structs = NULL;
    return program;
}

static void program_add_function(CnAstProgram *program, CnAstFunctionDecl *function_decl)
{
    size_t new_count;
    CnAstFunctionDecl **new_array;

    if (!program || !function_decl) {
        return;
    }

    new_count = program->function_count + 1;
    new_array = (CnAstFunctionDecl **)realloc(program->functions,
                                              new_count * sizeof(CnAstFunctionDecl *));
    if (!new_array) {
        return;
    }

    program->functions = new_array;
    program->functions[program->function_count] = function_decl;
    program->function_count = new_count;
}

// 添加结构体声明到program
static void program_add_struct(CnAstProgram *program, CnAstStmt *struct_decl)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !struct_decl) {
        return;
    }

    new_count = program->struct_count + 1;
    new_array = (CnAstStmt **)realloc(program->structs,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->structs = new_array;
    program->structs[program->struct_count] = struct_decl;
    program->struct_count = new_count;
}

// 添加枚举声明到program
static void program_add_enum(CnAstProgram *program, CnAstStmt *enum_decl)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !enum_decl) {
        return;
    }

    new_count = program->enum_count + 1;
    new_array = (CnAstStmt **)realloc(program->enums,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->enums = new_array;
    program->enums[program->enum_count] = enum_decl;
    program->enum_count = new_count;
}

// 添加类声明到program（阶段11 - 面向对象编程支持）
static void program_add_class(CnAstProgram *program, CnAstStmt *class_decl)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !class_decl) {
        return;
    }

    new_count = program->class_count + 1;
    new_array = (CnAstStmt **)realloc(program->classes,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->classes = new_array;
    program->classes[program->class_count] = class_decl;
    program->class_count = new_count;
}

// 添加接口声明到program（阶段11 - 面向对象编程支持）
static void program_add_interface(CnAstProgram *program, CnAstStmt *interface_decl)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !interface_decl) {
        return;
    }

    new_count = program->interface_count + 1;
    new_array = (CnAstStmt **)realloc(program->interfaces,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->interfaces = new_array;
    program->interfaces[program->interface_count] = interface_decl;
    program->interface_count = new_count;
}

// 添加模板函数声明到program（阶段13 - 泛型编程支持）
static void program_add_template_func(CnAstProgram *program, CnAstStmt *template_func_decl)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !template_func_decl) {
        return;
    }

    new_count = program->template_func_count + 1;
    new_array = (CnAstStmt **)realloc(program->template_funcs,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->template_funcs = new_array;
    program->template_funcs[program->template_func_count] = template_func_decl;
    program->template_func_count = new_count;
}

// 添加模板结构体声明到program（阶段13 - 泛型编程支持）
static void program_add_template_struct(CnAstProgram *program, CnAstStmt *template_struct_decl)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !template_struct_decl) {
        return;
    }

    new_count = program->template_struct_count + 1;
    new_array = (CnAstStmt **)realloc(program->template_structs,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->template_structs = new_array;
    program->template_structs[program->template_struct_count] = template_struct_decl;
    program->template_struct_count = new_count;
}

// 添加导入语句到program
static void program_add_import(CnAstProgram *program, CnAstStmt *import_stmt)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !import_stmt) {
        return;
    }

    new_count = program->import_count + 1;
    new_array = (CnAstStmt **)realloc(program->imports,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->imports = new_array;
    program->imports[program->import_count] = import_stmt;
    program->import_count = new_count;
}

// 添加全局变量声明到program
static void program_add_global_var(CnAstProgram *program, CnAstStmt *var_decl)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !var_decl) {
        return;
    }

    new_count = program->global_var_count + 1;
    new_array = (CnAstStmt **)realloc(program->global_vars,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->global_vars = new_array;
    program->global_vars[program->global_var_count] = var_decl;
    program->global_var_count = new_count;
}

// 解析结构体声明或“结构体 类型 变量”声明
static CnAstStmt *parse_struct_decl(CnParser *parser)
{
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_STRUCT)) {
        return NULL;
    }

    // 读取结构体名称
    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：结构体名称无效");
        }
        return NULL;
    }

    const char *struct_name = parser->current.lexeme_begin;
    size_t struct_name_length = parser->current.lexeme_length;
    parser_advance(parser);

    // 分支1：结构体类型后直接跟 '{' -> 结构体定义
    if (parser->current.kind == CN_TOKEN_LBRACE) {
        // 期望 { 开始结构体定义
        parser_advance(parser);

        // 解析字段列表
        size_t field_capacity = 4;
        size_t field_count = 0;
        CnAstStructField *fields = (CnAstStructField *)malloc(sizeof(CnAstStructField) * field_capacity);
        if (!fields) {
            return NULL;
        }

        while (parser->current.kind != CN_TOKEN_RBRACE && parser->current.kind != CN_TOKEN_EOF) {
            int field_is_const = 0;
            
            // 处理 '常量' 关键字：常量字段声明
            if (parser->current.kind == CN_TOKEN_KEYWORD_CONST) {
                field_is_const = 1;
                parser_advance(parser);
            }
            
            // 使用统一的 parse_type 解析字段类型
            CnType *field_type = parse_type(parser);
            if (!field_type) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少字段类型");
                }
                free(fields);
                return NULL;
            }

            // 检查是否是函数指针字段：返回类型(*)(参数列表) 后跟字段名
            // 或者是普通字段：类型 后跟字段名
            // 函数指针字段语法：返回类型(*字段名)(参数列表)
            const char *field_name = NULL;
            size_t field_name_length = 0;
            
            // 检查是否是函数指针字段语法：类型 后跟 '('
            if (parser->current.kind == CN_TOKEN_LPAREN) {
                // 可能是函数指针字段：返回类型(*字段名)(参数列表)
                parser_advance(parser); // 跳过 '('
                
                // 期望 '*'
                if (parser->current.kind == CN_TOKEN_STAR) {
                    parser_advance(parser); // 跳过 '*'
                    
                    // 读取字段名
                    if (parser->current.kind != CN_TOKEN_IDENT) {
                        parser->error_count++;
                        if (parser->diagnostics) {
                            cn_support_diagnostics_report(parser->diagnostics,
                                                          CN_DIAG_SEVERITY_ERROR,
                                                          CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                                          parser->lexer ? parser->lexer->filename : NULL,
                                                          parser->current.line,
                                                          parser->current.column,
                                                          "语法错误：函数指针字段名称无效");
                        }
                        free(fields);
                        return NULL;
                    }
                    
                    field_name = parser->current.lexeme_begin;
                    field_name_length = parser->current.lexeme_length;
                    parser_advance(parser);
                    
                    // 期望 ')'
                    if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                        free(fields);
                        return NULL;
                    }
                    
                    // 解析参数列表：'(' 类型1, 类型2, ... ')'
                    if (!parser_expect(parser, CN_TOKEN_LPAREN)) {
                        free(fields);
                        return NULL;
                    }
                    
                    size_t param_capacity = 4;
                    size_t param_count = 0;
                    CnType **param_types = (CnType **)malloc(sizeof(CnType*) * param_capacity);
                    if (!param_types) {
                        free(fields);
                        return NULL;
                    }
                    
                    // 解析参数类型，允许空参数列表
                    if (parser->current.kind != CN_TOKEN_RPAREN) {
                        do {
                            CnType *param_type = parse_type(parser);
                            if (!param_type) {
                                parser->error_count++;
                                if (parser->diagnostics) {
                                    cn_support_diagnostics_report(parser->diagnostics,
                                                                  CN_DIAG_SEVERITY_ERROR,
                                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                                  parser->current.line,
                                                                  parser->current.column,
                                                                  "语法错误：函数指针字段参数类型无效");
                                }
                                free(param_types);
                                free(fields);
                                return NULL;
                            }
                            
                            // 扩容
                            if (param_count >= param_capacity) {
                                param_capacity *= 2;
                                CnType **new_param_types = (CnType **)realloc(
                                    param_types, sizeof(CnType*) * param_capacity);
                                if (!new_param_types) {
                                    free(param_types);
                                    free(fields);
                                    return NULL;
                                }
                                param_types = new_param_types;
                            }
                            
                            param_types[param_count++] = param_type;
                            
                            if (parser->current.kind == CN_TOKEN_COMMA) {
                                parser_advance(parser);
                            } else {
                                break;
                            }
                        } while (parser->current.kind != CN_TOKEN_RPAREN &&
                                 parser->current.kind != CN_TOKEN_EOF);
                    }
                    
                    if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                        free(param_types);
                        free(fields);
                        return NULL;
                    }
                    
                    // 创建函数类型和函数指针类型
                    CnType *func_type = cn_type_new_function(field_type, param_types, param_count);
                    field_type = cn_type_new_pointer(func_type);
                } else {
                    // 不是函数指针，回退
                    parser->error_count++;
                    if (parser->diagnostics) {
                        cn_support_diagnostics_report(parser->diagnostics,
                                                      CN_DIAG_SEVERITY_ERROR,
                                                      CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                                      parser->lexer ? parser->lexer->filename : NULL,
                                                      parser->current.line,
                                                      parser->current.column,
                                                      "语法错误：期望函数指针字段声明或字段名称");
                    }
                    free(fields);
                    return NULL;
                }
            } else {
                // 普通字段：类型 字段名
                if (parser->current.kind != CN_TOKEN_IDENT) {
                    parser->error_count++;
                    if (parser->diagnostics) {
                        cn_support_diagnostics_report(parser->diagnostics,
                                                      CN_DIAG_SEVERITY_ERROR,
                                                      CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                                      parser->lexer ? parser->lexer->filename : NULL,
                                                      parser->current.line,
                                                      parser->current.column,
                                                      "语法错误：缺少字段名称");
                    }
                    free(fields);
                    return NULL;
                }
                
                field_name = parser->current.lexeme_begin;
                field_name_length = parser->current.lexeme_length;
                parser_advance(parser);
            }

            // 扩容字段数组
            if (field_count >= field_capacity) {
                field_capacity *= 2;
                CnAstStructField *new_fields = (CnAstStructField *)realloc(
                    fields, sizeof(CnAstStructField) * field_capacity);
                if (!new_fields) {
                    free(fields);
                    return NULL;
                }
                fields = new_fields;
            }

            fields[field_count].name = field_name;
            fields[field_count].name_length = field_name_length;
            fields[field_count].field_type = field_type;
            fields[field_count].is_const = field_is_const;  // 设置常量字段标记
            field_count++;

            // 期望分号
            parser_expect(parser, CN_TOKEN_SEMICOLON);
        }

        // 期望 } 结束结构体定义
        parser_expect(parser, CN_TOKEN_RBRACE);

        return make_struct_decl_stmt(struct_name, struct_name_length, fields, field_count);
    }

    // 分支2：结构体声明 + 变量：结构体 Point p = { ... };
    // 构造结构体类型（与 parse_type 中 IDENT 分支保持一致）
    CnType *declared_type = cn_type_new_struct(struct_name,
                                               struct_name_length,
                                               NULL,
                                               0,
                                               NULL,
                                               NULL,
                                               0);

    // 解析变量名
    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：结构体变量名称无效");
        }
        return NULL;
    }

    const char *var_name = parser->current.lexeme_begin;
    size_t var_name_length = parser->current.lexeme_length;
    parser_advance(parser);

    // 支持 C 风格数组声明：结构体 Point p[10];
    size_t dimension_capacity = 4;
    size_t dimension_count = 0;
    size_t *dimensions = NULL;

    if (parser->current.kind == CN_TOKEN_LBRACKET) {
        dimensions = (size_t *)malloc(sizeof(size_t) * dimension_capacity);
        if (!dimensions) {
            return NULL;
        }

        while (parser->current.kind == CN_TOKEN_LBRACKET) {
            parser_advance(parser);  // 跳过 '['

            size_t array_size = 0;

            // 检查是否指定了数组大小
            if (parser->current.kind != CN_TOKEN_RBRACKET) {
                if (parser->current.kind == CN_TOKEN_INTEGER) {
                    array_size = (size_t)strtol(parser->current.lexeme_begin, NULL, 10);
                    parser_advance(parser);
                } else {
                    parser->error_count++;
                    if (parser->diagnostics) {
                        cn_support_diagnostics_report(parser->diagnostics,
                                                      CN_DIAG_SEVERITY_ERROR,
                                                      CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                                      parser->lexer ? parser->lexer->filename : NULL,
                                                      parser->current.line,
                                                      parser->current.column,
                                                      "语法错误：数组大小必须是整数字面量");
                    }
                    free(dimensions);
                    return NULL;
                }
            }

            if (!parser_expect(parser, CN_TOKEN_RBRACKET)) {
                free(dimensions);
                return NULL;
            }

            if (dimension_count >= dimension_capacity) {
                dimension_capacity *= 2;
                size_t *new_dimensions = (size_t *)realloc(dimensions, sizeof(size_t) * dimension_capacity);
                if (!new_dimensions) {
                    free(dimensions);
                    return NULL;
                }
                dimensions = new_dimensions;
            }

            dimensions[dimension_count++] = array_size;
        }

        // 从右向左构建数组类型
        if (!declared_type) {
            declared_type = cn_type_new_primitive(CN_TYPE_INT);
        }
        for (int i = (int)dimension_count - 1; i >= 0; i--) {
            declared_type = cn_type_new_array(declared_type, dimensions[i]);
        }
        free(dimensions);
    }

    // 可选初始化：= {...} 或一般表达式
    CnAstExpr *initializer = NULL;
    if (parser->current.kind == CN_TOKEN_EQUAL) {
        parser_advance(parser);
        if (parser->current.kind == CN_TOKEN_LBRACE) {
            // 使用已知结构体类型名解析初始化列表
            initializer = parse_struct_literal_with_name(parser, struct_name, struct_name_length);
        } else {
            initializer = parse_expression(parser);
        }
    }

    parser_expect(parser, CN_TOKEN_SEMICOLON);

    return make_var_decl_stmt(var_name, var_name_length, declared_type, initializer, CN_VISIBILITY_DEFAULT);
}

// 创建结构体声明语句
static CnAstStmt *make_struct_decl_stmt(const char *name, size_t name_length, CnAstStructField *fields, size_t field_count)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_STRUCT_DECL;
    stmt->as.struct_decl.name = name;
    stmt->as.struct_decl.name_length = name_length;
    stmt->as.struct_decl.fields = fields;
    stmt->as.struct_decl.field_count = field_count;
    return stmt;
}

// 解析枚举声明
static CnAstStmt *parse_enum_decl(CnParser *parser)
{
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_ENUM)) {
        return NULL;
    }

    // 读取枚举名称
    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：枚举名称无效");
        }
        return NULL;
    }

    const char *enum_name = parser->current.lexeme_begin;
    size_t enum_name_length = parser->current.lexeme_length;
    parser_advance(parser);

    // 期望 { 开始枚举定义
    if (!parser_expect(parser, CN_TOKEN_LBRACE)) {
        return NULL;
    }

    // 解析枚举成员列表
    size_t member_capacity = 4;
    size_t member_count = 0;
    CnAstEnumMember *members = (CnAstEnumMember *)malloc(sizeof(CnAstEnumMember) * member_capacity);
    if (!members) {
        return NULL;
    }

    long next_value = 0; // 枚举默认值从0开始

    while (parser->current.kind != CN_TOKEN_RBRACE && parser->current.kind != CN_TOKEN_EOF) {
        // 解析枚举成员名称
        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：缺少枚举成员名称");
            }
            free(members);
            return NULL;
        }

        // 扩容成员数组
        if (member_count >= member_capacity) {
            member_capacity *= 2;
            CnAstEnumMember *new_members = (CnAstEnumMember *)realloc(
                members, sizeof(CnAstEnumMember) * member_capacity);
            if (!new_members) {
                free(members);
                return NULL;
            }
            members = new_members;
        }

        members[member_count].name = parser->current.lexeme_begin;
        members[member_count].name_length = parser->current.lexeme_length;
        members[member_count].has_value = 0;
        members[member_count].value = next_value;

        parser_advance(parser);

        // 检查是否有显式赋值
        if (parser->current.kind == CN_TOKEN_EQUAL) {
            parser_advance(parser);
            
            // 检查是否有负号
            int is_negative = 0;
            if (parser->current.kind == CN_TOKEN_MINUS) {
                is_negative = 1;
                parser_advance(parser);
            }
            
            if (parser->current.kind != CN_TOKEN_INTEGER) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：期望一个整数值");
                }
                free(members);
                return NULL;
            }

            // 解析枚举值
            long value = 0;
            size_t i;
            for (i = 0; i < parser->current.lexeme_length; i++) {
                value = value * 10 + (parser->current.lexeme_begin[i] - '0');
            }
            
            // 应用负号
            if (is_negative) {
                value = -value;
            }
            
            members[member_count].has_value = 1;
            members[member_count].value = value;
            next_value = value + 1;
            
            parser_advance(parser);
        } else {
            next_value++;
        }

        member_count++;

        // 枚举成员之间用逗号分隔，最后一个可选
        if (parser->current.kind == CN_TOKEN_COMMA) {
            parser_advance(parser);
        } else if (parser->current.kind != CN_TOKEN_RBRACE) {
            // 如果不是逗号也不是右大括号，则报错
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：期望逗号或右大括号");
            }
            free(members);
            return NULL;
        }
    }

    // 期望 } 结束枚举定义
    parser_expect(parser, CN_TOKEN_RBRACE);

    return make_enum_decl_stmt(enum_name, enum_name_length, members, member_count);
}

// 创建枚举声明语句
static CnAstStmt *make_enum_decl_stmt(const char *name, size_t name_length, CnAstEnumMember *members, size_t member_count)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_ENUM_DECL;
    stmt->as.enum_decl.name = name;
    stmt->as.enum_decl.name_length = name_length;
    stmt->as.enum_decl.members = members;
    stmt->as.enum_decl.member_count = member_count;
    return stmt;
}

// 创建结构体成员访问表达式
static CnAstExpr *make_member_access(CnAstExpr *object, const char *member_name, size_t member_name_length, int is_arrow)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_MEMBER_ACCESS;
    expr->type = NULL;
    expr->is_this_pointer = 0;  // 成员访问表达式本身不是this指针
    expr->is_base_pointer = 0;  // 成员访问表达式本身不是base指针
    expr->as.member.object = object;
    expr->as.member.member_name = member_name;
    expr->as.member.member_name_length = member_name_length;
    expr->as.member.is_arrow = is_arrow;
    // 初始化静态成员访问相关字段
    expr->as.member.is_static_member = 0;
    expr->as.member.class_name = NULL;
    expr->as.member.class_name_length = 0;
    return expr;
}

// 创建结构体字面量表达式
static CnAstExpr *make_struct_literal(const char *struct_name, size_t struct_name_length, CnAstStructFieldInit *fields, size_t field_count)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_STRUCT_LITERAL;
    expr->type = NULL;
    expr->as.struct_lit.struct_name = struct_name;
    expr->as.struct_lit.struct_name_length = struct_name_length;
    expr->as.struct_lit.fields = fields;
    expr->as.struct_lit.field_count = field_count;
    return expr;
}

static CnAstExpr *make_memory_read(CnAstExpr *address)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }
    expr->kind = CN_AST_EXPR_MEMORY_READ;
    expr->type = NULL;
    expr->as.memory_read.address = address;
    return expr;
}

static CnAstExpr *make_memory_write(CnAstExpr *address, CnAstExpr *value)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }
    expr->kind = CN_AST_EXPR_MEMORY_WRITE;
    expr->type = NULL;
    expr->as.memory_write.address = address;
    expr->as.memory_write.value = value;
    return expr;
}

static CnAstExpr *make_memory_copy(CnAstExpr *dest, CnAstExpr *src, CnAstExpr *size)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }
    expr->kind = CN_AST_EXPR_MEMORY_COPY;
    expr->type = NULL;
    expr->as.memory_copy.dest = dest;
    expr->as.memory_copy.src = src;
    expr->as.memory_copy.size = size;
    return expr;
}

static CnAstExpr *make_memory_set(CnAstExpr *address, CnAstExpr *value, CnAstExpr *size)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }
    expr->kind = CN_AST_EXPR_MEMORY_SET;
    expr->type = NULL;
    expr->as.memory_set.address = address;
    expr->as.memory_set.value = value;
    expr->as.memory_set.size = size;
    return expr;
}

static CnAstExpr *make_memory_map(CnAstExpr *address, CnAstExpr *size, CnAstExpr *prot, CnAstExpr *flags)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }
    expr->kind = CN_AST_EXPR_MEMORY_MAP;
    expr->type = NULL;
    expr->as.memory_map.address = address;
    expr->as.memory_map.size = size;
    expr->as.memory_map.prot = prot;
    expr->as.memory_map.flags = flags;
    return expr;
}

static CnAstExpr *make_memory_unmap(CnAstExpr *address, CnAstExpr *size)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }
    expr->kind = CN_AST_EXPR_MEMORY_UNMAP;
    expr->type = NULL;
    expr->as.memory_unmap.address = address;
    expr->as.memory_unmap.size = size;
    return expr;
}

static CnAstExpr *make_inline_asm(CnAstExpr *asm_code, CnAstExpr **outputs, size_t output_count, 
                                   CnAstExpr **inputs, size_t input_count, CnAstExpr *clobbers)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }
    expr->kind = CN_AST_EXPR_INLINE_ASM;
    expr->type = NULL;
    expr->as.inline_asm.asm_code = asm_code;
    expr->as.inline_asm.outputs = outputs;
    expr->as.inline_asm.output_count = output_count;
    expr->as.inline_asm.inputs = inputs;
    expr->as.inline_asm.input_count = input_count;
    expr->as.inline_asm.clobbers = clobbers;
    return expr;
}

// 解析导入语句
// 支持以下语法：
// 1. 导入 模块名;  (全量导入)
// 2. 导入 模块名 { 成员1, 成员2 };  (选择性导入)
// 3. 导入 模块名(别名);  (别名导入)
// 4. 导入 ./相对路径;  (相对路径导入)
// 5. 导入 工具/数学;  (斜杠路径导入)
// 6. 导入 工具.数学;  (点分路径导入)
static CnAstStmt *parse_import_stmt(CnParser *parser)
{
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_IMPORT)) {
        return NULL;
    }

    // 检查是否是路径语法（相对路径 ./ 或点/斜杠路径）
    // 相对路径：以点开始
    // 点斜杠路径：需要在标识符后看是否有点或斜杠
    int use_path_syntax = 0;
    if (parser->current.kind == CN_TOKEN_DOT) {
        use_path_syntax = 1;  // 相对路径：./xxx 或 ../xxx
    }

    // 如果是路径语法，使用 parse_module_path
    if (use_path_syntax) {
        // 解析模块路径
        CnAstModulePath *module_path = parse_module_path(parser);
        if (!module_path) {
            return NULL;
        }

        // 检查别名
        const char *alias = NULL;
        size_t alias_length = 0;
        if (parser->current.kind == CN_TOKEN_LPAREN) {
            parser_advance(parser);
            if (parser->current.kind != CN_TOKEN_IDENT) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少别名");
                }
                free(module_path->segments);
                free(module_path);
                return NULL;
            }
            alias = parser->current.lexeme_begin;
            alias_length = parser->current.lexeme_length;
            parser_advance(parser);
            if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                free(module_path->segments);
                free(module_path);
                return NULL;
            }
        }

        // 期望分号
        parser_expect(parser, CN_TOKEN_SEMICOLON);

        // 创建导入语句
        CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
        if (!stmt) {
            free(module_path->segments);
            free(module_path);
            return NULL;
        }

        stmt->kind = CN_AST_STMT_IMPORT;
        stmt->as.import_stmt.module_path = module_path;
        stmt->as.import_stmt.alias = alias;
        stmt->as.import_stmt.alias_length = alias_length;
        stmt->as.import_stmt.members = NULL;
        stmt->as.import_stmt.member_count = 0;
        stmt->as.import_stmt.kind = alias ? CN_IMPORT_ALIAS : CN_IMPORT_FULL;
        stmt->as.import_stmt.is_wildcard = 0;
        stmt->as.import_stmt.use_from_syntax = 0;
        // ./ 前缀表示包导入
        stmt->as.import_stmt.target_type = CN_IMPORT_TARGET_PACKAGE;
        // 传统字段（兼容）
        stmt->as.import_stmt.module_name = NULL;
        stmt->as.import_stmt.module_name_length = 0;
        return stmt;
    }

    // 传统语法：简单的模块名（不含路径）
    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：缺少模块名称");
        }
        return NULL;
    }

    const char *module_name = parser->current.lexeme_begin;
    size_t module_name_length = parser->current.lexeme_length;
    parser_advance(parser);

    // 模块别名支持：导入 模块名(别名);
    // 使用括号语法，不需要"为"关键字
    const char *alias = NULL;
    size_t alias_length = 0;
    
    if (parser->current.kind == CN_TOKEN_LPAREN) {
        // 解析别名：(别名)
        parser_advance(parser);
        
        // 读取别名
        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：缺少别名");
            }
            return NULL;
        }
        
        alias = parser->current.lexeme_begin;
        alias_length = parser->current.lexeme_length;
        parser_advance(parser);
        
        // 期望右括号
        if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
            return NULL;
        }
    }

    // 检查是否有选择性导入
    CnAstImportMember *members = NULL;
    size_t member_count = 0;
    
    if (parser->current.kind == CN_TOKEN_LBRACE) {
        // 选择性导入：导入 模块名 { 成员1, 成员2 };
        parser_advance(parser);
        
        // 解析成员列表
        size_t member_capacity = 4;
        members = (CnAstImportMember *)malloc(sizeof(CnAstImportMember) * member_capacity);
        if (!members) {
            return NULL;
        }
        
        while (parser->current.kind != CN_TOKEN_RBRACE && 
               parser->current.kind != CN_TOKEN_EOF) {
            // 读取成员名称
            if (parser->current.kind != CN_TOKEN_IDENT) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少成员名称");
                }
                free(members);
                return NULL;
            }
            
            // 扩容检查
            if (member_count >= member_capacity) {
                member_capacity *= 2;
                CnAstImportMember *new_members = (CnAstImportMember *)realloc(
                    members, sizeof(CnAstImportMember) * member_capacity);
                if (!new_members) {
                    free(members);
                    return NULL;
                }
                members = new_members;
            }
            
            // 保存成员信息
            members[member_count].name = parser->current.lexeme_begin;
            members[member_count].name_length = parser->current.lexeme_length;
            member_count++;
            
            parser_advance(parser);
            
            // 检查逗号
            if (parser->current.kind == CN_TOKEN_COMMA) {
                parser_advance(parser);
            } else if (parser->current.kind != CN_TOKEN_RBRACE) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：期望 ',' 或 '}'");
                }
                free(members);
                return NULL;
            }
        }
        
        // 期望右大括号
        if (!parser_expect(parser, CN_TOKEN_RBRACE)) {
            free(members);
            return NULL;
        }
    }

    // 期望分号
    parser_expect(parser, CN_TOKEN_SEMICOLON);

    // 创建导入语句
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        if (members) {
            free(members);
        }
        return NULL;
    }

    stmt->kind = CN_AST_STMT_IMPORT;
    stmt->as.import_stmt.module_name = module_name;
    stmt->as.import_stmt.module_name_length = module_name_length;
    stmt->as.import_stmt.alias = alias;
    stmt->as.import_stmt.alias_length = alias_length;
    stmt->as.import_stmt.members = members;
    stmt->as.import_stmt.member_count = member_count;
    // 初始化扩展字段（传统导入语法）
    stmt->as.import_stmt.kind = members ? CN_IMPORT_SELECTIVE : (alias ? CN_IMPORT_ALIAS : CN_IMPORT_FULL);
    stmt->as.import_stmt.module_path = NULL;
    stmt->as.import_stmt.is_wildcard = 0;
    stmt->as.import_stmt.use_from_syntax = 0;
    // 无 ./ 前缀表示模块导入
    stmt->as.import_stmt.target_type = CN_IMPORT_TARGET_MODULE;
    return stmt;
}

// 解析模块路径（支持斜杠和点分隔符）
// 支持语法：
//   1. 斜杠路径：工具/数学/高级
//   2. 点分路径：工具.数学.高级
//   3. 相对导入：./兄弟模块、../父级模块、../../祖父模块
// 返回值需要由调用者释放
static CnAstModulePath *parse_module_path(CnParser *parser)
{
    CnAstModulePath *path = (CnAstModulePath *)malloc(sizeof(CnAstModulePath));
    if (!path) {
        return NULL;
    }
    
    path->segments = NULL;
    path->segment_count = 0;
    path->is_relative = 0;
    path->relative_level = 0;
    
    // 检查相对导入：./ 或 ../ 语法
    // 支持：./模块、../模块、../../模块
    while (parser->current.kind == CN_TOKEN_DOT) {
        path->is_relative = 1;
        path->relative_level++;
        parser_advance(parser);
        
        // 检查是否跟着斜杠（如 ./、../）
        if (parser->current.kind == CN_TOKEN_SLASH) {
            parser_advance(parser);
            // 如果不是继续的点，则退出循环
            if (parser->current.kind != CN_TOKEN_DOT) {
                break;
            }
        }
    }
    
    // 解析模块路径段
    size_t capacity = 4;
    path->segments = (CnAstModulePathSegment *)malloc(sizeof(CnAstModulePathSegment) * capacity);
    if (!path->segments) {
        free(path);
        return NULL;
    }
    
    // 解析第一个标识符
    if (parser->current.kind != CN_TOKEN_IDENT) {
        // 如果是纯相对导入（如 ". "）没有模块名
        if (path->is_relative) {
            // 允许纯相对导入，但段数为 0
            return path;
        }
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：期望模块名称");
        }
        free(path->segments);
        free(path);
        return NULL;
    }
    
    // 解析所有路径段：包/子包/模块 或 包.子包.模块
    while (parser->current.kind == CN_TOKEN_IDENT) {
        // 扩容检查
        if (path->segment_count >= capacity) {
            capacity *= 2;
            CnAstModulePathSegment *new_segments = (CnAstModulePathSegment *)realloc(
                path->segments, sizeof(CnAstModulePathSegment) * capacity);
            if (!new_segments) {
                free(path->segments);
                free(path);
                return NULL;
            }
            path->segments = new_segments;
        }
        
        // 保存当前段
        path->segments[path->segment_count].name = parser->current.lexeme_begin;
        path->segments[path->segment_count].name_length = parser->current.lexeme_length;
        path->segment_count++;
        
        parser_advance(parser);
        
        // 检查是否还有更多路径段（支持斜杠和点作为分隔符）
        if (parser->current.kind == CN_TOKEN_SLASH || 
            parser->current.kind == CN_TOKEN_DOT) {
            parser_advance(parser);
            // 期望下一个标识符
            if (parser->current.kind != CN_TOKEN_IDENT) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：路径分隔符后期望模块名称");
                }
                free(path->segments);
                free(path);
                return NULL;
            }
        } else {
            break;
        }
    }
    
    return path;
}

// 解析「从...导入」语句
// 支持以下语法：
// 1. 从 模块 导入 { 成员1, 成员2 };  (选择性导入)
// 2. 从 模块 导入 *;  (通配符导入)
// 3. 从 包.子包 导入 模块;  (从包导入模块)
// 4. 从 .模块 导入 { 成员 };  (相对导入)
static CnAstStmt *parse_from_import_stmt(CnParser *parser)
{
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_FROM)) {
        return NULL;
    }
    
    // 解析模块路径
    CnAstModulePath *module_path = parse_module_path(parser);
    if (!module_path) {
        return NULL;
    }
    
    // 期望 "导入" 关键字
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_IMPORT)) {
        free(module_path->segments);
        free(module_path);
        return NULL;
    }
    
    CnAstImportMember *members = NULL;
    size_t member_count = 0;
    int is_wildcard = 0;
    CnAstImportKind kind = CN_IMPORT_FROM_SELECTIVE;
    
    // 检查是否是通配符导入 (*)
    if (parser->current.kind == CN_TOKEN_STAR) {
        is_wildcard = 1;
        kind = CN_IMPORT_FROM_WILDCARD;
        parser_advance(parser);
    }
    // 检查是否是选择性导入 ({ 成员1, 成员2 })
    else if (parser->current.kind == CN_TOKEN_LBRACE) {
        parser_advance(parser);
        
        // 解析成员列表
        size_t member_capacity = 4;
        members = (CnAstImportMember *)malloc(sizeof(CnAstImportMember) * member_capacity);
        if (!members) {
            free(module_path->segments);
            free(module_path);
            return NULL;
        }
        
        while (parser->current.kind != CN_TOKEN_RBRACE && 
               parser->current.kind != CN_TOKEN_EOF) {
            // 读取成员名称
            if (parser->current.kind != CN_TOKEN_IDENT) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少成员名称");
                }
                free(members);
                free(module_path->segments);
                free(module_path);
                return NULL;
            }
            
            // 扩容检查
            if (member_count >= member_capacity) {
                member_capacity *= 2;
                CnAstImportMember *new_members = (CnAstImportMember *)realloc(
                    members, sizeof(CnAstImportMember) * member_capacity);
                if (!new_members) {
                    free(members);
                    free(module_path->segments);
                    free(module_path);
                    return NULL;
                }
                members = new_members;
            }
            
            // 保存成员信息
            members[member_count].name = parser->current.lexeme_begin;
            members[member_count].name_length = parser->current.lexeme_length;
            members[member_count].alias = NULL;
            members[member_count].alias_length = 0;
            
            parser_advance(parser);
            
            // 检查是否有别名：成员(别名)
            if (parser->current.kind == CN_TOKEN_LPAREN) {
                parser_advance(parser);
                
                if (parser->current.kind != CN_TOKEN_IDENT) {
                    parser->error_count++;
                    if (parser->diagnostics) {
                        cn_support_diagnostics_report(parser->diagnostics,
                                                      CN_DIAG_SEVERITY_ERROR,
                                                      CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                      parser->lexer ? parser->lexer->filename : NULL,
                                                      parser->current.line,
                                                      parser->current.column,
                                                      "语法错误：缺少别名");
                    }
                    free(members);
                    free(module_path->segments);
                    free(module_path);
                    return NULL;
                }
                
                members[member_count].alias = parser->current.lexeme_begin;
                members[member_count].alias_length = parser->current.lexeme_length;
                parser_advance(parser);
                
                if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                    free(members);
                    free(module_path->segments);
                    free(module_path);
                    return NULL;
                }
            }
            
            member_count++;
            
            // 检查逗号
            if (parser->current.kind == CN_TOKEN_COMMA) {
                parser_advance(parser);
            } else if (parser->current.kind != CN_TOKEN_RBRACE) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：期望 ',' 或 '}'");
                }
                free(members);
                free(module_path->segments);
                free(module_path);
                return NULL;
            }
        }
        
        // 期望右大括号
        if (!parser_expect(parser, CN_TOKEN_RBRACE)) {
            free(members);
            free(module_path->segments);
            free(module_path);
            return NULL;
        }
        
        kind = CN_IMPORT_FROM_SELECTIVE;
    }
    // 从包导入模块：从 包 导入 模块名;
    else if (parser->current.kind == CN_TOKEN_IDENT) {
        // 将当前标识符作为要导入的模块名
        members = (CnAstImportMember *)malloc(sizeof(CnAstImportMember));
        if (!members) {
            free(module_path->segments);
            free(module_path);
            return NULL;
        }
        
        members[0].name = parser->current.lexeme_begin;
        members[0].name_length = parser->current.lexeme_length;
        members[0].alias = NULL;
        members[0].alias_length = 0;
        member_count = 1;
        
        parser_advance(parser);
        
        // 检查是否有别名
        if (parser->current.kind == CN_TOKEN_LPAREN) {
            parser_advance(parser);
            
            if (parser->current.kind != CN_TOKEN_IDENT) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少别名");
                }
                free(members);
                free(module_path->segments);
                free(module_path);
                return NULL;
            }
            
            members[0].alias = parser->current.lexeme_begin;
            members[0].alias_length = parser->current.lexeme_length;
            parser_advance(parser);
            
            if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                free(members);
                free(module_path->segments);
                free(module_path);
                return NULL;
            }
        }
        
        kind = CN_IMPORT_FROM_MODULE;
    }
    else {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：期望 '*'、'{' 或模块名称");
        }
        free(module_path->segments);
        free(module_path);
        return NULL;
    }
    
    // 期望分号
    parser_expect(parser, CN_TOKEN_SEMICOLON);
    
    // 创建导入语句
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        if (members) {
            free(members);
        }
        free(module_path->segments);
        free(module_path);
        return NULL;
    }
    
    stmt->kind = CN_AST_STMT_IMPORT;
    stmt->as.import_stmt.kind = kind;
    
    // 从 module_path 提取模块名（最后一个路径段）
    // 同时设置 module_name 和 module_path，保持向后兼容
    if (module_path && module_path->segment_count > 0) {
        size_t last_idx = module_path->segment_count - 1;
        stmt->as.import_stmt.module_name = module_path->segments[last_idx].name;
        stmt->as.import_stmt.module_name_length = module_path->segments[last_idx].name_length;
    } else {
        stmt->as.import_stmt.module_name = NULL;
        stmt->as.import_stmt.module_name_length = 0;
    }
    
    stmt->as.import_stmt.alias = NULL;
    stmt->as.import_stmt.alias_length = 0;
    stmt->as.import_stmt.members = members;
    stmt->as.import_stmt.member_count = member_count;
    stmt->as.import_stmt.module_path = module_path;
    stmt->as.import_stmt.is_wildcard = is_wildcard;
    stmt->as.import_stmt.use_from_syntax = 1;
    // 根据是否是相对路径决定查找策略：
    // - 相对路径（./xxx）：优先查找包（目录/__包__.cn），然后查找模块（.cn文件）
    // - 普通路径（xxx）：优先查找模块（.cn文件），然后查找包（目录/__包__.cn）
    if (module_path->is_relative) {
        stmt->as.import_stmt.target_type = CN_IMPORT_TARGET_PACKAGE;
    } else {
        stmt->as.import_stmt.target_type = CN_IMPORT_TARGET_MODULE;
    }
    
    return stmt;
}

/* ============================================================================
 * 类和接口解析函数实现（阶段11 - 面向对象编程支持）
 * ============================================================================ */

/**
 * @brief 解析访问级别标签
 *
 * 语法：公开: | 私有: | 保护:
 *
 * @param parser 解析器上下文
 * @return CnAccessLevel 解析到的访问级别，默认返回私有
 */
static CnAccessLevel parse_access_label(CnParser *parser)
{
    if (!parser || !parser->has_current) {
        return CN_ACCESS_PRIVATE;
    }
    
    if (parser->current.kind == CN_TOKEN_KEYWORD_PUBLIC) {
        parser_advance(parser);  // 消费 '公开'
        if (parser->current.kind == CN_TOKEN_COLON) {
            parser_advance(parser);  // 消费 ':'
            return CN_ACCESS_PUBLIC;
        } else {
            // 错误：缺少冒号
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：'公开' 后期望 ':'");
            }
            return CN_ACCESS_PUBLIC;  // 容错返回
        }
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_PRIVATE) {
        parser_advance(parser);  // 消费 '私有'
        if (parser->current.kind == CN_TOKEN_COLON) {
            parser_advance(parser);  // 消费 ':'
            return CN_ACCESS_PRIVATE;
        } else {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：'私有' 后期望 ':'");
            }
            return CN_ACCESS_PRIVATE;
        }
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_PROTECTED) {
        parser_advance(parser);  // 消费 '保护'
        if (parser->current.kind == CN_TOKEN_COLON) {
            parser_advance(parser);  // 消费 ':'
            return CN_ACCESS_PROTECTED;
        } else {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：'保护' 后期望 ':'");
            }
            return CN_ACCESS_PROTECTED;
        }
    }
    
    return CN_ACCESS_PRIVATE;  // 默认私有
}

/**
 * @brief 解析基类列表
 *
 * 语法：基类1, 基类2, ... 或 虚拟 基类1, ...
 *
 * @param parser 解析器上下文
 * @param class_decl 类声明节点
 */

/**
 * @brief 解析接口实现列表
 *
 * 语法：实现 接口名 [, 接口名2, ...]
 *
 * @param parser 解析器上下文
 * @param class_decl 类声明节点
 */
static void parse_interface_list(CnParser *parser, CnAstClassDecl *class_decl)
{
    if (!parser || !class_decl) {
        return;
    }
    
    // 期望 '实现' 关键字
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_IMPLEMENTS)) {
        return;
    }
    
    // 解析接口列表
    do {
        // 期望接口名称
        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：缺少接口名称");
            }
            return;
        }
        
        // 添加实现的接口
        cn_ast_class_decl_add_interface(class_decl,
                                        parser->current.lexeme_begin,
                                        parser->current.lexeme_length);
        
        parser_advance(parser);
        
        // 检查是否有更多接口（用逗号分隔）
        if (parser->current.kind == CN_TOKEN_COMMA) {
            parser_advance(parser);
            continue;
        } else {
            break;
        }
    } while (1);
}

static void parse_base_class_list(CnParser *parser, CnAstClassDecl *class_decl)
{
    if (!parser || !class_decl) {
        return;
    }
    
    // 期望 ':' 开始基类列表
    if (!parser_expect(parser, CN_TOKEN_COLON)) {
        return;
    }
    
    // 解析基类列表（支持多继承）
    // 语法：: [公开|私有|保护] 基类名 [, 基类名2, ...] [实现 接口列表]
    do {
        // 检查是否是 '实现' 关键字（接口实现）
        if (parser->current.kind == CN_TOKEN_KEYWORD_IMPLEMENTS) {
            parse_interface_list(parser, class_decl);
            break;
        }
        
        bool is_virtual = false;
        CnAccessLevel inheritance_access = CN_ACCESS_PUBLIC;  // 默认公开继承
        
        // 检查是否有 '虚拟' 关键字
        if (parser->current.kind == CN_TOKEN_KEYWORD_VIRTUAL) {
            is_virtual = true;
            parser_advance(parser);
        }
        
        // 解析继承方式（可选）
        // 语法：公开 | 私有 | 保护
        if (parser->current.kind == CN_TOKEN_KEYWORD_PUBLIC) {
            inheritance_access = CN_ACCESS_PUBLIC;
            parser_advance(parser);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_PRIVATE) {
            inheritance_access = CN_ACCESS_PRIVATE;
            parser_advance(parser);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_PROTECTED) {
            inheritance_access = CN_ACCESS_PROTECTED;
            parser_advance(parser);
        }
        
        // 检查是否是 '实现' 关键字（接口实现）
        if (parser->current.kind == CN_TOKEN_KEYWORD_IMPLEMENTS) {
            parse_interface_list(parser, class_decl);
            break;
        }
        
        // 期望基类名称
        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：缺少基类名称");
            }
            return;
        }
        
        // 添加基类（包含继承方式）
        cn_ast_class_decl_add_base(class_decl,
                                   parser->current.lexeme_begin,
                                   parser->current.lexeme_length,
                                   inheritance_access,
                                   is_virtual);
        
        parser_advance(parser);
        
        // 检查是否有更多基类（多继承支持，用逗号分隔）
        if (parser->current.kind == CN_TOKEN_COMMA) {
            parser_advance(parser);
            continue;
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_IMPLEMENTS) {
            // 解析接口实现列表
            parse_interface_list(parser, class_decl);
            break;
        } else {
            break;
        }
    } while (1);
}
/**
 * @brief 解析类成员
 *
 * 语法：访问标签 | 字段声明 | 方法声明 | 构造函数 | 析构函数
 *
 * @param parser 解析器上下文
 * @param default_access 默认访问级别
 * @param class_name 类名（用于识别构造函数）
 * @param class_name_length 类名长度
 * @return CnClassMember* 解析到的类成员，失败返回NULL
 */
static CnClassMember *parse_class_member(CnParser *parser, CnAccessLevel default_access,
                                          const char *class_name, size_t class_name_length)
{
    if (!parser || !parser->has_current) {
        return NULL;
    }
    
    CnAccessLevel current_access = default_access;
    bool is_override_member = false; // 重写成员标记
    bool is_static_member = false;  // 静态成员标记
    bool is_virtual_member = false; // 虚函数标记
    
    // 注意：访问级别标签已在 parse_class_decl 中处理，这里不再重复处理
    // 检查是否为重写成员（语法：重写 函数 方法名() 或 重写 静态 函数 方法名()）
    // 新语法：重写关键字移到函数签名开头
    if (parser->current.kind == CN_TOKEN_KEYWORD_OVERRIDE) {
        is_override_member = true;
        parser_advance(parser);  // 消费 '重写'
    }
    
    // 检查是否为静态成员（语法：静态 类型 成员名; 或 静态 函数 方法名()）
    if (parser->current.kind == CN_TOKEN_KEYWORD_STATIC) {
        is_static_member = true;
        parser_advance(parser);  // 消费 '静态'
    }
    
    // 检查是否为虚函数（语法：虚拟 函数 方法名() { 实现 }）
    if (parser->current.kind == CN_TOKEN_KEYWORD_VIRTUAL) {
        is_virtual_member = true;
        parser_advance(parser);  // 消费 '虚拟'
    }
    
    // 检查是否为函数声明（方法、构造函数、析构函数）
    if (parser->current.kind == CN_TOKEN_KEYWORD_FN) {
        parser_advance(parser);  // 消费 '函数'
        
        // 检查是否为析构函数（以 ~ 开头）
        bool is_destructor = false;
        if (parser->current.kind == CN_TOKEN_BITWISE_NOT) {
            is_destructor = true;
            parser_advance(parser);  // 消费 '~'
        }
        
        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              is_destructor ? "语法错误：析构函数缺少类名" : "语法错误：缺少方法名称");
            }
            return NULL;
        }
        
        const char *method_name = parser->current.lexeme_begin;
        size_t method_name_length = parser->current.lexeme_length;
        parser_advance(parser);
        
        // 检查是否为构造函数（函数名与类名相同）
        bool is_constructor = false;
        if (!is_destructor && class_name && class_name_length > 0 &&
            method_name_length == class_name_length &&
            memcmp(method_name, class_name, class_name_length) == 0) {
            is_constructor = true;
        }
        
        // 验证析构函数名必须与类名相同
        if (is_destructor) {
            if (!class_name || class_name_length == 0 ||
                method_name_length != class_name_length ||
                memcmp(method_name, class_name, class_name_length) != 0) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语义错误：析构函数名必须与类名相同");
                }
                // 继续解析，但标记为错误
            }
        }
        
        // 创建成员（构造函数、析构函数或方法）
        CnMemberKind member_kind;
        if (is_destructor) {
            member_kind = CN_MEMBER_DESTRUCTOR;
        } else if (is_constructor) {
            member_kind = CN_MEMBER_CONSTRUCTOR;
        } else {
            member_kind = CN_MEMBER_METHOD;
        }
        CnClassMember *member = cn_ast_class_member_create(method_name, method_name_length,
                                                           member_kind, current_access);
        if (!member) {
            return NULL;
        }
        
        // 设置静态标志
        member->is_static = is_static_member;
        
        // 设置虚函数标志
        member->is_virtual = is_virtual_member;
        
        // 设置重写标志（新语法：重写关键字在函数签名开头）
        member->is_override = is_override_member;
        
        // 重写方法自动继承基类虚函数特性，设置为虚函数
        if (is_override_member) {
            member->is_virtual = true;
        }
        
        // 静态方法不能是虚函数
        if (is_static_member && is_virtual_member) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语义错误：静态方法不能是虚函数");
            }
        }
        
        // 注意：根据语法规范文档（plans/001 CN Language语法规范设计文档.md 第954-974行示例），
        // 子类重写函数时只需要"重写"关键字，不需要"虚拟"关键字。
        // 重写方法会自动继承基类虚函数的虚函数特性，无需重复声明。
        // 因此移除了"重写方法必须声明为虚函数"的检查。
        
        // 解析参数列表
        if (parser_expect(parser, CN_TOKEN_LPAREN)) {
            // 解析参数
            if (parser->current.kind != CN_TOKEN_RPAREN) {
                size_t param_capacity = 4;
                size_t param_count = 0;
                CnAstParameter *params = (CnAstParameter *)malloc(sizeof(CnAstParameter) * param_capacity);
                if (!params) {
                    cn_ast_class_member_destroy(member);
                    return NULL;
                }
                
                do {
                    // 解析参数类型
                    CnType *param_type = parse_type(parser);
                    if (!param_type) {
                        free(params);
                        cn_ast_class_member_destroy(member);
                        return NULL;
                    }
                    
                    // 解析参数名
                    if (parser->current.kind != CN_TOKEN_IDENT) {
                        parser->error_count++;
                        if (parser->diagnostics) {
                            cn_support_diagnostics_report(parser->diagnostics,
                                                          CN_DIAG_SEVERITY_ERROR,
                                                          CN_DIAG_CODE_PARSE_INVALID_PARAM,
                                                          parser->lexer ? parser->lexer->filename : NULL,
                                                          parser->current.line,
                                                          parser->current.column,
                                                          "语法错误：缺少参数名称");
                        }
                        free(params);
                        cn_ast_class_member_destroy(member);
                        return NULL;
                    }
                    
                    // 扩容
                    if (param_count >= param_capacity) {
                        param_capacity *= 2;
                        CnAstParameter *new_params = (CnAstParameter *)realloc(params,
                            sizeof(CnAstParameter) * param_capacity);
                        if (!new_params) {
                            free(params);
                            cn_ast_class_member_destroy(member);
                            return NULL;
                        }
                        params = new_params;
                    }
                    
                    params[param_count].name = parser->current.lexeme_begin;
                    params[param_count].name_length = parser->current.lexeme_length;
                    params[param_count].declared_type = param_type;
                    params[param_count].is_const = false;
                    param_count++;
                    
                    parser_advance(parser);
                    
                    // 检查是否有更多参数
                    if (parser->current.kind == CN_TOKEN_COMMA) {
                        parser_advance(parser);
                    } else {
                        break;
                    }
                } while (1);
                
                member->parameters = params;
                member->parameter_count = param_count;
            }
            
            parser_expect(parser, CN_TOKEN_RPAREN);
        }
        
        // 解析返回类型（构造函数不能有返回类型）
        // 新语法：使用箭头 -> 代替冒号 : 表示返回类型
        if (parser->current.kind == CN_TOKEN_ARROW) {
            if (is_constructor) {
                // 构造函数不能有返回类型，报告错误
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语义错误：构造函数不能有返回类型");
                }
                // 跳过返回类型，继续解析
                parser_advance(parser);
                parse_type(parser);
            } else {
                parser_advance(parser);
                member->type = parse_type(parser);
            }
        }
        
        // 解析初始化列表（仅构造函数支持）
        // 语法：: 成员名(表达式), 成员名(表达式), ...
        // 注意：需要区分返回类型的冒号和初始化列表的冒号
        // 初始化列表的冒号出现在参数列表之后、函数体之前
        if (is_constructor && parser->current.kind == CN_TOKEN_COLON) {
            parser_advance(parser);  // 消费 ':'
            
            // 解析初始化列表
            size_t init_capacity = 4;
            size_t init_count = 0;
            CnAstStructFieldInit *init_list = (CnAstStructFieldInit *)malloc(
                sizeof(CnAstStructFieldInit) * init_capacity);
            if (!init_list) {
                cn_ast_class_member_destroy(member);
                return NULL;
            }
            
            do {
                // 解析成员名
                if (parser->current.kind != CN_TOKEN_IDENT) {
                    parser->error_count++;
                    if (parser->diagnostics) {
                        cn_support_diagnostics_report(parser->diagnostics,
                                                      CN_DIAG_SEVERITY_ERROR,
                                                      CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                      parser->lexer ? parser->lexer->filename : NULL,
                                                      parser->current.line,
                                                      parser->current.column,
                                                      "语法错误：初始化列表中缺少成员名称");
                    }
                    free(init_list);
                    cn_ast_class_member_destroy(member);
                    return NULL;
                }
                
                const char *member_name = parser->current.lexeme_begin;
                size_t member_name_length = parser->current.lexeme_length;
                parser_advance(parser);
                
                // 期望 '('
                if (!parser_expect(parser, CN_TOKEN_LPAREN)) {
                    free(init_list);
                    cn_ast_class_member_destroy(member);
                    return NULL;
                }
                
                // 解析初始化表达式
                CnAstExpr *init_expr = NULL;
                if (parser->current.kind != CN_TOKEN_RPAREN) {
                    init_expr = parse_expression(parser);
                }
                
                // 期望 ')'
                if (!parser_expect(parser, CN_TOKEN_RPAREN)) {
                    free(init_list);
                    cn_ast_class_member_destroy(member);
                    return NULL;
                }
                
                // 扩容
                if (init_count >= init_capacity) {
                    init_capacity *= 2;
                    CnAstStructFieldInit *new_init_list = (CnAstStructFieldInit *)realloc(init_list,
                        sizeof(CnAstStructFieldInit) * init_capacity);
                    if (!new_init_list) {
                        free(init_list);
                        cn_ast_class_member_destroy(member);
                        return NULL;
                    }
                    init_list = new_init_list;
                }
                
                init_list[init_count].field_name = member_name;
                init_list[init_count].field_name_length = member_name_length;
                init_list[init_count].value = init_expr;
                init_count++;
                
                // 检查是否有更多初始化项
                if (parser->current.kind == CN_TOKEN_COMMA) {
                    parser_advance(parser);
                } else {
                    break;
                }
            } while (1);
            
            // 将初始化列表存储到成员的扩展数据中
            // 注意：CnClassMember 没有专门的初始化列表字段，我们暂时使用 init_expr 存储
            // 这是一个简化处理，完整实现应该扩展 CnClassMember 结构
            // TODO: 扩展 CnClassMember 添加 initializer_list 字段
            (void)init_list;  // 暂时避免未使用警告
            (void)init_count;
            // 注意：这里存在内存泄漏，需要在 CnClassMember 中添加字段来存储
        }
        
        // 解析抽象关键字（纯虚函数：虚拟 函数 方法名() 抽象;）
        if (parser->current.kind == CN_TOKEN_KEYWORD_ABSTRACT) {
            // 只有虚函数才能标记为抽象
            if (!is_virtual_member) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语义错误：只有虚函数才能标记为抽象");
                }
            }
            member->is_pure_virtual = true;
            parser_advance(parser);  // 消费 '抽象'
        }
        
        // 解析函数体
        if (parser->current.kind == CN_TOKEN_LBRACE) {
            // 纯虚函数不能有函数体
            if (member->is_pure_virtual) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语义错误：纯虚函数不能有函数体");
                }
            }
            member->body = parse_block(parser);
        } else {
            // 期望分号（纯声明）
            parser_expect(parser, CN_TOKEN_SEMICOLON);
        }
        
        return member;
    }
    
    // 检查是否为字段声明（变量声明）
    if (parser->current.kind == CN_TOKEN_KEYWORD_VAR ||
        parser->current.kind == CN_TOKEN_KEYWORD_CONST) {
        bool is_const = (parser->current.kind == CN_TOKEN_KEYWORD_CONST);
        parser_advance(parser);
        
        // 解析字段类型
        CnType *field_type = parse_type(parser);
        if (!field_type) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：缺少字段类型");
            }
            return NULL;
        }
        
        // 解析字段名
        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：缺少字段名称");
            }
            return NULL;
        }
        
        const char *field_name = parser->current.lexeme_begin;
        size_t field_name_length = parser->current.lexeme_length;
        parser_advance(parser);
        
        // 创建字段成员
        CnClassMember *member = cn_ast_class_member_create(field_name, field_name_length,
                                                           CN_MEMBER_FIELD, current_access);
        if (!member) {
            return NULL;
        }
        
        member->type = field_type;
        member->is_const = is_const;
        member->is_static = is_static_member;  // 设置静态标志
        
        // 解析初始化表达式
        if (parser->current.kind == CN_TOKEN_EQUAL) {
            parser_advance(parser);
            member->init_expr = parse_expression(parser);
        }
        
        // 期望分号
        parser_expect(parser, CN_TOKEN_SEMICOLON);
        
        return member;
    }
    
    // 尝试解析类型开头的字段声明（如：整数 年龄;）
    CnType *field_type = parse_type(parser);
    if (field_type) {
        // 解析字段名
        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：缺少字段名称");
            }
            return NULL;
        }
        
        const char *field_name = parser->current.lexeme_begin;
        size_t field_name_length = parser->current.lexeme_length;
        parser_advance(parser);
        
        // 创建字段成员
        CnClassMember *member = cn_ast_class_member_create(field_name, field_name_length,
                                                           CN_MEMBER_FIELD, current_access);
        if (!member) {
            return NULL;
        }
        
        member->type = field_type;
        member->is_static = is_static_member;  // 设置静态标志
        
        // 解析初始化表达式
        if (parser->current.kind == CN_TOKEN_EQUAL) {
            parser_advance(parser);
            member->init_expr = parse_expression(parser);
        }
        
        // 期望分号
        parser_expect(parser, CN_TOKEN_SEMICOLON);
        
        return member;
    }
    
    return NULL;
}

/**
 * @brief 解析类定义
 *
 * 语法：类 类名 [: 基类列表] { 成员列表 }
 *
 * @param parser 解析器上下文
 * @return CnAstStmt* 类声明语句节点
 */
static CnAstStmt *parse_class_decl(CnParser *parser)
{
    // 检查是否为抽象类（抽象 类 ...）
    bool is_abstract = false;
    if (parser->current.kind == CN_TOKEN_KEYWORD_ABSTRACT) {
        is_abstract = true;
        parser_advance(parser);  // 消费 '抽象' 关键字
    }
    
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_CLASS)) {
        return NULL;
    }
    
    // 读取类名
    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：缺少类名称");
        }
        return NULL;
    }
    
    const char *class_name = parser->current.lexeme_begin;
    size_t class_name_length = parser->current.lexeme_length;
    parser_advance(parser);
    
    // 创建类声明节点
    CnAstClassDecl *class_decl = cn_ast_class_decl_create(class_name, class_name_length);
    if (!class_decl) {
        return NULL;
    }
    
    // 设置抽象类标记
    class_decl->is_abstract = is_abstract;
    
    // 解析基类列表（可选）
    if (parser->current.kind == CN_TOKEN_COLON) {
        parse_base_class_list(parser, class_decl);
    }
    
    // 期望 '{' 开始类体
    if (!parser_expect(parser, CN_TOKEN_LBRACE)) {
        cn_ast_class_decl_destroy(class_decl);
        return NULL;
    }
    
    // 解析成员列表
    CnAccessLevel current_access = CN_ACCESS_PRIVATE;  // 默认私有
    
    while (parser->current.kind != CN_TOKEN_RBRACE && parser->current.kind != CN_TOKEN_EOF) {
        // 检查访问级别标签
        if (parser->current.kind == CN_TOKEN_KEYWORD_PUBLIC ||
            parser->current.kind == CN_TOKEN_KEYWORD_PRIVATE ||
            parser->current.kind == CN_TOKEN_KEYWORD_PROTECTED) {
            current_access = parse_access_label(parser);
            continue;
        }
        
        // 解析成员（传入类名用于识别构造函数）
        CnClassMember *member = parse_class_member(parser, current_access,
                                                    class_name, class_name_length);
        if (member) {
            cn_ast_class_decl_add_member(class_decl, member);
        } else {
            // 如果解析失败且不是访问标签，跳过当前token
            if (parser->current.kind != CN_TOKEN_KEYWORD_PUBLIC &&
                parser->current.kind != CN_TOKEN_KEYWORD_PRIVATE &&
                parser->current.kind != CN_TOKEN_KEYWORD_PROTECTED &&
                parser->current.kind != CN_TOKEN_RBRACE) {
                parser_advance(parser);
            }
        }
    }
    
    // 期望 '}' 结束类体
    if (!parser_expect(parser, CN_TOKEN_RBRACE)) {
        cn_ast_class_decl_destroy(class_decl);
        return NULL;
    }
    
    // 包装为语句节点
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        cn_ast_class_decl_destroy(class_decl);
        return NULL;
    }
    
    stmt->kind = CN_AST_STMT_CLASS_DECL;
    stmt->as.class_decl = class_decl;
    
    return stmt;
}

/**
 * @brief 解析接口定义
 *
 * 语法：接口 接口名 [: 基接口列表] { 方法签名列表 }
 *
 * @param parser 解析器上下文
 * @return CnAstStmt* 接口声明语句节点
 */
static CnAstStmt *parse_interface_decl(CnParser *parser)
{
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_INTERFACE)) {
        return NULL;
    }
    
    // 读取接口名
    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：缺少接口名称");
        }
        return NULL;
    }
    
    const char *interface_name = parser->current.lexeme_begin;
    size_t interface_name_length = parser->current.lexeme_length;
    parser_advance(parser);
    
    // 创建接口声明节点
    CnAstInterfaceDecl *interface_decl = cn_ast_interface_decl_create(interface_name, interface_name_length);
    if (!interface_decl) {
        return NULL;
    }
    
    // 解析基接口列表（可选）
    // 语法：: 基接口名 [, 基接口名2, ...]
    if (parser->current.kind == CN_TOKEN_COLON) {
        parser_advance(parser);  // 消费 ':'
        
        do {
            // 期望基接口名称
            if (parser->current.kind != CN_TOKEN_IDENT) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少基接口名称");
                }
                cn_ast_interface_decl_destroy(interface_decl);
                return NULL;
            }
            
            // 添加基接口
            cn_ast_interface_decl_add_base_interface(interface_decl,
                                                     parser->current.lexeme_begin,
                                                     parser->current.lexeme_length);
            
            parser_advance(parser);
            
            // 检查是否有更多基接口（用逗号分隔）
            if (parser->current.kind == CN_TOKEN_COMMA) {
                parser_advance(parser);
                continue;
            } else {
                break;
            }
        } while (1);
    }
    
    // 期望 '{' 开始接口体
    if (!parser_expect(parser, CN_TOKEN_LBRACE)) {
        cn_ast_interface_decl_destroy(interface_decl);
        return NULL;
    }
    
    // 解析方法签名列表
    while (parser->current.kind != CN_TOKEN_RBRACE && parser->current.kind != CN_TOKEN_EOF) {
        // 期望 '函数' 关键字
        if (parser->current.kind != CN_TOKEN_KEYWORD_FN) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：接口中只能包含方法声明");
            }
            parser_advance(parser);
            continue;
        }
        
        parser_advance(parser);  // 消费 '函数'
        
        // 读取方法名
        if (parser->current.kind != CN_TOKEN_IDENT) {
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：缺少方法名称");
            }
            cn_ast_interface_decl_destroy(interface_decl);
            return NULL;
        }
        
        const char *method_name = parser->current.lexeme_begin;
        size_t method_name_length = parser->current.lexeme_length;
        parser_advance(parser);
        
        // 创建方法成员
        CnClassMember *method = cn_ast_class_member_create(method_name, method_name_length,
                                                           CN_MEMBER_METHOD, CN_ACCESS_PUBLIC);
        if (!method) {
            cn_ast_interface_decl_destroy(interface_decl);
            return NULL;
        }
        
        method->is_pure_virtual = true;  // 接口方法都是纯虚函数
        
        // 解析参数列表
        if (parser_expect(parser, CN_TOKEN_LPAREN)) {
            if (parser->current.kind != CN_TOKEN_RPAREN) {
                size_t param_capacity = 4;
                size_t param_count = 0;
                CnAstParameter *params = (CnAstParameter *)malloc(sizeof(CnAstParameter) * param_capacity);
                if (!params) {
                    cn_ast_class_member_destroy(method);
                    cn_ast_interface_decl_destroy(interface_decl);
                    return NULL;
                }
                
                do {
                    CnType *param_type = parse_type(parser);
                    if (!param_type) {
                        free(params);
                        cn_ast_class_member_destroy(method);
                        cn_ast_interface_decl_destroy(interface_decl);
                        return NULL;
                    }
                    
                    if (parser->current.kind != CN_TOKEN_IDENT) {
                        parser->error_count++;
                        free(params);
                        cn_ast_class_member_destroy(method);
                        cn_ast_interface_decl_destroy(interface_decl);
                        return NULL;
                    }
                    
                    if (param_count >= param_capacity) {
                        param_capacity *= 2;
                        CnAstParameter *new_params = (CnAstParameter *)realloc(params,
                            sizeof(CnAstParameter) * param_capacity);
                        if (!new_params) {
                            free(params);
                            cn_ast_class_member_destroy(method);
                            cn_ast_interface_decl_destroy(interface_decl);
                            return NULL;
                        }
                        params = new_params;
                    }
                    
                    params[param_count].name = parser->current.lexeme_begin;
                    params[param_count].name_length = parser->current.lexeme_length;
                    params[param_count].declared_type = param_type;
                    params[param_count].is_const = false;
                    param_count++;
                    
                    parser_advance(parser);
                    
                    if (parser->current.kind == CN_TOKEN_COMMA) {
                        parser_advance(parser);
                    } else {
                        break;
                    }
                } while (1);
                
                method->parameters = params;
                method->parameter_count = param_count;
            }
            
            parser_expect(parser, CN_TOKEN_RPAREN);
        }
        
        // 解析返回类型（使用 -> 语法，与语法规范一致）
        if (parser->current.kind == CN_TOKEN_ARROW) {
            parser_advance(parser);  // 跳过 '->'
            method->type = parse_type(parser);
        }
        
        // 期望分号
        parser_expect(parser, CN_TOKEN_SEMICOLON);
        
        // 添加方法到接口
        cn_ast_interface_decl_add_method(interface_decl, method);
    }
    
    // 期望 '}' 结束接口体
    if (!parser_expect(parser, CN_TOKEN_RBRACE)) {
        cn_ast_interface_decl_destroy(interface_decl);
        return NULL;
    }
    
    // 包装为语句节点
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        cn_ast_interface_decl_destroy(interface_decl);
        return NULL;
    }
    
    stmt->kind = CN_AST_STMT_INTERFACE_DECL;
    stmt->as.interface_decl = interface_decl;
    
    return stmt;
}

/* ============================================================================
 * 模板解析函数（阶段13 - 泛型编程支持）
 * ============================================================================ */

/**
 * @brief 解析模板参数列表
 *
 * 语法：模板<参数1, 参数2, ...>
 * 例如：模板<T>、模板<K, V>
 *
 * @param parser 解析器上下文
 * @return CnAstTemplateParams* 解析后的参数列表，失败返回NULL
 */
static CnAstTemplateParams *parse_template_params(CnParser *parser)
{
    CnAstTemplateParams *params;
    size_t capacity = 4;  // 初始容量
    
    if (!parser) {
        return NULL;
    }
    
    // 期望 '模板' 关键字
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_TEMPLATE)) {
        return NULL;
    }
    
    // 期望 '<'
    if (!parser_expect(parser, CN_TOKEN_LESS)) {
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：期望 '<' 开始模板参数列表");
        }
        return NULL;
    }
    
    // 分配参数列表结构
    params = (CnAstTemplateParams *)malloc(sizeof(CnAstTemplateParams));
    if (!params) {
        return NULL;
    }
    
    params->params = (CnAstTemplateParam *)malloc(sizeof(CnAstTemplateParam) * capacity);
    if (!params->params) {
        free(params);
        return NULL;
    }
    params->param_count = 0;
    
    // 确保有当前token
    if (!parser->has_current) {
        parser_advance(parser);
    }
    
    // 解析参数列表
    do {
        // 检查是否为空参数列表
        if (parser->current.kind == CN_TOKEN_GREATER) {
            break;
        }
        
        // 期望标识符作为类型参数名
        if (parser->current.kind != CN_TOKEN_IDENT) {
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：期望类型参数名称");
            }
            free(params->params);
            free(params);
            return NULL;
        }
        
        // 扩容检查
        if (params->param_count >= capacity) {
            capacity *= 2;
            CnAstTemplateParam *new_params = (CnAstTemplateParam *)realloc(params->params,
                                            sizeof(CnAstTemplateParam) * capacity);
            if (!new_params) {
                free(params->params);
                free(params);
                return NULL;
            }
            params->params = new_params;
        }
        
        // 记录参数信息
        params->params[params->param_count].name = parser->current.lexeme_begin;
        params->params[params->param_count].name_length = parser->current.lexeme_length;
        params->params[params->param_count].constraint = NULL;    // 第一期不支持
        params->params[params->param_count].default_type = NULL;  // 第一期不支持
        params->param_count++;
        
        parser_advance(parser);
        
        // 检查是否有逗号分隔
        if (parser->current.kind == CN_TOKEN_COMMA) {
            parser_advance(parser);
        } else {
            break;
        }
    } while (1);
    
    // 期望 '>'
    if (!parser_expect(parser, CN_TOKEN_GREATER)) {
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：期望 '>' 结束模板参数列表");
        }
        free(params->params);
        free(params);
        return NULL;
    }
    
    return params;
}

/**
 * @brief 解析模板声明
 *
 * 根据模板后的关键字判断是模板函数还是模板结构体
 * 语法：模板<T> 函数 ... 或 模板<T> 结构体 ...
 */
static CnAstStmt *parse_template_declaration(CnParser *parser)
{
    CnAstTemplateParams *params;
    
    if (!parser) {
        return NULL;
    }
    
    // 解析模板参数
    params = parse_template_params(parser);
    if (!params) {
        return NULL;
    }
    
    // 确保有当前token
    if (!parser->has_current) {
        parser_advance(parser);
    }
    
    // 判断是模板函数还是模板结构体
    if (parser->current.kind == CN_TOKEN_KEYWORD_FN) {
        // 模板函数
        return parse_template_function_decl_with_params(parser, params);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRUCT) {
        // 模板结构体
        return parse_template_struct_decl_with_params(parser, params);
    } else {
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：模板声明后期望 '函数' 或 '结构体'");
        }
        free(params->params);
        free(params);
        return NULL;
    }
}

/**
 * @brief 解析模板函数声明（带已有参数）
 *
 * 语法：模板<T> 函数 函数名(参数列表) -> 返回类型 { 函数体 }
 */
static CnAstStmt *parse_template_function_decl_with_params(CnParser *parser, CnAstTemplateParams *params)
{
    CnAstFunctionDecl *function;
    CnAstTemplateFunctionDecl *template_func;
    CnAstStmt *stmt;
    
    if (!parser || !params) {
        return NULL;
    }
    
    // 解析函数声明（复用现有函数解析逻辑）
    function = parse_function_decl(parser);
    if (!function) {
        free(params->params);
        free(params);
        return NULL;
    }
    
    // 创建模板函数节点
    template_func = (CnAstTemplateFunctionDecl *)malloc(sizeof(CnAstTemplateFunctionDecl));
    if (!template_func) {
        free(params->params);
        free(params);
        return NULL;
    }
    
    template_func->template_params = params;
    template_func->function = function;
    
    // 包装为语句节点
    stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        free(template_func);
        free(params->params);
        free(params);
        return NULL;
    }
    
    stmt->kind = CN_AST_STMT_TEMPLATE_FUNCTION_DECL;
    stmt->as.template_func_decl = template_func;
    
    return stmt;
}

/**
 * @brief 解析模板结构体声明（带已有参数）
 *
 * 语法：模板<T> 结构体 结构体名 { 字段列表 }
 */
static CnAstStmt *parse_template_struct_decl_with_params(CnParser *parser, CnAstTemplateParams *params)
{
    CnAstStmt *struct_stmt;
    CnAstTemplateStructDecl *template_struct;
    CnAstStmt *stmt;
    
    if (!parser || !params) {
        return NULL;
    }
    
    // 解析结构体声明（复用现有结构体解析逻辑）
    struct_stmt = parse_struct_decl(parser);
    if (!struct_stmt) {
        free(params->params);
        free(params);
        return NULL;
    }
    
    // 确保是结构体声明
    if (struct_stmt->kind != CN_AST_STMT_STRUCT_DECL) {
        free(struct_stmt);
        free(params->params);
        free(params);
        return NULL;
    }
    
    // 创建模板结构体节点
    template_struct = (CnAstTemplateStructDecl *)malloc(sizeof(CnAstTemplateStructDecl));
    if (!template_struct) {
        free(struct_stmt);
        free(params->params);
        free(params);
        return NULL;
    }
    
    template_struct->template_params = params;
    template_struct->struct_decl = &struct_stmt->as.struct_decl;
    
    // 包装为语句节点
    stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        free(template_struct);
        free(struct_stmt);
        free(params->params);
        free(params);
        return NULL;
    }
    
    stmt->kind = CN_AST_STMT_TEMPLATE_STRUCT_DECL;
    stmt->as.template_struct_decl = template_struct;
    
    // 释放原始结构体语句节点（但保留其内容）
    free(struct_stmt);
    
    return stmt;
}

/**
 * @brief 解析模板实例化表达式
 *
 * 语法：模板名<类型1, 类型2, ...>
 * 例如：最大值<整数>、数组<小数>、映射<字符串, 整数>
 *
 * @param parser 解析器上下文
 * @param name 模板名称
 * @param name_len 名称长度
 * @return CnAstExpr* 模板实例化表达式，失败返回NULL
 */
static CnAstExpr *parse_template_instantiation(CnParser *parser, const char *name, size_t name_len)
{
    CnAstTemplateInstantiationExpr *inst;
    CnAstExpr *expr;
    size_t capacity = 4;  // 初始容量
    
    if (!parser || !name) {
        return NULL;
    }
    
    // 期望 '<'
    if (!parser_expect(parser, CN_TOKEN_LESS)) {
        return NULL;  // 不是模板实例化，可能是比较表达式
    }
    
    // 分配实例化结构
    inst = (CnAstTemplateInstantiationExpr *)malloc(sizeof(CnAstTemplateInstantiationExpr));
    if (!inst) {
        return NULL;
    }
    
    inst->template_name = name;
    inst->template_name_length = name_len;
    inst->type_args = (CnType **)malloc(sizeof(CnType *) * capacity);
    if (!inst->type_args) {
        free(inst);
        return NULL;
    }
    inst->type_arg_count = 0;
    
    // 确保有当前token
    if (!parser->has_current) {
        parser_advance(parser);
    }
    
    // 解析类型实参列表
    do {
        // 检查是否为空参数列表
        if (parser->current.kind == CN_TOKEN_GREATER) {
            break;
        }
        
        // 解析类型
        CnType *type_arg = parse_type(parser);
        if (!type_arg) {
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：期望类型参数");
            }
            free(inst->type_args);
            free(inst);
            return NULL;
        }
        
        // 扩容检查
        if (inst->type_arg_count >= capacity) {
            capacity *= 2;
            CnType **new_args = (CnType **)realloc(inst->type_args,
                                                    sizeof(CnType *) * capacity);
            if (!new_args) {
                free(inst->type_args);
                free(inst);
                return NULL;
            }
            inst->type_args = new_args;
        }
        
        inst->type_args[inst->type_arg_count] = type_arg;
        inst->type_arg_count++;
        
        // 检查是否有逗号分隔
        if (parser->current.kind == CN_TOKEN_COMMA) {
            parser_advance(parser);
        } else {
            break;
        }
    } while (1);
    
    // 期望 '>'
    if (!parser_expect(parser, CN_TOKEN_GREATER)) {
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：期望 '>' 结束模板实参列表");
        }
        free(inst->type_args);
        free(inst);
        return NULL;
    }
    
    // 包装为表达式节点
    expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        free(inst->type_args);
        free(inst);
        return NULL;
    }
    
    expr->kind = CN_AST_EXPR_TEMPLATE_INSTANTIATION;
    expr->type = NULL;  // 语义分析阶段填充
    expr->is_this_pointer = 0;
    // 复制内容而不是指针（union中是值类型）
    expr->as.template_inst.template_name = inst->template_name;
    expr->as.template_inst.template_name_length = inst->template_name_length;
    expr->as.template_inst.type_args = inst->type_args;
    expr->as.template_inst.type_arg_count = inst->type_arg_count;
    free(inst);  // 释放临时结构，但保留其内部指针指向的内容
    
    return expr;
}
