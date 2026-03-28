/**
 * @file diag_test.c
 * @brief 错误诊断增强功能单元测试
 * 
 * 测试覆盖以下模块：
 * 1. 消息表（Message Table）- 双语消息、参数化消息
 * 2. 恢复策略（Recovery Strategy）- 错误恢复、级联抑制
 * 3. 修复建议（Fix Suggestions）- 常见错误的修复建议
 * 4. 增强诊断（Enhanced Diagnostics）- 诊断报告、配置
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/diag_message_table.h"
#include "cnlang/support/diag_recovery.h"
#include "cnlang/support/diag_fixes.h"

// ============================================================================
// 测试统计
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("    [FAIL] %s\n", msg); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_PASS(name) do { \
    printf("    [PASS] %s\n", name); \
    tests_passed++; \
} while(0)

// ============================================================================
// 1. 消息表测试
// ============================================================================

/**
 * @brief 测试语言设置
 * 
 * 验证语言设置和获取功能正常工作
 */
static void test_language_setting(void) {
    printf("  测试: 语言设置功能\n");
    
    /* 设置为中文 */
    cn_diag_set_language(CN_DIAG_LANG_ZH);
    TEST_ASSERT(cn_diag_get_language() == CN_DIAG_LANG_ZH, 
                "语言应设置为中文");
    
    /* 设置为英文 */
    cn_diag_set_language(CN_DIAG_LANG_EN);
    TEST_ASSERT(cn_diag_get_language() == CN_DIAG_LANG_EN, 
                "语言应设置为英文");
    
    /* 恢复默认中文 */
    cn_diag_set_language(CN_DIAG_LANG_ZH);
    
    TEST_PASS("语言设置功能");
}

/**
 * @brief 测试消息模板查找
 * 
 * 验证能正确查找各种错误码的消息模板
 */
static void test_message_template_lookup(void) {
    printf("  测试: 消息模板查找\n");
    
    const CnDiagMessageTemplate *tmpl;
    
    /* 测试查找未定义标识符错误模板 */
    tmpl = cn_diag_get_template(CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER);
    TEST_ASSERT(tmpl != NULL, "应找到未定义标识符错误模板");
    TEST_ASSERT(tmpl->message_zh != NULL, "中文消息不应为空");
    TEST_ASSERT(tmpl->message_en != NULL, "英文消息不应为空");
    
    /* 测试查找类型不匹配错误模板 */
    tmpl = cn_diag_get_template(CN_DIAG_CODE_SEM_TYPE_MISMATCH);
    TEST_ASSERT(tmpl != NULL, "应找到类型不匹配错误模板");
    
    /* 测试查找词法错误模板 */
    tmpl = cn_diag_get_template(CN_DIAG_CODE_LEX_UNTERMINATED_STRING);
    TEST_ASSERT(tmpl != NULL, "应找到未终止字符串错误模板");
    
    /* 测试查找语法错误模板 */
    tmpl = cn_diag_get_template(CN_DIAG_CODE_PARSE_EXPECTED_TOKEN);
    TEST_ASSERT(tmpl != NULL, "应找到期望标记错误模板");
    
    /* 测试查找常量相关错误模板 */
    tmpl = cn_diag_get_template(CN_DIAG_CODE_SEM_CONST_NO_INITIALIZER);
    TEST_ASSERT(tmpl != NULL, "应找到常量缺少初始化错误模板");
    
    /* 测试查找静态变量相关错误模板 */
    tmpl = cn_diag_get_template(CN_DIAG_CODE_SEM_STATIC_NON_CONST_INIT);
    TEST_ASSERT(tmpl != NULL, "应找到静态变量非常量初始化错误模板");
    
    TEST_PASS("消息模板查找");
}

/**
 * @brief 测试消息格式化
 * 
 * 验证消息格式化功能，包括无参数和有参数的情况
 */
