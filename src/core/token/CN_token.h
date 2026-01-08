/**
 * @file CN_token.h
 * @brief CN_Language 令牌结构体和操作函数
 * 
 * 定义令牌结构体、创建和销毁函数，以及令牌类型判断辅助函数。
 * 与现有测试文件兼容。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#ifndef CN_TOKEN_H
#define CN_TOKEN_H

#include "CN_token_types.h"
#include "CN_token_interface.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 令牌结构体
 * 
 * 表示词法分析过程中识别出的一个令牌。
 * 包含类型、词素、位置信息和字面量值。
 */
typedef struct Stru_Token_t {
    Eum_TokenType type;        ///< 令牌类型
    char* lexeme;              ///< 词素（源代码中的字符串）
    size_t line;               ///< 行号（从1开始）
    size_t column;             ///< 列号（从1开始）
    size_t length;             ///< 词素长度（字节数）
    
    /**
     * @brief 字面量值联合体
     * 
     * 根据令牌类型存储相应的字面量值。
     */
    union {
        long int_value;        ///< 整数值（如果是整数字面量）
        double float_value;    ///< 浮点数值（如果是浮点数字面量）
        bool bool_value;       ///< 布尔值（如果是布尔字面量）
    } literal_value;
} Stru_Token_t;

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
Stru_Token_t* F_create_token(Eum_TokenType type, const char* lexeme, size_t line, size_t column);

/**
 * @brief 销毁令牌
 * 
 * 释放令牌占用的所有内存。
 * 
 * @param token 要销毁的令牌指针
 */
void F_destroy_token(Stru_Token_t* token);

/**
 * @brief 令牌类型转字符串
 * 
 * 将令牌类型转换为可读的字符串表示。
 * 
 * @param type 令牌类型
 * @return const char* 类型字符串表示
 */
const char* F_token_type_to_string(Eum_TokenType type);

/**
 * @brief 判断是否为关键字令牌
 * 
 * 检查令牌类型是否属于关键字类别。
 * 
 * @param type 令牌类型
 * @return true 是关键字
 * @return false 不是关键字
 */
bool F_token_is_keyword(Eum_TokenType type);

/**
 * @brief 判断是否为运算符令牌
 * 
 * 检查令牌类型是否属于运算符类别（包括符号运算符和关键字运算符）。
 * 
 * @param type 令牌类型
 * @return true 是运算符
 * @return false 不是运算符
 */
bool F_token_is_operator(Eum_TokenType type);

/**
 * @brief 判断是否为字面量令牌
 * 
 * 检查令牌类型是否属于字面量类别。
 * 
 * @param type 令牌类型
 * @return true 是字面量
 * @return false 不是字面量
 */
bool F_token_is_literal(Eum_TokenType type);

/**
 * @brief 判断是否为分隔符令牌
 * 
 * 检查令牌类型是否属于分隔符类别。
 * 
 * @param type 令牌类型
 * @return true 是分隔符
 * @return false 不是分隔符
 */
bool F_token_is_delimiter(Eum_TokenType type);

/**
 * @brief 获取关键字优先级
 * 
 * 对于运算符关键字，返回其优先级值（1-10，值越小优先级越高）。
 * 对于非运算符关键字，返回0。
 * 
 * @param type 令牌类型
 * @return int 优先级值（1-10），0表示非运算符关键字
 */
int F_token_get_precedence(Eum_TokenType type);

/**
 * @brief 获取关键字的中文表示
 * 
 * 对于关键字令牌，返回其中文字符串。
 * 对于非关键字令牌，返回空字符串。
 * 
 * @param type 令牌类型
 * @return const char* 中文关键字字符串
 */
const char* F_token_get_chinese_keyword(Eum_TokenType type);

/**
 * @brief 获取关键字的英文表示
 * 
 * 对于关键字令牌，返回其英文等价词。
 * 对于非关键字令牌，返回空字符串。
 * 
 * @param type 令牌类型
 * @return const char* 英文关键字字符串
 */
const char* F_token_get_english_keyword(Eum_TokenType type);

/**
 * @brief 获取关键字的分类
 * 
 * 对于关键字令牌，返回其分类（1-9）。
 * 对于非关键字令牌，返回0。
 * 
 * @param type 令牌类型
 * @return int 分类编号（1-9），0表示非关键字
 */
int F_token_get_keyword_category(Eum_TokenType type);

/**
 * @brief 设置整数字面量值
 * 
 * 为整数字面量令牌设置值。
 * 
 * @param token 令牌指针
 * @param value 整数值
 */
void F_token_set_int_value(Stru_Token_t* token, long value);

/**
 * @brief 设置浮点数字面量值
 * 
 * 为浮点数字面量令牌设置值。
 * 
 * @param token 令牌指针
 * @param value 浮点数值
 */
void F_token_set_float_value(Stru_Token_t* token, double value);

/**
 * @brief 设置布尔字面量值
 * 
 * 为布尔字面量令牌设置值。
 * 
 * @param token 令牌指针
 * @param value 布尔值
 */
void F_token_set_bool_value(Stru_Token_t* token, bool value);

/**
 * @brief 获取整数字面量值
 * 
 * 获取整数字面量令牌的值。
 * 
 * @param token 令牌指针
 * @return long 整数值
 */
long F_token_get_int_value(const Stru_Token_t* token);

/**
 * @brief 获取浮点数字面量值
 * 
 * 获取浮点数字面量令牌的值。
 * 
 * @param token 令牌指针
 * @return double 浮点数值
 */
double F_token_get_float_value(const Stru_Token_t* token);

/**
 * @brief 获取布尔字面量值
 * 
 * 获取布尔字面量令牌的值。
 * 
 * @param token 令牌指针
 * @return bool 布尔值
 */
bool F_token_get_bool_value(const Stru_Token_t* token);

/**
 * @brief 复制令牌
 * 
 * 创建令牌的深拷贝。
 * 
 * @param token 要复制的令牌指针
 * @return Stru_Token_t* 新复制的令牌指针
 */
Stru_Token_t* F_token_copy(const Stru_Token_t* token);

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
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);

/**
 * @brief 打印令牌信息
 * 
 * 将令牌信息格式化为字符串输出。
 * 
 * @param token 令牌指针
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 写入的字符数（不包括终止空字符）
 */
int F_token_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);

#endif // CN_TOKEN_H
