/**
 * @file CN_token_query.h
 * @brief CN_Language 令牌类型查询模块
 * 
 * 负责令牌类型的查询和分类功能。
 * 遵循单一职责原则，专注于令牌类型查询。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_TOKEN_QUERY_H
#define CN_TOKEN_QUERY_H

#include "../CN_token_types.h"
#include <stdbool.h>

/**
 * @brief 判断是否为关键字令牌
 * 
 * 检查令牌类型是否属于关键字类别。
 * 
 * @param type 令牌类型
 * @return true 是关键字
 * @return false 不是关键字
 */
bool F_token_query_is_keyword(Eum_TokenType type);

/**
 * @brief 判断是否为运算符令牌
 * 
 * 检查令牌类型是否属于运算符类别（包括符号运算符和关键字运算符）。
 * 
 * @param type 令牌类型
 * @return true 是运算符
 * @return false 不是运算符
 */
bool F_token_query_is_operator(Eum_TokenType type);

/**
 * @brief 判断是否为字面量令牌
 * 
 * 检查令牌类型是否属于字面量类别。
 * 
 * @param type 令牌类型
 * @return true 是字面量
 * @return false 不是字面量
 */
bool F_token_query_is_literal(Eum_TokenType type);

/**
 * @brief 判断是否为分隔符令牌
 * 
 * 检查令牌类型是否属于分隔符类别。
 * 
 * @param type 令牌类型
 * @return true 是分隔符
 * @return false 不是分隔符
 */
bool F_token_query_is_delimiter(Eum_TokenType type);

/**
 * @brief 获取运算符优先级
 * 
 * 对于运算符令牌，返回其优先级值（1-10，值越小优先级越高）。
 * 对于非运算符令牌，返回0。
 * 
 * @param type 令牌类型
 * @return int 优先级值（1-10），0表示非运算符
 */
int F_token_query_get_precedence(Eum_TokenType type);

#endif // CN_TOKEN_QUERY_H
