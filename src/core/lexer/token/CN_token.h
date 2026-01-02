/******************************************************************************
 * 文件名: CN_token.h
 * 功能: Token类型定义和接口声明
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义Token类型和基本接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_TOKEN_H
#define CN_TOKEN_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Token类型枚举
 * 
 * 定义了CN_Language中所有可能的Token类型，包括关键字、标识符、字面量、运算符等。
 * 遵循项目命名规范：枚举名以Eum_开头，枚举值以Eum_TOKEN_开头。
 */
enum Eum_TokenType
{
    // 关键字类型（对应CN语言的关键字）
    Eum_TOKEN_KEYWORD_VARIABLE,      // 变量
    Eum_TOKEN_KEYWORD_INTEGER,       // 整数
    Eum_TOKEN_KEYWORD_FLOAT,         // 小数
    Eum_TOKEN_KEYWORD_STRING,        // 字符串
    Eum_TOKEN_KEYWORD_BOOLEAN,       // 布尔
    Eum_TOKEN_KEYWORD_ARRAY,         // 数组
    Eum_TOKEN_KEYWORD_STRUCT,        // 结构体
    Eum_TOKEN_KEYWORD_ENUM,          // 枚举
    Eum_TOKEN_KEYWORD_POINTER,       // 指针
    Eum_TOKEN_KEYWORD_REFERENCE,     // 引用
    
    // 控制结构关键字
    Eum_TOKEN_KEYWORD_IF,            // 如果
    Eum_TOKEN_KEYWORD_ELSE,          // 否则
    Eum_TOKEN_KEYWORD_WHILE,         // 当
    Eum_TOKEN_KEYWORD_FOR,           // 循环
    Eum_TOKEN_KEYWORD_BREAK,         // 中断
    Eum_TOKEN_KEYWORD_CONTINUE,      // 继续
    Eum_TOKEN_KEYWORD_SWITCH,        // 选择
    Eum_TOKEN_KEYWORD_CASE,          // 情况
    Eum_TOKEN_KEYWORD_DEFAULT,       // 默认
    
    // 函数相关关键字
    Eum_TOKEN_KEYWORD_FUNCTION,      // 函数
    Eum_TOKEN_KEYWORD_RETURN,        // 返回
    Eum_TOKEN_KEYWORD_MAIN,          // 主程序
    Eum_TOKEN_KEYWORD_PARAM,         // 参数
    Eum_TOKEN_KEYWORD_RECURSIVE,     // 递归
    
    // 逻辑运算符关键字
    Eum_TOKEN_KEYWORD_AND,           // 与
    Eum_TOKEN_KEYWORD_AND2,          // 且
    Eum_TOKEN_KEYWORD_OR,            // 或
    Eum_TOKEN_KEYWORD_NOT,           // 非
    
    // 字面量关键字
    Eum_TOKEN_KEYWORD_TRUE,          // 真
    Eum_TOKEN_KEYWORD_FALSE,         // 假
    Eum_TOKEN_KEYWORD_NULL,          // 空
    Eum_TOKEN_KEYWORD_VOID,          // 无
    
    // 模块系统关键字
    Eum_TOKEN_KEYWORD_MODULE,        // 模块
    Eum_TOKEN_KEYWORD_IMPORT,        // 导入
    Eum_TOKEN_KEYWORD_EXPORT,        // 导出
    Eum_TOKEN_KEYWORD_PACKAGE,       // 包
    Eum_TOKEN_KEYWORD_NAMESPACE,     // 命名空间
    
    // 运算符关键字
    Eum_TOKEN_KEYWORD_ADD,           // 加
    Eum_TOKEN_KEYWORD_SUBTRACT,      // 减
    Eum_TOKEN_KEYWORD_MULTIPLY,      // 乘
    Eum_TOKEN_KEYWORD_DIVIDE,        // 除
    Eum_TOKEN_KEYWORD_MODULO,        // 取模
    Eum_TOKEN_KEYWORD_EQUAL,         // 等于
    Eum_TOKEN_KEYWORD_NOT_EQUAL,     // 不等于
    Eum_TOKEN_KEYWORD_LESS,          // 小于
    Eum_TOKEN_KEYWORD_GREATER,       // 大于
    Eum_TOKEN_KEYWORD_LESS_EQUAL,    // 小于等于
    Eum_TOKEN_KEYWORD_GREATER_EQUAL, // 大于等于
    
    // 类型关键字
    Eum_TOKEN_KEYWORD_TYPE,          // 类型
    Eum_TOKEN_KEYWORD_INTERFACE,     // 接口
    Eum_TOKEN_KEYWORD_CLASS,         // 类
    Eum_TOKEN_KEYWORD_OBJECT,        // 对象
    Eum_TOKEN_KEYWORD_GENERIC,       // 泛型
    Eum_TOKEN_KEYWORD_TEMPLATE,      // 模板
    
