/**
 * @file CN_lexer_token_scanners.h
 * @brief CN_Language 词法分析器令牌扫描模块
 * 
 * 提供各种令牌类型的扫描功能，包括标识符、数字、字符串、运算符和分隔符。
 * 负责将字符序列转换为具体的令牌。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_LEXER_TOKEN_SCANNERS_H
#define CN_LEXER_TOKEN_SCANNERS_H

#include "../scanner/CN_lexer_scanner.h"
#include "../../token/CN_token.h"

/**
 * @brief 扫描标识符或关键字
 * 
 * 从当前位置开始扫描标识符或关键字，返回对应的令牌。
 * 
 * @param state 扫描器状态
 * @return Stru_Token_t* 标识符或关键字令牌
 * @retval NULL 扫描失败
 */
Stru_Token_t* F_scan_identifier(Stru_LexerScannerState_t* state);

/**
 * @brief 扫描数字字面量
 * 
 * 从当前位置开始扫描数字字面量（整数或浮点数），返回对应的令牌。
 * 
 * @param state 扫描器状态
 * @return Stru_Token_t* 数字字面量令牌
 * @retval NULL 扫描失败
 */
Stru_Token_t* F_scan_number(Stru_LexerScannerState_t* state);

/**
 * @brief 扫描字符串字面量
 * 
 * 从当前位置开始扫描字符串字面量，返回对应的令牌。
 * 
 * @param state 扫描器状态
 * @return Stru_Token_t* 字符串字面量令牌
 * @retval NULL 扫描失败
 */
Stru_Token_t* F_scan_string(Stru_LexerScannerState_t* state);

/**
 * @brief 扫描运算符
 * 
 * 从当前位置开始扫描运算符，返回对应的令牌。
 * 
 * @param state 扫描器状态
 * @return Stru_Token_t* 运算符令牌
 * @retval NULL 扫描失败
 */
Stru_Token_t* F_scan_operator(Stru_LexerScannerState_t* state);

/**
 * @brief 扫描分隔符
 * 
 * 从当前位置开始扫描分隔符，返回对应的令牌。
 * 
 * @param state 扫描器状态
 * @return Stru_Token_t* 分隔符令牌
 * @retval NULL 扫描失败
 */
Stru_Token_t* F_scan_delimiter(Stru_LexerScannerState_t* state);

/**
 * @brief 扫描下一个令牌
 * 
 * 根据当前位置的字符类型，调用相应的扫描函数，返回下一个令牌。
 * 
 * @param state 扫描器状态
 * @return Stru_Token_t* 下一个令牌
 * @retval NULL 扫描失败或到达文件末尾
 */
Stru_Token_t* F_scan_next_token(Stru_LexerScannerState_t* state);

/**
 * @brief 检查字符是否为字母（包括中文字符）
 * 
 * 检查字符是否为字母，支持ASCII字母和中文字符。
 * 
 * @param c 要检查的字符
 * @return true 是字母
 * @return false 不是字母
 */
bool F_is_alpha(char c);

/**
 * @brief 检查字符是否为数字
 * 
 * 检查字符是否为数字（0-9）。
 * 
 * @param c 要检查的字符
 * @return true 是数字
 * @return false 不是数字
 */
bool F_is_digit(char c);

/**
 * @brief 检查字符是否为字母或数字
 * 
 * 检查字符是否为字母或数字。
 * 
 * @param c 要检查的字符
 * @return true 是字母或数字
 * @return false 不是字母或数字
 */
bool F_is_alpha_numeric(char c);

/**
 * @brief 检查字符是否为运算符字符
 * 
 * 检查字符是否为运算符字符。
 * 
 * @param c 要检查的字符
 * @return true 是运算符字符
 * @return false 不是运算符字符
 */
bool F_is_operator_char(char c);

/**
 * @brief 检查字符是否为分隔符字符
 * 
 * 检查字符是否为分隔符字符。
 * 
 * @param c 要检查的字符
 * @return true 是分隔符字符
 * @return false 不是分隔符字符
 */
bool F_is_delimiter_char(char c);

#endif // CN_LEXER_TOKEN_SCANNERS_H
