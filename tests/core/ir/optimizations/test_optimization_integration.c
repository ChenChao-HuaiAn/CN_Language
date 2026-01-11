/******************************************************************************
 * 文件名: test_optimization_integration.c
 * 功能: CN_Language 优化集成端到端测试
 
 * 测试优化集成功能，包括：
 * 1. TAC优化器的各种优化算法
 * 2. 优化效果评估器
 * 3. 优化集成流程
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../../../../src/core/ir/CN_ir_optimizer.h"
#include "../../../../src/core/ir/optimizations/CN_tac_optimizer.h"
#include "../../../../src/core/ir/optimizations/CN_optimization_evaluator.h"
#include "../../../../src/core/ir/implementations/tac/CN_tac_interface.h"
#include "../../../../src/infrastructure/memory/utilities/CN_memory_utilities.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 创建测试TAC数据
 * 
 * 创建包含简单算术表达式的TAC数据用于测试。
 * 
 * @return Stru_TacData_t* 测试TAC数据
 */
static Stru_TacData_t* create_test_tac_data(void)
{
    Stru_TacData_t* tac_data = F_create_tac_data();
    if (!tac_data)
    {
        return NULL;
    }
    
    // 添加一些测试指令
    // 示例: a = 5 + 3 * 2
    Stru_TacInstruction_t* instr1 = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN,
        "t1",
        "3",
        NULL,
        NULL,
        1,
        1
    );
    
    Stru_TacInstruction_t* instr2 = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN,
        "t2",
        "2",
        NULL,
        NULL,
        2,
        1
    );
    
    Stru_TacInstruction_t* instr3 = F_create_tac_instruction(
        Eum_TAC_OP_MUL,
        "t3",
        "t1",
        "t2",
        NULL,
        3,
        1
    );
    
    Stru_TacInstruction_t* instr4 = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN,
        "t4",
        "5",
        NULL,
        NULL,
        4,
        1
    );
    
    Stru_TacInstruction_t* instr5 = F_create_tac_instruction(
        Eum_TAC_OP_ADD,
        "a",
        "t4",
        "t3",
        NULL,
        5,
        1
    );
    
    // 添加一些死代码
    Stru_TacInstruction_t* instr6 = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN,
        "dead_var",
        "10",
        NULL,
        NULL,
        6,
        1
    );
    
    // 添加一些公共子表达式
    Stru_TacInstruction_t* instr7 = F_create_tac_instruction(
        Eum_TAC_OP_ADD,
        "b",
        "t4",
        "t3",
        NULL,
        7,
        1
    );
    
    if (instr1) F_tac_data_add_instruction(tac_data, instr1);
    if (instr2) F_tac_data_add_instruction(tac_data, instr2);
    if (instr3) F_tac_data_add_instruction(tac_data, instr3);
    if (instr4) F_tac_data_add_instruction(tac_data, instr4);
    if (instr5) F_tac_data_add_instruction(tac_data, instr5);
    if (instr6) F_tac_data_add_instruction(tac_data, instr6);
    if (instr7) F_tac_data_add_instruction(tac_data, instr7);
    
    return tac_data;
}

/**
 * @brief 打印TAC指令
 * 
 * 打印TAC数据中的所有指令。
 * 
 * @param tac_data TAC数据
 * @param label 标签（如"原始TAC"或"优化后TAC"）
 */
static void print_tac_instructions(Stru_TacData_t* tac_data, const char* label)
{
    if (!tac_data || !label)
    {
        return;
    }
    
    printf("\n=== %s ===\n", label);
    
    size_t count = F_tac_data_get_instruction_count(tac_data);
    for (size_t i = 0; i < count; i++)
    {
        Stru_TacInstruction_t* instr = F_tac_data_get_instruction(tac_data, i);
        if (instr)
        {
            printf("指令 %zu: ", i + 1);
            
            switch (instr->opcode)
            {
                case Eum_TAC_OP_ASSIGN:
                    printf("%s = %s", instr->result, instr->operand1);
                    break;
                case Eum_TAC_OP_ADD:
                    printf("%s = %s + %s", instr->result, instr->operand1, instr->operand2);
                    break;
                case Eum_TAC_OP_SUB:
                    printf("%s = %s - %s", instr->result, instr->operand1, instr->operand2);
                    break;
                case Eum_TAC_OP_MUL:
                    printf("%s = %s * %s", instr->result, instr->operand1, instr->operand2);
                    break;
                case Eum_TAC_OP_DIV:
                    printf("%s = %s / %s", instr->result, instr->operand1, instr->operand2);
                    break;
                case Eum_TAC_OP_SHL:
                    printf("%s = %s << %s", instr->result, instr->operand1, instr->operand2);
                    break;
                default:
                    printf("未知操作码: %d", instr->opcode);
                    break;
            }
            
            printf(" (行: %zu, 列: %zu)\n", instr->line, instr->column);
        }
    }
    
    printf("总指令数: %zu\n", count);
}

