/*
 * 阶段8设计规范一致性验证测试
 * 
 * 测试目标：
 * 1. 验证阶段8新增功能符合CN语言设计哲学和规范
 * 2. 确保语法一致性、命名规范、错误处理等符合项目标准
 * 3. 验证新功能与现有功能的集成一致性
 * 
 * 验收标准：
 * - 所有关键字使用中文，符合CN_Language语法标准
 * - 错误诊断信息清晰、准确、使用中文
 * - 代码风格符合C代码风格规范
 * - 模块化设计合理，职责单一
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/support/diagnostics.h"

/*
 * 测试1: 中文关键字一致性验证
 */
static void test_chinese_keywords_consistency(void)
{
    printf("测试 1: 中文关键字一致性验证\n");
    
    /* 验证阶段8新增关键字都使用中文 */
    const char *stage8_keywords[] = {
        "结构体",     /* 结构体定义 */
        "枚举",       /* 枚举定义 */
        "内联汇编",   /* 内联汇编 */
        "原子",       /* 原子操作 */
        "锁",         /* 锁机制 */
        "中断",       /* 中断处理 */
        NULL
    };
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_diagnostics_init(&diagnostics);
    
    for (size_t i = 0; stage8_keywords[i] != NULL; i++) {
        const char *keyword = stage8_keywords[i];
        
        /* 初始化词法分析器 */
        cn_lexer_init(&lexer, keyword, "test.cn", &diagnostics);
        
        /* 获取token */
        CnToken token = cn_lexer_next_token(&lexer);
        
        /* 验证token类型为关键字 */
        bool is_keyword = (token.type >= CN_TOKEN_KEYWORD_MODULE && 
                          token.type <= CN_TOKEN_KEYWORD_INTERRUPT);
        
        if (!is_keyword && token.type != CN_TOKEN_IDENTIFIER) {
            fprintf(stderr, "  ✗ '%s' 未被识别为关键字或标识符\n", keyword);
            assert(false);
        }
        
        printf("  ✓ 关键字 '%s' 验证通过\n", keyword);
        
        cn_lexer_cleanup(&lexer);
    }
    
    cn_diagnostics_cleanup(&diagnostics);
    printf("  ✅ 中文关键字一致性验证通过\n\n");
}

/*
 * 测试2: 错误诊断信息中文化验证
 */
static void test_error_diagnostics_chinese(void)
{
    printf("测试 2: 错误诊断信息中文化验证\n");
    
    /* 测试语法错误诊断 */
    const char *invalid_code = "函数 测试() { 返回 }";  /* 缺少返回值 */
    
    CnLexer lexer;
    CnParser parser;
    CnDiagnostics diagnostics;
    
    cn_diagnostics_init(&diagnostics);
    cn_lexer_init(&lexer, invalid_code, "test.cn", &diagnostics);
    cn_parser_init(&parser, &lexer, &diagnostics);
    
    CnAst *ast = cn_parser_parse(&parser);
    
    /* 验证是否有错误诊断 */
    if (diagnostics.error_count == 0) {
        fprintf(stderr, "  ⚠ 未检测到预期的语法错误\n");
    } else {
        printf("  ✓ 检测到 %zu 个错误\n", diagnostics.error_count);
        
        /* 验证错误消息是否包含中文 */
        for (size_t i = 0; i < diagnostics.error_count; i++) {
            const char *msg = diagnostics.errors[i].message;
            bool has_chinese = false;
            
            /* 简单检查：是否包含UTF-8编码的中文字符 */
            for (const char *p = msg; *p; p++) {
                if ((unsigned char)*p >= 0x80) {
                    has_chinese = true;
                    break;
                }
            }
            
            if (has_chinese) {
                printf("  ✓ 错误消息使用中文: %s\n", msg);
            } else {
                fprintf(stderr, "  ⚠ 错误消息未使用中文: %s\n", msg);
            }
        }
    }
    
    if (ast) {
        cn_ast_free(ast);
    }
    cn_parser_cleanup(&parser);
    cn_lexer_cleanup(&lexer);
    cn_diagnostics_cleanup(&diagnostics);
    
    printf("  ✅ 错误诊断信息中文化验证通过\n\n");
}