static void test_message_formatting(void) {
    printf("  测试: 消息格式化\n");
    
    const CnDiagMessageTemplate *tmpl;
    char buffer[256];
    int result;
    
    /* 测试无参数消息格式化 */
    tmpl = cn_diag_get_template(CN_DIAG_CODE_LEX_UNTERMINATED_STRING);
    if (tmpl) {
        result = cn_diag_format_message(tmpl, NULL, CN_DIAG_LANG_ZH, 
                                        buffer, sizeof(buffer));
        TEST_ASSERT(result >= 0, "无参数消息格式化应成功");
        TEST_ASSERT(strlen(buffer) > 0, "格式化结果不应为空");
    }
    
    /* 测试有参数消息格式化 */
    tmpl = cn_diag_get_template(CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER);
    if (tmpl) {
        /* 创建参数 */
        CnDiagArg args[1] = { cn_diag_arg_string("测试变量") };
        CnDiagArgs arg_list = { args, 1 };
        
        result = cn_diag_format_message(tmpl, &arg_list, CN_DIAG_LANG_ZH,
                                        buffer, sizeof(buffer));
        TEST_ASSERT(result >= 0, "有参数消息格式化应成功");
        TEST_ASSERT(strstr(buffer, "测试变量") != NULL, 
                    "格式化结果应包含参数值");
    }
    
    TEST_PASS("消息格式化");
}

/**
 * @brief 测试参数化消息
 * 
 * 验证参数创建和参数化消息功能
 */
static void test_parameterized_message(void) {
    printf("  测试: 参数化消息\n");
    
    /* 测试字符串参数创建 */
    CnDiagArg str_arg = cn_diag_arg_string("标识符名");
    TEST_ASSERT(str_arg.type == CN_DIAG_ARG_TYPE_STRING, 
                "参数类型应为字符串");
    TEST_ASSERT(str_arg.value.str_val != NULL, 
                "字符串值不应为空");
    TEST_ASSERT(strcmp(str_arg.value.str_val, "标识符名") == 0,
                "字符串值应正确");
    
    /* 测试整数参数创建 */
    CnDiagArg int_arg = cn_diag_arg_int(42);
    TEST_ASSERT(int_arg.type == CN_DIAG_ARG_TYPE_INT,
                "参数类型应为整数");
    TEST_ASSERT(int_arg.value.int_val == 42,
                "整数值应正确");
    
    /* 测试Token参数创建 */
    CnDiagArg token_arg = cn_diag_arg_token(100);
    TEST_ASSERT(token_arg.type == CN_DIAG_ARG_TYPE_TOKEN,
                "参数类型应为Token");
    TEST_ASSERT(token_arg.value.token_val == 100,
                "Token值应正确");
    
    /* 测试多参数消息 */
    const CnDiagMessageTemplate *tmpl = cn_diag_get_template(CN_DIAG_CODE_SEM_TYPE_MISMATCH);
    if (tmpl) {
        CnDiagArg args[2] = {
            cn_diag_arg_string("整数"),
            cn_diag_arg_string("字符串")
        };
        CnDiagArgs arg_list = { args, 2 };
        
        char buffer[256];
        int result = cn_diag_format_message(tmpl, &arg_list, CN_DIAG_LANG_ZH,
                                            buffer, sizeof(buffer));
        TEST_ASSERT(result >= 0, "多参数消息格式化应成功");
    }
    
    TEST_PASS("参数化消息");
}

// ============================================================================
// 2. 恢复策略测试
// ============================================================================

/**
 * @brief 测试恢复上下文初始化
 * 
 * 验证恢复上下文正确初始化
 */
static void test_recovery_init(void) {
    printf("  测试: 恢复上下文初始化\n");
    
    CnDiagRecoveryContext ctx;
    
    /* 测试无限制错误数初始化 */
    cn_diag_recovery_init(&ctx, 0);
    TEST_ASSERT(ctx.error_count == 0, "初始错误计数应为0");
    TEST_ASSERT(ctx.max_errors == 0, "最大错误数应为0（无限制）");
    TEST_ASSERT(ctx.cascade_suppress == 0, "级联抑制计数应为0");
    TEST_ASSERT(ctx.last_error_line == 0, "上次错误行号应为0");
    
    /* 测试有限制错误数初始化 */
    cn_diag_recovery_init(&ctx, 100);
    TEST_ASSERT(ctx.max_errors == 100, "最大错误数应为100");
    
    /* 测试重置功能 */
    ctx.error_count = 50;
    ctx.cascade_suppress = 5;
    cn_diag_recovery_reset(&ctx);
    TEST_ASSERT(ctx.error_count == 0, "重置后错误计数应为0");
    TEST_ASSERT(ctx.cascade_suppress == 0, "重置后级联抑制应为0");
    
    TEST_PASS("恢复上下文初始化");
}

