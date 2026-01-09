/**
 * @file test_parser_phrase_recovery.c
 * @brief CN_Language 语法分析器短语级恢复模块测试
 * 
 * 测试短语级恢复功能，包括插入缺失令牌、删除多余令牌、替换错误令牌等。
 * 遵循项目架构规范，使用Unity测试框架。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "unity.h"
#include "unity_fixture.h"
#include "../../../../src/core/parser/error_handling/CN_parser_phrase_recovery.h"
#include "../../../../src/core/parser/CN_parser_interface.h"
#include "../../../../src/core/parser/interface/CN_parser_interface_impl.h"
#include "../../../../src/core/lexer/CN_lexer_interface.h"
#include "../../../../src/core/token/CN_token.h"
#include "../../../../src/core/parser/CN_syntax_error.h"
#include "../../../../src/infrastructure/memory/CN_memory_interface.h"
#include <stdio.h>
#include <string.h>

// ============================================
// 测试辅助函数
// ============================================

/**
 * @brief 创建测试令牌
 */
static Stru_Token_t* create_test_token(Eum_TokenType type, const char* lexeme, size_t line, size_t column)
{
    return F_create_token(type, lexeme, line, column);
}

/**
 * @brief 创建测试语法分析器接口
 */
static Stru_ParserInterface_t* create_test_parser_interface(void)
{
    // 创建语法分析器接口
    Stru_ParserInterface_t* parser = F_create_parser_interface();
    if (parser == NULL) {
        return NULL;
    }
    
    // 创建模拟词法分析器接口
    Stru_LexerInterface_t* lexer = cn_malloc(sizeof(Stru_LexerInterface_t));
    if (lexer == NULL) {
        parser->destroy(parser);
        return NULL;
    }
    
    // 初始化词法分析器接口（简化实现）
    memset(lexer, 0, sizeof(Stru_LexerInterface_t));
    
    // 初始化语法分析器
    if (!parser->initialize(parser, lexer)) {
        cn_free(lexer);
        parser->destroy(parser);
        return NULL;
    }
    
    return parser;
}

/**
 * @brief 创建测试语法错误
 */
static Stru_SyntaxError_t* create_test_syntax_error(Eum_SyntaxErrorType type, 
                                                   size_t line, size_t column,
                                                   const char* message,
                                                   const char* expected)
{
    // 创建错误对象
    Stru_SyntaxError_t* error = cn_malloc(sizeof(Stru_SyntaxError_t));
    if (error == NULL) {
        return NULL;
    }
    
    // 初始化错误对象
    error->type = type;
    error->severity = Eum_SEVERITY_ERROR;
    error->line = line;
    error->column = column;
    error->message = message != NULL ? cn_strdup(message) : NULL;
    error->token = NULL;
    
    // 设置错误数据
    if (expected != NULL) {
        error->data.expected = cn_strdup(expected);
    } else {
        error->data.expected = NULL;
    }
    
    return error;
}

// ============================================
// 测试用例
// ============================================

TEST_GROUP(PhraseRecoveryTests);

TEST_SETUP(PhraseRecoveryTests)
{
    // 测试前初始化
    cn_memory_initialize();
}

TEST_TEAR_DOWN(PhraseRecoveryTests)
{
    // 测试后清理
    cn_memory_cleanup();
}

/**
 * @brief 测试创建和销毁短语级恢复结果
 */
TEST(PhraseRecoveryTests, TestCreateDestroyRecoveryResult)
{
    // 创建恢复结果
    Stru_PhraseRecoveryResult_t* result = F_create_phrase_recovery_result();
    TEST_ASSERT_NOT_NULL(result);
    
    // 验证初始值
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL(Eum_RECOVERY_NONE, result->strategy);
    TEST_ASSERT_NULL(result->suggestion);
    TEST_ASSERT_EQUAL(0, result->tokens_affected);
    TEST_ASSERT_NULL(result->inserted_token);
    TEST_ASSERT_NULL(result->deleted_token);
    TEST_ASSERT_NULL(result->replaced_token);
    
    // 销毁恢复结果
    F_destroy_phrase_recovery_result(result);
}

