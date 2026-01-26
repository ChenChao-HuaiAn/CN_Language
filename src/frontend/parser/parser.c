#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct CnParser {
    CnLexer *lexer;
    CnToken current;
    int has_current;
    int error_count;
    CnDiagnostics *diagnostics;
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
static CnAstStmt *parse_module_decl(CnParser *parser);
static CnAstStmt *parse_import_stmt(CnParser *parser);
static CnAstBlockStmt *parse_block(CnParser *parser);
static CnAstStmt *parse_statement(CnParser *parser);
static CnType *parse_type(CnParser *parser);
static CnAstExpr *parse_expression(CnParser *parser);
static CnAstExpr *parse_assignment(CnParser *parser);
static CnAstExpr *parse_logical_or(CnParser *parser);
static CnAstExpr *parse_logical_and(CnParser *parser);
static CnAstExpr *parse_comparison(CnParser *parser);
static CnAstExpr *parse_shift(CnParser *parser);
static CnAstExpr *parse_additive(CnParser *parser);
static CnAstExpr *parse_term(CnParser *parser);
static CnAstExpr *parse_unary(CnParser *parser);
static CnAstExpr *parse_postfix(CnParser *parser);
static CnAstExpr *parse_factor(CnParser *parser);

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
static void program_add_module(CnAstProgram *program, CnAstStmt *module_decl);
static void program_add_import(CnAstProgram *program, CnAstStmt *import_stmt);
static void program_add_global_var(CnAstProgram *program, CnAstStmt *var_decl);

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
    return kind == CN_TOKEN_KEYWORD_CLASS ||
           kind == CN_TOKEN_KEYWORD_INTERFACE ||
           kind == CN_TOKEN_KEYWORD_TEMPLATE ||
           kind == CN_TOKEN_KEYWORD_NAMESPACE ||
           /* 注意：“常量” 关键字已在当前版本实现，不再视为预留关键字 */
           kind == CN_TOKEN_KEYWORD_STATIC ||
           kind == CN_TOKEN_KEYWORD_PUBLIC ||
           kind == CN_TOKEN_KEYWORD_PRIVATE ||
           kind == CN_TOKEN_KEYWORD_PROTECTED ||
           kind == CN_TOKEN_KEYWORD_VIRTUAL ||
           kind == CN_TOKEN_KEYWORD_OVERRIDE ||
           kind == CN_TOKEN_KEYWORD_ABSTRACT;
}

