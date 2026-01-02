# CN_Language 中文关键字模块 API 文档

## 概述

本文档详细描述了CN_Language中文关键字模块的所有API接口。关键字模块提供了完整的关键字管理、查询和操作功能。

## 头文件

```c
#include "CN_keywords.h"
```

## 数据类型

### 枚举类型

#### Eum_CN_KeywordCategory_t

关键字分类枚举，用于标识关键字的功能类别。

```c
typedef enum Eum_CN_KeywordCategory_t
{
    Eum_KEYWORD_CATEGORY_VARIABLE,      /**< 变量和类型声明关键字 */
    Eum_KEYWORD_CATEGORY_CONTROL,       /**< 控制结构关键字 */
    Eum_KEYWORD_CATEGORY_FUNCTION,      /**< 函数相关关键字 */
    Eum_KEYWORD_CATEGORY_LOGICAL,       /**< 逻辑运算符关键字 */
    Eum_KEYWORD_CATEGORY_LITERAL,       /**< 字面量关键字 */
    Eum_KEYWORD_CATEGORY_MODULE,        /**< 模块系统关键字 */
    Eum_KEYWORD_CATEGORY_OPERATOR,      /**< 运算符关键字 */
    Eum_KEYWORD_CATEGORY_TYPE,          /**< 类型关键字 */
    Eum_KEYWORD_CATEGORY_OTHER          /**< 其他关键字 */
} Eum_CN_KeywordCategory_t;
```

### 结构体类型

#### Stru_CN_KeywordInfo_t

关键字信息结构体，存储单个关键字的完整信息。

```c
typedef struct Stru_CN_KeywordInfo_t
{
    const char* keyword;                /**< 关键字字符串（UTF-8编码） */
    size_t length;                      /**< 关键字长度（字节数） */
    Eum_CN_KeywordCategory_t category;  /**< 关键字分类 */
    const char* description;            /**< 关键字功能描述 */
    const char* english_equivalent;     /**< 对应的英文关键字 */
    bool is_reserved;                   /**< 是否为保留关键字 */
    int precedence;                     /**< 运算符优先级（仅对运算符有效） */
} Stru_CN_KeywordInfo_t;
```

#### Stru_CN_KeywordTable_t

关键字表结构体，管理所有关键字的集合。

```c
typedef struct Stru_CN_KeywordTable_t
{
    Stru_CN_KeywordInfo_t* keywords;    /**< 关键字数组 */
    size_t count;                       /**< 关键字数量 */
    size_t capacity;                    /**< 数组容量 */
} Stru_CN_KeywordTable_t;
```

### 常量定义

```c
#define CN_MAX_KEYWORD_LENGTH 32                    /**< 最大关键字长度（UTF-8字节数） */
#define CN_DEFAULT_KEYWORD_TABLE_CAPACITY 64        /**< 默认关键字表容量 */
#define CN_MAX_KEYWORD_DESCRIPTION_LENGTH 256       /**< 关键字描述最大长度 */
```

## 函数参考

### 关键字表管理函数

#### F_create_keyword_table

创建关键字表。

```c
Stru_CN_KeywordTable_t* F_create_keyword_table(void);
```

**参数**: 无

**返回值**: 
- 成功: 新创建的关键字表指针
- 失败: NULL

**说明**: 
- 创建时会自动初始化预定义关键字
- 调用者负责使用`F_destroy_keyword_table()`释放资源

#### F_destroy_keyword_table

销毁关键字表。

```c
void F_destroy_keyword_table(Stru_CN_KeywordTable_t* table);
```

**参数**:
- `table`: 要销毁的关键字表

**返回值**: 无

**说明**:
- 对NULL参数安全
- 会释放所有关联的内存

#### F_add_keyword

向关键字表添加新关键字。

```c
bool F_add_keyword(Stru_CN_KeywordTable_t* table,
                   const char* keyword,
                   size_t length,
                   Eum_CN_KeywordCategory_t category,
                   const char* description,
                   const char* english_equivalent,
                   bool is_reserved,
                   int precedence);
```

