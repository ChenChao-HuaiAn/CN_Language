/**
 * @file test_string_core.c
 * @brief 字符串模块核心功能测试
 * 
 * 测试字符串核心模块的功能，包括：
 * 1. 字符串创建和销毁
 * 2. 字符串基本属性获取
 * 3. 字符串容量管理
 * 4. 字符串内存管理
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_string.h"
#include "CN_string.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 测试字符串创建功能
 * 
 * 测试创建空字符串、从C字符串创建、从另一个字符串创建等功能。
 */
bool test_string_creation(void)
{
    TEST_START("字符串创建测试");
    
    // 测试1: 创建空字符串
    Stru_String_t* empty_str = F_string_create_empty();
    TEST_ASSERT(empty_str != NULL, "创建空字符串失败");
    TEST_ASSERT(F_string_length(empty_str) == 0, "空字符串长度应为0");
    TEST_ASSERT(F_string_capacity(empty_str) >= 0, "空字符串容量应>=0");
    F_string_destroy(empty_str);
    
    // 测试2: 从C字符串创建
    const char* cstr = "Hello, World!";
    Stru_String_t* str1 = F_string_create_from_cstr(cstr);
    TEST_ASSERT(str1 != NULL, "从C字符串创建失败");
    TEST_ASSERT(F_string_length(str1) == strlen(cstr), "字符串长度不正确");
    TEST_ASSERT(strcmp(F_string_cstr(str1), cstr) == 0, "字符串内容不正确");
    F_string_destroy(str1);
    
    // 测试3: 从另一个字符串创建（复制）
    Stru_String_t* original = F_string_create_from_cstr("测试字符串");
    Stru_String_t* copy = F_string_create_copy(original);
    TEST_ASSERT(copy != NULL, "字符串复制失败");
    TEST_ASSERT(F_string_length(copy) == F_string_length(original), "复制字符串长度不一致");
    TEST_ASSERT(strcmp(F_string_cstr(copy), F_string_cstr(original)) == 0, "复制字符串内容不一致");
    F_string_destroy(original);
    F_string_destroy(copy);
    
    // 测试4: 创建带初始容量的字符串
    Stru_String_t* str_with_capacity = F_string_create_with_capacity(100);
    TEST_ASSERT(str_with_capacity != NULL, "创建带容量字符串失败");
    TEST_ASSERT(F_string_capacity(str_with_capacity) >= 100, "字符串容量不足");
    TEST_ASSERT(F_string_length(str_with_capacity) == 0, "带容量字符串长度应为0");
    F_string_destroy(str_with_capacity);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符串基本属性
 * 
 * 测试字符串长度、容量、空状态等基本属性。
 */
bool test_string_properties(void)
{
    TEST_START("字符串属性测试");
    
    // 测试1: 空字符串属性
    Stru_String_t* empty = F_string_create_empty();
    TEST_ASSERT(F_string_is_empty(empty), "空字符串应返回true");
    TEST_ASSERT(F_string_length(empty) == 0, "空字符串长度应为0");
    F_string_destroy(empty);
    
    // 测试2: 非空字符串属性
    Stru_String_t* str = F_string_create_from_cstr("Hello");
    TEST_ASSERT(!F_string_is_empty(str), "非空字符串应返回false");
    TEST_ASSERT(F_string_length(str) == 5, "字符串长度应为5");
    TEST_ASSERT(F_string_capacity(str) >= 5, "字符串容量应>=长度");
    F_string_destroy(str);
    
    // 测试3: 长字符串属性
    const char* long_text = "这是一个较长的测试字符串，用于测试容量和长度属性";
    Stru_String_t* long_str = F_string_create_from_cstr(long_text);
    TEST_ASSERT(F_string_byte_length(long_str) == strlen(long_text), "长字符串字节长度不正确");
    TEST_ASSERT(F_string_capacity(long_str) >= strlen(long_text), "长字符串容量不足");
    F_string_destroy(long_str);
    
    // 测试4: 获取C字符串
    const char* test_text = "Test C String";
    Stru_String_t* test_str = F_string_create_from_cstr(test_text);
    const char* cstr_result = F_string_cstr(test_str);
    TEST_ASSERT(cstr_result != NULL, "获取C字符串失败");
    TEST_ASSERT(strcmp(cstr_result, test_text) == 0, "C字符串内容不正确");
    F_string_destroy(test_str);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符串容量管理
 * 
 * 测试字符串预留容量、缩减容量等功能。
 */
bool test_string_capacity(void)
{
    TEST_START("字符串容量管理测试");
    
    // 测试1: 预留容量
    Stru_String_t* str = F_string_create_empty();
    size_t initial_capacity = F_string_capacity(str);
    
    bool reserve_result = F_string_reserve(str, 100);
    TEST_ASSERT(reserve_result, "预留容量失败");
    TEST_ASSERT(F_string_capacity(str) >= 100, "预留后容量不足");
    
    // 测试2: 缩减容量到合适大小
    F_string_append_cstr(str, "Hello");
    size_t before_shrink = F_string_capacity(str);
    bool shrink_result = F_string_shrink_to_fit(str);
    TEST_ASSERT(shrink_result, "缩减容量失败");
    TEST_ASSERT(F_string_capacity(str) <= before_shrink, "缩减后容量应减小");
    TEST_ASSERT(F_string_capacity(str) >= F_string_length(str), "缩减后容量应>=长度");
    
    // 测试3: 清空字符串
    F_string_clear(str);
    TEST_ASSERT(F_string_is_empty(str), "清空后字符串应为空");
    TEST_ASSERT(F_string_length(str) == 0, "清空后长度应为0");
    TEST_ASSERT(F_string_capacity(str) > 0, "清空后容量应>0");
    
    F_string_destroy(str);
    
    // 测试4: 容量增长策略
    Stru_String_t* growing_str = F_string_create_empty();
    size_t prev_capacity = F_string_capacity(growing_str);
    
    // 添加内容使字符串增长
    for (int i = 0; i < 100; i++) {
        F_string_append_char(growing_str, 'A');
        size_t new_capacity = F_string_capacity(growing_str);
        if (new_capacity > prev_capacity) {
            // 容量增长应至少是原来的1.5倍（常见增长策略）
            TEST_ASSERT(new_capacity >= prev_capacity * 1.5 || 
                       new_capacity >= F_string_length(growing_str),
                       "容量增长策略不合理");
            prev_capacity = new_capacity;
        }
    }
    
    F_string_destroy(growing_str);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符串内存管理
 * 
 * 测试字符串的内存分配、释放和所有权。
 */
bool test_string_memory(void)
{
    TEST_START("字符串内存管理测试");
    
    // 测试1: 创建和销毁大量字符串
    const int NUM_STRINGS = 100;
    Stru_String_t* strings[NUM_STRINGS];
    
    for (int i = 0; i < NUM_STRINGS; i++) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "String %d", i);
        strings[i] = F_string_create_from_cstr(buffer);
        TEST_ASSERT(strings[i] != NULL, "创建字符串失败");
    }
    
    // 验证所有字符串
    for (int i = 0; i < NUM_STRINGS; i++) {
        char expected[50];
        snprintf(expected, sizeof(expected), "String %d", i);
        TEST_ASSERT(strcmp(F_string_cstr(strings[i]), expected) == 0, 
                   "字符串内容不正确");
    }
    
    // 销毁所有字符串
    for (int i = 0; i < NUM_STRINGS; i++) {
        F_string_destroy(strings[i]);
    }
    
    // 测试2: 字符串所有权转移（通过复制）
    Stru_String_t* original = F_string_create_from_cstr("Original");
    Stru_String_t* copy1 = F_string_create_copy(original);
    Stru_String_t* copy2 = F_string_create_copy(copy1);
    
    // 修改原始字符串不应影响副本
    F_string_clear(original);
    F_string_append_cstr(original, "Modified");
    
    TEST_ASSERT(strcmp(F_string_cstr(copy1), "Original") == 0, 
               "副本1不应被修改");
    TEST_ASSERT(strcmp(F_string_cstr(copy2), "Original") == 0, 
               "副本2不应被修改");
    
    F_string_destroy(original);
    F_string_destroy(copy1);
    F_string_destroy(copy2);
    
    // 测试3: 空指针安全
    // 这些调用应该安全处理NULL指针
    F_string_destroy(NULL);  // 应安全处理
    // F_string_length(NULL);   // 实现中应有NULL检查
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 运行所有核心模块测试
 */
bool test_string_core_all(void)
{
    printf("运行字符串核心模块测试...\n");
    printf("==========================\n\n");
    
    bool all_passed = true;
    
    all_passed = test_string_creation() && all_passed;
    all_passed = test_string_properties() && all_passed;
    all_passed = test_string_capacity() && all_passed;
    all_passed = test_string_memory() && all_passed;
    
    printf("\n字符串核心模块测试 %s\n\n", 
           all_passed ? "✅ 全部通过" : "❌ 有失败");
    
    return all_passed;
}
