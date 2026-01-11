/******************************************************************************
 * 文件名: CN_bytecode_generator.c
 * 功能: 字节码生成器模块
 * 
 * 提供从AST和IR生成字节码的功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "../CN_bytecode_backend.h"
#include "CN_bytecode_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 内部函数声明
// ============================================================================

static void generate_bytecode_from_ast_node(Stru_BytecodeProgram_t* program,
                                           Stru_AstNode_t* node,
                                           const Stru_BytecodeBackendConfig_t* config);

static void generate_bytecode_from_ir_node(Stru_BytecodeProgram_t* program,
                                          void* ir_node,
                                          const Stru_BytecodeBackendConfig_t* config);

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 从AST节点生成字节码
 */
static void generate_bytecode_from_ast_node(Stru_BytecodeProgram_t* program,
                                           Stru_AstNode_t* node,
                                           const Stru_BytecodeBackendConfig_t* config)
{
    if (!program || !node) {
        return;
    }
    
    // 获取AST节点接口
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(node);
    if (!interface) {
        return;
    }
    
    // 获取节点类型
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    // 根据节点类型生成相应的字节码
    switch (node_type) {
        case Eum_AST_PROGRAM:
            // 程序节点：生成所有子节点的字节码
            for (size_t i = 0; i < interface->get_child_count(interface); i++) {
                Stru_AstNodeInterface_t* child = interface->get_child(interface, i);
                if (child) {
                    generate_bytecode_from_ast_node(program, (Stru_AstNode_t*)child, config);
                }
            }
            break;
            
        case Eum_AST_FUNCTION_DECL:
            // 函数声明：生成函数入口点
            {
                const char* func_name = F_ast_get_identifier(node);
                if (func_name && strcmp(func_name, "main") == 0) {
                    // 设置主函数为入口点
                    program->entry_point = func_name;
                    program->entry_point_offset = program->instruction_count;
                }
                
                // 生成函数体字节码
                // 这里简化处理，实际应该生成完整的函数字节码
                Stru_BytecodeInstruction_t entry_inst;
                memset(&entry_inst, 0, sizeof(entry_inst));
                entry_inst.opcode = Eum_BC_NOP;
                entry_inst.line = 1;
                entry_inst.column = 1;
                entry_inst.comment = "函数入口点";
                F_add_instruction(program, &entry_inst);
            }
            break;
            
        case Eum_AST_RETURN_STMT:
            // 返回语句：生成返回指令
            {
                Stru_BytecodeInstruction_t ret_inst;
                memset(&ret_inst, 0, sizeof(ret_inst));
                ret_inst.opcode = Eum_BC_RETURN_VOID;
                ret_inst.line = 1;
                ret_inst.column = 1;
                F_add_instruction(program, &ret_inst);
            }
            break;
            
        default:
            // 其他节点类型：生成NOP指令作为占位符
            {
                Stru_BytecodeInstruction_t nop_inst;
                memset(&nop_inst, 0, sizeof(nop_inst));
                nop_inst.opcode = Eum_BC_NOP;
                nop_inst.line = 1;
                nop_inst.column = 1;
                nop_inst.comment = "未实现的AST节点类型";
                F_add_instruction(program, &nop_inst);
            }
            break;
    }
}

/**
 * @brief 从IR节点生成字节码
 */
static void generate_bytecode_from_ir_node(Stru_BytecodeProgram_t* program,
                                          void* ir_node,
                                          const Stru_BytecodeBackendConfig_t* config)
{
    if (!program || !ir_node) {
        return;
    }
    
    // 这里简化处理，实际应该根据IR类型生成相应的字节码
    // 目前只生成NOP指令作为占位符
    Stru_BytecodeInstruction_t nop_inst;
    memset(&nop_inst, 0, sizeof(nop_inst));
    nop_inst.opcode = Eum_BC_NOP;
    nop_inst.line = 1;
    nop_inst.column = 1;
    nop_inst.comment = "从IR生成的字节码（占位符）";
    F_add_instruction(program, &nop_inst);
}

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 从AST生成字节码
 */
