# CN_Language 令牌模块 API 文档（模块化版本）

## 概述

令牌（Token）模块是CN_Language编译器的核心数据模型，采用模块化设计，遵循SOLID原则和分层架构。本模块将令牌功能拆分为多个子模块，每个子模块专注于单一职责，提高了代码的可维护性和可测试性。

## 模块架构

### 模块结构

```
src/core/token/
├── CN_token.h                    # 主头文件（向后兼容）
├── CN_token.c                    # 适配器（向后兼容）
├── CN_token_interface.h          # 抽象接口定义
├── CN_token_types.h              # 令牌类型定义
├── lifecycle/                    # 生命周期管理
│   ├── CN_token_lifecycle.h
│   └── CN_token_lifecycle.c
├── values/                       # 字面量值操作
│   ├── CN_token_values.h
│   └── CN_token_values.c
├── query/                        # 类型查询
│   ├── CN_token_query.h
│   └── CN_token_query.c
├── keywords/                     # 关键字管理
│   ├── CN_token_keywords.h
│   └── CN_token_keywords.c
├── tools/                        # 工具函数
│   ├── CN_token_tools.h
│   └── CN_token_tools.c
└── interface/                    # 接口实现
    └── CN_token_interface_impl.c
```

### 设计原则

1. **单一职责原则**：每个子模块只负责一个功能领域
2. **开闭原则**：通过抽象接口支持扩展
3. **接口隔离原则**：为不同客户端提供专用接口
4. **依赖倒置原则**：高层模块定义接口，低层模块实现接口

## 核心数据结构

### Eum_TokenType 枚举

定义了所有可能的令牌类型，包括70个中文关键字，分为9个类别。

```c
// 文件：CN_token_types.h
typedef enum Eum_TokenType {
    // 1. 变量和类型声明关键字（10个）
    Eum_TOKEN_KEYWORD_VAR,       // 变量
    Eum_TOKEN_KEYWORD_INT,       // 整数
    Eum_TOKEN_KEYWORD_FLOAT,     // 小数
    // ... 其他67个关键字
    
    // 非关键字令牌类型
    Eum_TOKEN_IDENTIFIER,        // 标识符
    Eum_TOKEN_LITERAL_INTEGER,   // 整数字面量
    Eum_TOKEN_LITERAL_FLOAT,     // 浮点数字面量
    Eum_TOKEN_LITERAL_STRING,    // 字符串字面量
    Eum_TOKEN_LITERAL_BOOLEAN,   // 布尔字面量
    
    // 运算符（符号形式）
    Eum_TOKEN_OPERATOR_PLUS,     // + 加法运算符
    Eum_TOKEN_OPERATOR_MINUS,    // - 减法运算符
    // ... 其他运算符
    
    // 分隔符
    Eum_TOKEN_DELIMITER_COMMA,   // , 逗号分隔符
    Eum_TOKEN_DELIMITER_SEMICOLON, // ; 分号分隔符
    // ... 其他分隔符
    
    // 特殊令牌
    Eum_TOKEN_EOF,               // 文件结束
    Eum_TOKEN_ERROR              // 错误令牌
} Eum_TokenType;
```

### Stru_Token_t 结构体

令牌的核心数据结构。

```c
// 文件：CN_token.h
typedef struct Stru_Token_t {
    Eum_TokenType type;        ///< 令牌类型
    char* lexeme;              ///< 词素（源代码中的字符串）
    size_t line;               ///< 行号（从1开始）
    size_t column;             ///< 列号（从1开始）
    size_t length;             ///< 词素长度（字节数）
    
    /**
     * @brief 字面量值联合体
     * 
     * 根据令牌类型存储相应的字面量值。
     */
    union {
        long int_value;        ///< 整数值（如果是整数字面量）
        double float_value;    ///< 浮点数值（如果是浮点数字面量）
        bool bool_value;       ///< 布尔值（如果是布尔字面量）
    } literal_value;
} Stru_Token_t;
```

### Stru_TokenInterface_t 结构体

令牌模块的抽象接口，遵循依赖倒置原则。

