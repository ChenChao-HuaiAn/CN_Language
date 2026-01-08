/**
 * @file CN_lexer_interface.c
 * @brief CN_Language 词法分析器接口实现
 * 
 * 实现词法分析器抽象接口的工厂函数。
 * 使用新的模块化实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 */

#include "CN_lexer_interface.h"
#include "interface/CN_lexer_interface_impl.h"

/**
 * @brief 创建词法分析器接口实例
 * 
 * 使用新的模块化实现创建词法分析器接口。
 * 
 * @return 词法分析器接口指针，失败返回NULL
 */
Stru_LexerInterface_t* F_create_lexer_interface(void)
{
    return F_create_lexer_interface_impl();
}
