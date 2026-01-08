# 令牌类型查询和分类模块

## 概述

令牌类型查询和分类模块负责对令牌类型进行分类和查询，提供快速判断令牌类型所属类别的能力。本模块实现了对70个中文关键字的分类查询，支持关键字、字面量、运算符、分隔符和标识符等类型的快速识别。

## 模块职责

- 令牌类型分类查询（关键字、字面量、运算符、分隔符、标识符）
- 关键字类型详细分类（控制结构、类型声明、运算符等9个类别）
- 提供快速类型判断函数
- 支持类型转换和验证

## 文件结构

```
src/core/token/query/
├── CN_token_query.h    # 头文件 - 声明类型查询函数
├── CN_token_query.c    # 源文件 - 实现类型查询函数
└── README.md           # 本文件 - 模块文档
```

## API 参考

### 基本类型查询

#### F_is_keyword_token

检查令牌类型是否为关键字。

```c
bool F_is_keyword_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是关键字类型
- `false`：不是关键字类型

**支持的关键字类型：**
- 所有以`Eum_TOKEN_KEYWORD_`开头的类型（70个中文关键字）

#### F_is_literal_token

检查令牌类型是否为字面量。

```c
bool F_is_literal_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是字面量类型
- `false`：不是字面量类型

**支持的字面量类型：**
- `Eum_TOKEN_LITERAL_INTEGER`
- `Eum_TOKEN_LITERAL_FLOAT`
- `Eum_TOKEN_LITERAL_STRING`
- `Eum_TOKEN_LITERAL_CHAR`
- `Eum_TOKEN_LITERAL_BOOLEAN`

#### F_is_operator_token

检查令牌类型是否为运算符。

```c
bool F_is_operator_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是运算符类型
- `false`：不是运算符类型

**支持的运算符类型：**
- 所有以`Eum_TOKEN_OPERATOR_`开头的类型
- 运算符关键字（加、减、乘、除、模）

#### F_is_delimiter_token

检查令牌类型是否为分隔符。

```c
bool F_is_delimiter_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是分隔符类型
- `false`：不是分隔符类型

**支持的分隔符类型：**
- 所有以`Eum_TOKEN_DELIMITER_`开头的类型

#### F_is_identifier_token

检查令牌类型是否为标识符。

