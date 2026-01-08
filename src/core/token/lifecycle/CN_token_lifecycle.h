/**
 * @file CN_token_lifecycle.h
 * @brief CN_Language 令牌生命周期管理模块
 * 
 * 负责令牌的创建、销毁、复制等生命周期管理功能。
 * 遵循单一职责原则，专注于令牌生命周期管理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_TOKEN_LIFECYCLE_H
#define CN_TOKEN_LIFECYCLE_H

#include "../CN_token.h"

/**
 * @brief 创建令牌
 * 
 * 分配并初始化一个新的令牌结构体。
 * 
 * @param type 令牌类型
 * @param lexeme 词素字符串（会被复制）
 * @param line 行号
 * @param column 列号
 * @return Stru_Token_t* 新创建的令牌指针，失败返回NULL
 */
Stru_Token_t* F_token_lifecycle_create(Eum_TokenType type, const char* lexeme, size_t line, size_t column);

/**
 * @brief 销毁令牌
 * 
 * 释放令牌占用的所有内存。
 * 
 * @param token 要销毁的令牌指针
 */
void F_token_lifecycle_destroy(Stru_Token_t* token);

/**
 * @brief 复制令牌
 * 
 * 创建令牌的深拷贝。
 * 
 * @param token 要复制的令牌指针
 * @return Stru_Token_t* 新复制的令牌指针
 */
Stru_Token_t* F_token_lifecycle_copy(const Stru_Token_t* token);

/**
 * @brief 比较两个令牌
 * 
 * 比较两个令牌的类型和词素是否相同。
 * 
 * @param token1 第一个令牌
 * @param token2 第二个令牌
 * @return true 令牌相同
 * @return false 令牌不同
 */
bool F_token_lifecycle_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);

/**
 * @brief 获取令牌类型
 * 
 * @param token 令牌指针
 * @return Eum_TokenType 令牌类型
 */
Eum_TokenType F_token_lifecycle_get_type(const Stru_Token_t* token);

/**
 * @brief 获取令牌词素
 * 
 * @param token 令牌指针
 * @return const char* 词素字符串
 */
const char* F_token_lifecycle_get_lexeme(const Stru_Token_t* token);

/**
 * @brief 获取令牌位置
 * 
 * @param token 令牌指针
 * @param line 输出行号
 * @param column 输出列号
 */
void F_token_lifecycle_get_position(const Stru_Token_t* token, size_t* line, size_t* column);

#endif // CN_TOKEN_LIFECYCLE_H
