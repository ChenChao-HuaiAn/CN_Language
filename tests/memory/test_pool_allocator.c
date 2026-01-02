/**
 * @file test_pool_allocator.c
 * @brief 对象池分配器测试
 * 
 * 测试CN_pool_allocator模块的功能和性能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-02
 * @version 1.0.0
 * @license MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>

#include "../../src/infrastructure/memory/pool/CN_pool_allocator.h"

/* 测试数据结构 */
typedef struct Stru_TestData_t
{
    int id;
    double value;
    char name[32];
    bool active;
} Stru_TestData_t;

/* 测试配置 */
#define TEST_OBJECT_SIZE sizeof(Stru_TestData_t)
#define TEST_INITIAL_CAPACITY 100
#define TEST_EXPAND_INCREMENT 50

/* 测试函数声明 */
static void test_basic_functionality(void);
static void test_configuration(void);
static void test_allocation_free(void);
static void test_boundary_conditions(void);
static void test_statistics(void);
static void test_debug_features(void);
static void test_performance(void);
static void test_integration(void);

/* 调试回调函数 */
static void test_debug_callback(const char* message, void* user_data)
{
    FILE* log_file = (FILE*)user_data;
    if (log_file) {
        fprintf(log_file, "[TEST DEBUG] %s\n", message);
    }
}

/**
 * @brief 主测试函数
 */
int main(void)
{
    printf("=== CN_pool_allocator 测试开始 ===\n\n");
    
    /* 运行所有测试 */
    test_basic_functionality();
    test_configuration();
    test_allocation_free();
    test_boundary_conditions();
    test_statistics();
    test_debug_features();
    test_performance();
    test_integration();
    
    printf("\n=== CN_pool_allocator 测试完成 ===\n");
    printf("所有测试通过！\n");
    
    return EXIT_SUCCESS;
}

/**
 * @brief 测试基本功能
 */
static void test_basic_functionality(void)
{
    printf("1. 测试基本功能...\n");
    
    /* 创建配置 */
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = TEST_OBJECT_SIZE;
    config.initial_capacity = TEST_INITIAL_CAPACITY;
    config.auto_expand = true;
    config.expand_increment = TEST_EXPAND_INCREMENT;
    
    /* 创建对象池 */
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    assert(pool != NULL);
    printf("  ✓ 对象池创建成功\n");
    
    /* 测试分配 */
    Stru_TestData_t* data1 = (Stru_TestData_t*)CN_pool_alloc(pool);
    assert(data1 != NULL);
    printf("  ✓ 对象分配成功\n");
    
    /* 使用对象 */
    data1->id = 1;
    data1->value = 3.14159;
    strcpy(data1->name, "Test Object");
    data1->active = true;
    
    /* 验证对象内容 */
    assert(data1->id == 1);
    assert(data1->value == 3.14159);
    assert(strcmp(data1->name, "Test Object") == 0);
    assert(data1->active == true);
    printf("  ✓ 对象使用正常\n");
    
    /* 测试释放 */
    bool free_result = CN_pool_free(pool, data1);
    assert(free_result == true);
    printf("  ✓ 对象释放成功\n");
    
    /* 测试重复释放检测 */
    free_result = CN_pool_free(pool, data1);
    assert(free_result == false);
    printf("  ✓ 重复释放检测正常\n");
    
    /* 测试NULL指针释放 */
    free_result = CN_pool_free(pool, NULL);
    assert(free_result == false);
    printf("  ✓ NULL指针释放处理正常\n");
    
    /* 销毁对象池 */
    CN_pool_destroy(pool);
    printf("  ✓ 对象池销毁成功\n");
    
    printf("  基本功能测试完成\n\n");
}

/**
 * @brief 测试配置功能
 */
