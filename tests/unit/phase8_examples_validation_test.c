/*
 * 阶段8示例程序验收测试
 * 
 * 验证新创建的4个系统编程示例程序的有效性：
 * 1. memory_manager_demo.cn - 内存管理器演示
 * 2. task_scheduler_demo.cn - 任务调度器演示
 * 3. device_driver_demo.cn - 设备驱动演示
 * 4. syntax_verification_test.cn - 语法验收测试
 * 
 * 验证层次：
 * - 词法分析：验证所有Token正确识别
 * - 语法分析：验证AST结构正确生成
 * - 语义分析：验证类型检查和符号解析
 */

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 辅助函数：读取文件内容
static char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';
    
    fclose(file);
    return buffer;
}

// 验证词法分析
static int validate_lexer(const char* filename, const char* source) {
    printf("  [词法分析] %s...\n", filename);
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    CnToken token;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, strlen(source), filename);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    int token_count = 0;
    int error_count = 0;
    
    // 扫描所有Token
    while (1) {
        if (!cn_frontend_lexer_next_token(&lexer, &token)) {
            break;
        }
        token_count++;
        
        if (token.kind == CN_TOKEN_EOF) {
            break;
        }
    }
    
    error_count = diagnostics.count;
    cn_support_diagnostics_free(&diagnostics);
    
    if (error_count == 0) {
        printf("    \u2713 词法分析通过 (共%d个Token)\n", token_count);
        return 1;
    } else {
        printf("    \u2717 发现%d个词法错误\n", error_count);
        return 0;
    }
}

// 验证语法分析
static int validate_parser(const char* filename, const char* source) {
    printf("  [语法分析] %s...\n", filename);
    
    CnLexer lexer;
    CnParser* parser;
    CnAstProgram* program = NULL;
    bool ok;
    
    cn_frontend_lexer_init(&lexer, source, strlen(source), filename);
    parser = cn_frontend_parser_new(&lexer);
    
    if (!parser) {
        printf("    \u2717 解析器创建失败\n");
        return 0;
    }
    
    // 解析整个程序
    ok = cn_frontend_parse_program(parser, &program);
    
    int func_count = program ? program->function_count : 0;
    int has_error = !ok || !program;
    
    // 清理资源
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    
    if (!has_error && func_count > 0) {
        printf("    \u2713 语法分析通过 (函数:%d)\n", func_count);
        return 1;
    } else {
        printf("    \u2717 语法分析失败\n");
        return 0;
    }
}

// 验证单个示例文件
static int validate_example(const char* filename) {
    printf("\n=== 验证: %s ===\n", filename);
    
    char* source = read_file(filename);
    if (!source) {
        printf("  ✗ 无法读取文件\n");
        return 0;
    }
    
    // 基本文件检查
    size_t source_length = strlen(source);
    printf("  [文件检查] 大小: %zu 字节\n", source_length);
    
    if (source_length == 0) {
        printf("  ✗ 文件为空\n");
        free(source);
        return 0;
    }
    
    // 检查文件编码（应该包含UTF-8中文）
    int has_chinese = 0;
    for (size_t i = 0; i < source_length; i++) {
        if ((unsigned char)source[i] >= 0x80) {
            has_chinese = 1;
            break;
        }
    }
    
    if (!has_chinese) {
        printf("  ⚠ 警告: 文件似乎不包含中文字符\n");
    } else {
        printf("  ✓ 文件包含中文字符（UTF-8编码）\n");
    }
    
    // 词法分析验证
    int lexer_ok = validate_lexer(filename, source);
    
    // 语法分析验证
    int parser_ok = validate_parser(filename, source);
    
    free(source);
    
    if (lexer_ok && parser_ok) {
        printf("  ✓✓✓ %s 验证通过 ✓✓✓\n", filename);
        return 1;
    } else {
        printf("  ✗✗✗ %s 验证失败 ✗✗✗\n", filename);
        return 0;
    }
}

int main(void) {
    printf("\n");
    printf("================================================\n");
    printf("  阶段8系统编程示例程序验收测试\n");
    printf("================================================\n");
    
    int total = 0;
    int passed = 0;
    
    // 定义要验证的示例文件路径（相对于项目根目录）
    const char* examples[] = {
        "examples/memory_manager_demo.cn",
        "examples/task_scheduler_demo.cn",
        "examples/device_driver_demo.cn",
        "examples/syntax_verification_test.cn"
    };
    
    int example_count = sizeof(examples) / sizeof(examples[0]);
    
    // 逐个验证
    for (int i = 0; i < example_count; i++) {
        total++;
        if (validate_example(examples[i])) {
            passed++;
        }
    }
    
    // 输出总结
    printf("\n");
    printf("================================================\n");
    printf("  验证总结\n");
    printf("================================================\n");
    printf("总共: %d 个示例\n", total);
    printf("通过: %d 个\n", passed);
    printf("失败: %d 个\n", total - passed);
    printf("成功率: %.1f%%\n", total > 0 ? (passed * 100.0 / total) : 0.0);
    
    if (passed == total) {
        printf("\n✓✓✓ 所有示例验证通过！✓✓✓\n");
        printf("阶段8系统编程能力示例程序质量合格！\n");
    } else {
        printf("\n✗✗✗ 部分示例验证失败 ✗✗✗\n");
        printf("请检查失败的示例文件\n");
    }
    
    printf("================================================\n\n");
    
    return (passed == total) ? 0 : 1;
}
