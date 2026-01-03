/**
 * @file test_dma_allocator.c
 * @brief DMA分配器测试
 * 
 * 测试CN_dma_allocator模块的核心功能。
 * 包括：基本分配、物理连续内存、缓存一致性等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-03
 * @version 1.0.0
 * @license MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "../../../../src/infrastructure/memory/dma/CN_dma_allocator.h"

/* 测试配置 */
#define TEST_DMA_REGION_SIZE (4 * 1024 * 1024)  /* 4MB DMA区域 */
#define TEST_PAGE_SIZE 4096                     /* 4KB页大小 */
#define TEST_CACHE_LINE_SIZE 64                 /* 64字节缓存行 */

/* 测试函数声明 */
static void test_basic_functionality(void);
static void test_physical_contiguous_memory(void);
static void test_memory_attributes(void);
static void test_statistics_and_debug(void);
static void test_error_handling(void);

/**
 * @brief 主测试函数
 */
int main(void)
{
    printf("=== CN_dma_allocator 测试开始 ===\n\n");
    
    /* 运行所有测试 */
    test_basic_functionality();
    test_physical_contiguous_memory();
    test_memory_attributes();
    test_statistics_and_debug();
    test_error_handling();
    
    printf("\n=== CN_dma_allocator 测试完成 ===\n");
    printf("所有测试通过！\n");
    
    return EXIT_SUCCESS;
}

/**
 * @brief 测试基本功能
 */
static void test_basic_functionality(void)
{
    printf("1. 测试基本功能...\n");
    
    /* 创建DMA分配器配置 */
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x10000000;  /* 模拟DMA区域起始地址 */
    config.dma_region_end = 0x10000000 + TEST_DMA_REGION_SIZE;
    config.page_size = TEST_PAGE_SIZE;
    config.cache_line_size = TEST_CACHE_LINE_SIZE;
    config.enable_statistics = true;
    config.enable_debug = false;
    config.zero_on_alloc = true;
    config.track_allocations = true;
    config.default_attribute = Eum_DMA_ATTRIBUTE_UNCACHED;
    config.name = "测试DMA分配器";
    config.support_scatter_gather = false;
    config.require_physical_contiguous = false;
    config.max_scatter_elements = 16;
    
    /* 创建DMA分配器 */
    Stru_CN_DmaAllocator_t* dma = CN_dma_create(&config);
    assert(dma != NULL);
    printf("  ✓ DMA分配器创建成功\n");
    
    /* 测试基本缓冲区分配 */
    Stru_CN_DmaBuffer_t* buffer1 = CN_dma_alloc_buffer(dma, 1024, 
        TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "测试缓冲区1");
    
    assert(buffer1 != NULL);
    assert(buffer1->virtual_address != NULL);
    assert(buffer1->size >= 1024);
    printf("  ✓ 基本缓冲区分配成功\n");
    
    /* 使用缓冲区 */
    memset(buffer1->virtual_address, 0xAA, 1024);
    uint8_t* data = (uint8_t*)buffer1->virtual_address;
    for (int i = 0; i < 1024; i++) {
        assert(data[i] == 0xAA);
    }
    printf("  ✓ 缓冲区使用正常\n");
    
    /* 测试不同大小的分配 */
    Stru_CN_DmaBuffer_t* small_buffer = CN_dma_alloc_buffer(dma, 64, 
        TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "小缓冲区");
    
    Stru_CN_DmaBuffer_t* large_buffer = CN_dma_alloc_buffer(dma, 64 * 1024, 
        TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "大缓冲区");
    
    assert(small_buffer != NULL);
    assert(large_buffer != NULL);
    printf("  ✓ 不同大小缓冲区分配成功\n");
    
    /* 测试缓冲区释放 */
    CN_dma_free_buffer(dma, buffer1, __FILE__, __LINE__);
    printf("  ✓ 缓冲区释放成功\n");
    
    /* 清理 */
    CN_dma_free_buffer(dma, small_buffer, __FILE__, __LINE__);
    CN_dma_free_buffer(dma, large_buffer, __FILE__, __LINE__);
    
    /* 销毁DMA分配器 */
    CN_dma_destroy(dma);
    printf("  ✓ DMA分配器销毁成功\n");
    
    printf("  基本功能测试完成\n\n");
}

/**
 * @brief 测试物理连续内存
 */
