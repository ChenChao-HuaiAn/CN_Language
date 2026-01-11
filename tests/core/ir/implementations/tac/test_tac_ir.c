/******************************************************************************
 * 文件名: test_tac_ir.c
 * 功能: CN_Language TAC IR实现测试
 * 
 * 测试三地址码（TAC）IR的具体实现。
 * 遵循项目测试规范，使用模块化测试结构。
 * 
 * 作者: CN_Language测试团队
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
#endif

/* 包含被测试的头文件 */
#include "../../../../src/core/ir/implementations/tac/CN_tac_interface.h"

/**
 * @brief 测试TAC指令创建
 * 
 * 验证TAC指令的创建和销毁功能。
 */
static bool test_tac_instruction_creation(void)
{
    printf("测试: TAC指令创建\n");
    
    // 测试创建TAC指令
    Stru_TacInstruction_t* instr = F_create_tac_instruction(
        Eum_TAC_OP_ADD, 
        "result", 
        "operand1", 
        "operand2", 
        NULL, 
        10, 5);
    
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(Eum_TAC_OP_ADD, instr->opcode);
    TEST_ASSERT_NOT_NULL(instr->result);
    TEST_ASSERT_EQUAL_STRING("result", instr->result);
    TEST_ASSERT_NOT_NULL(instr->operand1);
    TEST_ASSERT_EQUAL_STRING("operand1", instr->operand1);
    TEST_ASSERT_NOT_NULL(instr->operand2);
    TEST_ASSERT_EQUAL_STRING("operand2", instr->operand2);
    TEST_ASSERT_NULL(instr->label);
    TEST_ASSERT_EQUAL(10, instr->line);
    TEST_ASSERT_EQUAL(5, instr->column);
    TEST_ASSERT_NULL(instr->extra_data);
    
    // 测试创建带标签的TAC指令
    Stru_TacInstruction_t* labeled_instr = F_create_tac_instruction(
        Eum_TAC_OP_GOTO, 
        NULL, 
        NULL, 
        NULL, 
        "loop_start", 
        20, 1);
    
    TEST_ASSERT_NOT_NULL(labeled_instr);
    TEST_ASSERT_EQUAL(Eum_TAC_OP_GOTO, labeled_instr->opcode);
    TEST_ASSERT_NULL(labeled_instr->result);
    TEST_ASSERT_NULL(labeled_instr->operand1);
    TEST_ASSERT_NULL(labeled_instr->operand2);
    TEST_ASSERT_NOT_NULL(labeled_instr->label);
    TEST_ASSERT_EQUAL_STRING("loop_start", labeled_instr->label);
    TEST_ASSERT_EQUAL(20, labeled_instr->line);
    TEST_ASSERT_EQUAL(1, labeled_instr->column);
    
    // 清理
    F_destroy_tac_instruction(instr);
    F_destroy_tac_instruction(labeled_instr);
    
    return true;
}

/**
 * @brief 测试TAC操作码转换
 * 
 * 验证TAC操作码与字符串之间的转换功能。
 */
static bool test_tac_opcode_conversion(void)
{
    printf("测试: TAC操作码转换\n");
    
    // 测试操作码转字符串
    const char* add_str = F_tac_opcode_to_string(Eum_TAC_OP_ADD);
    TEST_ASSERT_NOT_NULL(add_str);
    TEST_ASSERT_EQUAL_STRING("ADD", add_str);
    
    const char* assign_str = F_tac_opcode_to_string(Eum_TAC_OP_ASSIGN);
    TEST_ASSERT_NOT_NULL(assign_str);
    TEST_ASSERT_EQUAL_STRING("ASSIGN", assign_str);
    
    const char* goto_str = F_tac_opcode_to_string(Eum_TAC_OP_GOTO);
    TEST_ASSERT_NOT_NULL(goto_str);
    TEST_ASSERT_EQUAL_STRING("GOTO", goto_str);
    
    // 测试字符串转操作码
    Eum_TacOpcode add_opcode = F_string_to_tac_opcode("ADD");
    TEST_ASSERT_EQUAL(Eum_TAC_OP_ADD, add_opcode);
    
    Eum_TacOpcode assign_opcode = F_string_to_tac_opcode("ASSIGN");
    TEST_ASSERT_EQUAL(Eum_TAC_OP_ASSIGN, assign_opcode);
    
    Eum_TacOpcode goto_opcode = F_string_to_tac_opcode("GOTO");
    TEST_ASSERT_EQUAL(Eum_TAC_OP_GOTO, goto_opcode);
    
    // 测试无效字符串
    Eum_TacOpcode invalid_opcode = F_string_to_tac_opcode("INVALID_OPCODE");
    TEST_ASSERT_EQUAL(Eum_TAC_OP_ASSIGN, invalid_opcode); // 默认返回ASSIGN
    
    return true;
}

