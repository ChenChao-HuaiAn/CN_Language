/**
 * @file test_string_operations.c
 * @brief 字符串操作模块测试
 
 * 测试字符串操作模块的功能，包括：
 * 1. 字符串追加操作
 * 2. 字符串比较操作
 * 3. 字符串复制操作
 * 4. 字符串连接操作
 
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
 * @brief 测试字符串追加功能
 
 * 测试追加C字符串、字符等功能。
 */
bool test_string_append(void)
{
    TEST_START("字符串追加测试");
    
    // 测试1: 追加C字符串
    Stru_String_t* str = F_string_create_from_cstr("Hello");
    bool append_result = F_string_append_cstr(str, ", World!");
    TEST_ASSERT(append_result, "追加C字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Hello, World!") == 0, 
               "追加后字符串内容不正确");
    F_string_destroy(str);
    
    // 测试2: 追加字符
    str = F_string_create_from_cstr("Test");
    for (int i = 0; i < 5; i++) {
        bool char_result = F_string_append_char(str, '!');
        TEST_ASSERT(char_result, "追加字符失败");
    }
    TEST_ASSERT(strcmp(F_string_cstr(str), "Test!!!!!") == 0, 
               "追加字符后内容不正确");
    F_string_destroy(str);
    
    // 测试3: 追加另一个字符串
    Stru_String_t* str1 = F_string_create_from_cstr("Hello ");
    Stru_String_t* str2 = F_string_create_from_cstr("World!");
    bool append_str_result = F_string_append(str1, str2);
    TEST_ASSERT(append_str_result, "追加字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(str1), "Hello World!") == 0, 
               "追加字符串后内容不正确");
    F_string_destroy(str1);
    F_string_destroy(str2);
    
    // 测试4: 追加空字符串
    str = F_string_create_from_cstr("Non-empty");
    bool append_empty_result = F_string_append_cstr(str, "");
    TEST_ASSERT(append_empty_result, "追加空字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Non-empty") == 0, 
               "追加空字符串不应改变内容");
    F_string_destroy(str);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符串比较功能
 
 * 测试字符串大小比较等功能。
 */
bool test_string_comparison(void)
{
    TEST_START("字符串比较测试");
    
    // 测试1: 大小比较
    Stru_String_t* str1 = F_string_create_from_cstr("Hello");
    Stru_String_t* str2 = F_string_create_from_cstr("Hello");
    Stru_String_t* str3 = F_string_create_from_cstr("World");
    
    TEST_ASSERT(F_string_compare(str1, str2) == 0, "相同字符串比较应为0");
    TEST_ASSERT(F_string_compare(str1, str3) < 0, "Hello应小于World");
    TEST_ASSERT(F_string_compare(str3, str1) > 0, "World应大于Hello");
    
    // 测试2: 不区分大小写比较
    Stru_String_t* str4 = F_string_create_from_cstr("hello");
    int result = F_string_compare_case_insensitive(str1, str4);
    TEST_ASSERT(result == 0, "不区分大小写比较应相等");
    
    // 测试3: 空字符串比较
    Stru_String_t* empty = F_string_create_empty();
    TEST_ASSERT(F_string_compare(empty, str1) < 0, "空字符串应小于非空字符串");
    
    F_string_destroy(str1);
    F_string_destroy(str2);
    F_string_destroy(str3);
    F_string_destroy(str4);
    F_string_destroy(empty);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符串复制功能
 
 * 测试字符串复制等功能。
 */
bool test_string_copy(void)
{
    TEST_START("字符串复制测试");
    
    // 测试1: 复制字符串
    Stru_String_t* original = F_string_create_from_cstr("Original String");
    Stru_String_t* copy = F_string_create_copy(original);
    TEST_ASSERT(copy != NULL, "复制字符串失败");
    
    // 使用compare函数检查是否相等
    TEST_ASSERT(F_string_compare(original, copy) == 0, "复制字符串应相等");
    
    // 修改副本不应影响原始字符串
    F_string_append_cstr(copy, " Modified");
    TEST_ASSERT(F_string_compare(original, copy) != 0, "修改后不应相等");
    TEST_ASSERT(strcmp(F_string_cstr(original), "Original String") == 0,
               "原始字符串不应被修改");
    
    F_string_destroy(original);
    F_string_destroy(copy);
    
    // 测试2: 复制到现有字符串
    Stru_String_t* dest = F_string_create_from_cstr("Destination");
    Stru_String_t* src = F_string_create_from_cstr("Source");
    bool copy_result = F_string_copy(dest, src);
    TEST_ASSERT(copy_result, "复制到现有字符串失败");
    TEST_ASSERT(F_string_compare(dest, src) == 0, "复制后应相等");
    
    F_string_destroy(dest);
    F_string_destroy(src);
    
    // 测试3: 复制C字符串
    dest = F_string_create_from_cstr("Old content");
    bool copy_cstr_result = F_string_copy_cstr(dest, "New content");
    TEST_ASSERT(copy_cstr_result, "复制C字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(dest), "New content") == 0,
               "复制C字符串后内容不正确");
    
    F_string_destroy(dest);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符串连接功能
 
 * 测试字符串连接等功能。
 */
bool test_string_concat(void)
{
    TEST_START("字符串连接测试");
    
    // 测试1: 连接两个字符串
    Stru_String_t* str1 = F_string_create_from_cstr("Hello ");
    Stru_String_t* str2 = F_string_create_from_cstr("World!");
    Stru_String_t* result = F_string_concat(str1, str2);
    
    TEST_ASSERT(result != NULL, "连接字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(result), "Hello World!") == 0,
               "连接后字符串内容不正确");
    
    F_string_destroy(str1);
    F_string_destroy(str2);
    F_string_destroy(result);
    
    // 测试2: 连接字符串和C字符串
    str1 = F_string_create_from_cstr("Number: ");
    result = F_string_concat_cstr(str1, "42");
    
    TEST_ASSERT(result != NULL, "连接C字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(result), "Number: 42") == 0,
               "连接C字符串后内容不正确");
    
    F_string_destroy(str1);
    F_string_destroy(result);
    
    // 测试3: 连接空字符串
    Stru_String_t* empty = F_string_create_empty();
    str1 = F_string_create_from_cstr("Non-empty");
    result = F_string_concat(empty, str1);
    
    TEST_ASSERT(result != NULL, "连接空字符串失败");
    TEST_ASSERT(F_string_compare(result, str1) == 0,
               "连接空字符串应等于非空字符串");
    
    F_string_destroy(empty);
    F_string_destroy(str1);
    F_string_destroy(result);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 运行所有操作模块测试
 */
bool test_string_operations_all(void)
{
    printf("运行字符串操作模块测试...\n");
    printf("============================\n\n");
    
    bool all_passed = true;
    
    all_passed = test_string_append() && all_passed;
    all_passed = test_string_comparison() && all_passed;
    all_passed = test_string_copy() && all_passed;
    all_passed = test_string_concat() && all_passed;
    
    printf("\n字符串操作模块测试 %s\n\n", 
           all_passed ? "✅ 全部通过" : "❌ 有失败");
    
    return all_passed;
}
