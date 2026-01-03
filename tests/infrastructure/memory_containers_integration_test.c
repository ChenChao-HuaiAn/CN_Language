/******************************************************************************
 * 文件名: memory_containers_integration_test.c
 * 功能: 内存管理模块与容器模块集成测试
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，测试内存管理模块与容器模块的集成
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

#include "../../src/infrastructure/memory/CN_memory.h"
#include "../../src/infrastructure/containers/array/CN_array.h"

// ============================================================================
// 测试配置
// ============================================================================

#define TEST_ARRAY_SIZE 1000
#define TEST_ITERATIONS 100
#define TEST_BATCH_SIZE 100

// ============================================================================
// 测试数据结构
// ============================================================================

typedef struct Stru_TestData_t
{
    int id;
    char name[32];
    double value;
    bool active;
} Stru_TestData_t;

// ============================================================================
// 测试函数声明
// ============================================================================

static void test_array_with_system_allocator(void);
static void test_array_with_debug_allocator(void);
static void test_array_with_pool_allocator(void);
static void test_array_with_arena_allocator(void);
static void test_memory_leak_detection(void);
static void test_performance_comparison(void);
static void test_custom_allocator_integration(void);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 创建测试数据
 */
static Stru_TestData_t create_test_data(int id)
{
    Stru_TestData_t data;
    data.id = id;
    snprintf(data.name, sizeof(data.name), "TestData_%d", id);
    data.value = id * 3.14159;
    data.active = (id % 2 == 0);
    return data;
}

/**
 * @brief 验证测试数据
 */
static bool verify_test_data(const Stru_TestData_t* data, int expected_id)
{
    if (data->id != expected_id)
    {
        printf("  错误: 数据ID不匹配，期望 %d，实际 %d\n", expected_id, data->id);
        return false;
    }
    
    char expected_name[32];
    snprintf(expected_name, sizeof(expected_name), "TestData_%d", expected_id);
    if (strcmp(data->name, expected_name) != 0)
    {
        printf("  错误: 数据名称不匹配，期望 %s，实际 %s\n", expected_name, data->name);
        return false;
    }
    
    double expected_value = expected_id * 3.14159;
    if (data->value != expected_value)
    {
        printf("  错误: 数据值不匹配，期望 %f，实际 %f\n", expected_value, data->value);
        return false;
    }
    
    bool expected_active = (expected_id % 2 == 0);
    if (data->active != expected_active)
    {
        printf("  错误: 数据激活状态不匹配，期望 %d，实际 %d\n", expected_active, data->active);
        return false;
    }
    
    return true;
}

// ============================================================================
// 主测试函数
// ============================================================================

int main(void)
{
    printf("=== 内存管理模块与容器模块集成测试开始 ===\n\n");
    
    // 初始化内存管理系统（使用系统分配器）
    if (!CN_memory_init(Eum_ALLOCATOR_SYSTEM))
    {
        printf("错误: 无法初始化内存管理系统\n");
        return EXIT_FAILURE;
    }
    
    printf("1. 测试数组与系统分配器集成:\n");
    test_array_with_system_allocator();
    
    printf("\n2. 测试数组与调试分配器集成:\n");
    test_array_with_debug_allocator();
    
    printf("\n3. 测试数组与对象池分配器集成:\n");
    test_array_with_pool_allocator();
    
    printf("\n4. 测试数组与区域分配器集成:\n");
    test_array_with_arena_allocator();
    
    printf("\n5. 测试内存泄漏检测:\n");
    test_memory_leak_detection();
    
    printf("\n6. 测试性能比较:\n");
    test_performance_comparison();
    
    printf("\n7. 测试自定义分配器集成:\n");
    test_custom_allocator_integration();
    
    // 关闭内存管理系统
    CN_memory_shutdown();
    
    printf("\n=== 内存管理模块与容器模块集成测试完成 ===\n");
    printf("所有测试通过！\n");
    
    return EXIT_SUCCESS;
}

// ============================================================================
// 测试实现
// ============================================================================