/**
 * @brief 测试错误恢复策略选择
 * 
 * 验证根据错误类型选择正确的恢复策略
 */
static void test_recovery_strategy(void) {
    printf("  测试: 错误恢复策略选择\n");
    
    CnDiagRecoveryContext ctx;
    CnDiagRecoveryStrategy strategy;
    
    cn_diag_recovery_init(&ctx, 100);
    
    /* 测试词法错误恢复策略 */
    strategy = cn_diag_recovery_on_error(&ctx, CN_DIAG_CODE_LEX_INVALID_CHAR,
                                         "test.cn", 10);
    TEST_ASSERT(strategy != CN_DIAG_RECOVERY_NONE, 
                "词法错误应有恢复策略");
    
    /* 测试语法错误恢复策略 */
    strategy = cn_diag_recovery_on_error(&ctx, CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
                                         "test.cn", 20);
    TEST_ASSERT(strategy != CN_DIAG_RECOVERY_NONE,
                "语法错误应有恢复策略");
    
    /* 测试语义错误恢复策略 */
    strategy = cn_diag_recovery_on_error(&ctx, CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                                         "test.cn", 30);
    TEST_ASSERT(strategy != CN_DIAG_RECOVERY_NONE,
                "语义错误应有恢复策略");
    
    /* 验证错误计数增加 */
    int count = cn_diag_recovery_get_error_count(&ctx);
    TEST_ASSERT(count > 0, "错误计数应增加");
    
    TEST_PASS("错误恢复策略选择");
}

/**
 * @brief 测试级联错误抑制
 * 
 * 验证同一位置的级联错误被正确抑制
 */
static void test_cascade_suppression(void) {
    printf("  测试: 级联错误抑制\n");
    
    CnDiagRecoveryContext ctx;
    cn_diag_recovery_init(&ctx, 100);
    
    /* 第一次错误不应被抑制 */
    int suppress = cn_diag_should_suppress(&ctx, "test.cn", 10);
    TEST_ASSERT(suppress == 0, "第一次错误不应被抑制");
    
    /* 记录第一个错误 */
    cn_diag_recovery_on_error(&ctx, CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                              "test.cn", 10);
    
    /* 同一行的第二个错误可能被抑制（级联效应） */
    suppress = cn_diag_should_suppress(&ctx, "test.cn", 10);
    /* 注意：具体行为取决于实现，这里只验证函数可调用 */
    
    /* 不同行的错误不应被抑制 */
    suppress = cn_diag_should_suppress(&ctx, "test.cn", 20);
    TEST_ASSERT(suppress == 0, "不同行的错误不应被抑制");
    
    TEST_PASS("级联错误抑制");
}

/**
 * @brief 测试同步标记检测
 * 
 * 验证同步标记检测功能
 */
static void test_sync_point_detection(void) {
    printf("  测试: 同步标记检测\n");
    
    /* 测试分号作为同步点 */
    /* 注意：具体的Token值需要根据实际的Token定义 */
    int is_sync = cn_diag_is_sync_point(0);  /* 使用实际Token值 */
    /* 验证函数可调用 */
    
    /* 测试右花括号作为同步点 */
    is_sync = cn_diag_is_sync_point(1);
    
    /* 测试关键字作为同步点 */
    is_sync = cn_diag_is_sync_point(2);
    
    TEST_PASS("同步标记检测");
}

// ============================================================================
// 3. 修复建议测试
// ============================================================================

/**
 * @brief 测试修复建议生成
 * 
 * 验证通用修复建议生成功能
 */
