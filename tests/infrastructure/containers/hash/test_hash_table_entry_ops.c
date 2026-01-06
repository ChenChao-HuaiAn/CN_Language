/**
 * @file test_hash_table_entry_ops.c
 * @brief 哈希表条目操作测试实现文件
 * 
 * 实现哈希表条目操作功能的测试函数，包括条目的创建、销毁、复制和比较。
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
 * @brief 测试条目操作功能
 */
bool test_hash_table_entry_operations(void)
{
    printf("测试: 条目操作\n");
    printf("--------------\n");
    
    bool all_passed = true;
    
    // 测试1: 创建条目（字符串键，整数值）
    printf("  1. 测试创建条目（字符串键，整数值）...\n");
    char key1[] = "test_key";
    int value1 = 42;
    
    Stru_HashEntry_t* entry = F_create_hash_entry(key1, &value1, 
                                                 sizeof(char[32]), sizeof(int));
    
    if (entry == NULL) {
        printf("    ✗ 创建条目失败\n");
        all_passed = false;
    } else {
        // 验证键
        char* retrieved_key = (char*)F_get_hash_entry_key(entry);
        if (retrieved_key == NULL || strcmp(retrieved_key, key1) != 0) {
            printf("    ✗ 条目键不正确\n");
            all_passed = false;
        }
        
        // 验证值
        int* retrieved_value = (int*)F_get_hash_entry_value(entry);
        if (retrieved_value == NULL || *retrieved_value != value1) {
            printf("    ✗ 条目值不正确\n");
            all_passed = false;
        }
        
        // 验证下一个指针
        Stru_HashEntry_t* next = F_get_hash_entry_next(entry);
        if (next != NULL) {
            printf("    ✗ 新条目的下一个指针应该为NULL\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 创建条目测试通过\n");
        }
        
        F_free_hash_entry(entry);
    }
    
    // 测试2: 创建条目（整数键，字符串值）
    printf("  2. 测试创建条目（整数键，字符串值）...\n");
    int key2 = 12345;
    char value2[] = "test_value";
    
    entry = F_create_hash_entry(&key2, value2, sizeof(int), sizeof(char[64]));
    
    if (entry == NULL) {
        printf("    ✗ 创建整数键条目失败\n");
        all_passed = false;
    } else {
        int* retrieved_int_key = (int*)F_get_hash_entry_key(entry);
        if (retrieved_int_key == NULL || *retrieved_int_key != key2) {
            printf("    ✗ 整数键条目键不正确\n");
            all_passed = false;
        }
        
        char* retrieved_str_value = (char*)F_get_hash_entry_value(entry);
        if (retrieved_str_value == NULL || strcmp(retrieved_str_value, value2) != 0) {
            printf("    ✗ 整数键条目值不正确\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 创建整数键条目测试通过\n");
        }
        
        F_free_hash_entry(entry);
    }
    
    // 测试3: 创建条目（NULL键或值）
    printf("  3. 测试创建条目（NULL键或值）...\n");
    
    // NULL键
    entry = F_create_hash_entry(NULL, &value1, sizeof(char[32]), sizeof(int));
    if (entry != NULL) {
        printf("    ✗ 创建NULL键条目应该失败\n");
        F_free_hash_entry(entry);
        all_passed = false;
    } else {
        printf("    ✓ NULL键条目创建测试通过\n");
    }
    
    // NULL值
    entry = F_create_hash_entry(key1, NULL, sizeof(char[32]), sizeof(int));
    if (entry != NULL) {
        printf("    ✗ 创建NULL值条目应该失败\n");
        F_free_hash_entry(entry);
        all_passed = false;
    } else {
        printf("    ✓ NULL值条目创建测试通过\n");
    }
    
    // 测试4: 复制条目
    printf("  4. 测试复制条目...\n");
    
    // 先创建一个源条目
    char src_key[] = "source_key";
    int src_value = 999;
    Stru_HashEntry_t* src_entry = F_create_hash_entry(src_key, &src_value, 
                                                     sizeof(char[32]), sizeof(int));
    
    if (src_entry == NULL) {
        printf("    ✗ 无法创建源条目\n");
        all_passed = false;
    } else {
        // 复制条目
        Stru_HashEntry_t* copied_entry = F_copy_hash_entry(src_entry, 
                                                          sizeof(char[32]), sizeof(int));
        
        if (copied_entry == NULL) {
            printf("    ✗ 复制条目失败\n");
            all_passed = false;
        } else {
            // 验证复制的条目
            char* copied_key = (char*)F_get_hash_entry_key(copied_entry);
            int* copied_value = (int*)F_get_hash_entry_value(copied_entry);
            
            if (copied_key == NULL || strcmp(copied_key, src_key) != 0) {
                printf("    ✗ 复制条目键不正确\n");
                all_passed = false;
            }
            
            if (copied_value == NULL || *copied_value != src_value) {
                printf("    ✗ 复制条目值不正确\n");
                all_passed = false;
            }
            
            // 验证是深拷贝（指针应该不同）
            if (copied_key == (char*)F_get_hash_entry_key(src_entry)) {
                printf("    ✗ 复制应该是深拷贝（键指针相同）\n");
                all_passed = false;
            }
            
            if (copied_value == (int*)F_get_hash_entry_value(src_entry)) {
                printf("    ✗ 复制应该是深拷贝（值指针相同）\n");
                all_passed = false;
            }
            
            if (all_passed) {
                printf("    ✓ 复制条目测试通过\n");
            }
            
            F_free_hash_entry(copied_entry);
        }
        
        F_free_hash_entry(src_entry);
    }
    
    // 测试5: 复制NULL条目
    printf("  5. 测试复制NULL条目...\n");
    Stru_HashEntry_t* null_copied = F_copy_hash_entry(NULL, sizeof(char[32]), sizeof(int));
    if (null_copied != NULL) {
        printf("    ✗ 复制NULL条目应该返回NULL\n");
        F_free_hash_entry(null_copied);
        all_passed = false;
    } else {
        printf("    ✓ 复制NULL条目测试通过\n");
    }
    
    // 测试6: 比较条目键
    printf("  6. 测试比较条目键...\n");
    
    // 创建两个相同键的条目
    char same_key[] = "same_key";
    int value_a = 100;
    int value_b = 200;
    
    Stru_HashEntry_t* entry_a = F_create_hash_entry(same_key, &value_a, 
                                                   sizeof(char[32]), sizeof(int));
    Stru_HashEntry_t* entry_b = F_create_hash_entry(same_key, &value_b, 
                                                   sizeof(char[32]), sizeof(int));
    
    if (entry_a == NULL || entry_b == NULL) {
        printf("    ✗ 无法创建比较测试条目\n");
        all_passed = false;
        if (entry_a) F_free_hash_entry(entry_a);
        if (entry_b) F_free_hash_entry(entry_b);
    } else {
        // 比较相同键
        bool keys_equal = F_compare_hash_entry_keys(entry_a, entry_b, F_string_compare_function);
        if (!keys_equal) {
            printf("    ✗ 相同键比较应该返回true\n");
            all_passed = false;
        }
        
        // 创建不同键的条目
        char different_key[] = "different_key";
        Stru_HashEntry_t* entry_c = F_create_hash_entry(different_key, &value_a, 
                                                       sizeof(char[32]), sizeof(int));
        
        if (entry_c == NULL) {
            printf("    ✗ 无法创建不同键条目\n");
            all_passed = false;
        } else {
            // 比较不同键
            keys_equal = F_compare_hash_entry_keys(entry_a, entry_c, F_string_compare_function);
            if (keys_equal) {
                printf("    ✗ 不同键比较应该返回false\n");
                all_passed = false;
            }
            
            F_free_hash_entry(entry_c);
        }
        
        if (all_passed) {
            printf("    ✓ 比较条目键测试通过\n");
        }
        
        F_free_hash_entry(entry_a);
        F_free_hash_entry(entry_b);
    }
    
    // 测试7: 比较NULL条目
    printf("  7. 测试比较NULL条目...\n");
    
    // 创建一个有效条目用于比较
    char test_key[] = "test";
    int test_value = 1;
    Stru_HashEntry_t* test_entry = F_create_hash_entry(test_key, &test_value, 
                                                      sizeof(char[32]), sizeof(int));
    
    if (test_entry == NULL) {
        printf("    ✗ 无法创建测试条目\n");
        all_passed = false;
    } else {
        // 比较NULL条目
        bool result = F_compare_hash_entry_keys(NULL, test_entry, F_string_compare_function);
        if (result) {
            printf("    ✗ 比较NULL条目应该返回false\n");
            all_passed = false;
        }
        
        result = F_compare_hash_entry_keys(test_entry, NULL, F_string_compare_function);
        if (result) {
            printf("    ✗ 比较条目与NULL应该返回false\n");
            all_passed = false;
        }
        
        result = F_compare_hash_entry_keys(NULL, NULL, F_string_compare_function);
        if (result) {
            printf("    ✗ 比较两个NULL条目应该返回false\n");
            all_passed = false;
        }
        
        // 测试NULL比较函数
        result = F_compare_hash_entry_keys(test_entry, test_entry, NULL);
        if (result) {
            printf("    ✗ 使用NULL比较函数应该返回false\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 比较NULL条目测试通过\n");
        }
        
        F_free_hash_entry(test_entry);
    }
    
    // 测试8: 设置条目值
    printf("  8. 测试设置条目值...\n");
    
    char set_key[] = "set_key";
    int original_value = 111;
    int new_value = 222;
    
    Stru_HashEntry_t* set_entry = F_create_hash_entry(set_key, &original_value, 
                                                     sizeof(char[32]), sizeof(int));
    
    if (set_entry == NULL) {
        printf("    ✗ 无法创建设置测试条目\n");
        all_passed = false;
    } else {
        // 获取原始值指针
        int* original_ptr = (int*)F_get_hash_entry_value(set_entry);
        
        // 设置新值
        bool set_result = F_set_hash_entry_value(set_entry, &new_value, sizeof(int));
        if (!set_result) {
            printf("    ✗ 设置条目值失败\n");
            all_passed = false;
        } else {
            // 验证新值
            int* updated_value = (int*)F_get_hash_entry_value(set_entry);
            if (updated_value == NULL || *updated_value != new_value) {
                printf("    ✗ 设置后值不正确\n");
                all_passed = false;
            }
            
            // 验证指针可能已改变（深拷贝）
            if (updated_value == original_ptr) {
                printf("    ⚠ 设置值后指针未改变（可能是原地更新）\n");
            }
        }
        
        // 测试设置NULL值
        set_result = F_set_hash_entry_value(set_entry, NULL, sizeof(int));
        if (set_result) {
            printf("    ✗ 设置NULL值应该失败\n");
            all_passed = false;
        }
        
        // 测试在NULL条目上设置值
        set_result = F_set_hash_entry_value(NULL, &new_value, sizeof(int));
        if (set_result) {
            printf("    ✗ 在NULL条目上设置值应该失败\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 设置条目值测试通过\n");
        }
        
        F_free_hash_entry(set_entry);
    }
    
    // 测试9: 设置条目下一个指针
    printf("  9. 测试设置条目下一个指针...\n");
    
    // 创建两个条目
    char key_a[] = "key_a";
    char key_b[] = "key_b";
    int val_a = 1, val_b = 2;
    
    Stru_HashEntry_t* entry1 = F_create_hash_entry(key_a, &val_a, sizeof(char[32]), sizeof(int));
    Stru_HashEntry_t* entry2 = F_create_hash_entry(key_b, &val_b, sizeof(char[32]), sizeof(int));
    
    if (entry1 == NULL || entry2 == NULL) {
        printf("    ✗ 无法创建链表测试条目\n");
        all_passed = false;
        if (entry1) F_free_hash_entry(entry1);
        if (entry2) F_free_hash_entry(entry2);
    } else {
        // 设置下一个指针
        F_set_hash_entry_next(entry1, entry2);
        
        // 验证下一个指针
        Stru_HashEntry_t* next_entry = F_get_hash_entry_next(entry1);
        if (next_entry != entry2) {
            printf("    ✗ 设置下一个指针失败\n");
            all_passed = false;
        }
        
        // 验证entry2的下一个指针应该为NULL
        next_entry = F_get_hash_entry_next(entry2);
        if (next_entry != NULL) {
            printf("    ✗ 新条目的下一个指针应该为NULL\n");
            all_passed = false;
        }
        
        // 测试在NULL条目上设置下一个指针
        F_set_hash_entry_next(NULL, entry2); // 不应该崩溃
        
        // 测试设置NULL下一个指针
        F_set_hash_entry_next(entry1, NULL);
        next_entry = F_get_hash_entry_next(entry1);
        if (next_entry != NULL) {
            printf("    ✗ 设置NULL下一个指针失败\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 设置条目下一个指针测试通过\n");
        }
        
        F_free_hash_entry(entry1);
        F_free_hash_entry(entry2);
    }
    
    // 测试10: 获取NULL条目的键/值/下一个指针
    printf("  10. 测试获取NULL条目的键/值/下一个指针...\n");
    
    if (F_get_hash_entry_key(NULL) != NULL) {
        printf("    ✗ 获取NULL条目键应该返回NULL\n");
        all_passed = false;
    }
    
    if (F_get_hash_entry_value(NULL) != NULL) {
        printf("    ✗ 获取NULL条目值应该返回NULL\n");
        all_passed = false;
    }
    
    if (F_get_hash_entry_next(NULL) != NULL) {
        printf("    ✗ 获取NULL条目下一个指针应该返回NULL\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 获取NULL条目属性测试通过\n");
    }
    
    // 测试11: 释放NULL条目
    printf("  11. 测试释放NULL条目...\n");
    F_free_hash_entry(NULL); // 不应该崩溃
    printf("    ✓ 释放NULL条目测试通过\n");
    
    // 测试12: 内存泄漏检查（多次创建和释放）
    printf("  12. 测试内存泄漏检查...\n");
    for (int i = 0; i < 100; i++) {
        char temp_key[32];
        sprintf(temp_key, "temp_key_%d", i);
        int temp_value = i * 10;
        
        Stru_HashEntry_t* temp_entry = F_create_hash_entry(temp_key, &temp_value, 
                                                          sizeof(char[32]), sizeof(int));
        if (temp_entry == NULL) {
            printf("    ✗ 多次创建条目失败: 第%d次\n", i);
            all_passed = false;
            break;
        }
        
        // 进行一些操作
        int* val = (int*)F_get_hash_entry_value(temp_entry);
        if (val == NULL || *val != temp_value) {
            printf("    ✗ 获取条目值失败: 第%d次\n", i);
            F_free_hash_entry(temp_entry);
            all_passed = false;
            break;
        }
        
        F_free_hash_entry(temp_entry);
    }
    
    if (all_passed) {
        printf("    ✓ 内存泄漏检查测试通过\n");
    }
    
    printf("条目操作测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}
