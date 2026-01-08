# 令牌接口实现模块

## 概述

令牌接口实现模块是令牌系统的核心集成模块，负责实现抽象接口并将各个子模块连接起来。本模块提供了完整的令牌接口实现，支持依赖注入和模块替换，是令牌系统架构的关键部分。

## 模块职责

- 实现令牌抽象接口（Stru_TokenInterface_t）
- 集成各个子模块的功能
- 提供依赖注入支持
- 实现模块替换机制
- 管理模块生命周期

## 文件结构

```
src/core/token/interface/
├── CN_token_interface_impl.c    # 源文件 - 接口实现
└── README.md                    # 本文件 - 模块文档
```

## 接口定义

### Stru_TokenInterface_t 结构体

令牌抽象接口定义，包含所有令牌操作的方法指针。

```c
typedef struct Stru_TokenInterface_t {
    // ============================================
    // 生命周期管理
    // ============================================
    
    /// 创建新令牌
    Stru_Token_t* (*create_token)(Eum_TokenType type, 
                                 const char* lexeme, 
                                 size_t line, 
                                 size_t column);
    
    /// 销毁令牌
    void (*destroy_token)(Stru_Token_t* token);
    
    /// 克隆令牌
    Stru_Token_t* (*clone_token)(const Stru_Token_t* token);
    
    /// 比较令牌
    bool (*compare_tokens)(const Stru_Token_t* token1, 
                          const Stru_Token_t* token2);
    
    /// 计算令牌哈希值
    size_t (*token_hash)(const Stru_Token_t* token);
    
    // ============================================
    // 字面量值操作
    // ============================================
    
    /// 设置整数值
    void (*set_int_value)(Stru_Token_t* token, long value);
    
    /// 设置浮点数值
    void (*set_float_value)(Stru_Token_t* token, double value);
    
    /// 设置字符串值
    void (*set_string_value)(Stru_Token_t* token, const char* value);
    
    /// 设置字符值
    void (*set_char_value)(Stru_Token_t* token, char value);
    
    /// 设置布尔值
    void (*set_bool_value)(Stru_Token_t* token, bool value);
    
    /// 获取整数值
    long (*get_int_value)(const Stru_Token_t* token);
    
    /// 获取浮点数值
    double (*get_float_value)(const Stru_Token_t* token);
    
    /// 获取字符串值
    const char* (*get_string_value)(const Stru_Token_t* token);
    
    /// 获取字符值
    char (*get_char_value)(const Stru_Token_t* token);
    
    /// 获取布尔值
    bool (*get_bool_value)(const Stru_Token_t* token);
    
    // ============================================
    // 类型查询和分类
    // ============================================
    
    /// 检查是否为关键字
    bool (*is_keyword_token)(Eum_TokenType type);
    
    /// 检查是否为字面量
    bool (*is_literal_token)(Eum_TokenType type);
    
    /// 检查是否为运算符
    bool (*is_operator_token)(Eum_TokenType type);
    
    /// 检查是否为分隔符
    bool (*is_delimiter_token)(Eum_TokenType type);
    
    /// 检查是否为标识符
    bool (*is_identifier_token)(Eum_TokenType type);
    
    /// 获取令牌类别
    const char* (*get_token_category)(Eum_TokenType type);
    
    // ============================================
    // 关键字信息管理
    // ============================================
    
    /// 通过中文关键字获取信息
    const Stru_KeywordInfo_t* (*get_keyword_info)(const char* chinese_keyword);
    
    /// 通过英文名获取信息
    const Stru_KeywordInfo_t* (*get_keyword_by_english)(const char* english_name);
    
    /// 通过类型获取信息
    const Stru_KeywordInfo_t* (*get_keyword_by_type)(Eum_TokenType type);
    
    /// 获取关键字总数
    size_t (*get_keyword_count)(void);
    
    // ============================================
    // 工具函数
    // ============================================
    
    /// 转换为字符串表示
    char* (*token_to_string)(const Stru_Token_t* token);
    
    /// 获取类型名称
    const char* (*get_token_type_name)(Eum_TokenType type);
    
    /// 打印令牌信息
    void (*print_token)(const Stru_Token_t* token);
    
    // ============================================
    // 模块管理
    // ============================================
    
    /// 初始化接口
    bool (*initialize)(void);
    
    /// 清理接口
    void (*cleanup)(void);
    
    /// 获取接口版本
    const char* (*get_version)(void);
    
} Stru_TokenInterface_t;
```