// 获取预留关键字的错误消息（静态字符串）
static const char *get_reserved_keyword_error_msg(CnTokenKind kind)
{
    switch (kind) {
    case CN_TOKEN_KEYWORD_CLASS:
        return "语法错误：关键字 '类' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_INTERFACE:
        return "语法错误：关键字 '接口' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_TEMPLATE:
        return "语法错误：关键字 '模板' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_NAMESPACE:
        return "语法错误：关键字 '命名空间' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_STATIC:
        return "语法错误：关键字 '静态' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_PUBLIC:
        return "语法错误：关键字 '公开' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_PRIVATE:
        return "语法错误：关键字 '私有' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_PROTECTED:
        return "语法错误：关键字 '保护' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_VIRTUAL:
        return "语法错误：关键字 '虚拟' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_OVERRIDE:
        return "语法错误：关键字 '重写' 为预留特性，当前版本暂不支持";
    case CN_TOKEN_KEYWORD_ABSTRACT:
        return "语法错误：关键字 '抽象' 为预留特性，当前版本暂不支持";
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

    parser_advance(parser);

    while (parser->current.kind != CN_TOKEN_EOF) {
        // 检查是否为预留关键字
        check_reserved_keyword(parser);

        if (parser->current.kind == CN_TOKEN_KEYWORD_IMPORT) {
            // 解析导入语句
            CnAstStmt *import_stmt = parse_import_stmt(parser);
            if (!import_stmt) {
                break;
            }
            program_add_import(program, import_stmt);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_MODULE) {
            // 解析模块声明
            CnAstStmt *module_decl = parse_module_decl(parser);
            if (!module_decl) {
                break;
            }
            program_add_module(program, module_decl);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRUCT) {
            // 解析结构体声明
            CnAstStmt *struct_decl = parse_struct_decl(parser);
            if (!struct_decl) {
                break;
            }
            program_add_struct(program, struct_decl);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_ENUM) {
            // 解析枚举声明
            CnAstStmt *enum_decl = parse_enum_decl(parser);
            if (!enum_decl) {
                break;
            }
            program_add_enum(program, enum_decl);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_FN) {
            // 解析函数声明
            CnAstFunctionDecl *fn = parse_function_decl(parser);
            if (!fn) {
                break;
            }
            program_add_function(program, fn);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_INTERRUPT_HANDLER) {
            // 解析中断处理函数
            CnAstFunctionDecl *isr = parse_interrupt_handler(parser);
            if (!isr) {
                break;
            }
            program_add_function(program, isr);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_VAR || 
                   parser->current.kind == CN_TOKEN_KEYWORD_CONST) {
            // 解析全局变量声明
            CnAstStmt *var_decl = parse_statement(parser);
            if (!var_decl) {
                break;
            }
            program_add_global_var(program, var_decl);
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

    // 函数名可以是任意标识符，或特殊的 '主程序' 关键字
    if (parser->current.kind != CN_TOKEN_IDENT &&
        parser->current.kind != CN_TOKEN_KEYWORD_MAIN) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：函数名无效，期望标识符或'主程序'");
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

            if (parser->current.kind != CN_TOKEN_IDENT) {
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

            params[param_count].name = parser->current.lexeme_begin;
            params[param_count].name_length = parser->current.lexeme_length;
            params[param_count].declared_type = param_type;
            params[param_count].is_const = param_is_const;
            param_count++;

            parser_advance(parser);

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

    body = parse_block(parser);
    fn->body = body;

    return fn;
}

// 解析中断处理函数声明
// 语法：中断处理 向量号 () { ... }
static CnAstFunctionDecl *parse_interrupt_handler(CnParser *parser)
{
    CnAstFunctionDecl *isr;
    CnAstBlockStmt *body;
    uint32_t vector_num = 0;

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
            else_block = parse_block(parser);
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

    // 解析 switch 语句：选择 (表达式) { [情况 值: 块]* [默认: 块]? }
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
                if (!parser_expect(parser, CN_TOKEN_SEMICOLON)) { // 使用分号作为分隔符
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
                if (!parser_expect(parser, CN_TOKEN_SEMICOLON)) {
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

            // 解析 case/default 的语句块
            case_body = parse_block(parser);
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

    if (parser->current.kind == CN_TOKEN_KEYWORD_CONST ||
        parser->current.kind == CN_TOKEN_KEYWORD_VAR ||
        parser->current.kind == CN_TOKEN_KEYWORD_INT ||
        parser->current.kind == CN_TOKEN_KEYWORD_FLOAT ||
        parser->current.kind == CN_TOKEN_KEYWORD_STRING ||
        parser->current.kind == CN_TOKEN_KEYWORD_BOOL ||
        parser->current.kind == CN_TOKEN_KEYWORD_ARRAY) {
        const char *var_name;
        size_t var_name_length;
        CnAstExpr *initializer = NULL;
        CnType *declared_type = NULL;
        int is_const = 0;

        // 处理 '常量' 关键字：常量变量声明
        if (parser->current.kind == CN_TOKEN_KEYWORD_CONST) {
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
            // 使用统一的 parse_type 解析类型
            declared_type = parse_type(parser);
            if (!declared_type) {
                return NULL;
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
                if (stmt && is_const) {
                    stmt->as.var_decl.is_const = 1;
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

        if (parser->current.kind == CN_TOKEN_EQUAL) {
            parser_advance(parser);
            initializer = parse_expression(parser);
        }

        parser_expect(parser, CN_TOKEN_SEMICOLON);

        CnAstStmt *stmt = make_var_decl_stmt(var_name, var_name_length, declared_type, initializer, CN_VISIBILITY_DEFAULT);
        if (stmt && is_const) {
            stmt->as.var_decl.is_const = 1;
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
    CnAstExpr *expr = parse_logical_or(parser);

    if (parser->current.kind == CN_TOKEN_EQUAL) {
        parser_advance(parser);
        CnAstExpr *value = parse_assignment(parser);  // 右结合
        return make_assign(expr, value);
    }

    return expr;
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
    CnAstExpr *left = parse_comparison(parser);

    while (parser->current.kind == CN_TOKEN_LOGICAL_AND) {
        parser_advance(parser);
        CnAstExpr *right = parse_comparison(parser);
        left = make_logical(CN_AST_LOGICAL_OP_AND, left, right);
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
           parser->current.kind == CN_TOKEN_PERCENT ||
           parser->current.kind == CN_TOKEN_BITWISE_AND ||
           parser->current.kind == CN_TOKEN_BITWISE_OR ||
           parser->current.kind == CN_TOKEN_BITWISE_XOR) {
        CnAstBinaryOp op;
        if (parser->current.kind == CN_TOKEN_STAR) {
            op = CN_AST_BINARY_OP_MUL;
        } else if (parser->current.kind == CN_TOKEN_SLASH) {
            op = CN_AST_BINARY_OP_DIV;
        } else if (parser->current.kind == CN_TOKEN_PERCENT) {
            op = CN_AST_BINARY_OP_MOD;
        } else if (parser->current.kind == CN_TOKEN_BITWISE_AND) {
            op = CN_AST_BINARY_OP_BITWISE_AND;
        } else if (parser->current.kind == CN_TOKEN_BITWISE_OR) {
            op = CN_AST_BINARY_OP_BITWISE_OR;
        } else {
            op = CN_AST_BINARY_OP_BITWISE_XOR;
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
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_MEMORY_ADDRESS) {
        type = cn_type_new_memory_address();
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_ARRAY) {
        // 数组类型：例如 "数组 整数" 或 "数组 小数"
        parser_advance(parser);

        // 解析元素类型（可选）
        CnType *element_type = NULL;
        if (parser->current.kind == CN_TOKEN_KEYWORD_INT) {
            element_type = cn_type_new_primitive(CN_TYPE_INT);
            parser_advance(parser);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_FLOAT) {
            element_type = cn_type_new_primitive(CN_TYPE_FLOAT);
            parser_advance(parser);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRING) {
            element_type = cn_type_new_primitive(CN_TYPE_STRING);
            parser_advance(parser);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_BOOL) {
            element_type = cn_type_new_primitive(CN_TYPE_BOOL);
            parser_advance(parser);
        } else {
            // 没有元素类型，默认为整数数组
            element_type = cn_type_new_primitive(CN_TYPE_INT);
        }

        // 创建数组类型（长度为0表示未知或动态）
        type = cn_type_new_array(element_type, 0);
    } else if (parser->current.kind == CN_TOKEN_IDENT) {
        // 可能是结构体类型或枚举类型（自定义类型）
        // 目前简化处理：将标识符视为结构体类型名
        // TODO: 未来可以通过符号表区分结构体和枚举
        const char *type_name = parser->current.lexeme_begin;
        size_t type_name_length = parser->current.lexeme_length;
        parser_advance(parser);

        // 创建结构体类型（字段列表留空，后续语义分析填充）
        type = cn_type_new_struct(type_name, type_name_length, NULL, 0);
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

    return type;
}

static CnAstExpr *parse_factor(CnParser *parser)
{
    CnAstExpr *expr = NULL;

    if (!parser->has_current) {
        parser_advance(parser);
    }

    if (parser->current.kind == CN_TOKEN_INTEGER) {
        long value = strtol(parser->current.lexeme_begin, NULL, 10);
        expr = make_integer_literal(value);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_FLOAT_LITERAL) {
        double value = strtod(parser->current.lexeme_begin, NULL);
        expr = make_float_literal(value);
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
    } else if (parser->current.kind == CN_TOKEN_IDENT) {
        // 保存标识符信息
        const char *ident_name = parser->current.lexeme_begin;
        size_t ident_name_length = parser->current.lexeme_length;
        parser_advance(parser);
        
        // 检查是否是结构体字面量：标识符 { ... }
        if (parser->current.kind == CN_TOKEN_LBRACE) {
            // 解析结构体字面量：点 { x: 10, y: 20 }
            parser_advance(parser);  // 跳过 {
            
            // 解析字段初始化列表
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
                    
                    // 解析字段名
                    if (parser->current.kind != CN_TOKEN_IDENT) {
                        parser->error_count++;
                        if (parser->diagnostics) {
                            cn_support_diagnostics_report(parser->diagnostics,
                                                          CN_DIAG_SEVERITY_ERROR,
                                                          CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                          parser->lexer ? parser->lexer->filename : NULL,
                                                          parser->current.line,
                                                          parser->current.column,
                                                          "语法错误：结构体字面量中缺少字段名");
                        }
                        free(fields);
                        return NULL;
                    }
                    
                    fields[field_count].field_name = parser->current.lexeme_begin;
                    fields[field_count].field_name_length = parser->current.lexeme_length;
                    parser_advance(parser);
                    
                    // 期望冒号
                    if (!parser_expect(parser, CN_TOKEN_COLON)) {
                        free(fields);
                        return NULL;
                    }
                    
                    // 解析字段值表达式
                    fields[field_count].value = parse_expression(parser);
                    if (!fields[field_count].value) {
                        free(fields);
                        return NULL;
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
                // 清理已分配的资源
                for (size_t i = 0; i < field_count; i++) {
                    if (fields[i].value) {
                        cn_frontend_ast_expr_free(fields[i].value);
                    }
                }
                free(fields);
                return NULL;
            }
            
            expr = make_struct_literal(ident_name, ident_name_length, fields, field_count);
        } else {
            // 普通标识符
            expr = make_identifier(ident_name, ident_name_length);
        }
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
    stmt->as.var_decl.is_const = 0; // 默认非常量，具体由解析器在需要时设置
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
    program->module_count = 0;
    program->modules = NULL;
    program->import_count = 0;
    program->imports = NULL;
    program->global_var_count = 0;
    program->global_vars = NULL;
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

// 添加模块声明到program
static void program_add_module(CnAstProgram *program, CnAstStmt *module_decl)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!program || !module_decl) {
        return;
    }

    new_count = program->module_count + 1;
    new_array = (CnAstStmt **)realloc(program->modules,
                                      new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    program->modules = new_array;
    program->modules[program->module_count] = module_decl;
    program->module_count = new_count;
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

// 解析结构体声明
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

    // 期望 { 开始结构体定义
    if (!parser_expect(parser, CN_TOKEN_LBRACE)) {
        return NULL;
    }

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

        // 解析字段名称
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

        fields[field_count].name = parser->current.lexeme_begin;
        fields[field_count].name_length = parser->current.lexeme_length;
        fields[field_count].field_type = field_type;
        fields[field_count].is_const = field_is_const;  // 设置常量字段标记
        field_count++;

        parser_advance(parser);

        // 期望分号
        parser_expect(parser, CN_TOKEN_SEMICOLON);
    }

    // 期望 } 结束结构体定义
    parser_expect(parser, CN_TOKEN_RBRACE);

    return make_struct_decl_stmt(struct_name, struct_name_length, fields, field_count);
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
    expr->as.member.object = object;
    expr->as.member.member_name = member_name;
    expr->as.member.member_name_length = member_name_length;
    expr->as.member.is_arrow = is_arrow;
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

// 解析模块声明
static CnAstStmt *parse_module_decl(CnParser *parser)
{
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_MODULE)) {
        return NULL;
    }

    // 读取模块名称
    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        if (parser->diagnostics) {
            cn_support_diagnostics_report(parser->diagnostics,
                                          CN_DIAG_SEVERITY_ERROR,
                                          CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
                                          parser->lexer ? parser->lexer->filename : NULL,
                                          parser->current.line,
                                          parser->current.column,
                                          "语法错误：模块名称无效");
        }
        return NULL;
    }

    const char *module_name = parser->current.lexeme_begin;
    size_t module_name_length = parser->current.lexeme_length;
    parser_advance(parser);

    // 期望 { 开始模块定义
    if (!parser_expect(parser, CN_TOKEN_LBRACE)) {
        return NULL;
    }

    // 解析模块内的语句列表（变量等）
    size_t stmt_capacity = 4;
    size_t stmt_count = 0;
    CnAstStmt **stmts = (CnAstStmt **)malloc(sizeof(CnAstStmt *) * stmt_capacity);
    if (!stmts) {
        return NULL;
    }

    // 解析模块内的函数声明列表
    size_t func_capacity = 4;
    size_t func_count = 0;
    CnAstFunctionDecl **functions = (CnAstFunctionDecl **)malloc(sizeof(CnAstFunctionDecl *) * func_capacity);
    if (!functions) {
        free(stmts);
        return NULL;
    }

    // 当前可见性状态（块级），默认为私有
    CnVisibility current_visibility = CN_VISIBILITY_PRIVATE;

    while (parser->current.kind != CN_TOKEN_RBRACE && parser->current.kind != CN_TOKEN_EOF) {
        CnAstStmt *stmt = NULL;
        
        // 检查是否遇到可见性修饰符（块级切换）
        if (parser->current.kind == CN_TOKEN_KEYWORD_PUBLIC) {
            current_visibility = CN_VISIBILITY_PUBLIC;  // 切换到公开块
            parser_advance(parser);
            // 期望冒号
            if (!parser_expect(parser, CN_TOKEN_COLON)) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：期望 ':' 在 '公开' 之后");
                }
                continue;
            }
            continue;  // 继续处理下一个成员
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_PRIVATE) {
            current_visibility = CN_VISIBILITY_PRIVATE;  // 切换到私有块
            parser_advance(parser);
            // 期望冒号
            if (!parser_expect(parser, CN_TOKEN_COLON)) {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：期望 ':' 在 '私有' 之后");
                }
                continue;
            }
            continue;  // 继续处理下一个成员
        }
        
        // 模块内可以包含函数、变量声明等
        if (parser->current.kind == CN_TOKEN_KEYWORD_FN) {
            // 解析函数声明
            CnAstFunctionDecl *fn_decl = parse_function_decl(parser);
            if (!fn_decl) {
                // 清理已分配的资源
                for (size_t i = 0; i < func_count; i++) {
                    free(functions[i]);
                }
                free(functions);
                for (size_t i = 0; i < stmt_count; i++) {
                    free(stmts[i]);
                }
                free(stmts);
                return NULL;
            }
            
            // 扩容函数数组
            if (func_count >= func_capacity) {
                func_capacity *= 2;
                CnAstFunctionDecl **new_functions = (CnAstFunctionDecl **)realloc(
                    functions, sizeof(CnAstFunctionDecl *) * func_capacity);
                if (!new_functions) {
                    free(fn_decl);
                    for (size_t i = 0; i < func_count; i++) {
                        free(functions[i]);
                    }
                    free(functions);
                    for (size_t i = 0; i < stmt_count; i++) {
                        free(stmts[i]);
                    }
                    free(stmts);
                    return NULL;
                }
                functions = new_functions;
            }
            
            functions[func_count] = fn_decl;
            func_count++;
            continue;
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_VAR || 
                   parser->current.kind == CN_TOKEN_KEYWORD_INT ||
                   parser->current.kind == CN_TOKEN_KEYWORD_FLOAT ||
                   parser->current.kind == CN_TOKEN_KEYWORD_STRING) {
            // 解析变量声明
            stmt = parse_statement(parser);
            if (!stmt) {
                for (size_t i = 0; i < func_count; i++) {
                    free(functions[i]);
                }
                free(functions);
                free(stmts);
                return NULL;
            }
            // 将当前可见性状态应用到变量声明
            if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                stmt->as.var_decl.visibility = current_visibility;
            }
        } else {
            // 不支持的语句类型
            parser->error_count++;
            if (parser->diagnostics) {
                cn_support_diagnostics_report(parser->diagnostics,
                                              CN_DIAG_SEVERITY_ERROR,
                                              CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                              parser->lexer ? parser->lexer->filename : NULL,
                                              parser->current.line,
                                              parser->current.column,
                                              "语法错误：模块内不支持此类型的声明");
            }
            parser_advance(parser); // 跳过错误 token
            continue;
        }

        // 扩容语句数组
        if (stmt_count >= stmt_capacity) {
            stmt_capacity *= 2;
            CnAstStmt **new_stmts = (CnAstStmt **)realloc(
                stmts, sizeof(CnAstStmt *) * stmt_capacity);
            if (!new_stmts) {
                free(stmt);
                for (size_t i = 0; i < func_count; i++) {
                    free(functions[i]);
                }
                free(functions);
                free(stmts);
                return NULL;
            }
            stmts = new_stmts;
        }

        stmts[stmt_count] = stmt;
        stmt_count++;
    }

    // 期望 } 结束模块定义
    parser_expect(parser, CN_TOKEN_RBRACE);

    // 创建模块声明语句
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        for (size_t i = 0; i < func_count; i++) {
            free(functions[i]);
        }
        free(functions);
        free(stmts);
        return NULL;
    }

    stmt->kind = CN_AST_STMT_MODULE_DECL;
    stmt->as.module_decl.name = module_name;
    stmt->as.module_decl.name_length = module_name_length;
    stmt->as.module_decl.stmts = stmts;
    stmt->as.module_decl.stmt_count = stmt_count;
    stmt->as.module_decl.functions = functions;
    stmt->as.module_decl.function_count = func_count;
    return stmt;
}

// 解析导入语句
// 支持三种语法：
// 1. 导入 模块名;  (导入所有)
// 2. 导入 模块名 { 成员1, 成员2 };  (选择性导入)
// 3. 导入 模块名 为 别名;  (使用别名)
static CnAstStmt *parse_import_stmt(CnParser *parser)
{
    if (!parser_expect(parser, CN_TOKEN_KEYWORD_IMPORT)) {
        return NULL;
    }

    // 读取模块名称
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

    // 检查是否有别名："为 别名"
    const char *alias = NULL;
    size_t alias_length = 0;
    
    if (parser->current.kind == CN_TOKEN_KEYWORD_AS) {
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
    return stmt;
}
