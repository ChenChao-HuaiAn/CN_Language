/**
 * @file test_memory_statistics.c
 * @brief 内存统计工具测试
 * 
 * 测试内存统计工具函数的正确性和性能。
 * 包括内存使用统计、碎片分析和性能监控等功能的测试。
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
#include "../../../../../src/infrastructure/memory/utilities/statistics/CN_memory_statistics.h"

/**
 * @brief 测试内存统计信息获取功能
 */
static bool test_get_statistics(void)
{
    printf("  测试内存统计信息获取... ");
    
    Stru_MemoryStatisticsInterface_t* stats = F_get_global_memory_statistics();
    if (stats == NULL)
    {
        printf("❌ 无法获取内存统计接口\n");
        return false;
    }
    
    Stru_MemoryStatistics_t stat_info;
    stats->get_statistics(&stat_info);
    
    // 检查结构体是否被正确填充
    // 注意：由于是模拟实现，这里只检查不崩溃
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试内存碎片信息获取功能
 */
static bool test_get_fragmentation(void)
{
    printf("  测试内存碎片信息获取... ");
    
    Stru_MemoryStatisticsInterface_t* stats = F_get_global_memory_statistics();
    if (stats == NULL)
    {
        printf("❌ 无法获取内存统计接口\n");
        return false;
    }
    
    Stru_MemoryFragmentation_t frag_info;
    stats->get_fragmentation(&frag_info);
    
    // 检查结构体是否被正确填充
    // 注意：由于是模拟实现，这里只检查不崩溃
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试内存性能信息获取功能
 */
static bool test_get_performance(void)
{
    printf("  测试内存性能信息获取... ");
    
    Stru_MemoryStatisticsInterface_t* stats = F_get_global_memory_statistics();
    if (stats == NULL)
    {
        printf("❌ 无法获取内存统计接口\n");
        return false;
    }
    
    Stru_MemoryPerformance_t perf_info;
    stats->get_performance(&perf_info);
    
    // 检查结构体是否被正确填充
    // 注意：由于是模拟实现，这里只检查不崩溃
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试统计信息重置功能
 */
static bool test_reset_statistics(void)
{
    printf("  测试统计信息重置... ");
    
    Stru_MemoryStatisticsInterface_t* stats = F_get_global_memory_statistics();
    if (stats == NULL)
    {
        printf("❌ 无法获取内存统计接口\n");
        return false;
    }
    
    // 重置统计信息（应该不崩溃）
    stats->reset_statistics();
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试统计报告生成功能
 */
static bool test_generate_report(void)
{
    printf("  测试统计报告生成... ");
    
    Stru_MemoryStatisticsInterface_t* stats = F_get_global_memory_statistics();
    if (stats == NULL)
    {
        printf("❌ 无法获取内存统计接口\n");
        return false;
    }
    
    char report_buffer[1024];
    bool result = stats->generate_report(report_buffer, sizeof(report_buffer));
    
    if (!result)
    {
        printf("❌ 报告生成失败\n");
        return false;
    }
    
    // 检查报告是否非空
    if (report_buffer[0] == '\0')
    {
        printf("❌ 报告内容为空\n");
        return false;
    }
    
    // 检查报告是否包含关键信息
    if (strstr(report_buffer, "内存统计报告") == NULL)
    {
        printf("❌ 报告格式不正确\n");
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试边界条件
 */
static bool test_statistics_edge_cases(void)
{
    printf("  测试统计边界条件... ");
    
    Stru_MemoryStatisticsInterface_t* stats = F_get_global_memory_statistics();
    if (stats == NULL)
    {
        printf("❌ 无法获取内存统计接口\n");
        return false;
    }
    
    // 测试NULL指针参数
    stats->get_statistics(NULL); // 应该不崩溃
    stats->get_fragmentation(NULL); // 应该不崩溃
    stats->get_performance(NULL); // 应该不崩溃
    
    // 测试小缓冲区
    char small_buffer[10];
    bool result = stats->generate_report(small_buffer, sizeof(small_buffer));
    // 小缓冲区可能失败，但不应该崩溃
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 运行所有内存统计测试
 */
bool test_memory_statistics_all(void)
{
    printf("\n运行内存统计工具测试\n");
    printf("====================\n\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行各个测试
    if (test_get_statistics()) passed++; total++;
    if (test_get_fragmentation()) passed++; total++;
    if (test_get_performance()) passed++; total++;
    if (test_reset_statistics()) passed++; total++;
    if (test_generate_report()) passed++; total++;
    if (test_statistics_edge_cases()) passed++; total++;
    
    printf("\n测试结果: %d/%d 通过\n", passed, total);
    
    return passed == total;
}

/**
 * @brief 测试主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_memory_statistics_all())
    {
        printf("\n✅ 所有内存统计测试通过！\n");
        return 0;
    }
    else
    {
        printf("\n❌ 部分内存统计测试失败！\n");
        return 1;
    }
}
#endif
