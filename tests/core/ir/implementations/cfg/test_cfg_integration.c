/******************************************************************************
 * 文件名: test_cfg_integration.c
 * 功能: CN_Language 控制流图集成测试
 * 
 * 测试控制流图模块的集成功能，包括：
 * 1. 基本块数据结构
 * 2. 控制流图构建
 * 3. 基本块划分算法
 * 4. 控制流分析
 * 5. 数据流分析
 * 6. 控制流图优化
 * 
 * 遵循项目测试规范，使用Unity测试框架。
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

/* 测试框架包含 */
#ifdef USE_UNITY
#include "unity.h"
#else
/* 简单测试框架 */
#define TEST_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FALSE(condition) TEST_ASSERT_TRUE(!(condition))
#define TEST_ASSERT_EQUAL(expected, actual) TEST_ASSERT_TRUE((expected) == (actual))
#define TEST_ASSERT_NOT_NULL(ptr) TEST_ASSERT_TRUE((ptr) != NULL)
#define TEST_ASSERT_NULL(ptr) TEST_ASSERT_TRUE((ptr) == NULL)
#define TEST_ASSERT_EQUAL_STRING(expected, actual) TEST_ASSERT_TRUE(strcmp((expected), (actual)) == 0)
#define TEST_ASSERT_GREATER_THAN(threshold, value) TEST_ASSERT_TRUE((value) > (threshold))
#define TEST_ASSERT_EQUAL_UINT(expected, actual) TEST_ASSERT_EQUAL((unsigned int)(expected), (unsigned int)(actual))
#define TEST_MESSAGE(msg) printf("测试: %s\n", msg)
#endif

/* 包含被测试的头文件 */
#include "../../../../src/core/ir/implementations/cfg/CN_cfg_basic_block.h"
#include "../../../../src/core/ir/implementations/cfg/CN_cfg_control_flow_graph.h"
#include "../../../../src/core/ir/implementations/cfg/CN_cfg_builder.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 创建测试IR指令
 * 
 * 创建用于测试的模拟IR指令。
 * 
 * @param type 指令类型
 * @param id 指令ID
 * @return Stru_IrInstruction_t* 创建的指令
 */
static Stru_IrInstruction_t* create_test_ir_instruction(Eum_IrInstructionType type, int id)
{
    Stru_IrInstruction_t* instruction = (Stru_IrInstruction_t*)malloc(sizeof(Stru_IrInstruction_t));
    if (!instruction)
    {
        return NULL;
    }
    
    memset(instruction, 0, sizeof(Stru_IrInstruction_t));
    instruction->type = type;
    instruction->id = id;
    
    // 设置指令名称
    static char name_buffer[64];
    snprintf(name_buffer, sizeof(name_buffer), "test_instruction_%d", id);
    instruction->name = strdup(name_buffer);
    
    return instruction;
}

/**
 * @brief 销毁测试IR指令
 * 
 * 销毁测试IR指令。
 * 
 * @param instruction 指令
 */
static void destroy_test_ir_instruction(Stru_IrInstruction_t* instruction)
{
    if (!instruction)
    {
        return;
    }
    
    if (instruction->name)
    {
        free(instruction->name);
    }
    
    free(instruction);
}

/**
 * @brief 创建测试指令数组
 * 
 * 创建用于测试的指令数组。
 * 
 * @param count 指令数量
 * @return Stru_IrInstruction_t** 指令数组
 */
