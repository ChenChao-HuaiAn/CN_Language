# Token 模块设计文档

## 概述

Token模块是CN_Language项目的核心组件之一，负责定义和操作词法分析过程中产生的词法单元（Token）。本文档详细描述了Token模块的设计原理、架构决策和技术实现。

## 设计目标

### 1. 核心目标
- **高效性**：支持高性能的词法分析，内存使用和CPU开销最小化
- **可扩展性**：易于添加新的Token类型和功能
- **可靠性**：健壮的错误处理和内存管理
- **易用性**：简洁直观的API接口

### 2. 质量属性
- **性能**：支持批量处理和缓存优化
- **可维护性**：清晰的代码结构和完整的文档
- **可测试性**：模块化设计便于单元测试
- **可移植性**：跨平台兼容性

## 架构设计

### 1. 模块分层架构

```
┌─────────────────────────────────────┐
│          应用层 (Application)        │
│  ┌─────────────────────────────┐    │
│  │     词法分析器 (Lexer)      │    │
│  └─────────────────────────────┘    │
└─────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────┐
│          接口层 (Interface)          │
│  ┌─────────────────────────────┐    │
│  │      Token API 接口         │    │
│  └─────────────────────────────┘    │
└─────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────┐
│          核心层 (Core)              │
│  ┌─────────────────────────────┐    │
│  │   Token 数据结构定义        │    │
│  │   Token 操作函数实现        │    │
│  └─────────────────────────────┘    │
└─────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────┐
│          基础层 (Foundation)         │
│  ┌─────────────────────────────┐    │
│  │     内存管理 (Memory)       │    │
│  │     错误处理 (Error)        │    │
│  └─────────────────────────────┘    │
└─────────────────────────────────────┘
```

### 2. 核心数据结构

#### 2.1 Token类型枚举 (Eum_TokenType)
```c
enum Eum_TokenType {
    // 关键字类别 (70+个预定义关键字)
    Eum_TOKEN_KEYWORD_VARIABLE = 1,
    Eum_TOKEN_KEYWORD_IF,
    Eum_TOKEN_KEYWORD_FUNCTION,
    // ... 其他关键字
    
    // 标识符
    Eum_TOKEN_IDENTIFIER,
    
    // 运算符类别
    Eum_TOKEN_OPERATOR_PLUS,
    Eum_TOKEN_OPERATOR_ASSIGN,
    // ... 其他运算符
    
    // 字面量类别
    Eum_TOKEN_INTEGER_LITERAL,
    Eum_TOKEN_STRING_LITERAL,
    // ... 其他字面量
    
    // 分隔符类别
    Eum_TOKEN_SEMICOLON,
    Eum_TOKEN_COMMA,
    // ... 其他分隔符
    
    // 特殊Token
    Eum_TOKEN_EOF,
    Eum_TOKEN_ERROR
};
```

#### 2.2 Token结构体 (Stru_Token_t)
```c
typedef struct Stru_Token_t {
    enum Eum_TokenType type;           // Token类型
    char* lexeme;                      // 词素文本（UTF-8编码）
    size_t length;                     // 词素长度（字节数）
    Stru_TokenPosition_t position;     // 源代码位置信息
} Stru_Token_t;
```

#### 2.3 位置信息结构体 (Stru_TokenPosition_t)
```c
typedef struct Stru_TokenPosition_t {
    int line;                          // 行号（从1开始）
    int column;                        // 列号（从1开始）
    size_t offset;                     // 文件偏移量（字节）
} Stru_TokenPosition_t;
```

## 设计决策

### 1. 内存管理策略

#### 1.1 动态内存分配
- **Token创建**：使用`malloc`分配内存，确保词素字符串的独立副本
- **Token销毁**：使用`free`释放所有分配的内存
- **内存泄漏防护**：所有创建函数都有对应的销毁函数

#### 1.2 所有权模型
- **明确的所有权**：创建者负责销毁
- **复制语义**：`F_token_copy`创建深拷贝
- **无共享所有权**：避免复杂的引用计数

### 2. 错误处理设计

#### 2.1 防御性编程
- **参数验证**：所有公共函数都验证输入参数
- **NULL安全**：正确处理NULL指针输入
- **资源清理**：错误时正确释放已分配的资源

