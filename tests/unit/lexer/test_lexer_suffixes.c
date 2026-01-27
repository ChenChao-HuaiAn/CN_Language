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

// 测试浮点后缀 f/F
void test_float_suffix() {
    printf("\n========== 测试浮点后缀 f/F ==========\n");
    
    CnToken token;
    
    // 基础浮点后缀
    token = get_first_token("3.14f");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "3.14f 应识别为浮点字面量");
    TEST_ASSERT(token.lexeme_length == 5, "3.14f 长度应为 5");
    
    token = get_first_token("2.0F");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "2.0F 应识别为浮点字面量");
    TEST_ASSERT(token.lexeme_length == 4, "2.0F 长度应为 4");
    
    // 科学计数法 + 浮点后缀
    token = get_first_token("1e10f");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "1e10f 应识别为浮点字面量");
    TEST_ASSERT(token.lexeme_length == 5, "1e10f 长度应为 5");
    
    token = get_first_token("2.5e-3F");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "2.5e-3F 应识别为浮点字面量");
    TEST_ASSERT(token.lexeme_length == 7, "2.5e-3F 长度应为 7");
    
    // 无后缀的浮点数（对比）
    token = get_first_token("3.14");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "3.14 应识别为浮点字面量");
    TEST_ASSERT(token.lexeme_length == 4, "3.14 长度应为 4");
}

// 测试长整数后缀 L/LL
void test_long_suffix() {
    printf("\n========== 测试长整数后缀 L/LL ==========\n");
    
    CnToken token;
    
    // long 后缀
    token = get_first_token("123L");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "123L 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 4, "123L 长度应为 4");
    
    token = get_first_token("456l");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "456l 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 4, "456l 长度应为 4");
    
    // long long 后缀
    token = get_first_token("789LL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "789LL 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 5, "789LL 长度应为 5");
    
    token = get_first_token("1000ll");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "1000ll 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 6, "1000ll 长度应为 6");
    
    // 十六进制 + long
    token = get_first_token("0xABCDL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0xABCDL 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 7, "0xABCDL 长度应为 7");
    
    // 二进制 + long long
    token = get_first_token("0b1010LL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0b1010LL 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 8, "0b1010LL 长度应为 8");
}

// 测试无符号整数后缀 U
void test_unsigned_suffix() {
    printf("\n========== 测试无符号整数后缀 U ==========\n");
    
    CnToken token;
    
    // unsigned 后缀
    token = get_first_token("123U");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "123U 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 4, "123U 长度应为 4");
    
    token = get_first_token("456u");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "456u 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 4, "456u 长度应为 4");
    
    // 十六进制 + unsigned
    token = get_first_token("0xFFFFFFFFU");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0xFFFFFFFFU 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 11, "0xFFFFFFFFU 长度应为 11");
    
    // 八进制 + unsigned
    token = get_first_token("0o777u");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0o777u 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 6, "0o777u 长度应为 6");
}

// 测试组合后缀 UL/ULL
void test_combined_suffix() {
    printf("\n========== 测试组合后缀 UL/ULL ==========\n");
    
    CnToken token;
    
    // unsigned long
    token = get_first_token("100UL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "100UL 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 5, "100UL 长度应为 5");
    
    token = get_first_token("200ul");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "200ul 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 5, "200ul 长度应为 5");
    
    token = get_first_token("300LU");  // L 和 U 顺序可以颠倒
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "300LU 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 5, "300LU 长度应为 5");
    
    // unsigned long long
    token = get_first_token("400ULL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "400ULL 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 6, "400ULL 长度应为 6");
    
    token = get_first_token("500ull");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "500ull 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 6, "500ull 长度应为 6");
    
    token = get_first_token("600LLU");  // LL 和 U 顺序可以颠倒
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "600LLU 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 6, "600LLU 长度应为 6");
    
    // 十六进制 + 组合后缀
    token = get_first_token("0x1234567890ABCDEFULL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0x1234567890ABCDEFULL 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 21, "0x1234567890ABCDEFULL 长度应为 21");
}

// 测试各种进制 + 后缀
void test_base_with_suffix() {
    printf("\n========== 测试各种进制 + 后缀 ==========\n");
    
    CnToken token;
    
    // 十六进制
    token = get_first_token("0xABL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0xABL 应识别为整数字面量");
    
    token = get_first_token("0xFFU");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0xFFU 应识别为整数字面量");
    
    token = get_first_token("0x123ULL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0x123ULL 应识别为整数字面量");
    
    // 二进制
    token = get_first_token("0b1010L");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0b1010L 应识别为整数字面量");
    
    token = get_first_token("0b1111U");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0b1111U 应识别为整数字面量");
    
    token = get_first_token("0b10101010UL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0b10101010UL 应识别为整数字面量");
    
    // 八进制
    token = get_first_token("0o777L");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0o777L 应识别为整数字面量");
    
    token = get_first_token("0o123U");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0o123U 应识别为整数字面量");
    
    token = get_first_token("0o755ULL");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0o755ULL 应识别为整数字面量");
}

// 测试无后缀的数字（对比）
void test_no_suffix() {
    printf("\n========== 测试无后缀的数字（对比） ==========\n");
    
    CnToken token;
    
    // 整数
    token = get_first_token("123");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "123 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 3, "123 长度应为 3");
    
    // 浮点数
    token = get_first_token("3.14");
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL, "3.14 应识别为浮点字面量");
    TEST_ASSERT(token.lexeme_length == 4, "3.14 长度应为 4");
    
    // 十六进制
    token = get_first_token("0xABC");
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER, "0xABC 应识别为整数字面量");
    TEST_ASSERT(token.lexeme_length == 5, "0xABC 长度应为 5");
}

// 测试表达式中的后缀
void test_suffix_in_expressions() {
    printf("\n========== 测试表达式中的后缀 ==========\n");
    
    CnLexer lexer;
    CnToken token;
    
    // 测试: 100L + 200UL
    const char *expr1 = "100L + 200UL";
    cn_frontend_lexer_init(&lexer, expr1, strlen(expr1), "test.cn");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER && token.lexeme_length == 4, "100L 识别正确");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_PLUS, "+ 识别正确");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_INTEGER && token.lexeme_length == 5, "200UL 识别正确");
    
    // 测试: 3.14f * 2.718F
    const char *expr2 = "3.14f * 2.718F";
    cn_frontend_lexer_init(&lexer, expr2, strlen(expr2), "test.cn");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL && token.lexeme_length == 5, "3.14f 识别正确");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_STAR, "* 识别正确");
    
    cn_frontend_lexer_next_token(&lexer, &token);
    TEST_ASSERT(token.kind == CN_TOKEN_FLOAT_LITERAL && token.lexeme_length == 6, "2.718F 识别正确");
}

int main() {
    printf("========================================\n");
    printf("数字字面量类型后缀单元测试\n");
    printf("========================================\n");
    
    test_float_suffix();
    test_long_suffix();
    test_unsigned_suffix();
    test_combined_suffix();
    test_base_with_suffix();
    test_no_suffix();
    test_suffix_in_expressions();
    
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
