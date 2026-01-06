/**
 * @file test_hash_table_utils.c
 * @brief 哈希表工具函数测试实现文件
 * 
 * 实现哈希表工具函数功能的测试函数，包括哈希函数、比较函数和工具函数。
 * 遵循模块化测试设计，每个测试功能在单独的文件中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_hash_table.h"
#include "../../../../src/infrastructure/containers/hash/CN_hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

/**
 * @brief 测试工具函数功能
 */
bool test_hash_table_utils_functions(void)
{
    printf("测试: 工具函数\n");
    printf("--------------\n");
    
    bool all_passed = true;
    
    // 测试1: 字符串哈希函数
    printf("  1. 测试字符串哈希函数...\n");
    
    char* test_strings[] = {"hello", "world", "test", "hash", "function"};
    uint64_t previous_hash = 0;
    
    for (int i = 0; i < 5; i++) {
        uint64_t hash = F_string_hash_function(test_strings[i]);
        
        if (hash == 0) {
            printf("    ✗ 字符串哈希函数返回0: %s\n", test_strings[i]);
            all_passed = false;
        }
        
        // 相同字符串应该产生相同哈希
        uint64_t hash2 = F_string_hash_function(test_strings[i]);
        if (hash != hash2) {
            printf("    ✗ 相同字符串哈希不一致: %s\n", test_strings[i]);
            all_passed = false;
        }
        
        // 不同字符串应该产生不同哈希（可能有碰撞，但概率很低）
        if (i > 0 && hash == previous_hash) {
            printf("    ⚠ 不同字符串哈希碰撞: %s 和上一个\n", test_strings[i]);
            // 不是错误，只是警告
        }
        
        previous_hash = hash;
    }
    
    // 测试NULL字符串
    uint64_t null_hash = F_string_hash_function(NULL);
    if (null_hash != 0) {
        printf("    ✗ NULL字符串哈希应该返回0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 字符串哈希函数测试通过\n");
    }
    
    // 测试2: 字符串比较函数
    printf("  2. 测试字符串比较函数...\n");
    
    // 相同字符串
    char str1[] = "hello";
    char str2[] = "hello";
    int cmp_result = F_string_compare_function(str1, str2);
    if (cmp_result != 0) {
        printf("    ✗ 相同字符串比较应该返回0\n");
        all_passed = false;
    }
    
    // 不同字符串
    char str3[] = "hello";
    char str4[] = "world";
    cmp_result = F_string_compare_function(str3, str4);
    if (cmp_result == 0) {
        printf("    ✗ 不同字符串比较应该返回非0\n");
        all_passed = false;
    }
    
    // NULL字符串
    cmp_result = F_string_compare_function(NULL, NULL);
    if (cmp_result != 0) {
        printf("    ✗ 两个NULL字符串比较应该返回0\n");
        all_passed = false;
    }
    
    cmp_result = F_string_compare_function(str1, NULL);
    if (cmp_result == 0) {
        printf("    ✗ 字符串与NULL比较应该返回非0\n");
        all_passed = false;
    }
    
    cmp_result = F_string_compare_function(NULL, str1);
    if (cmp_result == 0) {
        printf("    ✗ NULL与字符串比较应该返回非0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 字符串比较函数测试通过\n");
    }
    
    // 测试3: 整数哈希函数
    printf("  3. 测试整数哈希函数...\n");
    
    int test_ints[] = {0, 1, -1, 100, -100, INT_MAX, INT_MIN};
    previous_hash = 0;
    
    for (int i = 0; i < 7; i++) {
        uint64_t hash = F_int_hash_function(&test_ints[i]);
        
        // 相同整数应该产生相同哈希
        uint64_t hash2 = F_int_hash_function(&test_ints[i]);
        if (hash != hash2) {
            printf("    ✗ 相同整数哈希不一致: %d\n", test_ints[i]);
            all_passed = false;
        }
        
        // 不同整数应该产生不同哈希（可能有碰撞，但概率很低）
        if (i > 0 && hash == previous_hash) {
            printf("    ⚠ 不同整数哈希碰撞: %d 和上一个\n", test_ints[i]);
            // 不是错误，只是警告
        }
        
        previous_hash = hash;
    }
    
    // 测试NULL整数
    null_hash = F_int_hash_function(NULL);
    if (null_hash != 0) {
        printf("    ✗ NULL整数哈希应该返回0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 整数哈希函数测试通过\n");
    }
    
    // 测试4: 整数比较函数
    printf("  4. 测试整数比较函数...\n");
    
    int a = 10, b = 10, c = 20;
    
    // 相等
    cmp_result = F_int_compare_function(&a, &b);
    if (cmp_result != 0) {
        printf("    ✗ 相等整数比较应该返回0\n");
        all_passed = false;
    }
    
    // 小于
    cmp_result = F_int_compare_function(&a, &c);
    if (cmp_result >= 0) {
        printf("    ✗ 小于比较应该返回负数\n");
        all_passed = false;
    }
    
    // 大于
    cmp_result = F_int_compare_function(&c, &a);
    if (cmp_result <= 0) {
        printf("    ✗ 大于比较应该返回正数\n");
        all_passed = false;
    }
    
    // NULL测试
    cmp_result = F_int_compare_function(NULL, NULL);
    if (cmp_result != 0) {
        printf("    ✗ 两个NULL整数比较应该返回0\n");
        all_passed = false;
    }
    
    cmp_result = F_int_compare_function(&a, NULL);
    if (cmp_result == 0) {
        printf("    ✗ 整数与NULL比较应该返回非0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 整数比较函数测试通过\n");
    }
    
    // 测试5: 指针哈希函数
    printf("  5. 测试指针哈希函数...\n");
    
    int x = 10;
    int y = 20;
    void* ptr1 = &x;
    void* ptr2 = &y;
    void* ptr3 = &x; // 与ptr1相同
    
    uint64_t ptr_hash1 = F_pointer_hash_function(ptr1);
    uint64_t ptr_hash2 = F_pointer_hash_function(ptr2);
    uint64_t ptr_hash3 = F_pointer_hash_function(ptr3);
    
    if (ptr_hash1 == 0) {
        printf("    ✗ 指针哈希不应该返回0\n");
        all_passed = false;
    }
    
    // 相同指针应该产生相同哈希
    if (ptr_hash1 != ptr_hash3) {
        printf("    ✗ 相同指针哈希不一致\n");
        all_passed = false;
    }
    
    // 不同指针应该产生不同哈希（可能有碰撞，但概率很低）
    if (ptr_hash1 == ptr_hash2) {
        printf("    ⚠ 不同指针哈希碰撞\n");
        // 不是错误，只是警告
    }
    
    // NULL指针
    null_hash = F_pointer_hash_function(NULL);
    if (null_hash != 0) {
        printf("    ✗ NULL指针哈希应该返回0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 指针哈希函数测试通过\n");
    }
    
    // 测试6: 指针比较函数
    printf("  6. 测试指针比较函数...\n");
    
    // 相同指针
    cmp_result = F_pointer_compare_function(ptr1, ptr3);
    if (cmp_result != 0) {
        printf("    ✗ 相同指针比较应该返回0\n");
        all_passed = false;
    }
    
    // 不同指针
    cmp_result = F_pointer_compare_function(ptr1, ptr2);
    if (cmp_result == 0) {
        printf("    ✗ 不同指针比较应该返回非0\n");
        all_passed = false;
    }
    
    // NULL指针
    cmp_result = F_pointer_compare_function(NULL, NULL);
    if (cmp_result != 0) {
        printf("    ✗ 两个NULL指针比较应该返回0\n");
        all_passed = false;
    }
    
    cmp_result = F_pointer_compare_function(ptr1, NULL);
    if (cmp_result == 0) {
        printf("    ✗ 指针与NULL比较应该返回非0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 指针比较函数测试通过\n");
    }
    
    // 测试7: 下一个2的幂函数
    printf("  7. 测试下一个2的幂函数...\n");
    
    struct {
        size_t input;
        size_t expected;
    } power_tests[] = {
        {0, 1},
        {1, 1},
        {2, 2},
        {3, 4},
        {5, 8},
        {7, 8},
        {8, 8},
        {9, 16},
        {15, 16},
        {16, 16},
        {100, 128},
        {255, 256},
        {256, 256},
        {1000, 1024}
    };
    
    for (size_t i = 0; i < sizeof(power_tests)/sizeof(power_tests[0]); i++) {
        size_t result = F_next_power_of_two(power_tests[i].input);
        if (result != power_tests[i].expected) {
            printf("    ✗ 下一个2的幂错误: %zu -> %zu (期望: %zu)\n", 
                   power_tests[i].input, result, power_tests[i].expected);
            all_passed = false;
        }
    }
    
    if (all_passed) {
        printf("    ✓ 下一个2的幂函数测试通过\n");
    }
    
    // 测试8: 质数容量函数
    printf("  8. 测试质数容量函数...\n");
    
    struct {
        size_t input;
        size_t min_expected;
    } prime_tests[] = {
        {0, 53},
        {1, 53},
        {50, 53},
        {53, 53},
        {54, 97},
        {100, 193},
        {200, 389},
        {500, 769},
        {1000, 1543}
    };
    
    for (size_t i = 0; i < sizeof(prime_tests)/sizeof(prime_tests[0]); i++) {
        size_t result = F_prime_capacity(prime_tests[i].input);
        if (result < prime_tests[i].min_expected) {
            printf("    ✗ 质数容量错误: %zu -> %zu (至少期望: %zu)\n", 
                   prime_tests[i].input, result, prime_tests[i].min_expected);
            all_passed = false;
        }
    }
    
    if (all_passed) {
        printf("    ✓ 质数容量函数测试通过\n");
    }
    
    // 测试9: 混合哈希函数
    printf("  9. 测试混合哈希函数...\n");
    
    uint64_t h1 = 123456789;
    uint64_t h2 = 987654321;
    uint64_t h3 = 123456789; // 与h1相同
    
    uint64_t mixed1 = F_mix_hash(h1, h2);
    uint64_t mixed2 = F_mix_hash(h1, h3);
    uint64_t mixed3 = F_mix_hash(h2, h1);
    
    if (mixed1 == 0) {
        printf("    ✗ 混合哈希不应该返回0\n");
        all_passed = false;
    }
    
    // 相同输入应该产生相同输出
    uint64_t mixed1_again = F_mix_hash(h1, h2);
    if (mixed1 != mixed1_again) {
        printf("    ✗ 相同输入混合哈希不一致\n");
        all_passed = false;
    }
    
    // 不同输入应该产生不同输出
    if (mixed1 == mixed2) {
        printf("    ⚠ 不同输入混合哈希碰撞 (h1,h2) vs (h1,h3)\n");
        // 不是错误，只是警告
    }
    
    // 交换参数应该产生不同结果
    if (mixed1 == mixed3) {
        printf("    ⚠ 交换参数混合哈希相同\n");
        // 不是错误，只是警告
    }
    
    if (all_passed) {
        printf("    ✓ 混合哈希函数测试通过\n");
    }
    
    // 测试10: 字节数组哈希函数
    printf("  10. 测试字节数组哈希函数...\n");
    
    char byte_data1[] = "Hello, World!";
    char byte_data2[] = "Hello, World!";
    char byte_data3[] = "Hello, world!"; // 不同大小写
    
    uint64_t byte_hash1 = F_bytes_hash_function(byte_data1, strlen(byte_data1));
    uint64_t byte_hash2 = F_bytes_hash_function(byte_data2, strlen(byte_data2));
    uint64_t byte_hash3 = F_bytes_hash_function(byte_data3, strlen(byte_data3));
    
    if (byte_hash1 == 0) {
        printf("    ✗ 字节数组哈希不应该返回0\n");
        all_passed = false;
    }
    
    // 相同数据应该产生相同哈希
    if (byte_hash1 != byte_hash2) {
        printf("    ✗ 相同字节数组哈希不一致\n");
        all_passed = false;
    }
    
    // 不同数据应该产生不同哈希
    if (byte_hash1 == byte_hash3) {
        printf("    ⚠ 不同字节数组哈希碰撞\n");
        // 不是错误，只是警告
    }
    
    // NULL数据或零长度
    uint64_t null_byte_hash = F_bytes_hash_function(NULL, 10);
    if (null_byte_hash != 0) {
        printf("    ✗ NULL字节数组哈希应该返回0\n");
        all_passed = false;
    }
    
    null_byte_hash = F_bytes_hash_function(byte_data1, 0);
    if (null_byte_hash != 0) {
        printf("    ✗ 零长度字节数组哈希应该返回0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 字节数组哈希函数测试通过\n");
    }
    
    printf("工具函数测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}
