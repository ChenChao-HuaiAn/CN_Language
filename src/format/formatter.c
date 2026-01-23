#include "cnlang/format/formatter.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR '\\'
#define mkdir_func(path) _mkdir(path)
#else
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEPARATOR '/'
#define mkdir_func(path) mkdir(path, 0755)
#endif

// 格式化器上下文
typedef struct FormatContext {
    FILE *output;
    const CnFormatConfig *config;
    int indent_level;
    bool error_occurred;
} FormatContext;

// 初始化默认格式化配置
void cn_format_config_init_default(CnFormatConfig *config)
{
    // 缩进与空白
    config->indent_size = 4;
    config->max_line_width = 100;
    
    // 空格规则
    config->space_around_ops = true;
    config->space_after_comma = true;
    config->space_after_keywords = true;
    
    // 括号与大括号
    config->brace_on_new_line_func = true;
    config->brace_on_same_line_ctrl = true;
    config->always_use_braces = true;
    
    // 空行使用
    config->empty_line_between_funcs = true;
    config->max_consecutive_empty_lines = 1;
}

// 初始化默认格式化选项
void cn_format_options_init_default(CnFormatOptions *options)
{
    options->in_place = false;
    options->to_stdout = false;
    options->output_file = NULL;
    cn_format_config_init_default(&options->config);
}

// 输出缩进
static void format_indent(FormatContext *ctx)
{
    int spaces = ctx->indent_level * ctx->config->indent_size;
    for (int i = 0; i < spaces; i++) {
        fputc(' ', ctx->output);
    }
}

// 格式化二元运算符
static const char *format_binary_op(CnAstBinaryOp op)
{
    switch (op) {
        case CN_AST_BINARY_OP_ADD: return "+";
        case CN_AST_BINARY_OP_SUB: return "-";
        case CN_AST_BINARY_OP_MUL: return "*";
        case CN_AST_BINARY_OP_DIV: return "/";
        case CN_AST_BINARY_OP_EQ:  return "==";
        case CN_AST_BINARY_OP_NE:  return "!=";
        case CN_AST_BINARY_OP_LT:  return "<";
        case CN_AST_BINARY_OP_GT:  return ">";
        case CN_AST_BINARY_OP_LE:  return "<=";
        case CN_AST_BINARY_OP_GE:  return ">=";
        default: return "?";
    }
}

// 格式化逻辑运算符
static const char *format_logical_op(CnAstLogicalOp op)
{
    switch (op) {
        case CN_AST_LOGICAL_OP_AND: return "&&";
        case CN_AST_LOGICAL_OP_OR:  return "||";
        default: return "?";
    }
}

// 格式化一元运算符
static const char *format_unary_op(CnAstUnaryOp op)
{
    switch (op) {
        case CN_AST_UNARY_OP_NOT:   return "!";
        case CN_AST_UNARY_OP_MINUS: return "-";
        default: return "?";
    }
}

// 前向声明
static void format_expr(FormatContext *ctx, const CnAstExpr *expr);
static void format_stmt(FormatContext *ctx, const CnAstStmt *stmt);

// 格式化表达式
static void format_expr(FormatContext *ctx, const CnAstExpr *expr)
{
    if (!expr) {
        return;
    }

    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(ctx->output, "%ld", expr->as.integer_literal.value);
            break;

        case CN_AST_EXPR_STRING_LITERAL:
            // 字符串字面量已包含引号，直接输出
            fprintf(ctx->output, "%.*s",
                    (int)expr->as.string_literal.length,
                    expr->as.string_literal.value);
            break;

        case CN_AST_EXPR_IDENTIFIER:
            fprintf(ctx->output, "%.*s",
                    (int)expr->as.identifier.name_length,
                    expr->as.identifier.name);
            break;

        case CN_AST_EXPR_BINARY:
            fprintf(ctx->output, "(");
            format_expr(ctx, expr->as.binary.left);
            if (ctx->config->space_around_ops) {
                fprintf(ctx->output, " %s ", format_binary_op(expr->as.binary.op));
            } else {
                fprintf(ctx->output, "%s", format_binary_op(expr->as.binary.op));
            }
            format_expr(ctx, expr->as.binary.right);
            fprintf(ctx->output, ")");
            break;

        case CN_AST_EXPR_LOGICAL:
            fprintf(ctx->output, "(");
            format_expr(ctx, expr->as.logical.left);
            if (ctx->config->space_around_ops) {
                fprintf(ctx->output, " %s ", format_logical_op(expr->as.logical.op));
            } else {
                fprintf(ctx->output, "%s", format_logical_op(expr->as.logical.op));
            }
            format_expr(ctx, expr->as.logical.right);
            fprintf(ctx->output, ")");
            break;

        case CN_AST_EXPR_UNARY:
            fprintf(ctx->output, "%s", format_unary_op(expr->as.unary.op));
            format_expr(ctx, expr->as.unary.operand);
            break;

        case CN_AST_EXPR_CALL: {
            format_expr(ctx, expr->as.call.callee);
            fprintf(ctx->output, "(");
            for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                if (i > 0) {
                    fprintf(ctx->output, ctx->config->space_after_comma ? ", " : ",");
                }
                format_expr(ctx, expr->as.call.arguments[i]);
            }
            fprintf(ctx->output, ")");
            break;
        }

        case CN_AST_EXPR_ASSIGN:
            format_expr(ctx, expr->as.assign.target);
            if (ctx->config->space_around_ops) {
                fprintf(ctx->output, " = ");
            } else {
                fprintf(ctx->output, "=");
            }
            format_expr(ctx, expr->as.assign.value);
            break;

        case CN_AST_EXPR_ARRAY_LITERAL:
            fprintf(ctx->output, "[");
            for (size_t i = 0; i < expr->as.array_literal.element_count; i++) {
                if (i > 0) {
                    fprintf(ctx->output, ctx->config->space_after_comma ? ", " : ",");
                }
                format_expr(ctx, expr->as.array_literal.elements[i]);
            }
            fprintf(ctx->output, "]");
            break;

        case CN_AST_EXPR_INDEX:
            format_expr(ctx, expr->as.index.array);
            fprintf(ctx->output, "[");
            format_expr(ctx, expr->as.index.index);
            fprintf(ctx->output, "]");
            break;

        default:
            fprintf(ctx->output, "<?expr?>");
            break;
    }
}