**参数**:
- `table`: 关键字表
- `keyword`: 关键字字符串（UTF-8编码）
- `length`: 关键字长度（字节数），如果为0则自动计算
- `category`: 关键字分类
- `description`: 关键字功能描述（可选，可为NULL）
- `english_equivalent`: 对应的英文关键字（可选，可为NULL）
- `is_reserved`: 是否为保留关键字
- `precedence`: 运算符优先级（仅对运算符有效，其他关键字应为0）

**返回值**:
- 成功: true
- 失败: false（参数错误、内存不足、关键字已存在等）

#### F_remove_keyword

从关键字表移除关键字。

```c
bool F_remove_keyword(Stru_CN_KeywordTable_t* table,
                      const char* keyword,
                      size_t length);
```

**参数**:
- `table`: 关键字表
- `keyword`: 要移除的关键字
- `length`: 关键字长度（字节数），如果为0则自动计算

**返回值**:
- 成功: true
- 失败: false（参数错误、关键字不存在等）

### 关键字查询函数

#### F_is_keyword

检查字符串是否为关键字。

```c
bool F_is_keyword(Stru_CN_KeywordTable_t* table,
                  const char* str,
                  size_t length);
```

**参数**:
- `table`: 关键字表
- `str`: 要检查的字符串
- `length`: 字符串长度（字节数），如果为0则自动计算

**返回值**:
- 是关键字: true
- 不是关键字或参数错误: false

#### F_get_keyword_info

获取关键字信息。

```c
const Stru_CN_KeywordInfo_t* F_get_keyword_info(Stru_CN_KeywordTable_t* table,
                                                const char* keyword,
                                                size_t length);
```

**参数**:
- `table`: 关键字表
- `keyword`: 关键字字符串
- `length`: 关键字长度（字节数），如果为0则自动计算

**返回值**:
- 成功: 关键字信息指针
- 失败: NULL（关键字不存在或参数错误）

#### F_get_keywords_by_category

根据分类获取关键字列表。

```c
Stru_CN_KeywordInfo_t** F_get_keywords_by_category(Stru_CN_KeywordTable_t* table,
                                                   Eum_CN_KeywordCategory_t category,
                                                   size_t* count);
```

**参数**:
- `table`: 关键字表
- `category`: 关键字分类
- `count`: 输出参数，返回找到的关键字数量

**返回值**:
- 成功: 关键字信息指针数组
- 失败: NULL（参数错误或没有该分类的关键字）

**说明**:
- 调用者需要释放返回的数组（使用`free()`）
- 数组中的指针指向原始数据，不要修改或释放

#### F_get_english_equivalent

查找对应的英文关键字。

```c
const char* F_get_english_equivalent(Stru_CN_KeywordTable_t* table,
                                     const char* chinese_keyword,
                                     size_t length);
```

**参数**:
- `table`: 关键字表
- `chinese_keyword`: 中文关键字
- `length`: 关键字长度（字节数），如果为0则自动计算

**返回值**:
- 成功: 对应的英文关键字
- 失败: NULL（关键字不存在或没有英文等价）

#### F_get_chinese_equivalent

查找对应的中文关键字。

```c
const char* F_get_chinese_equivalent(Stru_CN_KeywordTable_t* table,
                                     const char* english_keyword);
```

**参数**:
- `table`: 关键字表
- `english_keyword`: 英文关键字

**返回值**:
- 成功: 对应的中文关键字
- 失败: NULL（关键字不存在或没有中文等价）

### 关键字表迭代函数

#### F_keyword_table_begin

获取关键字表迭代器（开始）。

```c
size_t F_keyword_table_begin(Stru_CN_KeywordTable_t* table);
```

**参数**:
- `table`: 关键字表

**返回值**:
- 第一个关键字的索引
- 如果表为空返回0

#### F_keyword_table_end

获取关键字表迭代器（结束）。

```c
size_t F_keyword_table_end(Stru_CN_KeywordTable_t* table);
```

**参数**:
- `table`: 关键字表

**返回值**:
- 最后一个关键字的索引+1

#### F_keyword_table_next