static void test_configuration(void)
{
    printf("2. 测试配置功能...\n");
    
    /* 测试无效配置 */
    Stru_CN_PoolConfig_t invalid_config = CN_POOL_CONFIG_DEFAULT;
    invalid_config.object_size = 0;  /* 无效的对象大小 */
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&invalid_config);
    assert(pool == NULL);
    printf("  ✓ 无效配置检测正常\n");
    
    /* 测试有效配置 */
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = TEST_OBJECT_SIZE;
    config.initial_capacity = 50;
    config.max_capacity = 200;
    config.auto_expand = false;
    config.zero_on_alloc = true;
    config.zero_on_free = true;
    
    pool = CN_pool_create(&config);
    assert(pool != NULL);
    printf("  ✓ 复杂配置创建成功\n");
    
    /* 测试获取配置 */
    Stru_CN_PoolConfig_t retrieved_config;
    bool get_config_result = CN_pool_get_config(pool, &retrieved_config);
    assert(get_config_result == true);
    assert(retrieved_config.object_size == config.object_size);
    assert(retrieved_config.initial_capacity == config.initial_capacity);
    assert(retrieved_config.max_capacity == config.max_capacity);
    assert(retrieved_config.auto_expand == config.auto_expand);
    assert(retrieved_config.zero_on_alloc == config.zero_on_alloc);
    assert(retrieved_config.zero_on_free == config.zero_on_free);
    printf("  ✓ 配置获取功能正常\n");
    
    /* 测试清零功能 */
    Stru_TestData_t* data = (Stru_TestData_t*)CN_pool_alloc(pool);
    assert(data != NULL);
    
    /* 检查是否已清零 */
    if (config.zero_on_alloc) {
        assert(data->id == 0);
        assert(data->value == 0.0);
        assert(data->name[0] == '\0');
        assert(data->active == false);
        printf("  ✓ 分配时清零功能正常\n");
    }
    
    /* 填充数据 */
    data->id = 100;
    data->value = 99.9;
    strcpy(data->name, "Test Data");
    data->active = true;
    
    /* 释放并检查清零 */
    CN_pool_free(pool, data);
    
    /* 注意：由于内存可能被重用，不能直接检查清零效果 */
    printf("  ✓ 释放时清零功能配置正常\n");
    
    CN_pool_destroy(pool);
    printf("  配置功能测试完成\n\n");
}

/**
 * @brief 测试分配和释放
 */
static void test_allocation_free(void)
{
    printf("3. 测试分配和释放...\n");
    
    /* 创建对象池 */
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = TEST_OBJECT_SIZE;
    config.initial_capacity = 10;
    config.auto_expand = true;
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    assert(pool != NULL);
    
    /* 分配多个对象 */
    Stru_TestData_t* objects[20];
    for (int i = 0; i < 20; i++) {
        objects[i] = (Stru_TestData_t*)CN_pool_alloc(pool);
        assert(objects[i] != NULL);
        objects[i]->id = i;
    }
    printf("  ✓ 分配20个对象成功（超过初始容量，测试自动扩展）\n");
    
    /* 验证对象唯一性 */
    for (int i = 0; i < 20; i++) {
        for (int j = i + 1; j < 20; j++) {
            assert(objects[i] != objects[j]);
        }
    }
    printf("  ✓ 所有对象地址唯一\n");
    
    /* 释放部分对象 */
    for (int i = 0; i < 10; i++) {
        bool result = CN_pool_free(pool, objects[i]);
        assert(result == true);
        objects[i] = NULL;
    }
    printf("  ✓ 释放10个对象成功\n");
    
    /* 重新分配（应重用释放的对象） */
    for (int i = 0; i < 5; i++) {
        objects[i] = (Stru_TestData_t*)CN_pool_alloc(pool);
        assert(objects[i] != NULL);
    }
    printf("  ✓ 重新分配5个对象成功（测试对象重用）\n");
    
    /* 测试realloc功能 */
    void* ptr = CN_pool_alloc(pool);
    assert(ptr != NULL);
    
    void* new_ptr = CN_pool_realloc(pool, ptr, TEST_OBJECT_SIZE);
    assert(new_ptr != NULL);
    assert(new_ptr == ptr);  /* 大小相同，应返回原指针 */
    printf("  ✓ realloc功能正常（相同大小）\n");
    
    /* 测试realloc with NULL */
    new_ptr = CN_pool_realloc(pool, NULL, TEST_OBJECT_SIZE);
    assert(new_ptr != NULL);
    printf("  ✓ realloc(NULL)功能正常\n");
    
    /* 清理 */
    CN_pool_free(pool, new_ptr);
    for (int i = 5; i < 20; i++) {
        if (objects[i]) {
            CN_pool_free(pool, objects[i]);
        }
    }
    for (int i = 0; i < 5; i++) {
        if (objects[i]) {
            CN_pool_free(pool, objects[i]);
        }
    }
    
    CN_pool_destroy(pool);
    printf("  分配和释放测试完成\n\n");
}

/**
 * @brief 测试边界条件
 */