// 格式化语句块
static void format_block(FormatContext *ctx, const CnAstBlockStmt *block)
{
    if (!block) {
        return;
    }

    for (size_t i = 0; i < block->stmt_count; i++) {
        format_stmt(ctx, block->stmts[i]);
    }
}

// 格式化语句
static void format_stmt(FormatContext *ctx, const CnAstStmt *stmt)
{
    if (!stmt) {
        return;
    }

    switch (stmt->kind) {
        case CN_AST_STMT_VAR_DECL: {
            format_indent(ctx);
            fprintf(ctx->output, "变量 %.*s",
                    (int)stmt->as.var_decl.name_length,
                    stmt->as.var_decl.name);
            if (stmt->as.var_decl.initializer) {
                if (ctx->config->space_around_ops) {
                    fprintf(ctx->output, " = ");
                } else {
                    fprintf(ctx->output, "=");
                }
                format_expr(ctx, stmt->as.var_decl.initializer);
            }
            fprintf(ctx->output, ";\n");
            break;
        }

        case CN_AST_STMT_EXPR:
            format_indent(ctx);
            format_expr(ctx, stmt->as.expr.expr);
            fprintf(ctx->output, ";\n");
            break;

        case CN_AST_STMT_RETURN:
            format_indent(ctx);
            fprintf(ctx->output, "返回");
            if (stmt->as.return_stmt.expr) {
                fprintf(ctx->output, " ");
                format_expr(ctx, stmt->as.return_stmt.expr);
            }
            fprintf(ctx->output, ";\n");
            break;

        case CN_AST_STMT_IF: {
            format_indent(ctx);
            fprintf(ctx->output, "如果");
            if (ctx->config->space_after_keywords) {
                fprintf(ctx->output, " ");
            }
            fprintf(ctx->output, "(");
            format_expr(ctx, stmt->as.if_stmt.condition);
            fprintf(ctx->output, ")");
            if (ctx->config->brace_on_same_line_ctrl) {
                fprintf(ctx->output, " {\n");
            } else {
                fprintf(ctx->output, "\n");
                format_indent(ctx);
                fprintf(ctx->output, "{\n");
            }
            ctx->indent_level++;
            format_block(ctx, stmt->as.if_stmt.then_block);
            ctx->indent_level--;
            format_indent(ctx);
            if (stmt->as.if_stmt.else_block) {
                if (ctx->config->brace_on_same_line_ctrl) {
                    fprintf(ctx->output, "} 否则 {\n");
                } else {
                    fprintf(ctx->output, "}\n");
                    format_indent(ctx);
                    fprintf(ctx->output, "否则\n");
                    format_indent(ctx);
                    fprintf(ctx->output, "{\n");
                }
                ctx->indent_level++;
                format_block(ctx, stmt->as.if_stmt.else_block);
                ctx->indent_level--;
                format_indent(ctx);
            }
            fprintf(ctx->output, "}\n");
            break;
        }

        case CN_AST_STMT_WHILE: {
            format_indent(ctx);
            fprintf(ctx->output, "循环");
            if (ctx->config->space_after_keywords) {
                fprintf(ctx->output, " ");
            }
            fprintf(ctx->output, "(");
            format_expr(ctx, stmt->as.while_stmt.condition);
            fprintf(ctx->output, ")");
            if (ctx->config->brace_on_same_line_ctrl) {
                fprintf(ctx->output, " {\n");
            } else {
                fprintf(ctx->output, "\n");
                format_indent(ctx);
                fprintf(ctx->output, "{\n");
            }
            ctx->indent_level++;
            format_block(ctx, stmt->as.while_stmt.body);
            ctx->indent_level--;
            format_indent(ctx);
            fprintf(ctx->output, "}\n");
            break;
        }

        case CN_AST_STMT_FOR: {
            format_indent(ctx);
            fprintf(ctx->output, "对于");
            if (ctx->config->space_after_keywords) {
                fprintf(ctx->output, " ");
            }
            fprintf(ctx->output, "(");
            if (stmt->as.for_stmt.init) {
                // 对于 init 语句，不需要换行和缩进
                if (stmt->as.for_stmt.init->kind == CN_AST_STMT_VAR_DECL) {
                    fprintf(ctx->output, "变量 %.*s",
                            (int)stmt->as.for_stmt.init->as.var_decl.name_length,
                            stmt->as.for_stmt.init->as.var_decl.name);
                    if (stmt->as.for_stmt.init->as.var_decl.initializer) {
                        if (ctx->config->space_around_ops) {
                            fprintf(ctx->output, " = ");
                        } else {
                            fprintf(ctx->output, "=");
                        }
                        format_expr(ctx, stmt->as.for_stmt.init->as.var_decl.initializer);
                    }
                }
            }
            fprintf(ctx->output, ctx->config->space_after_comma ? "; " : ";");
            
            if (stmt->as.for_stmt.condition) {
                format_expr(ctx, stmt->as.for_stmt.condition);
            }
            fprintf(ctx->output, ctx->config->space_after_comma ? "; " : ";");
            
            if (stmt->as.for_stmt.update) {
                format_expr(ctx, stmt->as.for_stmt.update);
            }
            fprintf(ctx->output, ")");
            if (ctx->config->brace_on_same_line_ctrl) {
                fprintf(ctx->output, " {\n");
            } else {
                fprintf(ctx->output, "\n");
                format_indent(ctx);
                fprintf(ctx->output, "{\n");
            }
            ctx->indent_level++;
            format_block(ctx, stmt->as.for_stmt.body);
            ctx->indent_level--;
            format_indent(ctx);
            fprintf(ctx->output, "}\n");
            break;
        }

        case CN_AST_STMT_BREAK:
            format_indent(ctx);
            fprintf(ctx->output, "跳出;\n");
            break;

        case CN_AST_STMT_CONTINUE:
            format_indent(ctx);
            fprintf(ctx->output, "继续;\n");
            break;

        case CN_AST_STMT_BLOCK:
            format_indent(ctx);
            fprintf(ctx->output, "{\n");
            ctx->indent_level++;
            format_block(ctx, stmt->as.block);
            ctx->indent_level--;
            format_indent(ctx);
            fprintf(ctx->output, "}\n");
            break;

        default:
            format_indent(ctx);
            fprintf(ctx->output, "<?stmt?>;\n");
            break;
    }
}

