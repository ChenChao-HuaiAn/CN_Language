// Token定义：词法分析器的输出单元（Task 1.1，Task 1.1a 补充自增/自减/问号）
// 覆盖：53个关键字、4种字面量、39个运算符、8个分隔符（含方括号双角色与问号）、标识符/文件结束/未知
// 命名规范：TokenType 枚举值统一使用英文（Kw_前缀表示关键字），tokenTypeToString 输出中文文本
#pragma once
#include <cstdint>
#include <string>
#include <utility>

#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {

// Token类型枚举：词法分析器产出的每种词法单元对应一个枚举值
enum class TokenType {
    // ==================== 关键字（54个） ====================

    // ---- 控制流关键字(10) ----
    Kw_If,            // 如果（条件分支）
    Kw_Else,          // 否则（否则分支）
    Kw_While,         // 当（条件循环）
    Kw_For,           // 循环（for风格循环）
    Kw_Return,        // 返回（函数返回）
    Kw_Break,         // 中断（跳出循环/选择）
    Kw_Continue,      // 继续（跳过本次循环）
    Kw_Switch,        // 选择（多分支选择）
    Kw_Case,          // 情况（分支标签）
    Kw_Default,       // 默认（默认分支）

    // ---- 类型关键字(21) ----
    Kw_Int,           // 整数（整32的别名）
    Kw_Double,        // 小数（浮64的别名）
    Kw_Int8,          // 整8（8位有符号整数）
    Kw_Int16,         // 整16（16位有符号整数）
    Kw_Int32,         // 整32（32位有符号整数）
    Kw_Int64,         // 整64（64位有符号整数）
    Kw_Int128,        // 整128（128位有符号整数）
    Kw_UInt8,         // 正8（8位无符号整数）
    Kw_UInt16,        // 正16（16位无符号整数）
    Kw_UInt32,        // 正32（32位无符号整数）
    Kw_UInt64,        // 正64（64位无符号整数）
    Kw_UInt128,       // 正128（128位无符号整数）
    Kw_Float32,       // 浮32（32位浮点）
    Kw_Float64,       // 浮64（64位浮点）
    Kw_Bool,          // 布尔（真/假）
    Kw_Char,          // 字符（4字节Unicode标量值）
    Kw_String,        // 字符串（UTF-8，以\0结尾）
    Kw_Void,          // 空类型（无返回值）
    Kw_Struct,        // 结构体（值类型聚合）
    Kw_Union,         // 联合体（所有字段共享同一内存区域，Task 2.7）
    Kw_Enum,          // 枚举（命名常量集合）

    // ---- 声明关键字(8) ----
    Kw_Function,      // 函数（函数定义/声明）
    Kw_Var,           // 变量（变量声明）
    Kw_Import,        // 导入（导入模块）
    Kw_From,          // 从（从模块导入）
    Kw_Public,        // 公开（公开可见性标签）
    Kw_Private,       // 私有（私有可见性标签）
    Kw_Static,        // 静态（静态变量/函数）
    Kw_Auto,          // 自动（类型推断声明，Task 2.10 lambda 赋值目标）

    // ---- 常量关键字(3) ----
    Kw_True,          // 真（布尔真值）
    Kw_False,         // 假（布尔假值）
    Kw_None,          // 无（空值/可选无值）

    // ---- OOP关键字(9) ----
    Kw_Class,         // 类（类定义）
    Kw_Interface,     // 接口（接口定义）
    Kw_Protected,     // 保护（保护可见性标签）
    Kw_Virtual,       // 虚拟（虚函数）
    Kw_Override,      // 重写（函数重写）
    Kw_Abstract,      // 抽象（抽象成员/类）
    Kw_Implements,    // 实现（接口实现）
    Kw_Self,          // 自身（访问自身成员）
    Kw_Super,         // 父类（调用父类方法）

    // ---- 错误处理关键字(2) ----
    Kw_Result,        // 结果（结果<T,E>类型）
    Kw_Optional,      // 可选（可选<T>类型）

    // ---- 字面量前缀关键字(2) ----
    Kw_Raw,           // 原始（原始字符串前缀）
    Kw_MultiLine,     // 多行（多行字符串前缀）

    // ==================== 标识符与字面量 ====================

