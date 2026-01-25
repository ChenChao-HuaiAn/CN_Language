#ifndef CN_FRONTEND_TOKEN_H
#define CN_FRONTEND_TOKEN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CnTokenKind {
    CN_TOKEN_INVALID = 0,   // 无效的标记
    CN_TOKEN_IDENT,         // 标识符
    CN_TOKEN_INTEGER,       // 整数
    CN_TOKEN_FLOAT_LITERAL, // 浮点数字面量
    CN_TOKEN_STRING_LITERAL, // 字符串字面量
    CN_TOKEN_KEYWORD_IF,      // 如果
    CN_TOKEN_KEYWORD_ELSE,    // 否则
    CN_TOKEN_KEYWORD_FN,      // 函数
    CN_TOKEN_KEYWORD_RETURN,  // 返回
    CN_TOKEN_KEYWORD_AND,    // 与
    CN_TOKEN_KEYWORD_OR,     // 或
    CN_TOKEN_KEYWORD_VAR,     // 变量
    CN_TOKEN_KEYWORD_INT,      // 整数
    CN_TOKEN_KEYWORD_FLOAT,    // 浮点数
    CN_TOKEN_KEYWORD_STRING,  // 字符串
    CN_TOKEN_KEYWORD_BOOL,    // 布尔值
    CN_TOKEN_KEYWORD_ARRAY,   // 数组
    CN_TOKEN_KEYWORD_STRUCT,  // 结构体
    CN_TOKEN_KEYWORD_ENUM,    // 枚举
    CN_TOKEN_KEYWORD_WHILE,   // 当
    CN_TOKEN_KEYWORD_FOR,     // 循环
    CN_TOKEN_KEYWORD_BREAK,   // 中断
    CN_TOKEN_KEYWORD_CONTINUE, // 继续
    CN_TOKEN_KEYWORD_SWITCH,   // 选择
    CN_TOKEN_KEYWORD_CASE,     // 情况
    CN_TOKEN_KEYWORD_DEFAULT,  // 默认
    CN_TOKEN_KEYWORD_MAIN,      // 主程序
    CN_TOKEN_KEYWORD_TRUE,     // 真
    CN_TOKEN_KEYWORD_FALSE,    // 假
    CN_TOKEN_KEYWORD_NULL,     // 无
    CN_TOKEN_KEYWORD_VOID,     // 空类型
    CN_TOKEN_KEYWORD_MODULE,   // 模块
    CN_TOKEN_KEYWORD_IMPORT,   // 导入
    CN_TOKEN_KEYWORD_NAMESPACE, // 命名空间
    CN_TOKEN_KEYWORD_INTERFACE, // 接口
    CN_TOKEN_KEYWORD_CLASS,     // 类
    CN_TOKEN_KEYWORD_TEMPLATE, // 模板
    CN_TOKEN_KEYWORD_CONST,    // 常量
    CN_TOKEN_KEYWORD_STATIC,   // 静态
    CN_TOKEN_KEYWORD_PUBLIC,   // 公开
    CN_TOKEN_KEYWORD_PRIVATE,   // 私有
    CN_TOKEN_KEYWORD_PROTECTED, // 保护
    CN_TOKEN_KEYWORD_VIRTUAL,   // 虚拟
    CN_TOKEN_KEYWORD_OVERRIDE, // 重写
    CN_TOKEN_KEYWORD_ABSTRACT, // 抽象
    CN_TOKEN_PLUS,             // 加 +
    CN_TOKEN_MINUS,            // 减 -
    CN_TOKEN_STAR,             // 乘 *
    CN_TOKEN_SLASH,            // 除 /
    CN_TOKEN_PERCENT,          // 取模 %
    CN_TOKEN_EQUAL,            // 赋值 =
    CN_TOKEN_EQUAL_EQUAL,      // 相等 ==
    CN_TOKEN_BANG,             // 逻辑非/取反 !
    CN_TOKEN_BANG_EQUAL,       // 不等 !=
    CN_TOKEN_LESS,             // 小于 <
    CN_TOKEN_LESS_EQUAL,       // 小于等于 <=
    CN_TOKEN_GREATER,          // 大于 >
    CN_TOKEN_GREATER_EQUAL,    // 大于等于 >=
    CN_TOKEN_LOGICAL_AND,      // 逻辑与 &&
    CN_TOKEN_LOGICAL_OR,       // 逻辑或 ||
    CN_TOKEN_AMPERSAND,        // 取地址 &
    CN_TOKEN_LPAREN,           // 左括号 (
    CN_TOKEN_RPAREN,           // 右括号 )
    CN_TOKEN_LBRACE,           // 左大括号 {
    CN_TOKEN_RBRACE,           // 右大括号 }
    CN_TOKEN_LBRACKET,         // 左中括号 [
    CN_TOKEN_RBRACKET,         // 右中括号 ]
    CN_TOKEN_SEMICOLON,        // 分号 ;
    CN_TOKEN_COMMA,            // 逗号 ,
    CN_TOKEN_DOT,              // 点 .
    CN_TOKEN_COLON,            // 冒号 :
    CN_TOKEN_ARROW,            // 箭头 ->
    CN_TOKEN_EOF,              // 文件结束
    
} CnTokenKind;

typedef struct CnToken {
    CnTokenKind kind;
    const char *lexeme_begin;
    size_t lexeme_length;
    int line;
    int column;
} CnToken;

const char *cn_frontend_token_kind_name(CnTokenKind kind);
void cn_frontend_token_print(const CnToken *token);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_TOKEN_H */
