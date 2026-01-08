/**
 * @file test_memory_alignment.c
 * @brief 内存对齐工具测试
 * 
 * 测试内存对齐工具函数的正确性和性能。
 * 包括对齐计算、对齐检查和辅助函数等功能的测试。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "../../../../../src/infrastructure/memory/utilities/alignment/CN_memory_alignment.h"

/**
 * @brief 测试向上对齐功能
 */
static bool test_align_up(void)
{
    printf("  测试向上对齐... ");
    
    Stru_MemoryAlignmentInterface_t* align = F_get_global_memory_alignment();
    if (align == NULL)
    {
        printf("❌ 无法获取内存对齐接口\n");
        return false;
    }
    
    // 测试2的幂对齐
    size_t result1 = align->align_up(10, 8);
    if (result1 != 16)
    {
        printf("❌ 10向上对齐到8应为16，实际为%zu\n", result1);
        return false;
    }
    
    size_t result2 = align->align_up(16, 16);
    if (result2 != 16)
    {
        printf("❌ 16向上对齐到16应为16，实际为%zu\n", result2);
        return false;
    }
    
    size_t result3 = align->align_up(0, 8);
    if (result3 != 0)
    {
        printf("❌ 0向上对齐到8应为0，实际为%zu\n", result3);
        return false;
    }
    
    // 测试非2的幂对齐（应该返回原始大小）
    size_t result4 = align->align_up(10, 7);
    if (result4 != 10)
    {
        printf("❌ 非2的幂对齐应返回原始大小，实际为%zu\n", result4);
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试向下对齐功能
 */
static bool test_align_down(void)
{
    printf("  测试向下对齐... ");
    
    Stru_MemoryAlignmentInterface_t* align = F_get_global_memory_alignment();
    if (align == NULL)
    {
        printf("❌ 无法获取内存对齐接口\n");
        return false;
    }
    
    // 测试2的幂对齐
    size_t result1 = align->align_down(10, 8);
    if (result1 != 8)
    {
        printf("❌ 10向下对齐到8应为8，实际为%zu\n", result1);
        return false;
    }
    
    size_t result2 = align->align_down(16, 16);
    if (result2 != 16)
    {
        printf("❌ 16向下对齐到16应为16，实际为%zu\n", result2);
        return false;
    }
    
    size_t result3 = align->align_down(7, 8);
    if (result3 != 0)
    {
        printf("❌ 7向下对齐到8应为0，实际为%zu\n", result3);
        return false;
    }
    
    // 测试非2的幂对齐（应该返回原始大小）
    size_t result4 = align->align_down(10, 7);
    if (result4 != 10)
    {
        printf("❌ 非2的幂对齐应返回原始大小，实际为%zu\n", result4);
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试对齐检查功能
 */
static bool test_is_aligned(void)
{
    printf("  测试对齐检查... ");
    
    Stru_MemoryAlignmentInterface_t* align = F_get_global_memory_alignment();
    if (align == NULL)
    {
        printf("❌ 无法获取内存对齐接口\n");
        return false;
    }
    
    // 测试对齐的指针
    char aligned_buffer[64] __attribute__((aligned(16)));
    bool result1 = align->is_aligned(aligned_buffer, 16);
    if (!result1)
    {
        printf("❌ 16字节对齐指针检查失败\n");
        return false;
    }
    
    // 测试不对齐的指针
    char* unaligned_ptr = aligned_buffer + 1;
    bool result2 = align->is_aligned(unaligned_ptr, 16);
    if (result2)
    {
        printf("❌ 非16字节对齐指针检查错误\n");
        return false;
    }
    
    // 测试NULL指针
    bool result3 = align->is_aligned(NULL, 16);
    if (result3)
    {
        printf("❌ NULL指针对齐检查错误\n");
        return false;
    }
    
    // 测试非2的幂对齐
    bool result4 = align->is_aligned(aligned_buffer, 7);
    if (result4)
    {
        printf("❌ 非2的幂对齐检查错误\n");
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试对齐分配辅助功能
 */
static bool test_aligned_alloc_helper(void)
{
    printf("  测试对齐分配辅助... ");
    
    Stru_MemoryAlignmentInterface_t* align = F_get_global_memory_alignment();
    if (align == NULL)
    {
        printf("❌ 无法获取内存对齐接口\n");
        return false;
    }
    
    // 测试有效对齐分配
    void* ptr1 = align->aligned_alloc_helper(100, 16);
    if (ptr1 == NULL)
    {
        printf("❌ 对齐分配失败\n");
        return false;
    }
    
    // 检查对齐
    if (!align->is_aligned(ptr1, 16))
    {
        printf("❌ 分配的内存未正确对齐\n");
        #ifdef _WIN32
        _aligned_free(ptr1);
        #else
        free(ptr1);
        #endif
        return false;
    }
    
    // 测试零大小
    void* ptr2 = align->aligned_alloc_helper(0, 16);
    if (ptr2 != NULL)
    {
        printf("❌ 零大小分配应返回NULL\n");
        #ifdef _WIN32
        _aligned_free(ptr2);
        #else
        free(ptr2);
        #endif
        return false;
    }
    
    // 测试非2的幂对齐
    void* ptr3 = align->aligned_alloc_helper(100, 7);
    if (ptr3 != NULL)
    {
        printf("❌ 非2的幂对齐分配应返回NULL\n");
        #ifdef _WIN32
        _aligned_free(ptr3);
        #else
        free(ptr3);
        #endif
        return false;
    }
    
    // 清理
    #ifdef _WIN32
    _aligned_free(ptr1);
    #else
    free(ptr1);
    #endif
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试填充大小计算功能
 */
static bool test_padding_size(void)
{
    printf("  测试填充大小计算... ");
    
    Stru_MemoryAlignmentInterface_t* align = F_get_global_memory_alignment();
    if (align == NULL)
    {
        printf("❌ 无法获取内存对齐接口\n");
        return false;
    }
    
    // 测试需要填充的情况
    size_t result1 = align->padding_size(10, 8);
    if (result1 != 6) // 10 + 6 = 16
    {
        printf("❌ 10对齐到8需要6字节填充，实际为%zu\n", result1);
        return false;
    }
    
    // 测试已经对齐的情况
    size_t result2 = align->padding_size(16, 8);
    if (result2 != 0)
    {
        printf("❌ 16对齐到8需要0字节填充，实际为%zu\n", result2);
        return false;
    }
    
    // 测试非2的幂对齐
    size_t result3 = align->padding_size(10, 7);
    if (result3 != 0)
    {
        printf("❌ 非2的幂对齐应返回0，实际为%zu\n", result3);
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 运行所有内存对齐测试
 */
bool test_memory_alignment_all(void)
{
    printf("\n运行内存对齐工具测试\n");
    printf("====================\n\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行各个测试
    if (test_align_up()) passed++;
    total++;
    if (test_align_down()) passed++;
    total++;
    if (test_is_aligned()) passed++;
    total++;
    if (test_aligned_alloc_helper()) passed++;
    total++;
    if (test_padding_size()) passed++;
    total++;
    
    printf("\n测试结果: %d/%d 通过\n", passed, total);
    
    return passed == total;
}

/**
 * @brief 测试主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_memory_alignment_all())
    {
        printf("\n✅ 所有内存对齐测试通过！\n");
        return 0;
    }
    else
    {
        printf("\n❌ 部分内存对齐测试失败！\n");
        return 1;
    }
}
#endif
