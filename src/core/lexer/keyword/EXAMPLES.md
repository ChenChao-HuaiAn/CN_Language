# CN_Language 中文关键字模块 使用示例

## 目录

1. [基本使用](#基本使用)
2. [关键字查询](#关键字查询)
3. [关键字表迭代](#关键字表迭代)
4. [添加和删除关键字](#添加和删除关键字)
5. [关键字分类操作](#关键字分类操作)
6. [工具函数使用](#工具函数使用)
7. [完整示例程序](#完整示例程序)

## 基本使用

### 示例1：创建和销毁关键字表

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    // 创建关键字表（自动包含预定义关键字）
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL)
    {
        printf("错误：创建关键字表失败\n");
        return 1;
    }

    printf("关键字表创建成功\n");
    printf("关键字总数：%zu\n", F_get_keyword_count(table));

    // 使用关键字表...

    // 销毁关键字表
    F_destroy_keyword_table(table);
    printf("关键字表已销毁\n");

    return 0;
}
```

### 示例2：检查关键字

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    // 检查各种字符串是否为关键字
    const char* test_strings[] = {
        "如果",      // 是关键字
        "变量",      // 是关键字
        "hello",     // 不是关键字
        "123",       // 不是关键字
        "函数",      // 是关键字
        "未知"       // 不是关键字
    };

    for (int i = 0; i < 6; i++)
    {
        const char* str = test_strings[i];
        if (F_is_keyword(table, str, 0))
        {
            printf("'%s' 是一个关键字\n", str);
        }
        else
        {
            printf("'%s' 不是一个关键字\n", str);
        }
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

## 关键字查询

### 示例3：获取关键字详细信息

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    // 要查询的关键字列表
    const char* keywords[] = {
        "变量", "如果", "函数", "真", "模块"
    };

    for (int i = 0; i < 5; i++)
    {
        const char* keyword = keywords[i];
        const Stru_CN_KeywordInfo_t* info = F_get_keyword_info(table, keyword, 0);
        
        if (info != NULL)
        {
            printf("关键字: %s\n", info->keyword);
            printf("  英文等价: %s\n", info->english_equivalent ? info->english_equivalent : "无");
            printf("  描述: %s\n", info->description ? info->description : "无");
            printf("  分类: %d\n", info->category);
            printf("  保留关键字: %s\n", info->is_reserved ? "是" : "否");
            printf("\n");
        }
        else
        {
            printf("未找到关键字: %s\n\n", keyword);
        }
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

### 示例4：中英文关键字互查

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    // 中文到英文查询
    const char* chinese_keywords[] = {"变量", "如果", "函数", "真"};
    
    printf("中文到英文查询:\n");
    for (int i = 0; i < 4; i++)
    {
        const char* chinese = chinese_keywords[i];
        const char* english = F_get_english_equivalent(table, chinese, 0);
        
        if (english != NULL)
        {
            printf("  %s -> %s\n", chinese, english);
        }
    }

    printf("\n英文到中文查询:\n");
    // 英文到中文查询
    const char* english_keywords[] = {"variable", "if", "function", "true"};
    
    for (int i = 0; i < 4; i++)
    {
        const char* english = english_keywords[i];
        const char* chinese = F_get_chinese_equivalent(table, english);
        
        if (chinese != NULL)
        {
            printf("  %s -> %s\n", english, chinese);
        }
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

## 关键字表迭代

### 示例5：使用迭代器遍历关键字表

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    printf("所有关键字列表:\n");
    printf("================\n");

    // 使用迭代器遍历所有关键字
    size_t count = 0;
    for (size_t i = F_keyword_table_begin(table);
         i != F_keyword_table_end(table);
         i = F_keyword_table_next(table, i))
    {
        const Stru_CN_KeywordInfo_t* info = F_get_keyword_by_index(table, i);
        if (info != NULL)
        {
            printf("%3zu. %s", ++count, info->keyword);
            
            if (info->english_equivalent != NULL)
            {
                printf(" (%s)", info->english_equivalent);
            }
            
            printf("\n");
        }
    }

    printf("================\n");
    printf("总计: %zu 个关键字\n", count);

    F_destroy_keyword_table(table);
    return 0;
}
```

### 示例6：按索引访问关键字

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    size_t total = F_get_keyword_count(table);
    
    printf("关键字表索引访问示例:\n");
    printf("=====================\n");

    // 访问前5个关键字
    for (size_t i = 0; i < 5 && i < total; i++)
    {
        const Stru_CN_KeywordInfo_t* info = F_get_keyword_by_index(table, i);
        if (info != NULL)
        {
            printf("索引 %zu: %s\n", i, info->keyword);
        }
    }

    printf("...\n");

    // 访问最后5个关键字
    for (size_t i = total > 5 ? total - 5 : 0; i < total; i++)
    {
        const Stru_CN_KeywordInfo_t* info = F_get_keyword_by_index(table, i);
        if (info != NULL)
        {
            printf("索引 %zu: %s\n", i, info->keyword);
        }
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

## 添加和删除关键字

### 示例7：添加自定义关键字

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    printf("添加自定义关键字前，关键字总数: %zu\n", F_get_keyword_count(table));

    // 添加自定义关键字
    bool success = F_add_keyword(table,
                                "测试关键字",
                                0,
                                Eum_KEYWORD_CATEGORY_OTHER,
                                "这是一个测试用的自定义关键字",
                                "test_keyword",
                                false,  // 不是保留关键字
                                0);     // 无优先级

    if (success)
    {
        printf("添加自定义关键字成功\n");
        printf("添加后关键字总数: %zu\n", F_get_keyword_count(table));

        // 验证添加的关键字
        if (F_is_keyword(table, "测试关键字", 0))
        {
            const Stru_CN_KeywordInfo_t* info = F_get_keyword_info(table, "测试关键字", 0);
            if (info != NULL)
            {
                printf("自定义关键字信息:\n");
                printf("  关键字: %s\n", info->keyword);
                printf("  描述: %s\n", info->description);
                printf("  英文: %s\n", info->english_equivalent);
                printf("  分类: %d\n", info->category);
            }
        }
    }
    else
    {
        printf("添加自定义关键字失败\n");
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

### 示例8：删除关键字

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    printf("删除前关键字总数: %zu\n", F_get_keyword_count(table));

    // 先添加一个自定义关键字
    F_add_keyword(table, "临时关键字", 0,
                  Eum_KEYWORD_CATEGORY_OTHER,
                  "临时添加的关键字，用于测试删除",
                  "temp_keyword",
                  false, 0);

    printf("添加临时关键字后总数: %zu\n", F_get_keyword_count(table));

    // 检查临时关键字是否存在
    if (F_is_keyword(table, "临时关键字", 0))
    {
        printf("临时关键字存在，准备删除...\n");
        
        // 删除临时关键字
        if (F_remove_keyword(table, "临时关键字", 0))
        {
            printf("删除临时关键字成功\n");
            printf("删除后关键字总数: %zu\n", F_get_keyword_count(table));
            
            // 验证已删除
            if (!F_is_keyword(table, "临时关键字", 0))
            {
                printf("验证：临时关键字已不存在\n");
            }
        }
        else
        {
            printf("删除临时关键字失败\n");
        }
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

## 关键字分类操作

### 示例9：按分类获取关键字

```c
#include "CN_keywords.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    // 获取控制结构关键字
    size_t control_count = 0;
    Stru_CN_KeywordInfo_t** control_keywords = 
        F_get_keywords_by_category(table, Eum_KEYWORD_CATEGORY_CONTROL, &control_count);

    if (control_keywords != NULL && control_count > 0)
    {
        printf("控制结构关键字 (共 %zu 个):\n", control_count);
        for (size_t i = 0; i < control_count; i++)
        {
            printf("  • %s", control_keywords[i]->keyword);
            if (control_keywords[i]->description != NULL)
            {
                printf(" - %s", control_keywords[i]->description);
            }
            printf("\n");
        }
        
        // 释放内存
        free(control_keywords);
    }

    printf("\n");

    // 获取字面量关键字
    size_t literal_count = 0;
    Stru_CN_KeywordInfo_t** literal_keywords = 
        F_get_keywords_by_category(table, Eum_KEYWORD_CATEGORY_LITERAL, &literal_count);

    if (literal_keywords != NULL && literal_count > 0)
    {
        printf("字面量关键字 (共 %zu 个):\n", literal_count);
        for (size_t i = 0; i < literal_count; i++)
        {
            printf("  • %s", literal_keywords[i]->keyword);
            if (literal_keywords[i]->english_equivalent != NULL)
            {
                printf(" (%s)", literal_keywords[i]->english_equivalent);
            }
            printf("\n");
        }
        
        // 释放内存
        free(literal_keywords);
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

### 示例10：统计各分类关键字数量

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    printf("各分类关键字数量统计:\n");
    printf("=====================\n");

    // 定义分类名称数组
    const char* category_names[] = {
        "变量和类型声明",
        "控制结构", 
        "函数相关",
        "逻辑运算符",
        "字面量",
        "模块系统",
        "运算符",
        "类型",
        "其他"
    };

    size_t total = 0;
    
    // 遍历所有分类
    for (int category = 0; category <= Eum_KEYWORD_CATEGORY_OTHER; category++)
    {
        size_t count = F_get_keyword_count_by_category(table, 
            (Eum_CN_KeywordCategory_t)category);
        
        if (count > 0)
        {
            printf("%-20s: %3zu 个\n", category_names[category], count);
            total += count;
        }
    }

    printf("=====================\n");
    printf("%-20s: %3zu 个\n", "总计", total);
    printf("保留关键字: %zu 个\n", F_get_reserved_keyword_count(table));

    F_destroy_keyword_table(table);
    return 0;
}
```

## 工具函数使用

### 示例11：打印关键字信息

```c
#include "CN_keywords.h"
#include <stdio.h>

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    printf("打印单个关键字信息示例:\n");
    printf("=======================\n\n");

    // 打印几个关键字的详细信息
    const char* sample_keywords[] = {"变量", "如果", "函数", "真", "模块"};
    
    for (int i = 0; i < 5; i++)
    {
        const Stru_CN_KeywordInfo_t* info = F_get_keyword_info(table, sample_keywords[i], 0);
        if (info != NULL)
        {
            F_print_keyword_info(info);
        }
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

### 示例12：打印整个关键字表

```c
#include "CN_keywords.h"

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    // 方法1：打印详细的关键字表
    printf("方法1：详细关键字表\n");
    F_print_keyword_table(table);
    
    printf("\n\n");
    
    // 方法2：按分类打印关键字表（更清晰）
    printf("方法2：按分类分组的关键字表\n");
    F_print_keyword_table_by_category(table);

    F_destroy_keyword_table(table);
    return 0;
}
```

### 示例13：排序关键字表

```c
#include "CN_keywords.h"
#include <stdio.h>
#include <string.h>

// 自定义比较函数：按关键字长度排序
int compare_by_length(const void* a, const void* b)
{
    const Stru_CN_KeywordInfo_t* keyword_a = (const Stru_CN_KeywordInfo_t*)a;
    const Stru_CN_KeywordInfo_t* keyword_b = (const Stru_CN_KeywordInfo_t*)b;
    
    // 首先按长度排序
    if (keyword_a->length != keyword_b->length)
    {
        return (int)keyword_a->length - (int)keyword_b->length;
    }
    
    // 长度相同按字母顺序排序
    return strcmp(keyword_a->keyword, keyword_b->keyword);
}

int main()
{
    Stru_CN_KeywordTable_t* table = F_create_keyword_table();
    if (table == NULL) return 1;

    printf("排序前的前10个关键字:\n");
    for (size_t i = 0; i < 10 && i < table->count; i++)
    {
        printf("  %s (长度: %zu)\n", table->keywords[i].keyword, table->keywords[i].length);
    }

    // 使用自定义比较函数排序
    F_sort_keyword_table(table, compare_by_length);
    
    printf("\n按长度排序后的前10个关键字:\n");
    for (size_t i = 0; i < 10 && i < table->count; i++)
    {
        printf("  %s (长度: %zu)\n", table->keywords[i].keyword, table->keywords[i].length);
    }

    // 使用默认比较函数排序（按分类和关键字）
    F_sort_keyword_table(table, NULL);
    
    printf("\n按默认方式排序后的前10个关键字:\n");
    for (size_t i = 0; i < 10 && i < table->count; i++)
    {
        printf("  %s\n", table->keywords[i].keyword);
    }

    F_destroy_keyword_table(table);
    return 0;
}
```

## 完整示例程序

### 示例14：关键字管理器

```c
#include "CN_keywords.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 显示菜单
void show_menu()
{
    printf("\n=== CN_Language 关键字管理器 ===\n");
    printf("1. 显示所有关键字\n");
    printf("2. 按分类显示关键字\n");
    printf("3. 查询关键字信息\n");
    printf("4. 检查字符串是否为关键字\n");
    printf("5. 添加自定义关键字\n");
    printf("6. 删除关键字\n");
    printf("7. 统计信息\n");
    printf("8. 退出\n");
    printf("请选择操作 (1-8):
