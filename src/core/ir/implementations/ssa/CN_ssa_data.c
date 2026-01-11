/******************************************************************************
 * 文件名: CN_ssa_data.c
 * 功能: CN_Language 静态单赋值形式（SSA）数据结构实现
 * 
 * 实现SSA数据结构的相关函数，包括创建、销毁、操作等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_interface.h"
#include "../../../infrastructure/memory/utilities/CN_memory_utilities.h"
#include "../../../infrastructure/containers/string/CN_string.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static char* cn_strdup(const char* str);
static char* F_generate_ssa_variable_name(const char* base_name, int version);
static bool F_ssa_data_resize_functions(Stru_SsaData_t* data, size_t new_capacity);
static bool F_ssa_function_resize_blocks(Stru_SsaFunction_t* function, size_t new_capacity);
static bool F_ssa_basic_block_resize_instructions(Stru_SsaBasicBlock_t* block, size_t new_capacity);
static bool F_ssa_basic_block_resize_phi_functions(Stru_SsaBasicBlock_t* block, size_t new_capacity);

// ============================================================================
// SSA变量函数实现
// ============================================================================

/**
 * @brief 创建SSA变量
 */
Stru_SsaVariable_t* F_create_ssa_variable(const char* name, int version, bool is_temporary)
{
    if (name == NULL) {
        return NULL;
    }
    
    Stru_SsaVariable_t* variable = (Stru_SsaVariable_t*)cn_malloc(sizeof(Stru_SsaVariable_t));
    if (variable == NULL) {
        return NULL;
    }
    
    variable->name = cn_strdup(name);
    if (variable->name == NULL) {
        cn_free(variable);
        return NULL;
    }
    
    variable->version = version;
    variable->full_name = F_generate_ssa_variable_name(name, version);
    if (variable->full_name == NULL) {
        cn_free(variable->name);
        cn_free(variable);
        return NULL;
    }
    
    variable->is_temporary = is_temporary;
    variable->definition_point = 0;
    variable->type_info = NULL;
    
    return variable;
}

/**
 * @brief 销毁SSA变量
 */
void F_destroy_ssa_variable(Stru_SsaVariable_t* variable)
{
    if (variable == NULL) {
        return;
    }
    
    if (variable->name != NULL) {
        cn_free(variable->name);
    }
    
    if (variable->full_name != NULL) {
        cn_free(variable->full_name);
    }
    
    // 注意：type_info由调用者管理，这里不释放
    
    cn_free(variable);
}

/**
 * @brief 获取SSA变量的完整名称
 */
const char* F_ssa_variable_get_full_name(const Stru_SsaVariable_t* variable)
{
    if (variable == NULL) {
        return NULL;
    }
    
    return variable->full_name;
}

/**
 * @brief 生成SSA变量名
 */
static char* F_generate_ssa_variable_name(const char* base_name, int version)
{
    if (base_name == NULL) {
        return NULL;
    }
    
    // 计算需要的缓冲区大小
    // 格式: name_version
    size_t base_len = strlen(base_name);
    size_t version_len = 10; // 足够存储版本号
    size_t total_len = base_len + 1 + version_len + 1; // name + '_' + version + '\0'
    
    char* full_name = (char*)cn_malloc(total_len);
    if (full_name == NULL) {
        return NULL;
    }
    
    if (version >= 0) {
        snprintf(full_name, total_len, "%s_%d", base_name, version);
    } else {
        // 如果没有版本号，直接使用基础名称
        snprintf(full_name, total_len, "%s", base_name);
    }
    
    return full_name;
}

// ============================================================================
// SSA指令函数实现
// ============================================================================

/**
 * @brief 创建SSA指令
 */
Stru_SsaInstruction_t* F_create_ssa_instruction(Eum_SsaOpcode opcode,
                                                Stru_SsaVariable_t* result,
                                                size_t line, size_t column)
{
    Stru_SsaInstruction_t* instruction = (Stru_SsaInstruction_t*)cn_malloc(sizeof(Stru_SsaInstruction_t));
    if (instruction == NULL) {
        return NULL;
    }
    
    instruction->opcode = opcode;
    instruction->result = result;
    instruction->operands = NULL;
    instruction->operand_count = 0;
    instruction->phi_args = NULL;
    instruction->phi_arg_count = 0;
    instruction->labels = NULL;
    instruction->label_count = 0;
    instruction->line = line;
    instruction->column = column;
    instruction->extra_data = NULL;
    
    return instruction;
}