```c
// 文件：CN_token_interface.h
typedef struct Stru_TokenInterface_t {
    // 令牌生命周期管理
    Stru_Token_t* (*create_token)(Eum_TokenType type, const char* lexeme, size_t line, size_t column);
    void (*destroy_token)(Stru_Token_t* token);
    Stru_Token_t* (*copy_token)(const Stru_Token_t* token);
    
    // 令牌属性访问
    Eum_TokenType (*get_token_type)(const Stru_Token_t* token);
    const char* (*get_token_lexeme)(const Stru_Token_t* token);
    void (*get_token_position)(const Stru_Token_t* token, size_t* line, size_t* column);
    
    // 字面量值操作
    void (*set_int_value)(Stru_Token_t* token, long value);
    void (*set_float_value)(Stru_Token_t* token, double value);
    void (*set_bool_value)(Stru_Token_t* token, bool value);
    long (*get_int_value)(const Stru_Token_t* token);
    double (*get_float_value)(const Stru_Token_t* token);
    bool (*get_bool_value)(const Stru_Token_t* token);
    
    // 令牌类型查询
    bool (*is_keyword)(Eum_TokenType type);
    bool (*is_operator)(Eum_TokenType type);
    bool (*is_literal)(Eum_TokenType type);
    bool (*is_delimiter)(Eum_TokenType type);
    
    // 关键字信息查询
    int (*get_precedence)(Eum_TokenType type);
    const char* (*get_chinese_keyword)(Eum_TokenType type);
    const char* (*get_english_keyword)(Eum_TokenType type);
    int (*get_keyword_category)(Eum_TokenType type);
    
    // 工具函数
    const char* (*type_to_string)(Eum_TokenType type);
    bool (*equals)(const Stru_Token_t* token1, const Stru_Token_t* token2);
    int (*to_string)(const Stru_Token_t* token, char* buffer, size_t buffer_size);
    
    // 模块生命周期管理
    bool (*initialize)(void);
    void (*cleanup)(void);
} Stru_TokenInterface_t;
```

## 子模块 API

### 生命周期管理模块 (lifecycle)

#### F_token_lifecycle_create

创建新令牌。

```c
Stru_Token_t* F_token_lifecycle_create(Eum_TokenType type, const char* lexeme, size_t line, size_t column);
```

#### F_token_lifecycle_destroy

销毁令牌。

```c
void F_token_lifecycle_destroy(Stru_Token_t* token);
```

#### F_token_lifecycle_copy

复制令牌。

```c
Stru_Token_t* F_token_lifecycle_copy(const Stru_Token_t* token);
```

#### F_token_lifecycle_equals

比较两个令牌。

```c
bool F_token_lifecycle_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);
```

### 字面量值操作模块 (values)

#### F_token_values_set_int

设置整数字面量值。

```c
void F_token_values_set_int(Stru_Token_t* token, long value);
```

#### F_token_values_set_float

设置浮点数字面量值。

```c
void F_token_values_set_float(Stru_Token_t* token, double value);
```

#### F_token_values_set_bool

设置布尔字面量值。

```c
void F_token_values_set_bool(Stru_Token_t* token, bool value);
```

#### F_token_values_get_int

获取整数字面量值。

```c
long F_token_values_get_int(const Stru_Token_t* token);
```

#### F_token_values_get_float

获取浮点数字面量值。

```c
double F_token_values_get_float(const Stru_Token_t* token);
```

#### F_token_values_get_bool

获取布尔字面量值。

```c
bool F_token_values_get_bool(const Stru_Token_t* token);
```

### 类型查询模块 (query)

#### F_token_query_is_keyword

判断是否为关键字令牌。

```c
bool F_token_query_is_keyword(Eum_TokenType type);
```

#### F_token_query_is_operator

判断是否为运算符令牌。

```c
bool F_token_query_is_operator(Eum_TokenType type);
```

#### F_token_query_is_literal

判断是否为字面量令牌。

```c
bool F_token_query_is_literal(Eum_TokenType type);
```

#### F_token_query_is_delimiter

判断是否为分隔符令牌。

```c
bool F_token_query_is_delimiter(Eum_TokenType type);
```

#### F_token_query_get_precedence

获取运算符优先级。

```c
int F_token_query_get_precedence(Eum_TokenType type);
```

### 关键字管理模块 (keywords)

#### F_token_keywords_find_info

查找关键字信息。

```c
const Stru_KeywordInfo_t* F_token_keywords_find_info(Eum_TokenType type);
```

#### F_token_keywords_get_chinese

获取关键字的中文表示。

```c
const char* F_token_keywords_get_chinese(Eum_TokenType type);
```

#### F_token_keywords_get_english

获取关键字的英文表示。

```c
const char* F_token_keywords_get_english(Eum_TokenType type);
```

#### F_token_keywords_get_category

获取关键字的分类。

```c
int F_token_keywords_get_category(Eum_TokenType type);
```

#### F_token_keywords_get_count

获取关键字表大小。

```c
size_t F_token_keywords_get_count(void);
```

#### F_token_keywords_get_all

获取所有关键字信息。

```c
const Stru_KeywordInfo_t* F_token_keywords_get_all(void);
```

### 工具函数模块 (tools)

#### F_token_tools_type_to_string

令牌类型转字符串。

```c
const char* F_token_tools_type_to_string(Eum_TokenType type);
```

#### F_token_tools_to_string

打印令牌信息。

```c
int F_token_tools_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);
```

#### F_token_tools_print

打印令牌信息到文件。

```c
int F_token_tools_print(const Stru_Token_t* token, FILE* stream);
```

#### F_token_tools_format_position

格式化令牌位置信息。

```c
int F_token_tools_format_position(size_t line, size_t column, char* buffer, size_t buffer_size);
```

## 接口使用

### 获取默认接口实例

```c
const Stru_TokenInterface_t* F_get_token_interface(void);
```

### 通过接口使用令牌功能