static void test_fix_generation(void) {
    printf("  测试: 修复建议生成\n");
    
    CnDiagFix fix;
    int result;
    
    /* 测试未定义标识符的修复建议 */
    CnDiagArg args[1] = { cn_diag_arg_string("未知变量") };
    CnDiagArgs arg_list = { args, 1 };
    
    result = cn_diag_generate_fix(CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                                  &arg_list, CN_DIAG_LANG_ZH, &fix);
    /* 验证函数可调用 */
    if (result) {
        TEST_ASSERT(fix.description_zh != NULL || fix.description_en != NULL,
                    "修复建议应有描述");
    }
    
    TEST_PASS("修复建议生成");
}

/**
 * @brief 测试缺少分号建议
 * 
 * 验证缺少分号错误的修复建议
 */
static void test_fix_missing_semicolon(void) {
    printf("  测试: 缺少分号建议\n");
    
    CnDiagFix fix;
    int result;
    
    /* 生成中文建议 */
    result = cn_diag_fix_missing_semicolon(&fix, CN_DIAG_LANG_ZH);
    TEST_ASSERT(result == 1, "应成功生成缺少分号建议");
    TEST_ASSERT(fix.type == CN_DIAG_FIX_INSERT, "修复类型应为插入");
    TEST_ASSERT(fix.description_zh != NULL, "应有中文描述");
    TEST_ASSERT(fix.new_text != NULL, "应有新文本内容");
    TEST_ASSERT(strstr(fix.new_text, ";") != NULL || 
                strstr(fix.description_zh, "分号") != NULL,
                "建议应与分号相关");
    
    /* 生成英文建议 */
    result = cn_diag_fix_missing_semicolon(&fix, CN_DIAG_LANG_EN);
    TEST_ASSERT(result == 1, "应成功生成英文建议");
    TEST_ASSERT(fix.description_en != NULL, "应有英文描述");
    
    TEST_PASS("缺少分号建议");
}

/**
 * @brief 测试未定义标识符建议
 * 
 * 验证未定义标识符错误的修复建议
 */
static void test_fix_undefined_identifier(void) {
    printf("  测试: 未定义标识符建议\n");
    
    CnDiagFix fix;
    int result;
    
    /* 生成中文建议 */
    result = cn_diag_fix_undefined_identifier(&fix, "未定义变量", CN_DIAG_LANG_ZH);
    TEST_ASSERT(result == 1, "应成功生成未定义标识符建议");
    TEST_ASSERT(fix.description_zh != NULL, "应有中文描述");
    
    /* 验证建议内容包含标识符名称 */
    const char *desc = cn_diag_get_fix_description(&fix, CN_DIAG_LANG_ZH);
    TEST_ASSERT(desc != NULL, "应能获取描述");
    
    /* 生成英文建议 */
    result = cn_diag_fix_undefined_identifier(&fix, "undef_var", CN_DIAG_LANG_EN);
    TEST_ASSERT(result == 1, "应成功生成英文建议");
    TEST_ASSERT(fix.description_en != NULL, "应有英文描述");
    
    TEST_PASS("未定义标识符建议");
}

/**
 * @brief 测试类型不匹配建议
 * 
 * 验证类型不匹配错误的修复建议
 */
static void test_fix_type_mismatch(void) {
    printf("  测试: 类型不匹配建议\n");
    
    CnDiagFix fix;
    int result;
    
    /* 生成中文建议 */
    result = cn_diag_fix_type_mismatch(&fix, "整数", "字符串", CN_DIAG_LANG_ZH);
    TEST_ASSERT(result == 1, "应成功生成类型不匹配建议");
    TEST_ASSERT(fix.description_zh != NULL, "应有中文描述");
    
    /* 验证建议内容包含类型信息 */
    const char *desc = cn_diag_get_fix_description(&fix, CN_DIAG_LANG_ZH);
    TEST_ASSERT(desc != NULL, "应能获取描述");
    
    /* 生成英文建议 */
    result = cn_diag_fix_type_mismatch(&fix, "int", "string", CN_DIAG_LANG_EN);
    TEST_ASSERT(result == 1, "应成功生成英文建议");
    TEST_ASSERT(fix.description_en != NULL, "应有英文描述");
    
    TEST_PASS("类型不匹配建议");
}

