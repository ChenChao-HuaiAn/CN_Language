/**
 * @file test_hash_table_interface.c
 * @brief 哈希表抽象接口测试
 * 
 * 测试哈希表的抽象接口功能，包括接口创建、配置、使用和销毁。
 * 验证接口模式在哈希表模块中的正确实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_hash_table.h"
#include "../../../../src/infrastructure/containers/hash/CN_hash_table_interface.h"
#include "../../../../src/infrastructure/containers/hash/CN_hash_table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief 测试接口获取和验证
 * @return 测试通过返回true，失败返回false
 */
static bool test_interface_get_and_validate(void)
{
    printf("  [测试] 接口获取和验证...\n");
    
    // 获取默认接口
    const Stru_HashTableInterface_t* interface = F_get_default_hash_table_interface();
    if (interface == NULL) {
        printf("    [失败] 获取默认接口失败\n");
        return false;
    }
    
    // 验证接口函数指针不为NULL
    if (interface->create == NULL || 
        interface->destroy == NULL || 
        interface->put == NULL || 
        interface->get == NULL || 
        interface->remove == NULL) {
        printf("    [失败] 接口函数指针为NULL\n");
        return false;
    }
    
    printf("    [通过] 接口获取和验证测试\n");
    return true;
}

/**
 * @brief 测试接口创建和销毁
 * @return 测试通过返回true，失败返回false
 */
static bool test_interface_create_and_destroy(void)
{
    printf("  [测试] 接口创建和销毁...\n");
    
    // 获取默认接口
    const Stru_HashTableInterface_t* interface = F_get_default_hash_table_interface();
    if (interface == NULL) {
        printf("    [失败] 获取默认接口失败\n");
        return false;
    }
    
    // 创建哈希表
    enum Eum_HashTableError error_code;
    void* table = interface->create(sizeof(char*), sizeof(int), 
                                   F_string_hash_function, 
                                   F_string_compare_function,
                                   16, &error_code);
    if (table == NULL) {
        printf("    [失败] 哈希表创建失败，错误码: %d\n", error_code);
        return false;
    }
    
    // 验证哈希表不为NULL
    if (table == NULL) {
        printf("    [失败] 创建的哈希表为NULL\n");
        return false;
    }
    
    // 销毁哈希表
    interface->destroy(table, &error_code);
    if (error_code != Eum_HASH_TABLE_SUCCESS) {
        printf("    [失败] 哈希表销毁失败，错误码: %d\n", error_code);
        return false;
    }
    
    printf("    [通过] 接口创建和销毁测试\n");
    return true;
}

/**
 * @brief 测试接口基本操作
 * @return 测试通过返回true，失败返回false
 */
static bool test_interface_basic_operations(void)
{
    printf("  [测试] 接口基本操作...\n");
    
    // 获取默认接口
    const Stru_HashTableInterface_t* interface = F_get_default_hash_table_interface();
    if (interface == NULL) {
        printf("    [失败] 获取默认接口失败\n");
        return false;
    }
    
    // 创建哈希表
    enum Eum_HashTableError error_code;
    void* table = interface->create(sizeof(char*), sizeof(int), 
                                   F_string_hash_function, 
                                   F_string_compare_function,
                                   8, &error_code);
    if (table == NULL) {
        printf("    [失败] 哈希表创建失败，错误码: %d\n", error_code);
        return false;
    }
    
    // 测试插入操作
    const char* key1 = "key1";
    int value1 = 100;
    bool put_result = interface->put(table, (void*)&key1, &value1, &error_code);
    if (!put_result) {
        printf("    [失败] 接口插入操作失败，错误码: %d\n", error_code);
        interface->destroy(table, NULL);
        return false;
    }
    
    // 测试获取操作
    int* retrieved_value = (int*)interface->get(table, (void*)&key1, &error_code);
    if (retrieved_value == NULL) {
        printf("    [失败] 接口获取操作失败，错误码: %d\n", error_code);
        interface->destroy(table, NULL);
        return false;
    }
    
    if (*retrieved_value != value1) {
        printf("    [失败] 获取的值不匹配: 期望 %d, 实际 %d\n", value1, *retrieved_value);
        interface->destroy(table, NULL);
        return false;
    }
    
    // 测试删除操作
    bool remove_result = interface->remove(table, (void*)&key1, &error_code);
    if (!remove_result) {
        printf("    [失败] 接口删除操作失败，错误码: %d\n", error_code);
        interface->destroy(table, NULL);
        return false;
    }
    
    // 验证已删除
    retrieved_value = (int*)interface->get(table, (void*)&key1, &error_code);
    if (retrieved_value != NULL) {
        printf("    [失败] 删除后仍能获取到值\n");
        interface->destroy(table, NULL);
        return false;
    }
    
    interface->destroy(table, NULL);
    
    printf("    [通过] 接口基本操作测试\n");
    return true;
}

