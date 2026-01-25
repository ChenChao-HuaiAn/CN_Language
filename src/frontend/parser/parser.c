#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <stdlib.h>
#include <string.h>

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

static CnAstProgram *parse_program_internal(CnParser *parser);
static CnAstFunctionDecl *parse_function_decl(CnParser *parser);
static CnAstStmt *parse_struct_decl(CnParser *parser);
static CnAstStmt *parse_enum_decl(CnParser *parser);
static CnAstBlockStmt *parse_block(CnParser *parser);
static CnAstStmt *parse_statement(CnParser *parser);
static CnAstExpr *parse_expression(CnParser *parser);
static CnAstExpr *parse_assignment(CnParser *parser);
static CnAstExpr *parse_logical_or(CnParser *parser);
static CnAstExpr *parse_logical_and(CnParser *parser);
static CnAstExpr *parse_comparison(CnParser *parser);
static CnAstExpr *parse_additive(CnParser *parser);
static CnAstExpr *parse_term(CnParser *parser);
static CnAstExpr *parse_unary(CnParser *parser);
static CnAstExpr *parse_postfix(CnParser *parser);
static CnAstExpr *parse_factor(CnParser *parser);

static CnAstExpr *make_integer_literal(long value);
static CnAstExpr *make_float_literal(double value);
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
static CnAstStmt *make_expr_stmt(CnAstExpr *expr);
static CnAstStmt *make_return_stmt(CnAstExpr *expr);
static CnAstStmt *make_if_stmt(CnAstExpr *condition, CnAstBlockStmt *then_block, CnAstBlockStmt *else_block);
static CnAstStmt *make_while_stmt(CnAstExpr *condition, CnAstBlockStmt *body);
static CnAstStmt *make_for_stmt(CnAstStmt *init, CnAstExpr *condition, CnAstExpr *update, CnAstBlockStmt *body);
static CnAstStmt *make_break_stmt(void);
static CnAstStmt *make_continue_stmt(void);
static CnAstStmt *make_var_decl_stmt(const char *name, size_t name_length, CnType *declared_type, CnAstExpr *initializer);
static CnAstStmt *make_struct_decl_stmt(const char *name, size_t name_length, CnAstStructField *fields, size_t field_count);
static CnAstStmt *make_enum_decl_stmt(const char *name, size_t name_length, CnAstEnumMember *members, size_t member_count);
static CnAstBlockStmt *make_block(void);
static void block_add_stmt(CnAstBlockStmt *block, CnAstStmt *stmt);
static CnAstProgram *make_program(void);
static void program_add_function(CnAstProgram *program, CnAstFunctionDecl *function_decl);
static void program_add_struct(CnAstProgram *program, CnAstStmt *struct_decl);
static void program_add_enum(CnAstProgram *program, CnAstStmt *enum_decl);

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