static Stru_IrInstruction_t** create_test_instructions(size_t count)
{
    if (count == 0)
    {
        return NULL;
    }
    
    Stru_IrInstruction_t** instructions = (Stru_IrInstruction_t**)malloc(sizeof(Stru_IrInstruction_t*) * count);
    if (!instructions)
    {
        return NULL;
    }
    
    // 创建不同类型的指令
    for (size_t i = 0; i < count; i++)
    {
        Eum_IrInstructionType type;
        
        // 创建不同类型的指令以测试基本块划分
        if (i == count - 1)
        {
            // 最后一条指令是返回指令
            type = Eum_IR_INSTR_RETURN;
        }
        else if (i % 5 == 0)
        {
            // 每5条指令有一条分支指令
            type = Eum_IR_INSTR_BRANCH;
        }
        else if (i % 7 == 0)
        {
            // 每7条指令有一条跳转指令
            type = Eum_IR_INSTR_JUMP;
        }
        else
        {
            // 其他指令是普通指令
            type = Eum_IR_INSTR_ASSIGN;
        }
        
        instructions[i] = create_test_ir_instruction(type, (int)i);
        if (!instructions[i])
        {
            // 清理已分配的指令
            for (size_t j = 0; j < i; j++)
            {
                destroy_test_ir_instruction(instructions[j]);
            }
            free(instructions);
            return NULL;
        }
    }
    
    return instructions;
}

/**
 * @brief 销毁测试指令数组
 * 
 * 销毁测试指令数组及其所有指令。
 * 
 * @param instructions 指令数组
 * @param count 指令数量
 */
static void destroy_test_instructions(Stru_IrInstruction_t** instructions, size_t count)
{
    if (!instructions)
    {
        return;
    }
    
    for (size_t i = 0; i < count; i++)
    {
        destroy_test_ir_instruction(instructions[i]);
    }
    
    free(instructions);
}

// ============================================================================
// 测试用例：基本块数据结构
// ============================================================================

/**
 * @brief 测试基本块创建和销毁
 */
void test_basic_block_create_destroy(void)
{
    TEST_MESSAGE("测试基本块创建和销毁");
    
    // 创建基本块
    Stru_BasicBlock_t* block = F_create_basic_block("test_block", Eum_BASIC_BLOCK_NORMAL, 0);
    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_EQUAL_STRING("test_block", block->name);
    TEST_ASSERT_EQUAL(Eum_BASIC_BLOCK_NORMAL, block->type);
    TEST_ASSERT_EQUAL(0, block->id);
    
    // 销毁基本块
    F_destroy_basic_block(block);
}

/**
 * @brief 测试基本块指令管理
 */
void test_basic_block_instruction_management(void)
{
    TEST_MESSAGE("测试基本块指令管理");
    
    // 创建基本块
    Stru_BasicBlock_t* block = F_create_basic_block("test_block", Eum_BASIC_BLOCK_NORMAL, 0);
    TEST_ASSERT_NOT_NULL(block);
    
    // 创建测试指令
    Stru_IrInstruction_t* instruction1 = create_test_ir_instruction(Eum_IR_INSTR_ASSIGN, 1);
    Stru_IrInstruction_t* instruction2 = create_test_ir_instruction(Eum_IR_INSTR_ASSIGN, 2);
    TEST_ASSERT_NOT_NULL(instruction1);
    TEST_ASSERT_NOT_NULL(instruction2);
    
    // 添加指令到基本块
    bool result = F_basic_block_add_instruction(block, instruction1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, block->instruction_count);
    
    result = F_basic_block_add_instruction(block, instruction2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, block->instruction_count);
    
    // 获取指令
    Stru_IrInstruction_t* retrieved = F_basic_block_get_instruction(block, 0);
    TEST_ASSERT_EQUAL(instruction1, retrieved);
    
    retrieved = F_basic_block_get_instruction(block, 1);
    TEST_ASSERT_EQUAL(instruction2, retrieved);
    
    // 测试越界访问
    retrieved = F_basic_block_get_instruction(block, 2);
    TEST_ASSERT_NULL(retrieved);
    
    // 清理
    F_destroy_basic_block(block);
    destroy_test_ir_instruction(instruction1);
    destroy_test_ir_instruction(instruction2);
}

/**
 * @brief 测试基本块控制流关系
 */