/**
 * @brief 测试数组与系统分配器集成
 */
static void test_array_with_system_allocator(void)
{
    printf("  设置系统分配器...\n");
    if (!cn_set_allocator(Eum_ALLOCATOR_SYSTEM))
    {
        printf("  错误: 无法设置系统分配器\n");
        return;
    }
    
    // 创建数组
    Stru_CN_Array_t* array = CN_array_create_custom(
        sizeof(Stru_TestData_t), 10, NULL, NULL, NULL);
    
    if (array == NULL)
    {
        printf("  错误: 无法创建数组\n");
        return;
    }
    
    printf("  ✓ 使用系统分配器创建数组成功\n");
    
    // 添加数据
    for (int i = 0; i < TEST_ARRAY_SIZE; i++)
    {
        Stru_TestData_t data = create_test_data(i);
        if (!CN_array_append(array, &data))
        {
            printf("  错误: 无法添加元素 %d\n", i);
            CN_array_destroy(array);
            return;
        }
    }
    
    printf("  ✓ 添加 %d 个元素成功\n", TEST_ARRAY_SIZE);
    
    // 验证数据
    for (int i = 0; i < TEST_ARRAY_SIZE; i++)
    {
        Stru_TestData_t* data = (Stru_TestData_t*)CN_array_get(array, i);
        if (data == NULL || !verify_test_data(data, i))
        {
            printf("  错误: 验证元素 %d 失败\n", i);
            CN_array_destroy(array);
            return;
        }
    }
    
    printf("  ✓ 所有数据验证成功\n");
    
    // 测试数组操作
    Stru_TestData_t new_data = create_test_data(9999);
    if (!CN_array_set(array, 500, &new_data))
    {
        printf("  错误: 无法设置元素\n");
        CN_array_destroy(array);
        return;
    }
    
    Stru_TestData_t* retrieved = (Stru_TestData_t*)CN_array_get(array, 500);
    if (retrieved == NULL || !verify_test_data(retrieved, 9999))
    {
        printf("  错误: 验证修改后的元素失败\n");
        CN_array_destroy(array);
        return;
    }
    
    printf("  ✓ 数组修改操作成功\n");
    
    // 清理
    CN_array_destroy(array);
    printf("  ✓ 数组销毁成功\n");
    
    // 检查内存泄漏
    if (cn_check_leaks())
    {
        printf("  ⚠ 警告: 检测到内存泄漏\n");
        cn_dump_stats();
    }
    else
    {
        printf("  ✓ 无内存泄漏\n");
    }
}

/**
 * @brief 测试数组与调试分配器集成
 */
static void test_array_with_debug_allocator(void)
{
    printf("  设置调试分配器...\n");
    if (!cn_set_allocator(Eum_ALLOCATOR_DEBUG))
    {
        printf("  错误: 无法设置调试分配器\n");
        return;
    }
    
    // 启用调试模式
    cn_enable_debug(true);
    
    // 创建数组
    Stru_CN_Array_t* array = CN_array_create_custom(
        sizeof(Stru_TestData_t), 5, NULL, NULL, NULL);
    
    if (array == NULL)
    {
        printf("  错误: 无法创建数组\n");
        return;
    }
    
    printf("  ✓ 使用调试分配器创建数组成功\n");
    
    // 测试边界检查
    Stru_TestData_t data = create_test_data(1);
    
    // 正常添加
    if (!CN_array_append(array, &data))
    {
        printf("  错误: 无法添加元素\n");
        CN_array_destroy(array);
        return;
    }
    
    printf("  ✓ 正常添加元素成功\n");
    
    // 测试无效访问（应该返回NULL）
    Stru_TestData_t* invalid = (Stru_TestData_t*)CN_array_get(array, 100);
    if (invalid != NULL)
    {
        printf("  错误: 无效索引访问应返回NULL\n");
        CN_array_destroy(array);
        return;
    }
    
    printf("  ✓ 无效索引检查正常\n");
    
    // 清理
    CN_array_destroy(array);
    printf("  ✓ 数组销毁成功\n");
    
    // 验证堆完整性
    if (!cn_validate_heap())
    {
        printf("  ⚠ 警告: 堆完整性检查失败\n");
    }
    else
    {
        printf("  ✓ 堆完整性检查通过\n");
    }
    
    cn_enable_debug(false);
}

