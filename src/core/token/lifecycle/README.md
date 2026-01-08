# 令牌生命周期管理模块

## 概述

令牌生命周期管理模块负责令牌对象的创建、销毁、复制和比较等基本操作。本模块实现了令牌对象的完整生命周期管理，确保内存安全和资源正确释放。

## 模块职责

- 令牌创建和初始化
- 令牌销毁和资源释放
- 令牌深拷贝（克隆）
- 令牌比较和相等性判断
- 令牌哈希值计算

## 文件结构

```
src/core/token/lifecycle/
├── CN_token_lifecycle.h    # 头文件 - 声明生命周期管理函数
├── CN_token_lifecycle.c    # 源文件 - 实现生命周期管理函数
└── README.md               # 本文件 - 模块文档
```

## API 参考

### 令牌创建

#### F_create_token

创建新令牌对象。

```c
Stru_Token_t* F_create_token(Eum_TokenType type, 
                            const char* lexeme, 
                            size_t line, 
                            size_t column);
```

**参数：**
- `type`：令牌类型（Eum_TokenType枚举值）
- `lexeme`：词素字符串（源代码中的原始文本）
- `line`：行号（从1开始）
- `column`：列号（从1开始）

**返回值：**
- 成功：新创建的令牌指针
- 失败：NULL（内存不足或参数无效）

**内存管理：**
- 分配令牌结构体内存
- 复制词素字符串
- 初始化所有字段为默认值

#### F_create_token_with_lexeme_copy

创建令牌并复制词素字符串。

```c
Stru_Token_t* F_create_token_with_lexeme_copy(Eum_TokenType type, 
                                             const char* lexeme, 
                                             size_t lexeme_len,
                                             size_t line, 
                                             size_t column);
```

**特点：**
- 允许指定词素字符串长度
- 更高效的内存使用
- 支持空字符处理

### 令牌销毁

#### F_destroy_token

销毁令牌对象，释放所有相关资源。

```c
void F_destroy_token(Stru_Token_t* token);
```

**资源释放：**
1. 释放词素字符串内存
2. 释放扩展数据内存（如果存在）
3. 释放令牌结构体本身

**安全特性：**
- 检查NULL指针
- 幂等操作（多次调用安全）
- 线程安全（单令牌操作）

### 令牌复制

#### F_clone_token

创建令牌的深拷贝。

```c
Stru_Token_t* F_clone_token(const Stru_Token_t* token);
```

**复制内容：**
- 令牌类型和位置信息
- 词素字符串（完整复制）
- 字面量值（根据类型）
- 扩展数据（完整复制）

**使用场景：**
- 令牌缓存
- 错误恢复
- 多线程处理

### 令牌比较

#### F_compare_tokens

比较两个令牌是否相等。

```c
bool F_compare_tokens(const Stru_Token_t* token1, const Stru_Token_t* token2);
```

**比较规则：**
1. 类型必须相同
2. 词素必须相同（字符串比较）
3. 字面量值必须相同（根据类型）
4. 位置信息不参与比较

**返回值：**
- `true`：令牌相等
- `false`：令牌不相等或任一令牌为NULL

#### F_compare_tokens_with_position

比较两个令牌是否相等（包含位置信息）。

```c
bool F_compare_tokens_with_position(const Stru_Token_t* token1, 
                                   const Stru_Token_t* token2);
```

**特点：**
- 包含行号和列号比较
- 用于调试和错误报告
- 更严格的相等性检查

### 令牌哈希

#### F_token_hash

计算令牌的哈希值。

```c
size_t F_token_hash(const Stru_Token_t* token);
```

**哈希算法：**
- 基于令牌类型、词素和字面量值
- 产生一致的哈希值
- 适用于哈希表存储

**用途：**
- 令牌缓存索引
- 快速相等性检查
- 集合操作

## 使用示例

### 基本使用

```c
#include "src/core/token/lifecycle/CN_token_lifecycle.h"
#include <stdio.h>

int main(void) {
    // 创建令牌
    Stru_Token_t* token = F_create_token(
        Eum_TOKEN_KEYWORD_VAR,
        "变量",
        1, 1
    );
    
    if (token == NULL) {
        fprintf(stderr, "创建令牌失败\n");
        return 1;
    }
    
    // 克隆令牌
    Stru_Token_t* cloned = F_clone_token(token);
    if (cloned == NULL) {
        fprintf(stderr, "克隆令牌失败\n");
        F_destroy_token(token);
        return 1;
    }
    
    // 比较令牌
    if (F_compare_tokens(token, cloned)) {
        printf("令牌相等\n");
    }
    
    // 计算哈希值
    size_t hash = F_token_hash(token);
    printf("令牌哈希值: %zu\n", hash);
    
    // 清理
    F_destroy_token(token);
    F_destroy_token(cloned);
    
    return 0;
}
```

### 错误处理

