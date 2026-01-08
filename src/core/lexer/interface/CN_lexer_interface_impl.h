/**
 * @file CN_lexer_interface_impl.h
 * @brief CN_Language 词法分析器接口实现
 * 
 * 提供词法分析器接口的具体实现，包括工厂函数和接口包装器。
 * 遵循项目架构规范，实现模块解耦和依赖注入。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_LEXER_INTERFACE_IMPL_H
#define CN_LEXER_INTERFACE_IMPL_H

#include "../CN_lexer_interface.h"

/**
 * @brief 创建词法分析器接口实例（实现版本）
 * 
 * 创建并初始化词法分析器接口实例，设置所有函数指针。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_LexerInterface_t* 新创建的词法分析器接口实例
 * @retval NULL 创建失败
 */
Stru_LexerInterface_t* F_create_lexer_interface_impl(void);

#endif // CN_LEXER_INTERFACE_IMPL_H
