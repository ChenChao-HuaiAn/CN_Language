# CN_Language Token模块

## 概述

CN_Language Token模块定义了词法分析过程中使用的Token类型和相关操作。Token是词法分析的基本单位，表示源代码中的最小有意义的元素，如关键字、标识符、字面量、运算符和分隔符。

## 功能特性

- **完整的Token类型定义**：支持70+种Token类型，涵盖CN语言的所有语法元素
- **Token生命周期管理**：提供Token创建、复制、销毁等完整生命周期管理
- **Token信息查询**：支持Token类型转换、分类判断、优先级查询等功能
- **Token比较和打印**：提供Token相等性比较和格式化输出功能
- **内存安全**：所有动态分配的内存都有对应的释放函数
- **错误处理**：对无效参数进行安全检查，避免程序崩溃

## 目录结构

```
src/core/lexer/token/
├── CN_token.h           # Token类型定义和接口声明
├── CN_token.c           # Token类型实现
├── README.md            # 本文件
├── API.md               # API接口文档
├── DESIGN.md            # 设计原理
└── EXAMPLES.md          # 使用示例
```

## 快速开始

### 包含头文件

```c
#include "CN_token.h"
```

### 基本使用示例

```c
#include <stdio.h>
#include "CN_token.h"

int main()
{
    // 1. 创建Token
    Stru_Token_t* token = F_token_create(Eum_TOKEN_IDENTIFIER, "变量名", 6, 1, 1);
    if (token == NULL) {
        printf("创建Token失败\n");
        return 1;
    }
    
    // 2. 打印Token信息
    printf("Token信息: ");
    F_token_print(token, true, false);
    
    // 3. 获取Token类型信息
    const char* type_name = F_token_type_to_string(token->type);
    const char* category = F_token_get_category(token->type);
    printf("Token类型: %s\n", type_name);
    printf("Token类别: %s\n", category);
    
    // 4. 检查Token属性
    if (F_token_is_keyword(token->type)) {
        printf("这是一个关键字Token\n");
    } else if (F_token_is_identifier(token->type)) {
        printf("这是一个标识符Token\n");
    }
    
    // 5. 复制Token
    Stru_Token_t* token_copy = F_token_copy(token);
    if (token_copy != NULL) {
        printf("Token复制成功\n");
        
        // 6. 比较Token
        if (F_token_equals(token, token_copy)) {
            printf("两个Token相等\n");
        }
        
        F_token_destroy(token_copy);
    }
    
    // 7. 销毁Token
    F_token_destroy(token);
    
    return 0;
}
```

## Token类型分类

Token模块支持以下主要Token类别：

### 1. 关键字Token (70+种)
- **变量和类型声明**：变量、整数、小数、字符串、布尔、数组、结构体、枚举、指针、引用
- **控制结构**：如果、否则、当、循环、中断、继续、选择、情况、默认
- **函数相关**：函数、返回、主程序、参数、递归
- **逻辑运算符**：与、且、或、非
- **字面量**：真、假、空、无
- **模块系统**：模块、导入、导出、包、命名空间
- **运算符**：加、减、乘、除、取模、等于、不等于、小于、大于、小于等于、大于等于
- **类型**：类型、接口、类、对象、泛型、模板
- **其他**：常量、静态、公开、私有、保护、虚拟、重写、抽象、最终、同步、异步、等待、抛出、捕获、尝试、最终

### 2. 标识符Token
- `Eum_TOKEN_IDENTIFIER` - 变量名、函数名等用户定义的标识符

### 3. 字面量Token
- `Eum_TOKEN_INTEGER_LITERAL` - 整数字面量
- `Eum_TOKEN_FLOAT_LITERAL` - 小数字面量
- `Eum_TOKEN_STRING_LITERAL` - 字符串字面量
- `Eum_TOKEN_BOOLEAN_LITERAL` - 布尔字面量

### 4. 运算符Token
- 算术运算符：`+`, `-`, `*`, `/`, `%`
- 赋值运算符：`=`, `+=`, `-=`, `*=`, `/=`
- 比较运算符：`==`, `!=`, `<`, `>`, `<=`, `>=`

### 5. 分隔符Token
- 标点符号：`;`, `,`, `.`, `:`, `::`
- 括号：`(`, `)`, `{`, `}`, `[`, `]`

