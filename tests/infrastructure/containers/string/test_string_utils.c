/**
 * @file test_string_utils.c
 * @brief 字符串工具模块测试
 * 
 * 测试字符串工具模块的功能，包括：
 * 1. UTF-8处理
 * 2. 字符分类
 * 3. 格式化功能
 * 4. 字符串分割和连接
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
 * @brief 测试UTF-8处理功能
 * 
 * 测试UTF-8编码验证、字符长度计算等功能。
 */
bool test_string_utf8_handling(void)
{
    TEST_START("UTF-8处理测试");
    
    // 测试1: 计算UTF-8字符串长度
    const char* utf8_str = "Hello 世界 🌍";
    size_t utf8_len = F_utf8_strlen(utf8_str);
    // "Hello 世界 🌍" 包含: H(1) e(1) l(1) l(1) o(1) 空格(1) 世(1) 界(1) 空格(1) 🌍(1)
    // 总共: 6个ASCII + 2个中文字符 + 1个空格 + 1个emoji = 10个Unicode字符
    TEST_ASSERT(utf8_len == 10, "UTF-8字符数量计算不正确");
    
    // 测试2: 获取UTF-8字符的字节长度
    size_t ascii_len = F_utf8_char_len('A');
    TEST_ASSERT(ascii_len == 1, "ASCII字符长度应为1");
    
    size_t chinese_len = F_utf8_char_len(0xE4); // 中文字符的第一个字节通常是0xE4
    TEST_ASSERT(chinese_len == 3, "中文字符长度应为3");
    
    // 测试3: 验证有效的UTF-8字符串
    Stru_String_t* valid_utf8 = F_string_create_from_cstr("Hello 世界 🌍");
    bool is_valid = F_string_validate_utf8(valid_utf8);
    TEST_ASSERT(is_valid, "有效的UTF-8字符串应通过验证");
    
    // 测试4: 验证C字符串UTF-8编码
    bool cstr_valid = F_cstr_validate_utf8("Hello World", 0);
    TEST_ASSERT(cstr_valid, "有效的ASCII字符串应通过验证");
    
    // 测试5: 获取UTF-8字符在字符串中的位置
    int64_t pos = F_string_utf8_char_position(valid_utf8, 6); // 位置6应该是'世'
    TEST_ASSERT(pos >= 0, "应找到UTF-8字符位置");
    TEST_ASSERT(pos == 6, "ASCII部分位置应正确");
    
    // 测试6: 验证无效的UTF-8字符串
    // 创建一个包含无效UTF-8序列的字符串
    char invalid_utf8[] = { 'H', 'e', 'l', 'l', 'o', 0xC0, 0x80, 0 }; // 无效的过度编码
    Stru_String_t* invalid_str = F_string_create_from_cstr(invalid_utf8);
    is_valid = F_string_validate_utf8(invalid_str);
    TEST_ASSERT(!is_valid, "无效的UTF-8字符串应失败验证");
    
    // 清理
    F_string_destroy(valid_utf8);
    F_string_destroy(invalid_str);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符分类功能
 * 
 * 测试字符类型检查、数字字母检查等功能。
 */
bool test_string_character_classification(void)
{
    TEST_START("字符分类测试");
    
    // 测试1: 检查是否只包含字母
    Stru_String_t* str = F_string_create_from_cstr("HelloWorld");
    TEST_ASSERT(F_string_is_alpha(str), "应只包含字母");
    
    F_string_copy_cstr(str, "Hello123");
    TEST_ASSERT(!F_string_is_alpha(str), "包含数字不应只包含字母");
    
    // 测试2: 检查是否只包含数字
    F_string_copy_cstr(str, "123456");
    TEST_ASSERT(F_string_is_digit(str), "应只包含数字");
    
    F_string_copy_cstr(str, "123abc");
    TEST_ASSERT(!F_string_is_digit(str), "包含字母不应只包含数字");
    
    // 测试3: 检查是否只包含字母数字
    F_string_copy_cstr(str, "Hello123");
    TEST_ASSERT(F_string_is_alnum(str), "应只包含字母数字");
    
    F_string_copy_cstr(str, "Hello 123");
    TEST_ASSERT(!F_string_is_alnum(str), "包含空格不应只包含字母数字");
    
    // 测试4: 检查是否只包含ASCII字符
    F_string_copy_cstr(str, "Hello123!@#");
    TEST_ASSERT(F_string_is_ascii(str), "应只包含ASCII字符");
    
    F_string_copy_cstr(str, "Hello 世界");
    TEST_ASSERT(!F_string_is_ascii(str), "包含中文字符不应只包含ASCII");
    
    // 测试5: 检查是否只包含空白字符
    F_string_copy_cstr(str, "   \t\n\r  ");
    TEST_ASSERT(F_string_is_space(str), "应只包含空白字符");
    
    F_string_copy_cstr(str, "  Hello  ");
    TEST_ASSERT(!F_string_is_space(str), "包含非空白字符不应只包含空白");
    
    // 测试6: 检查是否只包含小写字母
    F_string_copy_cstr(str, "helloworld");
    TEST_ASSERT(F_string_is_lower(str), "应只包含小写字母");
    
    F_string_copy_cstr(str, "HelloWorld");
    TEST_ASSERT(!F_string_is_lower(str), "包含大写字母不应只包含小写");
    
    // 测试7: 检查是否只包含大写字母
    F_string_copy_cstr(str, "HELLOWORLD");
    TEST_ASSERT(F_string_is_upper(str), "应只包含大写字母");
    
    F_string_copy_cstr(str, "HelloWorld");
    TEST_ASSERT(!F_string_is_upper(str), "包含小写字母不应只包含大写");
    
    // 测试8: 空字符串分类
    Stru_String_t* empty = F_string_create_empty();
    TEST_ASSERT(F_string_is_alpha(empty), "空字符串应视为只包含字母");
    TEST_ASSERT(F_string_is_digit(empty), "空字符串应视为只包含数字");
    TEST_ASSERT(F_string_is_alnum(empty), "空字符串应视为只包含字母数字");
    TEST_ASSERT(F_string_is_ascii(empty), "空字符串应视为只包含ASCII");
    TEST_ASSERT(F_string_is_space(empty), "空字符串应视为只包含空白");
    TEST_ASSERT(F_string_is_lower(empty), "空字符串应视为只包含小写");
    TEST_ASSERT(F_string_is_upper(empty), "空字符串应视为只包含大写");
    
    F_string_destroy(str);
    F_string_destroy(empty);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试格式化功能
 * 
 * 测试字符串格式化、解析、转换等功能。
 */
bool test_string_formatting(void)
{
    TEST_START("字符串格式化测试");
    
    // 测试1: 格式化字符串
    Stru_String_t* formatted = F_string_format("Hello %s! The answer is %d.", "World", 42);
    TEST_ASSERT(formatted != NULL, "格式化字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(formatted), "Hello World! The answer is 42.") == 0,
               "格式化后内容不正确");
    
    // 测试2: 格式化到现有字符串
    Stru_String_t* str = F_string_create_empty();
    bool format_to_result = F_string_format_to(str, "Value: %f", 3.14159);
    TEST_ASSERT(format_to_result, "格式化到现有字符串失败");
    TEST_ASSERT(strstr(F_string_cstr(str), "3.14159") != NULL, "格式化后应包含浮点数");
    
    // 测试3: 解析整数
    Stru_String_t* int_str = F_string_create_from_cstr("12345");
    int int_value;
    bool parse_int_result = F_string_to_int(int_str, &int_value, 10);
    TEST_ASSERT(parse_int_result, "解析整数失败");
    TEST_ASSERT(int_value == 12345, "解析的整数值不正确");
    
    // 测试4: 解析十六进制整数
    F_string_copy_cstr(int_str, "FF");
    parse_int_result = F_string_to_int(int_str, &int_value, 16);
    TEST_ASSERT(parse_int_result, "解析十六进制整数失败");
    TEST_ASSERT(int_value == 255, "解析的十六进制整数值不正确");
    
    // 测试5: 解析浮点数
    Stru_String_t* float_str = F_string_create_from_cstr("3.14159");
    double float_value;
    bool parse_float_result = F_string_to_double(float_str, &float_value);
    TEST_ASSERT(parse_float_result, "解析浮点数失败");
    TEST_ASSERT(float_value > 3.14158 && float_value < 3.14160, "解析的浮点数值不正确");
    
    // 测试6: 解析无效数字
    F_string_copy_cstr(int_str, "123abc");
    parse_int_result = F_string_to_int(int_str, &int_value, 10);
    TEST_ASSERT(!parse_int_result, "无效整数解析应失败");
    
    // 清理
    F_string_destroy(formatted);
    F_string_destroy(str);
    F_string_destroy(int_str);
    F_string_destroy(float_str);
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 测试字符串分割和连接功能
 * 
 * 测试字符串分割、连接等功能。
 */
bool test_string_split_and_join(void)
{
    TEST_START("字符串分割和连接测试");
    
    // 测试1: 分割字符串
    Stru_String_t* str = F_string_create_from_cstr("apple,banana,cherry,date");
    size_t count = 0;
    Stru_String_t** parts = F_string_split(str, ',', &count);
    
    TEST_ASSERT(parts != NULL, "分割字符串失败");
    TEST_ASSERT(count == 4, "应分割为4部分");
    
    if (parts != NULL && count == 4) {
        TEST_ASSERT(strcmp(F_string_cstr(parts[0]), "apple") == 0, "第一部分应为apple");
        TEST_ASSERT(strcmp(F_string_cstr(parts[1]), "banana") == 0, "第二部分应为banana");
        TEST_ASSERT(strcmp(F_string_cstr(parts[2]), "cherry") == 0, "第三部分应为cherry");
        TEST_ASSERT(strcmp(F_string_cstr(parts[3]), "date") == 0, "第四部分应为date");
        
        // 释放分割结果
        for (size_t i = 0; i < count; i++) {
            F_string_destroy(parts[i]);
        }
        free(parts);
    }
    
    // 测试2: 分割C字符串
    const char* cstr = "one|two|three";
    count = 0;
    parts = F_cstr_split(cstr, '|', &count);
    
    TEST_ASSERT(parts != NULL, "分割C字符串失败");
    TEST_ASSERT(count == 3, "应分割为3部分");
    
    if (parts != NULL && count == 3) {
        TEST_ASSERT(strcmp(F_string_cstr(parts[0]), "one") == 0, "第一部分应为one");
        TEST_ASSERT(strcmp(F_string_cstr(parts[1]), "two") == 0, "第二部分应为two");
        TEST_ASSERT(strcmp(F_string_cstr(parts[2]), "three") == 0, "第三部分应为three");
        
        // 释放分割结果
        for (size_t i = 0; i < count; i++) {
            F_string_destroy(parts[i]);
        }
        free(parts);
    }
    
    // 测试3: 连接字符串数组
    Stru_String_t* strings[3];
    strings[0] = F_string_create_from_cstr("apple");
    strings[1] = F_string_create_from_cstr("banana");
    strings[2] = F_string_create_from_cstr("cherry");
    
    Stru_String_t* joined = F_string_join(strings, 3, ", ");
    TEST_ASSERT(joined != NULL, "连接字符串失败");
    TEST_ASSERT(strcmp(F_string_cstr(joined), "apple, banana, cherry") == 0,
               "连接后内容不正确");
    
    // 测试4: 连接空数组
    Stru_String_t* empty_joined = F_string_join(NULL, 0, ", ");
    TEST_ASSERT(empty_joined != NULL, "连接空数组失败");
    TEST_ASSERT(F_string_is_empty(empty_joined), "连接空数组结果应为空");
    
    // 清理
    F_string_destroy(str);
    F_string_destroy(joined);
    F_string_destroy(empty_joined);
    for (int i = 0; i < 3; i++) {
        F_string_destroy(strings[i]);
    }
    
    TEST_END();
    TEST_PASS();
    return true;
}

/**
 * @brief 运行所有工具模块测试
 */
bool test_string_utils_all(void)
{
    printf("运行字符串工具模块测试...\n");
    printf("============================\n\n");
    
    bool all_passed = true;
    
    all_passed = test_string_utf8_handling() && all_passed;
    all_passed = test_string_character_classification() && all_passed;
    all_passed = test_string_formatting() && all_passed;
    all_passed = test_string_split_and_join() && all_passed;
    
    printf("\n字符串工具模块测试 %s\n\n", 
           all_passed ? "✅ 全部通过" : "❌ 有失败");
    
    return all_passed;
}
