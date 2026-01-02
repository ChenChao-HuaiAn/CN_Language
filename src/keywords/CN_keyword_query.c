/******************************************************************************
 * 文件名: CN_keyword_query.c
 * 功能: CN_Language关键字查询功能实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现关键字查询功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_keywords.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 关键字查询函数实现
// ============================================================================

bool F_is_keyword(Stru_CN_KeywordTable_t* table,
                  const char* str,
                  size_t length)
{
    if (table == NULL || str == NULL)
    {
        return false;
    }

    // 计算字符串长度
    if (length == 0)
    {
        length = strlen(str);
    }

    // 检查字符串长度
    if (length > CN_MAX_KEYWORD_LENGTH)
    {
        return false;
    }

    // 线性搜索关键字
    for (size_t i = 0; i < table->count; i++)
    {
        const Stru_CN_KeywordInfo_t* info = &table->keywords[i];
        if (info->length == length && memcmp(info->keyword, str, length) == 0)
        {
            return true;
        }
    }

    return false;
}

const Stru_CN_KeywordInfo_t* F_get_keyword_info(Stru_CN_KeywordTable_t* table,
                                                const char* keyword,
                                                size_t length)
{
    if (table == NULL || keyword == NULL)
    {
        return NULL;
    }

    // 计算字符串长度
    if (length == 0)
    {
        length = strlen(keyword);
    }

    // 检查字符串长度
    if (length > CN_MAX_KEYWORD_LENGTH)
    {
        return NULL;
    }

    // 线性搜索关键字
    for (size_t i = 0; i < table->count; i++)
    {
        const Stru_CN_KeywordInfo_t* info = &table->keywords[i];
        if (info->length == length && memcmp(info->keyword, keyword, length) == 0)
        {
            return info;
        }
    }

    return NULL;
}

Stru_CN_KeywordInfo_t** F_get_keywords_by_category(Stru_CN_KeywordTable_t* table,
                                                   Eum_CN_KeywordCategory_t category,
                                                   size_t* count)
{
    if (table == NULL || count == NULL)
    {
        return NULL;
    }

    // 首先计算该分类的关键字数量
    size_t category_count = 0;
    for (size_t i = 0; i < table->count; i++)
    {
        if (table->keywords[i].category == category)
        {
            category_count++;
        }
    }

    if (category_count == 0)
    {
        *count = 0;
        return NULL;
    }

    // 分配结果数组
    Stru_CN_KeywordInfo_t** result = (Stru_CN_KeywordInfo_t**)malloc(
        sizeof(Stru_CN_KeywordInfo_t*) * category_count);
    if (result == NULL)
    {
        *count = 0;
        return NULL;
    }

    // 填充结果数组
    size_t index = 0;
    for (size_t i = 0; i < table->count; i++)
    {
        if (table->keywords[i].category == category)
        {
            result[index++] = &table->keywords[i];
        }
    }

    *count = category_count;
    return result;
}

const char* F_get_english_equivalent(Stru_CN_KeywordTable_t* table,
                                     const char* chinese_keyword,
                                     size_t length)
{
    const Stru_CN_KeywordInfo_t* info = F_get_keyword_info(table, chinese_keyword, length);
    if (info == NULL)
    {
        return NULL;
    }
    return info->english_equivalent;
}

const char* F_get_chinese_equivalent(Stru_CN_KeywordTable_t* table,
                                     const char* english_keyword)
{
    if (table == NULL || english_keyword == NULL)
    {
        return NULL;
    }

    size_t length = strlen(english_keyword);

    // 线性搜索英文等价关键字
    for (size_t i = 0; i < table->count; i++)
    {
        const Stru_CN_KeywordInfo_t* info = &table->keywords[i];
        if (info->english_equivalent != NULL &&
            strcmp(info->english_equivalent, english_keyword) == 0)
        {
            return info->keyword;
        }
    }

    return NULL;
}

// ============================================================================
// 关键字表迭代函数实现
// ============================================================================

size_t F_keyword_table_begin(Stru_CN_KeywordTable_t* table)
{
    if (table == NULL || table->count == 0)
    {
        return 0;
    }
    return 0;
}

size_t F_keyword_table_end(Stru_CN_KeywordTable_t* table)
{
    if (table == NULL)
    {
        return 0;
    }
    return table->count;
}

size_t F_keyword_table_next(Stru_CN_KeywordTable_t* table, size_t index)
{
    if (table == NULL || index >= table->count)
    {
        return F_keyword_table_end(table);
    }
    return index + 1;
}

const Stru_CN_KeywordInfo_t* F_get_keyword_by_index(Stru_CN_KeywordTable_t* table,
                                                    size_t index)
{
    if (table == NULL || index >= table->count)
    {
        return NULL;
    }
    return &table->keywords[index];
}

// ============================================================================
// 关键字表统计函数实现
// ============================================================================

size_t F_get_keyword_count(Stru_CN_KeywordTable_t* table)
{
    if (table == NULL)
    {
        return 0;
    }
    return table->count;
}

size_t F_get_keyword_count_by_category(Stru_CN_KeywordTable_t* table,
                                       Eum_CN_KeywordCategory_t category)
{
    if (table == NULL)
    {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < table->count; i++)
    {
        if (table->keywords[i].category == category)
        {
            count++;
        }
    }

    return count;
}

size_t F_get_reserved_keyword_count(Stru_CN_KeywordTable_t* table)
{
    if (table == NULL)
    {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < table->count; i++)
    {
        if (table->keywords[i].is_reserved)
        {
            count++;
        }
    }

    return count;
}

// ============================================================================
// 工具函数实现
// ============================================================================

int F_compare_keywords(const void* a, const void* b)
{
    const Stru_CN_KeywordInfo_t* keyword_a = (const Stru_CN_KeywordInfo_t*)a;
    const Stru_CN_KeywordInfo_t* keyword_b = (const Stru_CN_KeywordInfo_t*)b;

    // 首先按分类排序
    if (keyword_a->category != keyword_b->category)
    {
        return (int)keyword_a->category - (int)keyword_b->category;
    }

    // 然后按关键字字符串排序
    return strcmp(keyword_a->keyword, keyword_b->keyword);
}

void F_sort_keyword_table(Stru_CN_KeywordTable_t* table,
                          int (*compare_func)(const void*, const void*))
{
    if (table == NULL || table->count == 0)
    {
        return;
    }

    if (compare_func == NULL)
    {
        compare_func = F_compare_keywords;
    }

    qsort(table->keywords, table->count, sizeof(Stru_CN_KeywordInfo_t), compare_func);
}