    // 其他关键字
    Eum_TOKEN_KEYWORD_CONST,         // 常量
    Eum_TOKEN_KEYWORD_STATIC,        // 静态
    Eum_TOKEN_KEYWORD_PUBLIC,        // 公开
    Eum_TOKEN_KEYWORD_PRIVATE,       // 私有
    Eum_TOKEN_KEYWORD_PROTECTED,     // 保护
    Eum_TOKEN_KEYWORD_VIRTUAL,       // 虚拟
    Eum_TOKEN_KEYWORD_OVERRIDE,      // 重写
    Eum_TOKEN_KEYWORD_ABSTRACT,      // 抽象
    Eum_TOKEN_KEYWORD_FINAL,         // 最终
    Eum_TOKEN_KEYWORD_SYNC,          // 同步
    Eum_TOKEN_KEYWORD_ASYNC,         // 异步
    Eum_TOKEN_KEYWORD_AWAIT,         // 等待
    Eum_TOKEN_KEYWORD_THROW,         // 抛出
    Eum_TOKEN_KEYWORD_CATCH,         // 捕获
    Eum_TOKEN_KEYWORD_TRY,           // 尝试
    Eum_TOKEN_KEYWORD_FINALLY,       // 最终（异常处理）
    
    // 标识符和字面量
    Eum_TOKEN_IDENTIFIER,            // 标识符（变量名、函数名等）
    Eum_TOKEN_INTEGER_LITERAL,       // 整数字面量
    Eum_TOKEN_FLOAT_LITERAL,         // 小数字面量
    Eum_TOKEN_STRING_LITERAL,        // 字符串字面量
    Eum_TOKEN_BOOLEAN_LITERAL,       // 布尔字面量
    
    // 运算符
    Eum_TOKEN_OPERATOR_PLUS,         // +
    Eum_TOKEN_OPERATOR_MINUS,        // -
    Eum_TOKEN_OPERATOR_MULTIPLY,     // *
    Eum_TOKEN_OPERATOR_DIVIDE,       // /
    Eum_TOKEN_OPERATOR_MODULO,       // %
    Eum_TOKEN_OPERATOR_ASSIGN,       // =
    Eum_TOKEN_OPERATOR_PLUS_ASSIGN,  // +=
    Eum_TOKEN_OPERATOR_MINUS_ASSIGN, // -=
    Eum_TOKEN_OPERATOR_MULTIPLY_ASSIGN, // *=
    Eum_TOKEN_OPERATOR_DIVIDE_ASSIGN,   // /=
    Eum_TOKEN_OPERATOR_EQUAL,        // ==
    Eum_TOKEN_OPERATOR_NOT_EQUAL,    // !=
    Eum_TOKEN_OPERATOR_LESS,         // <
    Eum_TOKEN_OPERATOR_GREATER,      // >
    Eum_TOKEN_OPERATOR_LESS_EQUAL,   // <=
    Eum_TOKEN_OPERATOR_GREATER_EQUAL,// >=
    
    // 分隔符和标点符号
    Eum_TOKEN_SEMICOLON,             // ;
    Eum_TOKEN_COMMA,                 // ,
    Eum_TOKEN_DOT,                   // .
    Eum_TOKEN_COLON,                 // :
    Eum_TOKEN_DOUBLE_COLON,          // ::
    Eum_TOKEN_LEFT_PAREN,            // (
    Eum_TOKEN_RIGHT_PAREN,           // )
    Eum_TOKEN_LEFT_BRACE,            // {
    Eum_TOKEN_RIGHT_BRACE,           // }
    Eum_TOKEN_LEFT_BRACKET,          // [
    Eum_TOKEN_RIGHT_BRACKET,         // ]
    
    // 特殊Token
    Eum_TOKEN_EOF,                   // 文件结束
    Eum_TOKEN_ERROR,                 // 错误Token
    Eum_TOKEN_COMMENT,               // 注释
    Eum_TOKEN_WHITESPACE,            // 空白字符
    Eum_TOKEN_NEWLINE                // 换行符
};

/**
 * @brief Token结构体
 * 
 * 表示一个词法单元，包含类型、值、位置等信息。
 * 遵循项目命名规范：结构体以Stru_前缀开头，以_t结尾。
 */
typedef struct Stru_Token_t
{
    enum Eum_TokenType type;         ///< Token类型
    char* lexeme;                    ///< Token的原始字符串表示
    size_t lexeme_length;            ///< 字符串长度
    int line_number;                 ///< 所在行号（从1开始）
    int column_number;               ///< 所在列号（从1开始）
    union
    {
        long integer_value;          ///< 整数值（如果Token是整数）
        double float_value;          ///< 浮点数值（如果Token是小数）
        bool boolean_value;          ///< 布尔值（如果Token是布尔）
    } literal_value;                 ///< 字面量值（仅对字面量Token有效）
} Stru_Token_t;