static CnAstProgram *parse_program_internal(CnParser *parser)
{
    CnAstProgram *program = make_program();

    parser_advance(parser);

    while (parser->current.kind != CN_TOKEN_EOF) {
        if (parser->current.kind == CN_TOKEN_KEYWORD_STRUCT) {
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

    if (!parser_expect(parser, CN_TOKEN_KEYWORD_FN)) {
        return NULL;
    }

    if (!parser->has_current) {
        parser_advance(parser);
    }

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
                                          "语法错误：函数名无效");
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
    fn->body = NULL;

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
            CnType *param_type = NULL;
            if (parser->current.kind == CN_TOKEN_KEYWORD_INT) {
                param_type = cn_type_new_primitive(CN_TYPE_INT);
                parser_advance(parser);

                // 支持指针参数类型：例如 "整数* 参数"
                while (parser->current.kind == CN_TOKEN_STAR) {
                    param_type = cn_type_new_pointer(param_type);
                    parser_advance(parser);
                }
            } else if (parser->current.kind == CN_TOKEN_KEYWORD_FLOAT) {
                param_type = cn_type_new_primitive(CN_TYPE_FLOAT);
                parser_advance(parser);

                // 支持指针参数类型：例如 "小数* 参数"
                while (parser->current.kind == CN_TOKEN_STAR) {
                    param_type = cn_type_new_pointer(param_type);
                    parser_advance(parser);
                }
            } else if (parser->current.kind == CN_TOKEN_KEYWORD_BOOL) {
                param_type = cn_type_new_primitive(CN_TYPE_BOOL);
                parser_advance(parser);

                // 支持指针参数类型：例如 "布尔* 参数"
                while (parser->current.kind == CN_TOKEN_STAR) {
                    param_type = cn_type_new_pointer(param_type);
                    parser_advance(parser);
                }
            } else if (parser->current.kind == CN_TOKEN_KEYWORD_ARRAY) {
                // 数组参数类型：例如 "数组 整数 参数"
                parser_advance(parser);
                
                // 检查是否有元素类型关键字（可选）
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
                    // 没有元素类型，默认为泛型数组
                    element_type = cn_type_new_primitive(CN_TYPE_INT);
                }
                
                // 创建数组类型（长度为0表示未知）
                param_type = cn_type_new_array(element_type, 0);
            } else if (parser->current.kind == CN_TOKEN_KEYWORD_VAR) {
                parser_advance(parser);
            } else {
                parser->error_count++;
                if (parser->diagnostics) {
                    cn_support_diagnostics_report(parser->diagnostics,
                                                  CN_DIAG_SEVERITY_ERROR,
                                                  CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                                  parser->lexer ? parser->lexer->filename : NULL,
                                                  parser->current.line,
                                                  parser->current.column,
                                                  "语法错误：缺少参数类型");
                }
                free(params);
                free(fn);
                return NULL;
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

    body = parse_block(parser);
    fn->body = body;

    return fn;
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

    if (parser->current.kind == CN_TOKEN_KEYWORD_VAR ||
        parser->current.kind == CN_TOKEN_KEYWORD_INT ||
        parser->current.kind == CN_TOKEN_KEYWORD_FLOAT ||
        parser->current.kind == CN_TOKEN_KEYWORD_STRING ||
        parser->current.kind == CN_TOKEN_KEYWORD_BOOL ||
        parser->current.kind == CN_TOKEN_KEYWORD_ARRAY) {
        const char *var_name;
        size_t var_name_length;
        CnAstExpr *initializer = NULL;
        CnType *declared_type = NULL;

        if (parser->current.kind == CN_TOKEN_KEYWORD_INT) {
            declared_type = cn_type_new_primitive(CN_TYPE_INT);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_FLOAT) {
            declared_type = cn_type_new_primitive(CN_TYPE_FLOAT);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRING) {
            declared_type = cn_type_new_primitive(CN_TYPE_STRING);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_BOOL) {
            declared_type = cn_type_new_primitive(CN_TYPE_BOOL);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_ARRAY) {
            // 数组关键字，后续可能跟元素类型关键字
            parser_advance(parser);
            
            // 检查是否有元素类型关键字（可选）
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
                // 没有元素类型关键字，默认为泛型数组（后续从初始化器推导）
                element_type = NULL;
            }
            
            // 创建数组类型（长度为0表示未知或动态）
            declared_type = cn_type_new_array(element_type ? element_type : cn_type_new_primitive(CN_TYPE_INT), 0);
            
            // 不再 advance，因为已经在上面处理了
            goto skip_advance;
        }

        parser_advance(parser);
        skip_advance:;

        // 显式类型后允许跟随若干个 '*' 表示指针层级，例如 "整数* 指针变量"
        if (declared_type) {
            while (parser->current.kind == CN_TOKEN_STAR) {
                declared_type = cn_type_new_pointer(declared_type);
                parser_advance(parser);
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
                        CnType *param_type = NULL;
                        
                        if (parser->current.kind == CN_TOKEN_KEYWORD_INT) {
                            param_type = cn_type_new_primitive(CN_TYPE_INT);
                            parser_advance(parser);
                            
                            // 支持指针参数类型
                            while (parser->current.kind == CN_TOKEN_STAR) {
                                param_type = cn_type_new_pointer(param_type);
                                parser_advance(parser);
                            }
                        } else if (parser->current.kind == CN_TOKEN_KEYWORD_FLOAT) {
                            param_type = cn_type_new_primitive(CN_TYPE_FLOAT);
                            parser_advance(parser);
                            
                            while (parser->current.kind == CN_TOKEN_STAR) {
                                param_type = cn_type_new_pointer(param_type);
                                parser_advance(parser);
                            }
                        } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRING) {
                            param_type = cn_type_new_primitive(CN_TYPE_STRING);
                            parser_advance(parser);
                            
                            while (parser->current.kind == CN_TOKEN_STAR) {
                                param_type = cn_type_new_pointer(param_type);
                                parser_advance(parser);
                            }
                        } else if (parser->current.kind == CN_TOKEN_KEYWORD_BOOL) {
                            param_type = cn_type_new_primitive(CN_TYPE_BOOL);
                            parser_advance(parser);
                            
                            while (parser->current.kind == CN_TOKEN_STAR) {
                                param_type = cn_type_new_pointer(param_type);
                                parser_advance(parser);
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
                
                return make_var_decl_stmt(var_name, var_name_length, declared_type, initializer);
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

        if (parser->current.kind == CN_TOKEN_EQUAL) {
            parser_advance(parser);
            initializer = parse_expression(parser);
        }

        parser_expect(parser, CN_TOKEN_SEMICOLON);

        return make_var_decl_stmt(var_name, var_name_length, declared_type, initializer);
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

static CnAstExpr *parse_comparison(CnParser *parser)
{
    CnAstExpr *left = parse_additive(parser);

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
    
    // 处理一元负号 -
    if (parser->current.kind == CN_TOKEN_MINUS) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);  // 递归处理多个 -
        return make_unary(CN_AST_UNARY_OP_MINUS, operand);
    }

    return parse_postfix(parser);  // 支持后缀表达式（如函数调用、数组索引）
}

// 解析后缀表达式（如函数调用、数组索引、成员访问）
static CnAstExpr *parse_postfix(CnParser *parser)
{
    CnAstExpr *expr = parse_factor(parser);

    while (parser->current.kind == CN_TOKEN_LPAREN || 
           parser->current.kind == CN_TOKEN_LBRACKET ||
           parser->current.kind == CN_TOKEN_DOT ||
           parser->current.kind == CN_TOKEN_ARROW) {
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
        }
    }

    return expr;
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
        expr = make_string_literal(parser->current.lexeme_begin, parser->current.lexeme_length);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_TRUE) {
        expr = make_bool_literal(1);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_FALSE) {
        expr = make_bool_literal(0);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_IDENT) {
        expr = make_identifier(parser->current.lexeme_begin, parser->current.lexeme_length);
        parser_advance(parser);
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

static CnAstStmt *make_var_decl_stmt(const char *name, size_t name_length, CnType *declared_type, CnAstExpr *initializer)
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
        // 解析字段类型
        CnType *field_type = NULL;
        if (parser->current.kind == CN_TOKEN_KEYWORD_INT) {
            field_type = cn_type_new_primitive(CN_TYPE_INT);
            parser_advance(parser);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRING) {
            field_type = cn_type_new_primitive(CN_TYPE_STRING);
            parser_advance(parser);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_BOOL) {
            field_type = cn_type_new_primitive(CN_TYPE_BOOL);
            parser_advance(parser);
        } else if (parser->current.kind == CN_TOKEN_KEYWORD_ARRAY) {
            // 数组字段类型：例如 "数组 整数 数据;"
            parser_advance(parser);
            
            // 检查是否有元素类型关键字（可选）
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
                // 没有元素类型，默认为泛型数组
                element_type = cn_type_new_primitive(CN_TYPE_INT);
            }
            
            // 创建数组类型（长度为0表示未知）
            field_type = cn_type_new_array(element_type, 0);
        } else {
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

        // 支持指针类型
        while (parser->current.kind == CN_TOKEN_STAR) {
            field_type = cn_type_new_pointer(field_type);
            parser_advance(parser);
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
