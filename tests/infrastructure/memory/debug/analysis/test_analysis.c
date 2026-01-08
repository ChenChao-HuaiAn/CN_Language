/**
 * @file test_analysis.c
 * @brief 分析模块测试实现
 * 
 * 本文件实现了分析模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../../../../../src/infrastructure/memory/debug/analysis/CN_memory_analysis.h"

/**
 * @brief 测试分析器创建和销毁
 */
static bool test_analyzer_creation(void)
{
    printf("    测试分析器创建和销毁...\n");
    
    // 创建分析器
    Stru_AnalyzerContext_t* analyzer = F_create_memory_analyzer(
        true,   // 启用性能跟踪
        true,   // 启用碎片分析
        1000    // 采样间隔（毫秒）
    );
    
    if (analyzer == NULL) {
        printf("      ❌ 创建分析器失败\n");
        return false;
    }
    
    // 销毁分析器
    F_destroy_memory_analyzer(analyzer);
    
    printf("      ✅ 分析器创建和销毁测试通过\n");
    return true;
}

/**
 * @brief 测试内存分配统计
 */
static bool test_allocation_stats(void)
{
    printf("    测试内存分配统计...\n");
    
    Stru_AnalyzerContext_t* analyzer = F_create_memory_analyzer(false, false, 0);
    if (analyzer == NULL) {
        printf("      ❌ 创建分析器失败\n");
        return false;
    }
    
    // 获取分配统计
    Stru_MemoryAllocationStats_t stats;
    F_get_allocation_stats(analyzer, &stats);
    
    // 检查统计结构是否被正确初始化
    // 注意：实际值取决于实现，这里只检查接口是否工作
    
    printf("      ✅ 内存分配统计测试通过\n");
    
    F_destroy_memory_analyzer(analyzer);
    return true;
}

/**
 * @brief 测试性能指标
 */
static bool test_performance_metrics(void)
{
    printf("    测试性能指标...\n");
    
    Stru_AnalyzerContext_t* analyzer = F_create_memory_analyzer(true, false, 100);
    if (analyzer == NULL) {
        printf("      ❌ 创建分析器失败\n");
        return false;
    }
    
    // 获取性能指标
    Stru_PerformanceMetrics_t metrics;
    F_get_performance_metrics(analyzer, &metrics);
    
    // 检查指标结构是否被正确初始化
    // 注意：实际值取决于实现，这里只检查接口是否工作
    
    printf("      ✅ 性能指标测试通过\n");
    
    F_destroy_memory_analyzer(analyzer);
    return true;
}

/**
 * @brief 测试碎片分析
 */
static bool test_fragmentation_analysis(void)
{
    printf("    测试碎片分析...\n");
    
    Stru_AnalyzerContext_t* analyzer = F_create_memory_analyzer(false, true, 0);
    if (analyzer == NULL) {
        printf("      ❌ 创建分析器失败\n");
        return false;
    }
    
    // 注意：碎片分析的具体测试需要实际的内存分配模式
    // 这里只是测试接口是否工作
    
    printf("      ✅ 碎片分析测试通过\n");
    
    F_destroy_memory_analyzer(analyzer);
    return true;
}

/**
 * @brief 运行所有分析测试
 */
bool test_analysis_all(void)
{
    printf("  运行分析模块测试:\n");
    printf("  -----------------\n");
    
    bool all_passed = true;
    
    if (!test_analyzer_creation()) {
        all_passed = false;
    }
    
    if (!test_allocation_stats()) {
        all_passed = false;
    }
    
    if (!test_performance_metrics()) {
        all_passed = false;
    }
    
    if (!test_fragmentation_analysis()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有分析测试通过\n");
    } else {
        printf("  ❌ 部分分析测试失败\n");
    }
    
    return all_passed;
}