## API 参考

### 接口获取和管理

#### F_get_token_interface

获取令牌接口实例。

```c
const Stru_TokenInterface_t* F_get_token_interface(void);
```

**返回值：**
- `const Stru_TokenInterface_t*`：令牌接口指针
- 永远不会返回NULL（如果初始化失败，返回最小功能接口）

**特点：**
- 线程安全
- 惰性初始化
- 返回常量指针（只读）

#### F_initialize_token_interface

初始化令牌接口。

```c
bool F_initialize_token_interface(void);
```

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

**初始化步骤：**
1. 检查是否已初始化
2. 初始化各个子模块
3. 设置接口方法指针
4. 验证接口完整性

#### F_cleanup_token_interface

清理令牌接口。

```c
void F_cleanup_token_interface(void);
```

**清理步骤：**
1. 清理各个子模块
2. 重置接口状态
3. 释放相关资源

### 接口使用

#### 直接使用接口

```c
// 获取接口
const Stru_TokenInterface_t* token_if = F_get_token_interface();

// 使用接口方法
Stru_Token_t* token = token_if->create_token(
    Eum_TOKEN_KEYWORD_VAR,
    "变量",
    1, 1
);

// 操作令牌
token_if->set_int_value(token, 42);
token_if->print_token(token);

// 清理
token_if->destroy_token(token);
```

#### 宏包装器

为了方便使用，提供了宏包装器：

```c
// 使用宏简化调用
CN_TOKEN_CREATE(Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
CN_TOKEN_DESTROY(token);
CN_TOKEN_PRINT(token);
CN_TOKEN_GET_TYPE_NAME(token->type);
```

### 依赖注入

#### F_set_token_interface

设置自定义令牌接口（依赖注入）。

```c
bool F_set_token_interface(const Stru_TokenInterface_t* custom_interface);
```

**参数：**
- `custom_interface`：自定义接口指针

**返回值：**
- `true`：设置成功
- `false`：设置失败（接口无效或已初始化）

**使用场景：**
- 单元测试（模拟接口）
- 性能优化（替换实现）
- 功能扩展（添加新功能）

#### F_reset_token_interface

重置为默认令牌接口。

```c
void F_reset_token_interface(void);
```

**作用：**
- 恢复默认接口实现
- 清理自定义接口
- 重新初始化默认模块

## 使用示例

### 基本接口使用

```c
#include "src/core/token/CN_token_interface.h"
#include <stdio.h>

int main(void) {
    // 获取令牌接口
    const Stru_TokenInterface_t* token_if = F_get_token_interface();
    
    if (token_if == NULL) {
        fprintf(stderr, "无法获取令牌接口\n");
        return 1;
    }
    
    // 检查接口版本
    const char* version = token_if->get_version();
    printf("令牌接口版本: %s\n", version);
    
    // 创建令牌
    Stru_Token_t* var_token = token_if->create_token(
        Eum_TOKEN_KEYWORD_VAR,
        "变量",
        1, 1
    );
    
    Stru_Token_t* int_token = token_if->create_token(
        Eum_TOKEN_LITERAL_INTEGER,
        "42",
        1, 8
    );
    
    // 设置值
    token_if->set_int_value(int_token, 42);
    
    // 类型查询
    bool is_keyword = token_if->is_keyword_token(var_token->type);
    bool is_literal = token_if->is_literal_token(int_token->type);
    
    printf("var_token是关键字: %s\n", is_keyword ? "是" : "否");
    printf("int_token是字面量: %s\n", is_literal ? "是" : "否");
    
    // 关键字信息
    const Stru_KeywordInfo_t* keyword_info = token_if->get_keyword_info("变量");
    if (keyword_info != NULL) {
        printf("关键字信息: %s -> %s\n", 
               keyword_info->chinese, keyword_info->english);
    }
    
    // 打印令牌
    printf("\n令牌信息:\n");
    token_if->print_token(var_token);
    token_if->print_token(int_token);
    
    // 转换为字符串
    char* str1 = token_if->token_to_string(var_token);
    char* str2 = token_if->token_to_string(int_token);
    
    printf("\n字符串表示:\n");
    printf("  %s\n", str1);
    printf("  %s\n", str2);
    
    // 清理
    free(str1);
    free(str2);
    token_if->destroy_token(var_token);
    token_if->destroy_token(int_token);
    
    return 0;
}
```