/**
 * @brief Token位置信息结构体
 * 
 * 用于记录Token在源代码中的位置，便于错误报告和调试。
 */
typedef struct Stru_TokenPosition_t
{
    int line_number;                 ///< 行号（从1开始）
    int column_number;               ///< 列号（从1开始）
    size_t offset;                   ///< 在源代码中的字节偏移量
    const char* filename;            ///< 源文件名
} Stru_TokenPosition_t;

/**
 * @brief 创建新的Token
 * 
 * @param type Token类型
 * @param lexeme Token的原始字符串（会被复制）
 * @param lexeme_length 字符串长度
 * @param line_number 行号
 * @param column_number 列号
 * @return 新创建的Token指针，失败返回NULL
 * 
 * @note 调用者负责使用F_token_destroy释放返回的Token
 */
Stru_Token_t* F_token_create(enum Eum_TokenType type, const char* lexeme, 
                             size_t lexeme_length, int line_number, int column_number);

/**
 * @brief 创建字面量Token
 * 
 * @param type 字面量类型（必须是Eum_TOKEN_INTEGER_LITERAL、Eum_TOKEN_FLOAT_LITERAL、
 *             Eum_TOKEN_BOOLEAN_LITERAL或Eum_TOKEN_STRING_LITERAL之一）
 * @param lexeme Token的原始字符串表示
 * @param lexeme_length 字符串长度
 * @param line_number 行号
 * @param column_number 列号
 * @param value 字面量值（根据类型不同，使用不同的union成员）
 * @return 新创建的Token指针，失败返回NULL
 */
Stru_Token_t* F_token_create_literal(enum Eum_TokenType type, const char* lexeme,
                                     size_t lexeme_length, int line_number, 
                                     int column_number, long value);

/**
 * @brief 销毁Token
 * 
 * @param token 要销毁的Token指针
 * 
 * @note 此函数会释放Token内部分配的所有内存，并将指针设为NULL
 */
void F_token_destroy(Stru_Token_t* token);

/**
 * @brief 复制Token
 * 
 * @param src 源Token
 * @return 新创建的Token副本，失败返回NULL
 */
Stru_Token_t* F_token_copy(const Stru_Token_t* src);

/**
 * @brief 获取Token类型名称
 * 
 * @param type Token类型枚举值
 * @return 类型名称字符串（静态常量，不需要释放）
 */
const char* F_token_type_to_string(enum Eum_TokenType type);

/**
 * @brief 获取Token类别
 * 
 * @param type Token类型
 * @return 类别字符串（"关键字"、"标识符"、"字面量"、"运算符"、"分隔符"等）
 */
const char* F_token_get_category(enum Eum_TokenType type);

/**
 * @brief 检查Token是否为关键字
 * 
 * @param type Token类型
 * @return 如果是关键字返回true，否则返回false
 */
bool F_token_is_keyword(enum Eum_TokenType type);

/**
 * @brief 检查Token是否为运算符
 * 
 * @param type Token类型
 * @return 如果是运算符返回true，否则返回false
 */
bool F_token_is_operator(enum Eum_TokenType type);

/**
 * @brief 检查Token是否为字面量
 * 
 * @param type Token类型
 * @return 如果是字面量返回true，否则返回false
 */
bool F_token_is_literal(enum Eum_TokenType type);

/**
 * @brief 检查Token是否为分隔符
 * 
 * @param type Token类型
 * @return 如果是分隔符返回true，否则返回false
 */
bool F_token_is_separator(enum Eum_TokenType type);

/**
 * @brief 打印Token信息
 * 
 * @param token 要打印的Token
 * @param show_position 是否显示位置信息
 * @param show_literal_value 是否显示字面量值
 */
void F_token_print(const Stru_Token_t* token, bool show_position, bool show_literal_value);

/**
 * @brief 获取Token的字符串表示
 * 
 * @param token Token
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括结尾的null字符）
 */
size_t F_token_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);

/**
 * @brief 比较两个Token是否相等
 * 
 * @param token1 第一个Token
 * @param token2 第二个Token
 * @return 如果两个Token类型和值都相等返回true，否则返回false
 */
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);

/**
 * @brief 获取Token的优先级（仅对运算符有效）
 * 
 * @param type Token类型（必须是运算符类型）
 * @return 优先级值（值越小优先级越高），如果不是运算符返回-1
 */
int F_token_get_precedence(enum Eum_TokenType type);

/**
 * @brief 获取Token的结合性
 * 
 * @param type Token类型（必须是运算符类型）
 * @return 结合性：0表示左结合，1表示右结合，-1表示不是运算符
 */
int F_token_get_associativity(enum Eum_TokenType type);

#endif // CN_TOKEN_H
