/******************************************************************************
 * 文件名: CN_bytecode_backend.h
 * 功能: CN_Language 字节码后端接口
 * 
 * 定义字节码后端的公共接口，提供将CN_Language抽象语法树或中间代码转换为字节码的功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_BACKEND_H
#define CN_BYTECODE_BACKEND_H

#include "../../CN_codegen_interface.h"
#include "../../../ir/CN_ir_interface.h"
#include "CN_bytecode_instructions.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内部类型定义（在头文件中公开，因为它们在公共接口中使用）
// ============================================================================

/**
 * @brief 行号表条目结构体
 */
typedef struct {
    uint32_t instruction_offset;  ///< 指令偏移
    uint32_t source_line;         ///< 源代码行号
    uint32_t source_column;       ///< 源代码列号
} Stru_LineTableEntry_t;

/**
 * @brief 调试符号结构体
 */
typedef struct {
    uint32_t offset;              ///< 指令偏移
    uint32_t type;                ///< 符号类型
    const char* name;             ///< 符号名称
} Stru_DebugSymbol_t;

// ============================================================================
// 字节码后端配置结构体
// ============================================================================

/**
 * @brief 字节码后端配置结构体
 * 
 * 配置字节码生成和解释的参数。
 */
typedef struct Stru_BytecodeBackendConfig_t {
    bool optimize_bytecode;          ///< 是否优化字节码
    int optimization_level;          ///< 优化级别 (0-3)
    bool include_debug_info;         ///< 是否包含调试信息
    size_t stack_size;               ///< 虚拟机栈大小（字节）
    size_t heap_size;                ///< 虚拟机堆大小（字节）
    bool enable_gc;                  ///< 是否启用垃圾回收
    bool enable_profiling;           ///< 是否启用性能分析
    const char* output_format;       ///< 输出格式 ("binary", "text", "hex")
} Stru_BytecodeBackendConfig_t;

/**
 * @brief 字节码程序结构体
 * 
 * 包含完整的字节码程序，包括指令、常量池、调试信息等。
 */
typedef struct Stru_BytecodeProgram_t {
    Stru_BytecodeInstruction_t* instructions;  ///< 指令数组
    size_t instruction_count;                  ///< 指令数量
    size_t instruction_capacity;               ///< 指令数组容量
    
    char** constant_pool;                      ///< 常量池（字符串常量）
    size_t constant_count;                     ///< 常量数量
    size_t constant_capacity;                  ///< 常量池容量
    
    Stru_LineTableEntry_t* line_table;         ///< 行号表（指令索引->源代码行号）
    size_t line_table_size;                    ///< 行号表大小
    
    Stru_DebugSymbol_t* debug_symbols;         ///< 调试符号表
    size_t debug_symbol_count;                 ///< 调试符号数量
    
    size_t stack_size;                         ///< 所需栈大小
    size_t heap_size;                          ///< 所需堆大小
    size_t global_count;                       ///< 全局变量数量
    
    const char* entry_point;                   ///< 入口点函数名
    uint32_t entry_point_offset;               ///< 入口点指令偏移
    
    void* internal_data;                       ///< 内部数据
} Stru_BytecodeProgram_t;

// ============================================================================
// 字节码后端接口函数
// ============================================================================

/**
 * @brief 从AST生成字节码
 * 
 * 将CN_Language的抽象语法树转换为字节码程序。
 * 
 * @param ast 抽象语法树根节点
 * @param config 字节码生成配置
 * @return Stru_BytecodeProgram_t* 字节码程序，调用者负责销毁
 * 
 * @note 此函数是字节码后端的主要入口点，负责协调整个字节码生成过程。
 *       生成的字节码可以直接由CN_Language虚拟机解释执行。
 */
Stru_BytecodeProgram_t* F_generate_bytecode_from_ast(Stru_AstNode_t* ast, 
                                                     const Stru_BytecodeBackendConfig_t* config);

/**
 * @brief 从IR生成字节码
 * 
 * 将中间代码表示（IR）转换为字节码程序。
 * 
 * @param ir 中间代码表示
 * @param config 字节码生成配置
 * @return Stru_BytecodeProgram_t* 字节码程序，调用者负责销毁
 * 
 * @note 此函数支持从多种IR形式（TAC、SSA等）生成字节码。
 */
Stru_BytecodeProgram_t* F_generate_bytecode_from_ir(void* ir, 
                                                    const Stru_BytecodeBackendConfig_t* config);

/**
 * @brief 获取字节码后端版本信息
 * 
 * 返回字节码后端的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 * 
 * @note 版本号遵循语义化版本规范：
 *       - 主版本号：不兼容的API修改
 *       - 次版本号：向下兼容的功能性新增
 *       - 修订号：向下兼容的问题修正
 */
void F_get_bytecode_backend_version(int* major, int* minor, int* patch);