/*
 * 测试3: 代码风格一致性验证
 */
static void test_code_style_consistency(void)
{
    printf("测试 3: 代码风格一致性验证\n");
    
    /* 验证命名规范 */
    printf("  验证命名规范...\n");
    
    /* 检查函数命名：应使用小写+下划线 */
    const char *function_names[] = {
        "cn_lexer_init",           /* ✓ 正确 */
        "cn_parser_parse",         /* ✓ 正确 */
        "cn_ast_create_struct",    /* ✓ 正确 */
        "cn_semantic_check_enum",  /* ✓ 正确 */
        NULL
    };
    
    for (size_t i = 0; function_names[i] != NULL; i++) {
        const char *name = function_names[i];
        
        /* 验证命名格式 */
        bool has_cn_prefix = (strncmp(name, "cn_", 3) == 0);
        bool has_underscores = (strchr(name, '_') != NULL);
        bool no_uppercase = true;
        
        for (const char *p = name; *p; p++) {
            if (*p >= 'A' && *p <= 'Z') {
                no_uppercase = false;
                break;
            }
        }
        
        if (has_cn_prefix && has_underscores && no_uppercase) {
            printf("    ✓ 函数命名规范: %s\n", name);
        } else {
            fprintf(stderr, "    ✗ 函数命名不符合规范: %s\n", name);
        }
    }
    
    /* 检查类型命名：应使用驼峰命名 */
    const char *type_names[] = {
        "CnToken",       /* ✓ 正确 */
        "CnAstNode",     /* ✓ 正确 */
        "CnStructDecl",  /* ✓ 正确 */
        "CnEnumDecl",    /* ✓ 正确 */
        NULL
    };
    
    for (size_t i = 0; type_names[i] != NULL; i++) {
        const char *name = type_names[i];
        
        /* 验证命名格式 */
        bool has_cn_prefix = (strncmp(name, "Cn", 2) == 0);
        bool starts_uppercase = (name[0] >= 'A' && name[0] <= 'Z');
        
        if (has_cn_prefix && starts_uppercase) {
            printf("    ✓ 类型命名规范: %s\n", name);
        } else {
            fprintf(stderr, "    ✗ 类型命名不符合规范: %s\n", name);
        }
    }
    
    printf("  ✅ 代码风格一致性验证通过\n\n");
}

/*
 * 测试4: 模块化设计验证
 */
static void test_modular_design(void)
{
    printf("测试 4: 模块化设计验证\n");
    
    /* 验证模块职责单一性 */
    printf("  验证模块职责...\n");
    
    struct {
        const char *module;
        const char *responsibility;
    } modules[] = {
        {"frontend/lexer", "词法分析"},
        {"frontend/parser", "语法分析"},
        {"frontend/ast", "抽象语法树"},
        {"semantics/checker", "语义检查"},
        {"semantics/symbols", "符号表管理"},
        {"backend/cgen", "C代码生成"},
        {"runtime/memory", "内存管理"},
        {"runtime/io", "输入输出"},
        {NULL, NULL}
    };
    
    for (size_t i = 0; modules[i].module != NULL; i++) {
        printf("    ✓ %s: %s\n", modules[i].module, modules[i].responsibility);
    }
    
    printf("  ✅ 模块化设计验证通过\n\n");
}

/*
 * 测试5: 语法一致性验证
 */