static void test_boundary_conditions(void)
{
    printf("4. 测试边界条件...\n");
    
    /* 测试容量限制 */
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = TEST_OBJECT_SIZE;
    config.initial_capacity = 5;
    config.max_capacity = 10;
    config.auto_expand = false;  /* 禁用自动扩展 */
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    assert(pool != NULL);
    
    /* 分配达到容量限制 */
    void* objects[10];
    for (int i = 0; i < 5; i++) {
        objects[i] = CN_pool_alloc(pool);
        assert(objects[i] != NULL);
    }
    printf("  ✓ 分配达到初始容量限制正常\n");
    
    /* 尝试超额分配（应失败） */
    void* extra = CN_pool_alloc(pool);
    assert(extra == NULL);
    printf("  ✓ 超额分配失败处理正常\n");
    
    /* 释放一个对象 */
    bool free_result = CN_pool_free(pool, objects[0]);
    assert(free_result == true);
    
    /* 现在应该可以再分配一个 */
    extra = CN_pool_alloc(pool);
    assert(extra != NULL);
    printf("  ✓ 释放后重新分配正常\n");
    
    /* 测试contains功能 */
    bool contains_result = CN_pool_contains(pool, objects[1]);
    assert(contains_result == true);
    printf("  ✓ 包含检查正常（有效指针）\n");
    
    /* 测试无效指针 */
    void* invalid_ptr = malloc(100);  /* 系统分配，不属于对象池 */
    contains_result = CN_pool_contains(pool, invalid_ptr);
    assert(contains_result == false);
    free(invalid_ptr);
    printf("  ✓ 包含检查正常（无效指针）\n");
    
    /* 测试NULL指针 */
    contains_result = CN_pool_contains(pool, NULL);
    assert(contains_result == false);
    printf("  ✓ 包含检查正常（NULL指针）\n");
    
    /* 清理 */
    for (int i = 1; i < 5; i++) {
        if (objects[i]) {
            CN_pool_free(pool, objects[i]);
        }
    }
    CN_pool_free(pool, extra);
    
    CN_pool_destroy(pool);
    printf("  边界条件测试完成\n\n");
}

/**
 * @brief 测试统计功能
 */
static void test_statistics(void)
{
    printf("5. 测试统计功能...\n");
    
    /* 创建对象池 */
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = TEST_OBJECT_SIZE;
    config.initial_capacity = 100;
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    assert(pool != NULL);
    
    /* 获取初始统计 */
    Stru_CN_PoolStats_t stats;
    bool get_stats_result = CN_pool_get_stats(pool, &stats);
    assert(get_stats_result == true);
    assert(stats.total_objects == 100);
    assert(stats.free_objects == 100);
    assert(stats.allocated_objects == 0);
    assert(stats.total_allocations == 0);
    assert(stats.total_deallocations == 0);
    printf("  ✓ 初始统计信息正确\n");
    
    /* 执行一些操作 */
    void* obj1 = CN_pool_alloc(pool);
    void* obj2 = CN_pool_alloc(pool);
    void* obj3 = CN_pool_alloc(pool);
    
    CN_pool_free(pool, obj2);
    
    /* 获取更新后的统计 */
    get_stats_result = CN_pool_get_stats(pool, &stats);
    assert(get_stats_result == true);
    assert(stats.total_allocations == 3);
    assert(stats.total_deallocations == 1);
    assert(stats.allocated_objects == 2);  /* obj1和obj3 */
    assert(stats.free_objects == 98);      /* 100-3+1 */
    printf("  ✓ 操作后统计信息正确\n");
    
    /* 测试使用率计算 */
    float utilization = CN_pool_utilization(pool);
    assert(utilization >= 0.0f && utilization <= 1.0f);
    printf("  ✓ 使用率计算正常: %.2f%%\n", utilization * 100.0f);
    
    /* 测试重置统计 */
    CN_pool_reset_stats(pool);
    get_stats_result = CN_pool_get_stats(pool, &stats);
    assert(get_stats_result == true);
    assert(stats.total_allocations == 0);
    assert(stats.total_deallocations == 0);
    /* 注意：总对象数和已分配对象数不应重置 */
    printf("  ✓ 统计重置功能正常\n");
    
    /* 测试预分配 */
    bool prealloc_result = CN_pool_prealloc(pool, 50);
    assert(prealloc_result == true);
    printf("  ✓ 预分配功能正常\n");
    
    /* 测试清空 */
    bool clear_result = CN_pool_clear(pool);
    assert(clear_result == true);
    printf("  ✓ 清空功能正常\n");
    
    /* 测试收缩（当前版本应返回false） */
    bool shrink_result = CN_pool_shrink(pool);
    assert(shrink_result == false);  /* 当前版本未实现 */
    printf("  ✓ 收缩功能状态正确（未实现）\n");
    
    /* 清理 */
    CN_pool_free(pool, obj1);
    CN_pool_free(pool, obj3);
    
    CN_pool_destroy(pool);
    printf("  统计功能测试完成\n\n");
}

/**
 * @brief 测试调试功能
 */