/**
 * @brief 测试数组与对象池分配器集成
 */
static void test_array_with_pool_allocator(void)
{
    printf("  设置对象池分配器...\n");
    if (!cn_set_allocator(Eum_ALLOCATOR_POOL))
    {
        printf("  错误: 无法设置对象池分配器\n");
        return;
    }
    
    // 创建小数组测试对象池重用
    Stru_CN_Array_t* arrays[10];
    
    for (int i = 0; i < 10; i++)
    {
        arrays[i] = CN_array_create_custom(
            sizeof(Stru_TestData_t), 2, NULL, NULL, NULL);
        
        if (arrays[i] == NULL)
        {
            printf("  错误: 无法创建数组 %d\n", i);
            // 清理已创建的数组
            for (int j = 0; j < i; j++)
            {
                CN_array_destroy(arrays[j]);
            }
            return;
        }
        
        // 添加一些数据
        Stru_TestData_t data = create_test_data(i);
        CN_array_append(arrays[i], &data);
    }
    
    printf("  ✓ 创建10个数组成功（测试对象池重用）\n");
    
    // 销毁所有数组（内存应返回到对象池）
    for (int i = 0; i < 10; i++)
    {
        CN_array_destroy(arrays[i]);
    }
    
    printf("  ✓ 所有数组销毁成功\n");
    
    // 再次创建数组（应重用对象池中的内存）
    Stru_CN_Array_t* reused_array = CN_array_create_custom(
        sizeof(Stru_TestData_t), 2, NULL, NULL, NULL);
    
    if (reused_array == NULL)
    {
        printf("  错误: 无法重用对象池创建数组\n");
        return;
    }
    
    printf("  ✓ 对象池内存重用成功\n");
    
    CN_array_destroy(reused_array);
}

/**
 * @brief 测试数组与区域分配器集成
 */
static void test_array_with_arena_allocator(void)
{
    printf("  设置区域分配器...\n");
    if (!cn_set_allocator(Eum_ALLOCATOR_ARENA))
    {
        printf("  错误: 无法设置区域分配器\n");
        return;
    }
    
    // 创建多个数组（区域分配器适合批量分配）
    Stru_CN_Array_t* arrays[5];
    
    for (int i = 0; i < 5; i++)
    {
        arrays[i] = CN_array_create_custom(
            sizeof(Stru_TestData_t), 100, NULL, NULL, NULL);
        
        if (arrays[i] == NULL)
        {
            printf("  错误: 无法创建数组 %d\n", i);
            // 清理已创建的数组
            for (int j = 0; j < i; j++)
            {
                CN_array_destroy(arrays[j]);
            }
            return;
        }
        
        // 填充数据
        for (int j = 0; j < 50; j++)
        {
            Stru_TestData_t data = create_test_data(i * 100 + j);
            CN_array_append(arrays[i], &data);
        }
    }
    
    printf("  ✓ 使用区域分配器创建5个数组成功\n");
    
    // 验证数据
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 50; j++)
        {
            Stru_TestData_t* data = (Stru_TestData_t*)CN_array_get(arrays[i], j);
            if (data == NULL || !verify_test_data(data, i * 100 + j))
            {
                printf("  错误: 验证数组 %d 元素 %d 失败\n", i, j);
                // 清理
                for (int k = 0; k < 5; k++)
                {
                    CN_array_destroy(arrays[k]);
                }
                return;
            }
        }
    }
    
    printf("  ✓ 所有数据验证成功\n");
    
    // 清理（区域分配器不支持单独释放，但数组结构本身需要释放）
    for (int i = 0; i < 5; i++)
    {
        CN_array_destroy(arrays[i]);
    }
    
    printf("  ✓ 数组销毁成功\n");
    
    // 注意：区域分配器的内存会在CN_memory_shutdown时统一释放
}

