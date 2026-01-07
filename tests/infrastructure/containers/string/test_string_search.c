/**
 * @file test_string_search.c
 * @brief 字符串搜索模块测试
 
 * 测试字符串搜索模块的功能，包括：
 * 1. 子字符串操作
 * 2. 查找操作
 * 3. 替换操作
 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_string.h"
#include "CN_string.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 测试子字符串功能
 
 * 测试获取子字符串等功能。
 */
bool test_string_substring(void)
{
    TEST_START("子字符串测试");
    
    // 测试1: 获取子字符串
    Stru_String_t* str = F_string_create_from_cstr("Hello, World!");
    Stru_String_t* substr = F_string_substring(str, 7, 5); // "World"
    
    TEST_ASSERT(substr != NULL, "获取子字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(substr), "World") == 0,
               "子字符串内容不正确");
    
    // 测试2: 获取左子字符串
    Stru_String_t* left = F_string_left(str, 5); // "Hello"
    TEST_ASSERT(left != NULL, "获取左子字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(left), "Hello") == 0,
               "左子字符串内容不正确");
    
    // 测试3: 获取右子字符串
    Stru_String_t* right = F_string_right(str, 6); // "World!"
    TEST_ASSERT(right != NULL, "获取右子字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(right), "World!") == 0,
               "右子字符串内容不正确");
    
    // 测试4: 边界情况
    Stru_String_t* full = F_string_substring(str, 0, F_string_length(str));
    TEST_ASSERT(full != NULL, "获取整个字符串失败");
    TEST_ASSERT(F_string_compare(str, full) == 0, "整个字符串应相等");
    
    // 测试5: 无效参数
    Stru_String_t* invalid = F_string_substring(str, 100, 5);
    TEST_ASSERT(invalid == NULL, "无效起始位置应返回NULL");
    
    F_string_destroy(str);
    F_string_destroy(substr);
    F_string_destroy(left);
    F_string_destroy(right);
    F_string_destroy(full);
    if (invalid) F_string_destroy(invalid);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试查找功能
 
 * 测试查找子字符串、字符等功能。
 */
bool test_string_find(void)
{
    TEST_START("字符串查找测试");
    
    // 测试1: 查找子字符串
    Stru_String_t* str = F_string_create_from_cstr("Hello, World! Hello again!");
    Stru_String_t* search = F_string_create_from_cstr("Hello");
    
    // 查找第一次出现
    int64_t pos = F_string_find(str, search, 0);
    TEST_ASSERT(pos == 0, "查找'Hello'应在位置0");
    
    // 从指定位置查找
    pos = F_string_find(str, search, 1);
    TEST_ASSERT(pos == 14, "从位置1查找'Hello'应在位置14");
    
    // 查找不存在的子字符串
    Stru_String_t* not_found = F_string_create_from_cstr("NotFound");
    pos = F_string_find(str, not_found, 0);
    TEST_ASSERT(pos == -1, "查找不存在的字符串应返回-1");
    
    // 测试2: 查找C子字符串
    pos = F_string_find_cstr(str, "World", 0);
    TEST_ASSERT(pos == 7, "查找C字符串'World'应在位置7");
    
    // 测试3: 查找字符
    pos = F_string_find_char(str, 'W', 0);
    TEST_ASSERT(pos == 7, "查找字符'W'应在位置7");
    
    pos = F_string_find_char(str, '!', 0);
    TEST_ASSERT(pos == 12, "查找字符'!'应在位置12");
    
    // 查找不存在的字符
    pos = F_string_find_char(str, 'z', 0);
    TEST_ASSERT(pos == -1, "查找不存在的字符应返回-1");
    
    // 测试4: 从后向前查找字符
    pos = F_string_rfind_char(str, 'o', -1);
    TEST_ASSERT(pos == 18, "反向查找字符'o'应在位置18");
    
    // 测试5: 空字符串查找
    Stru_String_t* empty = F_string_create_empty();
    Stru_String_t* test = F_string_create_from_cstr("test");
    pos = F_string_find(empty, test, 0);
    TEST_ASSERT(pos == -1, "空字符串查找应返回-1");
    
    F_string_destroy(str);
    F_string_destroy(search);
    F_string_destroy(not_found);
    F_string_destroy(empty);
    F_string_destroy(test);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试替换功能
 
 * 测试替换子字符串等功能。
 */
bool test_string_replace(void)
{
    TEST_START("字符串替换测试");
    
    // 测试1: 替换子字符串
    Stru_String_t* str = F_string_create_from_cstr("Hello, World!");
    Stru_String_t* old_str = F_string_create_from_cstr("World");
    Stru_String_t* new_str = F_string_create_from_cstr("Universe");
    
    bool replace_result = F_string_replace(str, old_str, new_str);
    TEST_ASSERT(replace_result, "替换子字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Hello, Universe!") == 0,
               "替换后内容不正确");
    
    // 测试2: 替换C子字符串
    replace_result = F_string_replace_cstr(str, "Universe", "Earth");
    TEST_ASSERT(replace_result, "替换C子字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Hello, Earth!") == 0,
               "替换C子字符串后内容不正确");
    
    // 测试3: 替换所有子字符串
    Stru_String_t* str2 = F_string_create_from_cstr("cat dog cat bird cat");
    Stru_String_t* old_cat = F_string_create_from_cstr("cat");
    Stru_String_t* new_rabbit = F_string_create_from_cstr("rabbit");
    
    int64_t replace_count = F_string_replace_all(str2, old_cat, new_rabbit);
    TEST_ASSERT(replace_count == 3, "应替换3次");
    TEST_ASSERT(strcmp(F_string_cstr(str2), "rabbit dog rabbit bird rabbit") == 0,
               "替换所有后内容不正确");
    
    // 测试4: 替换所有C子字符串
    Stru_String_t* str3 = F_string_create_from_cstr("apple banana apple");
    replace_count = F_string_replace_all_cstr(str3, "apple", "orange");
    TEST_ASSERT(replace_count == 2, "替换C字符串应替换2次");
    TEST_ASSERT(strcmp(F_string_cstr(str3), "orange banana orange") == 0,
               "替换C字符串后内容不正确");
    
    // 测试5: 替换不存在的字符串
    replace_count = F_string_replace_all_cstr(str3, "tiger", "lion");
    TEST_ASSERT(replace_count == 0, "替换不存在的字符串应返回0");
    
    F_string_destroy(str);
    F_string_destroy(old_str);
    F_string_destroy(new_str);
    F_string_destroy(str2);
    F_string_destroy(old_cat);
    F_string_destroy(new_rabbit);
    F_string_destroy(str3);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 运行所有搜索模块测试
 */
bool test_string_search_all(void)
{
    printf("运行字符串搜索模块测试...\n");
    printf("============================\n\n");
    
    bool all_passed = true;
    
    all_passed = test_string_substring() && all_passed;
    all_passed = test_string_find() && all_passed;
    all_passed = test_string_replace() && all_passed;
    
    printf("\n字符串搜索模块测试 %s\n\n", 
           all_passed ? "✅ 全部通过" : "❌ 有失败");
    
    return all_passed;
}