/**
 * @brief 测试TAC指令格式化
 * 
 * 验证TAC指令的格式化功能。
 */
static bool test_tac_instruction_formatting(void)
{
    printf("测试: TAC指令格式化\n");
    
    // 测试格式化二元运算指令
    Stru_TacInstruction_t* add_instr = F_create_tac_instruction(
        Eum_TAC_OP_ADD, 
        "t1", 
        "x", 
        "y", 
        NULL, 
        15, 3);
    
    char* formatted = F_format_tac_instruction(add_instr);
    TEST_ASSERT_NOT_NULL(formatted);
    TEST_ASSERT_GREATER_THAN(0, strlen(formatted));
    
    // 检查格式化结果是否包含预期内容
    TEST_ASSERT_TRUE(strstr(formatted, "ADD") != NULL);
    TEST_ASSERT_TRUE(strstr(formatted, "t1") != NULL);
    TEST_ASSERT_TRUE(strstr(formatted, "x") != NULL);
    TEST_ASSERT_TRUE(strstr(formatted, "y") != NULL);
    
    free(formatted);
    
    // 测试格式化跳转指令
    Stru_TacInstruction_t* goto_instr = F_create_tac_instruction(
        Eum_TAC_OP_GOTO, 
        NULL, 
        NULL, 
        NULL, 
        "loop_end", 
        25, 1);
    
    formatted = F_format_tac_instruction(goto_instr);
    TEST_ASSERT_NOT_NULL(formatted);
    TEST_ASSERT_GREATER_THAN(0, strlen(formatted));
    TEST_ASSERT_TRUE(strstr(formatted, "GOTO") != NULL);
    TEST_ASSERT_TRUE(strstr(formatted, "loop_end") != NULL);
    
    free(formatted);
    
    // 清理
    F_destroy_tac_instruction(add_instr);
    F_destroy_tac_instruction(goto_instr);
    
    return true;
}

/**
 * @brief 测试TAC数据操作
 * 
 * 验证TAC数据的创建、管理和销毁功能。
 */
static bool test_tac_data_operations(void)
{
    printf("测试: TAC数据操作\n");
    
    // 测试创建TAC数据
    Stru_TacData_t* data = F_create_tac_data();
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(0, data->instruction_count);
    TEST_ASSERT_EQUAL_UINT(0, data->temp_count);
    TEST_ASSERT_EQUAL_UINT(0, data->label_count);
    TEST_ASSERT_FALSE(data->has_errors);
    TEST_ASSERT_NULL(data->errors);
    
    // 测试添加TAC指令
    Stru_TacInstruction_t* instr1 = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN, "x", "10", NULL, NULL, 1, 1);
    
    bool add_result = F_tac_data_add_instruction(data, instr1);
    TEST_ASSERT_TRUE(add_result);
    TEST_ASSERT_EQUAL_UINT(1, data->instruction_count);
    
    // 测试获取TAC指令
    Stru_TacInstruction_t* retrieved_instr = F_tac_data_get_instruction(data, 0);
    TEST_ASSERT_NOT_NULL(retrieved_instr);
    TEST_ASSERT_EQUAL(Eum_TAC_OP_ASSIGN, retrieved_instr->opcode);
    TEST_ASSERT_EQUAL_STRING("x", retrieved_instr->result);
    TEST_ASSERT_EQUAL_STRING("10", retrieved_instr->operand1);
    
    // 测试无效索引
    Stru_TacInstruction_t* invalid_instr = F_tac_data_get_instruction(data, 1);
    TEST_ASSERT_NULL(invalid_instr);
    
    // 测试获取指令数量
    size_t count = F_tac_data_get_instruction_count(data);
    TEST_ASSERT_EQUAL_UINT(1, count);
    
    // 测试错误处理
    F_tac_data_add_error(data, "测试错误信息");
    TEST_ASSERT_TRUE(F_tac_data_has_errors(data));
    
    const char* errors = F_tac_data_get_errors(data);
    TEST_ASSERT_NOT_NULL(errors);
    TEST_ASSERT_TRUE(strstr(errors, "测试错误信息") != NULL);
    
    // 测试清除错误
    F_tac_data_clear_errors(data);
    TEST_ASSERT_FALSE(F_tac_data_has_errors(data));
    TEST_ASSERT_NULL(F_tac_data_get_errors(data));
    
    // 清理
    F_destroy_tac_data(data);
    
    return true;
}