```c
#include "src/core/token/CN_token_interface.h"

int main(void) {
    // 获取令牌接口
    const Stru_TokenInterface_t* token_if = F_get_token_interface();
    
    // 初始化模块
    token_if->initialize();
    
    // 创建令牌
    Stru_Token_t* token = token_if->create_token(
        Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    
    // 使用令牌功能
    Eum_TokenType type = token_if->get_token_type(token);
    const char* lexeme = token_if->get_token_lexeme(token);
    
    // 设置和获取值
    token_if->set_int_value(token, 42);
    long value = token_if->get_int_value(token);
    
    // 类型查询
    bool is_keyword = token_if->is_keyword(type);
    
    // 打印令牌信息
    char buffer[256];
    token_if->to_string(token, buffer, sizeof(buffer));
    printf("令牌信息：%s\n", buffer);
    
    // 清理
    token_if->destroy_token(token);
    token_if->cleanup();
    
    return 0;
}
```

## 向后兼容性

### 适配器模式

为了保持与现有代码的兼容性，提供了`CN_token.c`作为适配器，将旧接口映射到新模块化实现。

```c
// 旧代码继续工作
Stru_Token_t* token = F_create_token(Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
F_token_set_int_value(token, 42);
F_destroy_token(token);
```

### 兼容函数

所有旧函数仍然可用，但内部实现已改为调用新的模块化函数：

- `F_create_token()` → `F_token_lifecycle_create()`
- `F_destroy_token()` → `F_token_lifecycle_destroy()`
- `F_token_set_int_value()` → `F_token_values_set_int()`
- `F_token_is_keyword()` → `F_token_query_is_keyword()`
- 等等...

## 测试框架

### 测试结构

```
tests/core/token/
├── test_token_runner.c           # 总测试运行器
├── lifecycle/
│   └── test_token_lifecycle.c    # 生命周期管理测试
├── values/
│   └── test_token_values.c       # 字面量值操作测试
├── query/
│   └── test_token_query.c        # 类型查询测试
├── keywords/
│   └── test_token_keywords.c     # 关键字管理测试
├── tools/
│   └── test_token_tools.c        # 工具函数测试
└── interface/
    └── test_token_interface.c    # 接口测试
```

### 运行测试

```bash
# 编译所有测试
gcc -I./src -I./src/core/token \
    tests/core/token/test_token_runner.c \
    tests/core/token/lifecycle/test_token_lifecycle.c \
    tests/core/token/values/test_token_values.c \
    tests/core/token/query/test_token_query.c \
    tests/core/token/keywords/test_token_keywords.c \
    tests/core/token/tools/test_token_tools.c \
    tests/core/token/interface/test_token_interface.c \
    src/core/token/lifecycle/CN_token_lifecycle.c \
    src/core/token/values/CN_token_values.c \
    src/core/token/query/CN_token_query.c \
    src/core/token/keywords/CN_token_keywords.c \
    src/core/token/tools/CN_token_tools.c \
    src/core/token/interface/CN_token_interface_impl.c \
    src/core/token/CN_token.c \
    -o test_token

# 运行测试
./test_token
```

## 性能考虑

### 模块化开销

模块化设计引入了轻微的函数调用开销，但通过以下方式优化：

1. **内联函数**：关键路径上的函数可以内联
2. **编译时优化**：编译器可以优化跨模块调用
3. **缓存友好**：每个模块的数据局部性更好

### 内存使用

每个子模块独立管理自己的内存，减少了内存碎片：

1. **生命周期模块**：管理令牌结构体内存
2. **关键字模块**：静态关键字表，无动态分配
3. **工具模块**：临时缓冲区，可重用

## 扩展指南

### 添加新子模块

1. 在`src/core/token/`下创建新目录
2. 实现`.h`和`.c`文件
3. 在接口中定义新函数
4. 在`CN_token_interface_impl.c`中实现接口函数
5. 更新测试框架

### 替换实现

通过接口可以轻松替换模块实现：

```c
// 自定义令牌接口实现
static const Stru_TokenInterface_t g_custom_token_interface = {
    .create_token = custom_create_token,
    .destroy_token = custom_destroy_token,
    // ... 其他函数
};

// 使用自定义接口
const Stru_TokenInterface_t* get_custom_token_interface(void) {
    return &g_custom_token_interface;
}
```

## 迁移指南

### 从旧版本迁移

1. **立即兼容**：现有代码无需修改
2. **逐步迁移**：逐步使用新接口
3. **最终迁移**：移除旧函数调用

### 推荐迁移步骤

1. 继续使用旧函数（向后兼容）
2. 在新代码中使用新接口
3. 逐步将旧代码迁移到新接口
4. 最终移除旧函数（如果需要）

## 相关文档

- [CN_token.md](./CN_token.md) - 旧版本API文档
- [架构设计文档](../../../architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../specifications/CN_Language项目%20技术规范和编码标准.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 2.0.0 | 2026-01-08 | 模块化重构，引入抽象接口 |
| 2.0.1 | 2026-01-08 | 修复编译问题，完善测试 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