获取下一个关键字。

```c
size_t F_keyword_table_next(Stru_CN_KeywordTable_t* table, size_t index);
```

**参数**:
- `table`: 关键字表
- `index`: 当前索引

**返回值**:
- 下一个关键字的索引
- 如果到达末尾返回`F_keyword_table_end(table)`

#### F_get_keyword_by_index

获取关键字信息（通过索引）。

```c
const Stru_CN_KeywordInfo_t* F_get_keyword_by_index(Stru_CN_KeywordTable_t* table,
                                                    size_t index);
```

**参数**:
- `table`: 关键字表
- `index`: 关键字索引

**返回值**:
- 成功: 关键字信息指针
- 失败: NULL（索引无效）

### 关键字表统计函数

#### F_get_keyword_count

获取关键字总数。

```c
size_t F_get_keyword_count(Stru_CN_KeywordTable_t* table);
```

**参数**:
- `table`: 关键字表

**返回值**:
- 关键字总数
- 参数错误返回0

#### F_get_keyword_count_by_category

获取分类关键字数量。

```c
size_t F_get_keyword_count_by_category(Stru_CN_KeywordTable_t* table,
                                       Eum_CN_KeywordCategory_t category);
```

**参数**:
- `table`: 关键字表
- `category`: 关键字分类

**返回值**:
- 该分类的关键字数量
- 参数错误返回0

#### F_get_reserved_keyword_count

获取保留关键字数量。

```c
size_t F_get_reserved_keyword_count(Stru_CN_KeywordTable_t* table);
```

**参数**:
- `table`: 关键字表

**返回值**:
- 保留关键字数量
- 参数错误返回0

### 预定义关键字访问函数

#### F_initialize_predefined_keywords

初始化预定义关键字。

```c
bool F_initialize_predefined_keywords(Stru_CN_KeywordTable_t* table);
```

**参数**:
- `table`: 关键字表

**返回值**:
- 成功: true
- 失败: false（参数错误或内存不足）

**说明**:
- 将CN语言的所有预定义关键字添加到表中
- 通常在`F_create_keyword_table()`中自动调用

#### F_get_predefined_keyword_table

获取预定义的关键字表（只读）。

```c
const Stru_CN_KeywordTable_t* F_get_predefined_keyword_table(void);
```

**参数**: 无

**返回值**:
- 预定义的关键字表指针
- 失败返回NULL

**说明**:
- 返回的表是只读的，不要修改
- 首次调用时会初始化全局预定义关键字表

### 工具函数

#### F_print_keyword_info

打印关键字信息。

```c
void F_print_keyword_info(const Stru_CN_KeywordInfo_t* keyword_info);
```

**参数**:
- `keyword_info`: 关键字信息

**返回值**: 无

**说明**:
- 对NULL参数安全
- 输出到标准输出

#### F_print_keyword_table

打印整个关键字表。

```c
void F_print_keyword_table(Stru_CN_KeywordTable_t* table);
```

**参数**:
- `table`: 关键字表

**返回值**: 无

**说明**:
- 对NULL参数安全
- 输出到标准输出

#### F_print_keyword_table_by_category

按分类打印关键字表。

```c
void F_print_keyword_table_by_category(Stru_CN_KeywordTable_t* table);
```

**参数**:
- `table`: 关键字表

**返回值**: 无

**说明**:
- 对NULL参数安全
- 按分类分组输出到标准输出

#### F_compare_keywords

比较两个关键字（用于排序）。

```c
int F_compare_keywords(const void* a, const void* b);
```

**参数**:
- `a`: 第一个关键字
- `b`: 第二个关键字

**返回值**:
- <0: a < b
- 0: a == b
- >0: a > b

**说明**:
- 首先按分类排序，然后按关键字字符串排序
- 可用于`qsort()`函数

#### F_sort_keyword_table

对关键字表排序。

```c
void F_sort_keyword_table(Stru_CN_KeywordTable_t* table,
                          int (*compare_func)(const void*, const void*));
```

**参数**:
- `table`: 关键字表
- `compare_func`: 比较函数，如果为NULL使用默认比较函数