// 格式化函数声明
static void format_function(FormatContext *ctx, const CnAstFunctionDecl *func)
{
    if (!func) {
        return;
    }

    // 函数签名
    fprintf(ctx->output, "函数 %.*s(",
            (int)func->name_length,
            func->name);

    // 参数列表
    for (size_t i = 0; i < func->parameter_count; i++) {
        if (i > 0) {
            fprintf(ctx->output, ctx->config->space_after_comma ? ", " : ",");
        }
        fprintf(ctx->output, "%.*s",
                (int)func->parameters[i].name_length,
                func->parameters[i].name);
    }
    fprintf(ctx->output, ")");

    // 函数体：根据配置决定大括号是否独立成行
    if (ctx->config->brace_on_new_line_func) {
        fprintf(ctx->output, "\n{\n");
    } else {
        fprintf(ctx->output, " {\n");
    }

    ctx->indent_level++;
    format_block(ctx, func->body);
    ctx->indent_level--;

    fprintf(ctx->output, "}\n");
}

// 格式化程序
bool cn_format_program_to_file(
    const CnAstProgram *program,
    FILE *output,
    const CnFormatConfig *config)
{
    if (!program || !output || !config) {
        return false;
    }

    FormatContext ctx = {
        .output = output,
        .config = config,
        .indent_level = 0,
        .error_occurred = false
    };

    for (size_t i = 0; i < program->function_count; i++) {
        format_function(&ctx, program->functions[i]);
        // 根据配置在函数之间添加空行
        if (config->empty_line_between_funcs && i < program->function_count - 1) {
            fprintf(output, "\n");
        }
    }

    return !ctx.error_occurred;
}

