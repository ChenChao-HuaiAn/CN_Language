/**
 * @file CN_token_keywords.h
 * @brief CN_Language 令牌关键字管理模块
 * 
 * 负责令牌关键字信息的存储和查询。
 * 包含70个中文关键字的信息表。
 * 遵循单一职责原则，专注于关键字管理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_TOKEN_KEYWORDS_H
#define CN_TOKEN_KEYWORDS_H

#include "../CN_token_types.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 关键字信息结构体
 * 
 * 存储关键字的详细信息，包括中文、英文、描述、分类和优先级。
 */
typedef struct Stru_KeywordInfo_t {
    Eum_TokenType type;        ///< 令牌类型
    const char* chinese;       ///< 中文关键字
    const char* english;       ///< 英文等价词
    const char* description;   ///< 功能描述
    int category;              ///< 分类（1-9）
    int precedence;            ///< 优先级（1-10，0表示非运算符）
} Stru_KeywordInfo_t;

/**
 * @brief 查找关键字信息
 * 
 * 根据令牌类型查找对应的关键字信息。
 * 
 * @param type 令牌类型
 * @return const Stru_KeywordInfo_t* 关键字信息指针，找不到返回NULL
 */
const Stru_KeywordInfo_t* F_token_keywords_find_info(Eum_TokenType type);

/**
 * @brief 获取关键字的中文表示
 * 
 * 对于关键字令牌，返回其中文字符串。
 * 对于非关键字令牌，返回空字符串。
 * 
 * @param type 令牌类型
 * @return const char* 中文关键字字符串
 */
const char* F_token_keywords_get_chinese(Eum_TokenType type);

/**
 * @brief 获取关键字的英文表示
 * 
 * 对于关键字令牌，返回其英文等价词。
 * 对于非关键字令牌，返回空字符串。
 * 
 * @param type 令牌类型
 * @return const char* 英文关键字字符串
 */
const char* F_token_keywords_get_english(Eum_TokenType type);

/**
 * @brief 获取关键字的分类
 * 
 * 对于关键字令牌，返回其分类（1-9）。
 * 对于非关键字令牌，返回0。
 * 
 * @param type 令牌类型
 * @return int 分类编号（1-9），0表示非关键字
 */
int F_token_keywords_get_category(Eum_TokenType type);

/**
 * @brief 获取关键字表大小
 * 
 * 返回关键字表中关键字的数量。
 * 
 * @return size_t 关键字数量
 */
size_t F_token_keywords_get_count(void);

/**
 * @brief 获取所有关键字信息
 * 
 * 返回关键字表的指针，用于遍历所有关键字。
 * 
 * @return const Stru_KeywordInfo_t* 关键字表指针
 */
const Stru_KeywordInfo_t* F_token_keywords_get_all(void);

#endif // CN_TOKEN_KEYWORDS_H
