/******************************************************************************
 * 文件名: CN_x86_backend.h
 * 功能: CN_Language x86/x86-64汇编后端接口
 * 
 * 定义x86/x86-64汇编后端的公共接口，提供将CN_Language抽象语法树或中间表示转换为x86汇编代码的功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_X86_BACKEND_H
#define CN_X86_BACKEND_H

#include "../../CN_codegen_interface.h"
#include "../../CN_target_codegen_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * 类型定义
 * ============================================ */

/**
 * @brief x86架构枚举
 * 
 * 定义支持的x86架构变体。
 */
typedef enum Eum_X86Architecture {
    Eum_X86_ARCH_32BIT,          ///< x86 32位架构
    Eum_X86_ARCH_64BIT,          ///< x86-64 64位架构
    Eum_X86_ARCH_X32,            ///< x32 ABI（64位寄存器，32位指针）
    Eum_X86_ARCH_IA32,           ///< Intel IA-32架构
    Eum_X86_ARCH_AMD64           ///< AMD64架构
} Eum_X86Architecture;

/**
 * @brief x86指令集扩展枚举
 * 
 * 定义支持的x86指令集扩展。
 */
typedef enum Eum_X86Extension {
    Eum_X86_EXT_MMX,             ///< MMX扩展
    Eum_X86_EXT_SSE,             ///< SSE扩展
    Eum_X86_EXT_SSE2,            ///< SSE2扩展
    Eum_X86_EXT_SSE3,            ///< SSE3扩展
    Eum_X86_EXT_SSSE3,           ///< SSSE3扩展
    Eum_X86_EXT_SSE41,           ///< SSE4.1扩展
    Eum_X86_EXT_SSE42,           ///< SSE4.2扩展
    Eum_X86_EXT_AVX,             ///< AVX扩展
    Eum_X86_EXT_AVX2,            ///< AVX2扩展
    Eum_X86_EXT_AVX512,          ///< AVX-512扩展
    Eum_X86_EXT_FMA,             ///< FMA扩展
    Eum_X86_EXT_AES,             ///< AES指令集
    Eum_X86_EXT_SHA,             ///< SHA指令集
    Eum_X86_EXT_RDRAND,          ///< RDRAND指令
    Eum_X86_EXT_RDSEED           ///< RDSEED指令
} Eum_X86Extension;

/**
 * @brief x86后端配置结构体
 * 
 * 定义x86汇编后端的配置选项。
 */
typedef struct Stru_X86BackendConfig_t {
    Eum_X86Architecture architecture;   ///< x86架构变体
    bool use_64bit;                     ///< 是否使用64位模式
    const char* cpu_model;              ///< CPU型号（如 "corei7", "athlon64"）
    const char* features;               ///< CPU特性字符串
    int optimization_level;             ///< 优化级别（0-3）
    bool debug_info;                    ///< 是否生成调试信息
    bool position_independent;          ///< 是否生成位置无关代码
    bool stack_protector;               ///< 是否启用栈保护
    bool omit_frame_pointer;            ///< 是否省略帧指针
    bool red_zone;                      ///< 是否使用红区（x86-64）
    bool pic_base_register;             ///< 是否使用PIC基址寄存器
    const char* assembler_syntax;       ///< 汇编语法（"att", "intel"）
    const char* output_format;          ///< 输出格式（"asm", "obj", "exe"）
} Stru_X86BackendConfig_t;

/**
 * @brief x86后端状态结构体
 * 
 * 存储x86后端的内部状态。
 */
typedef struct Stru_X86BackendState_t {
    Stru_X86BackendConfig_t config;     ///< 当前配置
    Stru_DynamicArray_t* errors;        ///< 错误信息数组
    Stru_DynamicArray_t* warnings;      ///< 警告信息数组
    void* code_buffer;                  ///< 代码缓冲区
    size_t code_buffer_size;            ///< 代码缓冲区大小
    size_t code_buffer_capacity;        ///< 代码缓冲区容量
    void* symbol_table;                 ///< 符号表
    void* relocation_table;             ///< 重定位表
    bool initialized;                   ///< 是否已初始化
} Stru_X86BackendState_t;

/* ============================================
 * 公共接口函数
 * ============================================ */

/**
 * @brief 从AST生成x86汇编代码
 * 
 * 将CN_Language的抽象语法树转换为x86汇编代码。
 * 
 * @param ast 抽象语法树根节点
 * @param config x86后端配置（可为NULL，使用默认配置）
 * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
 * 
 * @note 此函数是x86后端的主要入口点，负责协调整个x86汇编代码生成过程。
 *       生成的汇编代码可以直接用汇编器（如NASM、GAS）汇编。
 */
Stru_CodeGenResult_t* F_generate_x86_asm_from_ast(Stru_AstNode_t* ast, const Stru_X86BackendConfig_t* config);

/**
 * @brief 从IR生成x86汇编代码
 * 
 * 将CN_Language的中间表示转换为x86汇编代码。
 * 
 * @param ir 中间表示数据
 * @param ir_size 中间表示大小
 * @param config x86后端配置（可为NULL，使用默认配置）
 * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
 */