/**
 * @brief 验证优化结果
 * 
 * 验证优化结果是否有效。
 * 
 * @param result 优化结果
 * @param expected_improvement 期望的改进比例
 * @return bool 验证通过返回true，否则返回false
 */
static bool validate_optimization_result(Stru_IrOptimizationResult_t* result, 
                                        double expected_improvement)
{
    if (!result)
    {
        printf("错误: 优化结果为空\n");
        return false;
    }
    
    if (!result->success)
    {
        printf("错误: 优化失败: %s\n", result->error_message ? result->error_message : "未知错误");
        return false;
    }
    
    if (result->original_instruction_count == 0)
    {
        printf("错误: 原始指令数为0\n");
        return false;
    }
    
    printf("优化成功:\n");
    printf("  原始指令数: %zu\n", result->original_instruction_count);
    printf("  优化后指令数: %zu\n", result->optimized_instruction_count);
    printf("  移除的指令数: %zu\n", result->removed_instructions);
    printf("  添加的指令数: %zu\n", result->added_instructions);
    printf("  改进比例: %.2f%%\n", result->improvement_ratio * 100);
    printf("  优化时间: %.6f秒\n", result->optimization_time);
    
    // 检查改进比例是否合理
    if (result->improvement_ratio < 0.0 || result->improvement_ratio > 1.0)
    {
        printf("警告: 改进比例超出合理范围: %.2f\n", result->improvement_ratio);
    }
    
    // 检查是否达到期望的改进
    if (expected_improvement > 0.0 && result->improvement_ratio < expected_improvement)
    {
        printf("警告: 改进比例低于期望值 (期望: %.2f%%, 实际: %.2f%%)\n", 
               expected_improvement * 100, result->improvement_ratio * 100);
    }
    
    return true;
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试TAC常量折叠优化
 */
static void test_tac_constant_folding(void)
{
    printf("\n=== 测试TAC常量折叠优化 ===\n");
    
    // 创建测试TAC数据
    Stru_TacData_t* tac_data = create_test_tac_data();
    if (!tac_data)
    {
        printf("错误: 无法创建测试TAC数据\n");
        return;
    }
    
    print_tac_instructions(tac_data, "原始TAC");
    
    // 应用常量折叠优化
    Stru_IrOptimizationConfig_t* config = F_create_tac_default_config();
    Stru_IrOptimizationResult_t* result = F_apply_tac_constant_folding(tac_data, config);
    
    // 验证结果
    bool success = validate_optimization_result(result, 0.05); // 期望至少5%的改进
    
    if (success && result->optimized_ir)
    {
        Stru_TacData_t* optimized_tac = (Stru_TacData_t*)result->optimized_ir;
        print_tac_instructions(optimized_tac, "常量折叠优化后TAC");
    }
    
    // 清理资源
    if (result) {
        // 注意：这里应该使用优化器接口的destroy_optimization_result函数
        // 但由于我们没有优化器实例，我们直接释放内存
        if (result->error_message) cn_free(result->error_message);
        if (result->warning_message) cn_free(result->warning_message);
        if (result->optimized_ir) F_destroy_tac_data((Stru_TacData_t*)result->optimized_ir);
        cn_free(result);
    }
    F_destroy_tac_data(tac_data);
    
    printf("测试%s\n", success ? "通过" : "失败");
}

/**
 * @brief 测试TAC死代码消除优化
 */
static void test_tac_dead_code_elimination(void)
{
    printf("\n=== 测试TAC死代码消除优化 ===\n");
    
    // 创建测试TAC数据
    Stru_TacData_t* tac_data = create_test_tac_data();
    if (!tac_data)
    {
        printf("错误: 无法创建测试TAC数据\n");
        return;
    }
    
    print_tac_instructions(tac_data, "原始TAC");
    
    // 应用死代码消除优化
    Stru_IrOptimizationConfig_t* config = F_create_tac_default_config();
    Stru_IrOptimizationResult_t* result = F_apply_tac_dead_code_elimination(tac_data, config);
    
    // 验证结果
    bool success = validate_optimization_result(result, 0.10); // 期望至少10%的改进
    
    if (success && result->optimized_ir)
    {
        Stru_TacData_t* optimized_tac = (Stru_TacData_t*)result->optimized_ir;
        print_tac_instructions(optimized_tac, "死代码消除优化后TAC");
    }
    
    // 清理资源
    if (result) {
        if (result->error_message) cn_free(result->error_message);
        if (result->warning_message) cn_free(result->warning_message);
        if (result->optimized_ir) F_destroy_tac_data((Stru_TacData_t*)result->optimized_ir);
        cn_free(result);
    }
    F_destroy_tac_data(tac_data);
    
    printf("测试%s\n", success ? "通过" : "失败");
}

/**
 * @brief 测试TAC公共子表达式消除优化
 */
static void test_tac_common_subexpression_elimination(void)
{
    printf("\n=== 测试TAC公共子表达式消除优化 ===\n");
    
    // 创建测试TAC数据
    Stru_TacData_t* tac_data = create_test_tac_data();
    if (!tac_data)
    {
        printf("错误: 无法创建测试TAC数据\n");
        return;
    }
    
    print_tac_instructions(tac_data, "原始TAC");
    
    // 应用公共子表达式消除优化
    Stru_IrOptimizationConfig_t* config = F_create_tac_default_config();
    Stru_IrOptimizationResult_t* result = F_apply_tac_common_subexpression_elimination(tac_data, config);
    
    // 验证结果
    bool success = validate_optimization_result(result, 0.15); // 期望至少15%的改进
    
    if (success && result->optimized_ir)
    {
        Stru_TacData_t* optimized_tac = (Stru_TacData_t*)result->optimized_ir;
        print_tac_instructions(optimized_tac, "公共子表达式消除优化后TAC");
    }
    
    // 清理资源
    if (result) {
        if (result->error_message) cn_free(result->error_message);
        if (result->warning_message) cn_free(result->warning_message);
        if (result->optimized_ir) F_destroy_tac_data((Stru_TacData_t*)result->optimized_ir);
        cn_free(result);
    }
    F_destroy_tac_data(tac_data);
    
    printf("测试%s\n", success ? "通过" : "失败");
}

/**
 * @brief 测试TAC强度削减优化
 */
static void test_tac_strength_reduction(void)
{
    printf("\n=== 测试TAC强度削减优化 ===\n");
    
    // 创建测试TAC数据（包含乘法指令）
    Stru_TacData_t* tac_data = F_create_tac_data();
    if (!tac_data)
    {
        printf("错误: 无法创建测试TAC数据\n");
        return;
    }
    
    // 添加一些包含乘法的指令
    Stru_TacInstruction_t* instr1 = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN,
        "x",
        "5",
        NULL,
        NULL,
        1,
        1
    );
    
    Stru_TacInstruction_t* instr2 = F_create_tac_instruction(
        Eum_TAC_OP_MUL,
        "y",
        "x",
        "8", // 8是2的3次方
        NULL,
        2,
        1
    );
    
    if (instr1) F_tac_data_add_instruction(tac_data, instr1);
    if (instr2) F_tac_data_add_instruction(tac_data, instr2);
    
    print_tac_instructions(tac_data, "原始TAC");
    
    // 应用强度削减优化
    Stru_IrOptimizationConfig_t* config = F_create_tac_default_config();
    Stru_IrOptimizationResult_t* result = F_apply_tac_strength_reduction(tac_data, config);
    
    // 验证结果
    bool success = validate_optimization_result(result, 0.08); // 期望至少8%的改进
    
    if (success && result->optimized_ir)
    {
        Stru_TacData_t* optimized_tac = (Stru_TacData_t*)result->optimized_ir;
        print_tac_instructions(optimized_tac, "强度削减优化后TAC");
    }
    
    // 清理资源
    if (result) {
        if (result->error_message) cn_free(result->error_message);
        if (result->warning_message) cn_free(result->warning_message);
        if (result->optimized_ir) F_destroy_tac_data((Stru_TacData_t*)result->optimized_ir);
        cn_free(result);
    }
    F_destroy_tac_data(tac_data);
    
    printf("测试%s\n", success ? "通过" : "失败");
}

