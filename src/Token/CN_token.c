/******************************************************************************
 * 文件名: CN_token.c
 * 功能: Token类型实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现Token类型相关函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_token.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 创建新的Token
 */
Stru_Token_t* F_token_create(enum Eum_TokenType type, const char* lexeme, 
                             size_t lexeme_length, int line_number, int column_number)
{
    if (lexeme == NULL && lexeme_length > 0)
    {
        return NULL;
    }
    
    Stru_Token_t* token = (Stru_Token_t*)malloc(sizeof(Stru_Token_t));
    if (token == NULL)
    {
        return NULL;
    }
    
    token->type = type;
    token->line_number = line_number;
    token->column_number = column_number;
    
    if (lexeme != NULL && lexeme_length > 0)
    {
        token->lexeme = (char*)malloc(lexeme_length + 1);
        if (token->lexeme == NULL)
        {
            free(token);
            return NULL;
        }
        memcpy(token->lexeme, lexeme, lexeme_length);
        token->lexeme[lexeme_length] = '\0';
        token->lexeme_length = lexeme_length;
    }
    else
    {
        token->lexeme = NULL;
        token->lexeme_length = 0;
    }
    
    token->literal_value.integer_value = 0;
    
    return token;
}

/**
 * @brief 销毁Token
 */
void F_token_destroy(Stru_Token_t* token)
{
    if (token == NULL)
    {
        return;
    }
    
    if (token->lexeme != NULL)
    {
        free(token->lexeme);
        token->lexeme = NULL;
    }
    
    free(token);
}

/**
 * @brief 复制Token
 */
Stru_Token_t* F_token_copy(const Stru_Token_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    Stru_Token_t* dest = F_token_create(src->type, src->lexeme, src->lexeme_length,
                                        src->line_number, src->column_number);
    if (dest == NULL)
    {
        return NULL;
    }
    
    dest->literal_value = src->literal_value;
    
    return dest;
}

/**
 * @brief 获取Token类型名称
 */
const char* F_token_type_to_string(enum Eum_TokenType type)
{
    switch (type)
    {
        // 关键字
        case Eum_TOKEN_KEYWORD_VARIABLE:   return "变量";
        case Eum_TOKEN_KEYWORD_INTEGER:    return "整数";
        case Eum_TOKEN_KEYWORD_FLOAT:      return "小数";
        case Eum_TOKEN_KEYWORD_STRING:     return "字符串";
        case Eum_TOKEN_KEYWORD_BOOLEAN:    return "布尔";
        case Eum_TOKEN_KEYWORD_ARRAY:      return "数组";
        case Eum_TOKEN_KEYWORD_STRUCT:     return "结构体";
        case Eum_TOKEN_KEYWORD_ENUM:       return "枚举";
        case Eum_TOKEN_KEYWORD_POINTER:    return "指针";
        case Eum_TOKEN_KEYWORD_REFERENCE:  return "引用";
        case Eum_TOKEN_KEYWORD_IF:         return "如果";
        case Eum_TOKEN_KEYWORD_ELSE:       return "否则";
        case Eum_TOKEN_KEYWORD_WHILE:      return "当";
        case Eum_TOKEN_KEYWORD_FOR:        return "循环";
        case Eum_TOKEN_KEYWORD_BREAK:      return "中断";
        case Eum_TOKEN_KEYWORD_CONTINUE:   return "继续";
        case Eum_TOKEN_KEYWORD_FUNCTION:   return "函数";
        case Eum_TOKEN_KEYWORD_RETURN:     return "返回";
        case Eum_TOKEN_KEYWORD_MAIN:       return "主程序";
        case Eum_TOKEN_KEYWORD_AND:        return "与";
        case Eum_TOKEN_KEYWORD_OR:         return "或";
        case Eum_TOKEN_KEYWORD_NOT:        return "非";
        case Eum_TOKEN_KEYWORD_TRUE:       return "真";
        case Eum_TOKEN_KEYWORD_FALSE:      return "假";
        case Eum_TOKEN_KEYWORD_NULL:       return "空";
        case Eum_TOKEN_KEYWORD_MODULE:     return "模块";
        case Eum_TOKEN_KEYWORD_IMPORT:     return "导入";
        
        // 标识符和字面量
        case Eum_TOKEN_IDENTIFIER:         return "标识符";
        case Eum_TOKEN_INTEGER_LITERAL:    return "整数字面量";
        case Eum_TOKEN_FLOAT_LITERAL:      return "小数字面量";
        case Eum_TOKEN_STRING_LITERAL:     return "字符串字面量";
        case Eum_TOKEN_BOOLEAN_LITERAL:    return "布尔字面量";
        
        // 运算符
        case Eum_TOKEN_OPERATOR_PLUS:      return "+";
        case Eum_TOKEN_OPERATOR_MINUS:     return "-";
        case Eum_TOKEN_OPERATOR_MULTIPLY:  return "*";
        case Eum_TOKEN_OPERATOR_DIVIDE:    return "/";
        case Eum_TOKEN_OPERATOR_MODULO:    return "%";
        case Eum_TOKEN_OPERATOR_ASSIGN:    return "=";
        case Eum_TOKEN_OPERATOR_EQUAL:     return "==";
        case Eum_TOKEN_OPERATOR_NOT_EQUAL: return "!=";
        case Eum_TOKEN_OPERATOR_LESS:      return "<";
        case Eum_TOKEN_OPERATOR_GREATER:   return ">";
        case Eum_TOKEN_OPERATOR_LESS_EQUAL:return "<=";
        case Eum_TOKEN_OPERATOR_GREATER_EQUAL: return ">=";
        
        // 分隔符
        case Eum_TOKEN_SEMICOLON:          return ";";
        case Eum_TOKEN_COMMA:              return ",";
        case Eum_TOKEN_DOT:                return ".";
        case Eum_TOKEN_COLON:              return ":";
        case Eum_TOKEN_LEFT_PAREN:         return "(";
        case Eum_TOKEN_RIGHT_PAREN:        return ")";
        case Eum_TOKEN_LEFT_BRACE:         return "{";
        case Eum_TOKEN_RIGHT_BRACE:        return "}";
        case Eum_TOKEN_LEFT_BRACKET:       return "[";
        case Eum_TOKEN_RIGHT_BRACKET:      return "]";
        
        // 特殊Token
        case Eum_TOKEN_EOF:                return "文件结束";
        case Eum_TOKEN_ERROR:              return "错误";
        case Eum_TOKEN_COMMENT:            return "注释";
        case Eum_TOKEN_WHITESPACE:         return "空白字符";
        case Eum_TOKEN_NEWLINE:            return "换行符";
        
        default:                           return "未知Token类型";
    }
}

