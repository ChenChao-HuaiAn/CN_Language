/******************************************************************************
 * 文件名: CN_llvm_backend.h
 * 功能: CN_Language LLVM IR后端接口
 * 
 * 定义LLVM IR后端的公共接口，提供将CN_Language抽象语法树或中间表示转换为LLVM IR的功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LLVM_BACKEND_H
#define CN_LLVM_BACKEND_H

#include "../../CN_codegen_interface.h"
#include "../../CN_target_codegen_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * 类型定义
 * ============================================ */

/**
 * @brief LLVM后端配置结构体
 * 
 * 定义LLVM IR后端的配置选项。
 */
typedef struct Stru_LLVMBackendConfig_t {
    const char* target_triple;          ///< 目标三元组（如 "x86_64-pc-linux-gnu"）
    const char* cpu;                    ///< CPU型号（如 "x86-64"）
    const char* features;               ///< CPU特性字符串
    int optimization_level;             ///< 优化级别（0-3）
    bool debug_info;                    ///< 是否生成调试信息
    bool position_independent;          ///< 是否生成位置无关代码
    bool stack_protector;               ///< 是否启用栈保护
    bool omit_frame_pointer;            ///< 是否省略帧指针
    bool verify_module;                 ///< 是否验证LLVM模块
    bool print_module;                  ///< 是否打印LLVM模块
    const char* output_format;          ///< 输出格式（"ir", "bc", "asm", "obj"）
} Stru_LLVMBackendConfig_t;

/**
 * @brief LLVM后端状态结构体
 * 
 * 存储LLVM后端的内部状态。
 */
typedef struct Stru_LLVMBackendState_t {
    void* llvm_context;                 ///< LLVM上下文
    void* llvm_module;                  ///< LLVM模块
    void* llvm_builder;                 ///< LLVM IR构建器
    void* llvm_pass_manager;            ///< LLVM优化管理器
    Stru_LLVMBackendConfig_t config;    ///< 当前配置
    Stru_DynamicArray_t* errors;        ///< 错误信息数组
    Stru_DynamicArray_t* warnings;      ///< 警告信息数组
    bool initialized;                   ///< 是否已初始化
} Stru_LLVMBackendState_t;

/* ============================================
 * 公共接口函数
 * ============================================ */

/**
 * @brief 从AST生成LLVM IR
 * 
 * 将CN_Language的抽象语法树转换为LLVM IR。
 * 
 * @param ast 抽象语法树根节点
 * @param config LLVM后端配置（可为NULL，使用默认配置）
 * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
 * 
 * @note 此函数是LLVM后端的主要入口点，负责协调整个LLVM IR生成过程。
 *       生成的LLVM IR可以直接用LLVM工具链优化和编译。
 */
Stru_CodeGenResult_t* F_generate_llvm_ir_from_ast(Stru_AstNode_t* ast, const Stru_LLVMBackendConfig_t* config);

/**
 * @brief 从IR生成LLVM IR
 * 
 * 将CN_Language的中间表示转换为LLVM IR。
 * 
 * @param ir 中间表示数据
 * @param ir_size 中间表示大小
 * @param config LLVM后端配置（可为NULL，使用默认配置）
 * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
 */
Stru_CodeGenResult_t* F_generate_llvm_ir_from_ir(const void* ir, size_t ir_size, const Stru_LLVMBackendConfig_t* config);

/**
 * @brief 优化LLVM IR
 * 
 * 对LLVM IR应用优化。
 * 
 * @param llvm_ir LLVM IR字符串
 * @param optimization_level 优化级别（0-3）
 * @return char* 优化后的LLVM IR字符串，调用者负责释放
 */
char* F_optimize_llvm_ir(const char* llvm_ir, int optimization_level);

/**
 * @brief 将LLVM IR编译为目标代码
 * 
 * 将LLVM IR编译为特定目标平台的汇编代码或目标文件。
 * 
 * @param llvm_ir LLVM IR字符串
 * @param target_config 目标配置
 * @return Stru_TargetCodeGenResult_t* 目标代码生成结果，调用者负责销毁
 */
Stru_TargetCodeGenResult_t* F_compile_llvm_ir_to_target(const char* llvm_ir, const Stru_TargetConfig_t* target_config);

/**
 * @brief 获取LLVM后端版本信息
 * 
 * 返回LLVM后端的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 * 
 * @note 版本号遵循语义化版本规范。
 */
void F_get_llvm_backend_version(int* major, int* minor, int* patch);

/**
 * @brief 获取LLVM后端版本字符串
 * 
 * 返回LLVM后端的版本字符串。
 * 
 * @return 版本字符串，格式为"主版本号.次版本号.修订号"
 * 
 * @note 返回的字符串是静态常量，调用者不应修改或释放。
 */
const char* F_get_llvm_backend_version_string(void);

/**
 * @brief 检查LLVM后端是否支持特定功能
 * 
 * 检查LLVM后端是否支持特定的代码生成功能。
 * 
 * @param feature 功能标识符
 * @return 支持返回true，不支持返回false
 * 
 * @note 功能标识符可以是以下值：
 *       - "llvm-16": 支持LLVM 16
 *       - "llvm-17": 支持LLVM 17
 *       - "llvm-18": 支持LLVM 18
 *       - "jit": 支持即时编译
 *       - "link-time-opt": 支持链接时优化
 *       - "profile-guided-opt": 支持基于性能分析的优化
 */
bool F_llvm_backend_supports_feature(const char* feature);

/**
 * @brief 创建默认LLVM后端配置
 * 
 * 创建并返回默认的LLVM后端配置。
 * 
 * @return Stru_LLVMBackendConfig_t 默认LLVM后端配置
 */
Stru_LLVMBackendConfig_t F_create_default_llvm_backend_config(void);

/**
 * @brief 配置LLVM后端选项
 * 
 * 配置LLVM后端的代码生成选项。
 * 
 * @param option 选项名称
 * @param value 选项值
 * @return 配置成功返回true，失败返回false
 * 
 * @note 支持的选项：
 *       - "target_triple": 目标三元组
 *       - "optimization_level": 优化级别 (0-3)
 *       - "debug_info": 是否生成调试信息 (true/false)
 *       - "position_independent": 是否生成位置无关代码 (true/false)
 */
bool F_configure_llvm_backend(const char* option, const char* value);

/**
 * @brief 创建LLVM后端代码生成器接口
 * 
 * 创建并返回一个LLVM后端特定的代码生成器接口实例。
 * 
 * @return Stru_CodeGeneratorInterface_t* LLVM后端代码生成器接口实例
 * 
 * @note 此函数创建专门用于LLVM IR代码生成的接口实例，
 *       与通用的F_create_codegen_interface()函数不同，
 *       此函数创建的接口已经预配置为LLVM后端。
 */
Stru_CodeGeneratorInterface_t* F_create_llvm_backend_interface(void);

/**
 * @brief 创建LLVM后端目标代码生成器接口
 * 
 * 创建并返回一个LLVM后端特定的目标代码生成器接口实例。
 * 
 * @return Stru_TargetCodeGeneratorInterface_t* LLVM后端目标代码生成器接口实例
 * 
 * @note 此函数创建专门用于从LLVM IR生成目标代码的接口实例。
 */
Stru_TargetCodeGeneratorInterface_t* F_create_llvm_target_codegen_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_LLVM_BACKEND_H */