/**
 * @brief 销毁SSA指令
 */
void F_destroy_ssa_instruction(Stru_SsaInstruction_t* instruction)
{
    if (instruction == NULL) {
        return;
    }
    
    // 注意：result变量由调用者管理，这里不释放
    
    // 释放操作数数组（只释放数组，不释放变量本身）
    if (instruction->operands != NULL) {
        cn_free(instruction->operands);
    }
    
    // 释放φ参数数组
    if (instruction->phi_args != NULL) {
        for (size_t i = 0; i < instruction->phi_arg_count; i++) {
            Stru_PhiArgument_t* arg = instruction->phi_args[i];
            if (arg != NULL) {
                // 注意：arg->variable由调用者管理，这里不释放
                if (arg->block_name != NULL) {
                    cn_free(arg->block_name);
                }
                cn_free(arg);
            }
        }
        cn_free(instruction->phi_args);
    }
    
    // 释放标签数组
    if (instruction->labels != NULL) {
        for (size_t i = 0; i < instruction->label_count; i++) {
            if (instruction->labels[i] != NULL) {
                cn_free(instruction->labels[i]);
            }
        }
        cn_free(instruction->labels);
    }
    
    // 注意：extra_data由调用者管理，这里不释放
    
    cn_free(instruction);
}

/**
 * @brief 添加操作数到SSA指令
 */
bool F_ssa_instruction_add_operand(Stru_SsaInstruction_t* instruction, 
                                   Stru_SsaVariable_t* operand)
{
    if (instruction == NULL || operand == NULL) {
        return false;
    }
    
    // 重新分配操作数数组
    size_t new_count = instruction->operand_count + 1;
    Stru_SsaVariable_t** new_operands = (Stru_SsaVariable_t**)cn_realloc(
        instruction->operands, new_count * sizeof(Stru_SsaVariable_t*));
    
    if (new_operands == NULL) {
        return false;
    }
    
    instruction->operands = new_operands;
    instruction->operands[instruction->operand_count] = operand;
    instruction->operand_count = new_count;
    
    return true;
}

/**
 * @brief 添加φ参数到SSA指令
 */
bool F_ssa_instruction_add_phi_argument(Stru_SsaInstruction_t* instruction,
                                        Stru_SsaVariable_t* variable,
                                        const char* block_name,
                                        size_t block_index)
{
    if (instruction == NULL || variable == NULL || block_name == NULL) {
        return false;
    }
    
    // 创建φ参数
    Stru_PhiArgument_t* arg = (Stru_PhiArgument_t*)cn_malloc(sizeof(Stru_PhiArgument_t));
    if (arg == NULL) {
        return false;
    }
    
    arg->variable = variable;
    arg->block_name = cn_strdup(block_name);
    if (arg->block_name == NULL) {
        cn_free(arg);
        return false;
    }
    
    arg->block_index = block_index;
    
    // 重新分配φ参数数组
    size_t new_count = instruction->phi_arg_count + 1;
    Stru_PhiArgument_t** new_args = (Stru_PhiArgument_t**)cn_realloc(
        instruction->phi_args, new_count * sizeof(Stru_PhiArgument_t*));
    
    if (new_args == NULL) {
        cn_free(arg->block_name);
        cn_free(arg);
        return false;
    }
    
    instruction->phi_args = new_args;
    instruction->phi_args[instruction->phi_arg_count] = arg;
    instruction->phi_arg_count = new_count;
    
    return true;
}

/**
 * @brief 添加标签到SSA指令
 */
bool F_ssa_instruction_add_label(Stru_SsaInstruction_t* instruction, const char* label)
{
    if (instruction == NULL || label == NULL) {
        return false;
    }
    
    // 重新分配标签数组
    size_t new_count = instruction->label_count + 1;
    char** new_labels = (char**)cn_realloc(
        instruction->labels, new_count * sizeof(char*));
    
    if (new_labels == NULL) {
        return false;
    }
    
    // 复制标签字符串
    char* label_copy = cn_strdup(label);
    if (label_copy == NULL) {
        return false;
    }
    
    instruction->labels = new_labels;
    instruction->labels[instruction->label_count] = label_copy;
    instruction->label_count = new_count;
    
    return true;
}

// ============================================================================
// SSA基本块函数实现
// ============================================================================

/**
 * @brief 创建SSA基本块
 */