/**
 * @brief 测试插入缺失令牌恢复
 */
TEST(PhraseRecoveryTests, TestInsertMissingTokenRecovery)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 创建缺失令牌错误
    Stru_SyntaxError_t* error = create_test_syntax_error(
        Eum_ERROR_MISSING_TOKEN, 
        10, 5, 
        "缺失分号", 
        "分号"
    );
    TEST_ASSERT_NOT_NULL(error);
    
    // 尝试插入缺失令牌恢复
    Stru_PhraseRecoveryResult_t* recovery = F_try_insert_missing_token(parser, error);
    TEST_ASSERT_NOT_NULL(recovery);
    
    // 验证恢复结果
    TEST_ASSERT_TRUE(recovery->success);
    TEST_ASSERT_EQUAL(Eum_RECOVERY_INSERT_TOKEN, recovery->strategy);
    TEST_ASSERT_NOT_NULL(recovery->suggestion);
    TEST_ASSERT_EQUAL(1, recovery->tokens_affected);
    TEST_ASSERT_NOT_NULL(recovery->inserted_token);
    
    // 验证插入的令牌
    TEST_ASSERT_EQUAL(Eum_TOKEN_IDENTIFIER, recovery->inserted_token->type);
    TEST_ASSERT_EQUAL_STRING("分号", recovery->inserted_token->lexeme);
    TEST_ASSERT_EQUAL(10, recovery->inserted_token->line);
    TEST_ASSERT_EQUAL(5, recovery->inserted_token->column);
    
    // 清理资源
    F_destroy_phrase_recovery_result(recovery);
    cn_free(error->message);
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    cn_free(error);
    parser->destroy(parser);
}

/**
 * @brief 测试删除多余令牌恢复
 */
TEST(PhraseRecoveryTests, TestDeleteExtraTokenRecovery)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 获取解析器状态并设置当前令牌
    Stru_ParserState_t* state = F_get_parser_state(parser);
    TEST_ASSERT_NOT_NULL(state);
    
    // 创建测试令牌作为当前令牌
    Stru_Token_t* token = create_test_token(Eum_TOKEN_IDENTIFIER, "多余的令牌", 10, 5);
    TEST_ASSERT_NOT_NULL(token);
    state->current_token = token;
    
    // 创建意外令牌错误
    Stru_SyntaxError_t* error = create_test_syntax_error(
        Eum_ERROR_UNEXPECTED_TOKEN, 
        10, 5, 
        "意外的令牌", 
        "分号"
    );
    TEST_ASSERT_NOT_NULL(error);
    error->token = token;
    
    // 尝试删除多余令牌恢复
    Stru_PhraseRecoveryResult_t* recovery = F_try_delete_extra_token(parser, error);
    TEST_ASSERT_NOT_NULL(recovery);
    
    // 验证恢复结果
    TEST_ASSERT_TRUE(recovery->success);
    TEST_ASSERT_EQUAL(Eum_RECOVERY_DELETE_TOKEN, recovery->strategy);
    TEST_ASSERT_NOT_NULL(recovery->suggestion);
    TEST_ASSERT_EQUAL(1, recovery->tokens_affected);
    TEST_ASSERT_NOT_NULL(recovery->deleted_token);
    
    // 验证删除的令牌
    TEST_ASSERT_EQUAL(Eum_TOKEN_IDENTIFIER, recovery->deleted_token->type);
    TEST_ASSERT_EQUAL_STRING("多余的令牌", recovery->deleted_token->lexeme);
    TEST_ASSERT_EQUAL(10, recovery->deleted_token->line);
    TEST_ASSERT_EQUAL(5, recovery->deleted_token->column);
    
    // 清理资源
    F_destroy_phrase_recovery_result(recovery);
    cn_free(error->message);
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    cn_free(error);
    F_destroy_token(token);
    parser->destroy(parser);
}

/**
 * @brief 测试替换错误令牌恢复
 */