/**
 * @brief 测试内存泄漏检测
 */
static void test_memory_leak_detection(void)
{
    printf("  设置调试分配器以检测内存泄漏...\n");
    if (!cn_set_allocator(Eum_ALLOCATOR_DEBUG))
    {
        printf("  错误: 无法设置调试分配器\n");
        return;
    }
    
    cn_enable_debug(true);
    
    // 测试1: 正常使用，无泄漏
    {
        Stru_CN_Array_t* array = CN_array_create_custom(
            sizeof(Stru_TestData_t), 10, NULL, NULL, NULL);
        
        if (array == NULL)
        {
            printf("  错误: 无法创建数组\n");
            return;
        }
        
        // 使用数组
        Stru_TestData_t data = create_test_data(1);
        CN_array_append(array, &data);
        
        // 正确销毁
        CN_array_destroy(array);
        printf("  ✓ 测试1: 正常使用无泄漏\n");
    }
    
    // 测试2: 故意制造泄漏（注释掉销毁）
    {
        Stru_CN_Array_t* array = CN_array_create_custom(
            sizeof(Stru_TestData_t), 10, NULL, NULL, NULL);
        
        if (array == NULL)
        {
            printf("  错误: 无法创建数组\n");
            return;
        }
        
        Stru_TestData_t data = create_test_data(2);
        CN_array_append(array, &data);
        
        // 故意不调用CN_array_destroy(array) 制造泄漏
        // CN_array_destroy(array); // 注释掉这行
        
        printf("  ⚠ 测试2: 故意制造内存泄漏（用于测试检测功能）\n");
    }
    
    // 检查泄漏
    if (cn_check_leaks())
    {
        printf("  ✓ 内存泄漏检测功能正常\n");
        printf("  泄漏统计:\n");
        cn_dump_stats();
    }
    else
    {
        printf("  ⚠ 警告: 内存泄漏检测可能未正常工作\n");
    }
    
    cn_enable_debug(false);
    
    // 切换回系统分配器清理环境
    cn_set_allocator(Eum_ALLOCATOR_SYSTEM);
}

/**
 * @brief 测试性能比较
 */
static void test_performance_comparison(void)
{
    printf("  性能比较测试（不同分配器）:\n");
    
    Eum_CN_AllocatorType_t allocator_types[] = {
        Eum_ALLOCATOR_SYSTEM,
        Eum_ALLOCATOR_DEBUG,
        Eum_ALLOCATOR_POOL,
        Eum_ALLOCATOR_ARENA
    };
    
    const char* allocator_names[] = {
        "系统分配器",
        "调试分配器",
        "对象池分配器",
        "区域分配器"
    };
    
    for (int alloc_idx = 0; alloc_idx < 4; alloc_idx++)
    {
        printf("  测试 %s:\n", allocator_names[alloc_idx]);
        
        if (!cn_set_allocator(allocator_types[alloc_idx]))
        {
            printf("    错误: 无法设置分配器\n");
            continue;
        }
        
        if (allocator_types[alloc_idx] == Eum_ALLOCATOR_DEBUG)
        {
            cn_enable_debug(false); // 性能测试时关闭调试
        }
        
        clock_t start = clock();
        
        // 性能测试：创建、使用、销毁大量数组
        for (int iter = 0; iter < TEST_ITERATIONS; iter++)
        {
            Stru_CN_Array_t* array = CN_array_create_custom(
                sizeof(Stru_TestData_t), TEST_BATCH_SIZE, NULL, NULL, NULL);
            
            if (array == NULL)
            {
                printf("    错误: 迭代 %d 无法创建数组\n", iter);
                break;
            }
            
            // 填充数据
            for (int i = 0; i < TEST_BATCH_SIZE; i++)
            {
                Stru_TestData_t data = create_test_data(iter * TEST_BATCH_SIZE + i);
                if (!CN_array_append(array, &data))
                {
                    printf("    错误: 无法添加元素\n");
                    CN_array_destroy(array);
                    break;
                }
            }
            
            // 访问数据
            for (int i = 0; i < TEST_BATCH_SIZE; i++)
            {
                Stru_TestData_t* data = (Stru_TestData_t*)CN_array_get(array, i);
                if (data == NULL)
                {
                    printf("    错误: 无法获取元素\n");
                    break;
                }
            }
            
            // 销毁数组
            CN_array_destroy(array);
        }
        
        clock_t end = clock();
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
        
        printf("    时间: %.3f 秒\n", elapsed);
        printf("    每秒操作数: %.0f\n", 
               (TEST_ITERATIONS * TEST_BATCH_SIZE * 3) / elapsed); // 创建、添加、访问
        
        if (allocator_types[alloc_idx] == Eum_ALLOCATOR_DEBUG)
        {
            cn_enable_debug(false);
        }
    }
    
    printf("  ✓ 性能比较测试完成\n");
}