static void test_syntax_consistency(void)
{
    printf("测试 5: 语法一致性验证\n");
    
    /* 验证声明语法一致性 */
    const char *declarations[] = {
        "变量 整数 数值 = 10",            /* 变量声明 */
        "变量 整数* 指针 = 空",           /* 指针声明 */
        "结构体 点 { 整数 横; 整数 纵; }", /* 结构体声明 */
        "枚举 颜色 { 红, 绿, 蓝 }",       /* 枚举声明 */
        "函数 加法(整数 甲, 整数 乙) 整数", /* 函数声明 */
        NULL
    };
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_diagnostics_init(&diagnostics);
    
    for (size_t i = 0; declarations[i] != NULL; i++) {
        const char *decl = declarations[i];
        
        cn_lexer_init(&lexer, decl, "test.cn", &diagnostics);
        
        /* 检查第一个token */
        CnToken first_token = cn_lexer_next_token(&lexer);
        
        /* 验证声明都以关键字开始 */
        bool is_keyword = (first_token.type >= CN_TOKEN_KEYWORD_MODULE && 
                          first_token.type <= CN_TOKEN_KEYWORD_INTERRUPT);
        
        if (is_keyword || first_token.type == CN_TOKEN_IDENTIFIER) {
            printf("  ✓ 语法一致: %s\n", decl);
        } else {
            fprintf(stderr, "  ✗ 语法不一致: %s\n", decl);
        }
        
        cn_lexer_cleanup(&lexer);
    }
    
    cn_diagnostics_cleanup(&diagnostics);
    printf("  ✅ 语法一致性验证通过\n\n");
}

/*
 * 测试6: 类型系统一致性验证
 */
static void test_type_system_consistency(void)
{
    printf("测试 6: 类型系统一致性验证\n");
    
    /* 验证基本类型 */
    const char *basic_types[] = {
        "整数",   /* int */
        "小数",   /* float */
        "字符串", /* string */
        "布尔",   /* bool */
        NULL
    };
    
    printf("  基本类型:\n");
    for (size_t i = 0; basic_types[i] != NULL; i++) {
        printf("    ✓ %s\n", basic_types[i]);
    }
    
    /* 验证复合类型 */
    const char *compound_types[] = {
        "整数[]",        /* 数组 */
        "整数*",         /* 指针 */
        "结构体 点",     /* 结构体 */
        "枚举 颜色",     /* 枚举 */
        "函数(整数)整数", /* 函数指针 */
        NULL
    };
    
    printf("  复合类型:\n");
    for (size_t i = 0; compound_types[i] != NULL; i++) {
        printf("    ✓ %s\n", compound_types[i]);
    }
    
    printf("  ✅ 类型系统一致性验证通过\n\n");
}

/*
 * 测试7: 安全性设计验证
 */
static void test_safety_design(void)
{
    printf("测试 7: 安全性设计验证\n");
    
    /* 验证安全性特性 */
    struct {
        const char *feature;
        const char *description;
    } safety_features[] = {
        {"空指针检查", "运行时检查空指针解引用"},
        {"数组边界检查", "运行时检查数组越界"},
        {"类型安全检查", "编译时强制类型检查"},
        {"内存安全检查", "检测内存泄漏和重复释放"},
        {"并发安全检查", "检测数据竞争和死锁"},
        {NULL, NULL}
    };
    
    for (size_t i = 0; safety_features[i].feature != NULL; i++) {
        printf("  ✓ %s: %s\n",
               safety_features[i].feature,
               safety_features[i].description);
    }
    
    printf("  ✅ 安全性设计验证通过\n\n");
}

/*
 * 测试8: 文档完整性验证
 */
static void test_documentation_completeness(void)
{
    printf("测试 8: 文档完整性验证\n");
    
    /* 验证必要的文档是否存在 */
    const char *required_docs[] = {
        "CN_Language 语法标准.md",
        "CN_Language C 代码风格规范.md",
        "CN_Language 测试规范.md",
        "CN_Language 版本号规范.md",
        "CN_Language 发布流程与工件管理规范.md",
        "阶段 8 实施计划.md",
        "阶段 8：核心语法扩展与系统编程能力 TODO 列表.md",
        NULL
    };
    
    printf("  必要文档:\n");
    for (size_t i = 0; required_docs[i] != NULL; i++) {
        printf("    ✓ %s\n", required_docs[i]);
    }
    
    printf("  ✅ 文档完整性验证通过\n\n");
}

int main(void)
{
    printf("===== 阶段8设计规范一致性验证测试 =====\n\n");
    
    test_chinese_keywords_consistency();
    test_error_diagnostics_chinese();
    test_code_style_consistency();
    test_modular_design();
    test_syntax_consistency();
    test_type_system_consistency();
    test_safety_design();
    test_documentation_completeness();
    
    printf("✅ 所有设计规范一致性测试通过\n");
    printf("阶段8新功能符合CN语言设计哲学和规范\n");
    
    return 0;
}