#### 2.2 错误指示
- **返回NULL**：创建失败时返回NULL
- **布尔返回值**：比较函数返回true/false
- **错误Token**：专门的`Eum_TOKEN_ERROR`类型

### 3. 性能优化设计

#### 3.1 缓存策略
- **类型名称缓存**：Token类型到字符串的映射使用静态数组
- **类别信息缓存**：Token类别信息预计算
- **运算符信息缓存**：优先级和结合性信息预定义

#### 3.2 批量处理支持
- **批量创建/销毁**：支持高效处理大量Token
- **内存池模式**：可选的Token池实现
- **零拷贝优化**：避免不必要的内存复制

## 接口设计原则

### 1. 一致性原则
- **命名一致性**：所有函数使用`F_`前缀
- **参数顺序**：一致的参数顺序（类型、词素、长度、行、列）
- **返回值一致性**：相似功能使用相似的返回值类型

### 2. 最小接口原则
- **只暴露必要接口**：头文件只包含公共API
- **隐藏实现细节**：结构体定义在源文件中
- **稳定的ABI**：公共接口保持向后兼容

### 3. 自文档化原则
- **清晰的函数名**：函数名明确表达功能
- **完整的注释**：所有公共函数都有Doxygen注释
- **示例代码**：提供丰富的使用示例

## 实现细节

### 1. Token创建实现
```c
Stru_Token_t* F_token_create(enum Eum_TokenType type, 
                            const char* lexeme, 
                            size_t length, 
                            int line, 
                            int column)
{
    // 1. 参数验证
    if (lexeme == NULL && length > 0) {
        return NULL;
    }
    
    // 2. 分配内存
    Stru_Token_t* token = (Stru_Token_t*)malloc(sizeof(Stru_Token_t));
    if (token == NULL) {
        return NULL;
    }
    
    // 3. 初始化字段
    token->type = type;
    token->length = length;
    token->position.line = line;
    token->position.column = column;
    token->position.offset = 0; // 可由调用者设置
    
    // 4. 复制词素
    if (length > 0) {
        token->lexeme = (char*)malloc(length + 1);
        if (token->lexeme == NULL) {
            free(token);
            return NULL;
        }
        memcpy(token->lexeme, lexeme, length);
        token->lexeme[length] = '\0';
    } else {
        token->lexeme = NULL;
    }
    
    return token;
}
```

### 2. 类型信息查询实现
```c
const char* F_token_type_to_string(enum Eum_TokenType type)
{
    // 使用静态数组缓存类型名称
    static const char* type_names[] = {
        [Eum_TOKEN_KEYWORD_VARIABLE] = "变量",
        [Eum_TOKEN_KEYWORD_IF] = "如果",
        // ... 其他类型名称
    };
    
    if (type >= 0 && type < sizeof(type_names) / sizeof(type_names[0])) {
        return type_names[type];
    }
    
    return "未知类型";
}
```

### 3. 运算符信息管理
```c
typedef struct {
    enum Eum_TokenType type;
    int precedence;
    int associativity; // 0=左结合, 1=右结合
} OperatorInfo;

static const OperatorInfo g_operator_info[] = {
    {Eum_TOKEN_OPERATOR_MULTIPLY, 3, 0},
    {Eum_TOKEN_OPERATOR_PLUS, 4, 0},
    {Eum_TOKEN_OPERATOR_ASSIGN, 10, 1},
    // ... 其他运算符信息
};

int F_token_get_precedence(enum Eum_TokenType type)
{
    for (size_t i = 0; i < sizeof(g_operator_info) / sizeof(g_operator_info[0]); i++) {
        if (g_operator_info[i].type == type) {
            return g_operator_info[i].precedence;
        }
    }
    return -1; // 不是运算符
}
```

## 扩展性设计

### 1. 添加新的Token类型
1. 在`Eum_TokenType`枚举中添加新类型
2. 在`F_token_type_to_string`函数中添加类型名称
3. 在`F_token_get_category`函数中指定类别
4. 如果需要，在运算符信息表中添加优先级和结合性

### 2. 添加新的功能
1. 在头文件中声明新的公共函数
2. 在源文件中实现功能
3. 添加相应的测试用例
4. 更新API文档和示例

### 3. 自定义配置
- **内存分配器**：可通过宏替换默认的malloc/free
- **调试输出**：使用`CN_DEBUG`宏控制调试信息
- **Unicode支持**：支持UTF-8编码的中文字符

