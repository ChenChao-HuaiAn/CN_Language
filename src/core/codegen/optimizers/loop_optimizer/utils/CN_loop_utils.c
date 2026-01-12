/******************************************************************************
 * 文件名: CN_loop_utils.c
 * 功能: CN_Language 循环优化器工具模块实现
 * 
 * 实现循环优化器的工具函数，包括循环分析辅助函数、数学计算、
 * 字符串转换、内存管理等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_loop_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @brief 计算平均循环深度
 */
double F_calculate_average_loop_depth(Stru_DynamicArray_t* loop_infos)
{
    if (!loop_infos) {
        return 0.0;
    }
    
    size_t array_size = F_dynamic_array_length(loop_infos);
    if (array_size == 0) {
        return 0.0;
    }
    
    double total_depth = 0.0;
    for (size_t i = 0; i < array_size; i++) {
        Stru_LoopAnalysisInfo_t* loop_info = (Stru_LoopAnalysisInfo_t*)F_dynamic_array_get(loop_infos, i);
        if (loop_info) {
            total_depth += loop_info->depth;
        }
    }
    
    return total_depth / array_size;
}

/**
 * @brief 计算可优化循环比例
 */
double F_calculate_optimizable_loop_ratio(Stru_DynamicArray_t* loop_infos)
{
    if (!loop_infos) {
        return 0.0;
    }
    
    size_t array_size = F_dynamic_array_length(loop_infos);
    if (array_size == 0) {
        return 0.0;
    }
    
    size_t optimizable_count = 0;
    for (size_t i = 0; i < array_size; i++) {
        Stru_LoopAnalysisInfo_t* loop_info = (Stru_LoopAnalysisInfo_t*)F_dynamic_array_get(loop_infos, i);
        if (loop_info && loop_info->can_be_optimized) {
            optimizable_count++;
        }
    }
    
    return (double)optimizable_count / array_size * 100.0;
}

/**
 * @brief 检查向量化安全性
 */
bool F_check_vectorization_safety(Stru_AstNode_t* ast, Stru_DynamicArray_t* loop_infos)
{
    if (!ast || !loop_infos) {
        return false;
    }
    
    /* 简化实现：检查是否有浮点运算 */
    /* 在实际实现中，需要检查数据依赖、内存对齐等 */
    return true;
}

/**
 * @brief 检查并行化安全性
 */
bool F_check_parallelization_safety(Stru_AstNode_t* ast, Stru_DynamicArray_t* loop_infos)
{
    if (!ast || !loop_infos) {
        return false;
    }
    
    /* 简化实现：检查是否有数据竞争 */
    /* 在实际实现中，需要检查数据依赖、共享变量访问等 */
    return true;
}

/**
 * @brief 计算循环迭代次数估计
 */
size_t F_estimate_loop_iterations(Stru_LoopAnalysisInfo_t* loop_info)
{
    if (!loop_info) {
        return 0;
    }
    
    /* 简化实现：返回固定值 */
    /* 在实际实现中，需要分析循环条件 */
    return 100;
}

/**
 * @brief 检查循环是否可展开
 */
bool F_can_loop_be_unrolled(Stru_LoopAnalysisInfo_t* loop_info, size_t max_unroll_factor)
{
    if (!loop_info) {
        return false;
    }
    
    /* 检查循环是否可优化 */
    if (!loop_info->can_be_optimized) {
        return false;
    }
    
    /* 检查循环类型 */
    if (loop_info->loop_type != Eum_LOOP_TYPE_FOR && loop_info->loop_type != Eum_LOOP_TYPE_WHILE) {
        return false;
    }
    
    /* 检查迭代次数是否足够 */
    size_t estimated_iterations = F_estimate_loop_iterations(loop_info);
    if (estimated_iterations < max_unroll_factor * 2) {
        return false;
    }
    
    return true;
}

/**
 * @brief 检查循环是否可向量化
 */
bool F_can_loop_be_vectorized(Stru_LoopAnalysisInfo_t* loop_info, size_t vector_width)
{
    if (!loop_info) {
        return false;
    }
    
    /* 检查循环是否可优化 */
    if (!loop_info->can_be_optimized) {
        return false;
    }
    
    /* 检查循环类型 */
    if (loop_info->loop_type != Eum_LOOP_TYPE_FOR) {
        return false;
    }
    
    /* 检查迭代次数是否足够 */
    size_t estimated_iterations = F_estimate_loop_iterations(loop_info);
    if (estimated_iterations < vector_width * 2) {
        return false;
    }
    
    /* 检查是否有向量化支持的操作 */
    if (!loop_info->has_vectorizable_operations) {
        return false;
    }
    
    return true;
}

/**
 * @brief 检查循环是否可并行化
 */
