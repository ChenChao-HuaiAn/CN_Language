/******************************************************************************
 * 文件名: CN_interpreter_engine.c
 * 功能: CN_Language 字节码解释器指令执行引擎
 * 
 * 负责解释器的指令执行功能，包括算术、逻辑、控制流等指令。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 从CN_bytecode_interpreter_main.c拆分出来
 * 版权: MIT许可证
 ******************************************************************************/

#include "../CN_bytecode_interpreter.h"
#include "../CN_bytecode_instructions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// 外部函数声明
bool F_push_value(Stru_BytecodeInterpreter_t* interpreter, Stru_BytecodeValue_t value);
Stru_BytecodeValue_t F_pop_value(Stru_BytecodeInterpreter_t* interpreter);
Stru_BytecodeValue_t F_peek_value(Stru_BytecodeInterpreter_t* interpreter, size_t offset);
void F_add_error_internal(Stru_BytecodeInterpreter_t* interpreter, const char* format, ...);
bool F_ensure_call_stack_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);

/**
 * @brief 执行算术指令
 */
Stru_BytecodeValue_t F_execute_arithmetic_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                     const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result;
    memset(&result, 0, sizeof(result));
    result.type = Eum_BCV_NULL;
    
    if (!interpreter || !instruction) {
        return result;
    }
    
    // 根据操作码执行算术指令
    switch (instruction->opcode) {
        case Eum_BC_ADD:
            // 加法运算
            if (interpreter->stack_top >= 2) {
                Stru_BytecodeValue_t b = F_pop_value(interpreter);
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                
                // 简化：只处理整数加法
                if (a.type == Eum_BCV_INT32 && b.type == Eum_BCV_INT32) {
                    result.type = Eum_BCV_INT32;
                    result.data.int32_value = a.data.int32_value + b.data.int32_value;
                    F_push_value(interpreter, result);
                }
            }
            break;
            
        case Eum_BC_SUB:
            // 减法运算
            if (interpreter->stack_top >= 2) {
                Stru_BytecodeValue_t b = F_pop_value(interpreter);
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                
                if (a.type == Eum_BCV_INT32 && b.type == Eum_BCV_INT32) {
                    result.type = Eum_BCV_INT32;
                    result.data.int32_value = a.data.int32_value - b.data.int32_value;
                    F_push_value(interpreter, result);
                }
            }
            break;
            
        case Eum_BC_MUL:
            // 乘法运算
            if (interpreter->stack_top >= 2) {
                Stru_BytecodeValue_t b = F_pop_value(interpreter);
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                
                if (a.type == Eum_BCV_INT32 && b.type == Eum_BCV_INT32) {
                    result.type = Eum_BCV_INT32;
                    result.data.int32_value = a.data.int32_value * b.data.int32_value;
                    F_push_value(interpreter, result);
                }
            }
            break;
            
        case Eum_BC_DIV:
            // 除法运算
            if (interpreter->stack_top >= 2) {
                Stru_BytecodeValue_t b = F_pop_value(interpreter);
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                
                if (a.type == Eum_BCV_INT32 && b.type == Eum_BCV_INT32) {
                    if (b.data.int32_value != 0) {
                        result.type = Eum_BCV_INT32;
                        result.data.int32_value = a.data.int32_value / b.data.int32_value;
                        F_push_value(interpreter, result);
                    } else {
                        F_add_error_internal(interpreter, "除法运算除数为零");
                    }
                }
            }
            break;
            
        case Eum_BC_MOD:
            // 取模运算
            if (interpreter->stack_top >= 2) {
                Stru_BytecodeValue_t b = F_pop_value(interpreter);
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                
                if (a.type == Eum_BCV_INT32 && b.type == Eum_BCV_INT32) {
                    if (b.data.int32_value != 0) {
                        result.type = Eum_BCV_INT32;
                        result.data.int32_value = a.data.int32_value % b.data.int32_value;
                        F_push_value(interpreter, result);
                    } else {
                        F_add_error_internal(interpreter, "取模运算除数为零");
                    }
                }
            }
            break;
            
        case Eum_BC_NEG:
            // 取负运算
            if (interpreter->stack_top > 0) {
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                result.type = a.type;
                
                switch (a.type) {
                    case Eum_BCV_INT32:
                        result.data.int32_value = -a.data.int32_value;
                        break;
                    case Eum_BCV_FLOAT32:
                        result.data.float32_value = -a.data.float32_value;
                        break;
                    default:
                        // 其他类型不支持取负操作
                        F_add_error_internal(interpreter, "不支持对类型 %d 进行取负操作", a.type);
                        result.type = Eum_BCV_NULL;
                        break;
                }
                
                if (result.type != Eum_BCV_NULL) {
                    F_push_value(interpreter, result);
                }
            }
            break;
            
        case Eum_BC_INC:
            // 自增运算
            if (interpreter->stack_top > 0) {
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                result.type = a.type;
                
                switch (a.type) {
                    case Eum_BCV_INT32:
                        result.data.int32_value = a.data.int32_value + 1;
                        break;
                    case Eum_BCV_FLOAT32:
                        result.data.float32_value = a.data.float32_value + 1.0f;
                        break;
                    default:
                        // 其他类型不支持自增操作
                        F_add_error_internal(interpreter, "不支持对类型 %d 进行自增操作", a.type);
                        result.type = Eum_BCV_NULL;
                        break;
                }
                
                if (result.type != Eum_BCV_NULL) {
                    F_push_value(interpreter, result);
                }
            }
            break;
            
        case Eum_BC_DEC:
            // 自减运算
            if (interpreter->stack_top > 0) {
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                result.type = a.type;
                
                switch (a.type) {
                    case Eum_BCV_INT32:
                        result.data.int32_value = a.data.int32_value - 1;
                        break;
                    case Eum_BCV_FLOAT32:
                        result.data.float32_value = a.data.float32_value - 1.0f;
                        break;
                    default:
                        // 其他类型不支持自减操作
                        F_add_error_internal(interpreter, "不支持对类型 %d 进行自减操作", a.type);
                        result.type = Eum_BCV_NULL;
                        break;
                }
                
                if (result.type != Eum_BCV_NULL) {
                    F_push_value(interpreter, result);
                }
            }
            break;
            
        default:
            // 不是算术指令，返回空值
            break;
    }
    
    return result;
}

