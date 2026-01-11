/******************************************************************************
 * 文件名: CN_ir_interface.c
 * 功能: CN_Language 中间代码表示（IR）接口实现
 * 
 * 实现IR接口的工厂函数和工具函数。
 * 提供创建、管理和销毁IR接口实例的功能。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ir_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 包含TAC实现
#include "implementations/tac/CN_tac_interface.h"

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 支持的IR类型数组
 */
static const Eum_IrType g_supported_ir_types[] = {
    Eum_IR_TYPE_TAC,
    // 未来可以添加更多IR类型
};

/**
 * @brief 支持的IR类型数量
 */
static const size_t g_supported_ir_types_count = 
    sizeof(g_supported_ir_types) / sizeof(g_supported_ir_types[0]);

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建IR接口实例
 */
Stru_IrInterface_t* F_create_ir_interface(Eum_IrType type)
{
    switch (type) {
        case Eum_IR_TYPE_TAC:
            return F_create_tac_interface();
            
        case Eum_IR_TYPE_SSA:
        case Eum_IR_TYPE_HIR:
        case Eum_IR_TYPE_MIR:
        case Eum_IR_TYPE_LIR:
            // 这些类型尚未实现
            return NULL;
            
        default:
            return NULL;
    }
}

/**
 * @brief 销毁IR接口实例
 */
void F_destroy_ir_interface(Stru_IrInterface_t* interface)
{
    if (!interface) {
        return;
    }
    
    // 调用接口的销毁函数
    if (interface->destroy) {
        interface->destroy(interface->internal_state);
    }
    
    free(interface);
}

/**
 * @brief 获取支持的IR类型
 */
bool F_get_supported_ir_types(Eum_IrType** types, size_t* count)
{
    if (!types || !count) {
        return false;
    }
    
    // 分配内存
    *types = (Eum_IrType*)malloc(g_supported_ir_types_count * sizeof(Eum_IrType));
    if (!*types) {
        return false;
    }
    
    // 复制数据
    memcpy(*types, g_supported_ir_types, g_supported_ir_types_count * sizeof(Eum_IrType));
    *count = g_supported_ir_types_count;
    
    return true;
}

/**
 * @brief 检查IR类型是否支持
 */
bool F_is_ir_type_supported(Eum_IrType type)
{
    for (size_t i = 0; i < g_supported_ir_types_count; i++) {
        if (g_supported_ir_types[i] == type) {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// 工具函数实现
// ============================================================================

/**
 * @brief IR类型转字符串
 */
const char* F_ir_type_to_string(Eum_IrType type)
{
    switch (type) {
        case Eum_IR_TYPE_TAC: return "Three-Address Code (TAC)";
        case Eum_IR_TYPE_SSA: return "Static Single Assignment (SSA)";
        case Eum_IR_TYPE_HIR: return "High-Level IR (HIR)";
        case Eum_IR_TYPE_MIR: return "Medium-Level IR (MIR)";
        case Eum_IR_TYPE_LIR: return "Low-Level IR (LIR)";
        default: return "Unknown IR Type";
    }
}

/**
 * @brief IR指令类型转字符串
 */
const char* F_ir_instruction_type_to_string(Eum_IrInstructionType type)
{
    switch (type) {
        case Eum_IR_INSTR_ASSIGN: return "ASSIGN";
        case Eum_IR_INSTR_BINARY_OP: return "BINARY_OP";
        case Eum_IR_INSTR_UNARY_OP: return "UNARY_OP";
        case Eum_IR_INSTR_LOAD: return "LOAD";
        case Eum_IR_INSTR_STORE: return "STORE";
        case Eum_IR_INSTR_CALL: return "CALL";
        case Eum_IR_INSTR_RETURN: return "RETURN";
        case Eum_IR_INSTR_BRANCH: return "BRANCH";
        case Eum_IR_INSTR_JUMP: return "JUMP";
        case Eum_IR_INSTR_PHI: return "PHI";
        case Eum_IR_INSTR_LABEL: return "LABEL";
        case Eum_IR_INSTR_FUNCTION: return "FUNCTION";
        case Eum_IR_INSTR_PARAM: return "PARAM";
        case Eum_IR_INSTR_ALLOCA: return "ALLOCA";
        case Eum_IR_INSTR_GET_ELEMENT_PTR: return "GET_ELEMENT_PTR";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 创建IR指令
 */
Stru_IrInstruction_t* F_create_ir_instruction(Eum_IrInstructionType type, 
                                              const char* opcode,
                                              size_t line, size_t column)
{
    Stru_IrInstruction_t* instr = (Stru_IrInstruction_t*)malloc(sizeof(Stru_IrInstruction_t));
    if (!instr) {
        return NULL;
    }
    
    instr->type = type;
    instr->opcode = opcode ? strdup(opcode) : NULL;
    instr->operands = NULL;
    instr->operand_count = 0;
    instr->line = line;
    instr->column = column;
    instr->extra_data = NULL;
    
    return instr;
}

/**
 * @brief 销毁IR指令
 */
void F_destroy_ir_instruction(Stru_IrInstruction_t* instruction)
{
    if (!instruction) {
        return;
    }
    
    free(instruction->opcode);
    
    // 销毁操作数
    for (size_t i = 0; i < instruction->operand_count; i++) {
        free(instruction->operands[i]);
    }
    free(instruction->operands);
    
    // 注意：extra_data 由调用者管理，这里不释放
    
    free(instruction);
}

/**
 * @brief 添加操作数到IR指令
 */
bool F_ir_instruction_add_operand(Stru_IrInstruction_t* instruction, const char* operand)
{
    if (!instruction || !operand) {
        return false;
    }
    
    // 重新分配内存
    char** new_operands = (char**)realloc(
        instruction->operands, 
        (instruction->operand_count + 1) * sizeof(char*));
    
    if (!new_operands) {
        return false;
    }
    
    instruction->operands = new_operands;
    instruction->operands[instruction->operand_count] = strdup(operand);
    
    if (!instruction->operands[instruction->operand_count]) {
        return false;
    }
    
    instruction->operand_count++;
    return true;
}

/**
 * @brief 获取IR指令的操作数
 */
const char* F_ir_instruction_get_operand(const Stru_IrInstruction_t* instruction, size_t index)
{
    if (!instruction || index >= instruction->operand_count) {
        return NULL;
    }
    
    return instruction->operands[index];
}

/**
 * @brief 设置IR指令的额外数据
 */
void F_ir_instruction_set_extra_data(Stru_IrInstruction_t* instruction, void* extra_data)
{
    if (!instruction) {
        return;
    }
    
    instruction->extra_data = extra_data;
}

/**
 * @brief 获取IR指令的额外数据
 */
void* F_ir_instruction_get_extra_data(const Stru_IrInstruction_t* instruction)
{
    return instruction ? instruction->extra_data : NULL;
}

// ============================================================================
// 版本信息函数
// ============================================================================

/**
 * @brief 获取IR模块版本信息
 */
void F_get_ir_module_version(int* major, int* minor, int* patch)
{
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (patch) *patch = 0;
}

/**
 * @brief 获取IR模块版本字符串
 */
const char* F_get_ir_module_version_string(void)
{
    return "1.0.0";
}
