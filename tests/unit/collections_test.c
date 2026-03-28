/**
 * @file collections_test.c
 * @brief CN语言运行时集合数据结构单元测试
 * 
 * 测试动态数组（Vector）、链表（LinkedList）、哈希表（HashMap）的功能
 * 
 * @version v1.0
 * @date 2026-03-28
 */

#include "cnlang/runtime/collections.h"
#include "cnlang/runtime/memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* =============================================================================
 * 哈希表（HashMap）测试
 * =============================================================================
 */

/**
 * @brief 测试哈希表创建和销毁
 */
static void test_map_create_destroy(void) {
    printf("测试: 哈希表创建和销毁\n");
    
    /* 测试默认容量 */
    CnMap* map = cn_rt_map_create(0);
    assert(map != NULL);
    assert(map->bucket_count == 16);
    assert(map->size == 0);
    assert(map->buckets != NULL);
    
    cn_rt_map_destroy(map);
    
    /* 测试自定义容量 */
    map = cn_rt_map_create(32);
    assert(map != NULL);
    assert(map->bucket_count == 32);
    assert(map->size == 0);
    
    cn_rt_map_destroy(map);
    
    /* 测试销毁NULL指针 */
    cn_rt_map_destroy(NULL);
    
    printf("[PASS] 哈希表创建和销毁测试\n\n");
}

/**
 * @brief 测试哈希表插入和获取
 */
static void test_map_insert_get(void) {
    printf("测试: 哈希表插入和获取\n");
    
    CnMap* map = cn_rt_map_create(0);
    assert(map != NULL);
    
    /* 测试插入 */
    int value1 = 100;
    int value2 = 200;
    int value3 = 300;
    
    assert(cn_rt_map_insert(map, "key1", &value1) == 0);
    assert(cn_rt_map_insert(map, "key2", &value2) == 0);
    assert(cn_rt_map_insert(map, "key3", &value3) == 0);
    assert(map->size == 3);
    
    /* 测试获取 */
    int* result = (int*)cn_rt_map_get(map, "key1");
    assert(result != NULL);
    assert(*result == 100);
    
    result = (int*)cn_rt_map_get(map, "key2");
    assert(result != NULL);
    assert(*result == 200);
    
    /* 测试不存在的键 */
    result = (int*)cn_rt_map_get(map, "nonexistent");
    assert(result == NULL);
    
    /* 测试更新已存在的键 */
    int new_value1 = 999;
    assert(cn_rt_map_insert(map, "key1", &new_value1) == 0);
    assert(map->size == 3);  /* 大小不变 */
    
    result = (int*)cn_rt_map_get(map, "key1");
    assert(result != NULL);
    assert(*result == 999);
    
    cn_rt_map_destroy(map);
    
    printf("[PASS] 哈希表插入和获取测试\n\n");
}

/**
 * @brief 测试哈希表删除
 */
static void test_map_remove(void) {
    printf("测试: 哈希表删除\n");
    
    CnMap* map = cn_rt_map_create(0);
    assert(map != NULL);
    
    /* 插入测试数据 */
    int value1 = 100;
    int value2 = 200;
    int value3 = 300;
    
    cn_rt_map_insert(map, "key1", &value1);
    cn_rt_map_insert(map, "key2", &value2);
    cn_rt_map_insert(map, "key3", &value3);
    assert(map->size == 3);
    
    /* 测试删除 */
    assert(cn_rt_map_remove(map, "key2") == 0);
    assert(map->size == 2);
    assert(cn_rt_map_get(map, "key2") == NULL);
    
    /* 测试删除不存在的键 */
    assert(cn_rt_map_remove(map, "nonexistent") == -1);
    assert(map->size == 2);
    
    /* 测试删除后再获取 */
    assert(cn_rt_map_get(map, "key1") != NULL);
    assert(cn_rt_map_get(map, "key3") != NULL);
    
    cn_rt_map_destroy(map);
    
    printf("[PASS] 哈希表删除测试\n\n");
}

/**
 * @brief 测试哈希表包含检查
 */
static void test_map_contains(void) {
    printf("测试: 哈希表包含检查\n");
    
    CnMap* map = cn_rt_map_create(0);
    assert(map != NULL);
    
    int value = 100;
    cn_rt_map_insert(map, "key1", &value);
    
    assert(cn_rt_map_contains(map, "key1") == 1);
    assert(cn_rt_map_contains(map, "nonexistent") == 0);
    
    cn_rt_map_destroy(map);
    
    printf("[PASS] 哈希表包含检查测试\n\n");
}

/**
 * @brief 测试哈希表大小和空检查
 */