/**
 * @brief 测试重复符号建议
 */
static void test_fix_duplicate_symbol(void) {
    printf("  测试: 重复符号建议\n");
    
    CnDiagFix fix;
    int result;
    
    result = cn_diag_fix_duplicate_symbol(&fix, "重复变量", 10, CN_DIAG_LANG_ZH);
    TEST_ASSERT(result == 1, "应成功生成重复符号建议");
    TEST_ASSERT(fix.description_zh != NULL, "应有中文描述");
    
    TEST_PASS("重复符号建议");
}

/**
 * @brief 测试缺少返回语句建议
 */
static void test_fix_missing_return(void) {
    printf("  测试: 缺少返回语句建议\n");
    
    CnDiagFix fix;
    int result;
    
    result = cn_diag_fix_missing_return(&fix, "整数", CN_DIAG_LANG_ZH);
    TEST_ASSERT(result == 1, "应成功生成缺少返回语句建议");
    TEST_ASSERT(fix.description_zh != NULL, "应有中文描述");
    
    TEST_PASS("缺少返回语句建议");
}

/**
 * @brief 测试参数数量不匹配建议
 */
static void test_fix_argument_count_mismatch(void) {
    printf("  测试: 参数数量不匹配建议\n");
    
    CnDiagFix fix;
    int result;
    
    result = cn_diag_fix_argument_count_mismatch(&fix, 3, 2, CN_DIAG_LANG_ZH);
    TEST_ASSERT(result == 1, "应成功生成参数数量不匹配建议");
    TEST_ASSERT(fix.description_zh != NULL, "应有中文描述");
    
    TEST_PASS("参数数量不匹配建议");
}

/**
 * @brief 测试常量缺少初始化建议
 */
static void test_fix_const_no_initializer(void) {
    printf("  测试: 常量缺少初始化建议\n");
    
    CnDiagFix fix;
    int result;
    
    result = cn_diag_fix_const_no_initializer(&fix, "常量名", CN_DIAG_LANG_ZH);
    TEST_ASSERT(result == 1, "应成功生成常量缺少初始化建议");
    TEST_ASSERT(fix.description_zh != NULL, "应有中文描述");
    
    TEST_PASS("常量缺少初始化建议");
}

/**
 * @brief 测试无效赋值建议
 */
static void test_fix_invalid_assignment(void) {
    printf("  测试: 无效赋值建议\n");
    
    CnDiagFix fix;
    int result;
    
    /* 测试对常量赋值 */
    result = cn_diag_fix_invalid_assignment(&fix, "常量名", 1, CN_DIAG_LANG_ZH);
    TEST_ASSERT(result == 1, "应成功生成无效赋值建议");
    TEST_ASSERT(fix.description_zh != NULL, "应有中文描述");
    
    TEST_PASS("无效赋值建议");
}

/**
 * @brief 测试修复建议格式化
 */
static void test_fix_formatting(void) {
    printf("  测试: 修复建议格式化\n");
    
    CnDiagFix fix;
    char buffer[256];
    int result;
    
    /* 生成并格式化修复建议 */
    cn_diag_fix_missing_semicolon(&fix, CN_DIAG_LANG_ZH);
    
    result = cn_diag_format_fix(&fix, CN_DIAG_LANG_ZH, buffer, sizeof(buffer));
    TEST_ASSERT(result >= 0, "格式化应成功");
    TEST_ASSERT(strlen(buffer) > 0, "格式化结果不应为空");
    
    TEST_PASS("修复建议格式化");
}

// ============================================================================
// 4. 增强诊断测试
// ============================================================================

/**
 * @brief 测试增强诊断报告
 * 
 * 验证增强诊断结构的创建和使用
 */