/**
 * @brief 测试TAC接口创建
 * 
 * 验证TAC接口的创建和基本功能。
 */
static bool test_tac_interface_creation(void)
{
    printf("测试: TAC接口创建\n");
    
    // 测试创建TAC接口
    Stru_IrInterface_t* tac_interface = F_create_tac_interface();
    TEST_ASSERT_NOT_NULL(tac_interface);
    
    // 测试接口基本信息
    const char* name = tac_interface->get_name();
    TEST_ASSERT_NOT_NULL(name);
    
    const char* version = tac_interface->get_version();
    TEST_ASSERT_NOT_NULL(version);
    
    Eum_IrType type = tac_interface->get_type();
    TEST_ASSERT_EQUAL(Eum_IR_TYPE_TAC, type);
    
    // 测试创建空IR
    void* ir = tac_interface->create_empty();
    TEST_ASSERT_NOT_NULL(ir);
    
    // 测试指令计数
    size_t count = tac_interface->get_instruction_count(ir);
    TEST_ASSERT_EQUAL_UINT(0, count);
    
    // 测试错误检查
    bool has_errors = tac_interface->has_errors(ir);
    TEST_ASSERT_FALSE(has_errors);
    
    // 测试验证
    bool valid = tac_interface->validate(ir);
    TEST_ASSERT_TRUE(valid);
    
    // 清理
    tac_interface->destroy(ir);
    
    // 注意：这里不调用F_destroy_ir_interface，因为TAC接口有自己的销毁逻辑
    // 在TAC实现中，F_create_tac_interface返回的接口应该由调用者通过F_destroy_ir_interface销毁
    
    return true;
}

/**
 * @brief 运行所有TAC IR测试
 * 
 * 运行TAC IR的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_tac_ir_all(void)
{
    printf("开始运行TAC IR测试...\n");
    printf("================================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_tac_instruction_creation()) {
        printf("❌ test_tac_instruction_creation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_tac_instruction_creation 通过\n");
    }
    
    if (!test_tac_opcode_conversion()) {
        printf("❌ test_tac_opcode_conversion 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_tac_opcode_conversion 通过\n");
    }
    
    if (!test_tac_instruction_formatting()) {
        printf("❌ test_tac_instruction_formatting 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_tac_instruction_formatting 通过\n");
    }
    
    if (!test_tac_data_operations()) {
        printf("❌ test_tac_data_operations 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_tac_data_operations 通过\n");
    }
    
    if (!test_tac_interface_creation()) {
        printf("❌ test_tac_interface_creation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_tac_interface_creation 通过\n");
    }
    
    printf("\n================================\n");
    if (all_passed) {
        printf("✅ 所有TAC IR测试通过！\n");
    } else {
        printf("❌ 有TAC IR测试失败\n");
    }
    
    return all_passed;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_tac_ir_all()) {
        return 0;
    } else {
        return 1;
    }
}
#endif
