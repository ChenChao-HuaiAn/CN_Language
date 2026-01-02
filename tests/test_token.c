/******************************************************************************
 * 文件名: test_token.c
 * 功能: 测试Token模块功能
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "src/core/lexer/token/CN_token.h"

/**
 * @brief 测试Token创建和销毁
 */
void test_token_creation()
{
    printf("=== 测试Token创建和销毁 ===\n");
    
    // 测试1: 创建标识符Token
    Stru_Token_t* token1 = F_token_create(
        Eum_TOKEN_IDENTIFIER,
        "myVariable",
        0,  // 自动计算长度
        1,  // 行号
        1   // 列号
    );
    
    if (token1 != NULL) {
        printf("测试1通过: 成功创建标识符Token\n");
        F_token_print(token1, true, false);
        F_token_destroy(token1);
        printf("Token已销毁\n");
    } else {
        printf("测试1失败: 无法创建标识符Token\n");
    }
    
    // 测试2: 创建关键字Token
    Stru_Token_t* token2 = F_token_create(
        Eum_TOKEN_KEYWORD_IF,
        "如果",
        0,
        2,
        5
    );
    
    if (token2 != NULL) {
        printf("测试2通过: 成功创建关键字Token\n");
        F_token_print(token2, true, false);
        F_token_destroy(token2);
        printf("Token已销毁\n");
    } else {
        printf("测试2失败: 无法创建关键字Token\n");
    }
    
    printf("\n");
}

/**
 * @brief 测试Token类型信息查询
 */
void test_token_type_info()
{
    printf("=== 测试Token类型信息查询 ===\n");
    
    // 测试各种Token类型
    enum Eum_TokenType test_types[] = {
        Eum_TOKEN_KEYWORD_VARIABLE,
        Eum_TOKEN_KEYWORD_IF,
        Eum_TOKEN_KEYWORD_WHILE,
        Eum_TOKEN_IDENTIFIER,
        Eum_TOKEN_INTEGER_LITERAL,
        Eum_TOKEN_OPERATOR_PLUS,
        Eum_TOKEN_OPERATOR_ASSIGN,
        Eum_TOKEN_SEMICOLON,
        Eum_TOKEN_EOF
    };
    
    const char* type_names[] = {
        "Eum_TOKEN_KEYWORD_VARIABLE",
        "Eum_TOKEN_KEYWORD_IF",
        "Eum_TOKEN_KEYWORD_WHILE",
        "Eum_TOKEN_IDENTIFIER",
        "Eum_TOKEN_INTEGER_LITERAL",
        "Eum_TOKEN_OPERATOR_PLUS",
        "Eum_TOKEN_OPERATOR_ASSIGN",
        "Eum_TOKEN_SEMICOLON",
        "Eum_TOKEN_EOF"
    };
    
    int count = sizeof(test_types) / sizeof(test_types[0]);
    
    for (int i = 0; i < count; i++) {
        enum Eum_TokenType type = test_types[i];
        
        printf("类型: %s\n", type_names[i]);
        printf("  类型名称: %s\n", F_token_type_to_string(type));
        printf("  类别: %s\n", F_token_get_category(type));
        printf("  是关键字? %s\n", F_token_is_keyword(type) ? "是" : "否");
        printf("  是运算符? %s\n", F_token_is_operator(type) ? "是" : "否");
        printf("  是字面量? %s\n", F_token_is_literal(type) ? "是" : "否");
        printf("  是分隔符? %s\n", F_token_is_separator(type) ? "是" : "否");
        printf("\n");
    }
}

/**
 * @brief 测试字面量Token创建
 */
void test_literal_token()
{
    printf("=== 测试字面量Token创建 ===\n");
    
    // 测试整数字面量
    Stru_Token_t* int_token = F_token_create_literal(
        Eum_TOKEN_INTEGER_LITERAL,
        "12345",
        0,
        5,
        1,
        12345
    );
    
    if (int_token != NULL) {
        printf("测试通过: 成功创建整数字面量Token\n");
        F_token_print(int_token, true, true);
        F_token_destroy(int_token);
    } else {
        printf("测试失败: 无法创建整数字面量Token\n");
    }
    
    printf("\n");
}

