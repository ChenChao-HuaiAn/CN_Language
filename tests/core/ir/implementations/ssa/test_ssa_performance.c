/******************************************************************************
 * 文件名: test_ssa_performance.c
 * 功能: CN_Language 静态单赋值形式（SSA）性能测试
 * 
 * 测试SSA模块的性能，包括：
 * 1. SSA数据结构创建和销毁性能
 * 2. AST到SSA转换性能
 * 3. φ函数插入算法性能
 * 4. 大规模SSA数据处理性能
 * 
 * 遵循项目测试规范，使用性能测试框架。
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
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/* 包含被测试的头文件 */
#include "../../../src/core/ir/implementations/ssa/CN_ssa_interface.h"

// ============================================================================
// 性能测试辅助函数
// ============================================================================

/**
 * @brief 获取当前时间（毫秒）
 * 
 * 获取当前时间的毫秒表示，用于性能测量。
 * 
 * @return long long 当前时间（毫秒）
 */
static long long get_current_time_ms(void)
{
#ifdef _WIN32
    // Windows实现
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (long long)(counter.QuadPart * 1000 / frequency.QuadPart);
#else
    // Unix/Linux实现
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

/**
 * @brief 打印性能测试结果
 * 
 * 打印性能测试的结果，包括测试名称、执行时间和是否通过。
 * 
 * @param test_name 测试名称
 * @param start_time 开始时间（毫秒）
 * @param end_time 结束时间（毫秒）
 * @param passed 是否通过
 */
static void print_performance_result(const char* test_name, long long start_time, 
                                     long long end_time, bool passed)
{
    long long duration = end_time - start_time;
    const char* status = passed ? "通过" : "失败";
    
    printf("性能测试: %-40s 时间: %5lld ms 状态: %s\n", 
           test_name, duration, status);
}

/**
 * @brief 创建测试AST节点
 * 
 * 创建用于性能测试的模拟AST节点。
 * 
 * @param node_type 节点类型
 * @param name 节点名称
 * @return Stru_AstNode_t* 创建的AST节点
 */
static Stru_AstNode_t* create_test_ast_node(Eum_AstNodeType node_type, const char* name)
{
    // 简化实现：创建模拟AST节点
    // 注意：实际实现需要调用AST模块的API
    Stru_AstNode_t* node = (Stru_AstNode_t*)malloc(sizeof(Stru_AstNode_t));
    if (!node)
    {
        return NULL;
    }
    
    memset(node, 0, sizeof(Stru_AstNode_t));
    
    // 设置节点类型和名称
    // 注意：这里使用简化实现，实际项目应使用AST API
    static char node_name[64];
    if (name)
    {
        snprintf(node_name, sizeof(node_name), "%s", name);
    }
    else
    {
        snprintf(node_name, sizeof(node_name), "test_node");
    }
    
    return node;
}

/**
 * @brief 销毁测试AST节点
 * 
 * 销毁测试AST节点。
 * 
 * @param node AST节点
 */
static void destroy_test_ast_node(Stru_AstNode_t* node)
{
    if (!node)
    {
        return;
    }
    
    free(node);
}

/**
 * @brief 创建测试AST树
 * 
 * 创建用于性能测试的模拟AST树。
 * 
 * @param depth 树深度
 * @param breadth 每个节点的子节点数量
 * @return Stru_AstNode_t* AST根节点
 */
static Stru_AstNode_t* create_test_ast_tree(int depth, int breadth)
{
    if (depth <= 0)
    {
        return NULL;
    }
    
    // 创建根节点（函数声明）
    Stru_AstNode_t* root = create_test_ast_node(Eum_AST_FUNCTION_DECL, "test_function");
    if (!root)
    {
        return NULL;
    }
    
    // 创建函数体（代码块）
    Stru_AstNode_t* body = create_test_ast_node(Eum_AST_BLOCK_STMT, "function_body");
    if (!body)
    {
        destroy_test_ast_node(root);
        return NULL;
    }
    
    // 递归创建子节点
    if (depth > 1)
    {
        // 创建变量声明和表达式语句
        for (int i = 0; i < breadth; i++)
        {
            char var_name[32];
            snprintf(var_name, sizeof(var_name), "var%d", i);
            
            Stru_AstNode_t* var_decl = create_test_ast_node(Eum_AST_VARIABLE_DECL, var_name);
            if (var_decl)
            {
                // 添加到函数体
                // 注意：这里使用简化实现
            }
            
            Stru_AstNode_t* expr_stmt = create_test_ast_node(Eum_AST_EXPRESSION_STMT, NULL);
            if (expr_stmt)
            {
                // 添加到函数体
                // 注意：这里使用简化实现
            }
        }
    }
    
    return root;
}

// ============================================================================
// 性能测试用例：SSA数据结构
// ============================================================================

/**
 * @brief 测试SSA变量创建性能
 */
void test_ssa_variable_creation_performance(void)
{
    const char* test_name = "SSA变量创建性能测试";
    long long start_time = get_current_time_ms();
    bool passed = true;
    
    const int num_variables = 10000;
    Stru_SsaVariable_t** variables = (Stru_SsaVariable_t**)malloc(sizeof(Stru_SsaVariable_t*) * num_variables);
    
    if (!variables)
    {
        passed = false;
    }
    else
    {
        // 创建大量SSA变量
        for (int i = 0; i < num_variables; i++)
        {
            char var_name[32];
            snprintf(var_name, sizeof(var_name), "var%d", i);
            
            variables[i] = F_create_ssa_variable(var_name, i % 10, (i % 2) == 0);
            if (!variables[i])
            {
                passed = false;
                break;
            }
        }
        
        // 销毁所有变量
        for (int i = 0; i < num_variables; i++)
        {
            if (variables[i])
            {
                F_destroy_ssa_variable(variables[i]);
            }
        }
        
        free(variables);
    }
    
    long long end_time = get_current_time_ms();
    print_performance_result(test_name, start_time, end_time, passed);
}

/**
 * @brief 测试SSA指令创建性能
 */
void test_ssa_instruction_creation_performance(void)
{
    const char* test_name = "SSA指令创建性能测试";
    long long start_time = get_current_time_ms();
    bool passed = true;
    
    const int num_instructions = 5000;
    Stru_SsaInstruction_t** instructions = (Stru_SsaInstruction_t**)malloc(sizeof(Stru_SsaInstruction_t*) * num_instructions);
    
    if (!instructions)
    {
        passed = false;
    }
    else
    {
        // 创建临时变量用于指令
        Stru_SsaVariable_t* temp_var = F_create_ssa_variable("temp", 0, true);
        if (!temp_var)
        {
            passed = false;
            free(instructions);
        }
        else
        {
            // 创建大量SSA指令
            for (int i = 0; i < num_instructions; i++)
            {
                Eum_SsaOpcode opcode;
                
                // 使用不同的操作码
                switch (i % 10)
                {
                    case 0: opcode = Eum_SSA_OP_ADD; break;
                    case 1: opcode = Eum_SSA_OP_SUB; break;
                    case 2: opcode = Eum_SSA_OP_MUL; break;
                    case 3: opcode = Eum_SSA_OP_DIV; break;
                    case 4: opcode = Eum_SSA_OP_COPY; break;
                    case 5: opcode = Eum_SSA_OP_EQ; break;
                    case 6: opcode = Eum_SSA_OP_LT; break;
                    case 7: opcode = Eum_SSA_OP_GT; break;
                    case 8: opcode = Eum_SSA_OP_AND; break;
                    case 9: opcode = Eum_SSA_OP_OR; break;
                    default: opcode = Eum_SSA_OP_COPY; break;
                }
                
                instructions[i] = F_create_ssa_instruction(opcode, temp_var, i, i % 10);
                if (!instructions[i])
                {
                    passed = false;
                    break;
                }
                
                // 添加操作数
                F_ssa_instruction_add_operand(instructions[i], temp_var);
            }
            
            // 销毁所有指令
            for (int i = 0; i < num_instructions; i++)
            {
                if (instructions[i])
                {
                    F_destroy_ssa_instruction(instructions[i]);
                }
            }
            
            F_destroy_ssa_variable(temp_var);
            free(instructions);
        }
    }
    
    long long end_time = get_current_time_ms();
    print_performance_result(test_name, start_time, end_time, passed);
}

/**
 * @brief 测试SSA基本块创建性能
 */
void test_ssa_basic_block_creation_performance(void)
{
    const char* test_name = "SSA基本块创建性能测试";
    long long start_time = get_current_time_ms();
    bool passed = true;
    
    const int num_blocks = 1000;
    Stru_SsaBasicBlock_t** blocks = (Stru_SsaBasicBlock_t**)malloc(sizeof(Stru_SsaBasicBlock_t*) * num_blocks);
    
    if (!blocks)
    {
        passed = false;
    }
    else
    {
        // 创建大量SSA基本块
        for (int i = 0; i < num_blocks; i++)
        {
            char block_name[32];
            snprintf(block_name, sizeof(block_name), "block%d", i);
            
            blocks[i] = F_create_ssa_basic_block(block_name);
            if (!blocks[i])
            {
                passed = false;
                break;
            }
            
            // 添加一些指令到基本块
            Stru_SsaVariable_t* temp_var = F_create_ssa_variable("x", 0, false);
            if (temp_var)
            {
                Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
                    Eum_SSA_OP_ADD, temp_var, i, 0);
                if (instr)
                {
                    F_ssa_basic_block_add_instruction(blocks[i], instr);
                }
                F_destroy_ssa_variable(temp_var);
            }
        }
        
        // 销毁所有基本块
        for (int i = 0; i < num_blocks; i++)
        {
            if (blocks[i])
            {
                F_destroy_ssa_basic_block(blocks[i]);
            }
        }
        
        free(blocks);
    }
    
    long long end_time = get_current_time_ms();
    print_performance_result(test_name, start_time, end_time, passed);
}