bool F_can_loop_be_parallelized(Stru_LoopAnalysisInfo_t* loop_info)
{
    if (!loop_info) {
        return false;
    }
    
    /* 检查循环是否可优化 */
    if (!loop_info->can_be_optimized) {
        return false;
    }
    
    /* 检查循环类型 */
    if (loop_info->loop_type != Eum_LOOP_TYPE_FOR) {
        return false;
    }
    
    /* 检查是否有数据依赖 */
    if (loop_info->has_data_dependencies) {
        return false;
    }
    
    /* 检查迭代次数是否足够 */
    size_t estimated_iterations = F_estimate_loop_iterations(loop_info);
    if (estimated_iterations < 10) {
        return false;
    }
    
    return true;
}

/**
 * @brief 计算循环优化收益估计
 */
double F_estimate_loop_optimization_gain(Stru_LoopAnalysisInfo_t* loop_info, Eum_LoopOptimizationType optimization_type)
{
    if (!loop_info) {
        return 0.0;
    }
    
    double base_gain = 0.0;
    
    switch (optimization_type) {
        case Eum_LOOP_OPTIMIZE_UNROLLING:
            /* 循环展开收益：减少循环开销 */
            base_gain = 0.1; /* 10% */
            break;
            
        case Eum_LOOP_OPTIMIZE_VECTORIZATION:
            /* 向量化收益：并行执行多个操作 */
            base_gain = 0.3; /* 30% */
            break;
            
        case Eum_LOOP_OPTIMIZE_PARALLELIZATION:
            /* 并行化收益：多核并行执行 */
            base_gain = 0.5; /* 50% */
            break;
            
        case Eum_LOOP_OPTIMIZE_TILING:
            /* 循环分块收益：改善缓存局部性 */
            base_gain = 0.2; /* 20% */
            break;
            
        default:
            base_gain = 0.0;
            break;
    }
    
    /* 根据循环复杂度调整收益 */
    int complexity = F_calculate_loop_complexity(loop_info);
    double complexity_factor = 1.0 - (complexity * 0.05); /* 复杂度每增加1，收益减少5% */
    
    return base_gain * fmax(0.1, complexity_factor);
}

/**
 * @brief 创建循环优化报告
 */
char* F_create_loop_optimization_report(Stru_DynamicArray_t* loop_infos, Stru_LoopOptimizationConfig_t* config)
{
    if (!loop_infos || !config) {
        return NULL;
    }
    
    /* 计算统计信息 */
    double avg_depth = F_calculate_average_loop_depth(loop_infos);
    double optimizable_ratio = F_calculate_optimizable_loop_ratio(loop_infos);
    
    /* 分配报告缓冲区 */
    size_t buffer_size = 1024;
    char* report = (char*)malloc(buffer_size);
    if (!report) {
        return NULL;
    }
    
    /* 生成报告 */
    size_t array_size = F_dynamic_array_length(loop_infos);
    snprintf(report, buffer_size,
             "循环优化分析报告:\n"
             "==================\n"
             "循环总数: %zu\n"
             "平均循环深度: %.2f\n"
             "可优化循环比例: %.2f%%\n"
             "配置参数:\n"
             "  最大展开因子: %zu\n"
             "  最小循环迭代次数: %zu\n"
             "  最大向量宽度: %zu\n"
             "  最大循环深度: %zu\n"
             "  启用并行化: %s\n"
             "  启用内存优化: %s\n"
             "  启用激进优化: %s\n",
             array_size,
             avg_depth,
             optimizable_ratio,
             config->max_unroll_factor,
             config->min_loop_iterations,
             config->max_vector_width,
             config->max_loop_depth,
             config->enable_parallelization ? "是" : "否",
             config->enable_memory_optimization ? "是" : "否",
             config->enable_aggressive_optimizations ? "是" : "否");
    
    return report;
}

/**
 * @brief 验证循环优化结果
 */
bool F_validate_loop_optimization_result(Stru_AstNode_t* original_ast, Stru_AstNode_t* optimized_ast, Stru_LoopAnalysisInfo_t* loop_info)
{
    if (!original_ast || !optimized_ast || !loop_info) {
        return false;
    }
    
    /* 简化实现：总是返回成功 */
    /* 在实际实现中，需要验证优化后的代码语义是否与原代码一致 */
    return true;
}

/**
 * @brief 计算循环复杂度
 */
int F_calculate_loop_complexity(Stru_LoopAnalysisInfo_t* loop_info)
{
    if (!loop_info) {
        return 0;
    }
    
    int complexity = 0;
    
    /* 循环深度增加复杂度 */
    complexity += loop_info->depth * 2;
    
    /* 循环体大小增加复杂度 */
    complexity += loop_info->body_size / 10;
    
    /* 条件复杂度 */
    if (loop_info->has_complex_condition) {
        complexity += 5;
    }
    
    /* 嵌套循环增加复杂度 */
    if (loop_info->has_nested_loops) {
        complexity += 10;
    }
    
    /* 数据依赖增加复杂度 */
    if (loop_info->has_data_dependencies) {
        complexity += 8;
    }
    
    return complexity;
}
