/**
 * @file CN_codegen_interface.h
 * @brief CN_Language 代码生成器抽象接口
 * 
 * 定义代码生成器的抽象接口，支持将抽象语法树转换为目标代码。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#ifndef CN_CODEGEN_INTERFACE_H
#define CN_CODEGEN_INTERFACE_H

#include "../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../ast/CN_ast.h"

/**
 * @brief 目标代码类型枚举
 * 
 * 定义支持的目标代码类型。
 */
typedef enum Eum_TargetCodeType {
    Eum_TARGET_C,           ///< C语言代码
    Eum_TARGET_LLVM_IR,     ///< LLVM中间表示
    Eum_TARGET_X86_64,      ///< x86-64汇编
    Eum_TARGET_ARM64,       ///< ARM64汇编
    Eum_TARGET_WASM,        ///< WebAssembly
    Eum_TARGET_BYTECODE     ///< 自定义字节码
} Eum_TargetCodeType;

/**
 * @brief 代码生成选项结构体
 * 
 * 定义代码生成的配置选项。
 */
typedef struct Stru_CodeGenOptions_t {
    Eum_TargetCodeType target_type;     ///< 目标代码类型
    bool optimize;                      ///< 是否启用优化
    int optimization_level;             ///< 优化级别 (0-3)
    bool debug_info;                    ///< 是否生成调试信息
    const char* output_file;            ///< 输出文件名（可选）
    bool verbose;                       ///< 是否输出详细信息
} Stru_CodeGenOptions_t;

/**
 * @brief 代码生成结果结构体
 * 
 * 包含代码生成的结果和状态信息。
 */
typedef struct Stru_CodeGenResult_t {
    bool success;                       ///< 是否成功
    const char* code;                   ///< 生成的代码字符串
    size_t code_length;                 ///< 代码长度
    Stru_DynamicArray_t* errors;        ///< 错误信息数组
    Stru_DynamicArray_t* warnings;      ///< 警告信息数组
    size_t instruction_count;           ///< 生成的指令数量
    size_t memory_usage;                ///< 内存使用量（字节）
} Stru_CodeGenResult_t;

/**
 * @brief 代码生成器抽象接口结构体
 * 
 * 定义代码生成器的完整接口，包含初始化、代码生成、优化和资源管理功能。
 * 所有函数指针必须由具体实现提供。
 */
