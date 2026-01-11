/******************************************************************************
 * 文件名: test_ir_interface.c
 * 功能: CN_Language IR接口测试
 * 
 * 测试IR接口工厂函数和工具函数。
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
#include "../../../src/core/ir/CN_ir_interface.h"

/**
 * @brief 测试IR工厂函数
 * 
 * 验证IR工厂函数能正确创建IR实例。
 */
static bool test_ir_factory_create_ir(void)
{
    printf("测试: IR工厂函数\n");
    
    // 测试创建TAC IR
    Stru_IrInterface_t* ir = F_create_ir_interface(Eum_IR_TYPE_TAC);
    TEST_ASSERT_NOT_NULL(ir);
    TEST_ASSERT_NOT_NULL(ir->get_name);
    TEST_ASSERT_NOT_NULL(ir->get_version);
    TEST_ASSERT_NOT_NULL(ir->get_type);
    TEST_ASSERT_NOT_NULL(ir->add_instruction);
    TEST_ASSERT_NOT_NULL(ir->get_instruction_count);
    TEST_ASSERT_NOT_NULL(ir->get_instruction);
    TEST_ASSERT_NOT_NULL(ir->serialize);
    TEST_ASSERT_NOT_NULL(ir->deserialize);
    TEST_ASSERT_NOT_NULL(ir->destroy);
    
    // 测试基本信息
    const char* name = ir->get_name();
    TEST_ASSERT_NOT_NULL(name);
    
    const char* version = ir->get_version();
    TEST_ASSERT_NOT_NULL(version);
    
    Eum_IrType type = ir->get_type();
    TEST_ASSERT_EQUAL(Eum_IR_TYPE_TAC, type);
    
    // 清理
    F_destroy_ir_interface(ir);
    
    return true;
}

/**
 * @brief 测试IR工具函数
 * 
 * 验证IR工具函数的正确性。
 */
static bool test_ir_utility_functions(void)
{
    printf("测试: IR工具函数\n");
    
    // 测试获取支持的IR类型
    Eum_IrType* types = NULL;
    size_t count = 0;
    bool result = F_get_supported_ir_types(&types, &count);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(types);
    TEST_ASSERT_EQUAL_UINT(1, count); // 目前只支持TAC
    TEST_ASSERT_EQUAL(Eum_IR_TYPE_TAC, types[0]);
    
    // 测试检查IR类型是否支持
    bool supported = F_is_ir_type_supported(Eum_IR_TYPE_TAC);
    TEST_ASSERT_TRUE(supported);
    
    supported = F_is_ir_type_supported(Eum_IR_TYPE_SSA);
    TEST_ASSERT_FALSE(supported); // SSA尚未实现
    
    // 测试IR类型转字符串
    const char* tac_str = F_ir_type_to_string(Eum_IR_TYPE_TAC);
    TEST_ASSERT_NOT_NULL(tac_str);
    TEST_ASSERT_EQUAL_STRING("Three-Address Code (TAC)", tac_str);
    
    const char* unknown_str = F_ir_type_to_string(999);
    TEST_ASSERT_NOT_NULL(unknown_str);
    TEST_ASSERT_EQUAL_STRING("Unknown IR Type", unknown_str);
    
    // 测试IR指令类型转字符串
    const char* assign_str = F_ir_instruction_type_to_string(Eum_IR_INSTR_ASSIGN);
    TEST_ASSERT_NOT_NULL(assign_str);
    TEST_ASSERT_EQUAL_STRING("ASSIGN", assign_str);
    
    const char* unknown_instr_str = F_ir_instruction_type_to_string(999);
    TEST_ASSERT_NOT_NULL(unknown_instr_str);
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", unknown_instr_str);
    
    // 测试版本信息
    int major = 0, minor = 0, patch = 0;
    F_get_ir_module_version(&major, &minor, &patch);
    TEST_ASSERT_EQUAL(1, major);
    TEST_ASSERT_EQUAL(0, minor);
    TEST_ASSERT_EQUAL(0, patch);
    
    const char* version_str = F_get_ir_module_version_string();
    TEST_ASSERT_NOT_NULL(version_str);
    TEST_ASSERT_EQUAL_STRING("1.0.0", version_str);
    
    // 清理
    free(types);
    
    return true;
}

/**
 * @brief 测试IR错误处理
 * 
 * 验证IR模块的错误处理机制。
 */