```c
bool F_is_identifier_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是标识符类型
- `false`：不是标识符类型

**支持的标识符类型：**
- `Eum_TOKEN_IDENTIFIER`

### 特殊类型查询

#### F_is_control_structure_keyword

检查令牌类型是否为控制结构关键字。

```c
bool F_is_control_structure_keyword(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是控制结构关键字
- `false`：不是控制结构关键字

**支持的控制结构关键字：**
- `Eum_TOKEN_KEYWORD_IF`（如果）
- `Eum_TOKEN_KEYWORD_ELSE`（否则）
- `Eum_TOKEN_KEYWORD_WHILE`（当）
- `Eum_TOKEN_KEYWORD_FOR`（循环）
- `Eum_TOKEN_KEYWORD_BREAK`（中断）
- `Eum_TOKEN_KEYWORD_CONTINUE`（继续）
- `Eum_TOKEN_KEYWORD_SWITCH`（选择）
- `Eum_TOKEN_KEYWORD_CASE`（情况）
- `Eum_TOKEN_KEYWORD_DEFAULT`（默认）

#### F_is_type_declaration_keyword

检查令牌类型是否为类型声明关键字。

```c
bool F_is_type_declaration_keyword(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是类型声明关键字
- `false`：不是类型声明关键字

**支持的类型声明关键字：**
- `Eum_TOKEN_KEYWORD_VAR`（变量）
- `Eum_TOKEN_KEYWORD_CONST`（常量）
- `Eum_TOKEN_KEYWORD_TYPE`（类型）
- `Eum_TOKEN_KEYWORD_CLASS`（类）
- `Eum_TOKEN_KEYWORD_OBJECT`（对象）

#### F_is_function_keyword

检查令牌类型是否为函数相关关键字。

```c
bool F_is_function_keyword(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是函数相关关键字
- `false`：不是函数相关关键字

**支持的函数关键字：**
- `Eum_TOKEN_KEYWORD_FUNCTION`（函数）
- `Eum_TOKEN_KEYWORD_RETURN`（返回）
- `Eum_TOKEN_KEYWORD_MAIN`（主程序）
- `Eum_TOKEN_KEYWORD_VOID`（无）

#### F_is_logical_operator_keyword

检查令牌类型是否为逻辑运算符关键字。

```c
bool F_is_logical_operator_keyword(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是逻辑运算符关键字
- `false`：不是逻辑运算符关键字

**支持的逻辑运算符关键字：**
- `Eum_TOKEN_KEYWORD_AND`（与）
- `Eum_TOKEN_KEYWORD_OR`（或）
- `Eum_TOKEN_KEYWORD_NOT`（非）

#### F_is_literal_keyword

检查令牌类型是否为字面量关键字。

```c
bool F_is_literal_keyword(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是字面量关键字
- `false`：不是字面量关键字

**支持的字面量关键字：**
- `Eum_TOKEN_KEYWORD_TRUE`（真）
- `Eum_TOKEN_KEYWORD_FALSE`（假）
- `Eum_TOKEN_KEYWORD_NULL`（空）

### 高级查询函数

#### F_get_token_category

获取令牌类型的类别。

```c
const char* F_get_token_category(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- 类别名称字符串
- 对于未知类型，返回"UNKNOWN"

**支持的类别：**
- "KEYWORD"：关键字
- "LITERAL"：字面量
- "OPERATOR"：运算符
- "DELIMITER"：分隔符
- "IDENTIFIER"：标识符
- "SPECIAL"：特殊类型（EOF、ERROR）

#### F_get_keyword_category

获取关键字的详细类别。

```c
const char* F_get_keyword_category(Eum_TokenType type);
```

**参数：**
- `type`：关键字令牌类型

**返回值：**
- 关键字类别字符串
- 对于非关键字类型，返回NULL

**支持的关键字类别：**
- "控制结构"：控制流程关键字
- "类型声明"：变量和类型声明关键字
- "函数相关"：函数定义和调用关键字
- "逻辑运算符"：逻辑运算关键字
- "字面量"：字面量值关键字
- "模块系统"：模块导入导出关键字
- "运算符"：算术运算符关键字
- "类型关键字"：数据类型关键字
- "其他"：其他关键字

#### F_is_token_of_category

检查令牌是否属于指定类别。

```c
bool F_is_token_of_category(Eum_TokenType type, const char* category);
```

**参数：**
- `type`：令牌类型
- `category`：类别名称

**返回值：**
- `true`：令牌属于指定类别
- `false`：令牌不属于指定类别或参数无效

### 类型转换和验证

#### F_validate_token_type

验证令牌类型是否有效。

```c
bool F_validate_token_type(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：有效的令牌类型
- `false`：无效的令牌类型

**验证规则：**
- 类型值在有效范围内
- 类型值对应有效的枚举值

#### F_convert_to_token_type

将字符串转换为令牌类型。

```c
Eum_TokenType F_convert_to_token_type(const char* type_name);
```

**参数：**
- `type_name`：类型名称字符串

**返回值：**
- 对应的令牌类型
- 如果未找到，返回`Eum_TOKEN_ERROR`

**支持的格式：**
- 枚举名称（如"Eum_TOKEN_KEYWORD_VAR"）
- 简化名称（如"KEYWORD_VAR"）
- 中文关键字（如"变量"）

## 使用示例

### 基本类型查询

```c
#include "src/core/token/query/CN_token_query.h"
#include <stdio.h>

int main(void) {
    // 测试不同类型
    Eum_TokenType types[] = {
        Eum_TOKEN_KEYWORD_VAR,
        Eum_TOKEN_LITERAL_INTEGER,
        Eum_TOKEN_OPERATOR_PLUS,
        Eum_TOKEN_DELIMITER_COMMA,
        Eum_TOKEN_IDENTIFIER
    };
    
    const char* type_names[] = {
        "KEYWORD_VAR",
        "LITERAL_INTEGER", 
        "OPERATOR_PLUS",
        "DELIMITER_COMMA",
        "IDENTIFIER"
    };
    
    for (int i = 0; i < 5; i++) {
        Eum_TokenType type = types[i];
        
        printf("类型 %s:\n", type_names[i]);
        printf("  是关键字: %s\n", F_is_keyword_token(type) ? "是" : "否");
        printf("  是字面量: %s\n", F_is_literal_token(type) ? "是" : "否");
        printf("  是运算符: %s\n", F_is_operator_token(type) ? "是" : "否");
        printf("  是分隔符: %s\n", F_is_delimiter_token(type) ? "是" : "否");
        printf("  是标识符: %s\n", F_is_identifier_token(type) ? "是" : "否");
        
        const char* category = F_get_token_category(type);
        printf("  类别: %s\n", category);
        
        printf("\n");
    }
    
    return 0;
}
```

### 关键字详细分类

```c
#include "src/core/token/query/CN_token_query.h"
#include <stdio.h>

void analyze_keyword(Eum_TokenType type) {
    if (!F_is_keyword_token(type)) {
        printf("不是关键字类型\n");
        return;
    }
    
    printf("关键字分析:\n");
    
    // 检查具体类别
    if (F_is_control_structure_keyword(type)) {
        printf("  类别: 控制结构\n");
    }
    
    if (F_is_type_declaration_keyword(type)) {
        printf("  类别: 类型声明\n");
    }
    
    if (F_is_function_keyword(type)) {
        printf("  类别: 函数相关\n");
    }
    
    if (F_is_logical_operator_keyword(type)) {
        printf("  类别: 逻辑运算符\n");
    }
    
    if (F_is_literal_keyword(type)) {
        printf("  类别: 字面量\n");
    }
    
    // 获取详细类别
    const char* detailed_category = F_get_keyword_category(type);
    if (detailed_category != NULL) {
        printf("  详细类别: %s\n", detailed_category);
    }
}

int main(void) {
    // 分析不同类型的关键字
    printf("分析'变量'关键字:\n");
    analyze_keyword(Eum_TOKEN_KEYWORD_VAR);
    printf("\n");
    
    printf("分析'如果'关键字:\n");
    analyze_keyword(Eum_TOKEN_KEYWORD_IF);
    printf("\n");
    
    printf("分析'函数'关键字:\n");
    analyze_keyword(Eum_TOKEN_KEYWORD_FUNCTION);
    printf("\n");
    
    printf("分析'与'关键字:\n");
    analyze_keyword(Eum_TOKEN_KEYWORD_AND);
    printf("\n");
    
    printf("分析'真'关键字:\n");
    analyze_keyword(Eum_TOKEN_KEYWORD_TRUE);
    
    return 0;
}
```

### 类型转换和验证

```c
#include "src/core/token/query/CN_token_query.h"
#include <stdio.h>

int main(void) {
    // 类型验证
    Eum_TokenType valid_type = Eum_TOKEN_KEYWORD_VAR;
    Eum_TokenType invalid_type = (Eum_TokenType)999; // 无效值
    
    printf("验证类型:\n");
    printf("  KEYWORD_VAR: %s\n", 
           F_validate_token_type(valid_type) ? "有效" : "无效");
    printf("  值999: %s\n", 
           F_validate_token_type(invalid_type) ? "有效" : "无效");
    
    // 类型转换
    printf("\n类型转换:\n");
    
    const char* test_cases[] = {
        "Eum_TOKEN_KEYWORD_VAR",
        "KEYWORD_VAR",
        "变量",
        "INVALID_TYPE"
    };
    
    for (int i = 0; i < 4; i++) {
        Eum_TokenType converted = F_convert_to_token_type(test_cases[i]);
        const char* result = (converted != Eum_TOKEN_ERROR) ? "成功" : "失败";
        printf("  转换'%s': %s\n", test_cases[i], result);
    }
    
    // 类别检查
    printf("\n类别检查:\n");
    
    Eum_TokenType test_types[] = {
        Eum_TOKEN_KEYWORD_VAR,
        Eum_TOKEN_LITERAL_INTEGER,
        Eum_TOKEN_OPERATOR_PLUS
    };
    
    const char* test_categories[] = {
        "KEYWORD",
        "LITERAL", 
        "OPERATOR"
    };
    
    for (int i = 0; i < 3; i++) {
        bool is_in_category = F_is_token_of_category(test_types[i], 
                                                    test_categories[i]);
        printf("  类型%d属于类别'%s': %s\n", 
               test_types[i], test_categories[i],
               is_in_category ? "是" : "否");
    }
    
    return 0;
}
```

## 性能优化

### 查询表设计

使用静态查询表实现O(1)时间复杂度的类型查询：

```c
// 类型分类查询表
static const struct {
    Eum_TokenType type;
    uint8_t categories; // 位掩码表示多个类别
} g_token_category_table[] = {
    {Eum_TOKEN_KEYWORD_VAR, CATEGORY_KEYWORD | CATEGORY_TYPE_DECLARATION},
    {Eum_TOKEN_LITERAL_INTEGER, CATEGORY_LITERAL},
    // ... 其他类型
};
```

### 位掩码技术

使用位掩码实现快速的多类别检查：

```c
// 类别位掩码定义
#define CATEGORY_KEYWORD          (1 << 0)
#define CATEGORY_LITERAL          (1 << 1)
#define CATEGORY_OPERATOR         (1 << 2)
#define CATEGORY_DELIMITER        (1 << 3)
#define CATEGORY_IDENTIFIER       (1 << 4)

// 快速检查
bool F_is_keyword_token(Eum_TokenType type) {
    uint8_t categories = get_token_categories(type);
    return (categories & CATEGORY_KEYWORD) != 0;
}
```

## 内存使用

### 静态数据大小

| 组件 | 大小 | 说明 |
|------|------|------|
| 类型查询表 | ~2KB | 包含所有70个关键字的分类信息 |
| 类别名称表 | ~1KB | 类别名称字符串 |
| 关键字类别表 | ~3KB | 关键字详细分类信息 |

### 运行时内存

- 无动态内存分配
- 所有查询表为静态常量
- 线程安全的只读访问

## 线程安全性

### 安全级别

- **所有查询函数**：线程安全（只读操作）
- **无状态设计**：不维护任何运行时状态
- **无副作用**：纯函数设计

### 线程安全保证

```c
// 所有函数都是纯函数，线程安全
__attribute__((const))
bool F_is_keyword_token(Eum_TokenType type);

// 无副作用，可重入
__attribute__((pure))
const char* F_get_token_category(Eum_TokenType type);
```

## 测试覆盖

### 单元测试

测试文件：`tests/core/token/query/test_token_query.c`

**测试用例：**
1. 所有类型的基本分类测试
2. 关键字详细分类测试
3. 类型验证测试
4. 类型转换测试
5. 边界条件测试（无效类型、NULL参数）
6. 性能基准测试

### 测试数据

```c
// 测试数据生成
typedef struct {
    Eum_TokenType type;
    bool expected_is_keyword;
    bool expected_is_literal;
    const char* expected_category;
} TestCase_t;

static TestCase_t g_test_cases[] = {
    {Eum_TOKEN_KEYWORD_VAR, true, false, "KEYWORD"},
    {Eum_TOKEN_LITERAL_INTEGER, false, true, "LITERAL"},
    // ... 其他测试用例
};
```

## 依赖关系

### 内部依赖

- `CN_token_types.h`：令牌类型定义
- `CN_token_keywords.h`：关键字信息（用于详细分类）

### 外部依赖

- 无外部依赖（纯查询模块）

## 设计原则

### 单一职责

本模块只负责类型查询和分类，不涉及：
- 令牌创建和销毁
- 字面量
