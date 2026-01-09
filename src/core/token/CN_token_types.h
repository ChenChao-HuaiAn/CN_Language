/**
 * @file CN_token_types.h
 * @brief CN_Language 令牌类型定义
 * 
 * 定义所有令牌类型，包括70个中文关键字对应的枚举类型。
 * 按照语法规范分为9个类别。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#ifndef CN_TOKEN_TYPES_H
#define CN_TOKEN_TYPES_H

/**
 * @brief 令牌类型枚举
 * 
 * 包含所有CN_Language支持的令牌类型，包括70个中文关键字。
 * 按照语法规范分为9个类别。
 */
typedef enum Eum_TokenType {
    // ============================================
    // 1. 变量和类型声明关键字（10个）
    // ============================================
    Eum_TOKEN_KEYWORD_VAR,       ///< 变量 - 变量声明（类型推断）
    Eum_TOKEN_KEYWORD_INT,       ///< 整数 - 整数类型
    Eum_TOKEN_KEYWORD_FLOAT,     ///< 小数 - 浮点数类型
    Eum_TOKEN_KEYWORD_STRING,    ///< 字符串 - 字符串类型
    Eum_TOKEN_KEYWORD_BOOL,      ///< 布尔 - 布尔类型
    Eum_TOKEN_KEYWORD_ARRAY,     ///< 数组 - 数组类型
    Eum_TOKEN_KEYWORD_STRUCT,    ///< 结构体 - 结构体类型定义
    Eum_TOKEN_KEYWORD_ENUM,      ///< 枚举 - 枚举类型定义
    Eum_TOKEN_KEYWORD_POINTER,   ///< 指针 - 指针类型
    Eum_TOKEN_KEYWORD_REFERENCE, ///< 引用 - 引用类型

    // ============================================
    // 2. 控制结构关键字（9个）
    // ============================================
    Eum_TOKEN_KEYWORD_IF,        ///< 如果 - 条件语句
    Eum_TOKEN_KEYWORD_ELSE,      ///< 否则 - 条件语句的 else 分支
    Eum_TOKEN_KEYWORD_WHILE,     ///< 当 - while 循环
    Eum_TOKEN_KEYWORD_FOR,       ///< 循环 - for 循环
    Eum_TOKEN_KEYWORD_BREAK,     ///< 中断 - 跳出循环
    Eum_TOKEN_KEYWORD_CONTINUE,  ///< 继续 - 继续下一次循环
    Eum_TOKEN_KEYWORD_SWITCH,    ///< 选择 - 多分支选择语句（switch）
    Eum_TOKEN_KEYWORD_CASE,      ///< 情况 - switch语句的分支（case）
    Eum_TOKEN_KEYWORD_DEFAULT,   ///< 默认 - switch语句的默认分支（default）

    // ============================================
    // 3. 函数相关关键字（5个）
    // ============================================
    Eum_TOKEN_KEYWORD_FUNCTION,  ///< 函数 - 函数声明
    Eum_TOKEN_KEYWORD_RETURN,    ///< 返回 - 返回值
    Eum_TOKEN_KEYWORD_MAIN,      ///< 主程序 - 主函数声明
    Eum_TOKEN_KEYWORD_PARAM,     ///< 参数 - 函数参数
    Eum_TOKEN_KEYWORD_RECURSIVE, ///< 递归 - 递归函数

    // ============================================
    // 4. 逻辑运算符关键字（4个）
    // ============================================
    Eum_TOKEN_KEYWORD_AND,       ///< 与 - 逻辑与
    Eum_TOKEN_KEYWORD_AND2,      ///< 且 - 逻辑与（同义词）
    Eum_TOKEN_KEYWORD_OR,        ///< 或 - 逻辑或
    Eum_TOKEN_KEYWORD_NOT,       ///< 非 - 逻辑非

    // ============================================
    // 5. 字面量关键字（4个）
    // ============================================
    Eum_TOKEN_KEYWORD_TRUE,      ///< 真 - 布尔真值
    Eum_TOKEN_KEYWORD_FALSE,     ///< 假 - 布尔假值
    Eum_TOKEN_KEYWORD_NULL,      ///< 空 - 空值
    Eum_TOKEN_KEYWORD_VOID,      ///< 无 - 无类型/无返回值

    // ============================================
    // 6. 模块系统关键字（5个）
    // ============================================
    Eum_TOKEN_KEYWORD_MODULE,    ///< 模块 - 模块定义
    Eum_TOKEN_KEYWORD_IMPORT,    ///< 导入 - 导入模块
    Eum_TOKEN_KEYWORD_EXPORT,    ///< 导出 - 导出模块成员
    Eum_TOKEN_KEYWORD_PACKAGE,   ///< 包 - 包定义
    Eum_TOKEN_KEYWORD_NAMESPACE, ///< 命名空间 - 命名空间定义

    // ============================================
    // 7. 运算符关键字（11个）
    // ============================================
    Eum_TOKEN_KEYWORD_ADD,       ///< 加 - 加法运算符（优先级4）
    Eum_TOKEN_KEYWORD_SUBTRACT,  ///< 减 - 减法运算符（优先级4）
    Eum_TOKEN_KEYWORD_MULTIPLY,  ///< 乘 - 乘法运算符（优先级3）
    Eum_TOKEN_KEYWORD_DIVIDE,    ///< 除 - 除法运算符（优先级3）
    Eum_TOKEN_KEYWORD_MODULO,    ///< 取模 - 取模运算符（优先级3）
    Eum_TOKEN_KEYWORD_EQUAL,     ///< 等于 - 等于比较运算符（优先级7）
    Eum_TOKEN_KEYWORD_NOT_EQUAL, ///< 不等于 - 不等于比较运算符（优先级7）
    Eum_TOKEN_KEYWORD_LESS,      ///< 小于 - 小于比较运算符（优先级6）
    Eum_TOKEN_KEYWORD_GREATER,   ///< 大于 - 大于比较运算符（优先级6）
    Eum_TOKEN_KEYWORD_LESS_EQUAL, ///< 小于等于 - 小于等于比较运算符（优先级6）
    Eum_TOKEN_KEYWORD_GREATER_EQUAL, ///< 大于等于 - 大于等于比较运算符（优先级6）

    // ============================================
    // 8. 类型关键字（6个）
    // ============================================
    Eum_TOKEN_KEYWORD_TYPE,      ///< 类型 - 类型定义
    Eum_TOKEN_KEYWORD_INTERFACE, ///< 接口 - 接口定义
    Eum_TOKEN_KEYWORD_CLASS,     ///< 类 - 类定义
    Eum_TOKEN_KEYWORD_OBJECT,    ///< 对象 - 对象类型
    Eum_TOKEN_KEYWORD_GENERIC,   ///< 泛型 - 泛型类型
    Eum_TOKEN_KEYWORD_TEMPLATE,  ///< 模板 - 模板类型

    // ============================================
    // 9. 其他关键字（16个）
    // ============================================
    Eum_TOKEN_KEYWORD_CONST,     ///< 常量 - 常量声明
    Eum_TOKEN_KEYWORD_STATIC,    ///< 静态 - 静态声明
    Eum_TOKEN_KEYWORD_PUBLIC,    ///< 公开 - 公开访问修饰符
    Eum_TOKEN_KEYWORD_PRIVATE,   ///< 私有 - 私有访问修饰符
    Eum_TOKEN_KEYWORD_PROTECTED, ///< 保护 - 保护访问修饰符
    Eum_TOKEN_KEYWORD_VIRTUAL,   ///< 虚拟 - 虚拟函数
    Eum_TOKEN_KEYWORD_OVERRIDE,  ///< 重写 - 函数重写
    Eum_TOKEN_KEYWORD_ABSTRACT,  ///< 抽象 - 抽象类/方法
    Eum_TOKEN_KEYWORD_FINAL,     ///< 最终 - 最终类/方法
    Eum_TOKEN_KEYWORD_SYNC,      ///< 同步 - 同步方法
    Eum_TOKEN_KEYWORD_ASYNC,     ///< 异步 - 异步函数
    Eum_TOKEN_KEYWORD_AWAIT,     ///< 等待 - 等待异步操作
    Eum_TOKEN_KEYWORD_THROW,     ///< 抛出 - 抛出异常
    Eum_TOKEN_KEYWORD_CATCH,     ///< 捕获 - 捕获异常
    Eum_TOKEN_KEYWORD_TRY,       ///< 尝试 - 尝试执行代码块
    Eum_TOKEN_KEYWORD_FINALLY,   ///< 最终块 - 最终执行代码块（用于异常处理）

    // ============================================
    // 10. 非关键字令牌类型
    // ============================================
    
    // 标识符
    Eum_TOKEN_IDENTIFIER,        ///< 标识符 - 变量名、函数名等
    
    // 字面量
    Eum_TOKEN_LITERAL_INTEGER,   ///< 整数字面量
    Eum_TOKEN_LITERAL_FLOAT,     ///< 浮点数字面量
    Eum_TOKEN_LITERAL_STRING,    ///< 字符串字面量
    Eum_TOKEN_LITERAL_BOOLEAN,   ///< 布尔字面量
    
    // 运算符（符号形式）
    Eum_TOKEN_OPERATOR_PLUS,     ///< + 加法运算符
    Eum_TOKEN_OPERATOR_MINUS,    ///< - 减法运算符
    Eum_TOKEN_OPERATOR_MULTIPLY, ///< * 乘法运算符
    Eum_TOKEN_OPERATOR_DIVIDE,   ///< / 除法运算符
    Eum_TOKEN_OPERATOR_MODULO,   ///< % 取模运算符
    Eum_TOKEN_OPERATOR_ASSIGN,   ///< = 赋值运算符
    Eum_TOKEN_OPERATOR_EQUAL,    ///< == 等于运算符
    Eum_TOKEN_OPERATOR_NOT_EQUAL,///< != 不等于运算符
    Eum_TOKEN_OPERATOR_LESS,     ///< < 小于运算符
    Eum_TOKEN_OPERATOR_GREATER,  ///< > 大于运算符
    Eum_TOKEN_OPERATOR_LESS_EQUAL, ///< <= 小于等于运算符
    Eum_TOKEN_OPERATOR_GREATER_EQUAL, ///< >= 大于等于运算符
    
    // 分隔符
    Eum_TOKEN_DELIMITER_COMMA,   ///< , 逗号分隔符
    Eum_TOKEN_DELIMITER_SEMICOLON, ///< ; 分号分隔符
    Eum_TOKEN_DELIMITER_COLON,   ///< : 冒号分隔符（用于类型注解）
    Eum_TOKEN_DELIMITER_LPAREN,  ///< ( 左括号
    Eum_TOKEN_DELIMITER_RPAREN,  ///< ) 右括号
    Eum_TOKEN_DELIMITER_LBRACE,  ///< { 左花括号
    Eum_TOKEN_DELIMITER_RBRACE,  ///< } 右花括号
    Eum_TOKEN_DELIMITER_LBRACKET, ///< [ 左方括号
    Eum_TOKEN_DELIMITER_RBRACKET, ///< ] 右方括号
    
    // 特殊令牌
    Eum_TOKEN_EOF,               ///< 文件结束
    Eum_TOKEN_ERROR              ///< 错误令牌
} Eum_TokenType;

#endif // CN_TOKEN_TYPES_H
