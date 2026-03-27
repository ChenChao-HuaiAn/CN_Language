# CN Language 语法规范设计文档

> **文档说明**：本文档严格基于 `src` 目录下的实际源代码编写，每个语法规则都标注了代码来源。未在代码中实现的功能标注为"预留"或"未实现"。
>
> **生成时间**：2026-03-27
> **代码版本**：基于当前 src 目录源码

---

## 目录

1. [关键字定义](#1-关键字定义)
2. [Token类型](#2-token类型)
3. [词法规则](#3-词法规则)
4. [类型系统](#4-类型系统)
5. [语法规则](#5-语法规则)
6. [模块系统](#6-模块系统)
7. [预留特性](#7-预留特性)

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

### 1.3 声明关键字（9个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `函数` | `CN_TOKEN_KEYWORD_FN` | 函数声明 | [`keywords.c:31`](src/frontend/lexer/keywords.c:31) |
| `变量` | `CN_TOKEN_KEYWORD_VAR` | 变量声明 | [`keywords.c:32`](src/frontend/lexer/keywords.c:32) |
| `模块` | `CN_TOKEN_KEYWORD_MODULE` | 模块声明 | [`keywords.c:33`](src/frontend/lexer/keywords.c:33) |
| `导入` | `CN_TOKEN_KEYWORD_IMPORT` | 导入语句 | [`keywords.c:34`](src/frontend/lexer/keywords.c:34) |
| `从` | `CN_TOKEN_KEYWORD_FROM` | 从...导入 | [`keywords.c:35`](src/frontend/lexer/keywords.c:35) |
| `公开` | `CN_TOKEN_KEYWORD_PUBLIC` | 公开可见性 | [`keywords.c:36`](src/frontend/lexer/keywords.c:36) |
| `私有` | `CN_TOKEN_KEYWORD_PRIVATE` | 私有可见性 | [`keywords.c:37`](src/frontend/lexer/keywords.c:37) |
| `常量` | `CN_TOKEN_KEYWORD_CONST` | 常量声明 | [`keywords.c:49`](src/frontend/lexer/keywords.c:49) |
| `静态` | `CN_TOKEN_KEYWORD_STATIC` | 静态局部变量 | [`keywords.c:50`](src/frontend/lexer/keywords.c:50) |

### 1.4 常量关键字（3个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `真` | `CN_TOKEN_KEYWORD_TRUE` | 布尔真值 | [`keywords.c:40`](src/frontend/lexer/keywords.c:40) |
| `假` | `CN_TOKEN_KEYWORD_FALSE` | 布尔假值 | [`keywords.c:41`](src/frontend/lexer/keywords.c:41) |
| `无` | `CN_TOKEN_KEYWORD_NULL` | 空值常量 | [`keywords.c:42`](src/frontend/lexer/keywords.c:42) |

### 1.5 预留关键字（8个）

| 关键字 | Token类型 | 说明 | 代码来源 |
|--------|-----------|------|----------|
| `命名空间` | `CN_TOKEN_KEYWORD_NAMESPACE` | 预留 | [`keywords.c:45`](src/frontend/lexer/keywords.c:45) |
| `接口` | `CN_TOKEN_KEYWORD_INTERFACE` | 预留 | [`keywords.c:46`](src/frontend/lexer/keywords.c:46) |
| `类` | `CN_TOKEN_KEYWORD_CLASS` | 预留 | [`keywords.c:47`](src/frontend/lexer/keywords.c:47) |
| `模板` | `CN_TOKEN_KEYWORD_TEMPLATE` | 预留 | [`keywords.c:48`](src/frontend/lexer/keywords.c:48) |
| `保护` | `CN_TOKEN_KEYWORD_PROTECTED` | 预留 | [`keywords.c:51`](src/frontend/lexer/keywords.c:51) |
| `虚拟` | `CN_TOKEN_KEYWORD_VIRTUAL` | 预留 | [`keywords.c:52`](src/frontend/lexer/keywords.c:52) |
| `重写` | `CN_TOKEN_KEYWORD_OVERRIDE` | 预留 | [`keywords.c:53`](src/frontend/lexer/keywords.c:53) |
| `抽象` | `CN_TOKEN_KEYWORD_ABSTRACT` | 预留 | [`keywords.c:54`](src/frontend/lexer/keywords.c:54) |

---

## 2. Token类型

### 2.1 字面量Token

| Token类型 | 说明 | 代码来源 |
|-----------|------|----------|
| `CN_TOKEN_INTEGER` | 整数字面量 | [`token.h:13`](include/cnlang/frontend/token.h:13) |
| `CN_TOKEN_FLOAT_LITERAL` | 浮点数字面量 | [`token.h:14`](include/cnlang/frontend/token.h:14) |
| `CN_TOKEN_STRING_LITERAL` | 字符串字面量 | [`token.h:15`](include/cnlang/frontend/token.h:15) |
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

---

## 5. 语法规则

### 5.1 程序结构

**语法**：
```
程序 ::= 顶层声明*
顶层声明 ::= 函数声明 | 结构体声明 | 枚举声明 | 模块声明 | 导入语句 | 全局变量声明
```

**代码来源**：[`parser.c:263-402`](src/frontend/parser/parser.c:263)

### 5.2 函数声明

**语法**：
```
函数声明 ::= "函数" 标识符 "(" 参数列表? ")" ("->" 类型)? 代码块
参数列表 ::= 参数 ("," 参数)*
参数 ::= 类型 标识符 ("[" 整数? "]")?
```

**示例**：
```cn
函数 加法(整数 a, 整数 b) -> 整数 {
    返回 a + b;
}

函数 无返回值() {
    // 无返回值函数
}
```

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
常量 PI = 3.14159;     // 常量
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

### 5.4 结构体声明

**语法**：
```
结构体声明 ::= "结构体" 标识符 "{" 字段列表? "}"
字段列表 ::= 字段 ("," 字段)*
字段 ::= ("常量"? 类型 标识符)
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
```

**代码来源**：[`parser.c:2980-3130`](src/frontend/parser/parser.c:2980)

### 5.5 枚举声明

**语法**：
```
枚举声明 ::= "枚举" 标识符 "{" 枚举成员列表? "}"
枚举成员列表 ::= 枚举成员 ("," 枚举成员)*
枚举成员 ::= 标识符 ("=" 整数)?
```

**示例**：
```cn
枚举 颜色 {
    红,        // 默认值 0
    绿,        // 默认值 1
    蓝 = 10,   // 显式赋值
    黄         // 值为 11
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

### 6.1 模块声明

**语法**：
```
模块声明 ::= "模块" 标识符 "{" 模块成员* "}"
模块成员 ::= 可见性修饰? (函数声明 | 变量声明)
可见性修饰 ::= ("公开" | "私有") ":"
```

**示例**：
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

**代码来源**：[`parser.c:3572-3782`](src/frontend/parser/parser.c:3572)

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
公开:    // 以下声明为公开
函数 外部接口() { }

私有:    // 以下声明为私有
函数 内部实现() { }
```

**模块级可见性**：
```cn
模块 示例 {
公开:
    函数 公开方法() { }
私有:
    函数 私有方法() { }
}
```

**代码来源**：[`parser.c:276-318`](src/frontend/parser/parser.c:276)

---

## 7. 预留特性

以下关键字已定义但功能未实现，使用时会报错：

| 关键字 | 错误信息 | 代码来源 |
|--------|----------|----------|
| `类` | 关键字 '类' 为预留特性，当前版本暂不支持 | [`parser.c:217`](src/frontend/parser/parser.c:217) |
| `接口` | 关键字 '接口' 为预留特性，当前版本暂不支持 | [`parser.c:220`](src/frontend/parser/parser.c:220) |
| `模板` | 关键字 '模板' 为预留特性，当前版本暂不支持 | [`parser.c:223`](src/frontend/parser/parser.c:223) |
| `命名空间` | 关键字 '命名空间' 为预留特性，当前版本暂不支持 | [`parser.c:226`](src/frontend/parser/parser.c:226) |
| `保护` | 关键字 '保护' 为预留特性，当前版本暂不支持 | [`parser.c:232`](src/frontend/parser/parser.c:232) |
| `虚拟` | 关键字 '虚拟' 为预留特性，当前版本暂不支持 | [`parser.c:235`](src/frontend/parser/parser.c:235) |
| `重写` | 关键字 '重写' 为预留特性，当前版本暂不支持 | [`parser.c:238`](src/frontend/parser/parser.c:238) |
| `抽象` | 关键字 '抽象' 为预留特性，当前版本暂不支持 | [`parser.c:241`](src/frontend/parser/parser.c:241) |

**预留关键字检测函数**：[`parser.c:197-210`](src/frontend/parser/parser.c:197)

---

## 8. 已移除的特性

以下特性在早期版本中存在，但已被移除：

### 8.1 中断处理关键字

**状态**：已移除

**原语法**：
```cn
中断处理 向量号 () { ... }
```

**替代方案**：使用运行时API注册中断处理函数

**代码来源**：[`parser.c:369-382`](src/frontend/parser/parser.c:369)

### 8.2 内存操作关键字

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

### 8.3 内联汇编关键字

**状态**：已移除

**原语法**：
```cn
内联汇编("asm code", outputs, inputs, clobbers)
```

**替代方案**：使用运行时API `cn_rt_inline_asm()`

**代码来源**：[`parser.c:2242-2299`](src/frontend/parser/parser.c:2242)

---

## 9. 代码引用示例

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

## 10. 附录

### 10.1 完整关键字列表（37个）

**已实现关键字（27个）**：
- 控制流：`如果`、`否则`、`当`、`循环`、`返回`、`中断`、`继续`、`选择`、`情况`、`默认`
- 类型：`整数`、`小数`、`字符串`、`布尔`、`空类型`、`结构体`、`枚举`
- 声明：`函数`、`变量`、`模块`、`导入`、`从`、`公开`、`私有`、`常量`、`静态`
- 常量：`真`、`假`、`无`

**预留关键字（8个）**：
`命名空间`、`接口`、`类`、`模板`、`保护`、`虚拟`、`重写`、`抽象`

### 10.2 源代码文件索引

| 文件路径 | 主要功能 |
|----------|----------|
| `src/frontend/lexer/keywords.c` | 关键字定义表 |
| `include/cnlang/frontend/token.h` | Token类型枚举 |
| `src/frontend/lexer/lexer.c` | 词法分析器实现 |
| `src/frontend/parser/parser.c` | 语法分析器实现 |
| `include/cnlang/frontend/semantics.h` | 类型系统定义 |
| `src/semantics/symbols/type_system.c` | 类型系统实现 |

---

**文档结束**

> 本文档严格基于源代码编写，所有规则均有代码依据。如有疑问，请参考对应的源代码文件。