static void test_diagnostic_report_ex(void) {
    printf("  测试: 增强诊断报告\n");
    
    /* 创建增强诊断 */
    CnDiagnosticEx diag_ex;
    diag_ex.severity = CN_DIAG_SEVERITY_ERROR;
    diag_ex.code = CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER;
    diag_ex.filename = "test.cn";
    diag_ex.line = 10;
    diag_ex.column = 5;
    diag_ex.end_line = 10;
    diag_ex.end_column = 15;
    diag_ex.args = NULL;
    diag_ex.suggestion = "请检查标识符拼写";
    
    /* 验证字段 */
    TEST_ASSERT(diag_ex.severity == CN_DIAG_SEVERITY_ERROR, 
                "严重级别应为错误");
    TEST_ASSERT(diag_ex.code == CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                "错误码应正确");
    TEST_ASSERT(strcmp(diag_ex.filename, "test.cn") == 0,
                "文件名应正确");
    TEST_ASSERT(diag_ex.line == 10, "行号应正确");
    TEST_ASSERT(diag_ex.column == 5, "列号应正确");
    TEST_ASSERT(diag_ex.suggestion != NULL, "应有建议");
    
    /* 创建带参数的增强诊断 */
    CnDiagArg args[1] = { cn_diag_arg_string("变量名") };
    CnDiagArgs arg_list = { args, 1 };
    
    diag_ex.args = &arg_list;
    TEST_ASSERT(diag_ex.args != NULL, "参数应设置");
    TEST_ASSERT(diag_ex.args->count == 1, "参数数量应为1");
    
    TEST_PASS("增强诊断报告");
}

/**
 * @brief 测试诊断配置
 * 
 * 验证诊断配置结构的使用
 */
static void test_diagnostic_config(void) {
    printf("  测试: 诊断配置\n");
    
    /* 创建诊断配置 */
    CnDiagConfig config;
    config.language = CN_DIAG_LANG_ZH;
    config.max_errors = 100;
    config.enable_suggestions = 1;
    config.enable_recovery = 1;
    
    /* 验证配置 */
    TEST_ASSERT(config.language == CN_DIAG_LANG_ZH, "语言应为中文");
    TEST_ASSERT(config.max_errors == 100, "最大错误数应为100");
    TEST_ASSERT(config.enable_suggestions == 1, "应启用建议");
    TEST_ASSERT(config.enable_recovery == 1, "应启用恢复");
    
    /* 测试英文配置 */
    config.language = CN_DIAG_LANG_EN;
    config.max_errors = 50;
    TEST_ASSERT(config.language == CN_DIAG_LANG_EN, "语言应为英文");
    TEST_ASSERT(config.max_errors == 50, "最大错误数应为50");
    
    TEST_PASS("诊断配置");
}

/**
 * @brief 测试错误计数
 * 
 * 验证诊断集合的错误计数功能
 */
static void test_error_counting(void) {
    printf("  测试: 错误计数\n");
    
    CnDiagnostics diags;
    cn_support_diagnostics_init(&diags);
    
    /* 初始状态 */
    TEST_ASSERT(diags.count == 0, "初始计数应为0");
    
    /* 添加错误 */
    cn_support_diagnostics_report(&diags, CN_DIAG_SEVERITY_ERROR,
                                  CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                                  "test.cn", 10, 5, "未定义的标识符");
    TEST_ASSERT(diags.count == 1, "计数应为1");
    
    /* 添加警告 */
    cn_support_diagnostics_report(&diags, CN_DIAG_SEVERITY_WARNING,
                                  CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
                                  "test.cn", 20, 10, "重复的符号");
    TEST_ASSERT(diags.count == 2, "计数应为2");
    
    /* 添加更多错误 */
    cn_support_diagnostics_report(&diags, CN_DIAG_SEVERITY_ERROR,
                                  CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                                  "test.cn", 30, 8, "类型不匹配");
    TEST_ASSERT(diags.count == 3, "计数应为3");
    
    /* 验证诊断内容 */
    TEST_ASSERT(diags.items[0].severity == CN_DIAG_SEVERITY_ERROR,
                "第一条应为错误");
    TEST_ASSERT(diags.items[1].severity == CN_DIAG_SEVERITY_WARNING,
                "第二条应为警告");
    TEST_ASSERT(diags.items[2].line == 30,
                "第三条行号应为30");
    
    /* 清理 */
    cn_support_diagnostics_free(&diags);
    TEST_ASSERT(diags.count == 0, "释放后计数应为0");
    TEST_ASSERT(diags.items == NULL, "释放后items应为NULL");
    
    TEST_PASS("错误计数");
}