static void test_physical_contiguous_memory(void)
{
    printf("2. 测试物理连续内存...\n");
    
    /* 创建DMA分配器配置 */
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x20000000;  /* 模拟DMA区域起始地址 */
    config.dma_region_end = 0x20000000 + TEST_DMA_REGION_SIZE;
    config.page_size = TEST_PAGE_SIZE;
    config.cache_line_size = TEST_CACHE_LINE_SIZE;
    config.enable_statistics = true;
    config.require_physical_contiguous = true;
    config.default_attribute = Eum_DMA_ATTRIBUTE_UNCACHED;
    
    /* 创建DMA分配器 */
    Stru_CN_DmaAllocator_t* dma = CN_dma_create(&config);
    assert(dma != NULL);
    
    /* 测试物理连续缓冲区分配 */
    Stru_CN_DmaBuffer_t* phys_buffer = CN_dma_alloc_contiguous(dma, 
        4 * TEST_PAGE_SIZE,  /* 4页大小 */
        TEST_PAGE_SIZE,      /* 页面对齐 */
        Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "物理连续缓冲区");
    
    assert(phys_buffer != NULL);
    assert(phys_buffer->virtual_address != NULL);
    assert(phys_buffer->physical_address != 0);
    assert(phys_buffer->size >= 4 * TEST_PAGE_SIZE);
    assert(phys_buffer->is_contiguous == true);
    printf("  ✓ 物理连续缓冲区分配成功\n");
    
    /* 验证物理地址对齐 */
    assert(CN_dma_is_page_aligned(phys_buffer->physical_address, TEST_PAGE_SIZE));
    printf("  ✓ 物理地址页对齐正常\n");
    
    /* 测试同步操作 */
    CN_dma_sync_to_device(phys_buffer, 0, 0);  /* 同步整个缓冲区 */
    printf("  ✓ 缓冲区同步到设备成功\n");
    
    CN_dma_sync_from_device(phys_buffer, 0, 0);  /* 从设备同步 */
    printf("  ✓ 缓冲区从设备同步成功\n");
    
    /* 清理 */
    CN_dma_free_buffer(dma, phys_buffer, __FILE__, __LINE__);
    
    /* 销毁DMA分配器 */
    CN_dma_destroy(dma);
    printf("  物理连续内存测试完成\n\n");
}

/**
 * @brief 测试内存属性
 */
static void test_memory_attributes(void)
{
    printf("3. 测试内存属性...\n");
    
    /* 创建DMA分配器配置 */
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x30000000;  /* 模拟DMA区域起始地址 */
    config.dma_region_end = 0x30000000 + TEST_DMA_REGION_SIZE;
    config.page_size = TEST_PAGE_SIZE;
    config.cache_line_size = TEST_CACHE_LINE_SIZE;
    config.enable_statistics = true;
    
    /* 创建DMA分配器 */
    Stru_CN_DmaAllocator_t* dma = CN_dma_create(&config);
    assert(dma != NULL);
    
    /* 测试不同内存属性的缓冲区 */
    Stru_CN_DmaBuffer_t* cached_buffer = CN_dma_alloc_buffer(dma, 1024, 
        TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_NORMAL,
        __FILE__, __LINE__, "缓存缓冲区");
    
    Stru_CN_DmaBuffer_t* uncached_buffer = CN_dma_alloc_buffer(dma, 1024,
        TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "不可缓存缓冲区");
    
    Stru_CN_DmaBuffer_t* device_buffer = CN_dma_alloc_buffer(dma, 1024,
        TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_DEVICE,
        __FILE__, __LINE__, "设备内存缓冲区");
    
    assert(cached_buffer != NULL);
    assert(uncached_buffer != NULL);
    assert(device_buffer != NULL);
    printf("  ✓ 不同内存属性缓冲区分配成功\n");
    
    /* 验证内存属性 */
    assert(cached_buffer->attribute == Eum_DMA_ATTRIBUTE_NORMAL);
    assert(uncached_buffer->attribute == Eum_DMA_ATTRIBUTE_UNCACHED);
    assert(device_buffer->attribute == Eum_DMA_ATTRIBUTE_DEVICE);
    printf("  ✓ 内存属性设置正确\n");
    
    /* 清理 */
    CN_dma_free_buffer(dma, cached_buffer, __FILE__, __LINE__);
    CN_dma_free_buffer(dma, uncached_buffer, __FILE__, __LINE__);
    CN_dma_free_buffer(dma, device_buffer, __FILE__, __LINE__);
    
    /* 销毁DMA分配器 */
    CN_dma_destroy(dma);
    printf("  内存属性测试完成\n\n");
}

/**
 * @brief 测试统计和调试功能
 */
