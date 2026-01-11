/******************************************************************************
 * 文件名: CN_tac_impl.c
 * 功能: CN_Language 三地址码（TAC）实现
 * 
 * 实现三地址码（Three-Address Code）的核心功能。
 * 包括TAC指令的创建、管理、格式化和转换。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_tac_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief TAC内部状态结构体
 * 
 * 存储TAC实现的内部状态。
 */
typedef struct {
    Stru_TacData_t* tac_data;        ///< TAC数据
    char* name;                      ///< 实现名称
    char* version;                   ///< 版本信息
    Eum_IrType ir_type;              ///< IR类型
    void* user_data;                 ///< 用户数据
} Stru_TacInternalState_t;

// ============================================================================
// 静态函数声明
// ============================================================================

static const char* tac_get_name(void);
static const char* tac_get_version(void);
static Eum_IrType tac_get_type(void);
static void* tac_create_from_ast(Stru_AstNode_t* ast);
static void* tac_create_empty(void);
static bool tac_add_instruction(void* ir, Stru_IrInstruction_t* instr);
static bool tac_remove_instruction(void* ir, size_t index);
static Stru_IrInstruction_t* tac_get_instruction(void* ir, size_t index);
static size_t tac_get_instruction_count(void* ir);
static bool tac_has_errors(void* ir);
static const char* tac_get_errors(void* ir);
static void tac_clear_errors(void* ir);
static void* tac_convert_to_lower(void* ir);
static void* tac_convert_to_higher(void* ir);
static void* tac_convert_to_type(void* ir, Eum_IrType target_type);
static bool tac_apply_optimization(void* ir, int optimization_type);
static bool tac_apply_all_optimizations(void* ir, int optimization_level);
static const char* tac_serialize(void* ir);
static void* tac_deserialize(const char* data);
static bool tac_validate(void* ir);
static void tac_destroy(void* ir);

static Stru_IrInstruction_t* convert_tac_to_ir_instruction(Stru_TacInstruction_t* tac_instr);
static Stru_TacInstruction_t* convert_ir_to_tac_instruction(Stru_IrInstruction_t* ir_instr);

// ============================================================================
// 工具函数实现
// ============================================================================

/**
 * @brief 创建TAC指令
 */
Stru_TacInstruction_t* F_create_tac_instruction(Eum_TacOpcode opcode,
                                                const char* result,
                                                const char* operand1,
                                                const char* operand2,
                                                const char* label,
                                                size_t line, size_t column)
{
    Stru_TacInstruction_t* instr = (Stru_TacInstruction_t*)malloc(sizeof(Stru_TacInstruction_t));
    if (!instr) {
        return NULL;
    }
    
    instr->opcode = opcode;
    instr->line = line;
    instr->column = column;
    instr->extra_data = NULL;
    
    // 复制字符串
    instr->result = result ? strdup(result) : NULL;
    instr->operand1 = operand1 ? strdup(operand1) : NULL;
    instr->operand2 = operand2 ? strdup(operand2) : NULL;
    instr->label = label ? strdup(label) : NULL;
    
    return instr;
}

/**
 * @brief 销毁TAC指令
 */
void F_destroy_tac_instruction(Stru_TacInstruction_t* instruction)
{
    if (!instruction) {
        return;
    }
    
    free(instruction->result);
    free(instruction->operand1);
    free(instruction->operand2);
    free(instruction->label);
    
    if (instruction->extra_data) {
        free(instruction->extra_data);
    }
    
    free(instruction);
}

/**
 * @brief TAC操作码转字符串
 */