/**
 * @brief 测试TAC窥孔优化
 */
static void test_tac_peephole_optimization(void)
{
    printf("\n=== 测试TAC窥孔优化 ===\n");
    
    // 创建测试TAC数据（包含可窥孔优化的模式）
    Stru_TacData_t* tac_data = F_create_tac_data();
    if (!tac_data)
    {
        printf("错误: 无法创建测试TAC数据\n");
        return;
    }
    
    // 添加一些可窥孔优化的指令
    Stru_TacInstruction_t* instr1 = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN,
        "t1",
        "x",
        NULL,
        NULL,
        1,
        1
    );
    
    Stru_TacInstruction_t* instr2 = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN,
        "y",
        "t1",
        NULL,
        NULL,
        2,
        1
    );
    
    if (instr1) F_tac_data_add_instruction(tac_data, instr1);
    if (instr2) F_tac_data_add_instruction(tac_data, instr2);
    
    print_tac_instructions(tac_data, "原始TAC");
    
    // 应用窥孔优化
    Stru_IrOptimizationConfig_t* config = F_create_tac_default_config();
    Stru_IrOptimizationResult_t* result = F_apply_tac_peephole_optimization(tac_data, config);
    
    // 验证结果
    bool success = validate_optimization_result(result, 0.03); // 期望至少3%的改进
    
    if (success && result->optimized_ir)
    {
        Stru_TacData_t* optimized_tac = (Stru_TacData_t*)result->optimized_ir;
        print_tac_instructions(optimized_tac, "窥孔优化后TAC");
    }
    
    // 清理资源
    if (result) {
        if (result->error_message) cn_free(result->error_message);
        if (result->warning_message) cn_free(result->warning_message);
        if (result->optimized_ir) F_destroy_tac_data((Stru_TacData_t*)result->optimized_ir);
        cn_free(result);
    }
    if (config) {
        // 注意：这里应该使用优化器接口的destroy_config函数
        // 但由于我们没有优化器实例，我们直接释放内存
        cn_free(config);
    }
    F_destroy_tac_data(tac_data);
    
    printf("测试%s\n", success ? "通过" : "失败");
}