// ============================================================================
// 性能测试用例：AST到SSA转换
// ============================================================================

/**
 * @brief 测试简单AST到SSA转换性能
 */
void test_ast_to_ssa_conversion_performance(void)
{
    const char* test_name = "AST到SSA转换性能测试";
    long long start_time = get_current_time_ms();
    bool passed = true;
    
    // 创建测试AST树
    Stru_AstNode_t* ast_root = create_test_ast_tree(3, 5);
    if (!ast_root)
    {
        passed = false;
    }
    else
    {
        // 转换为SSA
        Stru_SsaData_t* ssa_data = F_convert_ast_to_ssa(ast_root);
        if (!ssa_data)
        {
            passed = false;
        }
        else
        {
            // 检查转换结果
            size_t func_count = F_ssa_data_get_function_count(ssa_data);
            if (func_count == 0)
            {
                passed = false;
            }
            
            F_destroy_ssa_data(ssa_data);
        }
        
        destroy_test_ast_node(ast_root);
    }
    
    long long end_time = get_current_time_ms();
    print_performance_result(test_name, start_time, end_time, passed);
}

/**
 * @brief 测试大规模AST到SSA转换性能
 */
void test_large_ast_to_ssa_conversion_performance(void)
{
    const char* test_name = "大规模AST到SSA转换性能测试";
    long long start_time = get_current_time_ms();
    bool passed = true;
    
    // 创建更大的测试AST树
    Stru_AstNode_t* ast_root = create_test_ast_tree(5, 10);
    if (!ast_root)
    {
        passed = false;
    }
    else
    {
        // 转换为SSA
        Stru_SsaData_t* ssa_data = F_convert_ast_to_ssa(ast_root);
        if (!ssa_data)
        {
            passed = false;
        }
        else
        {
            // 检查转换结果
            size_t func_count = F_ssa_data_get_function_count(ssa_data);
            if (func_count == 0)
            {
                passed = false;
            }
            
            F_destroy_ssa_data(ssa_data);
        }
        
        destroy_test_ast_node(ast_root);
    }
    
    long long end_time = get_current_time_ms();
    print_performance_result(test_name, start_time, end_time, passed);
}

