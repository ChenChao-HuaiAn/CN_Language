#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/ir/ir.h"
#include "cnlang/ir/irgen.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/support/config.h"

#define REPL_VERSION "0.1.0"
#define MAX_INPUT_LINE 4096

/* REPL 配置结构 */
typedef struct {
    bool verbose;           // 详细输出模式
    bool show_ast;          // 是否显示 AST
    bool show_ir;           // 是否显示 IR
    const char *init_file;  // 初始化脚本文件（预留）
} ReplConfig;

/* 打印欢迎信息 */
static void print_welcome(void)
{
    printf("CN Language REPL v%s\n", REPL_VERSION);
    printf("输入 CN 语言表达式或语句，按 Ctrl+C 退出\n");
    printf("特殊命令:\n");
    printf("  :help    - 显示帮助信息\n");
    printf("  :quit    - 退出 REPL\n");
    printf("  :verbose - 切换详细输出模式\n");
    printf("  :ast     - 切换 AST 显示\n");
    printf("  :ir      - 切换 IR 显示\n");
    printf("\n");
}

/* 打印帮助信息 */
static void print_help(void)
{
    printf("\nCN Language REPL 帮助:\n");
    printf("--------------------\n");
    printf("  :help    - 显示此帮助信息\n");
    printf("  :quit    - 退出 REPL\n");
    printf("  :verbose - 切换详细输出模式\n");
    printf("  :ast     - 切换 AST 显示\n");
    printf("  :ir      - 切换 IR 显示\n");
    printf("\n");
    printf("直接输入 CN 语言代码执行:\n");
    printf("  例如: 变量 x = 10;\n");
    printf("       打印(x);\n");
    printf("\n");
}

/* 打印诊断信息 */
static void print_diagnostics(const CnDiagnostics *diagnostics)
{
    if (!diagnostics || diagnostics->count == 0) {
        return;
    }

    for (size_t i = 0; i < diagnostics->count; ++i) {
        const CnDiagnostic *d = &diagnostics->items[i];
        const char *severity_str = (d->severity == CN_DIAG_SEVERITY_ERROR) ? "错误" : "警告";
        const char *message = d->message ? d->message : "<无消息>";

        fprintf(stderr, "%s: %s (行 %d, 列 %d)\n",
                severity_str, message, d->line, d->column);
    }
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

/* 处理特殊命令 */
static bool handle_command(const char *line, ReplConfig *config)
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
static void process_input(const char *input, const ReplConfig *config)
{
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    CnTargetTriple target_triple;

    // 初始化诊断系统
    cn_support_diagnostics_init(&diagnostics);

    // 初始化词法分析器
    cn_frontend_lexer_init(&lexer, input, strlen(input), "<repl>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 创建语法分析器
    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "创建解析器失败\n");
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    // 解析程序
    bool ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        print_diagnostics(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }

    if (diagnostics_has_error(&diagnostics)) {
        print_diagnostics(&diagnostics);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }

    // 如果开启 AST 显示
    if (config->show_ast) {
        printf("=== AST ===\n");
        printf("函数数量: %zu\n", program->function_count);
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
        fprintf(stderr, "IR 生成失败\n");
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
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
    }

    // 清理资源
    cn_ir_module_free(ir_module);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

/* REPL 主循环 */
static void repl_loop(const ReplConfig *config)
{
    char line[MAX_INPUT_LINE];

    print_welcome();

    while (1) {
        printf("cn> ");
        fflush(stdout);

        // 读取一行输入
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        // 去除末尾换行符
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        // 跳过空行
        if (len == 0) {
            continue;
        }

        // 检查是否是特殊命令
        if (line[0] == ':') {
            if (!handle_command(line, (ReplConfig *)config)) {
                break; // 退出命令
            }
            continue;
        }

        // 处理代码输入
        process_input(line, config);
    }

    printf("再见!\n");
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
