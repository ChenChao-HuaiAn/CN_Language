/**
 * @file CN_token_tools.h
 * @brief CN_Language 令牌工具模块
 * 
 * 负责令牌类型转换和字符串格式化等工具功能。
 * 遵循单一职责原则，专注于令牌工具功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_TOKEN_TOOLS_H
#define CN_TOKEN_TOOLS_H

#include "../CN_token.h"
#include <stdio.h>

/**
 * @brief 令牌类型转字符串
 * 
 * 将令牌类型转换为可读的字符串表示。
 * 
 * @param type 令牌类型
 * @return const char* 类型字符串表示
 */
const char* F_token_tools_type_to_string(Eum_TokenType type);

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
int F_token_tools_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);

/**
 * @brief 打印令牌信息到文件
 * 
 * 将令牌信息输出到文件流。
 * 
 * @param token 令牌指针
 * @param stream 文件流
 * @return int 写入的字符数
 */
int F_token_tools_print(const Stru_Token_t* token, FILE* stream);

/**
 * @brief 格式化令牌位置信息
 * 
 * 将令牌位置信息格式化为字符串。
 * 
 * @param line 行号
 * @param column 列号
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 写入的字符数
 */
int F_token_tools_format_position(size_t line, size_t column, char* buffer, size_t buffer_size);

#endif // CN_TOKEN_TOOLS_H