void test_basic_block_control_flow_relations(void)
{
    TEST_MESSAGE("测试基本块控制流关系");
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
    Stru_BasicBlock_t* block3 = F_create_basic_block("block3", Eum_BASIC_BLOCK_NORMAL, 3);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    TEST_ASSERT_NOT_NULL(block3);
    
    // 添加后继
    bool result = F_basic_block_add_successor(block1, block2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, block1->successor_count);
    
    result = F_basic_block_add_successor(block1, block3);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, block1->successor_count);
    
    // 添加前驱
    result = F_basic_block_add_predecessor(block2, block1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, block2->predecessor_count);
    
    result = F_basic_block_add_predecessor(block3, block1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, block3->predecessor_count);
    
    // 检查后继
    Stru_BasicBlock_t* successor = F_basic_block_get_successor(block1, 0);
    TEST_ASSERT_EQUAL(block2, successor);
    
    successor = F_basic_block_get_successor(block1, 1);
    TEST_ASSERT_EQUAL(block3, successor);
    
    // 检查前驱
    Stru_BasicBlock_t* predecessor = F_basic_block_get_predecessor(block2, 0);
    TEST_ASSERT_EQUAL(block1, predecessor);
    
    // 清理
    F_destroy_basic_block(block1);
    F_destroy_basic_block(block2);
    F_destroy_basic_block(block3);
}

// ============================================================================
// 测试用例：控制流图数据结构
// ============================================================================

/**
 * @brief 测试控制流图创建和销毁
 */
void test_control_flow_graph_create_destroy(void)
{
    TEST_MESSAGE("测试控制流图创建和销毁");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    TEST_ASSERT_EQUAL_STRING("test_cfg", cfg->name);
    TEST_ASSERT_EQUAL(1, cfg->id);
    TEST_ASSERT_EQUAL(0, cfg->block_count);
    
    // 销毁控制流图
    F_destroy_control_flow_graph(cfg);
}

/**
 * @brief 测试控制流图基本块管理
 */