    Identifier,        // 标识符（中文/ASCII标识符）
    IntegerLiteral,    // 整数字面量（42 / 0xFF / 0b1010 / 0o777，含后缀）
    FloatLiteral,      // 浮点字面量（3.14 / 1.5e10）
    StringLiteral,     // 字符串字面量（"你好" / 原始"..." / 多行"""..."""）
    CharLiteral,       // 字符字面量（'A' / '中' / '\u{4E2D}'）

    // ==================== 运算符（39个） ====================

    // ---- 算术运算符(7) ----
    PlusPlus,          // ++（自增）
    MinusMinus,        // --（自减）
    Plus,              // +
    Minus,             // -
    Star,              // *（乘法）
    Slash,             // /
    Percent,           // %
    // ---- 比较运算符(6) ----
    EqualEqual,        // ==
    BangEqual,         // !=
    Less,              // <
    LessEqual,         // <=
    Greater,           // >
    GreaterEqual,      // >=
    // ---- 逻辑运算符(3) ----
    AndAnd,            // &&
    OrOr,              // ||
    Bang,              // !
    // ---- 位运算符(6) ----
    Amp,               // &（按位与）
    Pipe,              // |（按位或）
    Caret,             // ^（按位异或）
    Tilde,             // ~（按位非）
    LessLess,          // <<（左移）
    GreaterGreater,    // >>（右移）
    // ---- 赋值运算符(11) ----
    Equal,             // =
    PlusEqual,         // +=
    MinusEqual,        // -=
    StarEqual,         // *=
    SlashEqual,        // /=
    PercentEqual,      // %=
    AmpEqual,          // &=
    PipeEqual,         // |=
    CaretEqual,        // ^=
    LessLessEqual,     // <<=
    GreaterGreaterEqual, // >>=
    // ---- 指针运算符(3) ----
    Arrow,             // ->（成员访问，通过指针）
    AmpAddress,        // &（取地址，一元）
    StarDeref,         // *（解引用，一元）
    // ---- 其他运算符(3) ----
    Dot,               // .（成员访问）
    LeftBracket,       // [（下标开）
    RightBracket,      // ]（下标关）

    // ==================== 分隔符（8个基础） ====================
    // 注：方括号[ ]同时扮演下标运算符与分隔符，见 isDelimiter()
    LeftParen,         // （
    RightParen,        // ）
    LeftBrace,         // {
    RightBrace,        // }
    Semicolon,         // ;
    Comma,             // ,
    Colon,             // :
    Question,          // ?（问号，条件表达式/可选链标记）

    // ==================== 其他 ====================

    EndOfFile,         // 文件结束（EOF）
    Unknown,           // 未知字符（用于错误恢复）
};

// Token：词法单元，携带类型、文本值、源码位置
class Token {
public:
    // 构造函数：指定类型、文本值（字面量原文/标识符名/运算符符号）、源码位置
    Token(TokenType type, std::string value, SourceLocation location)
        : type_(type), value_(std::move(value)), location_(location) {}

    // ---- getter方法 ----
    TokenType getType() const { return type_; }                     // 获取Token类型
    const std::string& getValue() const { return value_; }          // 获取文本值
    const SourceLocation& getLocation() const { return location_; } // 获取源码位置

    // ---- 静态工具方法 ----
    // TokenType转字符串（调试输出/错误报告，返回中文文本）
    static const std::string& tokenTypeToString(TokenType type);
    // 判断TokenType是否为关键字（54个中文关键字之一）
    static bool isKeyword(TokenType type);
    // 判断TokenType是否为字面量（整数/浮点/字符串/字符）
    static bool isLiteral(TokenType type);
    // 判断TokenType是否为运算符（39个运算符之一）
    static bool isOperator(TokenType type);
    // 判断TokenType是否为分隔符（括号/花括号/分号/逗号/冒号/问号/方括号）
    static bool isDelimiter(TokenType type);

    // ---- 实例分类方法（委托给静态方法） ----
    // 注：用 Token:: 限定调用静态重载，避免旧版GCC对同名实例方法/静态方法的解析歧义
    bool isKeyword() const { return Token::isKeyword(type_); }   // 本Token是否为关键字
    bool isLiteral() const { return Token::isLiteral(type_); }   // 本Token是否为字面量
    bool isOperator() const { return Token::isOperator(type_); } // 本Token是否为运算符
    bool isDelimiter() const { return Token::isDelimiter(type_); } // 本Token是否为分隔符

private:
    TokenType type_;          // Token类型
    std::string value_;       // Token文本值（字面量原文/标识符名/运算符符号）
    SourceLocation location_; // 源码位置
};

} // namespace cn_compiler