Stru_SsaBasicBlock_t* F_create_ssa_basic_block(const char* name)
{
    if (name == NULL) {
        return NULL;
    }
    
    Stru_SsaBasicBlock_t* block = (Stru_SsaBasicBlock_t*)cn_malloc(sizeof(Stru_SsaBasicBlock_t));
    if (block == NULL) {
        return NULL;
    }
    
    block->name = cn_strdup(name);
    if (block->name == NULL) {
        cn_free(block);
        return NULL;
    }
    
    block->instructions = NULL;
    block->instruction_count = 0;
    block->phi_functions = NULL;
    block->phi_count = 0;
    block->predecessors = NULL;
    block->predecessor_count = 0;
    block->successors = NULL;
    block->successor_count = 0;
    block->dominators = NULL;
    block->dominator_count = 0;
    block->immediate_dominator = NULL;
    block->in_set = NULL;
    block->out_set = NULL;
    block->def_set = NULL;
    block->use_set = NULL;
    
    return block;
}

/**
 * @brief 销毁SSA基本块
 */
void F_destroy_ssa_basic_block(Stru_SsaBasicBlock_t* block)
{
    if (block == NULL) {
        return;
    }
    
    if (block->name != NULL) {
        cn_free(block->name);
    }
    
    // 释放指令数组
    if (block->instructions != NULL) {
        for (size_t i = 0; i < block->instruction_count; i++) {
            F_destroy_ssa_instruction(block->instructions[i]);
        }
        cn_free(block->instructions);
    }
    
    // 释放φ函数数组
    if (block->phi_functions != NULL) {
        for (size_t i = 0; i < block->phi_count; i++) {
            F_destroy_ssa_instruction(block->phi_functions[i]);
        }
        cn_free(block->phi_functions);
    }
    
    // 注意：前驱、后继、支配者数组只包含指针，不释放指向的对象
    if (block->predecessors != NULL) {
        cn_free(block->predecessors);
    }
    
    if (block->successors != NULL) {
        cn_free(block->successors);
    }
    
    if (block->dominators != NULL) {
        cn_free(block->dominators);
    }
    
    // 注意：数据流集合由专门的模块管理，这里不释放
    
    cn_free(block);
}

/**
 * @brief 添加指令到SSA基本块
 */
bool F_ssa_basic_block_add_instruction(Stru_SsaBasicBlock_t* block, 
                                       Stru_SsaInstruction_t* instruction)
{
    if (block == NULL || instruction == NULL) {
        return false;
    }
    
    // 检查是否需要调整数组大小
    if (block->instruction_count >= block->instruction_count) { // 总是为真，需要修复
        size_t new_capacity = block->instruction_count == 0 ? 4 : block->instruction_count * 2;
        if (!F_ssa_basic_block_resize_instructions(block, new_capacity)) {
            return false;
        }
    }
    
    // 添加指令
    block->instructions[block->instruction_count] = instruction;
    block->instruction_count++;
    
    return true;
}

/**
 * @brief 添加φ函数到SSA基本块
 */
bool F_ssa_basic_block_add_phi_function(Stru_SsaBasicBlock_t* block,
                                        Stru_SsaInstruction_t* phi_instruction)
{
    if (block == NULL || phi_instruction == NULL) {
        return false;
    }
    
    if (phi_instruction->opcode != Eum_SSA_OP_PHI) {
        return false;
    }
    
    // 检查是否需要调整数组大小
    if (block->phi_count >= block->phi_count) { // 总是为真，需要修复
        size_t new_capacity = block->phi_count == 0 ? 4 : block->phi_count * 2;
        if (!F_ssa_basic_block_resize_phi_functions(block, new_capacity)) {
            return false;
        }
    }
    
    // 添加φ函数
    block->phi_functions[block->phi_count] = phi_instruction;
    block->phi_count++;
    
    return true;
}

// ============================================================================
// SSA函数函数实现
// ============================================================================

/**
 * @brief 创建SSA函数
 */
Stru_SsaFunction_t* F_create_ssa_function(const char* name)
{
    if (name == NULL) {
        return NULL;
    }
    
    Stru_SsaFunction_t* function = (Stru_SsaFunction_t*)cn_malloc(sizeof(Stru_SsaFunction_t));
    if (function == NULL) {
        return NULL;
    }
    
    function->name = cn_strdup(name);
    if (function->name == NULL) {
        cn_free(function);
        return NULL;
    }
    
    function->blocks = NULL;
    function->block_count = 0;
    function->entry_block = NULL;
    function->exit_block = NULL;
    function->parameters = NULL;
    function->param_count = 0;
    function->local_vars = NULL;
    function->local_var_count = 0;
    function->temporaries = NULL;
    function->temp_count = 0;
    function->cfg = NULL;
    
    return function;
}

