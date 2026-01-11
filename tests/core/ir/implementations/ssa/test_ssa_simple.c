/******************************************************************************
 * 文件名: test_ssa_simple.c
 * 功能: CN_Language 静态单赋值形式（SSA）简单测试
 * 
 * 测试SSA模块的基本功能，包括：
 * 1. SSA数据结构创建和销毁
 * 2. SSA变量、指令、基本块创建
 * 3. 基本功能验证
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* 包含被测试的头文件 */
#include "../../../src/core/ir/implementations/ssa/CN_ssa_interface.h"

/**
 * @brief 测试SSA变量创建和销毁
 * 
 * 测试SSA变量的创建和销毁功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_ssa_variable_creation(void)
{
    printf("测试: SSA变量创建和销毁\n");
    
    // 创建SSA变量
    Stru_SsaVariable_t* var1 = F_create_ssa_variable("x", 0, false);
    if (!var1)
    {
        printf("  失败: 无法创建变量 x\n");
        return false;
    }
    
    Stru_SsaVariable_t* var2 = F_create_ssa_variable("y", 1, true);
    if (!var2)
    {
        printf("  失败: 无法创建变量 y\n");
        F_destroy_ssa_variable(var1);
        return false;
    }
    
    // 检查变量属性
    if (strcmp(var1->name, "x") != 0 || var1->version != 0 || var1->is_temporary != false)
    {
        printf("  失败: 变量 x 属性不正确\n");
        F_destroy_ssa_variable(var1);
        F_destroy_ssa_variable(var2);
        return false;
    }
    
    if (strcmp(var2->name, "y") != 0 || var2->version != 1 || var2->is_temporary != true)
    {
        printf("  失败: 变量 y 属性不正确\n");
        F_destroy_ssa_variable(var1);
        F_destroy_ssa_variable(var2);
        return false;
    }
    
    // 销毁变量
    F_destroy_ssa_variable(var1);
    F_destroy_ssa_variable(var2);
    
    printf("  通过: SSA变量创建和销毁测试\n");
    return true;
}

/**
 * @brief 测试SSA指令创建和销毁
 * 
 * 测试SSA指令的创建和销毁功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_ssa_instruction_creation(void)
{
    printf("测试: SSA指令创建和销毁\n");
    
    // 创建变量用于指令
    Stru_SsaVariable_t* result_var = F_create_ssa_variable("result", 0, true);
    if (!result_var)
    {
        printf("  失败: 无法创建结果变量\n");
        return false;
    }
    
    // 创建SSA指令
    Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
        Eum_SSA_OP_ADD, result_var, 10, 5);
    
    if (!instr)
    {
        printf("  失败: 无法创建SSA指令\n");
        F_destroy_ssa_variable(result_var);
        return false;
    }
    
    // 检查指令属性
    if (instr->opcode != Eum_SSA_OP_ADD || instr->result != result_var || 
        instr->line != 10 || instr->column != 5)
    {
        printf("  失败: 指令属性不正确\n");
        F_destroy_ssa_instruction(instr);
        F_destroy_ssa_variable(result_var);
        return false;
    }
    
    // 添加操作数
    Stru_SsaVariable_t* op1 = F_create_ssa_variable("a", 0, false);
    Stru_SsaVariable_t* op2 = F_create_ssa_variable("b", 0, false);
    
    if (!op1 || !op2)
    {
        printf("  失败: 无法创建操作数变量\n");
        if (op1) F_destroy_ssa_variable(op1);
        if (op2) F_destroy_ssa_variable(op2);
        F_destroy_ssa_instruction(instr);
        F_destroy_ssa_variable(result_var);
        return false;
    }
    
    if (!F_ssa_instruction_add_operand(instr, op1) || 
        !F_ssa_instruction_add_operand(instr, op2))
    {
        printf("  失败: 无法添加操作数到指令\n");
        F_destroy_ssa_variable(op1);
        F_destroy_ssa_variable(op2);
        F_destroy_ssa_instruction(instr);
        F_destroy_ssa_variable(result_var);
        return false;
    }
    
    // 检查操作数数量
    if (instr->operand_count != 2)
    {
        printf("  失败: 操作数数量不正确\n");
        F_destroy_ssa_variable(op1);
        F_destroy_ssa_variable(op2);
        F_destroy_ssa_instruction(instr);
        F_destroy_ssa_variable(result_var);
        return false;
    }
    
    // 清理
    F_destroy_ssa_variable(op1);
    F_destroy_ssa_variable(op2);
    F_destroy_ssa_instruction(instr);
    F_destroy_ssa_variable(result_var);
    
    printf("  通过: SSA指令创建和销毁测试\n");
    return true;
}

/**
 * @brief 测试SSA基本块创建和销毁
 * 
 * 测试SSA基本块的创建和销毁功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_ssa_basic_block_creation(void)
{
    printf("测试: SSA基本块创建和销毁\n");
    
    // 创建SSA基本块
    Stru_SsaBasicBlock_t* block = F_create_ssa_basic_block("entry");
    if (!block)
    {
        printf("  失败: 无法创建基本块\n");
        return false;
    }
    
    // 检查基本块属性
    if (strcmp(block->name, "entry") != 0)
    {
        printf("  失败: 基本块名称不正确\n");
        F_destroy_ssa_basic_block(block);
        return false;
    }
    
    // 创建指令并添加到基本块
    Stru_SsaVariable_t* var = F_create_ssa_variable("temp", 0, true);
    if (!var)
    {
        printf("  失败: 无法创建变量\n");
        F_destroy_ssa_basic_block(block);
        return false;
    }
    
    Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
        Eum_SSA_OP_COPY, var, 1, 1);
    
    if (!instr)
    {
        printf("  失败: 无法创建指令\n");
        F_destroy_ssa_variable(var);
        F_destroy_ssa_basic_block(block);
        return false;
    }
    
    if (!F_ssa_basic_block_add_instruction(block, instr))
    {
        printf("  失败: 无法添加指令到基本块\n");
        F_destroy_ssa_instruction(instr);
        F_destroy_ssa_variable(var);
        F_destroy_ssa_basic_block(block);
        return false;
    }
    
    // 检查指令数量
    if (block->instruction_count != 1)
    {
        printf("  失败: 基本块指令数量不正确\n");
        F_destroy_ssa_instruction(instr);
        F_destroy_ssa_variable(var);
        F_destroy_ssa_basic_block(block);
        return false;
    }
    
    // 清理
    F_destroy_ssa_instruction(instr);
    F_destroy_ssa_variable(var);
    F_destroy_ssa_basic_block(block);
    
    printf("  通过: SSA基本块创建和销毁测试\n");
    return true;
}

/**
 * @brief 测试SSA数据创建和销毁
 * 
 * 测试SSA数据的创建和销毁功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_ssa_data_creation(void)
{
    printf("测试: SSA数据创建和销毁\n");
    
    // 创建SSA数据
    Stru_SsaData_t* ssa_data = F_create_ssa_data();
    if (!ssa_data)
    {
        printf("  失败: 无法创建SSA数据\n");
        return false;
    }
    
    // 创建函数并添加到SSA数据
    Stru_SsaFunction_t* func = F_create_ssa_function("main");
    if (!func)
    {
        printf("  失败: 无法创建SSA函数\n");
        F_destroy_ssa_data(ssa_data);
        return false;
    }
    
    if (!F_ssa_data_add_function(ssa_data, func))
    {
        printf("  失败: 无法添加函数到SSA数据\n");
        F_destroy_ssa_function(func);
        F_destroy_ssa_data(ssa_data);
        return false;
    }
    
    // 检查函数数量
    size_t func_count = F_ssa_data_get_function_count(ssa_data);
    if (func_count != 1)
    {
        printf("  失败: SSA数据函数数量不正确\n");
        F_destroy_ssa_data(ssa_data);
        return false;
    }
    
    // 清理
    F_destroy_ssa_data(ssa_data);
    
    printf("  通过: SSA数据创建和销毁测试\n");
    return true;
}

/**
 * @brief 运行所有SSA简单测试
 * 
 * 运行所有SSA简单测试并打印结果摘要。
 * 
 * @return int 测试结果（0表示成功，非0表示失败）
 */
int main(void)
{
    printf("===============================================================\n");
    printf("CN_Language SSA简单功能测试\n");
    printf("===============================================================\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行所有测试
    if (test_ssa_variable_creation()) passed++;
    total++;
    
    if (test_ssa_instruction_creation()) passed++;
    total++;
    
    if (test_ssa_basic_block_creation()) passed++;
    total++;
    
    if (test_ssa_data_creation()) passed++;
    total++;
    
    printf("===============================================================\n");
    printf("测试结果: %d/%d 通过\n", passed, total);
    printf("===============================================================\n");
    
    return (passed == total) ? 0 : 1;
}