**返回值**: 无

### 关键字表导入导出函数

#### F_load_keyword_table_from_file

从文件加载关键字表。

```c
Stru_CN_KeywordTable_t* F_load_keyword_table_from_file(const char* filename);
```

**参数**:
- `filename`: 文件名

**返回值**:
- 成功: 加载的关键字表指针
- 失败: NULL

**说明**:
- 当前版本暂不支持
- 保留接口供未来扩展

#### F_save_keyword_table_to_file

保存关键字表到文件。

```c
bool F_save_keyword_table_to_file(Stru_CN_KeywordTable_t* table,
                                  const char* filename);
```

**参数**:
- `table`: 关键字表
- `filename`: 文件名

**返回值**:
- 成功: true
- 失败: false

**说明**:
- 当前版本暂不支持
- 保留接口供未来扩展

#### F_export_keyword_table_to_c_code

导出关键字表为C代码。

```c
bool F_export_keyword_table_to_c_code(Stru_CN_KeywordTable_t* table,
                                      const char* filename);
```

**参数**:
- `table`: 关键字表
- `filename`: 输出文件名

**返回值**:
- 成功: true
- 失败: false

**说明**:
- 生成可以直接编译的C代码文件
- 包含关键字数据数组和分类枚举

## 使用示例

### 示例1：基本使用

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    // 创建关键字表
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) {
        printf("创建关键字表失败\n");
        return 1;
    }

    // 检查关键字
    if (F_is_keyword(table, "如果", 0)) {
        printf("'如果'是一个关键字\n");
    }

    // 获取关键字信息
    const Stru_CN_KeywordInfo_t* info = F_get_keyword_info(table, "变量", 0);
    if (info != NULL) {
        printf("关键字: %s\n", info->keyword);
        printf("英文: %s\n", info->english_equivalent);
        printf("描述: %s\n", info->description);
    }

    // 打印关键字表
    F_print_keyword_table_by_category(table);

    // 销毁关键字表
    F_destroy_keyword_table(table);
    
    return 0;
}
```

### 示例2：迭代关键字表

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    // 使用迭代器遍历关键字表
    for (size_t i = F_keyword_table_begin(table); 
         i != F_keyword_table_end(table); 
         i = F_keyword_table_next(table, i)) {
        
        const Stru_CN_KeywordInfo_t* info = F_get_keyword_by_index(table, i);
        if (info != NULL) {
            printf("%zu. %s\n", i + 1, info->keyword);
        }
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

### 示例3：添加自定义关键字

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    // 添加自定义关键字
    bool success = F_add_keyword(table,
                                "测试关键字",
                                0,
                                Eum_KEYWORD_CATEGORY_OTHER,
                                "这是一个测试关键字",
                                "test_keyword",
                                false,
                                0);
    
    if (success) {
        printf("添加自定义关键字成功\n");
    }

    // 检查自定义关键字
    if (F_is_keyword(table, "测试关键字", 0)) {
        printf("自定义关键字已添加\n");
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

## 错误处理

所有函数都提供适当的错误处理：

1. **创建函数**：失败时返回NULL
2. **操作函数**：返回布尔值表示成功或失败
3. **查询函数**：找不到时返回NULL
4. **打印函数**：对NULL参数安全

## 内存管理

- 所有动态分配的内存都需要正确释放
- 使用`F_destroy_keyword_table()`释放整个关键字表
- 使用`free()`释放`F_get_keywords_by_category()`返回的数组
- 不要手动释放关键字信息结构体中的字符串指针

## 线程安全

当前版本不是线程安全的。如果需要在多线程环境中使用，需要外部同步。

## 版本历史

- v1.0.0 (2026-01-02): 初始版本
  - 完整的关键字管理功能
  - 预定义中文关键字支持
  - 基本查询和迭代功能
  - 工具函数和打印功能

## 相关文档

- [README.md](README.md) - 模块概述和使用指南
- [EXAMPLES.md](EXAMPLES.md) - 更多使用示例
- [DESIGN.md](DESIGN.md) - 设计原理和架构说明