/**
 * @brief 测试优化效果评估器
 */
static void test_optimization_evaluator(void)
{
    printf("\n=== 测试优化效果评估器 ===\n");
    
    // 创建测试TAC数据
    Stru_TacData_t* original_tac = create_test_tac_data();
    if (!original_tac)
    {
        printf("错误: 无法创建原始TAC数据\n");
        return;
    }
    
    // 创建优化后的TAC数据（模拟优化）
    Stru_TacData_t* optimized_tac = F_create_tac_data();
    if (!optimized_tac)
    {
        printf("错误: 无法创建优化后TAC数据\n");
        F_destroy_tac_data(original_tac);
        return;
    }
    
    // 复制原始指令，但移除一些指令（模拟优化）
    size_t original_count = F_tac_data_get_instruction_count(original_tac);
    for (size_t i = 0; i < original_count - 2; i++) // 移除最后2条指令
    {
        Stru_TacInstruction_t* instr = F_tac_data_get_instruction(original_tac, i);
        if (instr)
        {
            F_tac_data_add_instruction(optimized_tac, F_copy_tac_instruction(instr));
        }
    }
    
    // 创建优化效果评估器
    Stru_OptimizationEvaluatorInterface_t* evaluator = F_create_optimization_evaluator_interface();
    if (!evaluator)
    {
        printf("错误: 无法创建优化效果评估器\n");
        F_destroy_tac_data(original_tac);
        F_destroy_tac_data(optimized_tac);
        return;
    }
    
    // 评估优化效果
    Stru_OptimizationEvaluationResult_t* evaluation_result = 
        evaluator->evaluate_optimization(original_tac, optimized_tac, Eum_IR_OPT_CONSTANT_FOLDING);
    
    if (!evaluation_result)
    {
        printf("错误: 无法评估优化效果\n");
        evaluator->destroy();
        F_destroy_tac_data(original_tac);
        F_destroy_tac_data(optimized_tac);
        return;
    }
    
    // 打印评估结果
    printf("优化效果评估结果:\n");
    printf("  评估成功: %s\n", evaluation_result->evaluation_successful ? "是" : "否");
    printf("  评估时间: %.6f秒\n", evaluation_result->evaluation_time);
    
    if (evaluation_result->evaluation_successful)
    {
        printf("  优化指标:\n");
        printf("    指令减少比例: %.2f%%\n", evaluation_result->metrics.instruction_reduction_ratio * 100);
        printf("    代码大小减少比例: %.2f%%\n", evaluation_result->metrics.code_size_reduction_ratio * 100);
        printf("    估计性能改进: %.2f%%\n", evaluation_result->metrics.estimated_performance_improvement * 100);
        printf("    优化质量评分: %.1f/100\n", evaluation_result->metrics.optimization_quality_score);
        
        // 打印评估报告
        if (evaluation_result->evaluation_report)
        {
            printf("\n评估报告:\n%s\n", evaluation_result->evaluation_report);
        }
        
        // 打印优化建议
        if (evaluation_result->recommendations)
        {
            printf("\n优化建议:\n%s\n", evaluation_result->recommendations);
        }
    }
    else
    {
        printf("  评估失败原因: %s\n", 
               evaluation_result->evaluation_report ? evaluation_result->evaluation_report : "未知原因");
    }
    
    // 清理资源
    evaluator->destroy_evaluation_result(evaluation_result);
    // 使用正确的销毁函数
    F_destroy_optimization_evaluator_interface(evaluator);
    F_destroy_tac_data(original_tac);
    F_destroy_tac_data(optimized_tac);
    
    printf("优化效果评估器测试完成\n");
}

