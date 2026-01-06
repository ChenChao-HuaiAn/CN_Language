/**
 * @file test_hash_table_error_handling.c
 * @brief 哈希表错误处理测试
 * 
 * 测试哈希表的错误处理功能，包括边界条件、无效输入和错误恢复。
 * 验证错误码的正确返回和错误情况的正确处理。
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
#include <string.h>
#include <stdlib.h>

/**
 * @brief 测试NULL指针错误处理
 * @return 测试通过返回true，失败返回false
 */
static bool test_null_pointer_handling(void)
{
    printf("  [测试] NULL指针错误处理...\n");
    
    // 测试NULL哈希表指针
    Stru_HashTable_t* table = NULL;
    
    // 所有函数在接收NULL指针时应安全处理
    F_destroy_hash_table(table); // 应不崩溃
    
    void* value = F_hash_table_get(table, "key");
    if (value != NULL) {
        printf("    [失败] NULL表获取应返回NULL\n");
        return false;
    }
    
    bool result = F_hash_table_put(table, "key", "value");
    if (result) {
        printf("    [失败] NULL表插入应返回false\n");
        return false;
    }
    
    result = F_hash_table_remove(table, "key");
    if (result) {
        printf("    [失败] NULL表删除应返回false\n");
        return false;
    }
    
    result = F_hash_table_contains(table, "key");
    if (result) {
        printf("    [失败] NULL表包含检查应返回false\n");
        return false;
    }
    
    size_t size = F_hash_table_size(table);
    if (size != 0) {
        printf("    [失败] NULL表大小应返回0\n");
        return false;
    }
    
    size_t capacity = F_hash_table_capacity(table);
    if (capacity != 0) {
        printf("    [失败] NULL表容量应返回0\n");
        return false;
    }
    
    printf("    [通过] NULL指针错误处理测试\n");
    return true;
}

/**
 * @brief 测试无效参数错误处理
 * @return 测试通过返回true，失败返回false
 */