typedef struct Stru_CodeGeneratorInterface_t {
    // ============================================
    // 初始化函数
    // ============================================
    
    /**
     * @brief 初始化代码生成器
     * 
     * 设置代码生成选项，准备进行代码生成。
     * 
     * @param interface 代码生成器接口指针
     * @param options 代码生成选项
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool (*initialize)(struct Stru_CodeGeneratorInterface_t* interface, const Stru_CodeGenOptions_t* options);
    
    // ============================================
    // 核心功能
    // ============================================
    
    /**
     * @brief 生成代码
     * 
     * 从抽象语法树生成目标代码。
     * 
     * @param interface 代码生成器接口指针
     * @param ast 抽象语法树根节点
     * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
     */
    Stru_CodeGenResult_t* (*generate_code)(struct Stru_CodeGeneratorInterface_t* interface, Stru_AstNode_t* ast);
    
    /**
     * @brief 生成模块代码
     * 
     * 从多个AST节点生成模块级别的代码。
     * 
     * @param interface 代码生成器接口指针
     * @param modules AST节点数组
     * @param module_count 模块数量
     * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
     */
    Stru_CodeGenResult_t* (*generate_module_code)(struct Stru_CodeGeneratorInterface_t* interface, 
                                                  Stru_AstNode_t** modules, size_t module_count);
    
    // ============================================
    // 优化功能
    // ============================================
    
    /**
     * @brief 应用优化
     * 
     * 对生成的代码应用优化。
     * 
     * @param interface 代码生成器接口指针
     * @param code 要优化的代码
     * @param optimization_level 优化级别
     * @return Stru_CodeGenResult_t* 优化后的代码结果
     */
    Stru_CodeGenResult_t* (*optimize)(struct Stru_CodeGeneratorInterface_t* interface, 
                                      const Stru_CodeGenResult_t* code, int optimization_level);
    
    /**
     * @brief 注册优化器插件
     * 
     * 注册自定义优化器插件。
     * 
     * @param interface 代码生成器接口指针
     * @param plugin_name 插件名称
     * @param plugin_func 插件函数指针
     * @return true 注册成功
     * @return false 注册失败
     */
    bool (*register_optimizer_plugin)(struct Stru_CodeGeneratorInterface_t* interface,
                                      const char* plugin_name, void* plugin_func);
    
    // ============================================
    // 目标代码生成
    // ============================================
    
    /**
     * @brief 设置目标平台
     * 
     * 设置目标代码生成的平台和架构。
     * 
     * @param interface 代码生成器接口指针
     * @param target_type 目标代码类型
     * @param platform 平台名称
     * @param architecture 架构名称
     * @return true 设置成功
     * @return false 设置失败
     */
    bool (*set_target)(struct Stru_CodeGeneratorInterface_t* interface,
                       Eum_TargetCodeType target_type,
                       const char* platform, const char* architecture);
    
    /**
     * @brief 生成目标文件
     * 
     * 将生成的代码写入目标文件。
     * 
     * @param interface 代码生成器接口指针
     * @param code 生成的代码
     * @param output_file 输出文件路径
     * @return true 生成成功
     * @return false 生成失败
     */
    bool (*generate_target_file)(struct Stru_CodeGeneratorInterface_t* interface,
                                 const Stru_CodeGenResult_t* code, const char* output_file);
    
    // ============================================
    // 状态查询
    // ============================================
    
    /**
     * @brief 获取当前目标类型
     * 
     * 获取当前设置的目标代码类型。
     * 
     * @param interface 代码生成器接口指针
     * @return Eum_TargetCodeType 目标代码类型
     */
    Eum_TargetCodeType (*get_target_type)(struct Stru_CodeGeneratorInterface_t* interface);
    
    /**
     * @brief 获取支持的优化级别
     * 
     * 获取代码生成器支持的优化级别范围。
     * 
     * @param interface 代码生成器接口指针
     * @param min_level 输出参数：最小优化级别
     * @param max_level 输出参数：最大优化级别
     */
    void (*get_supported_optimization_levels)(struct Stru_CodeGeneratorInterface_t* interface,
                                              int* min_level, int* max_level);
    
    /**
     * @brief 获取已注册的优化器插件
     * 
     * 获取所有已注册的优化器插件名称。
     * 
     * @param interface 代码生成器接口指针
     * @return Stru_DynamicArray_t* 插件名称数组
     */
    Stru_DynamicArray_t* (*get_registered_plugins)(struct Stru_CodeGeneratorInterface_t* interface);
    
    // ============================================
    // 错误处理
    // ============================================
    
    /**
     * @brief 检查是否有错误
     * 
     * 检查代码生成过程中是否发生了错误。
     * 
     * @param interface 代码生成器接口指针
     * @return true 有错误
     * @return false 没有错误
     */
    bool (*has_errors)(struct Stru_CodeGeneratorInterface_t* interface);
    
    /**
     * @brief 获取错误信息
     * 
     * 获取所有错误信息的数组。
     * 
     * @param interface 代码生成器接口指针
     * @return Stru_DynamicArray_t* 错误信息数组
     */
    Stru_DynamicArray_t* (*get_errors)(struct Stru_CodeGeneratorInterface_t* interface);
    
    /**
     * @brief 清除错误
     * 
     * 清除所有错误信息。
     * 
     * @param interface 代码生成器接口指针
     */
    void (*clear_errors)(struct Stru_CodeGeneratorInterface_t* interface);
    
    // ============================================
    // 资源管理
    // ============================================
    
    /**
     * @brief 重置代码生成器
     * 
     * 重置代码生成器到初始状态。
     * 
     * @param interface 代码生成器接口指针
     */
    void (*reset)(struct Stru_CodeGeneratorInterface_t* interface);
    
    /**
     * @brief 销毁代码生成器
     * 
     * 释放代码生成器占用的所有资源。
     * 
     * @param interface 代码生成器接口指针
     */
    void (*destroy)(struct Stru_CodeGeneratorInterface_t* interface);
    
    // ============================================
    // 内部状态（不直接暴露）
    // ============================================
    
    /**
     * @brief 内部状态指针
     * 
     * 具体实现可以使用此指针存储内部状态。
     * 接口使用者不应直接访问此字段。
     */
    void* internal_state;
    
} Stru_CodeGeneratorInterface_t;

// ============================================
// 工厂函数
// ============================================

/**
 * @brief 创建代码生成器接口实例
 * 
 * 创建并返回一个新的代码生成器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_CodeGeneratorInterface_t* 新创建的代码生成器接口实例
 */
Stru_CodeGeneratorInterface_t* F_create_codegen_interface(void);

/**
 * @brief 创建默认代码生成选项
 * 
 * 创建并返回默认的代码生成选项。
 * 
 * @return Stru_CodeGenOptions_t 默认代码生成选项
 */
Stru_CodeGenOptions_t F_create_default_codegen_options(void);

/**
 * @brief 销毁代码生成结果
 * 
 * 释放代码生成结果占用的所有资源。
 * 
 * @param result 要销毁的代码生成结果
 */
void F_destroy_codegen_result(Stru_CodeGenResult_t* result);

#endif // CN_CODEGEN_INTERFACE_H
