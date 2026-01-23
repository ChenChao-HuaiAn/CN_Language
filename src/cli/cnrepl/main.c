#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>  // 为了 _isatty 和 _fileno
#endif

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/ir/ir.h"
#include "cnlang/ir/irgen.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/support/config.h"

#define REPL_VERSION "0.1.0"
#define MAX_INPUT_LINE 4096
#define MAX_MULTILINE_INPUT 16384  // 多行输入最大缓冲区

/* REPL 输入模式 */
typedef enum {
    REPL_INPUT_EXPRESSION,   // 单表达式（如：1 + 2）
    REPL_INPUT_STATEMENT,    // 单语句（如：变量 x = 10;）
    REPL_INPUT_PROGRAM       // 完整程序（包含函数定义）
} ReplInputMode;

/* REPL 配置结构 */
typedef struct {
    bool verbose;           // 详细输出模式
    bool show_ast;          // 是否显示 AST
    bool show_ir;           // 是否显示 IR
    const char *init_file;  // 初始化脚本文件（预留）
} ReplConfig;

/* REPL 会话状态：维护跨输入的符号表和 AST */
typedef struct {
    CnSemScope *global_scope;      // 全局作用域，保存所有已定义的变量和函数
    CnAstProgram *accumulated_ast; // 累积的程序 AST，包含所有已定义的函数
} ReplSession;

/* 初始化 REPL 会话 */
static ReplSession *repl_session_new(void)
{
    ReplSession *session = (ReplSession *)malloc(sizeof(ReplSession));
    if (!session) {
        return NULL;
    }

    // 创建全局作用域
    session->global_scope = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
    if (!session->global_scope) {
        free(session);
        return NULL;
    }

    // 创建累积的 AST（初始为空程序）
    session->accumulated_ast = (CnAstProgram *)malloc(sizeof(CnAstProgram));
    if (!session->accumulated_ast) {
        cn_sem_scope_free(session->global_scope);
        free(session);
        return NULL;
    }

    session->accumulated_ast->function_count = 0;
    session->accumulated_ast->functions = NULL;

    return session;
}

/* 销毁 REPL 会话 */
static void repl_session_free(ReplSession *session)
{
    if (!session) {
        return;
    }

    if (session->global_scope) {
        cn_sem_scope_free(session->global_scope);
    }

    if (session->accumulated_ast) {
        // 注意：不调用 cn_frontend_ast_program_free，因为函数指针可能已经被多次引用
        // 这里简单释放顶层结构
        if (session->accumulated_ast->functions) {
            free(session->accumulated_ast->functions);
        }
        free(session->accumulated_ast);
    }

    free(session);
}

/* 重置 REPL 会话状态 */
static void repl_session_reset(ReplSession *session)
{
    if (!session) {
        return;
    }

    // 释放旧的作用域
    if (session->global_scope) {
        cn_sem_scope_free(session->global_scope);
    }

    // 创建新的全局作用域
    session->global_scope = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);

    // 重置累积的 AST
    if (session->accumulated_ast) {
        if (session->accumulated_ast->functions) {
            free(session->accumulated_ast->functions);
        }
        session->accumulated_ast->function_count = 0;
        session->accumulated_ast->functions = NULL;
    }
}

/* 打印欢迎信息 */
static void print_welcome(void)
{
    printf("CN Language REPL v%s\n", REPL_VERSION);
    printf("输入 CN 语言表达式或语句，或使用特殊命令\n");
    printf("支持：单表达式求值、变量声明、小段程序\n");
    printf("会话状态：前次定义的变量/函数在后续输入中可见\n");
    printf("特殊命令:\n");
    printf("  :help/:h       - 显示帮助信息\n");
    printf("  :quit/:q       - 退出 REPL\n");
    printf("  :reset         - 重置会话状态\n");
    printf("  :verbose/:v    - 切换详细输出模式\n");
    printf("  :ast           - 切换 AST 显示\n");
    printf("  :ir            - 切换 IR 显示\n");
    printf("\n");
}