/**
 * @brief 测试优化集成流程
 */
static void test_optimization_integration_flow(void)
{
    printf("\n=== 测试优化集成流程 ===\n");
    
    // 创建测试TAC数据
    Stru_TacData_t* tac_data = create_test_tac_data();
    if (!tac_data)
    {
        printf("错误: 无法创建测试TAC数据\n");
        return;
    }
    
    print_tac_instructions(tac_data, "原始TAC");
    
    // 创建TAC优化器接口
    Stru_IrOptimizerInterface_t* optimizer = F_create_tac_optimizer_interface();
    if (!optimizer)
    {
        printf("错误: 无法创建TAC优化器接口\n");
        F_destroy_tac_data(tac_data);
        return;
    }
    
    // 应用所有优化（最高优化级别）
    Stru_IrOptimizationResult_t* result = optimizer->apply_all_optimizations(tac_data);
    
    // 验证结果
    bool success = validate_optimization_result(result, 0.20); // 期望至少20%的改进
    
    if (success && result->optimized_ir)
    {
        Stru_TacData_t* optimized_tac = (Stru_TacData_t*)result->optimized_ir;
        print_tac_instructions(optimized_tac, "集成优化后TAC");
        
        // 使用优化效果评估器评估优化效果
        Stru_OptimizationEvaluatorInterface_t* evaluator = F_create_optimization_evaluator_interface();
        if (evaluator)
        {
            Stru_OptimizationEvaluationResult_t* evaluation_result = 
                evaluator->evaluate_optimization(tac_data, optimized_tac, Eum_IR_OPT_CONSTANT_FOLDING);
            
            if (evaluation_result && evaluation_result->evaluation_successful)
            {
                printf("\n集成优化效果评估:\n");
                printf("  优化质量评分: %.1f/100\n", evaluation_result->metrics.optimization_quality_score);
                printf("  指令减少比例: %.2f%%\n", evaluation_result->metrics.instruction_reduction_ratio * 100);
            }
            
            if (evaluation_result)
            {
                evaluator->destroy_evaluation_result(evaluation_result);
            }
            F_destroy_optimization_evaluator_interface(evaluator);
        }
    }
    
    // 清理资源
    if (result) {
        if (result->error_message) cn_free(result->error_message);
        if (result->warning_message) cn_free(result->warning_message);
        if (result->optimized_ir) F_destroy_tac_data((Stru_TacData_t*)result->optimized_ir);
        cn_free(result);
    }
    F_destroy_tac_optimizer_interface(optimizer);
    F_destroy_tac_data(tac_data);
    
    printf("优化集成流程测试%s\n", success ? "通过" : "失败");
}