Stru_CodeGenResult_t* F_generate_x86_asm_from_ir(const void* ir, size_t ir_size, const Stru_X86BackendConfig_t* config);

/**
 * @brief 从LLVM IR生成x86汇编代码
 * 
 * 将LLVM IR转换为x86汇编代码。
 * 
 * @param llvm_ir LLVM IR字符串
 * @param config x86后端配置（可为NULL，使用默认配置）
 * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
 */
Stru_CodeGenResult_t* F_generate_x86_asm_from_llvm_ir(const char* llvm_ir, const Stru_X86BackendConfig_t* config);

/**
 * @brief 优化x86汇编代码
 * 
 * 对x86汇编代码应用优化。
 * 
 * @param asm_code x86汇编代码字符串
 * @param optimization_level 优化级别（0-3）
 * @return char* 优化后的x86汇编代码字符串，调用者负责释放
 */
char* F_optimize_x86_asm(const char* asm_code, int optimization_level);

/**
 * @brief 将x86汇编代码汇编为目标文件
 * 
 * 将x86汇编代码汇编为特定目标文件格式。
 * 
 * @param asm_code x86汇编代码字符串
 * @param target_config 目标配置
 * @return Stru_TargetCodeGenResult_t* 目标代码生成结果，调用者负责销毁
 */
Stru_TargetCodeGenResult_t* F_assemble_x86_asm_to_object(const char* asm_code, const Stru_TargetConfig_t* target_config);

/**
 * @brief 获取x86后端版本信息
 * 
 * 返回x86后端的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 * 
 * @note 版本号遵循语义化版本规范。
 */
void F_get_x86_backend_version(int* major, int* minor, int* patch);

/**
 * @brief 获取x86后端版本字符串
 * 
 * 返回x86后端的版本字符串。
 * 
 * @return 版本字符串，格式为"主版本号.次版本号.修订号"
 * 
 * @note 返回的字符串是静态常量，调用者不应修改或释放。
 */
const char* F_get_x86_backend_version_string(void);

/**
 * @brief 检查x86后端是否支持特定功能
 * 
 * 检查x86后端是否支持特定的代码生成功能。
 * 
 * @param feature 功能标识符
 * @return 支持返回true，不支持返回false
 * 
 * @note 功能标识符可以是以下值：
 *       - "x86-32": 支持x86 32位架构
 *       - "x86-64": 支持x86-64 64位架构
 *       - "sse": 支持SSE指令集
 *       - "avx": 支持AVX指令集
 *       - "avx512": 支持AVX-512指令集
 *       - "att-syntax": 支持AT&T汇编语法
 *       - "intel-syntax": 支持Intel汇编语法
 */
bool F_x86_backend_supports_feature(const char* feature);

/**
 * @brief 检查是否支持特定指令集扩展
 * 
 * 检查x86后端是否支持特定的指令集扩展。
 * 
 * @param extension 指令集扩展
 * @return 支持返回true，不支持返回false
 */
bool F_x86_backend_supports_extension(Eum_X86Extension extension);

/**
 * @brief 创建默认x86后端配置
 * 
 * 创建并返回默认的x86后端配置。
 * 
 * @return Stru_X86BackendConfig_t 默认x86后端配置
 */
Stru_X86BackendConfig_t F_create_default_x86_backend_config(void);

/**
 * @brief 配置x86后端选项
 * 
 * 配置x86后端的代码生成选项。
 * 
 * @param option 选项名称
 * @param value 选项值
 * @return 配置成功返回true，失败返回false
 * 
 * @note 支持的选项：
 *       - "architecture": 架构 ("x86", "x86-64")
 *       - "optimization_level": 优化级别 (0-3)
 *       - "debug_info": 是否生成调试信息 (true/false)
 *       - "assembler_syntax": 汇编语法 ("att", "intel")
 */
bool F_configure_x86_backend(const char* option, const char* value);

/**
 * @brief 创建x86后端代码生成器接口
 * 
 * 创建并返回一个x86后端特定的代码生成器接口实例。
 * 
 * @return Stru_CodeGeneratorInterface_t* x86后端代码生成器接口实例
 * 
 * @note 此函数创建专门用于x86汇编代码生成的接口实例，
 *       与通用的F_create_codegen_interface()函数不同，
 *       此函数创建的接口已经预配置为x86后端。
 */
Stru_CodeGeneratorInterface_t* F_create_x86_backend_interface(void);

/**
 * @brief 创建x86后端目标代码生成器接口
 * 
 * 创建并返回一个x86后端特定的目标代码生成器接口实例。
 * 
 * @return Stru_TargetCodeGeneratorInterface_t* x86后端目标代码生成器接口实例
 * 
 * @note 此函数创建专门用于从x86汇编代码生成目标代码的接口实例。
 */
Stru_TargetCodeGeneratorInterface_t* F_create_x86_target_codegen_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_X86_BACKEND_H */