/* 打印帮助信息 */
static void print_help(void)
{
    printf("\nCN Language REPL 帮助:\n");
    printf("--------------------\n");
    printf("特殊命令：\n");
    printf("  :help/:h       - 显示此帮助信息\n");
    printf("  :quit/:q       - 退出 REPL\n");
    printf("  :reset         - 重置会话状态（清空所有已定义的变量和函数）\n");
    printf("  :verbose/:v    - 切换详细输出模式\n");
    printf("  :ast           - 切换 AST 显示\n");
    printf("  :ir            - 切换 IR 显示\n");
    printf("\n");
    printf("支持的输入模式:\n");
    printf("  1. 单表达式求值（自动包装）:\n");
    printf("     例如: 1 + 2 * 3\n");
    printf("           \"你好\" + \"世界\"\n");
    printf("  2. 变量声明和简单语句:\n");
    printf("     例如: 变量 x = 10;\n");
    printf("           打印(x);\n");
    printf("  3. 完整程序（包含函数）:\n");
    printf("     函数 主程序() { 打印(\"测试\"); 返回 0; }\n");
    printf("\n");
    printf("会话状态管理:\n");
    printf("  - 在一个会话中，前次定义的变量和函数在后续输入中可见\n");
    printf("  - 使用 :reset 命令可以清空会话状态，重新开始\n");
    printf("\n");
    printf("多行输入: 在行尾输入 \\ 继续下一行\n");
    printf("\n");
}

/* 打印诊断信息（带代码片段的友好提示） */
static void print_diagnostics_with_context(
    const CnDiagnostics *diagnostics,
    const char *source_code)
{
    if (!diagnostics || diagnostics->count == 0) {
        return;
    }

    for (size_t i = 0; i < diagnostics->count; ++i) {
        const CnDiagnostic *d = &diagnostics->items[i];
        const char *severity_str = (d->severity == CN_DIAG_SEVERITY_ERROR) ? "错误" : "警告";
        const char *message = d->message ? d->message : "<无消息>";

        // 打印错误消息头
        fprintf(stderr, "\n%s: %s\n", severity_str, message);
        fprintf(stderr, "  位置: 行 %d, 列 %d\n", d->line, d->column);

        // 显示代码片段（如果提供了源码）
        if (source_code && d->line > 0) {
            // 查找错误所在行
            const char *line_start = source_code;
            const char *line_end = source_code;
            int current_line = 1;

            // 定位到目标行
            while (*line_end && current_line < d->line) {
                if (*line_end == '\n') {
                    current_line++;
                    line_start = line_end + 1;
                }
                line_end++;
            }

            // 找到行尾
            line_end = line_start;
            while (*line_end && *line_end != '\n') {
                line_end++;
            }

            // 显示该行代码
            if (line_end > line_start) {
                fprintf(stderr, "  代码: ");
                fwrite(line_start, 1, line_end - line_start, stderr);
                fprintf(stderr, "\n");

                // 显示错误位置标记（用 ^ 符号）
                if (d->column > 0) {
                    fprintf(stderr, "        ");
                    for (int j = 1; j < d->column; j++) {
                        fprintf(stderr, " ");
                    }
                    fprintf(stderr, "^\n");
                }
            }
        }

        // 根据错误类型提供友好提示
        switch (d->code) {
        case CN_DIAG_CODE_LEX_UNTERMINATED_STRING:
            fprintf(stderr, "  提示: 字符串字面量需要用引号闭合\n");
            break;
        case CN_DIAG_CODE_LEX_INVALID_CHAR:
            fprintf(stderr, "  提示: 检查是否包含非法字符\n");
            break;
        case CN_DIAG_CODE_PARSE_EXPECTED_TOKEN:
            fprintf(stderr, "  提示: 检查语法结构是否完整\n");
            break;
        case CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER:
            fprintf(stderr, "  提示: 确保变量或函数在使用前已定义\n");
            break;
        case CN_DIAG_CODE_SEM_TYPE_MISMATCH:
            fprintf(stderr, "  提示: 检查表达式两边的类型是否匹配\n");
            break;
        default:
            break;
        }
    }
    fprintf(stderr, "\n");
}