## 集成指南

### 1. 与词法分析器集成
```c
// 词法分析器使用Token模块的典型模式
Stru_Token_t* next_token(Lexer* lexer)
{
    // 1. 跳过空白字符
    skip_whitespace(lexer);
    
    // 2. 识别Token类型
    enum Eum_TokenType type = identify_token_type(lexer);
    
    // 3. 提取词素
    const char* lexeme = extract_lexeme(lexer);
    size_t length = get_lexeme_length(lexer);
    
    // 4. 创建Token
    return F_token_create(type, lexeme, length, 
                         lexer->line, lexer->column);
}
```

### 2. 与语法分析器集成
```c
// 语法分析器使用Token信息的典型模式
void parse_expression(Parser* parser)
{
    Stru_Token_t* token = parser->current_token;
    
    // 使用Token类型信息
    if (F_token_is_operator(token->type)) {
        int precedence = F_token_get_precedence(token->type);
        int associativity = F_token_get_associativity(token->type);
        
        // 根据优先级和结合性解析表达式
        parse_operator_expression(parser, precedence, associativity);
    }
}
```

## 测试策略

### 1. 单元测试
- **边界测试**：测试最小/最大长度、行号、列号
- **错误测试**：测试无效参数和错误条件
- **功能测试**：测试所有公共函数的功能

### 2. 集成测试
- **与词法分析器集成**：测试Token在真实场景中的使用
- **性能测试**：测试批量处理和内存使用
- **兼容性测试**：测试跨平台行为

### 3. 压力测试
- **内存压力**：测试大量Token的创建和销毁
- **性能压力**：测试高频率的Token操作
- **并发测试**：测试线程安全性（如果支持）

## 性能考虑

### 1. 内存使用优化
- **词素共享**：可选的词素字符串共享机制
- **内存池**：Token对象的内存池实现
- **栈分配**：对小Token使用栈分配

### 2. CPU性能优化
- **内联函数**：对小型关键函数使用内联
- **查找表**：使用查找表代替条件判断
- **预计算**：预计算常用信息

### 3. 缓存友好性
- **数据结构对齐**：优化内存访问模式
- **局部性原理**：相关数据放在一起
- **减少间接访问**：减少指针解引用

## 安全考虑

### 1. 输入验证
- **缓冲区溢出防护**：严格验证长度参数
- **整数溢出检查**：检查行号、列号等数值
- **空指针检查**：所有指针参数都验证

### 2. 内存安全
- **初始化所有字段**：避免未初始化内存
- **正确释放内存**：匹配的malloc/free调用
- **防止双重释放**：设置指针为NULL after free

### 3. 线程安全
- **当前设计**：非线程安全，每个线程使用独立实例
- **未来扩展**：可添加线程安全版本
- **最佳实践**：避免全局状态

## 维护指南

### 1. 代码维护
- **单一职责**：每个函数不超过50行
- **模块化**：每个.c文件不超过500行
- **清晰注释**：重要算法和决策都有注释

### 2. 文档维护
- **API文档**：随代码更新API文档
- **设计文档**：记录重要的设计决策
- **示例代码**：保持示例代码与API同步

### 3. 版本管理
- **语义版本**：遵循语义版本控制
- **变更日志**：记录所有不兼容的变更
- **向后兼容**：尽可能保持API稳定

## 总结

Token模块是CN_Language项目的基础组件，采用了模块化、高效和可靠的设计原则。通过清晰的接口设计、健壮的错误处理和性能优化，为词法分析提供了坚实的基础。

模块的设计充分考虑了可扩展性和可维护性，支持未来的功能扩展和性能优化。通过完整的文档和丰富的示例，开发者可以快速理解和使用Token模块。

## 参考资料

1. [CN_Language项目技术规范](../docs/specifications/CN_Language项目%20技术规范和编码标准.md)
2. [Token模块API文档](API.md)
3. [Token模块使用示例](EXAMPLES.md)
4. [Token模块README](README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-02 | 初始版本，包含完整的Token类型定义和基本操作 |
| 1.1.0 | 未来 | 计划添加Token池和性能优化功能 |

## 作者和维护者

- **设计**：CN_Language开发团队
- **实现**：CN_Language开发团队
- **维护**：CN_Language开发团队

## 许可证

MIT License - 详见项目根目录的LICENSE文件