/**
 * @brief 测试Token复制和比较
 */
void test_token_copy_and_compare()
{
    printf("=== 测试Token复制和比较 ===\n");
    
    // 创建原始Token
    Stru_Token_t* original = F_token_create(
        Eum_TOKEN_KEYWORD_WHILE,
        "当",
        0,
        10,
        5
    );
    
    if (original == NULL) {
        printf("测试失败: 无法创建原始Token\n");
        return;
    }
    
    // 复制Token
    Stru_Token_t* copy = F_token_copy(original);
    
    if (copy != NULL) {
        printf("测试通过: 成功复制Token\n");
        
        // 比较Token
        if (F_token_equals(original, copy)) {
            printf("  原始Token和复制Token相等\n");
        } else {
            printf("  原始Token和复制Token不相等\n");
        }
        
        F_token_destroy(copy);
    } else {
        printf("测试失败: 无法复制Token\n");
    }
    
    F_token_destroy(original);
    printf("\n");
}

/**
 * @brief 测试运算符优先级和结合性
 */
void test_operator_properties()
{
    printf("=== 测试运算符优先级和结合性 ===\n");
    
    // 测试各种运算符
    struct {
        enum Eum_TokenType type;
        const char* name;
    } operators[] = {
        {Eum_TOKEN_OPERATOR_MULTIPLY, "*"},
        {Eum_TOKEN_OPERATOR_DIVIDE, "/"},
        {Eum_TOKEN_OPERATOR_PLUS, "+"},
        {Eum_TOKEN_OPERATOR_MINUS, "-"},
        {Eum_TOKEN_OPERATOR_LESS, "<"},
        {Eum_TOKEN_OPERATOR_GREATER, ">"},
        {Eum_TOKEN_OPERATOR_EQUAL, "=="},
        {Eum_TOKEN_OPERATOR_NOT_EQUAL, "!="},
        {Eum_TOKEN_OPERATOR_ASSIGN, "="},
        {Eum_TOKEN_OPERATOR_PLUS_ASSIGN, "+="},
        {Eum_TOKEN_DOT, "."},
        {Eum_TOKEN_COMMA, ","}
    };
    
    int count = sizeof(operators) / sizeof(operators[0]);
    
    printf("运算符属性表:\n");
    printf("====================================\n");
    
    for (int i = 0; i < count; i++) {
        enum Eum_TokenType type = operators[i].type;
        int precedence = F_token_get_precedence(type);
        int associativity = F_token_get_associativity(type);
        
        const char* assoc_str = "未知";
        if (associativity == 0) assoc_str = "左结合";
        else if (associativity == 1) assoc_str = "右结合";
        
        printf("%-4s : 优先级=%2d, 结合性=%s\n", 
               operators[i].name, precedence, assoc_str);
    }
    
    printf("\n");
}

/**
 * @brief 测试Token字符串表示
 */
void test_token_string_representation()
{
    printf("=== 测试Token字符串表示 ===\n");
    
    // 创建Token
    Stru_Token_t* token = F_token_create(
        Eum_TOKEN_KEYWORD_FUNCTION,
        "函数",
        0,
        20,
        8
    );
    
    if (token == NULL) {
        printf("测试失败: 无法创建Token\n");
        return;
    }
    
    // 使用F_token_to_string
    char buffer[256];
    size_t length = F_token_to_string(token, buffer, sizeof(buffer));
    
    if (length > 0) {
        printf("测试通过: 成功获取Token字符串表示\n");
        printf("  字符串: %s\n", buffer);
        printf("  长度: %zu\n", length);
    } else {
        printf("测试失败: 无法获取Token字符串表示\n");
    }
    
    F_token_destroy(token);
    printf("\n");
}

/**
 * @brief 主函数
 */
int main()
{
    printf("CN_Language Token模块测试程序\n");
    printf("==============================\n\n");
    
    // 运行所有测试
    test_token_creation();
    test_token_type_info();
    test_literal_token();
    test_token_copy_and_compare();
    test_operator_properties();
    test_token_string_representation();
    
    printf("所有测试完成！\n");
    
    return 0;
}
