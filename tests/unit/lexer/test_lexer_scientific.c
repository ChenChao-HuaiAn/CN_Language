#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"

static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            test_passed++; \
            printf("✓ PASS: %s\n", message); \
        } else { \
            test_failed++; \
            printf("✗ FAIL: %s\n", message); \
        } \
    } while(0)

// 辅助函数：从源代码获取第一个token
static CnToken get_first_token(const char *source) {
    CnLexer lexer;
    CnToken token;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_next_token(&lexer, &token);
    return token;
}

// 测试基础科学计数法形式
void test_basic_scientific_notation() {
    printf("\n========== 测试基础科学计数法 ==========\n");
    
    CnToken token;
    
    // 测试正指数
    token = get_first_token("1e10");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1e10 应识别为浮点字面量");
    
    token = get_first_token("1.5e10");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1.5e10 应识别为浮点字面量");
    
    token = get_first_token("2.718E8");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "2.718E8 应识别为浮点字面量");
    
    // 测试负指数
    token = get_first_token("1e-5");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1e-5 应识别为浮点字面量");
    
    token = get_first_token("2.3E-10");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "2.3E-10 应识别为浮点字面量");
    
    // 测试显式正号
    token = get_first_token("1e+5");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1e+5 应识别为浮点字面量");
    
    token = get_first_token("3.14e+2");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "3.14e+2 应识别为浮点字面量");
}

// 测试整数科学计数法（无小数点）
void test_integer_scientific_notation() {
    printf("\n========== 测试整数科学计数法 ==========\n");
    
    CnToken token;
    
    // 整数形式的科学计数法应转换为浮点数
    token = get_first_token("5e3");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "5e3 应识别为浮点字面量");
    
    token = get_first_token("2e-4");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "2e-4 应识别为浮点字面量");
    
    token = get_first_token("7E+6");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "7E+6 应识别为浮点字面量");
}

// 测试大小写混合
void test_case_sensitivity() {
    printf("\n========== 测试大小写混合 ==========\n");
    
    CnToken token;
    
    // 小写 e
    token = get_first_token("1.23e45");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1.23e45 (小写e) 应识别为浮点字面量");
    
    // 大写 E
    token = get_first_token("1.23E45");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1.23E45 (大写E) 应识别为浮点字面量");
}

// 测试边界值
void test_boundary_values() {
    printf("\n========== 测试边界值 ==========\n");
    
    CnToken token;
    
    // 零指数
    token = get_first_token("1e0");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1e0 应识别为浮点字面量");
    
    token = get_first_token("1.5e+0");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1.5e+0 应识别为浮点字面量");
    
    token = get_first_token("2.0e-0");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "2.0e-0 应识别为浮点字面量");
    
    // 大指数值（接近double极限）
    token = get_first_token("1.0e308");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1.0e308 应识别为浮点字面量");
    
    token = get_first_token("1.0e-308");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1.0e-308 应识别为浮点字面量");
}

// 测试物理常量
void test_physical_constants() {
    printf("\n========== 测试物理常量 ==========\n");
    
    CnToken token;
    
    // 阿伏伽德罗常数
    token = get_first_token("6.02214076e23");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "阿伏伽德罗常数 6.02214076e23");
    
    // 光速
    token = get_first_token("2.99792458e8");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "光速 2.99792458e8 m/s");
    
    // 普朗克常数
    token = get_first_token("6.62607015e-34");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "普朗克常数 6.62607015e-34 J·s");
    
    // 电子质量
    token = get_first_token("9.10938356e-31");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "电子质量 9.10938356e-31 kg");
    
    // 引力常数
    token = get_first_token("6.67430e-11");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "引力常数 6.67430e-11 N·m²/kg²");
}

// 测试与普通浮点数的区分
void test_distinction_from_normal_floats() {
    printf("\n========== 测试与普通浮点数的区分 ==========\n");
    
    CnToken token;
    
    // 普通浮点数（无科学计数法）
    token = get_first_token("3.14");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "3.14 应识别为浮点字面量");
    
    token = get_first_token("0.5");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "0.5 应识别为浮点字面量");
    
    // 科学计数法浮点数
    token = get_first_token("3.14e2");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "3.14e2 应识别为浮点字面量");
    
    token = get_first_token("0.5e-1");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "0.5e-1 应识别为浮点字面量");
}

// 测试表达式中的科学计数法
void test_scientific_in_expressions() {
    printf("\n========== 测试表达式中的科学计数法 ==========\n");
    
    CnLexer lexer;
    CnToken token;
    
    // 测试: 1e10 + 2e5
    const char *expr1 = "1e10 + 2e5";
    cn_frontend_lexer_init(&lexer, expr1, strlen(expr1), "test.cn");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1e10 应识别为浮点字面量");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_PLUS, "+ 应识别为加号");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "2e5 应识别为浮点字面量");
    
    // 测试: 3.14e2 * 1.5e-3
    const char *expr2 = "3.14e2 * 1.5e-3";
    cn_frontend_lexer_init(&lexer, expr2, strlen(expr2), "test.cn");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "3.14e2 应识别为浮点字面量");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_STAR, "* 应识别为乘号");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1.5e-3 应识别为浮点字面量");
}

int main() {
    printf("========================================\n");
    printf("科学计数法词法分析单元测试\n");
    printf("========================================\n");
    
    test_basic_scientific_notation();
    test_integer_scientific_notation();
    test_case_sensitivity();
    test_boundary_values();
    test_physical_constants();
    test_distinction_from_normal_floats();
    test_scientific_in_expressions();
    
    printf("\n========================================\n");
    printf("测试结果汇总\n");
    printf("========================================\n");
    printf("通过: %d 个测试\n", test_passed);
    printf("失败: %d 个测试\n", test_failed);
    printf("总计: %d 个测试\n", test_passed + test_failed);
    
    if (test_failed == 0) {
        printf("\n✓ 所有测试通过！\n");
        return 0;
    } else {
        printf("\n✗ 部分测试失败！\n");
        return 1;
    }
}