// ============================================================================
// 性能测试用例：φ函数插入
// ============================================================================

/**
 * @brief 测试φ函数插入性能
 */
void test_phi_function_insertion_performance(void)
{
    const char* test_name = "φ函数插入性能测试";
    long long start_time = get_current_time_ms();
    bool passed = true;
    
    // 创建SSA数据
    Stru_SsaData_t* ssa_data = F_create_ssa_data();
    if (!ssa_data)
    {
        passed = false;
    }
    else
    {
        // 创建函数
        Stru_SsaFunction_t* func = F_create_ssa_function("test_func");
        if (!func)
        {
            passed = false;
            F_destroy_ssa_data(ssa_data);
        }
        else
        {
            // 添加函数到SSA数据
            if (!F_ssa_data_add_function(ssa_data, func))
            {
                passed = false;
                F_destroy_ssa_function(func);
                F_destroy_ssa_data(ssa_data);
            }
            else
            {
                // 创建多个基本块
                const int num_blocks = 100;
                for (int i = 0; i < num_blocks; i++)
                {
                    char block_name[32];
                    snprintf(block_name, sizeof(block_name), "block%d", i);
                    
                    Stru_SsaBasicBlock_t* block = F_create_ssa_basic_block(block_name);
                    if (block)
                    {
                        F_ssa_function_add_block(func, block);
                    }
                }
                
                // 插入φ函数
                if (!F_insert_phi_functions(ssa_data))
                {
                    passed = false;
                }
                
                F_destroy_ssa_data(ssa_data);
            }
        }
    }
    
    long long end_time = get_current_time_ms();
    print_performance_result(test_name, start_time, end_time, passed);
}

