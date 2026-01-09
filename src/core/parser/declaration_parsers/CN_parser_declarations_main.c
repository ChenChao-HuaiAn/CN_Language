/**
 * @file CN_parser_declarations_main.c
 * @brief CN_Language 声明解析主模块
 * 
 * 声明解析主模块，包含声明解析的入口函数和通用声明解析逻辑。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_declarations.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../error_handling/CN_parser_errors.h"
#include "../utils/CN_parser_utils.h"
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>

// ============================================
// 声明解析函数实现
// ============================================

/**
 * @brief 解析声明
 * 
 * 解析变量、函数、结构体或模块声明。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 声明AST节点，NULL表示错误
 */
Stru_AstNode_t* F_parse_declaration(Stru_ParserInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 根据当前令牌类型决定解析什么类型的声明
    switch (state->current_token->type)
    {
        case Eum_TOKEN_KEYWORD_VAR:
            // 变量声明
            return F_parse_variable_declaration(interface);
            
        case Eum_TOKEN_KEYWORD_FUNCTION:
            // 函数声明
            return F_parse_function_declaration(interface);
            
        case Eum_TOKEN_KEYWORD_STRUCT:
            // 结构体声明
            return F_parse_struct_declaration(interface);
            
        case Eum_TOKEN_KEYWORD_ENUM:
            // 枚举声明
            return F_parse_enum_declaration(interface);
            
        case Eum_TOKEN_KEYWORD_MODULE:
            // 模块声明
            return F_parse_module_declaration(interface);
            
        case Eum_TOKEN_KEYWORD_TYPE:
            // 类型声明
            return F_parse_type_declaration(interface);
            
        case Eum_TOKEN_KEYWORD_INTERFACE:
            // 接口声明
            return F_parse_interface_declaration(interface);
            
        case Eum_TOKEN_KEYWORD_CLASS:
            // 类声明
            return F_parse_class_declaration(interface);
            
        case Eum_TOKEN_KEYWORD_GENERIC:
            // 泛型声明
            return F_parse_generic_declaration(interface);
            
        default:
            // 不是声明，返回NULL
            return NULL;
    }
}
