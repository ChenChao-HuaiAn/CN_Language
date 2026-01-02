/******************************************************************************
 * 文件名: CN_keyword_table.c
 * 功能: CN_Language关键字表管理实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现关键字表管理功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_keywords.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 安全分配内存
 * @param size 要分配的内存大小
 * @return 分配的内存指针，失败返回NULL
 */
static void* safe_malloc(size_t size);

/**
 * @brief 安全重新分配内存
 * @param ptr 原内存指针
 * @param size 新的内存大小
 * @return 重新分配的内存指针，失败返回NULL
 */
static void* safe_realloc(void* ptr, size_t size);

/**
 * @brief 复制字符串
 * @param str 要复制的字符串
 * @param length 字符串长度
 * @return 新分配的字符串副本，失败返回NULL
 */
static char* duplicate_string(const char* str, size_t length);

/**
 * @brief 计算字符串长度
 * @param str 字符串
 * @return 字符串长度（字节数）
 */
static size_t calculate_string_length(const char* str);

/**
 * @brief 扩展关键字表容量
 * @param table 关键字表
 * @return 扩展成功返回true，失败返回false
 */
static bool expand_keyword_table(Stru_CN_KeywordTable_t* table);

/**
 * @brief 查找关键字索引
 * @param table 关键字表
 * @param keyword 关键字字符串
 * @param length 关键字长度
 * @return 关键字索引，如果不存在返回-1
 */
static int find_keyword_index(Stru_CN_KeywordTable_t* table,
                              const char* keyword,
                              size_t length);

// ============================================================================
// 关键字表管理函数实现
// ============================================================================

Stru_CN_KeywordTable_t* F_create_keyword_table(void)
{
    Stru_CN_KeywordTable_t* table = (Stru_CN_KeywordTable_t*)safe_malloc(sizeof(Stru_CN_KeywordTable_t));
    if (table == NULL)
    {
        return NULL;
    }

    // 初始化表结构
    table->keywords = (Stru_CN_KeywordInfo_t*)safe_malloc(
        sizeof(Stru_CN_KeywordInfo_t) * CN_DEFAULT_KEYWORD_TABLE_CAPACITY);
    if (table->keywords == NULL)
    {
        free(table);
        return NULL;
    }

    table->count = 0;
    table->capacity = CN_DEFAULT_KEYWORD_TABLE_CAPACITY;

    // 初始化预定义关键字
    if (!F_initialize_predefined_keywords(table))
    {
        F_destroy_keyword_table(table);
        return NULL;
    }

    return table;
}

void F_destroy_keyword_table(Stru_CN_KeywordTable_t* table)
{
    if (table == NULL)
    {
        return;
    }

    // 释放所有关键字字符串
    for (size_t i = 0; i < table->count; i++)
    {
        Stru_CN_KeywordInfo_t* info = &table->keywords[i];
        if (info->keyword != NULL)
        {
            free((void*)info->keyword);
        }
        if (info->description != NULL)
        {
            free((void*)info->description);
        }
        if (info->english_equivalent != NULL)
        {
            free((void*)info->english_equivalent);
        }
    }

    // 释放关键字数组和表结构
    if (table->keywords != NULL)
    {
        free(table->keywords);
    }
    free(table);
}