static void test_statistics_and_debug(void)
{
    printf("4. 测试统计和调试功能...\n");
    
    /* 创建DMA分配器配置 */
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x40000000;  /* 模拟DMA区域起始地址 */
    config.dma_region_end = 0x40000000 + TEST_DMA_REGION_SIZE;
    config.page_size = TEST_PAGE_SIZE;
    config.cache_line_size = TEST_CACHE_LINE_SIZE;
    config.enable_statistics = true;
    config.enable_debug = true;
    config.track_allocations = true;
    
    /* 创建DMA分配器 */
    Stru_CN_DmaAllocator_t* dma = CN_dma_create(&config);
    assert(dma != NULL);
    
    /* 获取初始统计信息 */
    Stru_CN_DmaStats_t stats;
    bool get_stats_result = CN_dma_get_stats(dma, &stats);
    assert(get_stats_result == true);
    printf("  ✓ 获取统计信息成功\n");
    
    /* 分配一些缓冲区 */
    Stru_CN_DmaBuffer_t* buffers[5];
    for (int i = 0; i < 5; i++) {
        buffers[i] = CN_dma_alloc_buffer(dma, 512 * (i + 1),
            TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_UNCACHED,
            __FILE__, __LINE__, "统计测试缓冲区");
        assert(buffers[i] != NULL);
    }
    printf("  ✓ 分配多个缓冲区成功\n");
    
    /* 获取更新后的统计信息 */
    get_stats_result = CN_dma_get_stats(dma, &stats);
    assert(get_stats_result == true);
    assert(stats.allocation_count >= 5);
    printf("  ✓ 统计信息更新正确\n");
    
    /* 测试获取配置 */
    Stru_CN_DmaConfig_t retrieved_config;
    bool get_config_result = CN_dma_get_config(dma, &retrieved_config);
    assert(get_config_result == true);
    assert(strcmp(retrieved_config.name, config.name) == 0);
    printf("  ✓ 获取配置信息成功\n");
    
    /* 测试验证功能 */
    bool validate_result = CN_dma_validate(dma);
    assert(validate_result == true);
    printf("  ✓ 分配器验证成功\n");
    
    /* 测试转储功能（不检查输出，只确保不崩溃） */
    CN_dma_dump(dma);
    printf("  ✓ 状态转储执行成功\n");
    
    /* 清理 */
    for (int i = 0; i < 5; i++) {
        CN_dma_free_buffer(dma, buffers[i], __FILE__, __LINE__);
    }
    
    /* 测试重置统计 */
    CN_dma_reset_stats(dma);
    printf("  ✓ 统计信息重置成功\n");
    
    /* 销毁DMA分配器 */
    CN_dma_destroy(dma);
    printf("  统计和调试功能测试完成\n\n");
}

/**
 * @brief 测试错误处理
 */
static void test_error_handling(void)
{
    printf("5. 测试错误处理...\n");
    
    /* 测试无效配置 */
    Stru_CN_DmaConfig_t invalid_config = CN_DMA_CONFIG_DEFAULT;
    invalid_config.dma_region_start = 0x50000000;
    invalid_config.dma_region_end = 0x50000000;  /* 结束地址等于起始地址 */
    
    Stru_CN_DmaAllocator_t* dma = CN_dma_create(&invalid_config);
    assert(dma == NULL);
    printf("  ✓ 无效配置检测正常\n");
    
    /* 创建有效DMA分配器 */
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x50000000;
    config.dma_region_end = 0x50000000 + TEST_DMA_REGION_SIZE;
    config.page_size = TEST_PAGE_SIZE;
    config.cache_line_size = TEST_CACHE_LINE_SIZE;
    
    dma = CN_dma_create(&config);
    assert(dma != NULL);
    
    /* 测试超大分配（应该失败） */
    Stru_CN_DmaBuffer_t* huge_buffer = CN_dma_alloc_buffer(dma, 
        TEST_DMA_REGION_SIZE * 2,  /* 超过DMA区域大小 */
        TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "超大缓冲区");
    
    assert(huge_buffer == NULL);
    printf("  ✓ 超大分配失败处理正常\n");
    
    /* 测试零大小分配（应该失败或返回最小缓冲区） */
    Stru_CN_DmaBuffer_t* zero_buffer = CN_dma_alloc_buffer(dma, 0,
        TEST_CACHE_LINE_SIZE, Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "零大小缓冲区");
    
    if (zero_buffer == NULL) {
        printf("  ✓ 零大小分配失败处理正常\n");
    } else {
        CN_dma_free_buffer(dma, zero_buffer, __FILE__, __LINE__);
        printf("  ⚠ 零大小分配返回了缓冲区（实现特定）\n");
    }
    
    /* 测试无效对齐（非2的幂次方） */
    Stru_CN_DmaBuffer_t* bad_align_buffer = CN_dma_alloc_buffer(dma, 1024,
        30,  /* 不是2的幂次方 */
        Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "错误对齐缓冲区");
    
    if (bad_align_buffer == NULL) {
        printf("  ✓ 无效对齐检测正常\n");
    } else {
        /* 实现可能向上对齐到最近的2的幂次方 */
        CN_dma_free_buffer(dma, bad_align_buffer, __FILE__, __LINE__);
        printf("  ⚠ 无效对齐被自动修正（实现特定）\n");
    }
    
    /* 销毁DMA分配器 */
    CN_dma_destroy(dma);
    printf("  错误处理测试完成\n\n");
}