Stru_BytecodeProgram_t* F_generate_bytecode_from_ast_impl(Stru_AstNode_t* ast, 
                                                         const Stru_BytecodeBackendConfig_t* config)
{
    if (!ast) {
        return NULL;
    }
    
    // 验证配置
    Stru_BytecodeBackendConfig_t effective_config;
    if (config) {
        if (!F_validate_config(config)) {
            return NULL;
        }
        effective_config = *config;
    } else {
        effective_config = F_create_default_bytecode_backend_config();
    }
    
    // 创建字节码程序
    Stru_BytecodeProgram_t* program = F_create_bytecode_program();
    if (!program) {
        return NULL;
    }
    
    // 应用配置
    program->stack_size = effective_config.stack_size;
    program->heap_size = effective_config.heap_size;
    
    // 生成字节码
    generate_bytecode_from_ast_node(program, ast, &effective_config);
    
    // 如果启用了优化，应用优化
    if (effective_config.optimize_bytecode) {
        // 这里可以调用优化函数
        // 目前只是占位符
    }
    
    return program;
}

/**
 * @brief 从IR生成字节码
 */
Stru_BytecodeProgram_t* F_generate_bytecode_from_ir_impl(void* ir, 
                                                        const Stru_BytecodeBackendConfig_t* config)
{
    if (!ir) {
        return NULL;
    }
    
    // 验证配置
    Stru_BytecodeBackendConfig_t effective_config;
    if (config) {
        if (!F_validate_config(config)) {
            return NULL;
        }
        effective_config = *config;
    } else {
        effective_config = F_create_default_bytecode_backend_config();
    }
    
    // 创建字节码程序
    Stru_BytecodeProgram_t* program = F_create_bytecode_program();
    if (!program) {
        return NULL;
    }
    
    // 应用配置
    program->stack_size = effective_config.stack_size;
    program->heap_size = effective_config.heap_size;
    
    // 生成字节码
    generate_bytecode_from_ir_node(program, ir, &effective_config);
    
    // 如果启用了优化，应用优化
    if (effective_config.optimize_bytecode) {
        // 这里可以调用优化函数
        // 目前只是占位符
    }
    
    return program;
}

/**
 * @brief 获取字节码后端版本信息
 */
void F_get_bytecode_backend_version_impl(int* major, int* minor, int* patch)
{
    if (major) *major = CN_BYTECODE_VERSION_MAJOR;
    if (minor) *minor = CN_BYTECODE_VERSION_MINOR;
    if (patch) *patch = CN_BYTECODE_VERSION_PATCH;
}

/**
 * @brief 获取字节码后端版本字符串
 */
const char* F_get_bytecode_backend_version_string_impl(void)
{
    return CN_BYTECODE_VERSION_STRING;
}

/**
 * @brief 检查字节码后端是否支持特定功能
 */
bool F_bytecode_backend_supports_feature_impl(const char* feature)
{
    if (!feature) return false;
    
    // 支持的功能列表
    if (strcmp(feature, "ast_to_bytecode") == 0) return true;
    if (strcmp(feature, "ir_to_bytecode") == 0) return true;
    if (strcmp(feature, "optimization") == 0) return true;
    if (strcmp(feature, "debug_info") == 0) return true;
    if (strcmp(feature, "profiling") == 0) return false;  // 暂不支持
    if (strcmp(feature, "gc") == 0) return false;         // 暂不支持
    
    return false;
}

/**
 * @brief 配置字节码后端选项
 */
bool F_configure_bytecode_backend_impl(const char* option, const char* value)
{
    if (!option || !value) {
        return false;
    }
    
    // 这里可以添加配置逻辑
    // 目前只是记录配置，实际实现中应该影响代码生成
    
    return true;
}

/**
 * @brief 创建字节码后端代码生成器接口
 */
Stru_CodeGeneratorInterface_t* F_create_bytecode_backend_interface_impl(void)
{
    // 分配接口内存
    Stru_CodeGeneratorInterface_t* interface = (Stru_CodeGeneratorInterface_t*)malloc(
        sizeof(Stru_CodeGeneratorInterface_t));
    if (!interface) {
        return NULL;
    }
    
    // 初始化所有函数指针为NULL
    memset(interface, 0, sizeof(Stru_CodeGeneratorInterface_t));
    
    // 设置内部状态
    interface->internal_state = NULL;
    
    return interface;
}