/**
 * @brief 测试优化级别
 */
static void test_optimization_levels(void)
{
    printf("\n=== 测试优化级别 ===\n");
    
    // 创建测试TAC数据
    Stru_TacData_t* tac_data = create_test_tac_data();
    if (!tac_data)
    {
        printf("错误: 无法创建测试TAC数据\n");
        return;
    }
    
    // 创建TAC优化器接口
    Stru_IrOptimizerInterface_t* optimizer = F_create_tac_optimizer_interface();
    if (!optimizer)
    {
        printf("错误: 无法创建TAC优化器接口\n");
        F_destroy_tac_data(tac_data);
        return;
    }
    
    // 测试不同优化级别
    Eum_IrOptimizationLevel levels[] = {
        Eum_IR_OPT_LEVEL_NONE,
        Eum_IR_OPT_LEVEL_O1,
        Eum_IR_OPT_LEVEL_O2,
        Eum_IR_OPT_LEVEL_O3,
        Eum_IR_OPT_LEVEL_Os,
        Eum_IR_OPT_LEVEL_Oz
    };
    
    const char* level_names[] = {
        "NONE (无优化)",
        "O1 (基本优化)",
        "O2 (标准优化)",
        "O3 (激进优化)",
        "Os (优化代码大小)",
        "Oz (最大化代码大小优化)"
    };
    
    size_t level_count = sizeof(levels) / sizeof(levels[0]);
    
    for (size_t i = 0; i < level_count; i++)
    {
        printf("\n--- 测试优化级别: %s ---\n", level_names[i]);
        
        // 应用优化级别
        Stru_IrOptimizationResult_t* result = optimizer->apply_optimization_level(tac_data, levels[i]);
        
        if (result && result->success)
        {
            printf("优化成功:\n");
            printf("  原始指令数: %zu\n", result->original_instruction_count);
            printf("  优化后指令数: %zu\n", result->optimized_instruction_count);
            printf("  改进比例: %.2f%%\n", result->improvement_ratio * 100);
            printf("  优化时间: %.6f秒\n", result->optimization_time);
        }
        else
        {
            printf("优化失败\n");
            if (result && result->error_message)
            {
                printf("错误信息: %s\n", result->error_message);
            }
        }
        
        if (result) {
            if (result->error_message) cn_free(result->error_message);
            if (result->warning_message) cn_free(result->warning_message);
            if (result->optimized_ir) F_destroy_tac_data((Stru_TacData_t*)result->optimized_ir);
            cn_free(result);
        }
    }
    
    // 清理资源
    F_destroy_tac_optimizer_interface(optimizer);
    F_destroy_tac_data(tac_data);
    
    printf("\n优化级别测试完成\n");
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有优化集成测试
 */
void run_optimization_integration_tests(void)
{
    printf("开始运行优化集成测试...\n");
    
    // 记录开始时间
    clock_t start_time = clock();
    
    // 运行各个测试用例
    test_tac_constant_folding();
    test_tac_dead_code_elimination();
    test_tac_common_subexpression_elimination();
    test_tac_strength_reduction();
    test_tac_peephole_optimization();
    test_optimization_evaluator();
    test_optimization_integration_flow();
    test_optimization_levels();
    
    // 计算总测试时间
    clock_t end_time = clock();
    double total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("\n=== 优化集成测试完成 ===\n");
    printf("总测试时间: %.3f秒\n", total_time);
    printf("所有测试用例执行完毕\n");
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    printf("CN_Language 优化集成端到端测试\n");
    printf("===============================\n");
    
    // 初始化内存系统（如果需要）
    // F_initialize_memory_system();
    
    // 运行测试
    run_optimization_integration_tests();
    
    // 清理内存系统（如果需要）
    // F_cleanup_memory_system();
    
    printf("\n测试程序结束\n");
    return 0;
}
#endif /* STANDALONE_TEST */
