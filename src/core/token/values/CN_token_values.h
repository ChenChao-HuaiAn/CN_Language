/**
 * @file CN_token_values.h
 * @brief CN_Language 令牌字面量值操作模块
 * 
 * 负责令牌字面量值的设置和获取操作。
 * 遵循单一职责原则，专注于令牌值操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_TOKEN_VALUES_H
#define CN_TOKEN_VALUES_H

#include "../CN_token.h"

/**
 * @brief 设置整数字面量值
 * 
 * 为整数字面量令牌设置值。
 * 
 * @param token 令牌指针
 * @param value 整数值
 */
void F_token_values_set_int(Stru_Token_t* token, long value);

/**
 * @brief 设置浮点数字面量值
 * 
 * 为浮点数字面量令牌设置值。
 * 
 * @param token 令牌指针
 * @param value 浮点数值
 */
void F_token_values_set_float(Stru_Token_t* token, double value);

/**
 * @brief 设置布尔字面量值
 * 
 * 为布尔字面量令牌设置值。
 * 
 * @param token 令牌指针
 * @param value 布尔值
 */
void F_token_values_set_bool(Stru_Token_t* token, bool value);

/**
 * @brief 获取整数字面量值
 * 
 * 获取整数字面量令牌的值。
 * 
 * @param token 令牌指针
 * @return long 整数值
 */
long F_token_values_get_int(const Stru_Token_t* token);

/**
 * @brief 获取浮点数字面量值
 * 
 * 获取浮点数字面量令牌的值。
 * 
 * @param token 令牌指针
 * @return double 浮点数值
 */
double F_token_values_get_float(const Stru_Token_t* token);

/**
 * @brief 获取布尔字面量值
 * 
 * 获取布尔字面量令牌的值。
 * 
 * @param token 令牌指针
 * @return bool 布尔值
 */
bool F_token_values_get_bool(const Stru_Token_t* token);

#endif // CN_TOKEN_VALUES_H