### 6. 特殊Token
- `Eum_TOKEN_EOF` - 文件结束
- `Eum_TOKEN_ERROR` - 错误Token
- `Eum_TOKEN_COMMENT` - 注释
- `Eum_TOKEN_WHITESPACE` - 空白字符
- `Eum_TOKEN_NEWLINE` - 换行符

## 数据结构

### Stru_Token_t
Token的主要数据结构，包含以下字段：
- `type` - Token类型（`Eum_TokenType`枚举）
- `lexeme` - Token的原始字符串表示（动态分配）
- `lexeme_length` - 字符串长度
- `line_number` - 所在行号（从1开始）
- `column_number` - 所在列号（从1开始）
- `literal_value` - 字面量值联合体（仅对字面量Token有效）

### Stru_TokenPosition_t
Token位置信息结构体，用于错误报告和调试：
- `line_number` - 行号
- `column_number` - 列号
- `offset` - 在源代码中的字节偏移量
- `filename` - 源文件名

## 编译和链接

### 编译选项

```bash
# 编译Token模块
gcc -c src/core/lexer/token/CN_token.c -o build/core/lexer/token/CN_token.o -I./src/core/lexer

# 链接到你的程序
gcc your_program.c build/core/lexer/token/CN_token.o -o your_program
```

### 使用Makefile（推荐）

```makefile
# Token模块
TOKEN_SOURCES = src/core/lexer/token/CN_token.c
TOKEN_OBJECTS = $(TOKEN_SOURCES:.c=.o)

# 编译规则
%.o: %.c
    $(CC) -c $< -o $@ $(CFLAGS) -I./src/core/lexer

# 链接规则
your_program: $(TOKEN_OBJECTS) your_program.o
    $(CC) -o $@ $^ $(LDFLAGS)
```

## API参考

详细API文档请参考 [API.md](API.md)。

### 主要函数

1. **Token创建和销毁**
   - `F_token_create()` - 创建新的Token
   - `F_token_create_literal()` - 创建字面量Token
   - `F_token_destroy()` - 销毁Token
   - `F_token_copy()` - 复制Token

2. **Token信息查询**
   - `F_token_type_to_string()` - 获取Token类型名称
   - `F_token_get_category()` - 获取Token类别
   - `F_token_is_keyword()` - 检查是否为关键字
   - `F_token_is_operator()` - 检查是否为运算符
   - `F_token_is_literal()` - 检查是否为字面量
   - `F_token_is_separator()` - 检查是否为分隔符

3. **Token操作**
   - `F_token_print()` - 打印Token信息
   - `F_token_to_string()` - 获取Token的字符串表示
   - `F_token_equals()` - 比较两个Token是否相等

4. **运算符属性**
   - `F_token_get_precedence()` - 获取运算符优先级
   - `F_token_get_associativity()` - 获取运算符结合性

## 内存管理

Token模块使用动态内存管理：

1. **Token创建**：`F_token_create()`和`F_token_create_literal()`动态分配Token结构体和lexeme字符串
2. **Token销毁**：`F_token_destroy()`释放所有关联的内存
3. **Token复制**：`F_token_copy()`创建深拷贝，需要单独销毁
4. **内存安全**：所有函数对NULL参数安全，避免内存泄漏

## 错误处理

- 创建函数在失败时返回NULL
- 查询函数对无效参数返回安全值
- 打印函数对NULL参数安全
- 所有函数都进行参数验证

## 性能考虑

1. **Token创建**：使用动态内存分配，适合词法分析场景
2. **Token查询**：使用常量时间操作，性能高效
3. **内存使用**：按需分配，避免不必要的内存占用
4. **扩展性**：支持新的Token类型添加

## 扩展性

Token模块设计为可扩展：

1. **添加新Token类型**：在`Eum_TokenType`枚举中添加新类型
2. **扩展Token信息**：在`F_token_type_to_string()`中添加新类型的字符串表示
3. **自定义Token类别**：扩展`F_token_get_category()`支持新的分类逻辑

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 贡献指南

1. 遵循项目编码规范
2. 添加单元测试
3. 更新相关文档
4. 保持向后兼容性

## 联系方式

- 项目仓库：https://github.com/ChenChao-HuaiAn/CN_Language
- 问题反馈：使用GitHub Issues