static bool test_ir_error_handling(void)
{
    printf("测试: IR错误处理\n");
    
    // 测试无效IR类型
    Stru_IrInterface_t* invalid_ir = F_create_ir_interface(999); // 无效类型
    TEST_ASSERT_NULL(invalid_ir);
    
    // 测试空指针处理
    F_destroy_ir_interface(NULL); // 应该安全处理
    
    // 测试无效参数
    Eum_IrType* types = NULL;
    size_t count = 0;
    bool result = F_get_supported_ir_types(NULL, &count);
    TEST_ASSERT_FALSE(result);
    
    result = F_get_supported_ir_types(&types, NULL);
    TEST_ASSERT_FALSE(result);
    
    // 测试IR指令创建错误处理
    Stru_IrInstruction_t* instr = F_create_ir_instruction(Eum_IR_INSTR_ASSIGN, NULL, 1, 1);
    TEST_ASSERT_NOT_NULL(instr);
    
    // 测试添加操作数错误处理
    bool add_result = F_ir_instruction_add_operand(NULL, "operand");
    TEST_ASSERT_FALSE(add_result);
    
    add_result = F_ir_instruction_add_operand(instr, NULL);
    TEST_ASSERT_FALSE(add_result);
    
    // 测试获取操作数错误处理
    const char* operand = F_ir_instruction_get_operand(instr, 0);
    TEST_ASSERT_NULL(operand);
    
    operand = F_ir_instruction_get_operand(NULL, 0);
    TEST_ASSERT_NULL(operand);
    
    // 清理
    F_destroy_ir_instruction(instr);
    
    return true;
}

/**
 * @brief 测试IR指令操作
 * 
 * 验证IR指令的创建和操作功能。
 */
static bool test_ir_instruction_operations(void)
{
    printf("测试: IR指令操作\n");
    
    // 测试创建IR指令
    Stru_IrInstruction_t* instr = F_create_ir_instruction(Eum_IR_INSTR_BINARY_OP, "ADD", 10, 5);
    TEST_ASSERT_NOT_NULL(instr);
    TEST_ASSERT_EQUAL(Eum_IR_INSTR_BINARY_OP, instr->type);
    TEST_ASSERT_NOT_NULL(instr->opcode);
    TEST_ASSERT_EQUAL_STRING("ADD", instr->opcode);
    TEST_ASSERT_EQUAL(10, instr->line);
    TEST_ASSERT_EQUAL(5, instr->column);
    TEST_ASSERT_EQUAL_UINT(0, instr->operand_count);
    TEST_ASSERT_NULL(instr->operands);
    TEST_ASSERT_NULL(instr->extra_data);
    
    // 测试添加操作数
    bool add_result = F_ir_instruction_add_operand(instr, "x");
    TEST_ASSERT_TRUE(add_result);
    TEST_ASSERT_EQUAL_UINT(1, instr->operand_count);
    
    add_result = F_ir_instruction_add_operand(instr, "y");
    TEST_ASSERT_TRUE(add_result);
    TEST_ASSERT_EQUAL_UINT(2, instr->operand_count);
    
    add_result = F_ir_instruction_add_operand(instr, "z");
    TEST_ASSERT_TRUE(add_result);
    TEST_ASSERT_EQUAL_UINT(3, instr->operand_count);
    
    // 测试获取操作数
    const char* operand1 = F_ir_instruction_get_operand(instr, 0);
    TEST_ASSERT_NOT_NULL(operand1);
    TEST_ASSERT_EQUAL_STRING("x", operand1);
    
    const char* operand2 = F_ir_instruction_get_operand(instr, 1);
    TEST_ASSERT_NOT_NULL(operand2);
    TEST_ASSERT_EQUAL_STRING("y", operand2);
    
    const char* operand3 = F_ir_instruction_get_operand(instr, 2);
    TEST_ASSERT_NOT_NULL(operand3);
    TEST_ASSERT_EQUAL_STRING("z", operand3);
    
    // 测试无效索引
    const char* invalid_operand = F_ir_instruction_get_operand(instr, 3);
    TEST_ASSERT_NULL(invalid_operand);
    
    // 测试额外数据操作
    int extra_data = 42;
    F_ir_instruction_set_extra_data(instr, &extra_data);
    void* retrieved_data = F_ir_instruction_get_extra_data(instr);
    TEST_ASSERT_NOT_NULL(retrieved_data);
    TEST_ASSERT_EQUAL(&extra_data, retrieved_data);
    
    // 清理
    F_destroy_ir_instruction(instr);
    
    return true;
}

/**
 * @brief 运行所有IR接口测试
 * 
 * 运行IR接口的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_ir_interface_all(void)
{
    printf("开始运行IR接口测试...\n");
    printf("================================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_ir_factory_create_ir()) {
        printf("❌ test_ir_factory_create_ir 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_ir_factory_create_ir 通过\n");
    }
    
    if (!test_ir_utility_functions()) {
        printf("❌ test_ir_utility_functions 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_ir_utility_functions 通过\n");
    }
    
    if (!test_ir_error_handling()) {
        printf("❌ test_ir_error_handling 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_ir_error_handling 通过\n");
    }
    
    if (!test_ir_instruction_operations()) {
        printf("❌ test_ir_instruction_operations 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_ir_instruction_operations 通过\n");
    }
    
    printf("\n================================\n");
    if (all_passed) {
        printf("✅ 所有IR接口测试通过！\n");
    } else {
        printf("❌ 有IR接口测试失败\n");
    }
    
    return all_passed;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_ir_interface_all()) {
        return 0;
    } else {
        return 1;
    }
}
#endif