/**
 * @brief 测试接口批量操作
 * @return 测试通过返回true，失败返回false
 */
static bool test_interface_batch_operations(void)
{
    printf("  [测试] 接口批量操作...\n");
    
    // 获取默认接口
    const Stru_HashTableInterface_t* interface = F_get_default_hash_table_interface();
    if (interface == NULL) {
        printf("    [失败] 获取默认接口失败\n");
        return false;
    }
    
    // 创建哈希表
    enum Eum_HashTableError error_code;
    void* table = interface->create(sizeof(char*), sizeof(int), 
                                   F_string_hash_function, 
                                   F_string_compare_function,
                                   16, &error_code);
    if (table == NULL) {
        printf("    [失败] 哈希表创建失败，错误码: %d\n", error_code);
        return false;
    }
    
    // 批量插入数据
    const char* keys[] = {"key1", "key2", "key3", "key4", "key5"};
    int values[] = {10, 20, 30, 40, 50};
    int count = sizeof(keys) / sizeof(keys[0]);
    
    for (int i = 0; i < count; i++) {
        bool put_result = interface->put(table, (void*)&keys[i], &values[i], &error_code);
        if (!put_result) {
            printf("    [失败] 批量插入失败 at index %d，错误码: %d\n", i, error_code);
            interface->destroy(table, NULL);
            return false;
        }
    }
    
    // 验证所有数据
    for (int i = 0; i < count; i++) {
        int* retrieved_value = (int*)interface->get(table, (void*)&keys[i], &error_code);
        if (retrieved_value == NULL) {
            printf("    [失败] 批量获取失败 at index %d，错误码: %d\n", i, error_code);
            interface->destroy(table, NULL);
            return false;
        }
        
        if (*retrieved_value != values[i]) {
            printf("    [失败] 值不匹配 at index %d: 期望 %d, 实际 %d\n", 
                   i, values[i], *retrieved_value);
            interface->destroy(table, NULL);
            return false;
        }
    }
    
    // 批量删除
    for (int i = 0; i < count; i++) {
        bool remove_result = interface->remove(table, (void*)&keys[i], &error_code);
        if (!remove_result) {
            printf("    [失败] 批量删除失败 at index %d，错误码: %d\n", i, error_code);
            interface->destroy(table, NULL);
            return false;
        }
    }
    
    // 验证所有数据已删除
    for (int i = 0; i < count; i++) {
        int* retrieved_value = (int*)interface->get(table, (void*)&keys[i], &error_code);
        if (retrieved_value != NULL) {
            printf("    [失败] 删除后仍能获取到值 at index %d\n", i);
            interface->destroy(table, NULL);
            return false;
        }
    }
    
    interface->destroy(table, NULL);
    
    printf("    [通过] 接口批量操作测试\n");
    return true;
}

/**
 * @brief 测试接口配置选项
 * @return 测试通过返回true，失败返回false
 */
static bool test_interface_configuration(void)
{
    printf("  [测试] 接口配置选项...\n");
    
    // 获取默认接口
    const Stru_HashTableInterface_t* interface = F_get_default_hash_table_interface();
    if (interface == NULL) {
        printf("    [失败] 获取默认接口失败\n");
        return false;
    }
    
    // 测试不同的哈希函数
    enum Eum_HashTableError error_code;
    void* table = interface->create(sizeof(int), sizeof(int), 
                                   F_int_hash_function, 
                                   F_int_compare_function,
                                   8, &error_code);
    if (table == NULL) {
        printf("    [失败] 整数哈希函数创建失败，错误码: %d\n", error_code);
        return false;
    }
    
    // 插入整数键值对
    int key1 = 12345;
    int value1 = 999;
    bool put_result = interface->put(table, &key1, &value1, &error_code);
    if (!put_result) {
        printf("    [失败] 整数键插入失败，错误码: %d\n", error_code);
        interface->destroy(table, NULL);
        return false;
    }
    
    // 获取整数键值对
    int* retrieved_value = (int*)interface->get(table, &key1, &error_code);
    if (retrieved_value == NULL || *retrieved_value != value1) {
        printf("    [失败] 整数键获取失败，错误码: %d\n", error_code);
        interface->destroy(table, NULL);
        return false;
    }
    
    interface->destroy(table, NULL);
    
    printf("    [通过] 接口配置选项测试\n");
    return true;
}

/**
 * @brief 测试接口查询操作
 * @return 测试通过返回true，失败返回false
 */