/* 检查诊断中是否存在错误 */
static bool diagnostics_has_error(const CnDiagnostics *diagnostics)
{
    if (!diagnostics) {
        return false;
    }

    for (size_t i = 0; i < diagnostics->count; ++i) {
        if (diagnostics->items[i].severity == CN_DIAG_SEVERITY_ERROR) {
            return true;
        }
    }

    return false;
}

/* 检测输入的代码类型 */
static ReplInputMode detect_input_mode(const char *input)
{
    // 如果包含 "函数" 关键字，认为是完整程序
    if (strstr(input, "函数") != NULL) {
        return REPL_INPUT_PROGRAM;
    }

    // 如果以 变量/整数/字符串 等类型关键字开头，或者包含语句结构，认为是语句
    const char *stmt_keywords[] = {
        "变量", "整数", "字符串", "数组",
        "如果", "当", "循环", "返回", "打印"
    };

    for (size_t i = 0; i < sizeof(stmt_keywords) / sizeof(stmt_keywords[0]); ++i) {
        if (strstr(input, stmt_keywords[i]) != NULL) {
            // 检查是否包含分号或大括号（忽略末尾的空白字符）
            size_t len = strlen(input);
            for (size_t j = 0; j < len; j++) {
                if (input[j] == ';' || input[j] == '}') {
                    return REPL_INPUT_STATEMENT;
                }
            }
        }
    }

    // 默认认为是表达式
    return REPL_INPUT_EXPRESSION;
}

/* 将表达式包装为完整程序 */
static char *wrap_expression_as_program(const char *expr)
{
    // 包装格式：函数 主程序() { 打印(<表达式>); 返回 0; }
    size_t expr_len = strlen(expr);
    size_t wrapper_size = 256 + expr_len;
    char *wrapped = (char *)malloc(wrapper_size);

    if (!wrapped) {
        return NULL;
    }

    snprintf(wrapped, wrapper_size,
             "函数 主程序() { 打印(%s); 返回 0; }",
             expr);

    return wrapped;
}

/* 将单语句包装为完整程序 */
static char *wrap_statement_as_program(const char *stmt)
{
    // 包装格式：函数 主程序() { <语句> 返回 0; }
    size_t stmt_len = strlen(stmt);
    size_t wrapper_size = 256 + stmt_len;
    char *wrapped = (char *)malloc(wrapper_size);

    if (!wrapped) {
        return NULL;
    }

    snprintf(wrapped, wrapper_size,
             "函数 主程序() { %s 返回 0; }",
             stmt);

    return wrapped;
}

/* 处理特殊命令 */
static bool handle_command(const char *line, ReplConfig *config, ReplSession *session)
{
    // 去除前后空白
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    if (strcmp(line, ":quit") == 0 || strcmp(line, ":q") == 0) {
        return false; // 返回 false 表示退出
    }

    if (strcmp(line, ":help") == 0 || strcmp(line, ":h") == 0) {
        print_help();
        return true;
    }

    if (strcmp(line, ":reset") == 0) {
        repl_session_reset(session);
        printf("会话状态已重置\n");
        return true;
    }

    if (strcmp(line, ":verbose") == 0 || strcmp(line, ":v") == 0) {
        config->verbose = !config->verbose;
        printf("详细输出模式: %s\n", config->verbose ? "开启" : "关闭");
        return true;
    }

    if (strcmp(line, ":ast") == 0) {
        config->show_ast = !config->show_ast;
        printf("AST 显示: %s\n", config->show_ast ? "开启" : "关闭");
        return true;
    }

    if (strcmp(line, ":ir") == 0) {
        config->show_ir = !config->show_ir;
        printf("IR 显示: %s\n", config->show_ir ? "开启" : "关闭");
        return true;
    }

    printf("未知命令: %s (输入 :help 查看帮助)\n", line);
    return true;
}