TEST(PhraseRecoveryTests, TestReplaceWrongTokenRecovery)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 获取解析器状态并设置当前令牌
    Stru_ParserState_t* state = F_get_parser_state(parser);
    TEST_ASSERT_NOT_NULL(state);
    
    // 创建测试令牌作为当前令牌
    Stru_Token_t* token = create_test_token(Eum_TOKEN_IDENTIFIER, "错误的令牌", 10, 5);
    TEST_ASSERT_NOT_NULL(token);
    state->current_token = token;
    
    // 创建令牌不匹配错误
    Stru_SyntaxError_t* error = create_test_syntax_error(
        Eum_ERROR_MISMATCHED_TOKEN, 
        10, 5, 
        "令牌不匹配", 
        "正确的令牌"
    );
    TEST_ASSERT_NOT_NULL(error);
    error->token = token;
    
    // 尝试替换错误令牌恢复
    Stru_PhraseRecoveryResult_t* recovery = F_try_replace_wrong_token(parser, error);
    TEST_ASSERT_NOT_NULL(recovery);
    
    // 验证恢复结果
    TEST_ASSERT_TRUE(recovery->success);
    TEST_ASSERT_EQUAL(Eum_RECOVERY_REPLACE_TOKEN, recovery->strategy);
    TEST_ASSERT_NOT_NULL(recovery->suggestion);
    TEST_ASSERT_EQUAL(1, recovery->tokens_affected);
    TEST_ASSERT_NOT_NULL(recovery->replaced_token);
    TEST_ASSERT_NOT_NULL(recovery->inserted_token);
    
    // 验证替换的令牌
    TEST_ASSERT_EQUAL(Eum_TOKEN_IDENTIFIER, recovery->replaced_token->type);
    TEST_ASSERT_EQUAL_STRING("错误的令牌", recovery->replaced_token->lexeme);
    
    // 验证插入的令牌
    TEST_ASSERT_EQUAL(Eum_TOKEN_IDENTIFIER, recovery->inserted_token->type);
    TEST_ASSERT_EQUAL_STRING("正确的令牌", recovery->inserted_token->lexeme);
    
    // 清理资源
    F_destroy_phrase_recovery_result(recovery);
    cn_free(error->message);
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    cn_free(error);
    F_destroy_token(token);
    parser->destroy(parser);
}

/**
 * @brief 测试跳过当前令牌恢复
 */
TEST(PhraseRecoveryTests, TestSkipCurrentTokenRecovery)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 获取解析器状态并设置当前令牌
    Stru_ParserState_t* state = F_get_parser_state(parser);
    TEST_ASSERT_NOT_NULL(state);
    
    // 创建测试令牌作为当前令牌
    Stru_Token_t* token = create_test_token(Eum_TOKEN_IDENTIFIER, "要跳过的令牌", 10, 5);
    TEST_ASSERT_NOT_NULL(token);
    state->current_token = token;
    
    // 创建一般语法错误
    Stru_SyntaxError_t* error = create_test_syntax_error(
        Eum_ERROR_INVALID_EXPRESSION, 
        10, 5, 
        "无效表达式", 
        NULL
    );
    TEST_ASSERT_NOT_NULL(error);
    error->token = token;
    
    // 尝试跳过当前令牌恢复
    Stru_PhraseRecoveryResult_t* recovery = F_try_skip_current_token(parser, error);
    TEST_ASSERT_NOT_NULL(recovery);
    
    // 验证恢复结果
    TEST_ASSERT_TRUE(recovery->success);
    TEST_ASSERT_EQUAL(Eum_RECOVERY_SKIP_TOKEN, recovery->strategy);
    TEST_ASSERT_NOT_NULL(recovery->suggestion);
    TEST_ASSERT_EQUAL(1, recovery->tokens_affected);
    TEST_ASSERT_NOT_NULL(recovery->deleted_token);
    
    // 验证跳过的令牌
    TEST_ASSERT_EQUAL(Eum_TOKEN_IDENTIFIER, recovery->deleted_token->type);
    TEST_ASSERT_EQUAL_STRING("要跳过的令牌", recovery->deleted_token->lexeme);
    
    // 清理资源
    F_destroy_phrase_recovery_result(recovery);
    cn_free(error->message);
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    cn_free(error);
    F_destroy_token(token);
    parser->destroy(parser);
}

/**
 * @brief 测试分析错误上下文
 */
