/**
 * @file test_string_transform.c
 * @brief 字符串转换模块测试
 * 
 * 测试字符串转换模块的功能，包括：
 * 1. 大小写转换
 * 2. 空白处理
 * 3. 字符串反转
 * 4. 字符串填充
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
#include <ctype.h>

/**
 * @brief 测试大小写转换功能
 * 
 * 测试转换为大写、小写、切换大小写等功能。
 */
bool test_string_case_conversion(void)
{
    TEST_START("字符串大小写转换测试");
    
    // 测试1: 转换为小写
    Stru_String_t* str = F_string_create_from_cstr("Hello WORLD! 123 Test");
    bool to_lower_result = F_string_to_lower(str);
    TEST_ASSERT(to_lower_result, "转换为小写失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "hello world! 123 test") == 0,
               "转换为小写后内容不正确");
    
    // 测试2: 转换为大写
    bool to_upper_result = F_string_to_upper(str);
    TEST_ASSERT(to_upper_result, "转换为大写失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "HELLO WORLD! 123 TEST") == 0,
               "转换为大写后内容不正确");
    
    // 测试3: 切换大小写
    F_string_copy_cstr(str, "Hello World");
    bool swap_case_result = F_string_swap_case(str);
    TEST_ASSERT(swap_case_result, "切换大小写失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "hELLO wORLD") == 0,
               "切换大小写后内容不正确");
    
    // 测试4: 首字母大写
    F_string_copy_cstr(str, "hello world test");
    bool capitalize_result = F_string_capitalize(str);
    TEST_ASSERT(capitalize_result, "首字母大写失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Hello world test") == 0,
               "首字母大写后内容不正确");
    
    // 测试5: 每个单词首字母大写
    F_string_copy_cstr(str, "hello world test example");
    bool title_result = F_string_title(str);
    TEST_ASSERT(title_result, "每个单词首字母大写失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Hello World Test Example") == 0,
               "每个单词首字母大写后内容不正确");
    
    // 测试6: 空字符串转换
    Stru_String_t* empty = F_string_create_empty();
    TEST_ASSERT(F_string_to_lower(empty), "空字符串转换为小写应成功");
    TEST_ASSERT(F_string_to_upper(empty), "空字符串转换为大写应成功");
    TEST_ASSERT(F_string_capitalize(empty), "空字符串首字母大写应成功");
    TEST_ASSERT(F_string_title(empty), "空字符串每个单词首字母大写应成功");
    TEST_ASSERT(F_string_swap_case(empty), "空字符串切换大小写应成功");
    
    F_string_destroy(str);
    F_string_destroy(empty);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试空白处理功能
 * 
 * 测试去除空白、压缩空白等功能。
 */
bool test_string_whitespace_handling(void)
{
    TEST_START("字符串空白处理测试");
    
    // 测试1: 去除左侧空白
    Stru_String_t* str = F_string_create_from_cstr("   Hello World   ");
    bool trim_left_result = F_string_ltrim(str);
    TEST_ASSERT(trim_left_result, "去除左侧空白失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Hello World   ") == 0,
               "去除左侧空白后内容不正确");
    
    // 测试2: 去除右侧空白
    bool trim_right_result = F_string_rtrim(str);
    TEST_ASSERT(trim_right_result, "去除右侧空白失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Hello World") == 0,
               "去除右侧空白后内容不正确");
    
    // 测试3: 去除两侧空白
    F_string_copy_cstr(str, "   \t\n  Test String  \n\r  ");
    bool trim_result = F_string_trim(str);
    TEST_ASSERT(trim_result, "去除两侧空白失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "Test String") == 0,
               "去除两侧空白后内容不正确");
    
    // 测试4: 去除所有空白
    F_string_copy_cstr(str, "Hello     World   \t  Test");
    bool remove_whitespace_result = F_string_remove_whitespace(str);
    TEST_ASSERT(remove_whitespace_result, "去除所有空白失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "HelloWorldTest") == 0,
               "去除所有空白后内容不正确");
    
    // 测试5: 检查是否只包含空白
    F_string_copy_cstr(str, "   \t\n\r  ");
    TEST_ASSERT(F_string_is_space(str), "应只包含空白");
    
    F_string_copy_cstr(str, "  Hello  ");
    TEST_ASSERT(!F_string_is_space(str), "不应只包含空白");
    
    // 测试6: 空字符串空白处理
    Stru_String_t* empty = F_string_create_empty();
    TEST_ASSERT(F_string_trim(empty), "空字符串去除空白应成功");
    TEST_ASSERT(F_string_remove_whitespace(empty), "空字符串去除所有空白应成功");
    TEST_ASSERT(F_string_is_space(empty), "空字符串应视为只包含空白");
    
    F_string_destroy(str);
    F_string_destroy(empty);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符串反转和填充功能
 * 
 * 测试反转字符串、填充字符串等功能。
 */