/* 处理用户输入的代码 */
static void process_input(const char *input, const ReplConfig *config, ReplSession *session)
{
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    CnTargetTriple target_triple;
    char *code_to_parse = NULL;
    bool need_free_code = false;

    // 检测输入模式
    ReplInputMode mode = detect_input_mode(input);

    // 根据模式准备要解析的代码
    switch (mode) {
    case REPL_INPUT_EXPRESSION:
        if (config->verbose) {
            printf("[模式: 表达式求值]\n");
        }
        code_to_parse = wrap_expression_as_program(input);
        need_free_code = true;
        break;

    case REPL_INPUT_STATEMENT:
        if (config->verbose) {
            printf("[模式: 语句执行]\n");
        }
        code_to_parse = wrap_statement_as_program(input);
        need_free_code = true;
        break;

    case REPL_INPUT_PROGRAM:
        if (config->verbose) {
            printf("[模式: 完整程序]\n");
        }
        code_to_parse = (char *)input;  // 直接使用原始输入
        need_free_code = false;
        break;
    }

    if (!code_to_parse) {
        // 使用诊断系统报告内部错误
        CnDiagnostics temp_diag;
        cn_support_diagnostics_init(&temp_diag);
        cn_support_diagnostics_report_error(
            &temp_diag,
            CN_DIAG_CODE_UNKNOWN,
            "<repl>",
            0, 0,
            "内存分配失败");
        cn_support_diagnostics_print(&temp_diag);
        cn_support_diagnostics_free(&temp_diag);
        return;
    }

    // 初始化诊断系统
    cn_support_diagnostics_init(&diagnostics);

    // 初始化词法分析器
    cn_frontend_lexer_init(&lexer, code_to_parse, strlen(code_to_parse), "<repl>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 创建语法分析器
    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        // 使用诊断系统报告内部错误
        cn_support_diagnostics_report_error(
            &diagnostics,
            CN_DIAG_CODE_UNKNOWN,
            "<repl>",
            0, 0,
            "创建解析器失败");
        cn_support_diagnostics_print(&diagnostics);
        cn_support_diagnostics_free(&diagnostics);
        if (need_free_code) {
            free(code_to_parse);
        }
        return;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    // 解析程序
    bool ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        print_diagnostics_with_context(&diagnostics, code_to_parse);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        if (need_free_code) {
            free(code_to_parse);
        }
        return;
    }

    if (diagnostics_has_error(&diagnostics)) {
        print_diagnostics_with_context(&diagnostics, code_to_parse);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        if (need_free_code) {
            free(code_to_parse);
        }
        return;
    }

    // 将新定义的函数和变量添加到会话状态
    // 注意：这里简化处理，假设新解析的 program 包含需要追加的函数
    if (program->function_count > 0) {
        // 扩展累积的 AST 函数列表
        size_t old_count = session->accumulated_ast->function_count;
        size_t new_count = old_count + program->function_count;
        
        CnAstFunctionDecl **new_functions = (CnAstFunctionDecl **)realloc(
            session->accumulated_ast->functions,
            sizeof(CnAstFunctionDecl *) * new_count
        );
        
        if (new_functions) {
            session->accumulated_ast->functions = new_functions;
            
            // 追加新函数
            for (size_t i = 0; i < program->function_count; i++) {
                session->accumulated_ast->functions[old_count + i] = program->functions[i];
                
                // 在全局作用域中注册函数符号
                const char *func_name = program->functions[i]->name;
                size_t name_len = strlen(func_name);
                cn_sem_scope_insert_symbol(
                    session->global_scope,
                    func_name,
                    name_len,
                    CN_SEM_SYMBOL_FUNCTION
                );
            }
            
            session->accumulated_ast->function_count = new_count;
            
            if (config->verbose) {
                printf("[会话状态: 已注册 %zu 个新函数，当前共 %zu 个函数]\n",
                       program->function_count, new_count);
            }
        }
    }

    // 如果开启 AST 显示
    if (config->show_ast) {
        printf("=== AST ===\n");
        printf("函数数量: %zu\n", program->function_count);
        printf("会话累积函数数: %zu\n", session->accumulated_ast->function_count);
        printf("===========\n");
    }

    // 生成 IR（使用默认目标）
    target_triple = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);

    CnIrModule *ir_module = cn_ir_gen_program(program, target_triple, CN_COMPILE_MODE_HOSTED);
    if (!ir_module) {
        // 使用诊断系统报告 IR 生成错误
        cn_support_diagnostics_report_error(
            &diagnostics,
            CN_DIAG_CODE_UNKNOWN,
            "<repl>",
            0, 0,
            "IR 生成失败");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        if (need_free_code) {
            free(code_to_parse);
        }
        return;
    }

    // 如果开启 IR 显示
    if (config->show_ir) {
        printf("=== IR ===\n");
        cn_ir_dump_module(ir_module);
        printf("==========\n");
    }

    if (config->verbose) {
        printf("✓ 解析和编译成功\n");
    } else {
        // 在非详细模式下，表达式和语句模式提示执行成功
        if (mode == REPL_INPUT_EXPRESSION || mode == REPL_INPUT_STATEMENT) {
            printf("✓\n");
        }
    }

    // 清理资源
    cn_ir_module_free(ir_module);
    // 注意：不释放 program 中的函数，因为它们已经被添加到 session->accumulated_ast
    // 只释放 program 的顶层结构
    if (program->functions) {
        // 不要 free(program->functions)，因为指针已被会话保存
        program->functions = NULL;
    }
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    if (need_free_code) {
        free(code_to_parse);
    }
}

