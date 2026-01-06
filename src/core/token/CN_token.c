/**
 * @file CN_token.c
 * @brief CN_Language 令牌操作函数实现
 * 
 * 实现令牌的创建、销毁、查询和操作函数。
 * 支持所有70个中文关键字的处理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#include "CN_token.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 关键字信息结构体
typedef struct {
    Eum_TokenType type;
    const char* chinese;      // 中文关键字
    const char* english;      // 英文等价词
    const char* description;  // 描述
    int category;             // 分类（1-9）
    int precedence;           // 优先级（1-10，0表示非运算符）
} Stru_KeywordInfo_t;

// 70个中文关键字的信息表
static const Stru_KeywordInfo_t g_keyword_table[] = {
    // 1. 变量和类型声明关键字（10个）
    {Eum_TOKEN_KEYWORD_VAR,       "变量",     "var",      "变量声明（类型推断）", 1, 0},
    {Eum_TOKEN_KEYWORD_INT,       "整数",     "int",      "整数类型", 1, 0},
    {Eum_TOKEN_KEYWORD_FLOAT,     "小数",     "float",    "浮点数类型", 1, 0},
    {Eum_TOKEN_KEYWORD_STRING,    "字符串",   "string",   "字符串类型", 1, 0},
    {Eum_TOKEN_KEYWORD_BOOL,      "布尔",     "bool",     "布尔类型", 1, 0},
    {Eum_TOKEN_KEYWORD_ARRAY,     "数组",     "array",    "数组类型", 1, 0},
    {Eum_TOKEN_KEYWORD_STRUCT,    "结构体",   "struct",   "结构体类型定义", 1, 0},
    {Eum_TOKEN_KEYWORD_ENUM,      "枚举",     "enum",     "枚举类型定义", 1, 0},
    {Eum_TOKEN_KEYWORD_POINTER,   "指针",     "pointer",  "指针类型", 1, 0},
    {Eum_TOKEN_KEYWORD_REFERENCE, "引用",     "reference","引用类型", 1, 0},
    
    // 2. 控制结构关键字（9个）
    {Eum_TOKEN_KEYWORD_IF,        "如果",     "if",       "条件语句", 2, 0},
    {Eum_TOKEN_KEYWORD_ELSE,      "否则",     "else",     "条件语句的 else 分支", 2, 0},
    {Eum_TOKEN_KEYWORD_WHILE,     "当",       "while",    "while 循环", 2, 0},
    {Eum_TOKEN_KEYWORD_FOR,       "循环",     "for",      "for 循环", 2, 0},
    {Eum_TOKEN_KEYWORD_BREAK,     "中断",     "break",    "跳出循环", 2, 0},
    {Eum_TOKEN_KEYWORD_CONTINUE,  "继续",     "continue", "继续下一次循环", 2, 0},
    {Eum_TOKEN_KEYWORD_SWITCH,    "选择",     "switch",   "多分支选择语句", 2, 0},
    {Eum_TOKEN_KEYWORD_CASE,      "情况",     "case",     "switch语句的分支", 2, 0},
    {Eum_TOKEN_KEYWORD_DEFAULT,   "默认",     "default",  "switch语句的默认分支", 2, 0},
    
    // 3. 函数相关关键字（5个）
    {Eum_TOKEN_KEYWORD_FUNCTION,  "函数",     "function", "函数声明", 3, 0},
    {Eum_TOKEN_KEYWORD_RETURN,    "返回",     "return",   "返回值", 3, 0},
    {Eum_TOKEN_KEYWORD_MAIN,      "主程序",   "main",     "主函数声明", 3, 0},
    {Eum_TOKEN_KEYWORD_PARAM,     "参数",     "param",    "函数参数", 3, 0},
    {Eum_TOKEN_KEYWORD_RECURSIVE, "递归",     "recursive","递归函数", 3, 0},
    
    // 4. 逻辑运算符关键字（4个）
    {Eum_TOKEN_KEYWORD_AND,       "与",       "and",      "逻辑与", 4, 8},
    {Eum_TOKEN_KEYWORD_AND2,      "且",       "and",      "逻辑与（同义词）", 4, 8},
    {Eum_TOKEN_KEYWORD_OR,        "或",       "or",       "逻辑或", 4, 9},
    {Eum_TOKEN_KEYWORD_NOT,       "非",       "not",      "逻辑非", 4, 3},
    
    // 5. 字面量关键字（4个）
    {Eum_TOKEN_KEYWORD_TRUE,      "真",       "true",     "布尔真值", 5, 0},
    {Eum_TOKEN_KEYWORD_FALSE,     "假",       "false",    "布尔假值", 5, 0},
    {Eum_TOKEN_KEYWORD_NULL,      "空",       "null",     "空值", 5, 0},
    {Eum_TOKEN_KEYWORD_VOID,      "无",       "void",     "无类型/无返回值", 5, 0},
    
    // 6. 模块系统关键字（5个）
    {Eum_TOKEN_KEYWORD_MODULE,    "模块",     "module",   "模块定义", 6, 0},
    {Eum_TOKEN_KEYWORD_IMPORT,    "导入",     "import",   "导入模块", 6, 0},
    {Eum_TOKEN_KEYWORD_EXPORT,    "导出",     "export",   "导出模块成员", 6, 0},
    {Eum_TOKEN_KEYWORD_PACKAGE,   "包",       "package",  "包定义", 6, 0},
    {Eum_TOKEN_KEYWORD_NAMESPACE, "命名空间", "namespace","命名空间定义", 6, 0},
    
    // 7. 运算符关键字（11个）
    {Eum_TOKEN_KEYWORD_ADD,       "加",       "add",      "加法运算符", 7, 4},
    {Eum_TOKEN_KEYWORD_SUBTRACT,  "减",       "subtract", "减法运算符", 7, 4},
    {Eum_TOKEN_KEYWORD_MULTIPLY,  "乘",       "multiply", "乘法运算符", 7, 3},
    {Eum_TOKEN_KEYWORD_DIVIDE,    "除",       "divide",   "除法运算符", 7, 3},
    {Eum_TOKEN_KEYWORD_MODULO,    "取模",     "modulo",   "取模运算符", 7, 3},
    {Eum_TOKEN_KEYWORD_EQUAL,     "等于",     "equal",    "等于比较运算符", 7, 7},
    {Eum_TOKEN_KEYWORD_NOT_EQUAL, "不等于",   "not_equal","不等于比较运算符", 7, 7},
    {Eum_TOKEN_KEYWORD_LESS,      "小于",     "less",     "小于比较运算符", 7, 6},
    {Eum_TOKEN_KEYWORD_GREATER,   "大于",     "greater",  "大于比较运算符", 7, 6},
    {Eum_TOKEN_KEYWORD_LESS_EQUAL,"小于等于", "less_equal","小于等于比较运算符", 7, 6},
    {Eum_TOKEN_KEYWORD_GREATER_EQUAL,"大于等于","greater_equal","大于等于比较运算符", 7, 6},
    
    // 8. 类型关键字（6个）
    {Eum_TOKEN_KEYWORD_TYPE,      "类型",     "type",     "类型定义", 8, 0},
    {Eum_TOKEN_KEYWORD_INTERFACE, "接口",     "interface","接口定义", 8, 0},
    {Eum_TOKEN_KEYWORD_CLASS,     "类",       "class",    "类定义", 8, 0},
    {Eum_TOKEN_KEYWORD_OBJECT,    "对象",     "object",   "对象类型", 8, 0},
    {Eum_TOKEN_KEYWORD_GENERIC,   "泛型",     "generic",  "泛型类型", 8, 0},
    {Eum_TOKEN_KEYWORD_TEMPLATE,  "模板",     "template", "模板类型", 8, 0},
    
    // 9. 其他关键字（16个）
    {Eum_TOKEN_KEYWORD_CONST,     "常量",     "const",    "常量声明", 9, 0},
    {Eum_TOKEN_KEYWORD_STATIC,    "静态",     "static",   "静态声明", 9, 0},
    {Eum_TOKEN_KEYWORD_PUBLIC,    "公开",     "public",   "公开访问修饰符", 9, 0},
    {Eum_TOKEN_KEYWORD_PRIVATE,   "私有",     "private",  "私有访问修饰符", 9, 0},
    {Eum_TOKEN_KEYWORD_PROTECTED, "保护",     "protected","保护访问修饰符", 9, 0},
    {Eum_TOKEN_KEYWORD_VIRTUAL,   "虚拟",     "virtual",  "虚拟函数", 9, 0},
    {Eum_TOKEN_KEYWORD_OVERRIDE,  "重写",     "override", "函数重写", 9, 0},
    {Eum_TOKEN_KEYWORD_ABSTRACT,  "抽象",     "abstract", "抽象类/方法", 9, 0},
    {Eum_TOKEN_KEYWORD_FINAL,     "最终",     "final",    "最终类/方法", 9, 0},
    {Eum_TOKEN_KEYWORD_SYNC,      "同步",     "sync",     "同步方法", 9, 0},
    {Eum_TOKEN_KEYWORD_ASYNC,     "异步",     "async",    "异步函数", 9, 0},
    {Eum_TOKEN_KEYWORD_AWAIT,     "等待",     "await",    "等待异步操作", 9, 0},
    {Eum_TOKEN_KEYWORD_THROW,     "抛出",     "throw",    "抛出异常", 9, 0},
    {Eum_TOKEN_KEYWORD_CATCH,     "捕获",     "catch",    "捕获异常", 9, 0},
    {Eum_TOKEN_KEYWORD_TRY,       "尝试",     "try",      "尝试执行代码块", 9, 0},
    {Eum_TOKEN_KEYWORD_FINALLY,   "最终",     "finally",  "最终执行代码块（用于异常处理）", 9, 0},
};

// 关键字表大小
static const size_t g_keyword_table_size = sizeof(g_keyword_table) / sizeof(g_keyword_table[0]);

// 查找关键字信息
static const Stru_KeywordInfo_t* F_find_keyword_info(Eum_TokenType type)
{
    for (size_t i = 0; i < g_keyword_table_size; i++)
    {
        if (g_keyword_table[i].type == type)
        {
            return &g_keyword_table[i];
        }
    }
    return NULL;
}

// 创建令牌
Stru_Token_t* F_create_token(Eum_TokenType type, const char* lexeme, size_t line, size_t column)
{
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    Stru_Token_t* token = (Stru_Token_t*)malloc(sizeof(Stru_Token_t));
    if (token == NULL)
    {
        return NULL;
    }
    
    // 初始化基本字段
    token->type = type;
    token->line = line;
    token->column = column;
    token->length = strlen(lexeme);
    
    // 复制词素
    token->lexeme = (char*)malloc(token->length + 1);
    if (token->lexeme == NULL)
    {
        free(token);
        return NULL;
    }
    strcpy(token->lexeme, lexeme);
    
    // 初始化字面量值
    token->literal_value.int_value = 0;
    
    return token;
}

// 销毁令牌
void F_destroy_token(Stru_Token_t* token)
{
    if (token == NULL)
    {
        return;
    }
    
    if (token->lexeme != NULL)
    {
        free(token->lexeme);
    }
    
    free(token);
}

// 令牌类型转字符串
const char* F_token_type_to_string(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_find_keyword_info(type);
    if (info != NULL)
    {
        return info->chinese;
    }
    
    // 非关键字令牌类型
    switch (type)
    {
        case Eum_TOKEN_IDENTIFIER:
            return "标识符";
        case Eum_TOKEN_LITERAL_INTEGER:
            return "整数字面量";
        case Eum_TOKEN_LITERAL_FLOAT:
            return "浮点数字面量";
        case Eum_TOKEN_LITERAL_STRING:
            return "字符串字面量";
        case Eum_TOKEN_LITERAL_BOOLEAN:
            return "布尔字面量";
        case Eum_TOKEN_OPERATOR_PLUS:
            return "+";
        case Eum_TOKEN_OPERATOR_MINUS:
            return "-";
        case Eum_TOKEN_OPERATOR_MULTIPLY:
            return "*";
        case Eum_TOKEN_OPERATOR_DIVIDE:
            return "/";
        case Eum_TOKEN_OPERATOR_MODULO:
            return "%";
        case Eum_TOKEN_OPERATOR_ASSIGN:
            return "=";
        case Eum_TOKEN_OPERATOR_EQUAL:
            return "==";
        case Eum_TOKEN_OPERATOR_NOT_EQUAL:
            return "!=";
        case Eum_TOKEN_OPERATOR_LESS:
            return "<";
        case Eum_TOKEN_OPERATOR_GREATER:
            return ">";
        case Eum_TOKEN_OPERATOR_LESS_EQUAL:
            return "<=";
        case Eum_TOKEN_OPERATOR_GREATER_EQUAL:
            return ">=";
        case Eum_TOKEN_DELIMITER_COMMA:
            return ",";
        case Eum_TOKEN_DELIMITER_SEMICOLON:
            return ";";
        case Eum_TOKEN_DELIMITER_LPAREN:
            return "(";
        case Eum_TOKEN_DELIMITER_RPAREN:
            return ")";
        case Eum_TOKEN_DELIMITER_LBRACE:
            return "{";
        case Eum_TOKEN_DELIMITER_RBRACE:
            return "}";
        case Eum_TOKEN_DELIMITER_LBRACKET:
            return "[";
        case Eum_TOKEN_DELIMITER_RBRACKET:
            return "]";
        case Eum_TOKEN_EOF:
            return "文件结束";
        case Eum_TOKEN_ERROR:
            return "错误";
        default:
            return "未知令牌类型";
    }
}

// 判断是否为关键字令牌
bool F_token_is_keyword(Eum_TokenType type)
{
    return F_find_keyword_info(type) != NULL;
}

// 判断是否为运算符令牌
bool F_token_is_operator(Eum_TokenType type)
{
    // 检查符号运算符
    if (type >= Eum_TOKEN_OPERATOR_PLUS && type <= Eum_TOKEN_OPERATOR_GREATER_EQUAL)
    {
        return true;
    }
    
    // 检查关键字运算符
    const Stru_KeywordInfo_t* info = F_find_keyword_info(type);
    if (info != NULL && info->precedence > 0)
    {
        return true;
    }
    
    return false;
}

// 判断是否为字面量令牌
bool F_token_is_literal(Eum_TokenType type)
{
    return (type >= Eum_TOKEN_LITERAL_INTEGER && type <= Eum_TOKEN_LITERAL_BOOLEAN) ||
           type == Eum_TOKEN_KEYWORD_TRUE ||
           type == Eum_TOKEN_KEYWORD_FALSE ||
           type == Eum_TOKEN_KEYWORD_NULL;
}

// 判断是否为分隔符令牌
bool F_token_is_delimiter(Eum_TokenType type)
{
    return type >= Eum_TOKEN_DELIMITER_COMMA && type <= Eum_TOKEN_DELIMITER_RBRACKET;
}

// 获取关键字优先级
int F_token_get_precedence(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_find_keyword_info(type);
    if (info != NULL)
    {
        return info->precedence;
    }
    
    // 符号运算符的优先级
    switch (type)
    {
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
        default:
            return 0;
    }
}

// 获取关键字的中文表示
const char* F_token_get_chinese_keyword(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_find_keyword_info(type);
    return info != NULL ? info->chinese : "";
}

// 获取关键字的英文表示
const char* F_token_get_english_keyword(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_find_keyword_info(type);
    return info != NULL ? info->english : "";
}

// 获取关键字的分类
int F_token_get_keyword_category(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_find_keyword_info(type);
    return info != NULL ? info->category : 0;
}

// 设置整数字面量值
void F_token_set_int_value(Stru_Token_t* token, long value)
{
    if (token != NULL && token->type == Eum_TOKEN_LITERAL_INTEGER)
    {
        token->literal_value.int_value = value;
    }
}

// 设置浮点数字面量值
void F_token_set_float_value(Stru_Token_t* token, double value)
{
    if (token != NULL && token->type == Eum_TOKEN_LITERAL_FLOAT)
    {
        token->literal_value.float_value = value;
    }
}

// 设置布尔字面量值
void F_token_set_bool_value(Stru_Token_t* token, bool value)
{
    if (token != NULL && 
        (token->type == Eum_TOKEN_LITERAL_BOOLEAN || 
         token->type == Eum_TOKEN_KEYWORD_TRUE || 
         token->type == Eum_TOKEN_KEYWORD_FALSE))
    {
        token->literal_value.bool_value = value;
    }
}

// 获取整数字面量值
long F_token_get_int_value(const Stru_Token_t* token)
{
    if (token != NULL && token->type == Eum_TOKEN_LITERAL_INTEGER)
    {
        return token->literal_value.int_value;
    }
    return 0;
}

// 获取浮点数字面量值
double F_token_get_float_value(const Stru_Token_t* token)
{
    if (token != NULL && token->type == Eum_TOKEN_LITERAL_FLOAT)
    {
        return token->literal_value.float_value;
    }
    return 0.0;
}

// 获取布尔字面量值
bool F_token_get_bool_value(const Stru_Token_t* token)
{
    if (token != NULL && 
        (token->type == Eum_TOKEN_LITERAL_BOOLEAN || 
         token->type == Eum_TOKEN_KEYWORD_TRUE || 
         token->type == Eum_TOKEN_KEYWORD_FALSE))
    {
        return token->literal_value.bool_value;
    }
    return false;
}

// 复制令牌
Stru_Token_t* F_token_copy(const Stru_Token_t* token)
{
    if (token == NULL)
    {
        return NULL;
    }
    
    Stru_Token_t* new_token = F_create_token(token->type, token->lexeme, token->line, token->column);
    if (new_token == NULL)
    {
        return NULL;
    }
    
    // 复制字面量值
    switch (token->type)
    {
        case Eum_TOKEN_LITERAL_INTEGER:
            new_token->literal_value.int_value = token->literal_value.int_value;
            break;
        case Eum_TOKEN_LITERAL_FLOAT:
            new_token->literal_value.float_value = token->literal_value.float_value;
            break;
        case Eum_TOKEN_LITERAL_BOOLEAN:
        case Eum_TOKEN_KEYWORD_TRUE:
        case Eum_TOKEN_KEYWORD_FALSE:
            new_token->literal_value.bool_value = token->literal_value.bool_value;
            break;
        default:
            break;
    }
    
    return new_token;
}

// 比较两个令牌
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2)
{
    if (token1 == NULL || token2 == NULL)
    {
        return token1 == token2;
    }
    
    if (token1->type != token2->type)
    {
        return false;
    }
    
    if (strcmp(token1->lexeme, token2->lexeme) != 0)
    {
        return false;
    }
    
    if (token1->line != token2->line || token1->column != token2->column)
    {
        return false;
    }
    
    return true;
}

// 打印令牌信息
int F_token_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size)
{
    if (token == NULL || buffer == NULL || buffer_size == 0)
    {
        return 0;
    }
    
    const char* type_str = F_token_type_to_string(token->type);
    
    int written = snprintf(buffer, buffer_size, 
                          "令牌[类型=%s, 词素='%s', 位置=%zu:%zu]",
                          type_str, token->lexeme, token->line, token->column);
    
    // 添加字面量值信息
    if (token->type == Eum_TOKEN_LITERAL_INTEGER)
    {
        written += snprintf(buffer + written, buffer_size - written, 
                           ", 值=%ld", token->literal_value.int_value);
    }
    else if (token->type == Eum_TOKEN_LITERAL_FLOAT)
    {
        written += snprintf(buffer + written, buffer_size - written, 
                           ", 值=%f", token->literal_value.float_value);
    }
    else if (token->type == Eum_TOKEN_LITERAL_BOOLEAN || 
             token->type == Eum_TOKEN_KEYWORD_TRUE || 
             token->type == Eum_TOKEN_KEYWORD_FALSE)
    {
        written += snprintf(buffer + written, buffer_size - written, 
                           ", 值=%s", token->literal_value.bool_value ? "真" : "假");
    }
    
    return written;
}