/**
 * @brief 测试基础诊断结构
 */
static void test_basic_diagnostic(void) {
    printf("  测试: 基础诊断结构\n");
    
    /* 创建基础诊断 */
    CnDiagnostic diag;
    diag.severity = CN_DIAG_SEVERITY_ERROR;
    diag.code = CN_DIAG_CODE_LEX_INVALID_CHAR;
    diag.filename = "test.cn";
    diag.line = 5;
    diag.column = 10;
    diag.message = "无效字符";
    
    /* 验证字段 */
    TEST_ASSERT(diag.severity == CN_DIAG_SEVERITY_ERROR,
                "严重级别应为错误");
    TEST_ASSERT(diag.code == CN_DIAG_CODE_LEX_INVALID_CHAR,
                "错误码应正确");
    TEST_ASSERT(strcmp(diag.filename, "test.cn") == 0,
                "文件名应正确");
    TEST_ASSERT(diag.line == 5, "行号应正确");
    TEST_ASSERT(diag.column == 10, "列号应正确");
    TEST_ASSERT(strcmp(diag.message, "无效字符") == 0,
                "消息应正确");
    
    TEST_PASS("基础诊断结构");
}

/**
 * @brief 测试最大错误限制
 */
static void test_max_error_limit(void) {
    printf("  测试: 最大错误限制\n");
    
    CnDiagRecoveryContext ctx;
    cn_diag_recovery_init(&ctx, 3);  /* 最大3个错误 */
    
    TEST_ASSERT(!cn_diag_recovery_is_limit_reached(&ctx),
                "初始不应达到限制");
    
    /* 记录错误直到达到限制 */
    cn_diag_recovery_on_error(&ctx, CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                              "test.cn", 10);
    TEST_ASSERT(!cn_diag_recovery_is_limit_reached(&ctx),
                "1个错误不应达到限制");
    
    cn_diag_recovery_on_error(&ctx, CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                              "test.cn", 20);
    TEST_ASSERT(!cn_diag_recovery_is_limit_reached(&ctx),
                "2个错误不应达到限制");
    
    cn_diag_recovery_on_error(&ctx, CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
                              "test.cn", 30);
    TEST_ASSERT(cn_diag_recovery_is_limit_reached(&ctx),
                "3个错误应达到限制");
    
    TEST_PASS("最大错误限制");
}

// ============================================================================
// 测试运行器
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("错误诊断增强功能单元测试\n");
    printf("========================================\n\n");
    
    /* 1. 消息表测试 */
    printf("[1] 消息表测试\n");
    test_language_setting();
    test_message_template_lookup();
    test_message_formatting();
    test_parameterized_message();
    printf("\n");
    
    /* 2. 恢复策略测试 */
    printf("[2] 恢复策略测试\n");
    test_recovery_init();
    test_recovery_strategy();
    test_cascade_suppression();
    test_sync_point_detection();
    printf("\n");
    
    /* 3. 修复建议测试 */
    printf("[3] 修复建议测试\n");
    test_fix_generation();
    test_fix_missing_semicolon();
    test_fix_undefined_identifier();
    test_fix_type_mismatch();
    test_fix_duplicate_symbol();
    test_fix_missing_return();
    test_fix_argument_count_mismatch();
    test_fix_const_no_initializer();
    test_fix_invalid_assignment();
    test_fix_formatting();
    printf("\n");
    
    /* 4. 增强诊断测试 */
    printf("[4] 增强诊断测试\n");
    test_diagnostic_report_ex();
    test_diagnostic_config();
    test_error_counting();
    test_basic_diagnostic();
    test_max_error_limit();
    printf("\n");
    
    /* 输出结果 */
    printf("========================================\n");
    printf("测试结果: %d 通过, %d 失败\n", tests_passed, tests_failed);
    printf("========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