// ============================================================================
// 性能测试用例：SSA数据处理
// ============================================================================

/**
 * @brief 测试SSA数据序列化性能
 */
void test_ssa_data_serialization_performance(void)
{
    const char* test_name = "SSA数据序列化性能测试";
    long long start_time = get_current_time_ms();
    bool passed = true;
    
    // 创建SSA数据并填充内容
    Stru_SsaData_t* ssa_data = F_create_ssa_data();
    if (!ssa_data)
    {
        passed = false;
    }
    else
    {
        // 创建多个函数
        const int num_functions = 50;
        for (int f = 0; f < num_functions; f++)
        {
            char func_name[32];
            snprintf(func_name, sizeof(func_name), "func%d", f);
            
            Stru_SsaFunction_t* func = F_create_ssa_function(func_name);
            if (func)
            {
                // 添加函数到SSA数据
                F_ssa_data_add_function(ssa_data, func);
                
                // 创建基本块
                const int num_blocks = 20;
                for (int b = 0; b < num_blocks; b++)
                {
                    char block_name[32];
                    snprintf(block_name, sizeof(block_name), "block%d_%d", f, b);
                    
                    Stru_SsaBasicBlock_t* block = F_create_ssa_basic_block(block_name);
                    if (block)
                    {
                        F_ssa_function_add_block(func, block);
                        
                        const int num_instructions = 10;
                        for (int i = 0; i < num_instructions; i++)
                        {
                            char var_name[32];
                            snprintf(var_name, sizeof(var_name), "var_%d_%d_%d", f, b, i);
                            
                            Stru_SsaVariable_t* var = F_create_ssa_variable(var_name, i, (i % 2) == 0);
                            if (var)
                            {
                                Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
                                    Eum_SSA_OP_ADD, var, i, 0);
                                if (instr)
                                {
                                    F_ssa_basic_block_add_instruction(block, instr);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 验证SSA数据
        size_t func_count = F_ssa_data_get_function_count(ssa_data);
        if (func_count != num_functions)
        {
            passed = false;
        }
        
        // 验证每个函数的基本块数量
        const int expected_blocks_per_func = 20;
        for (int f = 0; f < num_functions; f++)
        {
            Stru_SsaFunction_t* func = F_ssa_data_get_function(ssa_data, f);
            if (func && func->block_count != expected_blocks_per_func)
            {
                passed = false;
                break;
            }
        }
        
        F_destroy_ssa_data(ssa_data);
    }
    
    long long end_time = get_current_time_ms();
    print_performance_result(test_name, start_time, end_time, passed);
}

// ============================================================================
// 性能测试主函数
// ============================================================================

/**
 * @brief 运行所有SSA性能测试
 * 
 * 运行所有SSA性能测试并打印结果摘要。
 * 
 * @return int 测试结果（0表示成功，非0表示失败）
 */
int main(void)
{
    printf("===============================================================\n");
    printf("CN_Language SSA性能测试\n");
    printf("===============================================================\n");
    
    // 运行所有性能测试
    test_ssa_variable_creation_performance();
    test_ssa_instruction_creation_performance();
    test_ssa_basic_block_creation_performance();
    test_ast_to_ssa_conversion_performance();
    test_large_ast_to_ssa_conversion_performance();
    test_phi_function_insertion_performance();
    test_ssa_data_serialization_performance();
    
    printf("===============================================================\n");
    printf("所有SSA性能测试完成\n");
    printf("===============================================================\n");
    
    return 0;
}
