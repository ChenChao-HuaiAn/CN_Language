/**
 * @file CN_lexer_keywords.c
 * @brief CN_Language 词法分析器关键字识别模块实现
 * 
 * 实现中文关键字的识别功能，支持70个中文关键字。
 * 负责将标识符词素映射为对应的关键字令牌类型。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_lexer_keywords.h"
#include <string.h>

/**
 * @brief 检查是否为变量和类型声明关键字
 */
Eum_TokenType F_identify_var_type_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 变量和类型声明关键字
    if (strcmp(lexeme, "变量") == 0) return Eum_TOKEN_KEYWORD_VAR;
    if (strcmp(lexeme, "整数") == 0) return Eum_TOKEN_KEYWORD_INT;
    if (strcmp(lexeme, "小数") == 0) return Eum_TOKEN_KEYWORD_FLOAT;
    if (strcmp(lexeme, "字符串") == 0) return Eum_TOKEN_KEYWORD_STRING;
    if (strcmp(lexeme, "布尔") == 0) return Eum_TOKEN_KEYWORD_BOOL;
    if (strcmp(lexeme, "数组") == 0) return Eum_TOKEN_KEYWORD_ARRAY;
    if (strcmp(lexeme, "结构体") == 0) return Eum_TOKEN_KEYWORD_STRUCT;
    if (strcmp(lexeme, "枚举") == 0) return Eum_TOKEN_KEYWORD_ENUM;
    if (strcmp(lexeme, "指针") == 0) return Eum_TOKEN_KEYWORD_POINTER;
    if (strcmp(lexeme, "引用") == 0) return Eum_TOKEN_KEYWORD_REFERENCE;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 检查是否为控制结构关键字
 */
Eum_TokenType F_identify_control_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 控制结构关键字
    if (strcmp(lexeme, "如果") == 0) return Eum_TOKEN_KEYWORD_IF;
    if (strcmp(lexeme, "否则") == 0) return Eum_TOKEN_KEYWORD_ELSE;
    if (strcmp(lexeme, "当") == 0) return Eum_TOKEN_KEYWORD_WHILE;
    if (strcmp(lexeme, "循环") == 0) return Eum_TOKEN_KEYWORD_FOR;
    if (strcmp(lexeme, "中断") == 0) return Eum_TOKEN_KEYWORD_BREAK;
    if (strcmp(lexeme, "继续") == 0) return Eum_TOKEN_KEYWORD_CONTINUE;
    if (strcmp(lexeme, "选择") == 0) return Eum_TOKEN_KEYWORD_SWITCH;
    if (strcmp(lexeme, "情况") == 0) return Eum_TOKEN_KEYWORD_CASE;
    if (strcmp(lexeme, "默认") == 0) return Eum_TOKEN_KEYWORD_DEFAULT;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 检查是否为函数相关关键字
 */
Eum_TokenType F_identify_function_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 函数相关关键字
    if (strcmp(lexeme, "函数") == 0) return Eum_TOKEN_KEYWORD_FUNCTION;
    if (strcmp(lexeme, "返回") == 0) return Eum_TOKEN_KEYWORD_RETURN;
    if (strcmp(lexeme, "主程序") == 0) return Eum_TOKEN_KEYWORD_MAIN;
    if (strcmp(lexeme, "参数") == 0) return Eum_TOKEN_KEYWORD_PARAM;
    if (strcmp(lexeme, "递归") == 0) return Eum_TOKEN_KEYWORD_RECURSIVE;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 检查是否为逻辑运算符关键字
 */
Eum_TokenType F_identify_logic_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 逻辑运算符关键字
    if (strcmp(lexeme, "与") == 0) return Eum_TOKEN_KEYWORD_AND;
    if (strcmp(lexeme, "且") == 0) return Eum_TOKEN_KEYWORD_AND2;
    if (strcmp(lexeme, "或") == 0) return Eum_TOKEN_KEYWORD_OR;
    if (strcmp(lexeme, "非") == 0) return Eum_TOKEN_KEYWORD_NOT;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 检查是否为字面量关键字
 */
Eum_TokenType F_identify_literal_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 字面量关键字
    if (strcmp(lexeme, "真") == 0) return Eum_TOKEN_KEYWORD_TRUE;
    if (strcmp(lexeme, "假") == 0) return Eum_TOKEN_KEYWORD_FALSE;
    if (strcmp(lexeme, "空") == 0) return Eum_TOKEN_KEYWORD_NULL;
    if (strcmp(lexeme, "无") == 0) return Eum_TOKEN_KEYWORD_VOID;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 检查是否为模块系统关键字
 */
Eum_TokenType F_identify_module_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 模块系统关键字
    if (strcmp(lexeme, "模块") == 0) return Eum_TOKEN_KEYWORD_MODULE;
    if (strcmp(lexeme, "导入") == 0) return Eum_TOKEN_KEYWORD_IMPORT;
    if (strcmp(lexeme, "导出") == 0) return Eum_TOKEN_KEYWORD_EXPORT;
    if (strcmp(lexeme, "包") == 0) return Eum_TOKEN_KEYWORD_PACKAGE;
    if (strcmp(lexeme, "命名空间") == 0) return Eum_TOKEN_KEYWORD_NAMESPACE;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 检查是否为运算符关键字
 */