/**
 * @brief 销毁SSA函数
 */
void F_destroy_ssa_function(Stru_SsaFunction_t* function)
{
    if (function == NULL) {
        return;
    }
    
    if (function->name != NULL) {
        cn_free(function->name);
    }
    
    // 释放基本块数组
    if (function->blocks != NULL) {
        for (size_t i = 0; i < function->block_count; i++) {
            F_destroy_ssa_basic_block(function->blocks[i]);
        }
        cn_free(function->blocks);
    }
    
    // 释放参数变量数组
    if (function->parameters != NULL) {
        for (size_t i = 0; i < function->param_count; i++) {
            F_destroy_ssa_variable(function->parameters[i]);
        }
        cn_free(function->parameters);
    }
    
    // 释放局部变量数组
    if (function->local_vars != NULL) {
        for (size_t i = 0; i < function->local_var_count; i++) {
            F_destroy_ssa_variable(function->local_vars[i]);
        }
        cn_free(function->local_vars);
    }
    
    // 释放临时变量数组
    if (function->temporaries != NULL) {
        for (size_t i = 0; i < function->temp_count; i++) {
            F_destroy_ssa_variable(function->temporaries[i]);
        }
        cn_free(function->temporaries);
    }
    
    // 注意：cfg由专门的模块管理，这里不释放
    
    cn_free(function);
}

/**
 * @brief 添加基本块到SSA函数
 */
bool F_ssa_function_add_block(Stru_SsaFunction_t* function, Stru_SsaBasicBlock_t* block)
{
    if (function == NULL || block == NULL) {
        return false;
    }
    
    // 检查是否需要调整数组大小
    if (function->block_count >= function->block_count) { // 总是为真，需要修复
        size_t new_capacity = function->block_count == 0 ? 4 : function->block_count * 2;
        if (!F_ssa_function_resize_blocks(function, new_capacity)) {
            return false;
        }
    }
    
    // 添加基本块
    function->blocks[function->block_count] = block;
    function->block_count++;
    
    // 如果是第一个基本块，设置为入口块
    if (function->block_count == 1) {
        function->entry_block = block;
    }
    
    return true;
}

// ============================================================================
// SSA数据函数实现
// ============================================================================

/**
 * @brief 创建SSA数据
 */
Stru_SsaData_t* F_create_ssa_data(void)
{
    Stru_SsaData_t* data = (Stru_SsaData_t*)cn_malloc(sizeof(Stru_SsaData_t));
    if (data == NULL) {
        return NULL;
    }
    
    data->functions = NULL;
    data->function_count = 0;
    data->capacity = 0;
    data->global_vars = NULL;
    data->global_var_count = 0;
    data->errors = NULL;
    data->has_errors = false;
    data->internal_state = NULL;
    
    return data;
}

/**
 * @brief 销毁SSA数据
 */
void F_destroy_ssa_data(Stru_SsaData_t* data)
{
    if (data == NULL) {
        return;
    }
    
    // 释放函数数组
    if (data->functions != NULL) {
        for (size_t i = 0; i < data->function_count; i++) {
            F_destroy_ssa_function(data->functions[i]);
        }
        cn_free(data->functions);
    }
    
    // 释放全局变量数组
    if (data->global_vars != NULL) {
        for (size_t i = 0; i < data->global_var_count; i++) {
            F_destroy_ssa_variable(data->global_vars[i]);
        }
        cn_free(data->global_vars);
    }
    
    // 释放错误信息
    if (data->errors != NULL) {
        cn_free(data->errors);
    }
    
    // 注意：internal_state由调用者管理，这里不释放
    
    cn_free(data);
}

/**
 * @brief 添加函数到SSA数据
 */
bool F_ssa_data_add_function(Stru_SsaData_t* data, Stru_SsaFunction_t* function)
{
    if (data == NULL || function == NULL) {
        return false;
    }
    
    // 检查是否需要调整数组大小
    if (data->function_count >= data->capacity) {
        size_t new_capacity = data->capacity == 0 ? 4 : data->capacity * 2;
        if (!F_ssa_data_resize_functions(data, new_capacity)) {
            return false;
        }
    }
    
    // 添加函数
    data->functions[data->function_count] = function;
    data->function_count++;
    
    return true;
}