// 格式化程序到字符串（使用内存流）
char *cn_format_program_to_string(
    const CnAstProgram *program,
    const CnFormatConfig *config,
    size_t *out_length)
{
    FILE *memstream;
    char *buffer = NULL;
    size_t size = 0;

#ifdef _WIN32
    // Windows 不支持 open_memstream，使用临时文件
    char temp_path[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_path) == 0) {
        return NULL;
    }
    char temp_file[MAX_PATH];
    if (GetTempFileNameA(temp_path, "fmt", 0, temp_file) == 0) {
        return NULL;
    }
    memstream = fopen(temp_file, "w+b");
    if (!memstream) {
        return NULL;
    }
#else
    memstream = open_memstream(&buffer, &size);
    if (!memstream) {
        return NULL;
    }
#endif

    bool success = cn_format_program_to_file(program, memstream, config);

#ifdef _WIN32
    if (success) {
        fseek(memstream, 0, SEEK_END);
        size = ftell(memstream);
        fseek(memstream, 0, SEEK_SET);
        buffer = (char *)malloc(size + 1);
        if (buffer) {
            fread(buffer, 1, size, memstream);
            buffer[size] = '\0';
        }
    }
    fclose(memstream);
    remove(temp_file);
#else
    fclose(memstream);
#endif

    if (!success || !buffer) {
        free(buffer);
        return NULL;
    }

    if (out_length) {
        *out_length = size;
    }

    return buffer;
}

// 读取文件内容
static char *read_file(const char *filename, size_t *out_length)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, size, fp);
    buffer[read_size] = '\0';
    fclose(fp);

    if (out_length) {
        *out_length = read_size;
    }

    return buffer;
}

// 格式化文件
bool cn_format_file(const char *input_file, const CnFormatOptions *options)
{
    if (!input_file || !options) {
        return false;
    }

    // 读取源文件
    size_t source_length;
    char *source = read_file(input_file, &source_length);
    if (!source) {
        fprintf(stderr, "无法读取文件: %s\n", input_file);
        return false;
    }

    // 初始化诊断系统
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    // 词法分析
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, source_length, input_file);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 语法分析
    CnParser *parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "创建解析器失败\n");
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return false;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program || diagnostics.count > 0) {
        fprintf(stderr, "解析文件失败: %s\n", input_file);
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return false;
    }

    // 格式化
    FILE *output = NULL;
    bool success = false;

    if (options->to_stdout) {
        output = stdout;
    } else if (options->output_file) {
        output = fopen(options->output_file, "w");
    } else if (options->in_place) {
        char temp_file[1024];
        snprintf(temp_file, sizeof(temp_file), "%s.tmp", input_file);
        output = fopen(temp_file, "w");
    } else {
        output = stdout;
    }

    if (!output) {
        fprintf(stderr, "无法打开输出文件\n");
        goto cleanup;
    }

    success = cn_format_program_to_file(program, output, &options->config);

    if (output != stdout) {
        fclose(output);
    }

    // 原地修改：替换原文件
    if (success && options->in_place && !options->output_file) {
        char temp_file[1024];
        snprintf(temp_file, sizeof(temp_file), "%s.tmp", input_file);
#ifdef _WIN32
        remove(input_file);
#endif
        if (rename(temp_file, input_file) != 0) {
            fprintf(stderr, "无法替换原文件: %s\n", input_file);
            success = false;
        }
    }

cleanup:
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return success;
}

// 递归格式化目录
int cn_format_directory(const char *dir_path, const CnFormatOptions *options)
{
#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    HANDLE hFind;
    char search_path[1024];
    int count = 0;

    snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);
    hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        return -1;
    }

    do {
        if (strcmp(find_data.cFileName, ".") == 0 ||
            strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path, find_data.cFileName);

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            int subcount = cn_format_directory(full_path, options);
            if (subcount >= 0) {
                count += subcount;
            }
        } else {
            size_t len = strlen(find_data.cFileName);
            if (len > 3 && strcmp(find_data.cFileName + len - 3, ".cn") == 0) {
                if (cn_format_file(full_path, options)) {
                    count++;
                    printf("已格式化: %s\n", full_path);
                } else {
                    fprintf(stderr, "格式化失败: %s\n", full_path);
                }
            }
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
    return count;
#else
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return -1;
    }

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                int subcount = cn_format_directory(full_path, options);
                if (subcount >= 0) {
                    count += subcount;
                }
            } else {
                size_t len = strlen(entry->d_name);
                if (len > 3 && strcmp(entry->d_name + len - 3, ".cn") == 0) {
                    if (cn_format_file(full_path, options)) {
                        count++;
                        printf("已格式化: %s\n", full_path);
                    } else {
                        fprintf(stderr, "格式化失败: %s\n", full_path);
                    }
                }
            }
        }
    }

    closedir(dir);
    return count;
#endif
}