Eum_TokenType F_identify_operator_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 运算符关键字
    if (strcmp(lexeme, "加") == 0) return Eum_TOKEN_KEYWORD_ADD;
    if (strcmp(lexeme, "减") == 0) return Eum_TOKEN_KEYWORD_SUBTRACT;
    if (strcmp(lexeme, "乘") == 0) return Eum_TOKEN_KEYWORD_MULTIPLY;
    if (strcmp(lexeme, "除") == 0) return Eum_TOKEN_KEYWORD_DIVIDE;
    if (strcmp(lexeme, "取模") == 0) return Eum_TOKEN_KEYWORD_MODULO;
    if (strcmp(lexeme, "等于") == 0) return Eum_TOKEN_KEYWORD_EQUAL;
    if (strcmp(lexeme, "不等于") == 0) return Eum_TOKEN_KEYWORD_NOT_EQUAL;
    if (strcmp(lexeme, "小于") == 0) return Eum_TOKEN_KEYWORD_LESS;
    if (strcmp(lexeme, "大于") == 0) return Eum_TOKEN_KEYWORD_GREATER;
    if (strcmp(lexeme, "小于等于") == 0) return Eum_TOKEN_KEYWORD_LESS_EQUAL;
    if (strcmp(lexeme, "大于等于") == 0) return Eum_TOKEN_KEYWORD_GREATER_EQUAL;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 检查是否为类型关键字
 */
Eum_TokenType F_identify_type_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 类型关键字（根据CN_token_types.h中的定义）
    if (strcmp(lexeme, "类型") == 0) return Eum_TOKEN_KEYWORD_TYPE;
    if (strcmp(lexeme, "接口") == 0) return Eum_TOKEN_KEYWORD_INTERFACE;
    if (strcmp(lexeme, "类") == 0) return Eum_TOKEN_KEYWORD_CLASS;
    if (strcmp(lexeme, "对象") == 0) return Eum_TOKEN_KEYWORD_OBJECT;
    if (strcmp(lexeme, "泛型") == 0) return Eum_TOKEN_KEYWORD_GENERIC;
    if (strcmp(lexeme, "模板") == 0) return Eum_TOKEN_KEYWORD_TEMPLATE;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 检查是否为其他关键字
 */
Eum_TokenType F_identify_other_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 其他关键字（根据CN_token_types.h中的定义）
    if (strcmp(lexeme, "常量") == 0) return Eum_TOKEN_KEYWORD_CONST;
    if (strcmp(lexeme, "静态") == 0) return Eum_TOKEN_KEYWORD_STATIC;
    if (strcmp(lexeme, "公开") == 0) return Eum_TOKEN_KEYWORD_PUBLIC;
    if (strcmp(lexeme, "私有") == 0) return Eum_TOKEN_KEYWORD_PRIVATE;
    if (strcmp(lexeme, "保护") == 0) return Eum_TOKEN_KEYWORD_PROTECTED;
    if (strcmp(lexeme, "虚拟") == 0) return Eum_TOKEN_KEYWORD_VIRTUAL;
    if (strcmp(lexeme, "重写") == 0) return Eum_TOKEN_KEYWORD_OVERRIDE;
    if (strcmp(lexeme, "抽象") == 0) return Eum_TOKEN_KEYWORD_ABSTRACT;
    if (strcmp(lexeme, "最终") == 0) return Eum_TOKEN_KEYWORD_FINAL;
    if (strcmp(lexeme, "同步") == 0) return Eum_TOKEN_KEYWORD_SYNC;
    if (strcmp(lexeme, "异步") == 0) return Eum_TOKEN_KEYWORD_ASYNC;
    if (strcmp(lexeme, "等待") == 0) return Eum_TOKEN_KEYWORD_AWAIT;
    if (strcmp(lexeme, "抛出") == 0) return Eum_TOKEN_KEYWORD_THROW;
    if (strcmp(lexeme, "捕获") == 0) return Eum_TOKEN_KEYWORD_CATCH;
    if (strcmp(lexeme, "尝试") == 0) return Eum_TOKEN_KEYWORD_TRY;
    if (strcmp(lexeme, "最终块") == 0) return Eum_TOKEN_KEYWORD_FINALLY;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 识别关键字
 */
Eum_TokenType F_identify_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 按类别检查关键字，提高效率
    Eum_TokenType type;
    
    // 1. 检查变量和类型声明关键字
    type = F_identify_var_type_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 2. 检查控制结构关键字
    type = F_identify_control_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 3. 检查函数相关关键字
    type = F_identify_function_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 4. 检查逻辑运算符关键字
    type = F_identify_logic_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 5. 检查字面量关键字
    type = F_identify_literal_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 6. 检查模块系统关键字
    type = F_identify_module_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 7. 检查运算符关键字
    type = F_identify_operator_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 8. 检查类型关键字
    type = F_identify_type_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 9. 检查其他关键字
    type = F_identify_other_keyword(lexeme);
    if (type != Eum_TOKEN_IDENTIFIER) return type;
    
    // 不是关键字，是普通标识符
    return Eum_TOKEN_IDENTIFIER;
}
