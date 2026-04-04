# CN Language 语法规范设计文档

> **文档说明**：本文档严格基于 `src` 目录下的实际源代码编写，每个语法规则都标注了代码来源。未在代码中实现的功能标注为"预留"或"未实现"。
>
> **生成时间**：2026-03-29
> **代码版本**：基于当前 src 目录源码
> **更新记录**：
> - 2026-04-04：新增字符字面量语法（单引号包围），新增函数原型声明语法（无函数体）
> - 2026-03-31：删除"模块"关键字声明形式，采用"文件即模块"设计
> - 2026-03-30：将`常量`关键字从"预留"更新为"已实现"，移动至声明关键字章节

---

## 目录

1. [关键字定义](#1-关键字定义)
2. [Token类型](#2-token类型)
3. [词法规则](#3-词法规则)
4. [类型系统](#4-类型系统)
5. [语法规则](#5-语法规则)
6. [模块系统](#6-模块系统)
7. [面向对象编程（OOP）语法](#7-面向对象编程oop语法)
8. [函数指针](#8-函数指针)
9. [命令行参数标准库](#9-命令行参数标准库)
10. [预留特性](#10-预留特性)
11. [已移除的特性](#11-已移除的特性)
12. [代码引用示例](#12-代码引用示例)
13. [附录](#13-附录)

---

## 1. 关键字定义

### 1.1 控制流关键字（10个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `如果` | `CN_TOKEN_KEYWORD_IF` | 条件判断 | [`keywords.c:10`](src/frontend/lexer/keywords.c:10) |
| `否则` | `CN_TOKEN_KEYWORD_ELSE` | 否则分支 | [`keywords.c:11`](src/frontend/lexer/keywords.c:11) |
| `当` | `CN_TOKEN_KEYWORD_WHILE` | 当循环 | [`keywords.c:12`](src/frontend/lexer/keywords.c:12) |
| `循环` | `CN_TOKEN_KEYWORD_FOR` | 循环 | [`keywords.c:13`](src/frontend/lexer/keywords.c:13) |
| `返回` | `CN_TOKEN_KEYWORD_RETURN` | 返回语句 | [`keywords.c:14`](src/frontend/lexer/keywords.c:14) |
| `中断` | `CN_TOKEN_KEYWORD_BREAK` | 中断循环 | [`keywords.c:15`](src/frontend/lexer/keywords.c:15) |
| `继续` | `CN_TOKEN_KEYWORD_CONTINUE` | 继续循环 | [`keywords.c:16`](src/frontend/lexer/keywords.c:16) |
| `选择` | `CN_TOKEN_KEYWORD_SWITCH` | 选择语句 | [`keywords.c:17`](src/frontend/lexer/keywords.c:17) |
| `情况` | `CN_TOKEN_KEYWORD_CASE` | 情况分支 | [`keywords.c:18`](src/frontend/lexer/keywords.c:18) |
| `默认` | `CN_TOKEN_KEYWORD_DEFAULT` | 默认分支 | [`keywords.c:19`](src/frontend/lexer/keywords.c:19) |

### 1.2 类型关键字（7个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `整数` | `CN_TOKEN_KEYWORD_INT` | 整数类型 | [`keywords.c:22`](src/frontend/lexer/keywords.c:22) |
| `小数` | `CN_TOKEN_KEYWORD_FLOAT` | 浮点数类型 | [`keywords.c:23`](src/frontend/lexer/keywords.c:23) |
| `字符串` | `CN_TOKEN_KEYWORD_STRING` | 字符串类型 | [`keywords.c:24`](src/frontend/lexer/keywords.c:24) |
| `布尔` | `CN_TOKEN_KEYWORD_BOOL` | 布尔类型 | [`keywords.c:25`](src/frontend/lexer/keywords.c:25) |
| `空类型` | `CN_TOKEN_KEYWORD_VOID` | 空类型 | [`keywords.c:26`](src/frontend/lexer/keywords.c:26) |
| `结构体` | `CN_TOKEN_KEYWORD_STRUCT` | 结构体类型 | [`keywords.c:27`](src/frontend/lexer/keywords.c:27) |
| `枚举` | `CN_TOKEN_KEYWORD_ENUM` | 枚举类型 | [`keywords.c:28`](src/frontend/lexer/keywords.c:28) |

### 1.3 声明关键字（7个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `函数` | `CN_TOKEN_KEYWORD_FN` | 函数声明 | [`keywords.c:31`](src/frontend/lexer/keywords.c:31) |
| `变量` | `CN_TOKEN_KEYWORD_VAR` | 变量声明 | [`keywords.c:32`](src/frontend/lexer/keywords.c:32) |
| `导入` | `CN_TOKEN_KEYWORD_IMPORT` | 导入语句 | [`keywords.c:34`](src/frontend/lexer/keywords.c:34) |
| `从` | `CN_TOKEN_KEYWORD_FROM` | 从...导入 | [`keywords.c:35`](src/frontend/lexer/keywords.c:35) |
| `公开` | `CN_TOKEN_KEYWORD_PUBLIC` | 公开可见性 | [`keywords.c:36`](src/frontend/lexer/keywords.c:36) |
| `私有` | `CN_TOKEN_KEYWORD_PRIVATE` | 私有可见性 | [`keywords.c:37`](src/frontend/lexer/keywords.c:37) |
| `静态` | `CN_TOKEN_KEYWORD_STATIC` | 静态局部变量 | [`keywords.c:38`](src/frontend/lexer/keywords.c:38) |

### 1.4 常量关键字（3个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `真` | `CN_TOKEN_KEYWORD_TRUE` | 布尔真值 | [`keywords.c:40`](src/frontend/lexer/keywords.c:40) |
| `假` | `CN_TOKEN_KEYWORD_FALSE` | 布尔假值 | [`keywords.c:41`](src/frontend/lexer/keywords.c:41) |
| `无` | `CN_TOKEN_KEYWORD_NULL` | 空值常量 | [`keywords.c:42`](src/frontend/lexer/keywords.c:42) |

### 1.5 异常处理关键字（4个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `尝试` | `CN_TOKEN_KEYWORD_TRY` | try块 | [`keywords.c:64`](src/frontend/lexer/keywords.c:64) |
| `捕获` | `CN_TOKEN_KEYWORD_CATCH` | catch块 | [`keywords.c:65`](src/frontend/lexer/keywords.c:65) |
| `抛出` | `CN_TOKEN_KEYWORD_THROW` | 抛出异常 | [`keywords.c:66`](src/frontend/lexer/keywords.c:66) |
| `最终` | `CN_TOKEN_KEYWORD_FINALLY` | finally块 | [`keywords.c:67`](src/frontend/lexer/keywords.c:67) |

### 1.6 OOP关键字（9个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `类` | `CN_TOKEN_KEYWORD_CLASS` | 类声明（已实现） | [`keywords.c:51`](src/frontend/lexer/keywords.c:51) |
| `接口` | `CN_TOKEN_KEYWORD_INTERFACE` | 接口声明（已实现） | [`keywords.c:52`](src/frontend/lexer/keywords.c:52) |
| `保护` | `CN_TOKEN_KEYWORD_PROTECTED` | 保护成员 | [`keywords.c:53`](src/frontend/lexer/keywords.c:53) |
| `虚拟` | `CN_TOKEN_KEYWORD_VIRTUAL` | 虚函数 | [`keywords.c:54`](src/frontend/lexer/keywords.c:54) |
| `重写` | `CN_TOKEN_KEYWORD_OVERRIDE` | 重写方法 | [`keywords.c:55`](src/frontend/lexer/keywords.c:55) |
| `抽象` | `CN_TOKEN_KEYWORD_ABSTRACT` | 抽象类/方法 | [`keywords.c:56`](src/frontend/lexer/keywords.c:56) |
| `实现` | `CN_TOKEN_KEYWORD_IMPLEMENTS` | 实现接口 | [`keywords.c:57`](src/frontend/lexer/keywords.c:57) |
| `自身` | `CN_TOKEN_KEYWORD_THIS` | this指针 | [`keywords.c:58`](src/frontend/lexer/keywords.c:58) |
| `基类` | `CN_TOKEN_KEYWORD_BASE` | 基类访问 | [`keywords.c:59`](src/frontend/lexer/keywords.c:59) |

### 1.7 预留关键字（1个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `命名空间` | `CN_TOKEN_KEYWORD_NAMESPACE` | 预留 | [`keywords.c:46`](src/frontend/lexer/keywords.c:46) |

---

## 2. Token类型

### 2.1 字面量Token

| Token类型 | 说明 | 代码来源 |
|-----------|------|----------|
| `CN_TOKEN_INTEGER` | 整数字面量 | [`token.h:13`](include/cnlang/frontend/token.h:13) |
| `CN_TOKEN_FLOAT_LITERAL` | 浮点数字面量 | [`token.h:14`](include/cnlang/frontend/token.h:14) |
| `CN_TOKEN_STRING_LITERAL` | 字符串字面量 | [`token.h:15`](include/cnlang/frontend/token.h:15) |
| `CN_TOKEN_CHAR_LITERAL` | 字符字面量 | [`token.h:16`](include/cnlang/frontend/token.h:16) |
| `CN_TOKEN_IDENT` | 标识符 | [`token.h:12`](include/cnlang/frontend/token.h:12) |

### 2.2 运算符Token

| Token类型 | 符号 | 说明 | 代码来源 |
|-----------|------|------|----------|
| `CN_TOKEN_PLUS` | `+` | 加法 | [`token.h:54`](include/cnlang/frontend/token.h:54) |
| `CN_TOKEN_PLUS_PLUS` | `++` | 自增 | [`token.h:55`](include/cnlang/frontend/token.h:55) |
| `CN_TOKEN_MINUS` | `-` | 减法 | [`token.h:56`](include/cnlang/frontend/token.h:56) |
| `CN_TOKEN_MINUS_MINUS` | `--` | 自减 | [`token.h:57`](include/cnlang/frontend/token.h:57) |
| `CN_TOKEN_STAR` | `*` | 乘法/指针 | [`token.h:58`](include/cnlang/frontend/token.h:58) |
| `CN_TOKEN_SLASH` | `/` | 除法 | [`token.h:59`](include/cnlang/frontend/token.h:59) |
| `CN_TOKEN_PERCENT` | `%` | 取模 | [`token.h:60`](include/cnlang/frontend/token.h:60) |
| `CN_TOKEN_EQUAL` | `=` | 赋值 | [`token.h:61`](include/cnlang/frontend/token.h:61) |
| `CN_TOKEN_EQUAL_EQUAL` | `==` | 相等比较 | [`token.h:62`](include/cnlang/frontend/token.h:62) |
| `CN_TOKEN_BANG` | `!` | 逻辑非 | [`token.h:63`](include/cnlang/frontend/token.h:63) |
| `CN_TOKEN_BANG_EQUAL` | `!=` | 不等比较 | [`token.h:64`](include/cnlang/frontend/token.h:64) |
| `CN_TOKEN_LESS` | `<` | 小于 | [`token.h:65`](include/cnlang/frontend/token.h:65) |
| `CN_TOKEN_LESS_EQUAL` | `<=` | 小于等于 | [`token.h:66`](include/cnlang/frontend/token.h:66) |
| `CN_TOKEN_GREATER` | `>` | 大于 | [`token.h:67`](include/cnlang/frontend/token.h:67) |
| `CN_TOKEN_GREATER_EQUAL` | `>=` | 大于等于 | [`token.h:68`](include/cnlang/frontend/token.h:68) |
| `CN_TOKEN_LOGICAL_AND` | `&&` | 逻辑与 | [`token.h:69`](include/cnlang/frontend/token.h:69) |
| `CN_TOKEN_LOGICAL_OR` | `\|\|` | 逻辑或 | [`token.h:70`](include/cnlang/frontend/token.h:70) |
| `CN_TOKEN_AMPERSAND` | `&` | 取地址 | [`token.h:71`](include/cnlang/frontend/token.h:71) |
| `CN_TOKEN_BITWISE_AND` | `&` | 按位与 | [`token.h:72`](include/cnlang/frontend/token.h:72) |
| `CN_TOKEN_BITWISE_OR` | `\|` | 按位或 | [`token.h:73`](include/cnlang/frontend/token.h:73) |
| `CN_TOKEN_BITWISE_XOR` | `^` | 按位异或 | [`token.h:74`](include/cnlang/frontend/token.h:74) |
| `CN_TOKEN_BITWISE_NOT` | `~` | 按位取反 | [`token.h:75`](include/cnlang/frontend/token.h:75) |
| `CN_TOKEN_LEFT_SHIFT` | `<<` | 左移 | [`token.h:76`](include/cnlang/frontend/token.h:76) |
| `CN_TOKEN_RIGHT_SHIFT` | `>>` | 右移 | [`token.h:77`](include/cnlang/frontend/token.h:77) |
| `CN_TOKEN_ARROW` | `->` | 箭头操作符 | [`token.h:89`](include/cnlang/frontend/token.h:89) |
| `CN_TOKEN_QUESTION` | `?` | 三元运算符 | [`token.h:88`](include/cnlang/frontend/token.h:88) |

### 2.3 分隔符Token

| Token类型 | 符号 | 说明 | 代码来源 |
|-----------|------|------|----------|
| `CN_TOKEN_LPAREN` | `(` | 左圆括号 | [`token.h:78`](include/cnlang/frontend/token.h:78) |
| `CN_TOKEN_RPAREN` | `)` | 右圆括号 | [`token.h:79`](include/cnlang/frontend/token.h:79) |
| `CN_TOKEN_LBRACE` | `{` | 左花括号 | [`token.h:80`](include/cnlang/frontend/token.h:80) |
| `CN_TOKEN_RBRACE` | `}` | 右花括号 | [`token.h:81`](include/cnlang/frontend/token.h:81) |
| `CN_TOKEN_LBRACKET` | `[` | 左方括号 | [`token.h:82`](include/cnlang/frontend/token.h:82) |
| `CN_TOKEN_RBRACKET` | `]` | 右方括号 | [`token.h:83`](include/cnlang/frontend/token.h:83) |
| `CN_TOKEN_SEMICOLON` | `;` | 分号 | [`token.h:84`](include/cnlang/frontend/token.h:84) |
| `CN_TOKEN_COMMA` | `,` | 逗号 | [`token.h:85`](include/cnlang/frontend/token.h:85) |
| `CN_TOKEN_DOT` | `.` | 点操作符 | [`token.h:86`](include/cnlang/frontend/token.h:86) |
| `CN_TOKEN_COLON` | `:` | 冒号 | [`token.h:87`](include/cnlang/frontend/token.h:87) |

---

## 3. 词法规则

### 3.1 标识符规则

**规则**：标识符以 `_`、字母或 UTF-8 多字节字符（中文等）开头，后续字符可以是字母、数字、`_` 或 UTF-8 字符。

**代码来源**：[`lexer.c:158-182`](src/frontend/lexer/lexer.c:158)

```c
static int is_identifier_start(unsigned char c)
{
    if (c == '_' || isalpha(c)) {
        return 1;
    }
    if (c >= 0x80) {  // UTF-8 多字节字符
        return 1;
    }
    return 0;
}

static int is_identifier_continue(unsigned char c)
{
    if (isalnum(c) || c == '_') {
        return 1;
    }
    if (c >= 0x80) {
        return 1;
    }
    return 0;
}
```

### 3.2 数字字面量规则

#### 3.2.1 整数字面量

支持以下格式：

| 格式 | 前缀 | 示例 | 代码来源 |
|------|------|------|----------|
| 十进制 | 无 | `123` | [`lexer.c:394-426`](src/frontend/lexer/lexer.c:394) |
| 十六进制 | `0x`/`0X` | `0xFF` | [`lexer.c:285-302`](src/frontend/lexer/lexer.c:285) |
| 二进制 | `0b`/`0B` | `0b1010` | [`lexer.c:305-330`](src/frontend/lexer/lexer.c:305) |
| 八进制 | `0o`/`0O` | `0o755` | [`lexer.c:333-358`](src/frontend/lexer/lexer.c:333) |

#### 3.2.2 浮点数字面量

支持以下格式：

| 格式 | 示例 | 代码来源 |
|------|------|----------|
| 小数形式 | `3.14` | [`lexer.c:367-379`](src/frontend/lexer/lexer.c:367) |
| 科学计数法 | `1e10`, `1.5e-3` | [`lexer.c:59-87`](src/frontend/lexer/lexer.c:59) |

#### 3.2.3 数字后缀

| 后缀 | 类型 | 代码来源 |
|------|------|----------|
| `f`/`F` | float32 | [`lexer.c:97-100`](src/frontend/lexer/lexer.c:97) |
| `L`/`l` | int32 | [`lexer.c:116-121`](src/frontend/lexer/lexer.c:116) |
| `LL`/`ll` | int64 | [`lexer.c:134-136`](src/frontend/lexer/lexer.c:134) |
| `U`/`u` | uint32 | [`lexer.c:132-133`](src/frontend/lexer/lexer.c:132) |
| `UL`/`ul` | uint64 | [`lexer.c:130-131`](src/frontend/lexer/lexer.c:130) |
| `ULL`/`ull` | uint64 | [`lexer.c:128-129`](src/frontend/lexer/lexer.c:128) |

### 3.3 字符串字面量

**规则**：字符串以双引号 `"` 包围，支持转义字符。

**代码来源**：[`lexer.c:255-276`](src/frontend/lexer/lexer.c:255)

```c
if (c == '"') {
    advance(lexer);
    c = current_char(lexer);
    while (c != '"' && c != '\0') {
        if (c == '\\') {  // 处理转义字符
            advance(lexer);
            c = current_char(lexer);
            if (c == '\0') break;
        }
        advance(lexer);
        c = current_char(lexer);
    }
    // ...
}
```

### 3.4 字符字面量

**规则**：字符以单引号 `'` 包围，包含单个字符或转义序列。

**语法**：
```
字符字面量 ::= "'" (普通字符 | 转义序列) "'"
转义序列 ::= "\" ("n" | "t" | "r" | "\" | "'" | "0" | "x" 十六进制序列)
```

**支持的转义序列**：

| 转义序列 | 含义 | ASCII值 |
|---------|------|---------|
| `\n` | 换行符 | 10 |
| `\t` | 水平制表符 | 9 |
| `\r` | 回车符 | 13 |
| `\\` | 反斜杠 | 92 |
| `\'` | 单引号 | 39 |
| `\0` | 空字符 | 0 |
| `\xHH` | 十六进制表示 | HH |

**示例**：
```cn
变量 ch = 'A';      // 普通字符
变量 换行 = '\n';   // 换行符
变量 制表 = '\t';   // 制表符
变量 反斜杠 = '\\'; // 反斜杠
变量 单引号 = '\''; // 单引号
变量 空字符 = '\0'; // 空字符
变量 十六进制 = '\x41'; // 'A' 的十六进制表示
```

**类型说明**：字符字面量的类型为 `整数`，值为对应字符的ASCII码。

**代码来源**：[`lexer.c:277-310`](src/frontend/lexer/lexer.c:277)

```c
if (c == '\'') {
    advance(lexer);
    c = current_char(lexer);
    
    if (c == '\\') {  // 处理转义字符
        advance(lexer);
        c = current_char(lexer);
        // 解析转义序列
        switch (c) {
            case 'n': char_value = '\n'; break;
            case 't': char_value = '\t'; break;
            case 'r': char_value = '\r'; break;
            case '\\': char_value = '\\'; break;
            case '\'': char_value = '\''; break;
            case '0': char_value = '\0'; break;
            case 'x': /* 解析十六进制 */ break;
        }
    } else {
        char_value = c;
    }
    
    advance(lexer);
    // 期望闭合的单引号
    if (current_char(lexer) == '\'') {
        advance(lexer);
    }
    // ...
}
```

### 3.4 注释规则

**规则**：支持单行注释 `//` 和块注释 `/* */`。

| 注释类型 | 语法格式 | 说明 | 代码来源 |
|---------|---------|------|----------|
| 单行注释 | `// 注释内容` | 注释内容到行尾 | [`lexer.c:229-236`](src/frontend/lexer/lexer.c:229) |
| 块注释 | `/* 注释内容 */` | 可跨多行的注释 | [`lexer.c:238-262`](src/frontend/lexer/lexer.c:238) |

**错误处理**：未闭合的块注释会报告 `CN_DIAG_CODE_LEX_UNTERMINATED_BLOCK_COMMENT` 错误。

**代码来源**：[`lexer.c:227-238`](src/frontend/lexer/lexer.c:227)

```c
for (;;) {
    c = current_char(lexer);
    if (c == '/' && peek_char(lexer) == '/') {
        while (c != '\n' && c != '\0') {
            advance(lexer);
            c = current_char(lexer);
        }
        skip_whitespace(lexer);
        continue;
    }
    break;
}
```

---

## 4. 类型系统

### 4.1 基础类型

| 类型 | 枚举值 | 说明 | 代码来源 |
|------|--------|------|----------|
| `空类型` | `CN_TYPE_VOID` | 无类型 | [`semantics.h:24`](include/cnlang/frontend/semantics.h:24) |
| `整数` | `CN_TYPE_INT` | 整型（默认int32） | [`semantics.h:25`](include/cnlang/frontend/semantics.h:25) |
| `小数` | `CN_TYPE_FLOAT` | 浮点型（默认double） | [`semantics.h:26`](include/cnlang/frontend/semantics.h:26) |
| `布尔` | `CN_TYPE_BOOL` | 布尔型 | [`semantics.h:27`](include/cnlang/frontend/semantics.h:27) |
| `字符串` | `CN_TYPE_STRING` | 字符串型 | [`semantics.h:28`](include/cnlang/frontend/semantics.h:28) |

### 4.2 扩展数字类型

| 类型 | 枚举值 | 说明 | 代码来源 |
|------|--------|------|----------|
| `CN_TYPE_INT32` | int32 | 32位有符号整数 | [`semantics.h:37`](include/cnlang/frontend/semantics.h:37) |
| `CN_TYPE_INT64` | int64 | 64位有符号整数 | [`semantics.h:38`](include/cnlang/frontend/semantics.h:38) |
| `CN_TYPE_UINT32` | uint32 | 32位无符号整数 | [`semantics.h:39`](include/cnlang/frontend/semantics.h:39) |
| `CN_TYPE_UINT64` | uint64 | 64位无符号整数 | [`semantics.h:40`](include/cnlang/frontend/semantics.h:40) |
| `CN_TYPE_UINT64_LL` | uint64 | 64位无符号整数(ULL) | [`semantics.h:41`](include/cnlang/frontend/semantics.h:41) |
| `CN_TYPE_FLOAT32` | float32 | 单精度浮点 | [`semantics.h:42`](include/cnlang/frontend/semantics.h:42) |
| `CN_TYPE_FLOAT64` | float64 | 双精度浮点 | [`semantics.h:43`](include/cnlang/frontend/semantics.h:43) |

### 4.3 复合类型

| 类型 | 枚举值 | 说明 | 代码来源 |
|------|--------|------|----------|
| 指针 | `CN_TYPE_POINTER` | 指针类型 | [`semantics.h:29`](include/cnlang/frontend/semantics.h:29) |
| 数组 | `CN_TYPE_ARRAY` | 数组类型 | [`semantics.h:30`](include/cnlang/frontend/semantics.h:30) |
| 结构体 | `CN_TYPE_STRUCT` | 结构体类型 | [`semantics.h:31`](include/cnlang/frontend/semantics.h:31) |
| 枚举 | `CN_TYPE_ENUM` | 枚举类型 | [`semantics.h:32`](include/cnlang/frontend/semantics.h:32) |
| 函数 | `CN_TYPE_FUNCTION` | 函数类型 | [`semantics.h:33`](include/cnlang/frontend/semantics.h:33) |

### 4.4 类型解析

**代码来源**：[`parser.c:1803-1869`](src/frontend/parser/parser.c:1803)

```c
static CnType *parse_type(CnParser *parser)
{
    CnType *type = NULL;
    
    // 解析基础类型
    if (parser->current.kind == CN_TOKEN_KEYWORD_INT) {
        type = cn_type_new_primitive(CN_TYPE_INT);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_FLOAT) {
        type = cn_type_new_primitive(CN_TYPE_FLOAT);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_BOOL) {
        type = cn_type_new_primitive(CN_TYPE_BOOL);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_STRING) {
        type = cn_type_new_primitive(CN_TYPE_STRING);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_KEYWORD_VOID) {
        type = cn_type_new_primitive(CN_TYPE_VOID);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_IDENT) {
        // 自定义类型（结构体/枚举）
        type = cn_type_new_struct(type_name, type_name_length, NULL, 0, NULL, NULL, 0);
    }
    
    // 解析指针层级：支持多级指针
    while (parser->current.kind == CN_TOKEN_STAR) {
        type = cn_type_new_pointer(type);
        parser_advance(parser);
    }
    
    return type;
}
```

### 4.5 类型兼容性规则

**代码来源**：[`type_system.c:103-150`](src/semantics/symbols/type_system.c:103)

CN语言支持以下隐式类型转换：

| 源类型 | 目标类型 | 允许 | 说明 |
|--------|---------|------|------|
| `字符` | `整数` | ✅ | 隐式转换 |
| `整数` | `字符` | ✅ | 截断低8位 |
| `字符[]` | `字符*` | ✅ | 数组退化 |
| `字符[]` | `字符串` | ✅ | 安全转换 |
| `字符*` | `字符串` | ✅ | 安全转换（已实现） |
| `字符串` | `字符*` | ✅ | 安全转换（已实现） |
| `字符串` | `字符[]` | ❌ | 禁止（不可变） |
| `空类型*` | 任意指针 | ✅ | 通用指针转换 |
| 任意指针 | `空类型*` | ✅ | 通用指针转换 |

**实现代码**：

```c
// 字符* 到 字符串 的隐式转换
if (a->kind == CN_TYPE_POINTER && b->kind == CN_TYPE_STRING) {
    if (a->as.pointer_to && a->as.pointer_to->kind == CN_TYPE_CHAR) {
        return true;
    }
}

// 字符串 到 字符* 的隐式转换
if (a->kind == CN_TYPE_STRING && b->kind == CN_TYPE_POINTER) {
    if (b->as.pointer_to && b->as.pointer_to->kind == CN_TYPE_CHAR) {
        return true;
    }
}
```

---

## 5. 语法规则

### 5.1 程序结构

**语法**：
```
程序 ::= 顶层声明*
顶层声明 ::= 函数声明 | 结构体声明 | 枚举声明 | 导入语句 | 全局变量声明
```

**说明**：CN语言采用"文件即模块"设计，每个源文件自动成为一个模块，模块名由文件名决定。

**代码来源**：[`parser.c:263-402`](src/frontend/parser/parser.c:263)

### 5.2 函数声明

**语法**：
```
函数声明 ::= 函数定义 | 函数原型声明
函数定义 ::= 重写修饰? 静态修饰? "函数" 标识符 "(" 参数列表? ")" ("->" 类型)? 代码块
函数原型声明 ::= 重写修饰? 静态修饰? "函数" 标识符 "(" 参数列表? ")" ("->" 类型)? ";"
重写修饰 ::= "重写"
静态修饰 ::= "静态"
参数列表 ::= 参数 ("," 参数)*
参数 ::= 类型 标识符 ("[" 整数? "]")?
```

**说明**：
- **函数定义**：包含完整函数体（代码块），用于实现函数
- **函数原型声明**：以分号结尾，无函数体，用于前向声明
- `重写` 关键字位于函数声明开头，用于标记重写基类虚函数
- `静态` 关键字位于 `函数` 关键字之前，用于声明静态方法
- 返回类型使用 `-> 类型` 语法，位于参数列表之后
- 返回类型可省略，省略时根据函数体中的 `返回` 语句推断返回类型
- 若函数无 `返回` 语句或返回语句无值，则推断为 `空类型`

**函数定义示例**：
```cn
// 基本函数（返回类型可省略）
函数 加法(整数 a, 整数 b) -> 整数 {
    返回 a + b;
}

函数 无返回值() {
    // 无返回值函数
}

// 重写函数（重写关键字在开头）
重写 函数 发声() {
    打印("喵喵");
}

重写 函数 获取名称() -> 字符串 {
    返回 "猫";
}

// 重写静态函数
重写 静态 函数 工厂方法() -> 自身类型 {
    返回 创建实例();
}
```

**函数原型声明示例**：
```cn
// 前向声明（用于相互递归调用）
函数 计算阶乘(整数 n) -> 整数;

// 声明后定义
函数 主程序() -> 整数 {
    返回 计算阶乘(5);
}

函数 计算阶乘(整数 n) -> 整数 {
    如果 (n <= 1) {
        返回 1;
    }
    返回 n * 计算阶乘(n - 1);
}

// 类方法前向声明
类 动物 {
    虚拟 函数 发声();  // 抽象方法声明（无函数体）
}

// 静态方法声明
静态 函数 获取实例计数() -> 整数;
```

**函数原型声明的用途**：
1. **前向声明**：在函数定义前声明，支持相互递归调用
2. **接口定义**：在类中声明抽象方法（无函数体）
3. **API声明**：声明外部库函数，实现由链接器提供
4. **头文件模式**：声明与实现分离

**代码来源**：[`parser.c:405-617`](src/frontend/parser/parser.c:405)

### 5.3 变量声明

**语法**：
```
变量声明 ::= ("变量" | "常量" | 类型) 标识符 ("[" 整数? "]")* ("=" 表达式)? ";"
```

**示例**：
```cn
变量 x = 10;           // 类型推断
整数 y = 20;           // 显式类型
整数 arr[10];          // 数组
整数 matrix[3][4];     // 多维数组
常量 PI = 3.14159;     // 常量（类型推断）
常量 整数 MAX = 100;   // 常量（显式类型）
常量 字符串 NAME = "CN"; // 字符串常量
```

**代码来源**：[`parser.c:1050-1352`](src/frontend/parser/parser.c:1050)

### 5.3.1 静态局部变量

**语法**：
```
静态变量声明 ::= "静态" 类型 标识符 ("=" 表达式)? ";"
```

**说明**：
- `静态` 关键字用于声明函数内部的静态局部变量
- 静态变量在程序启动时分配内存，生命周期贯穿整个程序运行期
- 静态变量只在首次执行到声明语句时初始化一次
- 后续函数调用保持上次的值

**示例**：
```cn
函数 计数器() -> 整数 {
    静态 整数 计数 = 0;  // 只初始化一次
    计数 = 计数 + 1;
    返回 计数;
}

函数 斐波那契() -> 整数 {
    静态 整数 a = 0;     // 保持状态
    静态 整数 b = 1;
    整数 结果 = a;
    整数 temp = a + b;
    a = b;
    b = temp;
    返回 结果;
}
```

**语义规则**：
1. 静态变量必须显式指定类型（不支持类型推断）
2. 静态变量只能声明在函数内部
3. 静态变量的初始化表达式必须是编译时常量
4. 静态变量不能与 `常量` 关键字同时使用

**代码来源**：
- 词法定义：[`keywords.c:50`](src/frontend/lexer/keywords.c:50)
- 语法解析：[`parser.c:1061-1100`](src/frontend/parser/parser.c:1061)
- AST节点：[`ast.h:114`](include/cnlang/frontend/ast.h:114)
- 语义检查：[`semantic_passes.c:246`](src/semantics/checker/semantic_passes.c:246)

### 5.3.2 异常处理语句

**语法**：
```
try语句 ::= "尝试" 代码块 catch子句* finally子句?
catch子句 ::= "捕获" "(" (类型名 标识符?)? ")" 代码块
finally子句 ::= "最终" 代码块
throw语句 ::= "抛出" (表达式 | 字符串字面量 字符串字面量?) ";"
```

**说明**：
- `尝试` 关键字开始一个try块，用于捕获异常
- `捕获` 关键字定义catch块，可以指定异常类型和变量名
- `抛出` 关键字抛出异常
- `最终` 关键字定义finally块，无论是否发生异常都会执行

**示例**：
```cn
// 基本try-catch
尝试 {
    可能抛出异常的代码();
} 捕获 (运行时异常 e) {
    打印("捕获到异常: " + e.消息);
}

// try-catch-finally
尝试 {
    打开文件("data.txt");
} 捕获 (输入输出异常 e) {
    打印("IO错误: " + e.消息);
} 最终 {
    关闭文件();
}

// 抛出异常
抛出 "运行时异常" "发生错误";

// 捕获所有异常
尝试 {
    危险操作();
} 捕获 (e) {
    打印("捕获任意异常");
}
```

**代码来源**：
- 词法定义：[`keywords.c:64-67`](src/frontend/lexer/keywords.c:64)
- Token类型：[`token.h:59-62`](include/cnlang/frontend/token.h:59)
- AST节点：[`ast.h:213-254`](include/cnlang/frontend/ast.h:213)
- 语法解析：[`parser.c:1078-1230`](src/frontend/parser/parser.c:1078)
- 运行时支持：[`exception.h`](include/cnlang/runtime/exception.h)
- 代码生成：[`exception_cgen.c`](src/backend/cgen/exception_cgen.c)

### 5.4 结构体声明

**语法**：
```
结构体声明 ::= "结构体" 标识符 "{" 字段列表? "}"
字段列表 ::= 字段 ("," 字段)*
字段 ::= ("常量"? 类型 ("[" 整数? "]")* 标识符)
```

**示例**：
```cn
结构体 点 {
    整数 x;
    整数 y;
}

结构体 矩形 {
    点 左上;
    点 右下;
    常量 整数 面积;  // 常量字段
}

// 数组字段示例
结构体 数组字段测试 {
    整数[] 数组字段;        // 动态数组字段（指针）
    整数*[] 指针数组字段;   // 指针数组字段
    字符串[] 字符串数组;    // 字符串数组字段
    小数[10] 固定大小数组;  // 固定大小数组字段
}
```

**数组字段说明**：
- `类型[] 字段名` - 动态数组字段，编译为指针类型
- `类型[N] 字段名` - 固定大小数组字段，编译为指针类型
- `类型*[] 字段名` - 指针数组字段，编译为二级指针
- 多维数组支持：`类型[][] 字段名`

**枚举类型字段**：
结构体字段可以使用枚举类型，支持赋值和比较操作。

```cn
枚举 颜色 {
    红,
    绿,
    蓝
};

结构体 物体 {
    颜色 颜色值;  // 枚举类型字段
    整数 大小;
};

函数 主程序() -> 整数 {
    物体 球;
    球.颜色值 = 红;  // 枚举类型字段赋值
    
    如果 (球.颜色值 == 红) {  // 枚举类型字段比较
        返回 1;
    }
    
    返回 0;
}
```

**枚举类型字段支持的操作**：
- 赋值：可以将枚举成员赋值给枚举类型字段
- 比较：可以使用 `==` 和 `!=` 比较枚举类型字段与枚举成员

**代码来源**：[`parser.c:2980-3130`](src/frontend/parser/parser.c:2980)

### 5.5 枚举声明

**语法**：
```
枚举声明 ::= "枚举" 标识符 "{" 枚举成员列表? "}"
枚举成员列表 ::= 枚举成员 ("," 枚举成员)*
枚举成员 ::= 标识符 ("=" ("-"? 整数字面量))?
```

**示例**：
```cn
枚举 颜色 {
    红,        // 默认值 0
    绿,        // 默认值 1
    蓝 = 10,   // 显式赋值
    黄         // 值为 11
}

// 支持负数枚举值
枚举 状态 {
    成功 = 0,
    失败 = -1,      // 负数表示错误状态
    无效参数 = -2,
    超时 = -3
}
```

**枚举类型作为结构体字段**：
枚举类型可以作为结构体字段的类型使用，支持赋值和比较操作。

```cn
枚举 状态 {
    待处理,
    处理中,
    已完成
};

结构体 任务 {
    字符串 名称;
    状态 当前状态;  // 枚举类型字段
};

函数 处理任务(任务 t) {
    t.当前状态 = 处理中;  // 赋值枚举成员
    
    如果 (t.当前状态 == 已完成) {  // 比较枚举字段
        打印("任务完成");
    }
}
```

**代码来源**：[`parser.c:3283-3421`](src/frontend/parser/parser.c:3283)

### 5.6 控制流语句

#### 5.6.1 if语句

**语法**：
```
if语句 ::= "如果" "(" 表达式 ")" 代码块 ("否则" 代码块 | "否则" if语句)?
```

**示例**：
```cn
如果 (x > 0) {
    // 正数
} 否则 如果 (x < 0) {
    // 负数
} 否则 {
    // 零
}
```

**代码来源**：[`parser.c:770-820`](src/frontend/parser/parser.c:770)

#### 5.6.2 while语句

**语法**：
```
while语句 ::= "当" "(" 表达式 ")" 代码块
```

**示例**：
```cn
当 (i < 10) {
    i = i + 1;
}
```

**代码来源**：[`parser.c:822-850`](src/frontend/parser/parser.c:822)

#### 5.6.3 for语句

**语法**：
```
for语句 ::= "循环" "(" 变量声明? ";" 表达式? ";" 表达式? ")" 代码块
```

**示例**：
```cn
循环 (整数 i = 0; i < 10; i = i + 1) {
    打印(i);
}
```

**代码来源**：[`parser.c:852-920`](src/frontend/parser/parser.c:852)

#### 5.6.4 switch语句

**语法**：
```
switch语句 ::= "选择" "(" 表达式 ")" "{" case分支* "}"
case分支 ::= "情况" 整数 ":" 语句* | "默认" ":" 语句*
```

**示例**：
```cn
选择 (x) {
    情况 1:
        打印("一");
        中断;
    情况 2:
        打印("二");
        中断;
    默认:
        打印("其他");
}
```

**代码来源**：[`parser.c:922-1050`](src/frontend/parser/parser.c:922)

### 5.7 表达式

#### 5.7.1 运算符优先级（从低到高）

| 优先级 | 运算符 | 结合性 | 代码来源 |
|--------|--------|--------|----------|
| 1 | `=` | 右结合 | [`parser.c:1359-1379`](src/frontend/parser/parser.c:1359) |
| 2 | `? :` | 右结合 | [`parser.c:1383-1430`](src/frontend/parser/parser.c:1383) |
| 3 | `\|\|` | 左结合 | [`parser.c:1432-1443`](src/frontend/parser/parser.c:1432) |
| 4 | `&&` | 左结合 | [`parser.c:1445-1456`](src/frontend/parser/parser.c:1445) |
| 5 | `\|` | 左结合 | [`parser.c:1459-1470`](src/frontend/parser/parser.c:1459) |
| 6 | `^` | 左结合 | [`parser.c:1473-1484`](src/frontend/parser/parser.c:1473) |
| 7 | `&` | 左结合 | [`parser.c:1487-1499`](src/frontend/parser/parser.c:1487) |
| 8 | `==` `!=` | 左结合 | [`parser.c:1517-1567`](src/frontend/parser/parser.c:1517) |
| 9 | `<` `<=` `>` `>=` | 左结合 | [`parser.c:1517-1567`](src/frontend/parser/parser.c:1517) |
| 10 | `<<` `>>` | 左结合 | [`parser.c:1501-1515`](src/frontend/parser/parser.c:1501) |
| 11 | `+` `-` | 左结合 | [`parser.c:1569-1583`](src/frontend/parser/parser.c:1569) |
| 12 | `*` `/` `%` | 左结合 | [`parser.c:1585-1600`](src/frontend/parser/parser.c:1585) |
| 13 | `!` `-` `~` `&` `*` `++` `--` | 右结合 | [`parser.c:1602-1680`](src/frontend/parser/parser.c:1602) |

#### 5.7.2 表达式类型

**代码来源**：[`parser.c:1966-2075`](src/frontend/parser/parser.c:1966)

支持的表达式类型：
- 整数字面量：`123`, `0xFF`, `0b1010`, `0o755`
- 浮点数字面量：`3.14`, `1.5e-3`
- 字符串字面量：`"你好"`
- 字符字面量：`'A'`, `'\n'`, `'\x41'`
- 布尔字面量：`真`, `假`
- 空值：`无`
- 标识符引用
- 二元运算表达式
- 一元运算表达式
- 函数调用
- 数组索引
- 成员访问（`.` 和 `->`）
- 数组字面量
- 结构体字面量
- 三元条件表达式

---

## 6. 模块系统

### 6.1 文件即模块

**设计原则**：CN语言采用"文件即模块"设计，每个源文件（`.cn`）自动成为一个独立的模块。

**模块命名规则**：
- 模块名由文件名决定，例如 `数学工具.cn` 的模块名为 `数学工具`
- 文件名支持中文、英文和下划线
- 模块名与文件路径相关，支持相对路径和绝对路径导入

**示例**：
```
项目目录结构：
├── 主程序.cn          # 模块名: 主程序
├── 工具/
│   ├── 数学工具.cn    # 模块名: 工具/数学工具
│   └── 字符串工具.cn  # 模块名: 工具/字符串工具
└── 库/
    └── 网络库.cn      # 模块名: 库/网络库
```

**可见性控制**：
```cn
// 文件级可见性（在文件顶部声明）
公开:    // 以下声明为公开，可被其他模块导入
函数 外部接口() { }

私有:    // 以下声明为私有，仅本文件可见
函数 内部实现() { }
```

### 6.2 导入语句

#### 6.2.1 全量导入

**语法**：
```
导入 模块名;
```

**示例**：
```cn
导入 数学工具;
```

**代码来源**：[`parser.c:3792-4030`](src/frontend/parser/parser.c:3792)

#### 6.2.2 选择性导入

**语法**：
```
导入 模块名 { 成员1, 成员2 };
```

**示例**：
```cn
导入 数学工具 { 加法, 减法 };
```

**代码来源**：[`parser.c:3929-4001`](src/frontend/parser/parser.c:3929)

#### 6.2.3 别名导入

**语法**：
```
导入 模块名(别名);
```

**示例**：
```cn
导入 数学工具(数学);
```

**代码来源**：[`parser.c:3891-3923`](src/frontend/parser/parser.c:3891)

#### 6.2.4 路径导入

**语法**：
```
导入 ./相对路径;
导入 ../父级路径;
导入 包/子包/模块;
导入 包.子包.模块;
```

**示例**：
```cn
导入 ./兄弟模块;
导入 ../父级模块;
导入 工具/数学/高级;
导入 工具.数学.高级;
```

**代码来源**：[`parser.c:4032-4145`](src/frontend/parser/parser.c:4032)

### 6.3 从...导入语句

**语法**：
```
从 模块路径 导入 { 成员1, 成员2 };  // 选择性导入
从 模块路径 导入 *;                  // 通配符导入
从 包路径 导入 模块名;               // 从包导入模块
```

**示例**：
```cn
从 数学工具 导入 { 加法, 减法 };
从 工具.数学 导入 *;
从 ./兄弟模块 导入 { 功能A };
```

**代码来源**：[`parser.c:4147-4410`](src/frontend/parser/parser.c:4147)

### 6.4 可见性控制

**文件级可见性**：
```cn
公开:    // 以下声明为公开，可被其他模块导入
函数 外部接口() { }

私有:    // 以下声明为私有，仅本文件可见
函数 内部实现() { }
```

**说明**：
- `公开` 修饰的声明可以被其他模块通过 `导入` 语句访问
- `私有` 修饰的声明仅在本文件内可见
- 可见性修饰符影响其后的所有声明，直到下一个可见性修饰符或文件结束

**代码来源**：[`parser.c:276-318`](src/frontend/parser/parser.c:276)

---

## 7. 面向对象编程（OOP）语法

### 7.1 类声明

**语法**：
```
类声明 ::= ("抽象"? "类") 标识符 (":" 基类列表)? "{" 类成员* "}"
基类列表 ::= 标识符 ("," 标识符)*
类成员 ::= 访问级别标签? (字段声明 | 方法声明 | 构造函数)
访问级别标签 ::= ("公开" | "私有" | "保护") ":"
```

**示例**：
```cn
// 基本类
类 动物 {
    字符串 名称;
    
    函数 发声() {
        打印("动物发声");
    }
}

// 继承
类 狗 : 动物 {
    函数 发声() {
        打印("汪汪");
    }
}

// 抽象类
抽象 类 形状 {
    公开:
    函数 获取面积() -> 小数;  // 抽象方法
    私有:
    小数 面积;
}
```

**代码来源**：[`parser.c:5473-5578`](src/frontend/parser/parser.c:5473)

### 7.2 接口声明

**语法**：
```
接口声明 ::= 模板前缀? "接口" 标识符 (":" 接口列表)? "{" 接口成员* "}"
模板前缀 ::= "模板" "<" 模板参数列表 ">"
模板参数列表 ::= 标识符 ("," 标识符)*
接口成员 ::= 方法签名
方法签名 ::= "函数" 标识符 "(" 参数列表? ")" ("->" 类型)? ";"
```

**说明**：
- 接口支持可选的模板参数，用于定义类型安全的泛型接口
- 模板参数可在接口方法签名中作为类型使用
- 无模板参数的接口保持原有语法不变（向后兼容）

**示例**：
```cn
// 无模板参数的接口（现有语法）
接口 可序列化 {
    函数 序列化() -> 字符串;
    函数 反序列化(字符串 数据);
}

// 接口继承
接口 可比较 : 可序列化 {
    函数 比较(自身 其他) -> 整数;
}

// 带模板参数的接口（新语法）
// T 表示实现此接口的类的类型
模板<T>
接口 可比较接口 {
    函数 比较(T 其他) -> 整数;
}

// 多模板参数的接口
模板<K, V>
接口 映射接口 {
    函数 获取(K 键) -> V;
    函数 设置(K 键, V 值);
}
```

**代码来源**：[`parser.c:5580-5670`](src/frontend/parser/parser.c:5580)

### 7.3 类实现接口

**语法**：
```
类声明 ::= "类" 标识符 (":" 基类)? ("实现" 接口实例化列表)? "{" 类成员* "}"
接口实例化列表 ::= 接口实例化 ("," 接口实例化)*
接口实例化 ::= 标识符 ("<" 类型列表 ">")?
类型列表 ::= 类型 ("," 类型)*
```

**说明**：
- 类实现模板接口时，需要指定具体的类型参数
- 类型参数通常是实现接口的类自身，实现类型安全的"自身类型"语义
- 无模板参数的接口使用简单标识符即可

**示例**：
```cn
// 实现无模板参数的接口
类 学生 : 人员 实现 可序列化 {
    公开:
    函数 序列化() -> 字符串 {
        返回 "学生数据";
    }
}

// 实现模板接口（类型参数为类自身）
类 数据项 : 实现 可比较接口<数据项> {
    公开:
    整数 编号;
    字符串 名称;
    
    函数 比较(数据项 其他) -> 整数 {
        如果 (自身.编号 < 其他.编号) {
            返回 -1;
        } 否则 如果 (自身.编号 > 其他.编号) {
            返回 1;
        }
        返回 0;
    }
}

// 同时继承基类和实现多个接口
类 学生 : 人员 实现 可序列化, 可比较接口<学生> {
    公开:
    函数 序列化() -> 字符串 {
        返回 "学生数据";
    }
    
    函数 比较(学生 其他) -> 整数 {
        返回 0;
    }
}
```

**代码来源**：[`parser.c:5514-5536`](src/frontend/parser/parser.c:5514)

### 7.4 访问控制

**语法**：
```
访问级别 ::= "公开" | "私有" | "保护"
```

| 关键字 | 说明 | 代码来源 |
|--------|------|----------|
| `公开` | 公开成员，外部可访问 | [`keywords.c:36`](src/frontend/lexer/keywords.c:36) |
| `私有` | 私有成员，仅类内部可访问 | [`keywords.c:37`](src/frontend/lexer/keywords.c:37) |
| `保护` | 保护成员，类及子类可访问 | [`keywords.c:53`](src/frontend/lexer/keywords.c:53) |

### 7.5 虚函数与重写

**语法**：
```
虚函数声明 ::= "虚拟" 函数声明
重写函数声明 ::= "重写" 静态修饰? "函数" 标识符 "(" 参数列表? ")" ("->" 类型)? 代码块
静态修饰 ::= "静态"
```

**说明**：
- `重写` 关键字位于函数声明开头（在 `函数` 关键字之前）
- 重写函数可以与 `静态` 关键字组合使用
- 返回类型统一使用 `-> 类型` 语法
- **重要限制**：`重写` 只能用于重写基类的 `虚拟` 函数，与C++语义一致
  - 虚函数 + 重写 = 运行时多态（动态绑定）
  - 非虚函数 + 同名函数 = 隐藏（静态绑定），不能使用 `重写` 关键字

**示例**：
```cn
类 动物 {
    虚拟 函数 发声() {
        打印("动物发声");
    }
    
    虚拟 函数 获取名称() -> 字符串 {
        返回 "动物";
    }
}

类 猫 : 动物 {
    // 重写无返回值函数
    重写 函数 发声() {
        打印("喵喵");
    }
    
    // 重写有返回值函数
    重写 函数 获取名称() -> 字符串 {
        返回 "猫";
    }
}

// 重写静态函数示例
类 工厂基类 {
    虚拟 静态 函数 创建() -> 自身类型;
}

类 具体工厂 : 工厂基类 {
    重写 静态 函数 创建() -> 自身类型 {
        返回 新建 具体工厂();
    }
}
```

**语法变更说明**：
| 项目 | 旧语法 | 新语法 |
|------|--------|--------|
| 重写关键字位置 | 函数签名末尾 | 函数声明开头 |
| 返回类型语法 | `: 类型` 或 `-> 类型` | 统一为 `-> 类型` |

**代码来源**：
- 虚函数：[`keywords.c:54`](src/frontend/lexer/keywords.c:54)
- 重写：[`keywords.c:55`](src/frontend/lexer/keywords.c:55)

### 7.5.1 自身类型关键字

**语法**：
```
自身类型 ::= "自身类型"
```

**说明**：
- `自身类型` 关键字用于类方法的返回类型，表示返回当前类的类型
- 主要用于静态工厂方法模式，实现类型安全的对象创建
- **与接口模板参数的区别**：
  - `自身类型`：用于类方法的返回类型
  - 模板参数 `T`：用于接口方法的参数类型，表示实现此接口的类的类型

**示例**：
```cn
// 类方法使用自身类型作为返回类型
类 工厂基类 {
    虚拟 静态 函数 创建() -> 自身类型;
}

类 具体工厂 : 工厂基类 {
    重写 静态 函数 创建() -> 自身类型 {
        返回 新建 具体工厂();
    }
}

// 接口方法使用模板参数作为参数类型
模板<T>
接口 可比较接口 {
    函数 比较(T 其他) -> 整数;  // T 是实现类的类型
}

类 数据项 : 实现 可比较接口<数据项> {
    函数 比较(数据项 其他) -> 整数 {
        返回 0;
    }
}
```

**代码来源**：[`keywords.c:58`](src/frontend/lexer/keywords.c:58)

### 7.6 构造函数与析构函数

#### 7.6.1 构造函数

**语法**：
```
构造函数 ::= "函数" 类名 "(" 参数列表? ")" (":" 初始化列表)? "{" 函数体 "}"
初始化列表 ::= 成员初始化 ("," 成员初始化)*
成员初始化 ::= 成员名 "(" 表达式? ")"
```

**识别规则**：函数名与类名相同即为构造函数

**示例**：
```cn
类 学生 {
公开:
    字符串 姓名;
    整数 年龄;
    
    // 默认构造函数
    函数 学生() {
        姓名 = "未知";
        年龄 = 0;
    }
    
    // 带参数构造函数 + 初始化列表
    函数 学生(字符串 名, 整数 龄) : 姓名(名), 年龄(龄) {
        打印("学生创建完成");
    }
};
```

**限制**：
- 构造函数不能有返回类型
- 构造函数不能是静态的
- 构造函数不能是虚函数

**代码来源**：[`parser.c:5321-5620`](src/frontend/parser/parser.c:5321)

#### 7.6.2 析构函数

**语法**：
```
析构函数 ::= "函数" "~" 类名 "(" ")" "{" 函数体 "}"
```

**识别规则**：以 `~` 开头，后跟与类名相同的标识符

**示例**：
```cn
类 学生 {
公开:
    字符串* 动态数据;
    
    // 构造函数
    函数 学生() {
        动态数据 = 新建 字符串[100];
    }
    
    // 析构函数
    函数 ~学生() {
        删除[] 动态数据;
        打印("学生对象被销毁");
    }
};
```

**限制**：
- 析构函数名必须与类名相同
- 析构函数不能有参数
- 析构函数不能有返回类型
- 析构函数不能是静态的
- 析构函数不能是虚函数

**代码来源**：[`parser.c:5321-5620`](src/frontend/parser/parser.c:5321)

### 7.7 自身与基类访问

**语法**：
```
自身访问 ::= "自身" "." 标识符 | "自身" "->" 标识符
基类访问 ::= "基类" "." 标识符 | "基类" "->" 标识符
```

**示例**：
```cn
类 学生 {
    字符串 姓名;
    
    函数 打印姓名() {
        打印(自身.姓名);  // 访问自身成员
    }
}

类 研究生 : 学生 {
    字符串 导师;
    
    函数 打印信息() {
        基类.打印姓名();  // 调用基类方法
        打印(自身.导师);
    }
}
```

**代码来源**：
- `自身`：[`keywords.c:58`](src/frontend/lexer/keywords.c:58)
- `基类`：[`keywords.c:59`](src/frontend/lexer/keywords.c:59)

---

## 8. 函数指针

### 8.1 基本语法

**语法**：
```
函数指针声明 ::= 返回类型 "(" "*" 标识符? ")" "(" 参数类型列表? ")"
参数类型列表 ::= 参数类型 ("," 参数类型)*
```

**说明**：
- 函数指针用于存储函数的地址，实现回调机制
- 支持在变量声明、结构体字段、函数参数中使用

**示例**：
```cn
// 函数指针变量声明
整数(*回调)(整数, 整数);

// 函数指针赋值
回调 = 加法函数;

// 通过函数指针调用
整数 结果 = 回调(1, 2);
```

**代码来源**：
- 类型定义：[`semantics.h:33`](include/cnlang/frontend/semantics.h:33) `CN_TYPE_FUNCTION`
- 参数解析：[`parser.c:528-601`](src/frontend/parser/parser.c:528)
- 结构体字段解析：[`parser.c:3665-3713`](src/frontend/parser/parser.c:3665)

### 8.2 函数指针作为结构体字段

**语法**：
```
结构体字段 ::= 普通类型 标识符 | 返回类型 "(" "*" 标识符 ")" "(" 参数类型列表? ")"
```

**示例**：
```cn
结构体 事件处理器 {
    整数(*点击)(整数, 整数);    // 函数指针字段
    整数(*按键)(整数);          // 函数指针字段
};

// 使用
事件处理器 处理器;
处理器.点击 = 鼠标点击回调;
处理器.点击(100, 200);
```

**代码来源**：[`parser.c:3665-3713`](src/frontend/parser/parser.c:3665)

### 8.3 函数指针作为函数参数

**语法**：
```
函数参数 ::= 普通类型 标识符 | 返回类型 "(" "*" 标识符? ")" "(" 参数类型列表? ")" 标识符?
```

**示例**：
```cn
// 函数指针参数（匿名参数类型）
函数 遍历(整数* 数组, 整数(*)(整数) 处理函数) {
    // 处理函数 是一个接受整数参数返回整数的函数指针
}

// 函数指针参数（带名参数类型）
函数 排序(整数* 数据, 整数(*比较)(整数, 整数) 比较器) {
    // 比较器 是一个接受两个整数参数返回整数的函数指针
}

// 调用示例
函数 升序比较(整数 a, 整数 b) -> 整数 {
    返回 a - b;
}

排序(数据数组, 升序比较);
```

**代码来源**：[`parser.c:528-601`](src/frontend/parser/parser.c:528)

---

## 9. 命令行参数标准库

### 9.1 API列表

| 函数 | 返回类型 | 说明 | 代码来源 |
|------|----------|------|----------|
| `获取参数个数()` | 整数 | 返回命令行参数个数 | [`cli.h:45`](include/cnlang/runtime/cli.h:45) |
| `获取参数(索引)` | 字符串 | 获取指定索引的参数 | [`cli.h:52`](include/cnlang/runtime/cli.h:52) |
| `查找选项(选项名)` | 字符串 | 查找选项值（支持 `-o` 和 `--option`） | [`cli.h:60`](include/cnlang/runtime/cli.h:60) |
| `选项存在(选项名)` | 布尔 | 检查选项是否存在 | [`cli.h:68`](include/cnlang/runtime/cli.h:68) |

### 9.2 使用示例

**基本用法**：
```cn
函数 主程序() -> 整数 {
    整数 参数数量 = 获取参数个数();
    
    如果 (参数数量 > 0) {
        字符串 第一个参数 = 获取参数(0);
        打印(第一个参数);
    }
    
    返回 0;
}
```

**选项解析**：
```cn
函数 主程序() -> 整数 {
    // 检查帮助选项
    如果 (选项存在("--help")) {
        打印("用法: 程序名 [选项]");
        返回 0;
    }
    
    // 获取输出文件选项
    字符串 输出文件 = 查找选项("--output");
    如果 (输出文件 == 无) {
        输出文件 = 查找选项("-o");
    }
    
    如果 (输出文件 != 无) {
        打印("输出文件: " + 输出文件);
    }
    
    返回 0;
}
```

### 9.3 英文别名API

| 函数 | 说明 |
|------|------|
| `cn_rt_cli_argc()` | `获取参数个数()` 的英文别名 |
| `cn_rt_cli_argv(index)` | `获取参数(index)` 的英文别名 |
| `cn_rt_cli_find_option(name)` | `查找选项(选项名)` 的英文别名 |
| `cn_rt_cli_has_option(name)` | `选项存在(选项名)` 的英文别名 |

**代码来源**：
- 头文件：[`include/cnlang/runtime/cli.h`](include/cnlang/runtime/cli.h)
- 实现：[`src/runtime/cli/cli.c`](src/runtime/cli/cli.c)
- 测试：[`tests/unit/runtime/runtime_cli_test.c`](tests/unit/runtime/runtime_cli_test.c)

---

## 10. 预留特性

以下关键字已定义但功能未实现，使用时会报错：

| 关键字 | 错误信息 | 代码来源 |
|--------|----------|----------|
| `命名空间` | 关键字 '命名空间' 为预留特性，当前版本暂不支持 | [`parser.c:226`](src/frontend/parser/parser.c:226) |

**预留关键字检测函数**：[`parser.c:197-210`](src/frontend/parser/parser.c:197)

---

## 11. 已移除的特性

以下特性在早期版本中存在，但已被移除：

### 11.1 中断处理关键字

**状态**：已移除

**原语法**：
```cn
中断处理 向量号 () { ... }
```

**替代方案**：使用运行时API注册中断处理函数

**代码来源**：[`parser.c:369-382`](src/frontend/parser/parser.c:369)

### 11.2 内存操作关键字

**状态**：已移除

**原关键字**：
- `读取内存`
- `写入内存`
- `内存复制`
- `内存设置`
- `映射内存`
- `解除映射`

**替代方案**：使用运行时API，如 `cn_rt_mem_read()`, `cn_rt_mem_write()` 等

**代码来源**：[`parser.c:2076-2241`](src/frontend/parser/parser.c:2076)

### 11.3 内联汇编关键字

**状态**：已移除

**原语法**：
```cn
内联汇编("asm code", outputs, inputs, clobbers)
```

**替代方案**：使用运行时API `cn_rt_inline_asm()`

**代码来源**：[`parser.c:2242-2299`](src/frontend/parser/parser.c:2242)

---

## 12. 代码引用示例

以下是文档中3-5个关键规则的代码引用示例：

### 示例1：关键字定义

关键字 `如果` 对应 `CN_TOKEN_KEYWORD_IF`，定义在 [`keywords.c:10`](src/frontend/lexer/keywords.c:10)：
```c
{"如果", CN_TOKEN_KEYWORD_IF, "控制流关键字"},
```

### 示例2：类型解析

类型解析函数 `parse_type` 在 [`parser.c:1803-1869`](src/frontend/parser/parser.c:1803) 中实现，支持 `整数`、`小数`、`布尔`、`字符串`、`空类型` 和自定义类型。

### 示例3：函数声明解析

函数声明解析函数 `parse_function_decl` 在 [`parser.c:405-617`](src/frontend/parser/parser.c:405) 中实现，支持参数列表、返回类型和函数体。

### 示例4：模块导入解析

导入语句解析函数 `parse_import_stmt` 在 [`parser.c:3792-4030`](src/frontend/parser/parser.c:3792) 中实现，支持全量导入、选择性导入、别名导入和路径导入。

### 示例5：类型系统

类型枚举 `CnTypeKind` 定义在 [`semantics.h:23-46`](include/cnlang/frontend/semantics.h:23)，包含基础类型、复合类型和扩展数字类型。

---

## 13. 附录

### 13.1 完整关键字列表（43个）

**控制流关键字（10个）**：
`如果`、`否则`、`当`、`循环`、`返回`、`中断`、`继续`、`选择`、`情况`、`默认`

**类型关键字（7个）**：
`整数`、`小数`、`字符串`、`布尔`、`空类型`、`结构体`、`枚举`

**声明关键字（7个）**：
`函数`、`变量`、`导入`、`从`、`公开`、`私有`、`静态`

**常量关键字（3个）**：
`真`、`假`、`无`

**异常处理关键字（4个）**：
`尝试`、`捕获`、`抛出`、`最终`

**OOP关键字（9个）**：
`类`、`接口`、`保护`、`虚拟`、`重写`、`抽象`、`实现`、`自身`、`基类`

**预留关键字（1个）**：
`命名空间`

### 13.2 源代码文件索引

| 文件路径 | 主要功能 |
|----------|----------|
| `src/frontend/lexer/keywords.c` | 关键字定义表 |
| `include/cnlang/frontend/token.h` | Token类型枚举 |
| `src/frontend/lexer/lexer.c` | 词法分析器实现 |
| `src/frontend/parser/parser.c` | 语法分析器实现 |
| `include/cnlang/frontend/semantics.h` | 类型系统定义 |
| `src/semantics/symbols/type_system.c` | 类型系统实现 |

---

## 14. 语法变更历史

### 14.1 模块系统简化（2026-03-31）

#### 删除"模块"关键字声明形式

| 项目 | 旧语法 | 新语法 |
|------|--------|--------|
| 模块定义 | `模块 名字 { ... }` | 文件即模块（无需声明） |
| 模块名来源 | 声明时指定 | 由文件名决定 |

**旧语法示例**：
```cn
模块 数学工具 {
公开:
    函数 加法(整数 a, 整数 b) -> 整数 {
        返回 a + b;
    }
私有:
    变量 内部计数 = 0;
}
```

**新语法示例**：
```cn
// 文件名: 数学工具.cn
公开:
    函数 加法(整数 a, 整数 b) -> 整数 {
        返回 a + b;
    }
私有:
    变量 内部计数 = 0;
```

#### 变更原因

1. **简化语法**：减少一个关键字，降低学习成本
2. **避免混乱**：避免单文件多模块的复杂性
3. **统一设计**：与包系统更统一，自然配合 `__包__.cn` 机制
4. **路径导入更直观**：`导入 ./兄弟模块` 比模块声明更符合直觉

#### 影响范围

- 关键字数量：44个 → 43个
- 声明关键字：8个 → 7个
- 导入语句保持不变

### 14.2 函数语法变更（2026-03-30）

#### 重写关键字位置变更

| 项目 | 旧语法 | 新语法 |
|------|--------|--------|
| 重写关键字位置 | 函数签名末尾 | 函数声明开头 |
| 返回类型语法(类成员) | `: 类型` | `-> 类型` |
| 返回类型语法(全局函数) | `-> 类型` | 保持不变 |

**旧语法示例**：
```cn
类 猫 : 动物 {
    函数 发声() 重写 {
        打印("喵喵");
    }
    
    函数 获取名称(): 字符串 重写 {
        返回 "猫";
    }
}
```

**新语法示例**：
```cn
类 猫 : 动物 {
    重写 函数 发声() {
        打印("喵喵");
    }
    
    重写 函数 获取名称() -> 字符串 {
        返回 "猫";
    }
}
```

#### 变更原因

1. **一致性**：`重写` 关键字放在函数开头与 `虚拟`、`抽象` 等修饰符保持一致
2. **可读性**：函数声明开头即可识别是否为重写函数
3. **返回类型统一**：统一使用 `-> 类型` 语法，与全局函数保持一致

#### 支持的函数语法组合

| 语法形式 | 示例 |
|----------|------|
| 基本函数 | `函数 方法名(参数列表) { }` |
| 带返回类型 | `函数 方法名(参数列表) ->返回类型 { }` |
| 重写函数 | `重写 函数 方法名(参数列表) { }` |
| 重写带返回类型 | `重写 函数 方法名(参数列表) ->返回类型 { }` |
| 重写静态函数 | `重写 静态 函数 方法名(参数列表) ->返回类型 { }` |

**代码来源**：[`parser.c:5332`](src/frontend/parser/parser.c:5332)

### 14.3 字符字面量与函数原型声明（2026-04-04）

#### 新增字符字面量语法

| 项目 | 语法 | 说明 |
|------|------|------|
| 字符字面量 | `'A'`, `'\n'` | 单引号包围的单字符 |
| Token类型 | `CN_TOKEN_CHAR_LITERAL` | 新增Token类型 |

**语法规则**：
```
字符字面量 ::= "'" (普通字符 | 转义序列) "'"
转义序列 ::= "\" ("n" | "t" | "r" | "\" | "'" | "0" | "x" 十六进制序列)
```

**示例**：
```cn
变量 ch = 'A';      // 普通字符
变量 换行 = '\n';   // 转义字符
变量 十六进制 = '\x41'; // 'A' 的十六进制表示
```

**类型说明**：字符字面量的类型为 `整数`，值为对应字符的ASCII码。

#### 新增函数原型声明语法

| 项目 | 语法 | 说明 |
|------|------|------|
| 函数定义 | `函数 名(参数) -> 类型 { }` | 包含函数体 |
| 函数原型声明 | `函数 名(参数) -> 类型;` | 无函数体，以分号结尾 |

**语法规则**：
```
函数原型声明 ::= 重写修饰? 静态修饰? "函数" 标识符 "(" 参数列表? ")" ("->" 类型)? ";"
```

**示例**：
```cn
// 前向声明
函数 计算阶乘(整数 n) -> 整数;

// 声明后定义
函数 主程序() -> 整数 {
    返回 计算阶乘(5);
}

函数 计算阶乘(整数 n) -> 整数 {
    如果 (n <= 1) { 返回 1; }
    返回 n * 计算阶乘(n - 1);
}
```

**用途**：
1. 前向声明：支持相互递归调用
2. 接口定义：类中声明抽象方法
3. API声明：声明外部库函数
4. 头文件模式：声明与实现分离

#### 变更原因

1. **字符字面量**：与C语言保持一致，便于处理单个字符
2. **函数原型声明**：支持前向声明和接口定义，提高代码组织灵活性

---

**文档结束**

> 本文档严格基于源代码编写，所有规则均有代码依据。如有疑问，请参考对应的源代码文件。