### 依赖注入示例

```c
#include "src/core/token/CN_token_interface.h"
#include <stdio.h>
#include <string.h>

// 自定义令牌接口实现
static Stru_Token_t* custom_create_token(Eum_TokenType type, 
                                        const char* lexeme, 
                                        size_t line, 
                                        size_t column) {
    printf("[自定义] 创建令牌: %s\n", lexeme);
    // 调用默认实现或实现自定义逻辑
    return F_create_token(type, lexeme, line, column);
}

static void custom_destroy_token(Stru_Token_t* token) {
    printf("[自定义] 销毁令牌: %s\n", token->lexeme);
    F_destroy_token(token);
}

static void custom_print_token(const Stru_Token_t* token) {
    printf("[自定义] 打印令牌:\n");
    printf("  类型: %s\n", F_get_token_type_name(token->type));
    printf("  词素: %s\n", token->lexeme);
    printf("  位置: %zu:%zu\n", token->line, token->column);
}

// 自定义接口实例
static Stru_TokenInterface_t g_custom_interface = {
    .create_token = custom_create_token,
    .destroy_token = custom_destroy_token,
    .clone_token = F_clone_token,
    .compare_tokens = F_compare_tokens,
    .token_hash = F_token_hash,
    .set_int_value = F_token_set_int_value,
    .set_float_value = F_token_set_float_value,
    .set_string_value = F_token_set_string_value,
    .set_char_value = F_token_set_char_value,
    .set_bool_value = F_token_set_bool_value,
    .get_int_value = F_token_get_int_value,
    .get_float_value = F_token_get_float_value,
    .get_string_value = F_token_get_string_value,
    .get_char_value = F_token_get_char_value,
    .get_bool_value = F_token_get_bool_value,
    .is_keyword_token = F_is_keyword_token,
    .is_literal_token = F_is_literal_token,
    .is_operator_token = F_is_operator_token,
    .is_delimiter_token = F_is_delimiter_token,
    .is_identifier_token = F_is_identifier_token,
    .get_token_category = F_get_token_category,
    .get_keyword_info = F_get_keyword_info,
    .get_keyword_by_english = F_get_keyword_by_english,
    .get_keyword_by_type = F_get_keyword_by_type,
    .get_keyword_count = F_get_keyword_count,
    .token_to_string = F_token_to_string,
    .get_token_type_name = F_get_token_type_name,
    .print_token = custom_print_token,
    .initialize = NULL, // 使用默认初始化
    .cleanup = NULL,    // 使用默认清理
    .get_version = NULL // 使用默认版本
};

int main(void) {
    // 设置自定义接口
    bool success = F_set_token_interface(&g_custom_interface);
    if (!success) {
        fprintf(stderr, "设置自定义接口失败\n");
        return 1;
    }
    
    printf("已设置自定义接口\n");
    
    // 获取接口（现在返回自定义接口）
    const Stru_TokenInterface_t* token_if = F_get_token_interface();
    
    // 使用自定义接口
    Stru_Token_t* token = token_if->create_token(
        Eum_TOKEN_LITERAL_INTEGER,
        "100",
        1, 1
    );
    
    token_if->set_int_value(token, 100);
    token_if->print_token(token);
    
    // 转换为字符串（使用默认实现）
    char* str = token_if->token_to_string(token);
    printf("字符串表示: %s\n", str);
    free(str);
    
    // 清理
    token_if->destroy_token(token);
    
    // 重置为默认接口
    F_reset_token_interface();
    printf("\n已重置为默认接口\n");
    
    // 验证重置
    token_if = F_get_token_interface();
    token = token_if->create_token(Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    token_if->print_token(token); // 现在使用默认打印
    token_if->destroy_token(token);
    
    return 0;
}
```

