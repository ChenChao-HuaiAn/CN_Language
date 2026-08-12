// Token定义：词法分析器的输出单元（Task 1.1）
// 覆盖：53个关键字、4种字面量、37个运算符、9个分隔符（含方括号双角色）、标识符/文件结束/未知
#pragma once
#include <cstdint>
#include <string>
#include <utility>

#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {

// Token类型枚举：词法分析器产出的每种词法单元对应一个枚举值
enum class TokenType {
    // ==================== 关键字（53个） ====================

    // ---- 控制流关键字(10) ----
    关键字如果,        // 如果（条件分支）
    关键字否则,        // 否则（否则分支）
    关键字当,          // 当（条件循环）
    关键字循环,        // 循环（for风格循环）
    关键字返回,        // 返回（函数返回）
    关键字中断,        // 中断（跳出循环/选择）
    关键字继续,        // 继续（跳过本次循环）
    关键字选择,        // 选择（多分支选择）
    关键字情况,        // 情况（分支标签）
    关键字默认,        // 默认（默认分支）

    // ---- 类型关键字(20) ----
    关键字整数,        // 整数（整32的别名）
    关键字小数,        // 小数（浮64的别名）
    关键字整8,         // 整8（8位有符号整数）
    关键字整16,        // 整16（16位有符号整数）
    关键字整32,        // 整32（32位有符号整数）
    关键字整64,        // 整64（64位有符号整数）
    关键字整128,       // 整128（128位有符号整数）
    关键字正8,         // 正8（8位无符号整数）
    关键字正16,        // 正16（16位无符号整数）
    关键字正32,        // 正32（32位无符号整数）
    关键字正64,        // 正64（64位无符号整数）
    关键字正128,       // 正128（128位无符号整数）
    关键字浮32,        // 浮32（32位浮点）
    关键字浮64,        // 浮64（64位浮点）
    关键字布尔,        // 布尔（真/假）
    关键字字符,        // 字符（4字节Unicode标量值）
    关键字字符串,      // 字符串（UTF-8，以\0结尾）
    关键字空类型,      // 空类型（无返回值）
    关键字结构体,      // 结构体（值类型聚合）
    关键字枚举,        // 枚举（命名常量集合）

    // ---- 声明关键字(7) ----
    关键字函数,        // 函数（函数定义/声明）
    关键字变量,        // 变量（变量声明）
    关键字导入,        // 导入（导入模块）
    关键字从,          // 从（从模块导入）
    关键字公开,        // 公开（公开可见性标签）
    关键字私有,        // 私有（私有可见性标签）
    关键字静态,        // 静态（静态变量/函数）

    // ---- 常量关键字(3) ----
    关键字真,          // 真（布尔真值）
    关键字假,          // 假（布尔假值）
    关键字无,          // 无（空值/可选无值）

    // ---- OOP关键字(9) ----
    关键字类,          // 类（类定义）
    关键字接口,        // 接口（接口定义）
    关键字保护,        // 保护（保护可见性标签）
    关键字虚拟,        // 虚拟（虚函数）
    关键字重写,        // 重写（函数重写）
    关键字抽象,        // 抽象（抽象成员/类）
    关键字实现,        // 实现（接口实现）
    关键字自身,        // 自身（访问自身成员）
    关键字父类,        // 父类（调用父类方法）

    // ---- 错误处理关键字(2) ----
    关键字结果,        // 结果（结果<T,E>类型）
    关键字可选,        // 可选（可选<T>类型）

    // ---- 字面量前缀关键字(2) ----
    关键字原始,        // 原始（原始字符串前缀）
    关键字多行,        // 多行（多行字符串前缀）

    // ==================== 标识符与字面量 ====================

    Identifier,        // 标识符（中文/ASCII标识符）
    IntegerLiteral,    // 整数字面量（42 / 0xFF / 0b1010 / 0o777，含后缀）
    FloatLiteral,      // 浮点字面量（3.14 / 1.5e10）
    StringLiteral,     // 字符串字面量（"你好" / 原始"..." / 多行"""..."""）
    CharLiteral,       // 字符字面量（'A' / '中' / '\u{4E2D}'）

    // ==================== 运算符（37个） ====================

    // ---- 算术运算符(5) ----
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

    // ==================== 分隔符（7个基础） ====================
    // 注：方括号[ ]同时扮演下标运算符与分隔符，见 isDelimiter()
    LeftParen,         // （
    RightParen,        // ）
    LeftBrace,         // {
    RightBrace,        // }
    Semicolon,         // ;
    Comma,             // ,
    Colon,             // :

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
    // TokenType转字符串（调试输出/错误报告）
    static const std::string& tokenTypeToString(TokenType type);
    // 判断TokenType是否为关键字（53个中文关键字之一）
    static bool isKeyword(TokenType type);
    // 判断TokenType是否为字面量（整数/浮点/字符串/字符）
    static bool isLiteral(TokenType type);
    // 判断TokenType是否为运算符（37个运算符之一）
    static bool isOperator(TokenType type);
    // 判断TokenType是否为分隔符（括号/花括号/分号/逗号/冒号/方括号）
    static bool isDelimiter(TokenType type);

    // ---- 实例分类方法（委托给静态方法） ----
    bool isKeyword() const { return isKeyword(type_); }   // 本Token是否为关键字
    bool isLiteral() const { return isLiteral(type_); }   // 本Token是否为字面量
    bool isOperator() const { return isOperator(type_); } // 本Token是否为运算符
    bool isDelimiter() const { return isDelimiter(type_); } // 本Token是否为分隔符

private:
    TokenType type_;          // Token类型
    std::string value_;       // Token文本值（字面量原文/标识符名/运算符符号）
    SourceLocation location_; // 源码位置
};

} // namespace cn_compiler