bool F_add_keyword(Stru_CN_KeywordTable_t* table,
                   const char* keyword,
                   size_t length,
                   Eum_CN_KeywordCategory_t category,
                   const char* description,
                   const char* english_equivalent,
                   bool is_reserved,
                   int precedence)
{
    // 参数检查
    if (table == NULL || keyword == NULL)
    {
        return false;
    }

    // 检查关键字是否已存在
    if (find_keyword_index(table, keyword, length) != -1)
    {
        return false; // 关键字已存在
    }

    // 检查是否需要扩展容量
    if (table->count >= table->capacity)
    {
        if (!expand_keyword_table(table))
        {
            return false;
        }
    }

    // 计算字符串长度
    if (length == 0)
    {
        length = calculate_string_length(keyword);
    }

    // 检查关键字长度
    if (length > CN_MAX_KEYWORD_LENGTH)
    {
        return false;
    }

    // 复制关键字字符串
    char* keyword_copy = duplicate_string(keyword, length);
    if (keyword_copy == NULL)
    {
        return false;
    }

    // 复制描述字符串
    char* description_copy = NULL;
    if (description != NULL)
    {
        size_t desc_length = calculate_string_length(description);
        if (desc_length > CN_MAX_KEYWORD_DESCRIPTION_LENGTH)
        {
            free(keyword_copy);
            return false;
        }
        description_copy = duplicate_string(description, desc_length);
        if (description_copy == NULL)
        {
            free(keyword_copy);
            return false;
        }
    }

    // 复制英文等价字符串
    char* english_copy = NULL;
    if (english_equivalent != NULL)
    {
        english_copy = duplicate_string(english_equivalent, 0);
        if (english_copy == NULL)
        {
            free(keyword_copy);
            if (description_copy != NULL) free(description_copy);
            return false;
        }
    }

    // 添加新关键字
    Stru_CN_KeywordInfo_t* info = &table->keywords[table->count];
    info->keyword = keyword_copy;
    info->length = length;
    info->category = category;
    info->description = description_copy;
    info->english_equivalent = english_copy;
    info->is_reserved = is_reserved;
    info->precedence = precedence;

    table->count++;

    return true;
}

bool F_remove_keyword(Stru_CN_KeywordTable_t* table,
                      const char* keyword,
                      size_t length)
{
    if (table == NULL || keyword == NULL)
    {
        return false;
    }

    // 查找关键字索引
    int index = find_keyword_index(table, keyword, length);
    if (index == -1)
    {
        return false; // 关键字不存在
    }

    // 计算字符串长度
    if (length == 0)
    {
        length = calculate_string_length(keyword);
    }

    // 释放关键字字符串内存
    Stru_CN_KeywordInfo_t* info = &table->keywords[index];
    if (info->keyword != NULL)
    {
        free((void*)info->keyword);
    }
    if (info->description != NULL)
    {
        free((void*)info->description);
    }
    if (info->english_equivalent != NULL)
    {
        free((void*)info->english_equivalent);
    }

    // 移动后续元素覆盖被删除的元素
    for (size_t i = index; i < table->count - 1; i++)
    {
        table->keywords[i] = table->keywords[i + 1];
    }

    table->count--;

    return true;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

static void* safe_malloc(size_t size)
{
    void* ptr = malloc(size);
    if (ptr == NULL)
    {
        fprintf(stderr, "内存分配失败: 请求大小 %zu 字节\n", size);
    }
    return ptr;
}

static void* safe_realloc(void* ptr, size_t size)
{
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL && size > 0)
    {
        fprintf(stderr, "内存重新分配失败: 请求大小 %zu 字节\n", size);
        free(ptr); // 释放原指针
    }
    return new_ptr;
}

static char* duplicate_string(const char* str, size_t length)
{
    if (str == NULL)
    {
        return NULL;
    }

    if (length == 0)
    {
        length = strlen(str);
    }

    char* copy = (char*)safe_malloc(length + 1);
    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, str, length);
    copy[length] = '\0';

    return copy;
}

static size_t calculate_string_length(const char* str)
{
    if (str == NULL)
    {
        return 0;
    }
    return strlen(str);
}

static bool expand_keyword_table(Stru_CN_KeywordTable_t* table)
{
    size_t new_capacity = table->capacity * 2;
    Stru_CN_KeywordInfo_t* new_keywords = (Stru_CN_KeywordInfo_t*)safe_realloc(
        table->keywords, sizeof(Stru_CN_KeywordInfo_t) * new_capacity);
    
    if (new_keywords == NULL)
    {
        return false;
    }

    table->keywords = new_keywords;
    table->capacity = new_capacity;

    return true;
}

static int find_keyword_index(Stru_CN_KeywordTable_t* table,
                              const char* keyword,
                              size_t length)
{
    if (table == NULL || keyword == NULL)
    {
        return -1;
    }

    // 计算字符串长度
    if (length == 0)
    {
        length = calculate_string_length(keyword);
    }

    // 线性搜索关键字
    for (size_t i = 0; i < table->count; i++)
    {
        const Stru_CN_KeywordInfo_t* info = &table->keywords[i];
        if (info->length == length && memcmp(info->keyword, keyword, length) == 0)
        {
            return (int)i;
        }
    }

    return -1;
}