/**
 * @brief 获取Token类别
 */
const char* F_token_get_category(enum Eum_TokenType type)
{
    if (type >= Eum_TOKEN_KEYWORD_VARIABLE && type <= Eum_TOKEN_KEYWORD_FINALLY)
    {
        return "关键字";
    }
    else if (type >= Eum_TOKEN_OPERATOR_PLUS && type <= Eum_TOKEN_OPERATOR_GREATER_EQUAL)
    {
        return "运算符";
    }
    else if (type == Eum_TOKEN_IDENTIFIER)
    {
        return "标识符";
    }
    else if (type >= Eum_TOKEN_INTEGER_LITERAL && type <= Eum_TOKEN_BOOLEAN_LITERAL)
    {
        return "字面量";
    }
    else if (type >= Eum_TOKEN_SEMICOLON && type <= Eum_TOKEN_RIGHT_BRACKET)
    {
        return "分隔符";
    }
    else if (type == Eum_TOKEN_EOF)
    {
        return "文件结束";
    }
    else if (type == Eum_TOKEN_ERROR)
    {
        return "错误";
    }
    else if (type == Eum_TOKEN_COMMENT)
    {
        return "注释";
    }
    else if (type == Eum_TOKEN_WHITESPACE || type == Eum_TOKEN_NEWLINE)
    {
        return "空白字符";
    }
    else
    {
        return "未知";
    }
}

/**
 * @brief 检查Token是否为关键字
 */
bool F_token_is_keyword(enum Eum_TokenType type)
{
    return (type >= Eum_TOKEN_KEYWORD_VARIABLE && type <= Eum_TOKEN_KEYWORD_FINALLY);
}

/**
 * @brief 检查Token是否为运算符
 */
bool F_token_is_operator(enum Eum_TokenType type)
{
    return (type >= Eum_TOKEN_OPERATOR_PLUS && type <= Eum_TOKEN_OPERATOR_GREATER_EQUAL);
}

/**
 * @brief 检查Token是否为字面量
 */
bool F_token_is_literal(enum Eum_TokenType type)
{
    return (type == Eum_TOKEN_INTEGER_LITERAL ||
            type == Eum_TOKEN_FLOAT_LITERAL ||
            type == Eum_TOKEN_STRING_LITERAL ||
            type == Eum_TOKEN_BOOLEAN_LITERAL);
}

/**
 * @brief 检查Token是否为分隔符
 */