/**
 * @brief 执行比较指令
 */
Stru_BytecodeValue_t F_execute_comparison_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                     const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result;
    memset(&result, 0, sizeof(result));
    result.type = Eum_BCV_NULL;
    
    if (!interpreter || !instruction) {
        return result;
    }
    
    // 根据操作码执行比较指令
    switch (instruction->opcode) {
        case Eum_BC_EQ:
            // 等于比较
            if (interpreter->stack_top >= 2) {
                Stru_BytecodeValue_t b = F_pop_value(interpreter);
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                
                result.type = Eum_BCV_BOOL;
                
                if (a.type == b.type) {
                    switch (a.type) {
                        case Eum_BCV_INT32:
                            result.data.bool_value = a.data.int32_value == b.data.int32_value;
                            break;
                        case Eum_BCV_FLOAT32:
                            result.data.bool_value = a.data.float32_value == b.data.float32_value;
                            break;
                        case Eum_BCV_BOOL:
                            result.data.bool_value = a.data.bool_value == b.data.bool_value;
                            break;
                        case Eum_BCV_STRING:
                            if (a.data.string_value == NULL && b.data.string_value == NULL) {
                                result.data.bool_value = true;
                            } else if (a.data.string_value == NULL || b.data.string_value == NULL) {
                                result.data.bool_value = false;
                            } else {
                                result.data.bool_value = strcmp(a.data.string_value, b.data.string_value) == 0;
                            }
                            break;
                        default:
                            result.data.bool_value = false;
                            break;
                    }
                } else {
                    result.data.bool_value = false;
                }
                
                F_push_value(interpreter, result);
            }
            break;
            
        case Eum_BC_NE:
            // 不等于比较
            if (interpreter->stack_top >= 2) {
                Stru_BytecodeValue_t b = F_pop_value(interpreter);
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                
                result.type = Eum_BCV_BOOL;
                
                if (a.type == b.type) {
                    switch (a.type) {
                        case Eum_BCV_INT32:
                            result.data.bool_value = a.data.int32_value != b.data.int32_value;
                            break;
                        case Eum_BCV_FLOAT32:
                            result.data.bool_value = a.data.float32_value != b.data.float32_value;
                            break;
                        case Eum_BCV_BOOL:
                            result.data.bool_value = a.data.bool_value != b.data.bool_value;
                            break;
                        case Eum_BCV_STRING:
                            if (a.data.string_value == NULL && b.data.string_value == NULL) {
                                result.data.bool_value = false;
                            } else if (a.data.string_value == NULL || b.data.string_value == NULL) {
                                result.data.bool_value = true;
                            } else {
                                result.data.bool_value = strcmp(a.data.string_value, b.data.string_value) != 0;
                            }
                            break;
                        default:
                            result.data.bool_value = true;
                            break;
                    }
                } else {
                    result.data.bool_value = true;
                }
                
                F_push_value(interpreter, result);
            }
            break;
            
        default:
            // 不是比较指令，返回空值
            break;
    }
    
    return result;
}

/**
 * @brief 执行逻辑指令
 */
Stru_BytecodeValue_t F_execute_logical_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                  const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result;
    memset(&result, 0, sizeof(result));
    result.type = Eum_BCV_NULL;
    
    if (!interpreter || !instruction) {
        return result;
    }
    
    // 根据操作码执行逻辑指令
    switch (instruction->opcode) {
        case Eum_BC_NOT:
            // 逻辑非
            if (interpreter->stack_top > 0) {
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                result.type = Eum_BCV_BOOL;
                
                if (a.type == Eum_BCV_BOOL) {
                    result.data.bool_value = !a.data.bool_value;
                } else if (a.type == Eum_BCV_INT32) {
                    result.data.bool_value = a.data.int32_value == 0;
                } else if (a.type == Eum_BCV_FLOAT32) {
                    result.data.bool_value = a.data.float32_value == 0.0f;
                } else if (a.type == Eum_BCV_STRING) {
                    result.data.bool_value = a.data.string_value == NULL || a.data.string_value[0] == '\0';
                } else {
                    result.data.bool_value = true; // null或其他类型视为假
                }
                
                F_push_value(interpreter, result);
            }
            break;
            
        default:
            // 不是逻辑指令，返回空值
            break;
    }
    
    return result;
}
