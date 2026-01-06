/**
 * @file test_lexer_interface.c
 * @brief 词法分析器接口单元测试
 * 
 * 测试词法分析器抽象接口的功能和正确性。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#include "../../../../src/core/lexer/CN_lexer_interface.h"
#include "../../../../src/core/token/CN_token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 测试辅助函数
static void test_initialize(void)
{
    printf("测试: 初始化接口\n");
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    assert(lexer != NULL);
    
    const char* source = "变量 x = 42";
    bool result = lexer->initialize(source, strlen(source), "test.cn");
    assert(result == true);
    
    lexer->destroy();
    free(lexer);
    
    printf("  ✓ 初始化测试通过\n");
}

static void test_token_basic(void)
{
    printf("测试: 基本令牌识别\n");
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    assert(lexer != NULL);
    
    const char* source = "变量 x = 42";
    bool result = lexer->initialize(source, strlen(source), "test.cn");
    assert(result == true);
    
    // 测试第一个令牌：关键字"变量"
    Stru_Token_t* token = lexer->next_token();
    assert(token != NULL);
    assert(token->type == Eum_TOKEN_KEYWORD_VAR);
    assert(strncmp(token->lexeme, "变量", 2) == 0);
    F_destroy_token(token);
    
    // 测试第二个令牌：标识符"x"
    token = lexer->next_token();
    assert(token != NULL);
    assert(token->type == Eum_TOKEN_IDENTIFIER);
    assert(strncmp(token->lexeme, "x", 1) == 0);
    F_destroy_token(token);
    
    // 测试第三个令牌：运算符"="
    token = lexer->next_token();
    assert(token != NULL);
    assert(token->type == Eum_TOKEN_OPERATOR_ASSIGN);
    assert(strncmp(token->lexeme, "=", 1) == 0);
    F_destroy_token(token);
    
    // 测试第四个令牌：数字"42"
    token = lexer->next_token();
    assert(token != NULL);
    assert(token->type == Eum_TOKEN_LITERAL_INTEGER);
    assert(strncmp(token->lexeme, "42", 2) == 0);
    F_destroy_token(token);
    
    lexer->destroy();
    free(lexer);
    
    printf("  ✓ 基本令牌测试通过\n");
}

static void test_chinese_keywords(void)
{
    printf("测试: 中文关键字识别\n");
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    assert(lexer != NULL);
    
    const char* source = "如果 变量 常量 循环 当 返回 程序 开始 结束 否则";
    bool result = lexer->initialize(source, strlen(source), "test.cn");
    assert(result == true);
    
    // 测试所有中文关键字
    Eum_TokenType expected_keywords[] = {
        Eum_TOKEN_KEYWORD_IF,
        Eum_TOKEN_KEYWORD_VAR,
        Eum_TOKEN_KEYWORD_CONST,
        Eum_TOKEN_KEYWORD_LOOP,
        Eum_TOKEN_KEYWORD_WHILE,
        Eum_TOKEN_KEYWORD_RETURN,
        Eum_TOKEN_KEYWORD_PROGRAM,
        Eum_TOKEN_KEYWORD_BEGIN,
        Eum_TOKEN_KEYWORD_END,
        Eum_TOKEN_KEYWORD_ELSE
    };
    
    const char* expected_lexemes[] = {
        "如果", "变量", "常量", "循环", "当", "返回", "程序", "开始", "结束", "否则"
    };
    
    for (int i = 0; i < 10; i++)
    {
        Stru_Token_t* token = lexer->next_token();
        assert(token != NULL);
        assert(token->type == expected_keywords[i]);
        assert(strncmp(token->lexeme, expected_lexemes[i], strlen(expected_lexemes[i])) == 0);
        F_destroy_token(token);
        
        // 跳过空格
        if (i < 9)
        {
            token = lexer->next_token();
            assert(token != NULL);
            assert(token->type == Eum_TOKEN_DELIMITER_COMMA); // 实际上应该是空格，但我们的词法分析器跳过空格
            F_destroy_token(token);
        }
    }
    
    lexer->destroy();
    free(lexer);
    
    printf("  ✓ 中文关键字测试通过\n");
}

static void test_operators(void)
{
    printf("测试: 运算符识别\n");
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    assert(lexer != NULL);
    
    const char* source = "+ - * / = == != < <= > >=";
    bool result = lexer->initialize(source, strlen(source), "test.cn");
    assert(result == true);
    
    // 测试所有运算符
    Eum_TokenType expected_operators[] = {
        Eum_TOKEN_OPERATOR_PLUS,
        Eum_TOKEN_OPERATOR_MINUS,
        Eum_TOKEN_OPERATOR_MULTIPLY,
        Eum_TOKEN_OPERATOR_DIVIDE,
        Eum_TOKEN_OPERATOR_ASSIGN,
        Eum_TOKEN_OPERATOR_EQUAL,
        Eum_TOKEN_OPERATOR_NOT_EQUAL,
        Eum_TOKEN_OPERATOR_LESS,
        Eum_TOKEN_OPERATOR_LESS_EQUAL,
        Eum_TOKEN_OPERATOR_GREATER,
        Eum_TOKEN_OPERATOR_GREATER_EQUAL
    };
    
    const char* expected_lexemes[] = {
        "+", "-", "*", "/", "=", "==", "!=", "<", "<=", ">", ">="
    };
    
    for (int i = 0; i < 11; i++)
    {
        Stru_Token_t* token = lexer->next_token();
        assert(token != NULL);
        assert(token->type == expected_operators[i]);
        assert(strncmp(token->lexeme, expected_lexemes[i], strlen(expected_lexemes[i])) == 0);
        F_destroy_token(token);
        
        // 跳过空格
        if (i < 10)
        {
            token = lexer->next_token();
            assert(token != NULL);
            assert(token->type == Eum_TOKEN_DELIMITER_COMMA); // 实际上应该是空格
            F_destroy_token(token);
        }
    }
    
    lexer->destroy();
    free(lexer);
    
    printf("  ✓ 运算符测试通过\n");
}

static void test_tokenize_all(void)
{
    printf("测试: 批量令牌化\n");
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    assert(lexer != NULL);
    
    const char* source = "变量 x = 42 + y";
    bool result = lexer->initialize(source, strlen(source), "test.cn");
    assert(result == true);
    
    Stru_DynamicArray_t* tokens = lexer->tokenize_all();
    assert(tokens != NULL);
    
    size_t token_count = F_dynamic_array_length(tokens);
    assert(token_count == 6); // 变量, x, =, 42, +, y, EOF
    
    // 验证令牌顺序
    Eum_TokenType expected_types[] = {
        Eum_TOKEN_KEYWORD_VAR,
        Eum_TOKEN_IDENTIFIER,
        Eum_TOKEN_OPERATOR_ASSIGN,
        Eum_TOKEN_LITERAL_INTEGER,
        Eum_TOKEN_OPERATOR_PLUS,
        Eum_TOKEN_IDENTIFIER,
        Eum_TOKEN_EOF
    };
    
    for (size_t i = 0; i < token_count; i++)
    {
        Stru_Token_t** token_ptr = (Stru_Token_t**)F_dynamic_array_get(tokens, i);
        assert(token_ptr != NULL);
        Stru_Token_t* token = *token_ptr;
        assert(token != NULL);
        
        if (i < 6) // 前6个令牌
        {
            assert(token->type == expected_types[i]);
        }
        
        F_destroy_token(token);
    }
    
    F_destroy_dynamic_array(tokens);
    lexer->destroy();
    free(lexer);
    
    printf("  ✓ 批量令牌化测试通过\n");
}

static void test_position_tracking(void)
{
    printf("测试: 位置跟踪\n");
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    assert(lexer != NULL);
    
    const char* source = "变量 x\n= 42";
    bool result = lexer->initialize(source, strlen(source), "test.cn");
    assert(result == true);
    
    // 获取第一个令牌并检查位置
    Stru_Token_t* token = lexer->next_token();
    assert(token != NULL);
    assert(token->line == 1);
    assert(token->column == 1);
    F_destroy_token(token);
    
    // 获取第二个令牌
    token = lexer->next_token();
    assert(token != NULL);
    assert(token->line == 1);
    assert(token->column == 4); // "变量"占2个字符+1个空格
    F_destroy_token(token);
    
    // 获取第三个令牌（在第二行）
    token = lexer->next_token();
    assert(token != NULL);
    assert(token->line == 2);
    assert(token->column == 1);
    F_destroy_token(token);
    
    // 测试get_position函数
    size_t line, column;
    lexer->get_position(&line, &column);
    assert(line == 2);
    assert(column == 3); // "="占1个字符+1个空格
    
    lexer->destroy();
    free(lexer);
    
    printf("  ✓ 位置跟踪测试通过\n");
}

static void test_error_handling(void)
{
    printf("测试: 错误处理\n");
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    assert(lexer != NULL);
    
    // 测试无效初始化
    bool result = lexer->initialize(NULL, 0, "test.cn");
    assert(result == false);
    assert(lexer->has_errors() == true);
    assert(strlen(lexer->get_last_error()) > 0);
    
    // 重置并测试有效代码
    lexer->reset();
    const char* source = "变量 x = @"; // @是无效字符
    result = lexer->initialize(source, strlen(source), "test.cn");
    assert(result == true);
    
    // 跳过有效令牌
    Stru_Token_t* token = lexer->next_token(); // 变量
    F_destroy_token(token);
    token = lexer->next_token(); // x
    F_destroy_token(token);
    token = lexer->next_token(); // =
    F_destroy_token(token);
    
    // 获取错误令牌
    token = lexer->next_token();
    assert(token != NULL);
    assert(token->type == Eum_TOKEN_ERROR);
    assert(lexer->has_errors() == true);
    assert(strlen(lexer->get_last_error()) > 0);
    F_destroy_token(token);
    
    lexer->destroy();
    free(lexer);
    
    printf("  ✓ 错误处理测试通过\n");
}

// 主测试函数
int main(void)
{
    printf("开始词法分析器接口单元测试\n");
    printf("============================\n");
    
    test_initialize();
    test_token_basic();
    test_chinese_keywords();
    test_operators();
    test_tokenize_all();
    test_position_tracking();
    test_error_handling();
    
    printf("\n============================\n");
    printf("所有测试通过！\n");
    
    return 0;
}