static bool test_interface_query_operations(void)
{
    printf("  [测试] 接口查询操作...\n");
    
    // 获取默认接口
    const Stru_HashTableInterface_t* interface = F_get_default_hash_table_interface();
    if (interface == NULL) {
        printf("    [失败] 获取默认接口失败\n");
        return false;
    }
    
    // 创建哈希表
    enum Eum_HashTableError error_code;
    void* table = interface->create(sizeof(char*), sizeof(int), 
                                   F_string_hash_function, 
                                   F_string_compare_function,
                                   8, &error_code);
    if (table == NULL) {
        printf("    [失败] 哈希表创建失败，错误码: %d\n", error_code);
        return false;
    }
    
    // 插入一些数据
    const char* key1 = "key1";
    int value1 = 100;
    interface->put(table, (void*)&key1, &value1, NULL);
    
    // 测试大小查询
    size_t size = interface->size(table, &error_code);
    if (size != 1) {
        printf("    [失败] 大小查询失败: 期望 1, 实际 %zu\n", size);
        interface->destroy(table, NULL);
        return false;
    }
    
    // 测试容量查询
    size_t capacity = interface->capacity(table, &error_code);
    if (capacity < 8) {
        printf("    [失败] 容量查询失败: 期望 >= 8, 实际 %zu\n", capacity);
        interface->destroy(table, NULL);
        return false;
    }
    
    // 测试是否为空
    bool is_empty = interface->is_empty(table, &error_code);
    if (is_empty) {
        printf("    [失败] 是否为空查询失败: 期望 false, 实际 true\n");
        interface->destroy(table, NULL);
        return false;
    }
    
    // 测试包含检查
    bool contains = interface->contains(table, (void*)&key1, &error_code);
    if (!contains) {
        printf("    [失败] 包含检查失败: 期望 true, 实际 false\n");
        interface->destroy(table, NULL);
        return false;
    }
    
    interface->destroy(table, NULL);
    
    printf("    [通过] 接口查询操作测试\n");
    return true;
}

/**
 * @brief 测试接口错误处理
 * @return 测试通过返回true，失败返回false
 */
static bool test_interface_error_handling(void)
{
    printf("  [测试] 接口错误处理...\n");
    
    // 获取默认接口
    const Stru_HashTableInterface_t* interface = F_get_default_hash_table_interface();
    if (interface == NULL) {
        printf("    [失败] 获取默认接口失败\n");
        return false;
    }
    
    // 测试NULL参数
    enum Eum_HashTableError error_code;
    
    // 测试NULL接口指针（应该由调用者处理，这里只是验证不会崩溃）
    void* table = interface->create(sizeof(char*), sizeof(int), 
                                   F_string_hash_function, 
                                   F_string_compare_function,
                                   8, &error_code);
    if (table == NULL) {
        printf("    [注意] 正常创建失败，可能参数无效\n");
    }
    
    // 测试无效容量
    void* table2 = interface->create(sizeof(char*), sizeof(int), 
                                    F_string_hash_function, 
                                    F_string_compare_function,
                                    0, &error_code);
    if (table2 != NULL) {
        printf("    [失败] 零容量创建应失败\n");
        interface->destroy(table2, NULL);
        return false;
    }
    
    // 测试NULL哈希函数
    void* table3 = interface->create(sizeof(char*), sizeof(int), 
                                    NULL, 
                                    F_string_compare_function,
                                    8, &error_code);
    if (table3 != NULL) {
        printf("    [失败] NULL哈希函数创建应失败\n");
        interface->destroy(table3, NULL);
        return false;
    }
    
    // 测试NULL比较函数
    void* table4 = interface->create(sizeof(char*), sizeof(int), 
                                    F_string_hash_function, 
                                    NULL,
                                    8, &error_code);
    if (table4 != NULL) {
        printf("    [失败] NULL比较函数创建应失败\n");
        interface->destroy(table4, NULL);
        return false;
    }
    
    printf("    [通过] 接口错误处理测试\n");
    return true;
}

/**
 * @brief 测试抽象接口功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_abstract_interface(void)
{
    printf("\n[测试套件] 哈希表抽象接口功能测试\n");
    printf("========================================\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行所有接口测试
    total++; if (test_interface_get_and_validate()) passed++;
    total++; if (test_interface_create_and_destroy()) passed++;
    total++; if (test_interface_basic_operations()) passed++;
    total++; if (test_interface_batch_operations()) passed++;
    total++; if (test_interface_configuration()) passed++;
    total++; if (test_interface_query_operations()) passed++;
    total++; if (test_interface_error_handling()) passed++;
    
    printf("========================================\n");
    printf("[结果] 接口测试: %d/%d 通过\n\n", passed, total);
    
    return passed == total;
}