const char* F_tac_opcode_to_string(Eum_TacOpcode opcode)
{
    switch (opcode) {
        case Eum_TAC_OP_ASSIGN: return "ASSIGN";
        case Eum_TAC_OP_ADD: return "ADD";
        case Eum_TAC_OP_SUB: return "SUB";
        case Eum_TAC_OP_MUL: return "MUL";
        case Eum_TAC_OP_DIV: return "DIV";
        case Eum_TAC_OP_MOD: return "MOD";
        case Eum_TAC_OP_NEG: return "NEG";
        case Eum_TAC_OP_NOT: return "NOT";
        case Eum_TAC_OP_AND: return "AND";
        case Eum_TAC_OP_OR: return "OR";
        case Eum_TAC_OP_EQ: return "EQ";
        case Eum_TAC_OP_NE: return "NE";
        case Eum_TAC_OP_LT: return "LT";
        case Eum_TAC_OP_LE: return "LE";
        case Eum_TAC_OP_GT: return "GT";
        case Eum_TAC_OP_GE: return "GE";
        case Eum_TAC_OP_BIT_AND: return "BIT_AND";
        case Eum_TAC_OP_BIT_OR: return "BIT_OR";
        case Eum_TAC_OP_BIT_XOR: return "BIT_XOR";
        case Eum_TAC_OP_BIT_NOT: return "BIT_NOT";
        case Eum_TAC_OP_SHL: return "SHL";
        case Eum_TAC_OP_SHR: return "SHR";
        case Eum_TAC_OP_LOAD: return "LOAD";
        case Eum_TAC_OP_STORE: return "STORE";
        case Eum_TAC_OP_LOAD_ADDR: return "LOAD_ADDR";
        case Eum_TAC_OP_CALL: return "CALL";
        case Eum_TAC_OP_RETURN: return "RETURN";
        case Eum_TAC_OP_GOTO: return "GOTO";
        case Eum_TAC_OP_IF: return "IF";
        case Eum_TAC_OP_IF_FALSE: return "IF_FALSE";
        case Eum_TAC_OP_PARAM: return "PARAM";
        case Eum_TAC_OP_LABEL: return "LABEL";
        case Eum_TAC_OP_FUNCTION: return "FUNCTION";
        case Eum_TAC_OP_END_FUNCTION: return "END_FUNCTION";
        case Eum_TAC_OP_ALLOCA: return "ALLOCA";
        case Eum_TAC_OP_GET_ELEMENT_PTR: return "GET_ELEMENT_PTR";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 字符串转TAC操作码
 */
Eum_TacOpcode F_string_to_tac_opcode(const char* str)
{
    if (!str) {
        return Eum_TAC_OP_ASSIGN;
    }
    
    // 比较字符串并返回对应的操作码（支持大写）
    if (strcmp(str, "ASSIGN") == 0 || strcmp(str, "assign") == 0) return Eum_TAC_OP_ASSIGN;
    if (strcmp(str, "ADD") == 0 || strcmp(str, "add") == 0) return Eum_TAC_OP_ADD;
    if (strcmp(str, "SUB") == 0 || strcmp(str, "sub") == 0) return Eum_TAC_OP_SUB;
    if (strcmp(str, "MUL") == 0 || strcmp(str, "mul") == 0) return Eum_TAC_OP_MUL;
    if (strcmp(str, "DIV") == 0 || strcmp(str, "div") == 0) return Eum_TAC_OP_DIV;
    if (strcmp(str, "MOD") == 0 || strcmp(str, "mod") == 0) return Eum_TAC_OP_MOD;
    if (strcmp(str, "NEG") == 0 || strcmp(str, "neg") == 0) return Eum_TAC_OP_NEG;
    if (strcmp(str, "NOT") == 0 || strcmp(str, "not") == 0) return Eum_TAC_OP_NOT;
    if (strcmp(str, "AND") == 0 || strcmp(str, "and") == 0) return Eum_TAC_OP_AND;
    if (strcmp(str, "OR") == 0 || strcmp(str, "or") == 0) return Eum_TAC_OP_OR;
    if (strcmp(str, "EQ") == 0 || strcmp(str, "eq") == 0) return Eum_TAC_OP_EQ;
    if (strcmp(str, "NE") == 0 || strcmp(str, "ne") == 0) return Eum_TAC_OP_NE;
    if (strcmp(str, "LT") == 0 || strcmp(str, "lt") == 0) return Eum_TAC_OP_LT;
    if (strcmp(str, "LE") == 0 || strcmp(str, "le") == 0) return Eum_TAC_OP_LE;
    if (strcmp(str, "GT") == 0 || strcmp(str, "gt") == 0) return Eum_TAC_OP_GT;
    if (strcmp(str, "GE") == 0 || strcmp(str, "ge") == 0) return Eum_TAC_OP_GE;
    if (strcmp(str, "BIT_AND") == 0 || strcmp(str, "bit_and") == 0) return Eum_TAC_OP_BIT_AND;
    if (strcmp(str, "BIT_OR") == 0 || strcmp(str, "bit_or") == 0) return Eum_TAC_OP_BIT_OR;
    if (strcmp(str, "BIT_XOR") == 0 || strcmp(str, "bit_xor") == 0) return Eum_TAC_OP_BIT_XOR;
    if (strcmp(str, "BIT_NOT") == 0 || strcmp(str, "bit_not") == 0) return Eum_TAC_OP_BIT_NOT;
    if (strcmp(str, "SHL") == 0 || strcmp(str, "shl") == 0) return Eum_TAC_OP_SHL;
    if (strcmp(str, "SHR") == 0 || strcmp(str, "shr") == 0) return Eum_TAC_OP_SHR;
    if (strcmp(str, "LOAD") == 0 || strcmp(str, "load") == 0) return Eum_TAC_OP_LOAD;
    if (strcmp(str, "STORE") == 0 || strcmp(str, "store") == 0) return Eum_TAC_OP_STORE;
    if (strcmp(str, "LOAD_ADDR") == 0 || strcmp(str, "load_addr") == 0) return Eum_TAC_OP_LOAD_ADDR;
    if (strcmp(str, "CALL") == 0 || strcmp(str, "call") == 0) return Eum_TAC_OP_CALL;
    if (strcmp(str, "RETURN") == 0 || strcmp(str, "return") == 0) return Eum_TAC_OP_RETURN;
    if (strcmp(str, "GOTO") == 0 || strcmp(str, "goto") == 0) return Eum_TAC_OP_GOTO;
    if (strcmp(str, "IF") == 0 || strcmp(str, "if") == 0) return Eum_TAC_OP_IF;
    if (strcmp(str, "IF_FALSE") == 0 || strcmp(str, "if_false") == 0) return Eum_TAC_OP_IF_FALSE;
    if (strcmp(str, "PARAM") == 0 || strcmp(str, "param") == 0) return Eum_TAC_OP_PARAM;
    if (strcmp(str, "LABEL") == 0 || strcmp(str, "label") == 0) return Eum_TAC_OP_LABEL;
    if (strcmp(str, "FUNCTION") == 0 || strcmp(str, "function") == 0) return Eum_TAC_OP_FUNCTION;
    if (strcmp(str, "END_FUNCTION") == 0 || strcmp(str, "end_function") == 0) return Eum_TAC_OP_END_FUNCTION;
    if (strcmp(str, "ALLOCA") == 0 || strcmp(str, "alloca") == 0) return Eum_TAC_OP_ALLOCA;
    if (strcmp(str, "GET_ELEMENT_PTR") == 0 || strcmp(str, "get_element_ptr") == 0) return Eum_TAC_OP_GET_ELEMENT_PTR;
    
    return Eum_TAC_OP_ASSIGN;
}

/**
 * @brief 格式化TAC指令
 */
char* F_format_tac_instruction(const Stru_TacInstruction_t* instruction)
{
    if (!instruction) {
        return strdup("");
    }
    
    const char* opcode_str = F_tac_opcode_to_string(instruction->opcode);
    char buffer[512];
    
    switch (instruction->opcode) {
        case Eum_TAC_OP_ASSIGN:
        case Eum_TAC_OP_ADD:
        case Eum_TAC_OP_SUB:
        case Eum_TAC_OP_MUL:
        case Eum_TAC_OP_DIV:
        case Eum_TAC_OP_MOD:
        case Eum_TAC_OP_AND:
        case Eum_TAC_OP_OR:
        case Eum_TAC_OP_EQ:
        case Eum_TAC_OP_NE:
        case Eum_TAC_OP_LT:
        case Eum_TAC_OP_LE:
        case Eum_TAC_OP_GT:
        case Eum_TAC_OP_GE:
        case Eum_TAC_OP_BIT_AND:
        case Eum_TAC_OP_BIT_OR:
        case Eum_TAC_OP_BIT_XOR:
        case Eum_TAC_OP_SHL:
        case Eum_TAC_OP_SHR:
            if (instruction->result && instruction->operand1 && instruction->operand2) {
                snprintf(buffer, sizeof(buffer), "%s = %s %s %s", 
                        instruction->result, instruction->operand1, 
                        opcode_str, instruction->operand2);
            } else if (instruction->result && instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "%s = %s", 
                        instruction->result, instruction->operand1);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_NEG:
        case Eum_TAC_OP_NOT:
        case Eum_TAC_OP_BIT_NOT:
            if (instruction->result && instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "%s = %s %s", 
                        instruction->result, opcode_str, instruction->operand1);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_LOAD:
            if (instruction->result && instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "%s = *%s", 
                        instruction->result, instruction->operand1);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_STORE:
            if (instruction->operand1 && instruction->operand2) {
                snprintf(buffer, sizeof(buffer), "*%s = %s", 
                        instruction->operand1, instruction->operand2);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_LOAD_ADDR:
            if (instruction->result && instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "%s = &%s", 
                        instruction->result, instruction->operand1);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_CALL:
            if (instruction->result && instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "%s = call %s", 
                        instruction->result, instruction->operand1);
            } else if (instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "call %s", instruction->operand1);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_RETURN:
            if (instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "return %s", instruction->operand1);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_GOTO:
            if (instruction->label) {
                snprintf(buffer, sizeof(buffer), "%s %s", opcode_str, instruction->label);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_IF:
        case Eum_TAC_OP_IF_FALSE:
            if (instruction->operand1 && instruction->label) {
                snprintf(buffer, sizeof(buffer), "%s %s goto %s", 
                        opcode_str, instruction->operand1, instruction->label);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_PARAM:
            if (instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "param %s", instruction->operand1);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_LABEL:
            if (instruction->label) {
                snprintf(buffer, sizeof(buffer), "%s:", instruction->label);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_FUNCTION:
            if (instruction->label) {
                snprintf(buffer, sizeof(buffer), "function %s:", instruction->label);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_END_FUNCTION:
            snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            break;
            
        case Eum_TAC_OP_ALLOCA:
            if (instruction->result && instruction->operand1) {
                snprintf(buffer, sizeof(buffer), "%s = alloca %s", 
                        instruction->result, instruction->operand1);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        case Eum_TAC_OP_GET_ELEMENT_PTR:
            if (instruction->result && instruction->operand1 && instruction->operand2) {
                snprintf(buffer, sizeof(buffer), "%s = &%s[%s]", 
                        instruction->result, instruction->operand1, instruction->operand2);
            } else {
                snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            }
            break;
            
        default:
            snprintf(buffer, sizeof(buffer), "%s", opcode_str);
            break;
    }
    
    return strdup(buffer);
}

/**
 * @brief 创建TAC数据
 */
Stru_TacData_t* F_create_tac_data(void)
{
    Stru_TacData_t* data = (Stru_TacData_t*)malloc(sizeof(Stru_TacData_t));
    if (!data) {
        return NULL;
    }
    
    data->instructions = NULL;
    data->instruction_count = 0;
    data->capacity = 0;
    data->temporaries = NULL;
    data->temp_count = 0;
    data->labels = NULL;
    data->label_count = 0;
    data->errors = NULL;
    data->has_errors = false;
    data->internal_state = NULL;
    
    return data;
}

/**
 * @brief 销毁TAC数据
 */
void F_destroy_tac_data(Stru_TacData_t* data)
{
    if (!data) {
        return;
    }
    
    // 销毁所有指令
    for (size_t i = 0; i < data->instruction_count; i++) {
        F_destroy_tac_instruction(data->instructions[i]);
    }
    free(data->instructions);
    
    // 销毁临时变量
    for (size_t i = 0; i < data->temp_count; i++) {
        free(data->temporaries[i]);
    }
    free(data->temporaries);
    
    // 销毁标签
    for (size_t i = 0; i < data->label_count; i++) {
        free(data->labels[i]);
    }
    free(data->labels);
    
    // 销毁错误信息
    free(data->errors);
    
    free(data);
}

/**
 * @brief 添加TAC指令到数据
 */
bool F_tac_data_add_instruction(Stru_TacData_t* data, Stru_TacInstruction_t* instruction)
{
    if (!data || !instruction) {
        return false;
    }
    
    // 检查是否需要扩容
    if (data->instruction_count >= data->capacity) {
        size_t new_capacity = data->capacity == 0 ? 8 : data->capacity * 2;
        Stru_TacInstruction_t** new_instructions = (Stru_TacInstruction_t**)realloc(
            data->instructions, new_capacity * sizeof(Stru_TacInstruction_t*));
        
        if (!new_instructions) {
            return false;
        }
        
        data->instructions = new_instructions;
        data->capacity = new_capacity;
    }
    
    data->instructions[data->instruction_count] = instruction;
    data->instruction_count++;
    
    return true;
}

/**
 * @brief 从TAC数据获取指令
 */
Stru_TacInstruction_t* F_tac_data_get_instruction(Stru_TacData_t* data, size_t index)
{
    if (!data || index >= data->instruction_count) {
        return NULL;
    }
    
    return data->instructions[index];
}

/**
 * @brief 获取TAC数据指令数量
 */
size_t F_tac_data_get_instruction_count(const Stru_TacData_t* data)
{
    return data ? data->instruction_count : 0;
}

/**
 * @brief 添加错误到TAC数据
 */
void F_tac_data_add_error(Stru_TacData_t* data, const char* error)
{
    if (!data || !error) {
        return;
    }
    
    data->has_errors = true;
    
    if (!data->errors) {
        data->errors = strdup(error);
    } else {
        size_t old_len = strlen(data->errors);
        size_t error_len = strlen(error);
        char* new_errors = (char*)realloc(data->errors, old_len + error_len + 3); // +2 for "\n" and +1 for null terminator
        
        if (new_errors) {
            data->errors = new_errors;
            strcat(data->errors, "\n");
            strcat(data->errors, error);
        }
    }
}

/**
 * @brief 清除TAC数据错误
 */
void F_tac_data_clear_errors(Stru_TacData_t* data)
{
    if (!data) {
        return;
    }
    
    free(data->errors);
    data->errors = NULL;
    data->has_errors = false;
}

/**
 * @brief 检查TAC数据是否有错误
 */
bool F_tac_data_has_errors(const Stru_TacData_t* data)
{
    return data ? data->has_errors : false;
}

/**
 * @brief 获取TAC数据错误信息
 */
const char* F_tac_data_get_errors(const Stru_TacData_t* data)
{
    return data ? data->errors : NULL;
}

/**
 * @brief 复制TAC指令
 */
Stru_TacInstruction_t* F_copy_tac_instruction(const Stru_TacInstruction_t* instruction)
{
    if (!instruction) {
        return NULL;
    }
    
    // 创建新的指令
    Stru_TacInstruction_t* new_instr = (Stru_TacInstruction_t*)malloc(sizeof(Stru_TacInstruction_t));
    if (!new_instr) {
        return NULL;
    }
    
    // 复制基本字段
    new_instr->opcode = instruction->opcode;
    new_instr->line = instruction->line;
    new_instr->column = instruction->column;
    
    // 复制字符串字段（深拷贝）
    new_instr->result = instruction->result ? strdup(instruction->result) : NULL;
    new_instr->operand1 = instruction->operand1 ? strdup(instruction->operand1) : NULL;
    new_instr->operand2 = instruction->operand2 ? strdup(instruction->operand2) : NULL;
    new_instr->label = instruction->label ? strdup(instruction->label) : NULL;
    
    // 复制额外数据（如果有）
    new_instr->extra_data = NULL;
    if (instruction->extra_data) {
        // 注意：这里假设extra_data是一个字符串，实际实现可能需要根据具体类型处理
        // 为了简单起见，这里只复制字符串
        new_instr->extra_data = strdup((const char*)instruction->extra_data);
    }
    
    return new_instr;
}

// ============================================================================
// 接口函数实现
// ============================================================================

/**
 * @brief 获取TAC实现名称
 */
static const char* tac_get_name(void)
{
    return "Three-Address Code (TAC)";
}

/**
 * @brief 获取TAC实现版本
 */
static const char* tac_get_version(void)
{
    return "1.0.0";
}

/**
 * @brief 获取TAC实现类型
 */
static Eum_IrType tac_get_type(void)
{
    return Eum_IR_TYPE_TAC;
}

/**
 * @brief 从AST创建TAC
 */
static void* tac_create_from_ast(Stru_AstNode_t* ast)
{
    // 创建TAC数据
    Stru_TacData_t* tac_data = F_create_tac_data();
    if (!tac_data) {
        return NULL;
    }
    
    // 创建内部状态
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)malloc(sizeof(Stru_TacInternalState_t));
    if (!state) {
        F_destroy_tac_data(tac_data);
        return NULL;
    }
    
    state->tac_data = tac_data;
    state->name = strdup(tac_get_name());
    state->version = strdup(tac_get_version());
    state->ir_type = tac_get_type();
    state->user_data = NULL;
    
    // TODO: 实现AST到TAC的实际转换
    // 这里添加一个示例指令
    Stru_TacInstruction_t* sample_instr = F_create_tac_instruction(
        Eum_TAC_OP_ASSIGN, "t1", "42", NULL, NULL, 1, 1);
    
    if (sample_instr) {
        F_tac_data_add_instruction(tac_data, sample_instr);
    }
    
    return state;
}

/**
 * @brief 创建空的TAC
 */
static void* tac_create_empty(void)
{
    Stru_TacData_t* tac_data = F_create_tac_data();
    if (!tac_data) {
        return NULL;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)malloc(sizeof(Stru_TacInternalState_t));
    if (!state) {
        F_destroy_tac_data(tac_data);
        return NULL;
    }
    
    state->tac_data = tac_data;
    state->name = strdup(tac_get_name());
    state->version = strdup(tac_get_version());
    state->ir_type = tac_get_type();
    state->user_data = NULL;
    
    return state;
}

/**
 * @brief 添加指令到TAC
 */
static bool tac_add_instruction(void* ir, Stru_IrInstruction_t* instr)
{
    if (!ir || !instr) {
        return false;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    
    // 将IR指令转换为TAC指令
    Stru_TacInstruction_t* tac_instr = convert_ir_to_tac_instruction(instr);
    if (!tac_instr) {
        return false;
    }
    
    return F_tac_data_add_instruction(state->tac_data, tac_instr);
}

/**
 * @brief 从TAC移除指令
 */
static bool tac_remove_instruction(void* ir, size_t index)
{
    if (!ir) {
        return false;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    Stru_TacData_t* data = state->tac_data;
    
    if (index >= data->instruction_count) {
        return false;
    }
    
    // 销毁要移除的指令
    F_destroy_tac_instruction(data->instructions[index]);
    
    // 移动后续指令
    for (size_t i = index; i < data->instruction_count - 1; i++) {
        data->instructions[i] = data->instructions[i + 1];
    }
    
    data->instruction_count--;
    
    return true;
}

/**
 * @brief 从TAC获取指令
 */
static Stru_IrInstruction_t* tac_get_instruction(void* ir, size_t index)
{
    if (!ir) {
        return NULL;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    Stru_TacData_t* data = state->tac_data;
    
    if (index >= data->instruction_count) {
        return NULL;
    }
    
    Stru_TacInstruction_t* tac_instr = data->instructions[index];
    return convert_tac_to_ir_instruction(tac_instr);
}

/**
 * @brief 获取TAC指令数量
 */
static size_t tac_get_instruction_count(void* ir)
{
    if (!ir) {
        return 0;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    return F_tac_data_get_instruction_count(state->tac_data);
}

/**
 * @brief 检查TAC是否有错误
 */
static bool tac_has_errors(void* ir)
{
    if (!ir) {
        return false;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    return F_tac_data_has_errors(state->tac_data);
}

/**
 * @brief 获取TAC错误信息
 */
static const char* tac_get_errors(void* ir)
{
    if (!ir) {
        return NULL;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    return F_tac_data_get_errors(state->tac_data);
}

/**
 * @brief 清除TAC错误
 */
static void tac_clear_errors(void* ir)
{
    if (!ir) {
        return;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    F_tac_data_clear_errors(state->tac_data);
}

/**
 * @brief 将TAC转换为低级IR
 */
static void* tac_convert_to_lower(void* ir)
{
    // TAC已经是一种低级IR，所以返回自身
    return ir;
}

/**
 * @brief 将TAC转换为高级IR
 */
static void* tac_convert_to_higher(void* ir)
{
    // 暂时不支持转换为高级IR
    F_tac_data_add_error(((Stru_TacInternalState_t*)ir)->tac_data, 
                        "不支持从TAC转换为高级IR");
    return ir;
}

/**
 * @brief 将TAC转换为指定类型
 */
static void* tac_convert_to_type(void* ir, Eum_IrType target_type)
{
    if (!ir) {
        return NULL;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    
    if (target_type == Eum_IR_TYPE_TAC) {
        return ir; // 已经是TAC
    }
    
    F_tac_data_add_error(state->tac_data, 
                        "不支持从TAC转换为其他IR类型");
    return ir;
}

/**
 * @brief 应用优化到TAC
 */
static bool tac_apply_optimization(void* ir, int optimization_type)
{
    if (!ir) {
        return false;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    
    // TODO: 实现具体的优化算法
    switch (optimization_type) {
        case 1: // 常量折叠
            F_tac_data_add_error(state->tac_data, "常量折叠优化尚未实现");
            break;
        case 2: // 死代码消除
            F_tac_data_add_error(state->tac_data, "死代码消除优化尚未实现");
            break;
        default:
            F_tac_data_add_error(state->tac_data, "未知的优化类型");
            break;
    }
    
    return false;
}

/**
 * @brief 应用所有优化到TAC
 */
static bool tac_apply_all_optimizations(void* ir, int optimization_level)
{
    if (!ir) {
        return false;
    }
    
    // TODO: 根据优化级别应用不同的优化
    bool success = true;
    
    if (optimization_level >= 1) {
        success = success && tac_apply_optimization(ir, 1); // 常量折叠
    }
    
    if (optimization_level >= 2) {
        success = success && tac_apply_optimization(ir, 2); // 死代码消除
    }
    
    return success;
}

/**
 * @brief 序列化TAC
 */
static const char* tac_serialize(void* ir)
{
    if (!ir) {
        return NULL;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    Stru_TacData_t* data = state->tac_data;
    
    // 计算所需缓冲区大小
    size_t total_size = 1; // null terminator
    for (size_t i = 0; i < data->instruction_count; i++) {
        char* formatted = F_format_tac_instruction(data->instructions[i]);
        if (formatted) {
            total_size += strlen(formatted) + 1; // +1 for newline
            free(formatted);
        }
    }
    
    // 分配缓冲区
    char* buffer = (char*)malloc(total_size);
    if (!buffer) {
        return NULL;
    }
    
    buffer[0] = '\0';
    
    // 构建序列化字符串
    for (size_t i = 0; i < data->instruction_count; i++) {
        char* formatted = F_format_tac_instruction(data->instructions[i]);
        if (formatted) {
            strcat(buffer, formatted);
            strcat(buffer, "\n");
            free(formatted);
        }
    }
    
    return buffer;
}

/**
 * @brief 反序列化TAC
 */
static void* tac_deserialize(const char* data)
{
    if (!data) {
        return NULL;
    }
    
    // 创建空的TAC
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)tac_create_empty();
    if (!state) {
        return NULL;
    }
    
    Stru_TacData_t* tac_data = state->tac_data;
    
    // 复制数据以便处理
    char* data_copy = strdup(data);
    if (!data_copy) {
        tac_destroy(state);
        return NULL;
    }
    
    char* line = strtok(data_copy, "\n");
    size_t line_number = 1;
    
    while (line != NULL) {
        // 跳过空行
        if (strlen(line) == 0) {
            line = strtok(NULL, "\n");
            line_number++;
            continue;
        }
        
        // 解析TAC指令
        // 这里实现一个简单的解析器，实际实现应更健壮
        
        // 检查是否是标签
        if (strstr(line, ":") != NULL && strlen(line) > 1) {
            // 标签指令
            char* label = strdup(line);
            // 移除冒号
            char* colon = strchr(label, ':');
            if (colon) {
                *colon = '\0';
            }
            
            Stru_TacInstruction_t* instr = F_create_tac_instruction(
                Eum_TAC_OP_LABEL,
                NULL, // result
                NULL, // operand1
                NULL, // operand2
                label,
                line_number, 1
            );
            
            free(label);
            
            if (instr) {
                F_tac_data_add_instruction(tac_data, instr);
            }
        }
        // 检查是否是赋值指令
        else if (strstr(line, "=") != NULL) {
            // 简单的赋值指令解析
            char* equal_sign = strchr(line, '=');
            if (equal_sign) {
                *equal_sign = '\0';
                char* result = strdup(line);
                char* rest = equal_sign + 1;
                
                // 跳过空格
                while (*rest == ' ') rest++;
                
                // 创建指令
                Stru_TacInstruction_t* instr = F_create_tac_instruction(
                    Eum_TAC_OP_ASSIGN,
                    result,
                    rest,
                    NULL,
                    NULL,
                    line_number, 1
                );
                
                free(result);
                
                if (instr) {
                    F_tac_data_add_instruction(tac_data, instr);
                }
            }
        }
        // 其他指令类型的解析可以在这里添加
        
        line = strtok(NULL, "\n");
        line_number++;
    }
    
    free(data_copy);
    
    // 如果解析失败，添加错误信息
    if (tac_data->instruction_count == 0) {
        F_tac_data_add_error(tac_data, "反序列化失败：未解析到有效指令");
    }
    
    return state;
}

/**
 * @brief 验证TAC
 */
static bool tac_validate(void* ir)
{
    if (!ir) {
        return false;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    Stru_TacData_t* data = state->tac_data;
    
    // 基本验证：检查指令是否有效
    for (size_t i = 0; i < data->instruction_count; i++) {
        Stru_TacInstruction_t* instr = data->instructions[i];
        if (!instr) {
            F_tac_data_add_error(data, "发现空指令");
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 销毁TAC
 */
static void tac_destroy(void* ir)
{
    if (!ir) {
        return;
    }
    
    Stru_TacInternalState_t* state = (Stru_TacInternalState_t*)ir;
    
    // 销毁TAC数据
    F_destroy_tac_data(state->tac_data);
    
    // 销毁内部状态
    free(state->name);
    free(state->version);
    free(state);
}

/**
 * @brief 将TAC指令转换为IR指令
 */
static Stru_IrInstruction_t* convert_tac_to_ir_instruction(Stru_TacInstruction_t* tac_instr)
{
    if (!tac_instr) {
        return NULL;
    }
    
    Stru_IrInstruction_t* ir_instr = (Stru_IrInstruction_t*)malloc(sizeof(Stru_IrInstruction_t));
    if (!ir_instr) {
        return NULL;
    }
    
    // 设置基本属性
    ir_instr->type = Eum_IR_INSTR_BINARY_OP; // 默认类型，需要根据操作码调整
    ir_instr->opcode = strdup(F_tac_opcode_to_string(tac_instr->opcode));
    ir_instr->line = tac_instr->line;
    ir_instr->column = tac_instr->column;
    ir_instr->extra_data = NULL;
    
    // 设置操作数
    ir_instr->operand_count = 0;
    ir_instr->operands = NULL;
    
    // 根据操作码确定操作数数量
    size_t max_operands = 3; // TAC最多3个操作数
    ir_instr->operands = (char**)malloc(max_operands * sizeof(char*));
    if (ir_instr->operands) {
        if (tac_instr->result) {
            ir_instr->operands[ir_instr->operand_count] = strdup(tac_instr->result);
            ir_instr->operand_count++;
        }
        
        if (tac_instr->operand1) {
            ir_instr->operands[ir_instr->operand_count] = strdup(tac_instr->operand1);
            ir_instr->operand_count++;
        }
        
        if (tac_instr->operand2) {
            ir_instr->operands[ir_instr->operand_count] = strdup(tac_instr->operand2);
            ir_instr->operand_count++;
        }
    }
    
    return ir_instr;
}

/**
 * @brief 将IR指令转换为TAC指令
 */
static Stru_TacInstruction_t* convert_ir_to_tac_instruction(Stru_IrInstruction_t* ir_instr)
{
    if (!ir_instr) {
        return NULL;
    }
    
    // 将IR操作码转换为TAC操作码
    Eum_TacOpcode tac_opcode = F_string_to_tac_opcode(ir_instr->opcode);
    
    // 创建TAC指令
    Stru_TacInstruction_t* tac_instr = F_create_tac_instruction(
        tac_opcode,
        NULL, // result
        NULL, // operand1
        NULL, // operand2
        NULL, // label
        ir_instr->line,
        ir_instr->column
    );
    
    if (!tac_instr) {
        return NULL;
    }
    
    // 设置操作数
    if (ir_instr->operand_count > 0) {
        tac_instr->result = ir_instr->operands[0] ? strdup(ir_instr->operands[0]) : NULL;
    }
    
    if (ir_instr->operand_count > 1) {
        tac_instr->operand1 = ir_instr->operands[1] ? strdup(ir_instr->operands[1]) : NULL;
    }
    
    if (ir_instr->operand_count > 2) {
        tac_instr->operand2 = ir_instr->operands[2] ? strdup(ir_instr->operands[2]) : NULL;
    }
    
    return tac_instr;
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建TAC接口实例
 */
Stru_IrInterface_t* F_create_tac_interface(void)
{
    Stru_IrInterface_t* interface = (Stru_IrInterface_t*)malloc(sizeof(Stru_IrInterface_t));
    if (!interface) {
        return NULL;
    }
    
    // 设置接口函数指针
    interface->get_name = tac_get_name;
    interface->get_version = tac_get_version;
    interface->get_type = tac_get_type;
    interface->create_from_ast = tac_create_from_ast;
    interface->create_empty = tac_create_empty;
    interface->add_instruction = tac_add_instruction;
    interface->remove_instruction = tac_remove_instruction;
    interface->get_instruction = tac_get_instruction;
    interface->get_instruction_count = tac_get_instruction_count;
    interface->has_errors = tac_has_errors;
    interface->get_errors = tac_get_errors;
    interface->clear_errors = tac_clear_errors;
    interface->convert_to_lower = tac_convert_to_lower;
    interface->convert_to_higher = tac_convert_to_higher;
    interface->convert_to_type = tac_convert_to_type;
    interface->apply_optimization = tac_apply_optimization;
    interface->apply_all_optimizations = tac_apply_all_optimizations;
    interface->serialize = tac_serialize;
    interface->deserialize = tac_deserialize;
    interface->validate = tac_validate;
    interface->destroy = tac_destroy;
    interface->internal_state = NULL;
    
    return interface;
}
