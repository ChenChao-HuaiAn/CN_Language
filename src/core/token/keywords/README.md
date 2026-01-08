# 令牌关键字信息管理模块

## 概述

令牌关键字信息管理模块负责管理70个中文关键字的信息，包括关键字的中文名称、英文对应词、功能描述、所属类别和优先级等。本模块提供了完整的关键字信息查询功能，支持通过中文、英文或令牌类型查找关键字详细信息。

## 模块职责

- 管理70个中文关键字的信息表
- 提供关键字信息查询功能（通过中文、英文、令牌类型）
- 支持关键字分类和优先级查询
- 提供关键字统计和枚举功能

## 文件结构

```
src/core/token/keywords/
├── CN_token_keywords.h    # 头文件 - 声明关键字管理函数和数据结构
├── CN_token_keywords.c    # 源文件 - 实现关键字管理函数
└── README.md              # 本文件 - 模块文档
```

## 数据结构

### Stru_KeywordInfo_t 结构体

关键字信息结构体，包含关键字的完整信息。

```c
typedef struct Stru_KeywordInfo_t {
    const char* chinese;            ///< 中文关键字
    const char* english;            ///< 英文对应词
    const char* description;        ///< 功能描述
    const char* category;           ///< 所属类别
    int precedence;                 ///< 优先级（0-10，越高优先级越高）
    Eum_TokenType token_type;       ///< 对应的令牌类型
} Stru_KeywordInfo_t;
```

### 关键字类别定义

70个中文关键字分为9个类别：

1. **变量和类型声明**（5个）：变量、常量、类型、类、对象
2. **控制结构**（9个）：如果、否则、当、循环、中断、继续、选择、情况、默认
3. **函数相关**（4个）：函数、返回、主程序、无
4. **逻辑运算符**（3个）：与、或、非
5. **字面量**（3个）：真、假、空
6. **模块系统**（3个）：模块、导入、导出
7. **运算符**（5个）：加、减、乘、除、模
8. **类型关键字**（5个）：整数、浮点数、字符串、布尔、字符
9. **其他关键字**（33个）：尝试、捕获、抛出、最终等

## API 参考

### 关键字信息查询

#### F_get_keyword_info

通过中文关键字获取关键字信息。

```c
const Stru_KeywordInfo_t* F_get_keyword_info(const char* chinese_keyword);
```

**参数：**
- `chinese_keyword`：中文关键字字符串

**返回值：**
- `const Stru_KeywordInfo_t*`：关键字信息指针
- `NULL`：未找到对应的关键字

**示例：**
```c
const Stru_KeywordInfo_t* info = F_get_keyword_info("变量");
if (info != NULL) {
    printf("关键字：%s\n", info->chinese);
    printf("英文：%s\n", info->english);
    printf("描述：%s\n", info->description);
    printf("类别：%s\n", info->category);
    printf("优先级：%d\n", info->precedence);
}
```

#### F_get_keyword_by_english

通过英文名获取关键字信息。

```c
const Stru_KeywordInfo_t* F_get_keyword_by_english(const char* english_name);
```

**参数：**
- `english_name`：英文名称

**返回值：**
- `const Stru_KeywordInfo_t*`：关键字信息指针
- `NULL`：未找到对应的关键字

**使用场景：**
- 国际化支持
- 代码生成
- 文档翻译

#### F_get_keyword_by_type

通过令牌类型获取关键字信息。

```c
const Stru_KeywordInfo_t* F_get_keyword_by_type(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型（必须是关键字类型）

**返回值：**
- `const Stru_KeywordInfo_t*`：关键字信息指针
- `NULL`：不是关键字类型或未找到

**特点：**
- 类型安全的查询
- 编译时类型检查
- 快速查找（O(1)时间复杂度）

### 关键字统计和枚举

#### F_get_keyword_count

获取关键字总数。

```c
size_t F_get_keyword_count(void);
```

**返回值：**
- 关键字总数（当前为70）

**用途：**
- 内存分配
- 循环边界
- 统计信息

#### F_get_keyword_at_index

通过索引获取关键字信息。

```c
const Stru_KeywordInfo_t* F_get_keyword_at_index(size_t index);
```

**参数：**
- `index`：索引（0到关键字总数-1）

**返回值：**
- `const Stru_KeywordInfo_t*`：关键字信息指针
- `NULL`：索引越界

**使用场景：**
- 遍历所有关键字
- 批量处理
- 测试和验证

#### F_get_keywords_by_category

获取指定类别的所有关键字。

```c
size_t F_get_keywords_by_category(const char* category, 
                                 const Stru_KeywordInfo_t** results, 
                                 size_t max_results);
