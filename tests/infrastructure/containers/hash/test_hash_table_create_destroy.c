/**
 * @file test_hash_table_create_destroy.c
 * @brief 哈希表创建和销毁测试实现文件
 * 
 * 实现哈希表创建和销毁功能的测试函数。
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

/**
 * @brief 测试创建和销毁功能
 */
bool test_hash_table_create_and_destroy(void)
{
    printf("测试: 创建和销毁\n");
    printf("----------------\n");
    
    bool all_passed = true;
    
    // 测试1: 基本创建（字符串键，整数值）
    printf("  1. 测试基本创建（字符串键，整数值）...\n");
    Stru_HashTable_t* table = F_create_hash_table(
        sizeof(char[32]),  // 键大小：32字符的字符串
        sizeof(int),       // 值大小：整数
        F_string_hash_function,
        F_string_compare_function,
        16                 // 初始容量
    );
    
    if (table == NULL) {
        printf("    ✗ 基本创建失败: 返回NULL\n");
        all_passed = false;
    } else {
        if (F_hash_table_size(table) != 0) {
            printf("    ✗ 基本创建失败: 初始大小不为0\n");
            all_passed = false;
        }
        if (F_hash_table_capacity(table) < 16) {
            printf("    ✗ 基本创建失败: 容量小于指定值\n");
            all_passed = false;
        }
        if (all_passed) {
            printf("    ✓ 基本创建测试通过\n");
        }
        F_destroy_hash_table(table);
    }
    
    // 测试2: 基本创建（整数键，字符串值）
    printf("  2. 测试基本创建（整数键，字符串值）...\n");
    table = F_create_hash_table(
        sizeof(int),       // 键大小：整数
        sizeof(char[64]),  // 值大小：64字符的字符串
        F_int_hash_function,
        F_int_compare_function,
        32                 // 初始容量
    );
    
    if (table == NULL) {
        printf("    ✗ 整数键创建失败: 返回NULL\n");
        all_passed = false;
    } else {
        if (F_hash_table_capacity(table) < 32) {
            printf("    ✗ 整数键创建失败: 容量小于指定值\n");
            all_passed = false;
        }
        if (all_passed) {
            printf("    ✓ 整数键创建测试通过\n");
        }
        F_destroy_hash_table(table);
    }
    
    // 测试3: 使用默认容量创建
    printf("  3. 测试使用默认容量创建...\n");
    table = F_create_hash_table(
        sizeof(int),
        sizeof(int),
        F_int_hash_function,
        F_int_compare_function,
        0                  // 使用默认容量
    );
    
    if (table == NULL) {
        printf("    ✗ 默认容量创建失败: 返回NULL\n");
        all_passed = false;
    } else {
        if (F_hash_table_capacity(table) == 0) {
            printf("    ✗ 默认容量创建失败: 容量为0\n");
            all_passed = false;
        }
        if (all_passed) {
            printf("    ✓ 默认容量创建测试通过\n");
        }
        F_destroy_hash_table(table);
    }
    
    // 测试4: 无效参数创建
    printf("  4. 测试无效参数创建...\n");
    
    // 测试键大小为0
    table = F_create_hash_table(
        0,                 // 无效键大小
        sizeof(int),
        F_int_hash_function,
        F_int_compare_function,
        16
    );
    if (table != NULL) {
        printf("    ✗ 无效键大小创建失败: 应该返回NULL\n");
        F_destroy_hash_table(table);
        all_passed = false;
    } else {
        printf("    ✓ 无效键大小创建测试通过\n");
    }
    
    // 测试值大小为0
    table = F_create_hash_table(
        sizeof(int),
        0,                 // 无效值大小
        F_int_hash_function,
        F_int_compare_function,
        16
    );
    if (table != NULL) {
        printf("    ✗ 无效值大小创建失败: 应该返回NULL\n");
        F_destroy_hash_table(table);
        all_passed = false;
    } else {
        printf("    ✓ 无效值大小创建测试通过\n");
    }
    
    // 测试哈希函数为NULL
    table = F_create_hash_table(
        sizeof(int),
        sizeof(int),
        NULL,              // 无效哈希函数
        F_int_compare_function,
        16
    );
    if (table != NULL) {
        printf("    ✗ NULL哈希函数创建失败: 应该返回NULL\n");
        F_destroy_hash_table(table);
        all_passed = false;
    } else {
        printf("    ✓ NULL哈希函数创建测试通过\n");
    }
    
    // 测试比较函数为NULL
    table = F_create_hash_table(
        sizeof(int),
        sizeof(int),
        F_int_hash_function,
        NULL,              // 无效比较函数
        16
    );
    if (table != NULL) {
        printf("    ✗ NULL比较函数创建失败: 应该返回NULL\n");
        F_destroy_hash_table(table);
        all_passed = false;
    } else {
        printf("    ✓ NULL比较函数创建测试通过\n");
    }
    
    // 测试5: 销毁NULL指针
    printf("  5. 测试销毁NULL指针...\n");
    F_destroy_hash_table(NULL); // 不应该崩溃
    printf("    ✓ 销毁NULL指针测试通过\n");
    
    // 测试6: 创建后立即销毁（内存泄漏检查）
    printf("  6. 测试创建后立即销毁...\n");
    for (int i = 0; i < 100; i++) {
        table = F_create_hash_table(
            sizeof(int),
            sizeof(int),
            F_int_hash_function,
            F_int_compare_function,
            16
        );
        if (table == NULL) {
            printf("    ✗ 多次创建失败: 第%d次\n", i);
            all_passed = false;
            break;
        }
        F_destroy_hash_table(table);
    }
    if (all_passed) {
        printf("    ✓ 创建后立即销毁测试通过\n");
    }
    
    // 测试7: 使用指针键创建
    printf("  7. 测试使用指针键创建...\n");
    table = F_create_hash_table(
        sizeof(void*),     // 键大小：指针
        sizeof(int),
        F_pointer_hash_function,
        F_pointer_compare_function,
        16
    );
    
    if (table == NULL) {
        printf("    ✗ 指针键创建失败: 返回NULL\n");
        all_passed = false;
    } else {
        // 测试指针键操作
        int key1 = 100;
        int value1 = 42;
        
        if (!F_hash_table_put(table, &key1, &value1)) {
            printf("    ✗ 指针键插入失败\n");
            all_passed = false;
        }
        
        int* retrieved = (int*)F_hash_table_get(table, &key1);
        if (retrieved == NULL || *retrieved != value1) {
            printf("    ✗ 指针键获取失败\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 指针键创建测试通过\n");
        }
        F_destroy_hash_table(table);
    }
    
    printf("创建和销毁测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}