TEST(PhraseRecoveryTests, TestAnalyzeErrorContext)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 获取解析器状态并设置当前令牌
    Stru_ParserState_t* state = F_get_parser_state(parser);
    TEST_ASSERT_NOT_NULL(state);
    
    // 创建测试令牌
    Stru_Token_t* token = create_test_token(Eum_TOKEN_IDENTIFIER, "测试令牌", 10, 5);
    TEST_ASSERT_NOT_NULL(token);
    state->current_token = token;
    
    // 创建语法错误
    Stru_SyntaxError_t* error = create_test_syntax_error(
        Eum_ERROR_UNEXPECTED_TOKEN, 
        10, 5, 
        "意外的令牌", 
        "分号"
    );
    TEST_ASSERT_NOT_NULL(error);
    error->token = token;
    
    // 分析错误上下文
    char context_buffer[256];
    int context_len = F_analyze_error_context(parser, error, context_buffer, sizeof(context_buffer));
    TEST_ASSERT_GREATER_THAN(0, context_len);
    
    // 验证上下文包含错误信息
    TEST_ASSERT_NOT_NULL(strstr(context_buffer, "错误令牌"));
    TEST_ASSERT_NOT_NULL(strstr(context_buffer, "行 10"));
    TEST_ASSERT_NOT_NULL(strstr(context_buffer, "列 5"));
    
    // 清理资源
    cn_free(error->message);
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    cn_free(error);
    F_destroy_token(token);
    parser->destroy(parser);
}

/**
 * @brief 测试生成修复建议
 */
TEST(PhraseRecoveryTests, TestGenerateFixSuggestion)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 创建缺失令牌错误
    Stru_SyntaxError_t* error = create_test_syntax_error(
        Eum_ERROR_MISSING_TOKEN, 
        10, 5, 
        "缺失分号", 
        "分号"
    );
    TEST_ASSERT_NOT_NULL(error);
    
    // 生成修复建议
    char suggestion_buffer[256];
    int suggestion_len = F_generate_fix_suggestion(parser, error, suggestion_buffer, sizeof(suggestion_buffer));
    TEST_ASSERT_GREATER_THAN(0, suggestion_len);
    
    // 验证建议包含修复信息
    TEST_ASSERT_NOT_NULL(strstr(suggestion_buffer, "建议"));
    TEST_ASSERT_NOT_NULL(strstr(suggestion_buffer, "插入"));
    TEST_ASSERT_NOT_NULL(strstr(suggestion_buffer, "分号"));
    
    // 清理资源
    cn_free(error->message);
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    cn_free(error);
    parser->destroy(parser);
}

/**
 * @brief 测试检查常见错误模式
 */
TEST(PhraseRecoveryTests, TestCheckCommonErrorPattern)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 获取解析器状态并设置当前令牌
    Stru_ParserState_t* state = F_get_parser_state(parser);
    TEST_ASSERT_NOT_NULL(state);
    
    // 创建缺失分号错误
    Stru_SyntaxError_t* error = create_test_syntax_error(
        Eum_ERROR_MISSING_TOKEN, 
        10, 5, 
        "缺失分号", 
        "分号"
    );
    TEST_ASSERT_NOT_NULL(error);
    
    // 检查常见错误模式
    char pattern_buffer[256];
    bool found_pattern = F_check_common_error_pattern(parser, error, pattern_buffer, sizeof(pattern_buffer));
    TEST_ASSERT_TRUE(found_pattern);
    
    // 验证模式信息
    TEST_ASSERT_NOT_NULL(strstr(pattern_buffer, "缺失分号错误模式"));
    
    // 清理资源
    cn_free(error->message);
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    cn_free(error);
    parser->destroy(parser);
}

/**
 * @brief 测试应用恢复策略
 */