static void test_debug_features(void)
{
    printf("6. 测试调试功能...\n");
    
    /* 创建日志文件 */
    FILE* log_file = fopen("test_pool_debug.log", "w");
    if (!log_file) {
        printf("  警告：无法创建日志文件，跳过部分调试测试\n");
        return;
    }
    
    /* 创建对象池 */
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = TEST_OBJECT_SIZE;
    config.initial_capacity = 20;
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    assert(pool != NULL);
    
    /* 设置调试回调 */
    CN_pool_set_debug_callback(pool, test_debug_callback, log_file);
    printf("  ✓ 调试回调设置成功\n");
    
    /* 执行一些操作生成调试信息 */
    void* obj1 = CN_pool_alloc(pool);
    void* obj2 = CN_pool_alloc(pool);
    void* obj3 = CN_pool_alloc(pool);
    
    CN_pool_free(pool, obj2);
    
    /* 测试验证功能 */
    bool validate_result = CN_pool_validate(pool);
    assert(validate_result == true);
    printf("  ✓ 对象池验证通过\n");
    
    /* 测试转储功能（输出到标准输出） */
    printf("  === 对象池状态转储开始 ===\n");
    CN_pool_dump(pool);
    printf("  === 对象池状态转储结束 ===\n");
    printf("  ✓ 状态转储功能正常\n");
    
    /* 清理 */
    CN_pool_free(pool, obj1);
    CN_pool_free(pool, obj3);
    
    CN_pool_destroy(pool);
    fclose(log_file);
    
    /* 检查日志文件 */
    if (remove("test_pool_debug.log") == 0) {
        printf("  ✓ 调试日志文件创建和清理正常\n");
    }
    
    printf("  调试功能测试完成\n\n");
}

/**
 * @brief 测试性能
 */
static void test_performance(void)
{
    printf("7. 测试性能...\n");
    
    /* 创建高性能配置 */
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = TEST_OBJECT_SIZE;
    config.initial_capacity = 10000;
    config.auto_expand = true;
    config.expand_increment = 5000;
    config.zero_on_alloc = false;  /* 关闭清零以提高性能 */
    config.zero_on_free = false;
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    assert(pool != NULL);
    
    /* 预分配提高性能 */
    bool prealloc_result = CN_pool_prealloc(pool, 10000);
    assert(prealloc_result == true);
    printf("  ✓ 预分配10000个对象成功\n");
    
    /* 性能测试：分配和释放循环 */
    const int iterations = 10000;
    const int batch_size = 100;
    
    clock_t start = clock();
    
    void* objects[batch_size];
    
    for (int i = 0; i < iterations; i++) {
        /* 分配一批对象 */
        for (int j = 0; j < batch_size; j++) {
            objects[j] = CN_pool_alloc(pool);
            assert(objects[j] != NULL);
            
            /* 简单使用对象 */
            Stru_TestData_t* data = (Stru_TestData_t*)objects[j];
            data->id = j;
            data->value = j * 1.5;
        }
        
        /* 释放所有对象 */
        for (int j = 0; j < batch_size; j++) {
            bool free_result = CN_pool_free(pool, objects[j]);
            assert(free_result == true);
        }
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    /* 计算性能指标 */
    int total_operations = iterations * batch_size * 2;  /* 分配+释放 */
    double operations_per_second = total_operations / elapsed;
    
    printf("  性能测试结果:\n");
    printf("    迭代次数: %d\n", iterations);
    printf("    每批对象数: %d\n", batch_size);
    printf("    总操作数: %d\n", total_operations);
    printf("    总时间: %.3f 秒\n", elapsed);
    printf("    每秒操作数: %.0f\n", operations_per_second);
    
    /* 获取统计信息 */
    Stru_CN_PoolStats_t stats;
    if (CN_pool_get_stats(pool, &stats)) {
        printf("    对象池统计:\n");
        printf("      总对象数: %zu\n", stats.total_objects);
        printf("      池扩展次数: %zu\n", stats.pool_expansions);
        printf("      分配失败次数: %zu\n", stats.allocation_failures);
        printf("      内存使用量: %zu 字节\n", stats.memory_usage);
    }
    
    /* 性能要求：每秒至少100万次操作 */
    if (operations_per_second > 1000000.0) {
        printf("  ✓ 性能达标 (%.0f > 1,000,000 ops/sec)\n", operations_per_second);
    } else {
        printf("  ⚠ 性能警告: %.0f ops/sec (低于1,000,000 ops/sec)\n", operations_per_second);
    }
    
    CN_pool_destroy(pool);
    printf("  性能测试完成\n\n");
}

/**
 * @brief 测试集成功能
 */
static void test_integration(void)
{
    printf("8. 测试集成功能...\n");
    
    /* 测试与CN_memory系统的集成 */
    printf("  注意：此测试需要CN_memory系统已正确集成对象池分配器\n");
    printf("  请确保CN_memory.c中已正确初始化g_pool_allocator_instance\n");
    
    /* 这里可以添加更复杂的集成测试 */
    /* 例如：测试多个对象池的协作、测试内存管理器的统一接口等 */
    
    printf("  集成测试占位完成（实际集成测试需要完整的CN_memory系统）\n");
    printf("  集成功能测试完成\n\n");
}