static bool test_invalid_parameter_handling(void)
{
    printf("  [测试] 无效参数错误处理...\n");
    
    // 创建有效的哈希表
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 8);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    // 测试NULL键
    bool result = F_hash_table_put(table, NULL, "value");
    if (result) {
        printf("    [失败] NULL键插入应返回false\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试NULL值
    const char* key = "valid_key";
    result = F_hash_table_put(table, (void*)&key, NULL);
    if (result) {
        printf("    [失败] NULL值插入应返回false\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试NULL键获取
    void* retrieved = F_hash_table_get(table, NULL);
    if (retrieved != NULL) {
        printf("    [失败] NULL键获取应返回NULL\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试NULL键删除
    result = F_hash_table_remove(table, NULL);
    if (result) {
        printf("    [失败] NULL键删除应返回false\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试NULL键包含检查
    result = F_hash_table_contains(table, NULL);
    if (result) {
        printf("    [失败] NULL键包含检查应返回false\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 无效参数错误处理测试\n");
    return true;
}

/**
 * @brief 测试内存分配错误处理
 * @return 测试通过返回true，失败返回false
 */
static bool test_memory_allocation_handling(void)
{
    printf("  [测试] 内存分配错误处理...\n");
    
    // 测试超大容量（可能触发内存分配失败）
    // 注意：实际测试中可能不会失败，取决于系统内存
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 SIZE_MAX); // 极大容量
    if (table != NULL) {
        // 如果分配成功，正常清理
        F_destroy_hash_table(table);
        printf("    [注意] 超大容量分配成功（系统内存充足）\n");
    } else {
        printf("    [注意] 超大容量分配失败（预期行为）\n");
    }
    
    // 测试零容量
    table = F_create_hash_table(sizeof(char*), sizeof(int),
                               F_string_hash_function,
                               F_string_compare_function,
                               0);
    if (table == NULL) {
        printf("    [失败] 零容量创建应使用默认容量\n");
        return false;
    }
    
    // 验证使用了默认容量
    size_t capacity = F_hash_table_capacity(table);
    if (capacity == 0) {
        printf("    [失败] 零容量创建后容量不应为0\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 内存分配错误处理测试\n");
    return true;
}

/**
 * @brief 测试键值大小错误处理
 * @return 测试通过返回true，失败返回false
 */
static bool test_key_value_size_handling(void)
{
    printf("  [测试] 键值大小错误处理...\n");
    
    // 测试零键大小
    Stru_HashTable_t* table = F_create_hash_table(0, sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 8);
    if (table != NULL) {
        printf("    [失败] 零键大小创建应失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试零值大小
    table = F_create_hash_table(sizeof(char*), 0,
                               F_string_hash_function,
                               F_string_compare_function,
                               8);
    if (table != NULL) {
        printf("    [失败] 零值大小创建应失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试超大键大小
    table = F_create_hash_table(SIZE_MAX, sizeof(int),
                               F_string_hash_function,
                               F_string_compare_function,
                               8);
    if (table != NULL) {
        F_destroy_hash_table(table);
        printf("    [注意] 超大键大小分配成功\n");
    } else {
        printf("    [注意] 超大键大小分配失败（预期行为）\n");
    }
    
    // 测试超大值大小
    table = F_create_hash_table(sizeof(char*), SIZE_MAX,
                               F_string_hash_function,
                               F_string_compare_function,
                               8);
    if (table != NULL) {
        F_destroy_hash_table(table);
        printf("    [注意] 超大值大小分配成功\n");
    } else {
        printf("    [注意] 超大值大小分配失败（预期行为）\n");
    }
    
    printf("    [通过] 键值大小错误处理测试\n");
    return true;
}

/**
 * @brief 测试哈希函数错误处理
 * @return 测试通过返回true，失败返回false
 */
static bool test_hash_function_handling(void)
{
    printf("  [测试] 哈希函数错误处理...\n");
    
    // 测试NULL哈希函数
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 NULL,
                                                 F_string_compare_function,
                                                 8);
    if (table != NULL) {
        printf("    [失败] NULL哈希函数创建应失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试NULL比较函数
    table = F_create_hash_table(sizeof(char*), sizeof(int),
                               F_string_hash_function,
                               NULL,
                               8);
    if (table != NULL) {
        printf("    [失败] NULL比较函数创建应失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    printf("    [通过] 哈希函数错误处理测试\n");
    return true;
}

/**
 * @brief 测试操作顺序错误处理
 * @return 测试通过返回true，失败返回false
 */
static bool test_operation_order_handling(void)
{
    printf("  [测试] 操作顺序错误处理...\n");
    
    // 创建哈希表
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 8);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    // 测试重复插入相同键
    const char* key = "test_key";
    int value1 = 100;
    int value2 = 200;
    
    bool result = F_hash_table_put(table, (void*)&key, &value1);
    if (!result) {
        printf("    [失败] 第一次插入失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 重复插入相同键（应更新值）
    result = F_hash_table_put(table, (void*)&key, &value2);
    if (!result) {
        printf("    [失败] 重复插入失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 验证值已更新
    int* retrieved = (int*)F_hash_table_get(table, (void*)&key);
    if (retrieved == NULL || *retrieved != value2) {
        printf("    [失败] 值未正确更新\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试删除不存在的键
    const char* non_existent_key = "non_existent";
    result = F_hash_table_remove(table, (void*)&non_existent_key);
    if (result) {
        printf("    [失败] 删除不存在的键应返回false\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试获取不存在的键
    retrieved = (int*)F_hash_table_get(table, (void*)&non_existent_key);
    if (retrieved != NULL) {
        printf("    [失败] 获取不存在的键应返回NULL\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试清空后操作
    F_hash_table_clear(table);
    
    size_t size = F_hash_table_size(table);
    if (size != 0) {
        printf("    [失败] 清空后大小应为0\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 清空后获取键应返回NULL
    retrieved = (int*)F_hash_table_get(table, (void*)&key);
    if (retrieved != NULL) {
        printf("    [失败] 清空后获取应返回NULL\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 操作顺序错误处理测试\n");
    return true;
}

/**
 * @brief 测试调整容量错误处理
 * @return 测试通过返回true，失败返回false
 */
static bool test_resize_error_handling(void)
{
    printf("  [测试] 调整容量错误处理...\n");
    
    // 创建哈希表
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 8);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    // 测试调整到零容量
    bool result = F_hash_table_resize(table, 0);
    if (result) {
        printf("    [失败] 调整到零容量应失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试调整到当前容量
    size_t current_capacity = F_hash_table_capacity(table);
    result = F_hash_table_resize(table, current_capacity);
    if (!result) {
        printf("    [失败] 调整到当前容量应成功\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试调整到较小容量（但大于0）
    if (current_capacity > 4) {
        result = F_hash_table_resize(table, 4);
        if (!result) {
            printf("    [失败] 调整到较小容量应成功\n");
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 测试调整到较大容量
    result = F_hash_table_resize(table, 32);
    if (!result) {
        printf("    [失败] 调整到较大容量应成功\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试超大容量
    result = F_hash_table_resize(table, SIZE_MAX);
    if (result) {
        // 如果成功，验证容量正确
        size_t new_capacity = F_hash_table_capacity(table);
        printf("    [注意] 超大容量调整成功，新容量: %zu\n", new_capacity);
    } else {
        printf("    [注意] 超大容量调整失败（预期行为）\n");
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 调整容量错误处理测试\n");
    return true;
}

/**
 * @brief 测试错误恢复能力
 * @return 测试通过返回true，失败返回false
 */
static bool test_error_recovery(void)
{
    printf("  [测试] 错误恢复能力...\n");
    
    // 创建哈希表
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 8);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    // 插入一些数据
    const char* keys[] = {"key1", "key2", "key3"};
    int values[] = {10, 20, 30};
    int count = sizeof(keys) / sizeof(keys[0]);
    
    for (int i = 0; i < count; i++) {
        if (!F_hash_table_put(table, (void*)&keys[i], &values[i])) {
            printf("    [失败] 插入数据失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 执行一系列可能失败的操作后验证数据完整性
    
    // 1. 尝试插入NULL键（应失败但不影响现有数据）
    bool result = F_hash_table_put(table, NULL, &values[0]);
    if (result) {
        printf("    [失败] NULL键插入应失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 验证现有数据仍然存在
    for (int i = 0; i < count; i++) {
        int* retrieved = (int*)F_hash_table_get(table, (void*)&keys[i]);
        if (retrieved == NULL || *retrieved != values[i]) {
            printf("    [失败] 错误操作后数据损坏 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 2. 尝试调整到无效容量（应失败但不影响现有数据）
    result = F_hash_table_resize(table, 0);
    if (result) {
        printf("    [失败] 调整到零容量应失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 验证现有数据仍然存在
    for (int i = 0; i < count; i++) {
        int* retrieved = (int*)F_hash_table_get(table, (void*)&keys[i]);
        if (retrieved == NULL || *retrieved != values[i]) {
            printf("    [失败] 错误调整后数据损坏 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 3. 执行正常操作验证功能正常
    const char* new_key = "new_key";
    int new_value = 999;
    result = F_hash_table_put(table, (void*)&new_key, &new_value);
    if (!result) {
        printf("    [失败] 错误恢复后正常操作失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 验证新数据
    int* retrieved = (int*)F_hash_table_get(table, (void*)&new_key);
    if (retrieved == NULL || *retrieved != new_value) {
        printf("    [失败] 新数据验证失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 错误恢复能力测试\n");
    return true;
}

/**
 * @brief 测试错误处理功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_error_handling(void)
{
    printf("\n[测试套件] 哈希表错误处理功能测试\n");
    printf("========================================\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行所有错误处理测试
    total++; if (test_null_pointer_handling()) passed++;
    total++; if (test_invalid_parameter_handling()) passed++;
    total++; if (test_memory_allocation_handling()) passed++;
    total++; if (test_key_value_size_handling()) passed++;
    total++; if (test_hash_function_handling()) passed++;
    total++; if (test_operation_order_handling()) passed++;
    total++; if (test_resize_error_handling()) passed++;
    total++; if (test_error_recovery()) passed++;
    
    printf("========================================\n");
    printf("[结果] 错误处理测试: %d/%d 通过\n\n", passed, total);
    
    return passed == total;
}