bool test_string_reverse_and_padding(void)
{
    TEST_START("字符串反转和填充测试");
    
    // 测试1: 反转字符串
    Stru_String_t* str = F_string_create_from_cstr("Hello World");
    bool reverse_result = F_string_reverse(str);
    TEST_ASSERT(reverse_result, "反转字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "dlroW olleH") == 0,
               "反转后内容不正确");
    
    // 测试2: 填充左侧
    bool pad_left_result = F_string_pad_left(str, 15, '*');
    TEST_ASSERT(pad_left_result, "左侧填充失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "****dlroW olleH") == 0,
               "左侧填充后内容不正确");
    
    // 测试3: 填充右侧
    bool pad_right_result = F_string_pad_right(str, 20, '-');
    TEST_ASSERT(pad_right_result, "右侧填充失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "****dlroW olleH-----") == 0,
               "右侧填充后内容不正确");
    
    // 测试4: 两侧填充（居中）
    F_string_copy_cstr(str, "Hello");
    bool pad_center_result = F_string_pad_both(str, 11, '=');
    TEST_ASSERT(pad_center_result, "居中填充失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "===Hello===") == 0,
               "居中填充后内容不正确");
    
    // 测试5: 奇数长度居中
    F_string_copy_cstr(str, "Hi");
    pad_center_result = F_string_pad_both(str, 7, '+');
    TEST_ASSERT(pad_center_result, "奇数长度居中填充失败");
    TEST_ASSERT(strcmp(F_string_cstr(str), "++Hi+++") == 0,
               "奇数长度居中填充后内容不正确");
    
    // 测试6: 转换为ASCII
    F_string_copy_cstr(str, "Hello 世界!");
    bool to_ascii_result = F_string_to_ascii(str, '?');
    TEST_ASSERT(to_ascii_result, "转换为ASCII失败");
    // 注意："??!"是三字符序列，在C中代表'|'，所以我们需要避免使用三字符序列
    // 期望输出："Hello ??"（两个中文字符被替换为两个'?'，然后加上'!'）
    // 为了避免三字符序列，我们手动构建期望的字符串
    char expected[20];
    expected[0] = 'H';
    expected[1] = 'e';
    expected[2] = 'l';
    expected[3] = 'l';
    expected[4] = 'o';
    expected[5] = ' ';
    expected[6] = '?';
    expected[7] = '?';
    expected[8] = '!';
    expected[9] = '\0';
    TEST_ASSERT(strcmp(F_string_cstr(str), expected) == 0,
               "转换为ASCII后内容不正确");
    
    // 测试7: 空字符串反转和填充
    Stru_String_t* empty = F_string_create_empty();
    TEST_ASSERT(F_string_reverse(empty), "空字符串反转应成功");
    TEST_ASSERT(F_string_pad_left(empty, 5, 'X'), "空字符串左侧填充应成功");
    TEST_ASSERT(strcmp(F_string_cstr(empty), "XXXXX") == 0,
               "空字符串左侧填充后内容不正确");
    
    F_string_destroy(str);
    F_string_destroy(empty);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 运行所有转换模块测试
 */
bool test_string_transform_all(void)
{
    printf("运行字符串转换模块测试...\n");
    printf("============================\n\n");
    
    bool all_passed = true;
    
    all_passed = test_string_case_conversion() && all_passed;
    all_passed = test_string_whitespace_handling() && all_passed;
    all_passed = test_string_reverse_and_padding() && all_passed;
    
    printf("\n字符串转换模块测试 %s\n\n", 
           all_passed ? "✅ 全部通过" : "❌ 有失败");
    
    return all_passed;
}
