/**
 * @file CN_token_keywords.c
 * @brief CN_Language 令牌关键字管理模块实现
 * 
 * 实现令牌关键字信息的存储和查询。
 * 包含70个中文关键字的信息表。
 * 遵循单一职责原则，专注于关键字管理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_token_keywords.h"

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
const Stru_KeywordInfo_t* F_token_keywords_find_info(Eum_TokenType type)
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

// 获取关键字的中文表示
const char* F_token_keywords_get_chinese(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_token_keywords_find_info(type);
    return info != NULL ? info->chinese : "";
}

// 获取关键字的英文表示
const char* F_token_keywords_get_english(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_token_keywords_find_info(type);
    return info != NULL ? info->english : "";
}

// 获取关键字的分类
int F_token_keywords_get_category(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_token_keywords_find_info(type);
    return info != NULL ? info->category : 0;
}

// 获取关键字表大小
size_t F_token_keywords_get_count(void)
{
    return g_keyword_table_size;
}

// 获取所有关键字信息
const Stru_KeywordInfo_t* F_token_keywords_get_all(void)
{
    return g_keyword_table;
}