bool F_token_is_separator(enum Eum_TokenType type)
{
    return (type >= Eum_TOKEN_SEMICOLON && type <= Eum_TOKEN_RIGHT_BRACKET);
}

/**
 * @brief 打印Token信息
 */
void F_token_print(const Stru_Token_t* token, bool show_position, bool show_literal_value)
{
    if (token == NULL)
    {
        printf("Token: NULL\n");
        return;
    }
    
    const char* type_name = F_token_type_to_string(token->type);
    const char* category = F_token_get_category(token->type);
    
    printf("Token[%s]: ", category);
    
    if (token->lexeme != NULL)
    {
        printf("'%s' ", token->lexeme);
    }
    
    printf("(%s)", type_name);
    
    if (show_position)
    {
        printf(" 位置: %d:%d", token->line_number, token->column_number);
    }
    
    if (show_literal_value && F_token_is_literal(token->type))
    {
        switch (token->type)
        {
            case Eum_TOKEN_INTEGER_LITERAL:
                printf(" 值: %ld", token->literal_value.integer_value);
                break;
                
            case Eum_TOKEN_FLOAT_LITERAL:
                printf(" 值: %f", token->literal_value.float_value);
                break;
                
            case Eum_TOKEN_BOOLEAN_LITERAL:
                printf(" 值: %s", token->literal_value.boolean_value ? "真" : "假");
                break;
                
            default:
                break;
        }
    }
    
    printf("\n");
}

/**
 * @brief 比较两个Token是否相等
 */
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2)
{
    if (token1 == token2)
    {
        return true;
    }
    
    if (token1 == NULL || token2 == NULL)
    {
        return false;
    }
    
    if (token1->type != token2->type ||
        token1->line_number != token2->line_number ||
        token1->column_number != token2->column_number)
    {
        return false;
    }
    
    if (token1->lexeme == NULL && token2->lexeme == NULL)
    {
        // 两者都为NULL
    }
    else if (token1->lexeme == NULL || token2->lexeme == NULL)
    {
        return false;
    }
    else
    {
        if (strcmp(token1->lexeme, token2->lexeme) != 0)
        {
            return false;
        }
    }
    
    if (F_token_is_literal(token1->type))
    {
        switch (token1->type)
        {
            case Eum_TOKEN_INTEGER_LITERAL:
                if (token1->literal_value.integer_value != token2->literal_value.integer_value)
                {
                    return false;
                }
                break;
                
            case Eum_TOKEN_FLOAT_LITERAL:
                if (token1->literal_value.float_value != token2->literal_value.float_value)
                {
                    return false;
                }
                break;
                
            case Eum_TOKEN_BOOLEAN_LITERAL:
                if (token1->literal_value.boolean_value != token2->literal_value.boolean_value)
                {
                    return false;
                }
                break;
                
            default:
                break;
        }
    }
    
    return true;
}

/**
 * @brief 获取Token的优先级
 */
int F_token_get_precedence(enum Eum_TokenType type)
{
    if (!F_token_is_operator(type))
    {
        return -1;
    }
    
    switch (type)
    {
        case Eum_TOKEN_DOT:
        case Eum_TOKEN_LEFT_PAREN:
        case Eum_TOKEN_LEFT_BRACKET:
            return 1;
            
        case Eum_TOKEN_OPERATOR_MULTIPLY:
        case Eum_TOKEN_OPERATOR_DIVIDE:
        case Eum_TOKEN_OPERATOR_MODULO:
            return 3;
            
        case Eum_TOKEN_OPERATOR_PLUS:
        case Eum_TOKEN_OPERATOR_MINUS:
            return 4;
            
        case Eum_TOKEN_OPERATOR_LESS:
        case Eum_TOKEN_OPERATOR_GREATER:
        case Eum_TOKEN_OPERATOR_LESS_EQUAL:
        case Eum_TOKEN_OPERATOR_GREATER_EQUAL:
            return 6;
            
        case Eum_TOKEN_OPERATOR_EQUAL:
        case Eum_TOKEN_OPERATOR_NOT_EQUAL:
            return 7;
            
        case Eum_TOKEN_OPERATOR_ASSIGN:
            return 10;
            
        default:
            return -1;
    }
}

/**
 * @brief 获取Token的结合性
 */
int F_token_get_associativity(enum Eum_TokenType type)
{
    if (!F_token_is_operator(type))
    {
        return -1;
    }
    
    switch (type)
    {
        case Eum_TOKEN_OPERATOR_ASSIGN:
            return 1; // 右结合
            
        default:
            return 0; // 左结合
    }
}