### 模块初始化和清理

```c
#include "src/core/token/CN_token_interface.h"
#include <stdio.h>

int main(void) {
    // 手动初始化接口
    printf("初始化令牌接口...\n");
    bool initialized = F_initialize_token_interface();
    
    if (!initialized) {
        fprintf(stderr, "令牌接口初始化失败\n");
        return 1;
    }
    
    printf("令牌接口初始化成功\n");
    
    // 获取接口
    const Stru_TokenInterface_t* token_if = F_get_token_interface();
    
    // 使用接口
    Stru_Token_t* token = token_if->create_token(
        Eum_TOKEN_KEYWORD_FUNCTION,
        "函数",
        1, 1
    );
    
    // 获取关键字信息
    size_t keyword_count = token_if->get_keyword_count();
    printf("关键字总数: %zu\n", keyword_count);
    
    // 遍历所有关键字
    for (size_t i = 0; i < keyword_count; i++) {
        const Stru_KeywordInfo_t* info = F_get_keyword_at_index(i);
        if (info != NULL) {
            printf("%3zu. %-6s (%s)\n", 
                   i + 1, info->chinese, info->category);
        }
    }
    
    // 类型查询示例
    Eum_TokenType test_types[] = {
        Eum_TOKEN_KEYWORD_VAR,
        Eum_TOKEN_LITERAL_INTEGER,
        Eum_TOKEN_OPERATOR_PLUS,
        Eum_TOKEN_DELIMITER_COMMA
    };
    
    printf("\n类型查询:\n");
    for (int i = 0; i < 4; i++) {
        Eum_TokenType type = test_types[i];
        const char* category = token_if->get_token_category(type);
        const char* type_name = token_if->get_token_type_name(type);
        
        printf("  %s: %s\n", type_name, category);
    }
    
    // 清理
    token_if->destroy_token(token);
    
    // 手动清理接口
    printf("\n清理令牌接口...\n");
    F_cleanup_token_interface();
    printf("令牌接口清理完成\n");
    
    return 0;
}
```

### 线程安全使用

```c
#include "src/core/token/CN_token_interface.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define THREAD_COUNT 5
#define TOKENS_PER_THREAD 10

// 线程参数
typedef struct {
    int thread_id;
    const Stru_TokenInterface_t* token_if;
} ThreadParam_t;

// 线程函数
void* thread_function(void* arg) {
    ThreadParam_t* param = (ThreadParam_t*)arg;
    const Stru_TokenInterface_t* token_if = param->token_if;
    
    printf("线程 %d 开始\n", param->thread_id);
    
    // 每个线程创建自己的令牌
    Stru_Token_t* tokens[TOKENS_PER_THREAD];
    
    for (int i = 0; i < TOKENS_PER_THREAD; i++) {
        // 创建不同类型的令牌
        Eum_TokenType type;
        const char* lexeme;
        
        if (i % 3 == 0) {
            type = Eum_TOKEN_KEYWORD_VAR;
            lexeme = "变量";
        } else if (i % 3 == 1) {
            type = Eum_TOKEN_LITERAL_INTEGER;
            lexeme = "100";
        } else {
            type = Eum_TOKEN_OPERATOR_PLUS;
            lexeme = "+";
        }
        
        tokens[i] = token_if->create_token(
            type,
            lexeme,
            param->thread_id + 1,
            i + 1
        );
        
        if (type == Eum_TOKEN_LITERAL_INTEGER) {
            token_if->set_int_value(tokens[i], 100 + i);
        }
        
        // 线程安全的查询操作
        const char* category = token_if->get_token_category(type);