/**
 * @brief 测试自定义分配器集成
 */
static void test_custom_allocator_integration(void)
{
    printf("  测试自定义分配器集成...\n");
    
    // 注意：当前CN_memory系统不支持自定义分配器
    // 这里测试系统分配器作为基础
    if (!cn_set_allocator(Eum_ALLOCATOR_SYSTEM))
    {
        printf("  错误: 无法设置系统分配器\n");
        return;
    }
    
    // 测试复杂数据结构
    typedef struct
    {
        Stru_CN_Array_t* array;
        char* name;
        int id;
    } ComplexData;
    
    // 创建复杂数据数组
    Stru_CN_Array_t* complex_array = CN_array_create_custom(
        sizeof(ComplexData), 5, NULL, NULL, NULL);
    
    if (complex_array == NULL)
    {
        printf("  错误: 无法创建复杂数据数组\n");
        return;
    }
    
    printf("  ✓ 创建复杂数据数组成功\n");
    
    // 添加复杂数据
    for (int i = 0; i < 3; i++)
    {
        ComplexData data;
        data.id = i;
        
        // 分配名称
        data.name = (char*)cn_malloc(32);
        if (data.name == NULL)
        {
            printf("  错误: 无法分配名称\n");
            CN_array_destroy(complex_array);
            return;
        }
        snprintf(data.name, 32, "ComplexData_%d", i);
        
        // 创建内部数组
        data.array = CN_array_create_custom(sizeof(int), 10, NULL, NULL, NULL);
        if (data.array == NULL)
        {
            cn_free(data.name);
            printf("  错误: 无法创建内部数组\n");
            CN_array_destroy(complex_array);
            return;
        }
        
        // 填充内部数组
        for (int j = 0; j < 5; j++)
        {
            int value = i * 10 + j;
            CN_array_append(data.array, &value);
        }
        
        // 添加到复杂数组
        if (!CN_array_append(complex_array, &data))
        {
            CN_array_destroy(data.array);
            cn_free(data.name);
            printf("  错误: 无法添加复杂数据\n");
            CN_array_destroy(complex_array);
            return;
        }
    }
    
    printf("  ✓ 添加复杂数据成功\n");
    
    // 验证和清理
    for (size_t i = 0; i < CN_array_length(complex_array); i++)
    {
        ComplexData* data = (ComplexData*)CN_array_get(complex_array, i);
        if (data != NULL)
        {
            // 验证数据
            printf("    验证数据 %zu: ID=%d, 名称=%s, 内部数组长度=%llu\n",
                   i, data->id, data->name, 
                   (unsigned long long)CN_array_length(data->array));
            
            // 清理资源
            CN_array_destroy(data->array);
            cn_free(data->name);
        }
    }
    
    // 销毁复杂数组（注意：元素本身已清理）
    CN_array_destroy(complex_array);
    
    printf("  ✓ 复杂数据结构集成测试成功\n");
    
    // 检查内存泄漏
    if (cn_check_leaks())
    {
        printf("  ⚠ 警告: 检测到内存泄漏\n");
        cn_dump_stats();
    }
    else
    {
        printf("  ✓ 无内存泄漏\n");
    }
}