static void test_map_size_empty(void) {
    printf("测试: 哈希表大小和空检查\n");
    
    CnMap* map = cn_rt_map_create(0);
    assert(map != NULL);
    
    /* 测试空哈希表 */
    assert(cn_rt_map_size(map) == 0);
    assert(cn_rt_map_is_empty(map) == 1);
    
    /* 插入元素 */
    int value = 100;
    cn_rt_map_insert(map, "key1", &value);
    
    assert(cn_rt_map_size(map) == 1);
    assert(cn_rt_map_is_empty(map) == 0);
    
    /* 清空 */
    cn_rt_map_clear(map);
    assert(cn_rt_map_size(map) == 0);
    assert(cn_rt_map_is_empty(map) == 1);
    
    cn_rt_map_destroy(map);
    
    printf("[PASS] 哈希表大小和空检查测试\n\n");
}

/**
 * @brief 测试哈希表清空
 */
static void test_map_clear(void) {
    printf("测试: 哈希表清空\n");
    
    CnMap* map = cn_rt_map_create(0);
    assert(map != NULL);
    
    /* 插入多个元素 */
    int values[10];
    for (int i = 0; i < 10; i++) {
        char key[16];
        sprintf(key, "key%d", i);
        values[i] = i * 10;
        cn_rt_map_insert(map, key, &values[i]);
    }
    assert(map->size == 10);
    
    /* 清空 */
    cn_rt_map_clear(map);
    assert(map->size == 0);
    assert(cn_rt_map_is_empty(map) == 1);
    
    /* 验证所有键都不存在 */
    for (int i = 0; i < 10; i++) {
        char key[16];
        sprintf(key, "key%d", i);
        assert(cn_rt_map_contains(map, key) == 0);
    }
    
    cn_rt_map_destroy(map);
    
    printf("[PASS] 哈希表清空测试\n\n");
}

/**
 * @brief 测试哈希表冲突处理
 */
static void test_map_collision(void) {
    printf("测试: 哈希表冲突处理\n");
    
    /* 使用很小的桶数量来强制冲突 */
    CnMap* map = cn_rt_map_create(4);
    assert(map != NULL);
    
    /* 插入多个元素，必然会产生冲突 */
    int values[20];
    for (int i = 0; i < 20; i++) {
        char key[16];
        sprintf(key, "key%d", i);
        values[i] = i * 10;
        assert(cn_rt_map_insert(map, key, &values[i]) == 0);
    }
    assert(map->size == 20);
    
    /* 验证所有元素都能正确获取 */
    for (int i = 0; i < 20; i++) {
        char key[16];
        sprintf(key, "key%d", i);
        int* result = (int*)cn_rt_map_get(map, key);
        assert(result != NULL);
        assert(*result == i * 10);
    }
    
    /* 删除部分元素 */
    for (int i = 0; i < 10; i++) {
        char key[16];
        sprintf(key, "key%d", i);
        assert(cn_rt_map_remove(map, key) == 0);
    }
    assert(map->size == 10);
    
    /* 验证剩余元素仍然正确 */
    for (int i = 10; i < 20; i++) {
        char key[16];
        sprintf(key, "key%d", i);
        int* result = (int*)cn_rt_map_get(map, key);
        assert(result != NULL);
        assert(*result == i * 10);
    }
    
    cn_rt_map_destroy(map);
    
    printf("[PASS] 哈希表冲突处理测试\n\n");
}

/**
 * @brief 测试中文别名宏
 */
static void test_map_chinese_aliases(void) {
    printf("测试: 哈希表中文别名\n");
    
    /* 使用中文别名 */
    CnMap* map = 创建哈希表(0);
    assert(map != NULL);
    
    int value = 100;
    assert(哈希表插入(map, "测试键", &value) == 0);
    assert(哈希表大小(map) == 1);
    assert(哈希表包含(map, "测试键") == 1);
    
    int* result = (int*)哈希表获取(map, "测试键");
    assert(result != NULL);
    assert(*result == 100);
    
    assert(哈希表删除(map, "测试键") == 0);
    assert(哈希表为空(map) == 1);
    
    销毁哈希表(map);
    
    printf("[PASS] 哈希表中文别名测试\n\n");
}

/* =============================================================================
 * 主函数
 * =============================================================================
 */

int main(void) {
    printf("========================================\n");
    printf("CN语言运行时集合数据结构单元测试\n");
    printf("========================================\n\n");
    
    /* 初始化内存系统 */
    cn_rt_memory_init(NULL);
    
    /* 哈希表测试 */
    test_map_create_destroy();
    test_map_insert_get();
    test_map_remove();
    test_map_contains();
    test_map_size_empty();
    test_map_clear();
    test_map_collision();
    test_map_chinese_aliases();
    
    printf("========================================\n");
    printf("测试结果: 全部通过\n");
    printf("========================================\n");
    
    return 0;
}
