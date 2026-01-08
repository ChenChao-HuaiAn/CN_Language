# CN_Language 词法分析器接口 API 文档

## 概述

词法分析器接口是CN_Language编译器的核心组件之一，负责将源代码字符流转换为令牌序列。本接口采用面向接口的编程模式，提供完整的抽象接口定义，支持所有70个中文关键字的识别。

## 模块化架构

词法分析器采用模块化设计，分为以下层次：

### 1. 接口层
- **CN_lexer_interface.h/.c** - 抽象接口定义和工厂函数
- **interface/CN_lexer_interface_impl.h/.c** - 接口具体实现

### 2. 核心层
- **scanner/** - 字符扫描和位置管理
- **token_scanners/** - 各种令牌类型的扫描器
- **keywords/** - 70个中文关键字识别
- **operators/** - 运算符识别
- **utils/** - 工具函数（字符分类、字符串处理等）

### 3. 适配器层
- **CN_lexer_impl.c** - 向后兼容的适配器实现
- **CN_lexer_impl_legacy.c** - 原始实现（备份）

### 架构优势
- **单一职责原则**：每个模块只负责一个功能领域
- **接口隔离原则**：通过抽象接口提供功能，隐藏实现细节
- **依赖倒置原则**：高层模块定义接口，低层模块实现接口
- **开闭原则**：对扩展开放，对修改封闭

## 接口定义

### Stru_LexerInterface_t

词法分析器的主要接口结构体，定义所有公共方法。

```c
typedef struct Stru_LexerInterface_t {
    // 初始化函数
    bool (*initialize)(struct Stru_LexerInterface_t* interface, 
                      const char* source, size_t length, 
                      const char* source_name);
    
    // 核心功能
    Stru_Token_t* (*next_token)(struct Stru_LexerInterface_t* interface);
    bool (*has_more_tokens)(struct Stru_LexerInterface_t* interface);
    Stru_DynamicArray_t* (*tokenize_all)(struct Stru_LexerInterface_t* interface);
    
    // 状态查询
    void (*get_position)(struct Stru_LexerInterface_t* interface, 
                        size_t* line, size_t* column);
    const char* (*get_source_name)(struct Stru_LexerInterface_t* interface);
    
    // 错误处理
    bool (*has_errors)(struct Stru_LexerInterface_t* interface);
    const char* (*get_last_error)(struct Stru_LexerInterface_t* interface);
    
    // 资源管理
    void (*reset)(struct Stru_LexerInterface_t* interface);
    void (*destroy)(struct Stru_LexerInterface_t* interface);
    
    // 内部状态
    void* internal_state;
} Stru_LexerInterface_t;
```

## API 参考

### initialize

初始化词法分析器，设置源代码和源文件名。

**函数签名：**
```c
bool initialize(Stru_LexerInterface_t* interface, 
                const char* source, 
                size_t length, 
                const char* source_name);
```

**参数：**
- `interface`：词法分析器接口指针
- `source`：源代码字符串（UTF-8编码）
- `length`：源代码长度（字节数）
- `source_name`：源文件名（可选，可为NULL）

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

**示例：**
```c
Stru_LexerInterface_t* lexer = F_create_lexer_interface();
const char* source = "变量 x = 42";
bool success = lexer->initialize(lexer, source, strlen(source), "test.cn");
```

### next_token

获取源代码中的下一个令牌。

**函数签名：**
```c
Stru_Token_t* next_token(Stru_LexerInterface_t* interface);
```

**参数：**
- `interface`：词法分析器接口指针

**返回值：**
- `Stru_Token_t*`：下一个令牌指针
- `NULL`：错误或已到达文件末尾

**注意事项：**
- 调用者负责销毁返回的令牌
- 当返回`Eum_TOKEN_EOF`类型的令牌时，表示已到达文件末尾
- 错误令牌的类型为`Eum_TOKEN_ERROR`

**示例：**
```c
while (lexer->has_more_tokens(lexer)) {
    Stru_Token_t* token = lexer->next_token(lexer);
    if (token != NULL) {
        // 处理令牌
        F_destroy_token(token);
    }
}
```

### has_more_tokens

检查是否还有更多令牌可读取。

**函数签名：**
```c
bool has_more_tokens(Stru_LexerInterface_t* interface);
```

**参数：**
- `interface`：词法分析器接口指针

**返回值：**
- `true`：还有更多令牌
- `false`：没有更多令牌或发生错误

**实现说明：**
此方法会跳过空白字符和注释，然后检查是否还有非空白字符。

### tokenize_all

将整个源代码一次性令牌化，返回所有令牌的数组。

**函数签名：**
```c
Stru_DynamicArray_t* tokenize_all(Stru_LexerInterface_t* interface);
```

**参数：**
- `interface`：词法分析器接口指针

**返回值：**
- `Stru_DynamicArray_t*`：包含所有令牌的动态数组
- `NULL`：令牌化失败

**注意事项：**
- 调用者负责销毁返回的数组和其中的所有令牌
- 数组中的元素类型为`Stru_Token_t*`
- 数组的最后一个元素是`Eum_TOKEN_EOF`令牌

**示例：**
```c
Stru_DynamicArray_t* tokens = lexer->tokenize_all(lexer);
if (tokens != NULL) {
    size_t count = F_dynamic_array_length(tokens);
    for (size_t i = 0; i < count; i++) {
        Stru_Token_t** token_ptr = (Stru_Token_t**)F_dynamic_array_get(tokens, i);
        if (token_ptr != NULL && *token_ptr != NULL) {
            // 处理令牌
            F_destroy_token(*token_ptr);
        }
    }
    F_destroy_dynamic_array(tokens);
}
```

### get_position

获取当前词法分析的位置（行号和列号）。

**函数签名：**
```c
void get_position(Stru_LexerInterface_t* interface, 
                  size_t* line, 
                  size_t* column);
```

**参数：**
- `interface`：词法分析器接口指针
- `line`：输出参数，行号（从1开始）
- `column`：输出参数，列号（从1开始）

**示例：**
```c
size_t line, column;
lexer->get_position(lexer, &line, &column);
printf("当前位置：第%zu行，第%zu列\n", line, column);
```

### get_source_name

获取当前正在分析的源文件名。

**函数签名：**
```c
const char* get_source_name(Stru_LexerInterface_t* interface);
```

**参数：**
- `interface`：词法分析器接口指针

**返回值：**
- `const char*`：源文件名
- 如果未设置源文件名，返回"unknown.cn"

### has_errors

检查词法分析过程中是否发生了错误。

**函数签名：**
```c
bool has_errors(Stru_LexerInterface_t* interface);
```

**参数：**
- `interface`：词法分析器接口指针

**返回值：**
- `true`：有错误
- `false`：没有错误

### get_last_error

获取最后一个错误的详细描述。

**函数签名：**
```c
const char* get_last_error(Stru_LexerInterface_t* interface);
```

**参数：**
- `interface`：词法分析器接口指针

**返回值：**
- `const char*`：错误信息字符串
- 如果没有错误，返回空字符串""

### reset

重置词法分析器到初始状态，可以重新分析相同的源代码。

**函数签名：**
```c
void reset(Stru_LexerInterface_t* interface);
```

**参数：**
- `interface`：词法分析器接口指针

**效果：**
- 将当前位置重置为源代码开头
- 清除所有错误状态
- 行号和列号重置为1:1

### destroy

销毁词法分析器，释放所有资源。

**函数签名：**
```c
void destroy(Stru_LexerInterface_t* interface);
```

**参数：**
- `interface`：词法分析器接口指针

**注意事项：**
- 调用此函数后，接口指针不再有效
- 此函数会释放所有内部状态和资源
- 如果接口是通过`F_create_lexer_interface()`创建的，必须调用此函数

## 工厂函数

### F_create_lexer_interface

创建词法分析器接口实例。

**函数签名：**
```c
Stru_LexerInterface_t* F_create_lexer_interface(void);
```

**返回值：**
- `Stru_LexerInterface_t*`：新创建的词法分析器接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_LexerInterface_t* lexer = F_create_lexer_interface();
if (lexer == NULL) {
    fprintf(stderr, "无法创建词法分析器\n");
    return 1;
}

// 使用词法分析器...

lexer->destroy(lexer);
```

## 错误处理

### 错误类型

词法分析器可能遇到以下类型的错误：

1. **初始化错误**：无效的源代码或参数
2. **字符编码错误**：无效的UTF-8序列
3. **令牌识别错误**：无法识别的字符或令牌
4. **内存错误**：内存分配失败
5. **状态错误**：在无效状态下调用方法

### 错误信息格式

错误信息包含以下内容：
- 错误类型描述
- 错误位置（行号、列号）
- 相关字符或上下文
- 建议的修复方法

**示例错误信息：**
```
第3行第15列：未知字符 '#' (0x23)，期望标识符、数字或运算符
```

### 错误恢复

词法分析器实现以下错误恢复策略：

1. **跳过无效字符**：跳过无法识别的字符，继续分析
2. **部分令牌恢复**：对于部分有效的令牌，尽可能恢复
3. **错误令牌标记**：创建错误令牌，包含错误信息
4. **状态保存**：保存错误发生前的状态，便于调试

## 使用模式

### 基本使用模式

```c
// 创建词法分析器
Stru_LexerInterface_t* lexer = F_create_lexer_interface();
if (lexer == NULL) {
    // 处理创建失败
}

// 初始化
const char* source = "变量 x = 42 + 3.14";
if (!lexer->initialize(lexer, source, strlen(source), "example.cn")) {
    // 处理初始化失败
    const char* error = lexer->get_last_error(lexer);
    fprintf(stderr, "初始化失败：%s\n", error);
    lexer->destroy(lexer);
    return 1;
}

// 逐个处理令牌
while (lexer->has_more_tokens(lexer)) {
    Stru_Token_t* token = lexer->next_token(lexer);
    if (token == NULL) {
        // 处理获取令牌失败
        break;
    }
    
    // 检查错误
    if (token->type == Eum_TOKEN_ERROR) {
        fprintf(stderr, "词法错误：%s\n", token->lexeme);
    }
    
    // 处理令牌...
    F_destroy_token(token);
}

// 检查是否有错误
if (lexer->has_errors(lexer)) {
    const char* error = lexer->get_last_error(lexer);
    fprintf(stderr, "词法分析错误：%s\n", error);
}

// 清理
lexer->destroy(lexer);
```

### 批量处理模式

```c
// 创建和初始化词法分析器
Stru_LexerInterface_t* lexer = F_create_lexer_interface();
lexer->initialize(lexer, source, strlen(source), "batch.cn");

// 批量令牌化
Stru_DynamicArray_t* tokens = lexer->tokenize_all(lexer);
if (tokens != NULL) {
    // 处理所有令牌
    process_all_tokens(tokens);
    
    // 清理
    for (size_t i = 0; i < F_dynamic_array_length(tokens); i++) {
        Stru_Token_t** token_ptr = (Stru_Token_t**)F_dynamic_array_get(tokens, i);
        if (token_ptr != NULL && *token_ptr != NULL) {
            F_destroy_token(*token_ptr);
        }
    }
    F_destroy_dynamic_array(tokens);
}

lexer->destroy(lexer);
```

### 错误处理模式

```c
Stru_LexerInterface_t* lexer = F_create_lexer_interface();

// 尝试初始化
if (!lexer->initialize(lexer, source, strlen(source), NULL)) {
    // 获取详细错误信息
    const char* error = lexer->get_last_error(lexer);
    size_t line, column;
    lexer->get_position(lexer, &line, &column);
    
    fprintf(stderr, "初始化失败（位置 %zu:%zu）：%s\n", 
            line, column, error);
    
    lexer->destroy(lexer);
    return 1;
}

// 处理令牌，检查错误
while (lexer->has_more_tokens(lexer)) {
    Stru_Token_t* token = lexer->next_token(lexer);
    
    if (token->type == Eum_TOKEN_ERROR) {
        // 记录错误但继续处理
        log_error(token->line, token->column, token->lexeme);
    } else if (token->type == Eum_TOKEN_EOF) {
        // 正常结束
        break;
    }
    
    F_destroy_token(token);
}

// 检查最终错误状态
if (lexer->has_errors(lexer)) {
    fprintf(stderr, "词法分析完成，但有错误：%s\n", 
            lexer->get_last_error(lexer));
}

lexer->destroy(lexer);
```

## 性能考虑

### 内存使用

1. **令牌内存**：每个令牌约64-128字节
2. **内部状态**：约256-512字节
3. **动态数组**：按需增长，每次增长50%

### 时间复杂性

1. **初始化**：O(1)
2. **next_token**：平均O(1)，最坏O(n)
3. **tokenize_all**：O(n)，其中n为源代码长度
4. **其他操作**：O(1)

### 优化建议

1. **批量处理**：对于大文件，使用`tokenize_all`比多次调用`next_token`更高效
2. **内存重用**：重用令牌对象减少分配开销
3. **缓冲区管理**：使用固定大小的缓冲区减少内存碎片

## 线程安全性

### 线程安全级别

词法分析器接口是**非线程安全**的。多个线程不能同时访问同一个`Stru_LexerInterface_t`实例。

### 线程安全使用模式

```c
// 每个线程创建自己的词法分析器实例
void* thread_function(void* arg) {
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    // ... 使用词法分析器 ...
    lexer->destroy(lexer);
    return NULL;
}

// 或者使用互斥锁保护共享实例
pthread_mutex_t lexer_mutex = PTHREAD_MUTEX_INITIALIZER;
Stru_LexerInterface_t* shared_lexer = NULL;

void safe_lexer_operation(const char* source) {
    pthread_mutex_lock(&lexer_mutex);
    
    if (shared_lexer == NULL) {
        shared_lexer = F_create_lexer_interface();
    }
    
    shared_lexer->initialize(shared_lexer, source, strlen(source), NULL);
    // ... 其他操作 ...
    
    pthread_mutex_unlock(&lexer_mutex);
}
```

## 版本兼容性

### API 稳定性

词法分析器接口遵循以下版本兼容性规则：

1. **主版本号变更**：不兼容的API修改
2. **次版本号变更**：向下兼容的功能性新增
3. **修订号变更**：向下兼容的问题修正

### 向后兼容性保证

1. 现有函数签名不会改变
2. 现有枚举值不会删除
3. 结构体布局保持稳定
4. 错误代码含义不变

### 废弃策略

1. 废弃的API会标记为`DEPRECATED`
2. 废弃的API至少保留两个主版本
3. 提供迁移指南和替代方案

## 扩展指南

### 添加新功能

要添加新功能到词法分析器接口：

1. 在`Stru_LexerInterface_t`结构体中添加新函数指针
2. 在实现文件中提供默认实现
3. 更新工厂函数设置函数指针
4. 编写相应的文档和测试

### 自定义实现

可以创建自定义的词法分析器实现：

```c
// 自定义词法分析器结构体
typedef struct {
    Stru_LexerInterface_t base_interface;
    // 自定义字段...
} CustomLexer_t;

// 自定义实现函数
static bool custom_initialize(Stru_LexerInterface_t* interface, 
                             const char* source, size_t length, 
                             const char* source_name) {
    // 自定义实现...
    return true;
}

// 创建自定义词法分析器
Stru_LexerInterface_t* create_custom_lexer(void) {
    CustomLexer_t* custom = malloc(sizeof(CustomLexer_t));
    
    // 设置接口函数
    custom->base_interface.initialize = custom_initialize;
    // ... 设置其他函数 ...
    
    return (Stru_LexerInterface_t*)custom;
}
```

## 相关文档

- [令牌模块 API 文档](../token/CN_token.md)
- [动态数组 API 文档](../../infrastructure/containers/array/CN_dynamic_array.md)
- [架构设计文档](../../../architecture/001-中文编程语言CN_Language开发规划.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本发布 |
| 1.0.1 | 2026-01-06 | 修复初始化错误处理 |
| 1.1.0 | 2026-01-06 | 添加批量令牌化功能 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
