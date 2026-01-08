/**
 * @file CN_token.c
 * @brief CN_Language 令牌模块适配器
 * 
 * 提供向后兼容的接口，调用新的模块化实现。
 * 遵循适配器模式，将旧接口适配到新架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 */

#include "CN_token.h"
#include "lifecycle/CN_token_lifecycle.h"
#include "values/CN_token_values.h"
#include "query/CN_token_query.h"
#include "keywords/CN_token_keywords.h"
#include "tools/CN_token_tools.h"

// ============================================
// 向后兼容的函数实现
// ============================================

// 创建令牌
Stru_Token_t* F_create_token(Eum_TokenType type, const char* lexeme, size_t line, size_t column)
{
    return F_token_lifecycle_create(type, lexeme, line, column);
}

// 销毁令牌
void F_destroy_token(Stru_Token_t* token)
{
    F_token_lifecycle_destroy(token);
}

// 令牌类型转字符串
const char* F_token_type_to_string(Eum_TokenType type)
{
    return F_token_tools_type_to_string(type);
}

// 判断是否为关键字令牌
bool F_token_is_keyword(Eum_TokenType type)
{
    return F_token_query_is_keyword(type);
}

// 判断是否为运算符令牌
bool F_token_is_operator(Eum_TokenType type)
{
    return F_token_query_is_operator(type);
}

// 判断是否为字面量令牌
bool F_token_is_literal(Eum_TokenType type)
{
    return F_token_query_is_literal(type);
}

// 判断是否为分隔符令牌
bool F_token_is_delimiter(Eum_TokenType type)
{
    return F_token_query_is_delimiter(type);
}

// 获取关键字优先级
int F_token_get_precedence(Eum_TokenType type)
{
    return F_token_query_get_precedence(type);
}

// 获取关键字的中文表示
const char* F_token_get_chinese_keyword(Eum_TokenType type)
{
    return F_token_keywords_get_chinese(type);
}

// 获取关键字的英文表示
const char* F_token_get_english_keyword(Eum_TokenType type)
{
    return F_token_keywords_get_english(type);
}

// 获取关键字的分类
int F_token_get_keyword_category(Eum_TokenType type)
{
    return F_token_keywords_get_category(type);
}

// 设置整数字面量值
void F_token_set_int_value(Stru_Token_t* token, long value)
{
    F_token_values_set_int(token, value);
}

// 设置浮点数字面量值
void F_token_set_float_value(Stru_Token_t* token, double value)
{
    F_token_values_set_float(token, value);
}

// 设置布尔字面量值
void F_token_set_bool_value(Stru_Token_t* token, bool value)
{
    F_token_values_set_bool(token, value);
}

// 获取整数字面量值
long F_token_get_int_value(const Stru_Token_t* token)
{
    return F_token_values_get_int(token);
}

// 获取浮点数字面量值
double F_token_get_float_value(const Stru_Token_t* token)
{
    return F_token_values_get_float(token);
}

// 获取布尔字面量值
bool F_token_get_bool_value(const Stru_Token_t* token)
{
    return F_token_values_get_bool(token);
}

// 复制令牌
Stru_Token_t* F_token_copy(const Stru_Token_t* token)
{
    return F_token_lifecycle_copy(token);
}

// 比较两个令牌
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2)
{
    return F_token_lifecycle_equals(token1, token2);
}

// 打印令牌信息
int F_token_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size)
{
    return F_token_tools_to_string(token, buffer, buffer_size);
}