/**
 * @brief 从SSA数据获取函数
 */
Stru_SsaFunction_t* F_ssa_data_get_function(Stru_SsaData_t* data, size_t index)
{
    if (data == NULL || index >= data->function_count) {
        return NULL;
    }
    
    return data->functions[index];
}

/**
 * @brief 获取SSA数据函数数量
 */
size_t F_ssa_data_get_function_count(const Stru_SsaData_t* data)
{
    if (data == NULL) {
        return 0;
    }
    
    return data->function_count;
}

/**
 * @brief 添加错误到SSA数据
 */
void F_ssa_data_add_error(Stru_SsaData_t* data, const char* error)
{
    if (data == NULL || error == NULL) {
        return;
    }
    
    // 计算新错误信息的长度
    size_t error_len = strlen(error);
    size_t current_len = (data->errors != NULL) ? strlen(data->errors) : 0;
    size_t new_len = current_len + error_len + 2; // +2 用于换行符和结束符
    
    // 重新分配错误信息缓冲区
    char* new_errors = (char*)cn_realloc(data->errors, new_len);
    if (new_errors == NULL) {
        return;
    }
    
    data->errors = new_errors;
    
    // 如果是第一条错误信息，不需要添加换行符
    if (current_len == 0) {
        snprintf(data->errors, new_len, "%s", error);
    } else {
        // 追加错误信息，用换行符分隔
        snprintf(data->errors + current_len, new_len - current_len, "\n%s", error);
    }
    
    data->has_errors = true;
}

/**
 * @brief 清除SSA数据错误
 */
void F_ssa_data_clear_errors(Stru_SsaData_t* data)
{
    if (data == NULL) {
        return;
    }
    
    if (data->errors != NULL) {
        cn_free(data->errors);
        data->errors = NULL;
    }
    
    data->has_errors = false;
}

/**
 * @brief 检查SSA数据是否有错误
 */
bool F_ssa_data_has_errors(const Stru_SsaData_t* data)
{
    if (data == NULL) {
        return false;
    }
    
    return data->has_errors;
}

/**
 * @brief 获取SSA数据错误信息
 */