/**
 * @brief 获取字节码后端版本字符串
 * 
 * 返回字节码后端的版本字符串。
 * 
 * @return 版本字符串，格式为"主版本号.次版本号.修订号"
 * 
 * @note 返回的字符串是静态常量，调用者不应修改或释放。
 */
const char* F_get_bytecode_backend_version_string(void);

/**
 * @brief 检查字节码后端是否支持特定功能
 * 
 * 检查字节码后端是否支持特定的代码生成功能。
 * 
 * @param feature 功能标识符
 * @return 支持返回true，不支持返回false
 * 
 * @note 功能标识符可以是以下值：
 *       - "ast_to_bytecode": 支持从AST生成字节码
 *       - "ir_to_bytecode": 支持从IR生成字节码
 *       - "optimization": 支持字节码优化
 *       - "debug_info": 支持调试信息生成
 *       - "profiling": 支持性能分析
 *       - "gc": 支持垃圾回收
 */
bool F_bytecode_backend_supports_feature(const char* feature);

/**
 * @brief 配置字节码后端选项
 * 
 * 配置字节码后端的代码生成选项。
 * 
 * @param option 选项名称
 * @param value 选项值
 * @return 配置成功返回true，失败返回false
 * 
 * @note 支持的选项：
 *       - "optimization_level": 优化级别 (0-3)
 *       - "debug_info": 是否生成调试信息 (true/false)
 *       - "stack_size": 栈大小（字节）
 *       - "heap_size": 堆大小（字节）
 *       - "enable_gc": 是否启用垃圾回收 (true/false)
 *       - "output_format": 输出格式 ("binary", "text", "hex")
 */
bool F_configure_bytecode_backend(const char* option, const char* value);

/**
 * @brief 创建字节码后端代码生成器接口
 * 
 * 创建并返回一个字节码后端特定的代码生成器接口实例。
 * 
 * @return Stru_CodeGeneratorInterface_t* 字节码后端代码生成器接口实例
 * 
 * @note 此函数创建专门用于字节码生成的接口实例，
 *       与通用的F_create_codegen_interface()函数不同，
 *       此函数创建的接口已经预配置为字节码后端。
 */
Stru_CodeGeneratorInterface_t* F_create_bytecode_backend_interface(void);

/**
 * @brief 创建默认字节码后端配置
 * 
 * 创建并返回默认的字节码后端配置。
 * 
 * @return Stru_BytecodeBackendConfig_t 默认字节码后端配置
 */
Stru_BytecodeBackendConfig_t F_create_default_bytecode_backend_config(void);

/**
 * @brief 销毁字节码程序
 * 
 * 释放字节码程序占用的所有资源。
 * 
 * @param program 要销毁的字节码程序
 */
void F_destroy_bytecode_program(Stru_BytecodeProgram_t* program);

/**
 * @brief 序列化字节码程序
 * 
 * 将字节码程序序列化为二进制格式。
 * 
 * @param program 字节码程序
 * @param data 输出参数，序列化数据
 * @param size 输出参数，数据大小
 * @return 成功返回true，失败返回false
 */
bool F_serialize_bytecode_program(const Stru_BytecodeProgram_t* program, 
                                  uint8_t** data, size_t* size);

/**
 * @brief 反序列化字节码程序
 * 
 * 从二进制数据反序列化字节码程序。
 * 
 * @param data 序列化数据
 * @param size 数据大小
 * @return Stru_BytecodeProgram_t* 反序列化的字节码程序，失败返回NULL
 */
Stru_BytecodeProgram_t* F_deserialize_bytecode_program(const uint8_t* data, size_t size);

/**
 * @brief 优化字节码程序
 * 
 * 对字节码程序应用优化。
 * 
 * @param program 要优化的字节码程序
 * @param optimization_level 优化级别 (0-3)
 * @return 优化后的字节码程序，调用者负责销毁原程序和新程序
 */
Stru_BytecodeProgram_t* F_optimize_bytecode_program(Stru_BytecodeProgram_t* program, 
                                                    int optimization_level);

/**
 * @brief 验证字节码程序
 * 
 * 验证字节码程序的正确性和安全性。
 * 
 * @param program 要验证的字节码程序
 * @param errors 输出参数，错误信息数组
 * @param error_count 输出参数，错误数量
 * @return 验证通过返回true，失败返回false
 */
bool F_validate_bytecode_program(const Stru_BytecodeProgram_t* program, 
                                 char*** errors, size_t* error_count);

/**
 * @brief 格式化字节码程序为文本
 * 
 * 将字节码程序格式化为可读的文本格式。
 * 
 * @param program 字节码程序
 * @return 格式化后的文本，调用者负责释放
 */
char* F_format_bytecode_program(const Stru_BytecodeProgram_t* program);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_BACKEND_H */