/* REPL 主循环 */
static void repl_loop(const ReplConfig *config)
{
    char line[MAX_INPUT_LINE];
    char multiline_buffer[MAX_MULTILINE_INPUT];  // 多行输入缓冲区
    bool in_multiline = false;

    // 创建 REPL 会话
    ReplSession *session = repl_session_new();
    if (!session) {
        // 使用诊断系统报告会话创建错误
        CnDiagnostics temp_diag;
        cn_support_diagnostics_init(&temp_diag);
        cn_support_diagnostics_report_error(
            &temp_diag,
            CN_DIAG_CODE_UNKNOWN,
            "<repl>",
            0, 0,
            "创建 REPL 会话失败");
        cn_support_diagnostics_print(&temp_diag);
        cn_support_diagnostics_free(&temp_diag);
        return;
    }

#ifdef _WIN32
    // 在 Windows 上使用宽字符读取以正确处理 Unicode
    wchar_t wline[MAX_INPUT_LINE];
    bool use_wide_input = _isatty(_fileno(stdin));
#endif

    print_welcome();

    while (1) {
        // 根据是否在多行模式显示不同的提示符
        if (in_multiline) {
            printf("... ");
        } else {
            printf("cn> ");
        }
        fflush(stdout);

#ifdef _WIN32
        // Windows: 如果是交互式终端，直接使用 Windows Console API
        if (use_wide_input) {
            HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
            DWORD charsRead = 0;
            
            if (!ReadConsoleW(hStdin, wline, MAX_INPUT_LINE - 1, &charsRead, NULL)) {
                printf("\n");
                break;
            }
            
            wline[charsRead] = L'\0';
            
            // DEBUG: 打印宽字符
            if (config->verbose) {
                printf("[DEBUG] 宽字符: ");
                for (size_t i = 0; i < wcslen(wline) && i < 20; i++) {
                    printf("%04X ", (unsigned int)wline[i]);
                }
                printf("\n");
            }
            
            // 转换 UTF-16 到 UTF-8
            int len = WideCharToMultiByte(CP_UTF8, 0, wline, -1, line, MAX_INPUT_LINE, NULL, NULL);
            if (len <= 0) {
                // 使用诊断系统报告编码转换错误
                CnDiagnostics temp_diag;
                cn_support_diagnostics_init(&temp_diag);
                cn_support_diagnostics_report_error(
                    &temp_diag,
                    CN_DIAG_CODE_UNKNOWN,
                    "<repl>",
                    0, 0,
                    "编码转换失败");
                cn_support_diagnostics_print(&temp_diag);
                cn_support_diagnostics_free(&temp_diag);
                continue;
            }
        } else {
            // 管道输入，使用普通 fgets
            if (!fgets(line, sizeof(line), stdin)) {
                printf("\n");
                break;
            }
        }
#else
        // 读取一行输入
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
#endif

        // DEBUG: 打印读取到的字节（前50个字符的十六进制）
        if (config->verbose) {
            printf("[DEBUG] 读取到的字节: ");
            for (size_t i = 0; i < strlen(line) && i < 50; i++) {
                printf("%02X ", (unsigned char)line[i]);
            }
            printf("\n");
        }

        // 去除末尾换行符
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        // 如果不在多行模式，跳过空行
        if (!in_multiline && len == 0) {
            continue;
        }

        // 检查是否是特殊命令（只在非多行模式下）
        if (!in_multiline && line[0] == ':') {
            if (!handle_command(line, (ReplConfig *)config, session)) {
                break; // 退出命令
            }
            continue;
        }

        // 检查是否以反斜杠结尾（表示继续下一行）
        bool continue_next_line = false;
        if (len > 0 && line[len - 1] == '\\') {
            line[len - 1] = '\0';  // 移除反斜杠
            len--;
            continue_next_line = true;
        }

        // 处理多行输入
        if (!in_multiline) {
            // 开始新的输入
            if (continue_next_line) {
                // 进入多行模式
                strncpy(multiline_buffer, line, MAX_MULTILINE_INPUT - 1);
                multiline_buffer[MAX_MULTILINE_INPUT - 1] = '\0';
                in_multiline = true;
            } else {
                // 单行输入，直接处理
                process_input(line, config, session);
            }
        } else {
            // 在多行模式中，继续追加内容
            size_t current_len = strlen(multiline_buffer);
            if (current_len + len + 2 < MAX_MULTILINE_INPUT) {
                strncat(multiline_buffer, " ", MAX_MULTILINE_INPUT - current_len - 1);
                strncat(multiline_buffer, line, MAX_MULTILINE_INPUT - current_len - 2);
            } else {
                // 使用诊断系统报告多行输入超长错误
                CnDiagnostics temp_diag;
                cn_support_diagnostics_init(&temp_diag);
                cn_support_diagnostics_report_error(
                    &temp_diag,
                    CN_DIAG_CODE_UNKNOWN,
                    "<repl>",
                    0, 0,
                    "错误：多行输入超过最大长度");
                cn_support_diagnostics_print(&temp_diag);
                cn_support_diagnostics_free(&temp_diag);
                in_multiline = false;
                multiline_buffer[0] = '\0';
                continue;
            }

            if (!continue_next_line) {
                // 结束多行输入，处理完整内容
                process_input(multiline_buffer, config, session);
                in_multiline = false;
                multiline_buffer[0] = '\0';
            }
        }
    }

    printf("再见!\n");
    
    // 释放会话
    repl_session_free(session);
}

/* 打印使用说明 */
static void print_usage(const char *prog_name)
{
    printf("用法: %s [选项]\n", prog_name);
    printf("\n选项:\n");
    printf("  -h, --help       显示此帮助信息\n");
    printf("  -v, --verbose    启用详细输出模式\n");
    printf("  -i, --init <文件> 从文件加载初始化脚本（暂未实现）\n");
    printf("  --version        显示版本信息\n");
    printf("\n");
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    // 设置 Windows 控制台为 UTF-8 模式
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    ReplConfig config = {
        .verbose = false,
        .show_ast = false,
        .show_ir = false,
        .init_file = NULL
    };

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("CN Language REPL v%s\n", REPL_VERSION);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            config.verbose = true;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--init") == 0) {
            if (i + 1 < argc) {
                config.init_file = argv[++i];
                printf("注意: 初始化脚本功能暂未实现\n");
            } else {
                fprintf(stderr, "错误: -i/--init 需要指定文件路径\n");
                return 1;
            }
        } else {
            fprintf(stderr, "未知参数: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // 启动 REPL 主循环
    repl_loop(&config);

    return 0;
}