```c
#include "src/core/token/lifecycle/CN_token_lifecycle.h"
#include <stdio.h>

Stru_Token_t* safe_create_token(Eum_TokenType type, 
                               const char* lexeme, 
                               size_t line, 
                               size_t column) {
    // 参数验证
    if (lexeme == NULL) {
        fprintf(stderr, "错误：词素不能为NULL\n");
        return NULL;
    }
    
    if (line == 0 || column == 0) {
        fprintf(stderr, "错误：行号和列号必须从1开始\n");
        return NULL;
    }
    
    // 创建令牌
    Stru_Token_t* token = F_create_token(type, lexeme, line, column);
    if (token == NULL) {
        fprintf(stderr, "错误：内存不足，无法创建令牌\n");
    }
    
    return token;
}

void safe_destroy_token(Stru_Token_t** token_ptr) {
    if (token_ptr == NULL || *token_ptr == NULL) {
        return;
    }
    
    F_destroy_token(*token_ptr);
    *token_ptr = NULL;  // 防止悬空指针
}
```

## 内存管理策略

### 分配策略

1. **令牌结构体**：使用`cn_malloc`分配
2. **词素字符串**：使用`cn_strdup`复制
3. **扩展数据**：按需分配，使用相同分配器

### 释放策略

1. **顺序释放**：先释放内部资源，再释放结构体
2. **NULL检查**：所有释放操作都检查NULL
3. **幂等性**：多次释放同一令牌安全

### 内存泄漏检测

```c
// 调试版本启用内存跟踪
#ifdef CN_DEBUG
#define F_create_token(type, lexeme, line, column) \
    F_create_token_debug(type, lexeme, line, column, __FILE__, __LINE__)

Stru_Token_t* F_create_token_debug(Eum_TokenType type, 
                                  const char* lexeme, 
                                  size_t line, 
                                  size_t column,
                                  const char* file,
                                  int line_num);
#endif
```

## 性能考虑

### 时间复杂度

| 操作 | 平均时间复杂度 | 最坏情况 |
|------|----------------|----------|
| 创建令牌 | O(n) | O(n) |
| 销毁令牌 | O(1) | O(1) |
| 克隆令牌 | O(n) | O(n) |
| 比较令牌 | O(n) | O(n) |
| 计算哈希 | O(n) | O(n) |

*注：n为词素字符串长度*

### 内存使用

| 组件 | 大小 | 说明 |
|------|------|------|
| 令牌结构体 | 64字节 | 固定大小 |
| 词素字符串 | 变长 | 实际字符串长度+1 |
| 扩展数据 | 变长 | 按需分配 |

### 优化建议

1. **重用令牌**：频繁使用的令牌类型使用对象池
2. **字符串内联**：短词素使用内联存储
3. **延迟分配**：扩展数据按需分配

## 线程安全性

### 安全级别

- **创建/销毁**：非线程安全（需要外部同步）
- **比较/哈希**：线程安全（只读操作）
- **克隆**：非线程安全（涉及内存分配）

### 线程安全模式

```c
// 每个线程有自己的令牌池
__thread Stru_TokenPool_t* thread_token_pool = NULL;

Stru_Token_t* thread_safe_create_token(Eum_TokenType type,
                                      const char* lexeme,
                                      size_t line,
                                      size_t column) {
    // 从线程局部池分配
    if (thread_token_pool == NULL) {
        thread_token_pool = F_create_token_pool();
    }
    
    return F_token_pool_allocate(thread_token_pool, type, lexeme, line, column);
}
```

## 测试覆盖

### 单元测试

测试文件：`tests/core/token/lifecycle/test_token_lifecycle.c`

**测试用例：**
1. 正常创建和销毁
2. 边界条件测试（NULL参数、零长度字符串）
3. 克隆功能测试
4. 比较功能测试
5. 哈希一致性测试
6. 内存泄漏测试

### 集成测试

与以下模块集成测试：
1. 字面量值模块
2. 类型查询模块
3. 关键字管理模块

## 依赖关系

### 内部依赖

- `CN_token_types.h`：令牌类型定义
- `CN_token_interface.h`：抽象接口定义

### 外部依赖

- `CN_memory.h`：内存管理接口
- `CN_utils_string.h`：字符串工具函数

## 设计原则

### 单一职责原则

本模块只负责令牌生命周期管理，不涉及：
- 字面量值操作
- 类型查询和分类
- 关键字信息管理

### 开闭原则

通过抽象接口支持扩展：
- 可替换的内存分配器
- 可定制的复制策略
- 可扩展的比较算法

### 资源管理

遵循"谁创建谁销毁"原则：
- 创建函数返回的令牌必须由调用者销毁
- 克隆函数返回的令牌是独立对象
- 所有资源都有明确的释放路径

## 版本历史

| 版本 | 日期 | 变更描述 |
|------|------|----------|
| 1.0.0 | 2026-01-08 | 初始版本，基本生命周期管理 |
| 1.0.1 | 2026-01-08 | 添加调试版本内存跟踪 |
| 1.1.0 | 2026-01-08 | 添加线程安全支持 |

## 相关文档

- [令牌模块主文档](../CN_token.md)
- [令牌模块化架构文档](../../../docs/api/core/token/CN_token_module.md)
- [内存管理接口文档](../../../docs/api/infrastructure/memory/CN_memory.md)