const char* F_ssa_data_get_errors(const Stru_SsaData_t* data)
{
    if (data == NULL) {
        return NULL;
    }
    
    return data->errors;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 调整SSA数据函数数组大小
 */
static bool F_ssa_data_resize_functions(Stru_SsaData_t* data, size_t new_capacity)
{
    if (data == NULL || new_capacity <= data->function_count) {
        return false;
    }
    
    Stru_SsaFunction_t** new_functions = (Stru_SsaFunction_t**)cn_realloc(
        data->functions, new_capacity * sizeof(Stru_SsaFunction_t*));
    
    if (new_functions == NULL) {
        return false;
    }
    
    data->functions = new_functions;
    data->capacity = new_capacity;
    
    return true;
}

/**
 * @brief 调整SSA函数基本块数组大小
 */
static bool F_ssa_function_resize_blocks(Stru_SsaFunction_t* function, size_t new_capacity)
{
    if (function == NULL || new_capacity <= function->block_count) {
        return false;
    }
    
    Stru_SsaBasicBlock_t** new_blocks = (Stru_SsaBasicBlock_t**)cn_realloc(
        function->blocks, new_capacity * sizeof(Stru_SsaBasicBlock_t*));
    
    if (new_blocks == NULL) {
        return false;
    }
    
    function->blocks = new_blocks;
    
    return true;
}

/**
 * @brief 调整SSA基本块指令数组大小
 */
static bool F_ssa_basic_block_resize_instructions(Stru_SsaBasicBlock_t* block, size_t new_capacity)
{
    if (block == NULL || new_capacity <= block->instruction_count) {
        return false;
    }
    
    Stru_SsaInstruction_t** new_instructions = (Stru_SsaInstruction_t**)cn_realloc(
        block->instructions, new_capacity * sizeof(Stru_SsaInstruction_t*));
    
    if (new_instructions == NULL) {
        return false;
    }
    
    block->instructions = new_instructions;
    
    return true;
}

/**
 * @brief 调整SSA基本块φ函数数组大小
 */
static bool F_ssa_basic_block_resize_phi_functions(Stru_SsaBasicBlock_t* block, size_t new_capacity)
{
    if (block == NULL || new_capacity <= block->phi_count) {
        return false;
    }
    
    Stru_SsaInstruction_t** new_phi_functions = (Stru_SsaInstruction_t**)cn_realloc(
        block->phi_functions, new_capacity * sizeof(Stru_SsaInstruction_t*));
    
    if (new_phi_functions == NULL) {
        return false;
    }
    
    block->phi_functions = new_phi_functions;
    
    return true;
}

// ============================================================================
// 工具函数实现
// ============================================================================

/**
 * @brief SSA操作码转字符串
 */
const char* F_ssa_opcode_to_string(Eum_SsaOpcode opcode)
{
    switch (opcode) {
        case Eum_SSA_OP_PHI: return "phi";
        case Eum_SSA_OP_COPY: return "copy";
        case Eum_SSA_OP_ADD: return "add";
        case Eum_SSA_OP_SUB: return "sub";
        case Eum_SSA_OP_MUL: return "mul";
        case Eum_SSA_OP_DIV: return "div";
        case Eum_SSA_OP_MOD: return "mod";
        case Eum_SSA_OP_NEG: return "neg";
        case Eum_SSA_OP_NOT: return "not";
        case Eum_SSA_OP_AND: return "and";
        case Eum_SSA_OP_OR: return "or";
        case Eum_SSA_OP_EQ: return "eq";
        case Eum_SSA_OP_NE: return "ne";
        case Eum_SSA_OP_LT: return "lt";
        case Eum_SSA_OP_LE: return "le";
        case Eum_SSA_OP_GT: return "gt";
        case Eum_SSA_OP_GE: return "ge";
        case Eum_SSA_OP_BIT_AND: return "bit_and";
        case Eum_SSA_OP_BIT_OR: return "bit_or";
        case Eum_SSA_OP_BIT_XOR: return "bit_xor";
        case Eum_SSA_OP_BIT_NOT: return "bit_not";
        case Eum_SSA_OP_SHL: return "shl";
        case Eum_SSA_OP_SHR: return "shr";
        case Eum_SSA_OP_LOAD: return "load";
        case Eum_SSA_OP_STORE: return "store";
        case Eum_SSA_OP_LOAD_ADDR: return "load_addr";
        case Eum_SSA_OP_CALL: return "call";
        case Eum_SSA_OP_RETURN: return "return";
        case Eum_SSA_OP_BRANCH: return "branch";
        case Eum_SSA_OP_JUMP: return "jump";
        case Eum_SSA_OP_LABEL: return "label";
        case Eum_SSA_OP_FUNCTION: return "function";
        case Eum_SSA_OP_END_FUNCTION: return "end_function";
        case Eum_SSA_OP_ALLOCA: return "alloca";
        case Eum_SSA_OP_GET_ELEMENT_PTR: return "get_element_ptr";
        default: return "unknown";
    }
}

/**
 * @brief 字符串转SSA操作码
 */
Eum_SsaOpcode F_string_to_ssa_opcode(const char* str)
{
    if (str == NULL) {
        return Eum_SSA_OP_COPY;
    }
    
    // 比较字符串并返回对应的操作码
    if (strcmp(str, "phi") == 0) return Eum_SSA_OP_PHI;
    if (strcmp(str, "copy") == 0) return Eum_SSA_OP_COPY;
    if (strcmp(str, "add") == 0) return Eum_SSA_OP_ADD;
    if (strcmp(str, "sub") == 0) return Eum_SSA_OP_SUB;
    if (strcmp(str, "mul") == 0) return Eum_SSA_OP_MUL;
    if (strcmp(str, "div") == 0) return Eum_SSA_OP_DIV;
    if (strcmp(str, "mod") == 0) return Eum_SSA_OP_MOD;
    if (strcmp(str, "neg") == 0) return Eum_SSA_OP_NEG;
    if (strcmp(str, "not") == 0) return Eum_SSA_OP_NOT;
    if (strcmp(str, "and") == 0) return Eum_SSA_OP_AND;
    if (strcmp(str, "or") == 0) return Eum_SSA_OP_OR;
    if (strcmp(str, "eq") == 0) return Eum_SSA_OP_EQ;
    if (strcmp(str, "ne") == 0) return Eum_SSA_OP_NE;
    if (strcmp(str, "lt") == 0) return Eum_SSA_OP_LT;
    if (strcmp(str, "le") == 0) return Eum_SSA_OP_LE;
    if (strcmp(str, "gt") == 0) return Eum_SSA_OP_GT;
    if (strcmp(str, "ge") == 0) return Eum_SSA_OP_GE;
    if (strcmp(str, "bit_and") == 0) return Eum_SSA_OP_BIT_AND;
    if (strcmp(str, "bit_or") == 0) return Eum_SSA_OP_BIT_OR;
    if (strcmp(str, "bit_xor") == 0) return Eum_SSA_OP_BIT_XOR;
    if (strcmp(str, "bit_not") == 0) return Eum_SSA_OP_BIT_NOT;
    if (strcmp(str, "shl") == 0) return Eum_SSA_OP_SHL;
    if (strcmp(str, "shr") == 0) return Eum_SSA_OP_SHR;
    if (strcmp(str, "load") == 0) return Eum_SSA_OP_LOAD;
    if (strcmp(str, "store") == 0) return Eum_SSA_OP_STORE;
    if (strcmp(str, "load_addr") == 0) return Eum_SSA_OP_LOAD_ADDR;
    if (strcmp(str, "call") == 0) return Eum_SSA_OP_CALL;
    if (strcmp(str, "return") == 0) return Eum_SSA_OP_RETURN;
    if (strcmp(str, "branch") == 0) return Eum_SSA_OP_BRANCH;
    if (strcmp(str, "jump") == 0) return Eum_SSA_OP_JUMP;
    if (strcmp(str, "label") == 0) return Eum_SSA_OP_LABEL;
    if (strcmp(str, "function") == 0) return Eum_SSA_OP_FUNCTION;
    if (strcmp(str, "end_function") == 0) return Eum_SSA_OP_END_FUNCTION;
    if (strcmp(str, "alloca") == 0) return Eum_SSA_OP_ALLOCA;
    if (strcmp(str, "get_element_ptr") == 0) return Eum_SSA_OP_GET_ELEMENT_PTR;
    
    return Eum_SSA_OP_COPY;
}

/**
 * @brief 格式化SSA指令
 */
char* F_format_ssa_instruction(const Stru_SsaInstruction_t* instruction)
{
    if (instruction == NULL) {
        return cn_strdup("(null instruction)");
    }
    
    const char* opcode_str = F_ssa_opcode_to_string(instruction->opcode);
    
    // 根据指令类型格式化
    switch (instruction->opcode) {
        case Eum_SSA_OP_PHI: {
            // φ函数格式: result = φ(arg1@block1, arg2@block2, ...)
            size_t buffer_size = 256; // 初始大小
            char* buffer = (char*)cn_malloc(buffer_size);
            if (buffer == NULL) {
                return NULL;
            }
            
            if (instruction->result != NULL) {
                snprintf(buffer, buffer_size, "%s = φ(", 
                        instruction->result->full_name);
            } else {
                snprintf(buffer, buffer_size, "? = φ(");
            }
            
            // 添加φ参数
            for (size_t i = 0; i < instruction->phi_arg_count; i++) {
                Stru_PhiArgument_t* arg = instruction->phi_args[i];
                if (arg != NULL && arg->variable != NULL) {
                    size_t current_len = strlen(buffer);
                    size_t needed = current_len + strlen(arg->variable->full_name) + 
                                   strlen(arg->block_name) + 10; // 额外空间
                    
                    if (needed >= buffer_size) {
                        buffer_size = needed + 256;
                        char* new_buffer = (char*)cn_realloc(buffer, buffer_size);
                        if (new_buffer == NULL) {
                            cn_free(buffer);
                            return NULL;
                        }
                        buffer = new_buffer;
                    }
                    
                    if (i > 0) {
                        strcat(buffer, ", ");
                    }
                    strcat(buffer, arg->variable->full_name);
                    strcat(buffer, "@");
                    strcat(buffer, arg->block_name);
                }
            }
            
            strcat(buffer, ")");
            return buffer;
        }
        
        case Eum_SSA_OP_BRANCH: {
            // 分支指令格式: br cond, label1, label2
            if (instruction->operand_count >= 1 && instruction->label_count >= 2) {
                size_t buffer_size = 256;
                char* buffer = (char*)cn_malloc(buffer_size);
                if (buffer == NULL) {
                    return NULL;
                }
                
                snprintf(buffer, buffer_size, "br %s, %s, %s",
                        instruction->operands[0]->full_name,
                        instruction->labels[0],
                        instruction->labels[1]);
                return buffer;
            }
            break;
        }
        
        case Eum_SSA_OP_JUMP: {
            // 跳转指令格式: jump label
            if (instruction->label_count >= 1) {
                size_t buffer_size = 256;
                char* buffer = (char*)cn_malloc(buffer_size);
                if (buffer == NULL) {
                    return NULL;
                }
                
                snprintf(buffer, buffer_size, "jump %s", instruction->labels[0]);
                return buffer;
            }
            break;
        }
        
        case Eum_SSA_OP_LABEL: {
            // 标签指令格式: label:
            if (instruction->label_count >= 1) {
                size_t buffer_size = 256;
                char* buffer = (char*)cn_malloc(buffer_size);
                if (buffer == NULL) {
                    return NULL;
                }
                
                snprintf(buffer, buffer_size, "%s:", instruction->labels[0]);
                return buffer;
            }
            break;
        }
        
        case Eum_SSA_OP_RETURN: {
            // 返回指令格式: return value
            size_t buffer_size = 256;
            char* buffer = (char*)cn_malloc(buffer_size);
            if (buffer == NULL) {
                return NULL;
            }
            
            if (instruction->operand_count >= 1) {
                snprintf(buffer, buffer_size, "return %s", 
                        instruction->operands[0]->full_name);
            } else {
                snprintf(buffer, buffer_size, "return");
            }
            return buffer;
        }
        
        case Eum_SSA_OP_CALL: {
            // 调用指令格式: result = call function(args...)
            size_t buffer_size = 256;
            char* buffer = (char*)cn_malloc(buffer_size);
            if (buffer == NULL) {
                return NULL;
            }
            
            if (instruction->result != NULL) {
                snprintf(buffer, buffer_size, "%s = call ", 
                        instruction->result->full_name);
            } else {
                snprintf(buffer, buffer_size, "call ");
            }
            
            // 添加函数名（如果有标签）
            if (instruction->label_count >= 1) {
                size_t current_len = strlen(buffer);
                size_t needed = current_len + strlen(instruction->labels[0]) + 10;
                
                if (needed >= buffer_size) {
                    buffer_size = needed + 256;
                    char* new_buffer = (char*)cn_realloc(buffer, buffer_size);
                    if (new_buffer == NULL) {
                        cn_free(buffer);
                        return NULL;
                    }
                    buffer = new_buffer;
                }
                
                strcat(buffer, instruction->labels[0]);
            } else {
                strcat(buffer, "?");
            }
            
            // 添加参数
            strcat(buffer, "(");
            for (size_t i = 0; i < instruction->operand_count; i++) {
                if (i > 0) {
                    strcat(buffer, ", ");
                }
                strcat(buffer, instruction->operands[i]->full_name);
            }
            strcat(buffer, ")");
            
            return buffer;
        }
        
        default: {
            // 通用指令格式: result = opcode operand1, operand2, ...
            size_t buffer_size = 256;
            char* buffer = (char*)cn_malloc(buffer_size);
            if (buffer == NULL) {
                return NULL;
            }
            
            if (instruction->result != NULL) {
                snprintf(buffer, buffer_size, "%s = %s ", 
                        instruction->result->full_name, opcode_str);
            } else {
                snprintf(buffer, buffer_size, "%s ", opcode_str);
            }
            
            // 添加操作数
            for (size_t i = 0; i < instruction->operand_count; i++) {
                if (i > 0) {
                    strcat(buffer, ", ");
                }
                
                size_t current_len = strlen(buffer);
                size_t needed = current_len + strlen(instruction->operands[i]->full_name) + 10;
                
                if (needed >= buffer_size) {
                    buffer_size = needed + 256;
                    char* new_buffer = (char*)cn_realloc(buffer, buffer_size);
                    if (new_buffer == NULL) {
                        cn_free(buffer);
                        return NULL;
                    }
                    buffer = new_buffer;
                }
                
                strcat(buffer, instruction->operands[i]->full_name);
            }
            
            return buffer;
        }
    }
    
    // 默认返回操作码字符串
    return cn_strdup(opcode_str);
}

/**
 * @brief 复制字符串
 * 
 * 复制C风格字符串，使用cn_malloc分配内存。
 * 
 * @param str 要复制的字符串
 * @return 新分配的字符串副本，失败返回NULL
 */
static char* cn_strdup(const char* str)
{
    if (str == NULL) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* copy = (char*)cn_malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    
    memcpy(copy, str, len);
    return copy;
}