void test_control_flow_graph_block_management(void)
{
    TEST_MESSAGE("测试控制流图基本块管理");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    
    // 添加基本块到控制流图
    bool result = F_cfg_add_basic_block(cfg, block1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cfg->block_count);
    
    result = F_cfg_add_basic_block(cfg, block2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, cfg->block_count);
    
    // 获取基本块
    Stru_BasicBlock_t* retrieved = F_cfg_get_basic_block(cfg, 0);
    TEST_ASSERT_EQUAL(block1, retrieved);
    
    retrieved = F_cfg_get_basic_block(cfg, 1);
    TEST_ASSERT_EQUAL(block2, retrieved);
    
    // 设置入口和出口块
    result = F_cfg_set_entry_block(cfg, block1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(block1, cfg->entry_block);
    
    result = F_cfg_set_exit_block(cfg, block2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(block2, cfg->exit_block);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

// ============================================================================
// 测试用例：基本块划分算法
// ============================================================================

/**
 * @brief 测试简单基本块划分算法
 */
void test_basic_block_partition_simple(void)
{
    TEST_MESSAGE("测试简单基本块划分算法");
    
    // 创建测试指令数组
    size_t instruction_count = 20;
    Stru_IrInstruction_t** instructions = create_test_instructions(instruction_count);
    TEST_ASSERT_NOT_NULL(instructions);
    
    // 划分基本块
    size_t block_count = 0;
    Stru_BasicBlock_t** blocks = F_partition_basic_blocks_simple(instructions, instruction_count, &block_count);
    
    // 验证结果
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_TRUE(block_count > 0);
    
    // 验证每个基本块
    size_t total_instructions = 0;
    for (size_t i = 0; i < block_count; i++)
    {
        TEST_ASSERT_NOT_NULL(blocks[i]);
        total_instructions += blocks[i]->instruction_count;
    }
    
    // 验证指令总数
    TEST_ASSERT_EQUAL(instruction_count, total_instructions);
    
    // 清理
    for (size_t i = 0; i < block_count; i++)
    {
        F_destroy_basic_block(blocks[i]);
    }
    free(blocks);
    destroy_test_instructions(instructions, instruction_count);
}

/**
 * @brief 测试高级基本块划分算法
 */
void test_basic_block_partition_advanced(void)
{
    TEST_MESSAGE("测试高级基本块划分算法");
    
    // 创建测试指令数组
    size_t instruction_count = 30;
    Stru_IrInstruction_t** instructions = create_test_instructions(instruction_count);
    TEST_ASSERT_NOT_NULL(instructions);
    
    // 划分基本块
    size_t block_count = 0;
    Stru_BasicBlock_t** blocks = F_partition_basic_blocks_advanced(instructions, instruction_count, &block_count);
    
    // 验证结果
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_TRUE(block_count > 0);
    
    // 验证每个基本块
    size_t total_instructions = 0;
    for (size_t i = 0; i < block_count; i++)
    {
        TEST_ASSERT_NOT_NULL(blocks[i]);
        total_instructions += blocks[i]->instruction_count;
    }
    
    // 验证指令总数
    TEST_ASSERT_EQUAL(instruction_count, total_instructions);
    
    // 清理
    for (size_t i = 0; i < block_count; i++)
    {
        F_destroy_basic_block(blocks[i]);
    }
    free(blocks);
    destroy_test_instructions(instructions, instruction_count);
}

/**
 * @brief 测试优化基本块划分算法
 */
void test_basic_block_partition_optimized(void)
{
    TEST_MESSAGE("测试优化基本块划分算法");
    
    // 创建测试指令数组
    size_t instruction_count = 40;
    Stru_IrInstruction_t** instructions = create_test_instructions(instruction_count);
    TEST_ASSERT_NOT_NULL(instructions);
    
    // 划分基本块
    size_t block_count = 0;
    Stru_BasicBlock_t** blocks = F_partition_basic_blocks_optimized(instructions, instruction_count, &block_count);
    
    // 验证结果
    TEST_ASSERT_NOT_NULL(blocks);
    TEST_ASSERT_TRUE(block_count > 0);
    
    // 验证每个基本块
    size_t total_instructions = 0;
    for (size_t i = 0; i < block_count; i++)
    {
        TEST_ASSERT_NOT_NULL(blocks[i]);
        total_instructions += blocks[i]->instruction_count;
    }
    
    // 验证指令总数
    TEST_ASSERT_EQUAL(instruction_count, total_instructions);
    
    // 清理
    for (size_t i = 0; i < block_count; i++)
    {
        F_destroy_basic_block(blocks[i]);
    }
    free(blocks);
    destroy_test_instructions(instructions, instruction_count);
}

// ============================================================================
// 测试用例：控制流图构建器
// ============================================================================

/**
 * @brief 测试控制流图构建器接口创建
 */
void test_cfg_builder_interface_create(void)
{
    TEST_MESSAGE("测试控制流图构建器接口创建");
    
    // 创建构建器接口
    Stru_CfgBuilderInterface_t* builder = F_create_cfg_builder_interface();
    TEST_ASSERT_NOT_NULL(builder);
    
    // 验证接口函数
    TEST_ASSERT_NOT_NULL(builder->get_name);
    TEST_ASSERT_NOT_NULL(builder->get_version);
    TEST_ASSERT_NOT_NULL(builder->build_from_instructions);
    TEST_ASSERT_NOT_NULL(builder->partition_basic_blocks);
    TEST_ASSERT_NOT_NULL(builder->destroy);
    
    // 获取名称和版本
    const char* name = builder->get_name();
    const char* version = builder->get_version();
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_NOT_NULL(version);
    
    // 销毁构建器
    F_destroy_cfg_builder_interface(builder);
}

/**
 * @brief 测试从指令数组构建控制流图
 */
void test_cfg_build_from_instructions(void)
{
    TEST_MESSAGE("测试从指令数组构建控制流图");
    
    // 创建构建器接口
    Stru_CfgBuilderInterface_t* builder = F_create_cfg_builder_interface();
    TEST_ASSERT_NOT_NULL(builder);
    
    // 创建测试指令数组
    size_t instruction_count = 25;
    Stru_IrInstruction_t** instructions = create_test_instructions(instruction_count);
    TEST_ASSERT_NOT_NULL(instructions);
    
    // 构建控制流图
    Stru_CfgBuildResult_t* result = builder->build_from_instructions(instructions, instruction_count);
    TEST_ASSERT_NOT_NULL(result);
    
    // 验证构建结果
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(result->cfg);
    TEST_ASSERT_EQUAL(instruction_count, result->instruction_count);
    TEST_ASSERT_TRUE(result->basic_block_count > 0);
    
    // 验证控制流图
    Stru_ControlFlowGraph_t* cfg = result->cfg;
    TEST_ASSERT_NOT_NULL(cfg->entry_block);
    TEST_ASSERT_NOT_NULL(cfg->exit_block);
    TEST_ASSERT_EQUAL(result->basic_block_count, cfg->block_count);
    
    // 清理
    builder->destroy_build_result(result);
    F_destroy_cfg_builder_interface(builder);
    destroy_test_instructions(instructions, instruction_count);
}

// ============================================================================
// 测试用例：控制流分析算法
// ============================================================================

/**
 * @brief 测试支配关系计算
 */
void test_compute_dominators(void)
{
    TEST_MESSAGE("测试支配关系计算");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
    Stru_BasicBlock_t* block3 = F_create_basic_block("block3", Eum_BASIC_BLOCK_NORMAL, 3);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    TEST_ASSERT_NOT_NULL(block3);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    F_cfg_add_basic_block(cfg, block2);
    F_cfg_add_basic_block(cfg, block3);
    
    // 设置控制流关系
    F_basic_block_add_successor(block1, block2);
    F_basic_block_add_successor(block1, block3);
    F_basic_block_add_predecessor(block2, block1);
    F_basic_block_add_predecessor(block3, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 计算支配关系
    bool result = F_compute_dominators(cfg);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

/**
 * @brief 测试后序遍历计算
 */
void test_compute_postorder(void)
{
    TEST_MESSAGE("测试后序遍历计算");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    F_cfg_add_basic_block(cfg, block2);
    
    // 设置控制流关系
    F_basic_block_add_successor(block1, block2);
    F_basic_block_add_predecessor(block2, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 计算后序遍历
    bool result = F_compute_postorder(cfg);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

/**
 * @brief 测试循环识别
 */
void test_identify_loops(void)
{
    TEST_MESSAGE("测试循环识别");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    F_cfg_add_basic_block(cfg, block2);
    
    // 创建循环关系
    F_basic_block_add_successor(block1, block2);
    F_basic_block_add_successor(block2, block1);
    F_basic_block_add_predecessor(block1, block2);
    F_basic_block_add_predecessor(block2, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 识别循环
    bool result = F_identify_loops(cfg);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

// ============================================================================
// 测试用例：数据流分析算法
// ============================================================================

/**
 * @brief 测试到达定义计算
 */
void test_compute_reaching_definitions(void)
{
    TEST_MESSAGE("测试到达定义计算");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    TEST_ASSERT_NOT_NULL(block1);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 计算到达定义
    bool result = F_compute_reaching_definitions(cfg);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

/**
 * @brief 测试活跃变量计算
 */
void test_compute_live_variables(void)
{
    TEST_MESSAGE("测试活跃变量计算");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    TEST_ASSERT_NOT_NULL(block1);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 计算活跃变量
    bool result = F_compute_live_variables(cfg);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

/**
 * @brief 测试可用表达式计算
 */
void test_compute_available_expressions(void)
{
    TEST_MESSAGE("测试可用表达式计算");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    TEST_ASSERT_NOT_NULL(block1);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 计算可用表达式
    bool result = F_compute_available_expressions(cfg);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

// ============================================================================
// 测试用例：控制流图优化
// ============================================================================

/**
 * @brief 测试控制流图优化
 */
void test_optimize_control_flow_graph(void)
{
    TEST_MESSAGE("测试控制流图优化");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    F_cfg_add_basic_block(cfg, block2);
    
    // 设置控制流关系
    F_basic_block_add_successor(block1, block2);
    F_basic_block_add_predecessor(block2, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 优化控制流图（级别1）
    bool result = F_optimize_control_flow_graph(cfg, 1);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

/**
 * @brief 测试移除不可达基本块
 */
void test_remove_unreachable_blocks(void)
{
    TEST_MESSAGE("测试移除不可达基本块");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    F_cfg_add_basic_block(cfg, block2);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 移除不可达基本块
    bool result = F_remove_unreachable_blocks(cfg);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

/**
 * @brief 测试合并基本块
 */
void test_merge_basic_blocks(void)
{
    TEST_MESSAGE("测试合并基本块");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    F_cfg_add_basic_block(cfg, block2);
    
    // 设置控制流关系
    F_basic_block_add_successor(block1, block2);
    F_basic_block_add_predecessor(block2, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 合并基本块
    bool result = F_merge_basic_blocks(cfg);
    TEST_ASSERT_TRUE(result);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

// ============================================================================
// 测试用例：控制流图验证
// ============================================================================

/**
 * @brief 测试控制流图验证
 */
void test_validate_control_flow_graph(void)
{
    TEST_MESSAGE("测试控制流图验证");
    
    // 创建控制流图
    Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("test_cfg", 1);
    TEST_ASSERT_NOT_NULL(cfg);
    
    // 创建基本块
    Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
    TEST_ASSERT_NOT_NULL(block1);
    
    // 添加基本块到控制流图
    F_cfg_add_basic_block(cfg, block1);
    
    // 设置入口块
    F_cfg_set_entry_block(cfg, block1);
    
    // 验证控制流图
    char* error_message = NULL;
    bool result = F_validate_control_flow_graph_comprehensive(cfg, &error_message);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(error_message);
    
    // 清理
    F_destroy_control_flow_graph(cfg);
}

// ============================================================================
// 测试主函数
// ============================================================================

/**
 * @brief 测试运行前设置
 */
void setUp(void)
{
    // 测试运行前的设置代码
}

/**
 * @brief 测试运行后清理
 */
void tearDown(void)
{
    // 测试运行后的清理代码
}

/**
 * @brief 测试主函数
 * 
 * 运行所有控制流图集成测试。
 * 
 * @return int 测试结果
 */
int main(void)
{
    // 初始化Unity测试框架
    UNITY_BEGIN();
    
    // 运行基本块数据结构测试
    RUN_TEST(test_basic_block_create_destroy);
    RUN_TEST(test_basic_block_instruction_management);
    RUN_TEST(test_basic_block_control_flow_relations);
    
    // 运行控制流图数据结构测试
    RUN_TEST(test_control_flow_graph_create_destroy);
    RUN_TEST(test_control_flow_graph_block_management);
    
    // 运行基本块划分算法测试
    RUN_TEST(test_basic_block_partition_simple);
    RUN_TEST(test_basic_block_partition_advanced);
    RUN_TEST(test_basic_block_partition_optimized);
    
    // 运行控制流图构建器测试
    RUN_TEST(test_cfg_builder_interface_create);
    RUN_TEST(test_cfg_build_from_instructions);
    
    // 运行控制流分析算法测试
    RUN_TEST(test_compute_dominators);
    RUN_TEST(test_compute_postorder);
    RUN_TEST(test_identify_loops);
    
    // 运行数据流分析算法测试
    RUN_TEST(test_compute_reaching_definitions);
    RUN_TEST(test_compute_live_variables);
    RUN_TEST(test_compute_available_expressions);
    
    // 运行控制流图优化测试
    RUN_TEST(test_optimize_control_flow_graph);
    RUN_TEST(test_remove_unreachable_blocks);
    RUN_TEST(test_merge_basic_blocks);
    
    // 运行控制流图验证测试
    RUN_TEST(test_validate_control_flow_graph);
    
    // 结束测试
    return UNITY_END();
}