```

**参数：**
- `category`：类别名称
- `results`：结果数组指针
- `max_results`：结果数组最大容量

**返回值：**
- 实际找到的关键字数量

**示例：**
```c
const Stru_KeywordInfo_t* control_keywords[10];
size_t count = F_get_keywords_by_category("控制结构", control_keywords, 10);
printf("找到 %zu 个控制结构关键字\n", count);
```

### 关键字验证和转换

#### F_is_valid_keyword

验证字符串是否为有效的中文关键字。

```c
bool F_is_valid_keyword(const char* chinese_keyword);
```

**参数：**
- `chinese_keyword`：中文关键字字符串

**返回值：**
- `true`：是有效关键字
- `false`：不是有效关键字

**验证规则：**
- 精确匹配（包括标点符号）
- 大小写敏感
- 支持全角和半角字符

#### F_convert_keyword_to_type

将中文关键字转换为对应的令牌类型。

```c
Eum_TokenType F_convert_keyword_to_type(const char* chinese_keyword);
```

**参数：**
- `chinese_keyword`：中文关键字字符串

**返回值：**
- 对应的令牌类型
- 如果未找到，返回`Eum_TOKEN_ERROR`

#### F_convert_type_to_keyword

将令牌类型转换为对应的中文关键字。

```c
const char* F_convert_type_to_keyword(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型（必须是关键字类型）

**返回值：**
- 中文关键字字符串
- 对于非关键字类型，返回NULL

### 关键字优先级管理

#### F_get_keyword_precedence

获取关键字的优先级。

```c
int F_get_keyword_precedence(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型（必须是关键字类型）

**返回值：**
- 优先级值（0-10）
- 对于非关键字类型，返回-1

**优先级规则：**
- 10：最高优先级（控制结构、类型声明）
- 8-9：高优先级（函数、运算符）
- 5-7：中优先级（逻辑运算符、字面量）
- 0-4：低优先级（其他关键字）

#### F_compare_keyword_precedence

比较两个关键字的优先级。

```c
int F_compare_keyword_precedence(Eum_TokenType type1, Eum_TokenType type2);
```

**参数：**
- `type1`：第一个关键字类型
- `type2`：第二个关键字类型

**返回值：**
- `>0`：type1优先级高于type2
- `=0`：优先级相同
- `<0`：type1优先级低于type2

## 使用示例

### 基本关键字查询

```c
#include "src/core/token/keywords/CN_token_keywords.h"
#include <stdio.h>

int main(void) {
    // 查询关键字信息
    const char* keywords[] = {"变量", "如果", "函数", "与", "真"};
    
    for (int i = 0; i < 5; i++) {
        const Stru_KeywordInfo_t* info = F_get_keyword_info(keywords[i]);
        if (info != NULL) {
            printf("关键字: %s\n", info->chinese);
            printf("  英文: %s\n", info->english);
            printf("  描述: %s\n", info->description);
            printf("  类别: %s\n", info->category);
            printf("  优先级: %d\n", info->precedence);
            printf("  令牌类型: %d\n", info->token_type);
            printf("\n");
        }
    }
    
    // 关键字总数
    size_t count = F_get_keyword_count();
    printf("总关键字数: %zu\n", count);
    
    return 0;
}
```

### 关键字遍历和分类

```c
#include "src/core/token/keywords/CN_token_keywords.h"
#include <stdio.h>

void print_keywords_by_category(const char* category) {
    const Stru_KeywordInfo_t* keywords[20];
    size_t count = F_get_keywords_by_category(category, keywords, 20);
    
    printf("类别 '%s' 的关键字 (%zu 个):\n", category, count);
    for (size_t i = 0; i < count; i++) {
        printf("  %s (%s)\n", keywords[i]->chinese, keywords[i]->english);
    }
    printf("\n");
}

int main(void) {
    // 按类别打印关键字
    print_keywords_by_category("控制结构");
    print_keywords_by_category("类型声明");
    print_keywords_by_category("函数相关");
    print_keywords_by_category("逻辑运算符");
    print_keywords_by_category("字面量");
    
    // 遍历所有关键字
    size_t total = F_get_keyword_count();
    printf("所有关键字 (%zu 个):\n", total);
    
    for (size_t i = 0; i < total; i++) {
        const Stru_KeywordInfo_t* info = F_get_keyword_at_index(i);
        if (info != NULL) {
            printf("%3zu. %-6s (%-10s) [%s]\n", 
                   i + 1, info->chinese, info->english, info->category);
        }
    }
    
    return 0;
}
```

### 关键字验证和转换

```c
#include "src/core/token/keywords/CN_token_keywords.h"
#include <stdio.h>

void test_keyword_conversion(const char* chinese_keyword) {
    // 验证关键字
    bool valid = F_is_valid_keyword(chinese_keyword);
    printf("关键字 '%s': %s\n", chinese_keyword, valid ? "有效" : "无效");
    
    if (valid) {
        // 转换为令牌类型
        Eum_TokenType type = F_convert_keyword_to_type(chinese_keyword);
        printf("  令牌类型: %d\n", type);
        
        // 转换回关键字
        const char* converted_back = F_convert_type_to_keyword(type);
        printf("  转换回: %s\n", converted_back);
        
        // 获取优先级
        int precedence = F_get_keyword_precedence(type);
        printf("  优先级: %d\n", precedence);
    }
    
    printf("\n");
}

int main(void) {
    // 测试有效关键字
    test_keyword_conversion("变量");
    test_keyword_conversion("如果");
    test_keyword_conversion("函数");
    
    // 测试无效关键字
    test_keyword_conversion("无效关键字");
    test_keyword_conversion("");
    test_keyword_conversion(NULL);
    
    // 优先级比较
    Eum_TokenType type1 = Eum_TOKEN_KEYWORD_VAR;    // 变量
    Eum_TokenType type2 = Eum_TOKEN_KEYWORD_IF;     // 如果
    
    int comparison = F_compare_keyword_precedence(type1, type2);
    const char* result;
    if (comparison > 0) {
        result = "高于";
    } else if (comparison < 0) {
        result = "低于";
    } else {
        result = "等于";
    }
    
    printf("'变量'优先级 %s '如果'\n", result);
    
    return 0;
}
```

### 高级关键字处理

```c
#include "src/core/token/keywords/CN_token_keywords.h"
#include <stdio.h>
#include <string.h>

// 关键字处理器接口
typedef void (*KeywordProcessor_t)(const Stru_KeywordInfo_t* info, void* context);

// 处理指定类别的关键字
void process_keywords_by_category(const char* category, 
                                 KeywordProcessor_t processor,
                                 void* context) {
    const Stru_KeywordInfo_t* keywords[20];
    size_t count = F_get_keywords_by_category(category, keywords, 20);
    
    for (size_t i = 0; i < count; i++) {
        processor(keywords[i], context);
    }
}

// 示例处理器：打印关键字信息
void print_keyword_info(const Stru_KeywordInfo_t* info, void* context) {
    int* count = (int*)context;
    (*count)++;
    
    printf("%2d. %-6s | %-10s | %-8s | %d\n",
           *count, info->chinese, info->english, 
           info->category, info->precedence);
}

// 示例处理器：收集关键字到数组
void collect_keywords(const Stru_KeywordInfo_t* info, void* context) {
    const Stru_KeywordInfo_t*** array_ptr = (const Stru_KeywordInfo_t***)context;
    **array_ptr = info;
    (*array_ptr)++;
}

int main(void) {
    // 使用处理器模式
    printf("控制结构关键字:\n");
    printf("序号 中文    英文        类别      优先级\n");
    printf("-----------------------------------------\n");
    
    int count = 0;
    process_keywords_by_category("控制结构", print_keyword_info, &count);
    
    // 收集关键字到数组
    const Stru_KeywordInfo_t* collected[10];
    const Stru_KeywordInfo_t** collector = collected;
    
    process_keywords_by_category("类型声明", collect_keywords, &collector);
    
    size_t collected_count = collector - collected;
    printf("\n收集到 %zu 个类型声明关键字\n", collected_count);
    
    // 通过英文名查询
    const Stru_KeywordInfo_t* var_info = F_get_keyword_by_english("var");
    if (var_info != NULL) {
        printf("\n通过英文名查询:\n");
        printf("  'var' -> '%s' (%s)\n", 
               var_info->chinese, var_info->description);
    }
    
    // 通过类型查询
    const Stru_KeywordInfo_t* if_info = F_get_keyword_by_type(Eum_TOKEN_KEYWORD_IF);
    if (if_info != NULL) {
        printf("通过类型查询:\n");
        printf("  Eum_TOKEN_KEYWORD_IF -> '%s'\n", if_info->chinese);
    }
    
    return 0;
}
```

## 性能优化

### 哈希表查询

使用静态哈希表实现O(1)时间复杂度的关键字查询：

```c
// 关键字哈希表条目
typedef struct {
    const char* key;               // 关键字字符串
    const Stru_KeywordInfo_t* info; // 指向关键字信息的指针
} KeywordHashEntry_t;

// 静态哈希表
static const KeywordHashEntry_t g_keyword_hash_table[] = {
    {"变量", &g_keyword_info[0]},
    {"如果", &g_keyword_info[1]},
    // ... 其他关键字
};

// 哈希函数
static size_t keyword_hash(const char* str) {
    // 简单但有效的哈希函数
    size_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash % KEYWORD_HASH_TABLE_SIZE;
}
```

### 缓存优化

对常用查询结果进行缓存：

```c
// 查询缓存
typedef struct {
    Eum_TokenType type;
    const Stru_KeywordInfo_t* info;
    uint32_t access_count;
} KeywordCacheEntry_t;

// 线程安全的缓存访问
const Stru_KeywordInfo_t* cached_get_keyword_by_type(Eum_TokenType type) {
    // 首先检查缓存
    KeywordCacheEntry_t* cached = find_in_cache(type);
    if (cached != NULL) {
        cached->access_count++;
        return cached->info;
    }
    
    // 缓存未命中，执行查询
    const Stru_KeywordInfo_t* info = F_get_keyword_by_type(type);
    if (info != NULL) {
        add_to_cache(type, info);
    }
    
    return info;
}
```

## 内存管理

### 静态数据大小

| 组件 | 大小 | 说明 |
|------|------|------|
| 关键字信息表 | ~10KB | 70个关键字的信息结构体 |
| 哈希表 | ~4KB | 关键字查询哈希表 |
| 类别索引 | ~2KB | 按类别组织的关键字索引 |
| 缓存 | ~1KB | 查询缓存（可选） |

### 内存布局优化

```c
// 紧凑的内存布局
typedef struct {
    const char chinese[8];      // 中文关键字（最大7字符+1终止符）
    const char english[12];     // 英文对应词
    const char description[32]; // 功能描述
    const char category[12];    // 所属类别
    uint8_t precedence;         // 优先级（0-10）
    Eum_TokenType token_type;   // 令牌类型
} PackedKeywordInfo_t;
```

## 线程安全性

### 安全级别

- **所有查询函数**：线程安全（只读操作）
- **静态数据**：编译时初始化，不可修改
- **无状态设计**：不维护任何运行时状态

### 线程安全保证

```c
// 所有数据都是const，线程安全
static const Stru_KeywordInfo_t g_keyword_info[] = {
    {"