TEST(PhraseRecoveryTests, TestApplyRecoveryStrategy)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 获取解析器状态并设置当前令牌
    Stru_ParserState_t* state = F_get_parser_state(parser);
    TEST_ASSERT_NOT_NULL(state);
    
    // 创建测试令牌作为当前令牌
    Stru_Token_t* token = create_test_token(Eum_TOKEN_IDENTIFIER, "测试令牌", 10, 5);
    TEST_ASSERT_NOT_NULL(token);
    state->current_token = token;
    
    // 创建删除令牌恢复结果
    Stru_PhraseRecoveryResult_t* recovery = F_create_phrase_recovery_result();
    TEST_ASSERT_NOT_NULL(recovery);
    
    recovery->success = true;
    recovery->strategy = Eum_RECOVERY_DELETE_TOKEN;
    recovery->suggestion = cn_strdup("删除多余的令牌");
    recovery->tokens_affected = 1;
    recovery->deleted_token = F_copy_token(token);
    
    // 应用恢复策略
    bool applied = F_apply_recovery_strategy(parser, recovery);
    TEST_ASSERT_TRUE(applied);
    
    // 验证令牌已前进（简化实现）
    // 在实际实现中，这里会验证令牌流的状态
    
    // 清理资源
    F_destroy_phrase_recovery_result(recovery);
    F_destroy_token(token);
    parser->destroy(parser);
}

/**
 * @brief 测试短语级恢复主函数
 */
TEST(PhraseRecoveryTests, TestTryPhraseRecovery)
{
    // 创建测试语法分析器接口
    Stru_ParserInterface_t* parser = create_test_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 获取解析器状态并设置当前令牌
    Stru_ParserState_t* state = F_get_parser_state(parser);
    TEST_ASSERT_NOT_NULL(state);
    
    // 创建测试令牌作为当前令牌
    Stru_Token_t* token = create_test_token(Eum_TOKEN_IDENTIFIER, "意外的令牌", 10, 5);
    TEST_ASSERT_NOT_NULL(token);
    state->current_token = token;
    
    // 创建意外令牌错误
    Stru_SyntaxError_t* error = create_test_syntax_error(
        Eum_ERROR_UNEXPECTED_TOKEN, 
        10, 5, 
        "意外的令牌", 
        "分号"
    );
    TEST_ASSERT_NOT_NULL(error);
    error->token = token;
    
    // 尝试短语级恢复
    Stru_PhraseRecoveryResult_t* recovery = F_try_phrase_recovery(parser, error);
    TEST_ASSERT_NOT_NULL(recovery);
    
    // 验证恢复结果
    TEST_ASSERT_TRUE(recovery->success);
    TEST_ASSERT_EQUAL(Eum_RECOVERY_DELETE_TOKEN, recovery->strategy);
    TEST_ASSERT_NOT_NULL(recovery->suggestion);
    
    // 清理资源
    F_destroy_phrase_recovery_result(recovery);
    cn_free(error->message);
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    cn_free(error);
    F_destroy_token(token);
    parser->destroy(parser);
}

// ============================================
// 测试运行器
// ============================================

TEST_GROUP_RUNNER(PhraseRecoveryTests)
{
    RUN_TEST_CASE(PhraseRecoveryTests, TestCreateDestroyRecoveryResult);
    RUN_TEST_CASE(PhraseRecoveryTests, TestInsertMissingTokenRecovery);
    RUN_TEST_CASE(PhraseRecoveryTests, TestDeleteExtraTokenRecovery);
    RUN_TEST_CASE(PhraseRecoveryTests, TestReplaceWrongTokenRecovery);
    RUN_TEST_CASE(PhraseRecoveryTests, TestSkipCurrentTokenRecovery);
    RUN_TEST_CASE(PhraseRecoveryTests, TestAnalyzeErrorContext);
    RUN_TEST_CASE(PhraseRecoveryTests, TestGenerateFixSuggestion);
    RUN_TEST_CASE(PhraseRecoveryTests, TestCheckCommonErrorPattern);
    RUN_TEST_CASE(PhraseRecoveryTests, TestApplyRecoveryStrategy);
    RUN_TEST_CASE(PhraseRecoveryTests, TestTryPhraseRecovery);
}

/**
 * @brief 主测试函数
 */
int main(int argc, char* argv[])
{
    // 初始化测试框架
    UNITY_BEGIN();
    
    // 运行测试组
    RUN_TEST_GROUP(PhraseRecoveryTests);
    
    // 结束测试
    return UNITY_END();
}
